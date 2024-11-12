#include <Windows.h>
#include <comdef.h>
#include <cstdio>

#include <fileapi.h>
#include <glad/gl.h>
#include <glad/wgl.h>

#include <cassert>
#include <platform/platform.h>
#include <platform/types.h>
#include <sysinfoapi.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <xaudio2.h>

constexpr WORD BITSPERSSAMPLE = 16;          // 16 bits per sample.
constexpr DWORD SAMPLESPERSEC = 44100;       // 44,100 samples per second.
constexpr double CYCLESPERSEC = 220.0;       // 220 cycles per second (frequency of the audible tone).
constexpr double VOLUME = 0.5;               // 50% volume.
constexpr WORD AUDIOBUFFERSIZEINCYCLES = 10; // 10 cycles per audio buffer.
constexpr double PI2 = 3.14159265358979323846;

// Calculated constants.
constexpr DWORD SAMPLESPERCYCLE = static_cast<DWORD>(SAMPLESPERSEC / CYCLESPERSEC);      // 200 samples per cycle.
constexpr DWORD AUDIOBUFFERSIZEINSAMPLES = SAMPLESPERCYCLE * AUDIOBUFFERSIZEINCYCLES;    // 2,000 samples per buffer.
constexpr UINT32 AUDIOBUFFERSIZEINBYTES = AUDIOBUFFERSIZEINSAMPLES * BITSPERSSAMPLE / 8; // 4,000 bytes per buffer.

struct SoundDataBuffer {
  u8* data;
  i32 size;
  i32 max_size;
};
struct Audio {
  // Xaudio stuff
  IXAudio2* xAudio2;
  IXAudio2MasteringVoice* masteringVoice;
  IXAudio2SourceVoice* sourceVoice;
  XAUDIO2_BUFFER buffer_desc[2];

  SoundDataBuffer buffer[2];
  i32 active_buffer;

  /*audio.sourceVoice->Stop();*/
  /*audio.sourceVoice->DestroyVoice();*/
  /*audio.masteringVoice->DestroyVoice();*/
  /*audio.xAudio2->Release();*/
};

auto win32_init_audio(Audio& audio) -> bool {
  HRESULT result;
  result = XAudio2Create(&audio.xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
  if (FAILED(result)) {
    DWORD err_code = GetLastError();
    printf("win32_init_audio::XAudio2Create failed: %lu", err_code);
    return false;
  }

  if (FAILED(audio.xAudio2->CreateMasteringVoice(&audio.masteringVoice))) {
    DWORD err_code = GetLastError();
    printf("win32_init_audio::XAudio2CreateMasteringVoice failed: %lu", err_code);
    return false;
  }

  auto num_channels = 1;
  auto sample_size = sizeof(u16);
  auto samples_per_second = 44100;
  auto buffer_size = 44100 * sample_size * num_channels;
  for (auto i = 0; i < 2; i++) {
    audio.buffer[i].max_size = buffer_size;
    audio.buffer[i].size = 0;
    audio.buffer[i].data = (u8*)VirtualAlloc(nullptr, // TODO: Might want to set this
        (SIZE_T)buffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  }

  // https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/ns-mmeapi-waveformatex
  WAVEFORMATEX waveFormatEx{};
  waveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
  waveFormatEx.nChannels = 1; // 1 channel
  waveFormatEx.nSamplesPerSec = samples_per_second;
  waveFormatEx.nBlockAlign = SoundSampleSize * waveFormatEx.nChannels;
  waveFormatEx.nAvgBytesPerSec = waveFormatEx.nSamplesPerSec * waveFormatEx.nBlockAlign;
  waveFormatEx.wBitsPerSample = SoundSampleSize * 8;
  waveFormatEx.cbSize = 0;

  // Create a source voice.
  if (FAILED(audio.xAudio2->CreateSourceVoice(&audio.sourceVoice, &waveFormatEx))) {
    DWORD err_code = GetLastError();
    printf("win32_play_sound::CreateSourceVoice failed: %lu", err_code);
    return false;
  }
  return true;
}

auto win32_play_sound(Audio& audio, SoundBuffer& src_buffer) -> bool {
  audio.active_buffer = audio.active_buffer == 0 ? 1 : 0;
  auto& target_buffer = audio.buffer[audio.active_buffer];
  target_buffer.size = 0;
  assert(target_buffer.max_size <= src_buffer.num_samples * SoundSampleSize);
  for (auto i = 0; i < src_buffer.num_samples; i++) {
    int16_t sample = src_buffer.samples[i];
    target_buffer.data[target_buffer.size++] = static_cast<BYTE>(sample);      // Lower byte
    target_buffer.data[target_buffer.size++] = static_cast<BYTE>(sample >> 8); // Upper byte
  }

  /*Indicates that there cannot be any buffers in the queue after this buffer.
   * The only effect of this flag is to suppress debug output warnings caused
   * by starvation of the buffer queue. */
  XAUDIO2_BUFFER buffer_desc = {};
  buffer_desc.Flags = 0;
  buffer_desc.AudioBytes = target_buffer.size;
  buffer_desc.pAudioData = target_buffer.data;
  // Zero means start at the begining and play until end
  buffer_desc.PlayBegin = 0;
  buffer_desc.PlayLength = 0;
  buffer_desc.LoopBegin = 0;
  buffer_desc.LoopLength = 0;
  buffer_desc.LoopCount = 0;

  // Submit the buffer and start playback.
  if (FAILED(audio.sourceVoice->SubmitSourceBuffer(&buffer_desc))) {
    DWORD err_code = GetLastError();
    printf("win32_init_audio::SubmitSourceBuffer failed: %lu", err_code);
    return false;
  }

  if (FAILED(audio.sourceVoice->Start(0))) {
    DWORD err_code = GetLastError();
    printf("win32_init_audio::Stat: %lu", err_code);
    return false;
  }
  return true;
}

struct EngineFunctions {
  HMODULE handle = nullptr;
  UPDATE_AND_RENDER_PROC update_and_render = nullptr;
  LOAD_PROC load = nullptr;
  GET_SOUND_SAMPLES_PROC get_sound_samples = nullptr;
  FILETIME last_loaded_dll_write_time = { 0, 0 };
};

u64 win32_file_time_to_u64(const FILETIME& ft) {
  ULARGE_INTEGER uli;
  uli.LowPart = ft.dwLowDateTime;
  uli.HighPart = ft.dwHighDateTime;
  return uli.QuadPart;
}

u64 win32_file_size(const char* path) {
  WIN32_FILE_ATTRIBUTE_DATA file_info;
  if (!GetFileAttributesExA(path, GetFileExInfoStandard, &file_info)) {
    printf("PLATFORM: (win32_file_size): unable to get file attributes for: '%s'.\n", path);
    return 0;
  }

  ULARGE_INTEGER size;
  size.HighPart = file_info.nFileSizeHigh;
  size.LowPart = file_info.nFileSizeLow;
  return size.QuadPart;
}

/// \param path Path to the file
/// \param read_buffer Buffer to put the content of the file into
/// \param buffer_size Number of bytes one wish to read, but remember to add one for the zero terminating character.
/// \return Success
bool win32_read_text_file(const char* path, char* read_buffer, const u64 buffer_size) {
  HANDLE file_handle;
  file_handle = CreateFileA(path, // file to open
      GENERIC_READ,               // open for reading
      FILE_SHARE_READ,            // share for reading
      nullptr,                    // default security
      OPEN_EXISTING,              // existing file only
      FILE_ATTRIBUTE_NORMAL,      // normal file
      nullptr);                   // no attr. template

  if (file_handle == INVALID_HANDLE_VALUE) {
    DWORD error = GetLastError();
    printf("Failed to open file %s. Error code: %lu\n", path, error);
    return false;
  }

  // Read one character less than the buffer size to save room for
  // the terminating NULL character.
  DWORD bytes_read = 0;
  if (!ReadFile(file_handle, read_buffer, buffer_size - 1, &bytes_read, nullptr)) {
    printf("Unable to read from file: %s\nGetLastError=%lu\n", path, GetLastError());
    CloseHandle(file_handle);
    return false;
  }

  assert(bytes_read == buffer_size - 1);
  read_buffer[buffer_size - 1] = '\0';
  CloseHandle(file_handle);
  return true;
}

bool win32_write_file(const char* path, const char* data, const u64 data_size) {
  HANDLE file_handle;
  file_handle = CreateFileA(path, // file to open
      GENERIC_WRITE,              // open for writing
      0,                          // Do not share
      nullptr,                    // default security
      CREATE_ALWAYS,              // overwrite existing
      FILE_ATTRIBUTE_NORMAL,      // normal file
      nullptr);                   // no attr. template

  if (file_handle == INVALID_HANDLE_VALUE) {
    DWORD error = GetLastError();
    printf("Failed to open file %s for writing. Error code: %lu\n", path, error);
    return false;
  }

  DWORD bytes_written = 0;
  if (!WriteFile(file_handle, data, data_size, &bytes_written, nullptr)) {
    printf("Unable to write to file: %s\nGetLastError=%lu\n", path, GetLastError());
    CloseHandle(file_handle);
    return false;
  }

  assert(bytes_written == data_size);
  CloseHandle(file_handle);
  return true;
}

void win32_debug_print_readable_timestamp(u64 timestamp) {
  FILETIME file_time;
  file_time.dwLowDateTime = timestamp & 0xFFFFFFFF;
  file_time.dwHighDateTime = timestamp >> 32;

  SYSTEMTIME system_time;
  FILETIME local_file_time;

  FileTimeToLocalFileTime(&file_time, &local_file_time);
  FileTimeToSystemTime(&local_file_time, &system_time);

  printf("%02d:%02d %02d/%02d/%04d", system_time.wHour, system_time.wMinute, system_time.wDay, system_time.wMonth,
      system_time.wYear);
}

u64 win32_file_last_modified(const char* path) {
  WIN32_FILE_ATTRIBUTE_DATA file_info;
  if (GetFileAttributesExA(path, GetFileExInfoStandard, &file_info)) {
    return win32_file_time_to_u64(file_info.ftLastWriteTime);
  } else {
    printf("PLATFORM (win32_file_last_modified): unable to get file attributes for: '%s'.\n", path);
    return 0;
  }
}

struct Recording {
  u64 num_frames_recorded;
  u64 current_playback_frame;
  HANDLE frame_input_handle;
};

struct Playback {
  u64 num_frames_recorded;
  u64 current_playback_frame;
  EngineInput* input;
  void* permanent_memory;
  void* asset_memory;
};

const char Permanent_Memory_Block_Recording_File[] = "permanent_memory_block_recording.dat";
const char Asset_Memory_Block_Recording_File[] = "asset_memory_block_recording.dat";
const char User_Input_Recording_File[] = "user_input_recording.dat";

bool win32_overwrite_file(const char* path, void* memory, size_t size) {
  HANDLE handle = CreateFile(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (handle == INVALID_HANDLE_VALUE) {
    DWORD error = GetLastError();
    printf("[ERROR]: overwrite_file failed when creating file '%s'.\nError code: %lu.\n", path, error);
    return false;
  }

  DWORD bytes_written;
  auto is_success = WriteFile(handle, memory, size, &bytes_written, nullptr);
  assert(size == bytes_written);
  CloseHandle(handle);
  if (!is_success) {
    DWORD error = GetLastError();
    printf("[ERROR]: overwrite_file failed when attempting to write to file '%s'.\nError code: %lu.\n", path, error);
    return false;
  }
  return true;
}

bool win32_read_binary_file(const char* path, void* destination_buffer, const u64 buffer_size) {
  HANDLE file_handle;
  file_handle = CreateFileA(path, // file to open
      GENERIC_READ,               // open for reading
      FILE_SHARE_READ,            // share for reading
      nullptr,                    // default security
      OPEN_EXISTING,              // existing file only
      FILE_ATTRIBUTE_NORMAL,      // normal file
      nullptr);                   // no attr. template

  if (file_handle == INVALID_HANDLE_VALUE) {
    DWORD error = GetLastError();
    printf("Failed to open file %s. Error code: %lu\n", path, error);
    return false;
  }

  // Read one character less than the buffer size to save room for
  // the terminating NULL character.
  DWORD bytes_read = 0;
  if (!ReadFile(file_handle, destination_buffer, buffer_size, &bytes_read, nullptr)) {
    printf("[ERROR] win32_read_binary file: Unable to read file '%s'.\nWindows error code=%lu\n", path, GetLastError());
    CloseHandle(file_handle);
    return false;
  }

  assert(bytes_read == buffer_size);
  CloseHandle(file_handle);
  return true;
}

bool win32_start_recording(EngineMemory* memory, Recording& recording) {
  if (!win32_overwrite_file(Permanent_Memory_Block_Recording_File, memory->permanent, Permanent_Memory_Block_Size)) {
    printf("[ERROR]: win32_start_recording: Unable to write permanent memory.\n");
    return false;
  }

  if (!win32_overwrite_file(Asset_Memory_Block_Recording_File, memory->asset, Assets_Memory_Block_Size)) {
    printf("[ERROR]: win32_start_recording: Unable to write asset memory.\n");
    return false;
  }

  HANDLE user_input_record_file =
      CreateFile(User_Input_Recording_File, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (user_input_record_file == INVALID_HANDLE_VALUE) {
    printf("[ERROR]: win32_start_recording: Failed creating file 'recorded_input'.\n");
    return false;
  }

  recording.frame_input_handle = user_input_record_file;
  recording.current_playback_frame = 0;
  recording.num_frames_recorded = 0;

  return true;
}

void win32_stop_recording(Recording& recording) {
  CloseHandle(recording.frame_input_handle);
  recording.frame_input_handle = INVALID_HANDLE_VALUE;
}

bool win32_record_frame_input(EngineInput* input, Recording& recording) {
  DWORD pos = SetFilePointer(recording.frame_input_handle, 0, nullptr, FILE_END);
  if (pos == INVALID_SET_FILE_POINTER) {
    printf("[ERROR]: record_frame_input failed: Failed to set file pointer to end of file 'recorded_input'.\n");
    return false;
  }

  DWORD bytes_written;
  auto is_success = WriteFile(recording.frame_input_handle, input, sizeof(EngineInput), &bytes_written, nullptr);

  if (!is_success) {
    printf("[ERROR]: record_frame_input failed: Unable to write to 'recorded_input'.\n");
    return false;
  }
  recording.num_frames_recorded++;
  return true;
}

bool win32_init_playback(Playback& playback) {
  playback.permanent_memory = VirtualAlloc(nullptr, // TODO: Might want to set this
      (SIZE_T)Permanent_Memory_Block_Size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!win32_read_binary_file(Permanent_Memory_Block_Recording_File, playback.permanent_memory, Permanent_Memory_Block_Size)) {
    printf("[ERROR]: win32_init_playback: Failed to read permanent memory block recording file.\n");
    VirtualFree(playback.permanent_memory, 0, MEM_RELEASE);
    return false;
  }

  playback.asset_memory = VirtualAlloc(nullptr, (SIZE_T)Assets_Memory_Block_Size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!win32_read_binary_file(Asset_Memory_Block_Recording_File, playback.asset_memory, Assets_Memory_Block_Size)) {
    printf("[ERROR]: win32_init_playback: Failed to read asset memory block recording file.\n");
    VirtualFree(playback.permanent_memory, 0, MEM_RELEASE);
    VirtualFree(playback.asset_memory, 0, MEM_RELEASE);
    return false;
  }

  auto input_size = win32_file_size(User_Input_Recording_File);
  playback.input = (EngineInput*)VirtualAlloc(nullptr, input_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!win32_read_binary_file(User_Input_Recording_File, playback.input, input_size)) {
    printf("[ERROR]: win32_init_playback: Failed to read user input recording file.\n");
    VirtualFree(playback.permanent_memory, 0, MEM_RELEASE);
    VirtualFree(playback.asset_memory, 0, MEM_RELEASE);
    VirtualFree(playback.input, 0, MEM_RELEASE);
    return false;
  }

  assert(input_size % sizeof(EngineInput) == 0);
  playback.num_frames_recorded = input_size / sizeof(EngineInput);
  playback.current_playback_frame = 0;
  return true;
}

void win32_stop_playback(Playback& playback) {
  VirtualFree(playback.permanent_memory, 0, MEM_RELEASE);
  VirtualFree(playback.asset_memory, 0, MEM_RELEASE);
  VirtualFree(playback.input, 0, MEM_RELEASE);
  playback.permanent_memory = nullptr;
  playback.asset_memory = nullptr;
  playback.input = nullptr;
  playback.num_frames_recorded = 0;
  playback.current_playback_frame = 0;
}

LPCTSTR dll_path = R"(.\bin\app\engine_dyn.dll)";
LPCTSTR pdb_path = R"(.\bin\app\engine_dyn.pdb)";
LPCTSTR versioned_dll_path = R"(.\bin\versions\)";

bool win32_should_reload_dll(EngineFunctions* app_functions) {
  if (app_functions->update_and_render == nullptr) {
    return true;
  }

  WIN32_FILE_ATTRIBUTE_DATA file_info;
  if (GetFileAttributesEx(dll_path, GetFileExInfoStandard, &file_info)) {
    auto result = CompareFileTime(&file_info.ftLastWriteTime, &app_functions->last_loaded_dll_write_time);
    return result > 0;
  } else {
    printf("Unable to open read time of '%s'.\n", dll_path);
    return false;
  }
}

void win32_load_dll(EngineFunctions* functions) {
  if (functions->handle != nullptr) {
    if (!FreeLibrary(functions->handle)) {
      DWORD error = GetLastError();
      printf("Failed to unload .dll. Error code: %lu. Exiting...\n", error);
      exit(1);
    }
    functions->handle = nullptr;
  }

  SYSTEMTIME curr_time;
  GetSystemTime(&curr_time);
  char timestamp[20];
  sprintf(timestamp, "%04d%02d%02d%02d%02d%02d", curr_time.wYear, curr_time.wMonth, curr_time.wDay, curr_time.wHour,
      curr_time.wMinute, curr_time.wSecond);

  char dir_path[128];
  sprintf(dir_path, "%s%s", versioned_dll_path, timestamp);

  if (!CreateDirectory(dir_path, nullptr)) {
    DWORD error = GetLastError();
    printf("Failed to create directory %s. Error code: %lu\n", dir_path, error);
    fprintf(stderr, "Test\n");
    fflush(stderr);
    fflush(stdout);
    exit(1);
  }

  char dll_to_load_path[128];
  sprintf(dll_to_load_path, "%s\\%s", dir_path, "engine_dyn.dll");
  while (!CopyFile(dll_path, dll_to_load_path, FALSE)) {
    DWORD error = GetLastError();
    printf("Failed to copy %s to %s. Error code: %lu\n", dll_path, dll_to_load_path, error);
    // exit(1);
    // TODO: Make this more airtight
  }

  char pdb_to_load_path[128];
  sprintf(pdb_to_load_path, "%s\\%s", dir_path, "engine_dyn.pdb");
  if (!CopyFile(pdb_path, pdb_to_load_path, FALSE)) {
    DWORD error = GetLastError();
    printf("Failed to copy %s to %s. Error code: %lu\n", pdb_path, pdb_to_load_path, error);
    exit(1);
  }

  functions->handle = LoadLibrary(dll_to_load_path);
  if (functions->handle == nullptr) {
    DWORD error = GetLastError();
    printf("Unable to load %s. Error: %lu\n", dll_to_load_path, error);
    functions->update_and_render = nullptr;
    return;
  }

  functions->update_and_render = (UPDATE_AND_RENDER_PROC)GetProcAddress(functions->handle, "update_and_render");
  if (functions->update_and_render == nullptr) {
    printf("Unable to load 'update_and_render' function in engine_dyn.dll\n");
    // TODO: Why do I free the handle here?
    FreeLibrary(functions->handle);
  }

  functions->load = (LOAD_PROC)GetProcAddress(functions->handle, "load");
  if (functions->load == nullptr) {
    printf("Unable to load 'load' function in engine_dyn.dll\n");
    FreeLibrary(functions->handle);
  }

  functions->get_sound_samples = (GET_SOUND_SAMPLES_PROC)GetProcAddress(functions->handle, "get_sound_samples");
  if (functions->get_sound_samples == nullptr) {
    printf("Unable to load 'get_sound_samples' function in engine_dyn.dll\n");
    FreeLibrary(functions->handle);
  }

  WIN32_FILE_ATTRIBUTE_DATA file_info;
  if (GetFileAttributesEx(dll_path, GetFileExInfoStandard, &file_info)) {
    functions->last_loaded_dll_write_time = file_info.ftLastWriteTime;
  }
}

void win32_process_keyboard_message(ButtonState& new_state, bool is_down) {
  if (new_state.ended_down != is_down) {
    new_state.ended_down = is_down;
    new_state.half_transition_count++;
  }
}

void win32_process_pending_messages(HWND hwnd, bool& is_running, UserInput& new_input, UserInput& old_input) {
  MSG message;

  new_input.mouse_raw.dx = 0;
  new_input.mouse_raw.dy = 0;
  while (PeekMessage(&message, hwnd, 0, 0, PM_REMOVE)) {
    switch (message.message) {
    case WM_QUIT: {
      is_running = false;
    } break;
    case WM_INPUT: {
      UINT dwSize;

      GetRawInputData((HRAWINPUT)message.lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
      auto lpb = new BYTE[dwSize];
      if (GetRawInputData((HRAWINPUT)message.lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
        // Error: couldn't get raw input data
      }

      auto* raw = (RAWINPUT*)lpb;

      if (raw->header.dwType == RIM_TYPEMOUSE) {
        int dx = raw->data.mouse.lLastX;
        int dy = raw->data.mouse.lLastY;
        new_input.mouse_raw.dx = dx;
        new_input.mouse_raw.dy = -dy;

        if (raw->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
          new_input.mouse_raw.buttons[0].ended_down = true;
          new_input.mouse_raw.buttons[0].half_transition_count++;
        }
        if (raw->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
          new_input.mouse_raw.buttons[1].ended_down = true;
          new_input.mouse_raw.buttons[1].half_transition_count++;
        }
        if (raw->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) {
          new_input.mouse_raw.buttons[0].ended_down = false;
          new_input.mouse_raw.buttons[0].half_transition_count++;
        }
        if (raw->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) {
          new_input.mouse_raw.buttons[1].ended_down = false;
          new_input.mouse_raw.buttons[1].half_transition_count++;
        }
      }

      delete[] lpb;
      break;
    }
    case WM_LBUTTONDOWN: {
    } break;
    case WM_LBUTTONUP: {
      break;
    }

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP: {
      u32 vk_code = (u32)message.wParam;
      bool was_down = ((message.lParam & (1 << 30)) != 0);
      bool is_down = ((message.lParam & (1 << 31)) == 0);
      if (was_down != is_down) {
        if (vk_code == 'A') {
          win32_process_keyboard_message(new_input.a, is_down);
        }
        if (vk_code == 'B') {
          win32_process_keyboard_message(new_input.b, is_down);
        }
        if (vk_code == 'C') {
          win32_process_keyboard_message(new_input.c, is_down);
        }
        if (vk_code == 'D') {
          win32_process_keyboard_message(new_input.d, is_down);
        }
        if (vk_code == 'E') {
          win32_process_keyboard_message(new_input.e, is_down);
        }
        if (vk_code == 'F') {
          win32_process_keyboard_message(new_input.f, is_down);
        }
        if (vk_code == 'G') {
          win32_process_keyboard_message(new_input.g, is_down);
        }
        if (vk_code == 'H') {
          win32_process_keyboard_message(new_input.h, is_down);
        }
        if (vk_code == 'I') {
          win32_process_keyboard_message(new_input.i, is_down);
        }
        if (vk_code == 'J') {
          win32_process_keyboard_message(new_input.j, is_down);
        }
        if (vk_code == 'K') {
          win32_process_keyboard_message(new_input.k, is_down);
        }
        if (vk_code == 'L') {
          win32_process_keyboard_message(new_input.l, is_down);
        }
        if (vk_code == 'M') {
          win32_process_keyboard_message(new_input.m, is_down);
        }
        if (vk_code == 'N') {
          win32_process_keyboard_message(new_input.n, is_down);
        }
        if (vk_code == 'O') {
          win32_process_keyboard_message(new_input.o, is_down);
        }
        if (vk_code == 'P') {
          win32_process_keyboard_message(new_input.p, is_down);
        }
        if (vk_code == 'Q') {
          win32_process_keyboard_message(new_input.q, is_down);
        }
        if (vk_code == 'R') {
          win32_process_keyboard_message(new_input.r, is_down);
        }
        if (vk_code == 'S') {
          win32_process_keyboard_message(new_input.s, is_down);
        }
        if (vk_code == 'T') {
          win32_process_keyboard_message(new_input.t, is_down);
        }
        if (vk_code == 'U') {
          win32_process_keyboard_message(new_input.u, is_down);
        }
        if (vk_code == 'V') {
          win32_process_keyboard_message(new_input.v, is_down);
        }
        if (vk_code == 'W') {
          win32_process_keyboard_message(new_input.w, is_down);
        }
        if (vk_code == 'X') {
          win32_process_keyboard_message(new_input.x, is_down);
        }
        if (vk_code == 'Y') {
          win32_process_keyboard_message(new_input.y, is_down);
        }
        if (vk_code == 'Z') {
          win32_process_keyboard_message(new_input.z, is_down);
        }

        if (vk_code == VK_BACK) {
          win32_process_keyboard_message(new_input.back, is_down);
        }
        if (vk_code == VK_RETURN) {
          win32_process_keyboard_message(new_input.enter, is_down);
        }
        if (vk_code == VK_TAB) {
          win32_process_keyboard_message(new_input.tab, is_down);
        }

        if (vk_code == VK_UP) {
        }
        if (vk_code == VK_DOWN) {
        }
        if (vk_code == VK_RIGHT) {
        }
        if (vk_code == VK_LEFT) {
        }
        if (vk_code == VK_ESCAPE) {
          is_running = false;
        }
        if (vk_code == VK_SPACE) {
          win32_process_keyboard_message(new_input.space, is_down);
        }

        if (vk_code == VK_OEM_5) {
          win32_process_keyboard_message(new_input.oem_5, is_down);
        }
      }
    } break;
    default: TranslateMessage(&message); DispatchMessageA(&message);
    }
  }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
  return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

#include <errhandlingapi.h>

#if _DEBUG
#pragma comment(linker, "/subsystem:console")

int main() {
  return WinMain(GetModuleHandle(nullptr), nullptr, GetCommandLineA(), SW_SHOWDEFAULT);
}

#else
#pragma comment(linker, "/subsystem:windows")
#endif

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const GLchar* message, const void* userParam) {

  if (severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_HIGH) {
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
    // fprintf(stderr, "Exiting...\n");
    // exit(1);
  }
}

void win32_bind_gl_funcs(GLFunctions* gl) {
  gl->attach_shader = glAttachShader;
  gl->detach_shader = glDetachShader;
  gl->bind_buffer_base = glBindBufferBase;
  gl->bind_vertex_array = glBindVertexArray;
  gl->clear = glClear;
  gl->clear_color = glClearColor;
  gl->compile_shader = glCompileShader;
  gl->create_buffers = glCreateBuffers;
  gl->create_program = glCreateProgram;
  gl->create_shader = glCreateShader;
  gl->create_vertex_arrays = glCreateVertexArrays;
  gl->delete_buffers = glDeleteBuffers;
  gl->delete_program = glDeleteProgram;
  gl->delete_shader = glDeleteShader;
  gl->delete_vertex_array = glDeleteVertexArrays;
  gl->draw_arrays = glDrawArrays;
  gl->enable = glEnable;
  gl->enable_vertex_array_attrib = glEnableVertexArrayAttrib;
  gl->finish = glFinish;
  gl->get_error = glGetError;
  gl->get_program_info_log = glGetProgramInfoLog;
  gl->get_shader_info_log = glGetShaderInfoLog;
  gl->get_uniform_location = glGetUniformLocation;
  gl->link_program = glLinkProgram;
  gl->named_buffer_storage = glNamedBufferStorage;
  gl->named_buffer_sub_data = glNamedBufferSubData;
  gl->polygon_mode = glPolygonMode;
  gl->shader_source = glShaderSource;
  gl->uniform_4f = glUniform4f;
  gl->use_program = glUseProgram;
  gl->vertex_array_attrib_binding = glVertexArrayAttribBinding;
  gl->vertex_array_attrib_format = glVertexArrayAttribFormat;
  gl->vertex_array_vertex_buffer = glVertexArrayVertexBuffer;
  gl->viewport = glViewport;
  gl->get_programiv = glGetProgramiv;
  gl->stencil_op = glStencilOp;
  gl->stencil_func = glStencilFunc;
  gl->stencil_mask = glStencilMask;
  gl->disable = glDisable;
  gl->gen_framebuffers = glGenFramebuffers;
  gl->bind_framebuffer = glBindFramebuffer;
  gl->framebuffer_check_status = glCheckFramebufferStatus;
  gl->delete_framebuffers = glDeleteFramebuffers;
  gl->textures_gen = glGenTextures;
  gl->texture_bind = glBindTexture;
  gl->tex_image_2d = glTexImage2D;
  gl->tex_parameter_i = glTexParameteri;
  gl->framebuffer_texture_2d = glFramebufferTexture2D;
  gl->renderbuffers_gen = glGenRenderbuffers;
  gl->renderbuffer_bind = glBindRenderbuffer;
  gl->renderbuffer_storage = glRenderbufferStorage;
  gl->renderbuffer_storage_multisample = glRenderbufferStorageMultisample;
  gl->framebuffer_renderbuffer = glFramebufferRenderbuffer;
  gl->tex_image_2d_multisample = glTexImage2DMultisample;
  gl->framebuffer_blit = glBlitFramebuffer;
  gl->bind_buffer = glBindBuffer;
  gl->buffer_data = glBufferData;
  gl->enable_vertex_attrib_array = glEnableVertexAttribArray;
  gl->vertex_attrib_pointer = glVertexAttribPointer;
  gl->bind_texture = glBindTexture;
  gl->buffer_sub_data = glBufferSubData;
  gl->active_texture = glActiveTexture;
  gl->gen_textures = glGenTextures;
  gl->pixel_store_i = glPixelStorei;
  gl->gl_uniform_matrix_4_fv = glUniformMatrix4fv;
  gl->blend_func = glBlendFunc;
  gl->uniform_3f = glUniform3f;
  gl->draw_arrays_instanced_base_instance = glDrawArraysInstancedBaseInstance;
  gl->vertex_array_element_buffer = glVertexArrayElementBuffer;
  gl->draw_elements = glDrawElements;
  gl->get_tex_image = glGetTexImage;
  gl->copy_tex_sub_image_2d = glCopyTexSubImage2D;
  gl->tex_sub_image_2d = glTexSubImage2D;
  gl->generate_mip_map = glGenerateMipmap;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {
  /* CREATE WINDOW */
  WNDCLASSEX wndclass;
  wndclass.cbSize = sizeof(WNDCLASSEX);
  wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wndclass.lpfnWndProc = WndProc; // Needed, otherwise it crashes
  wndclass.cbClsExtra = 0;
  wndclass.cbWndExtra = 0;
  wndclass.hInstance = hInstance;
  wndclass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
  wndclass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
  wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wndclass.lpszMenuName = nullptr;
  wndclass.lpszClassName = "Shot 'em up";
  RegisterClassEx(&wndclass);

  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);
  int client_width = 1280;
  int client_height = 960;
  RECT windowRect;
  SetRect(&windowRect, (screenWidth / 2) - (client_width / 2), (screenHeight / 2) - (client_height / 2),
      (screenWidth / 2) + (client_width / 2), (screenHeight / 2) + (client_height / 2));

  DWORD style = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX); // WS_THICKFRAME to resize
  AdjustWindowRectEx(&windowRect, style, FALSE, 0);
  HWND hwnd = CreateWindowEx(0, wndclass.lpszClassName, "Game Window", style, windowRect.left, windowRect.top,
      windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, nullptr, nullptr, hInstance, szCmdLine);

  HDC hdc = GetDC(hwnd);

  PIXELFORMATDESCRIPTOR pfd;
  memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
  pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 24;
  pfd.cDepthBits = 32;
  pfd.cStencilBits = 8;
  pfd.iLayerType = PFD_MAIN_PLANE;
  int pixelFormat = ChoosePixelFormat(hdc, &pfd);
  SetPixelFormat(hdc, pixelFormat, &pfd);

  /*  CREATE OPEN_GL CONTEXT */
  HGLRC tempRC = wglCreateContext(hdc);
  wglMakeCurrent(hdc, tempRC);
  PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
  wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

  const int attribList[] = {
    WGL_CONTEXT_MAJOR_VERSION_ARB,
    4,
    WGL_CONTEXT_MINOR_VERSION_ARB,
    3,
    WGL_CONTEXT_FLAGS_ARB,
    0,
    WGL_CONTEXT_PROFILE_MASK_ARB,
    WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
    0,
  };
  HGLRC hglrc = wglCreateContextAttribsARB(hdc, nullptr, attribList);

  wglMakeCurrent(nullptr, nullptr);
  wglDeleteContext(tempRC);
  wglMakeCurrent(hdc, hglrc);

  GLFunctions gl_funcs = {};
  if (!gladLoaderLoadGL()) {
    printf("Could not initialize GLAD\n");
    exit(1);
  } else {
    win32_bind_gl_funcs(&gl_funcs);
  }

  auto _wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");
  bool swapControlSupported = strstr(_wglGetExtensionsStringEXT(), "WGL_EXT_swap_control") != 0;

  int vsynch = 0;
  if (swapControlSupported) {
    // TODO: Remove auto?
    auto wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    auto wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");

    if (wglSwapIntervalEXT(1)) {
      vsynch = wglGetSwapIntervalEXT();
    } else {
      printf("Could not enable vsync\n");
    }
  } else { // !swapControlSupported
    printf("WGL_EXT_swap_control not supported\n");
  }

  ShowWindow(hwnd, SW_SHOW);
  UpdateWindow(hwnd);

  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Calls to the callback will be synchronous
  glDebugMessageCallback(MessageCallback, 0);

  // region Setup Memory
  EngineMemory memory = {};

  void* memory_block = VirtualAlloc(nullptr, // TODO: Might want to set this
      (SIZE_T)Total_Memory_Size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (memory_block == nullptr) {
    auto error = GetLastError();
    printf("Unable to allocate memory: %lu", error);
    return -1;
  }
  memory.permanent = memory_block;
  memory.transient = (u8*)memory.permanent + Permanent_Memory_Block_Size;
  memory.asset = (u8*)memory.permanent + Permanent_Memory_Block_Size + Transient_Memory_Block_Size;
  // endregion

  // region Setup Input
  EngineInput app_input = {};
  EngineFunctions app_functions = {};

  RAWINPUTDEVICE mouse;
  mouse.usUsagePage = 0x01;       // HID_USAGE_PAGE_GENERIC
  mouse.usUsage = 0x02;           // HID_USAGE_GENERIC_MOUSE
  mouse.dwFlags = RIDEV_NOLEGACY; // adds mouse and also ignores legacy mouse messages, which hides the cursor
  mouse.hwndTarget = hwnd;
  ShowCursor(FALSE);

  if (RegisterRawInputDevices(&mouse, 1, sizeof(mouse)) == FALSE) {
    exit(1);
  }

  UserInput inputs[2] = {};
  u32 curr_input_idx = 0;
  auto* current_input = &inputs[curr_input_idx];
  auto* previous_input = &inputs[curr_input_idx + 1];
  // endregion

  // region Set platform callbacks
  Platform platform = {};
  platform.get_file_last_modified = &win32_file_last_modified;
  platform.get_file_size = &win32_file_size;
  platform.read_file = &win32_read_text_file;
  platform.write_file = &win32_write_file;
  platform.debug_print_readable_timestamp = &win32_debug_print_readable_timestamp;
  // endregion

  Audio audio;
  win32_init_audio(audio);
  /* MAIN LOOP */
  auto is_running = true;
  auto is_recording = false;
  auto is_playing_back = false;
  Recording recording = {};
  Playback playback = {};

  auto last_tick = GetTickCount64();
  while (is_running) {
    auto this_tick = GetTickCount64();
    app_input.dt_ms = this_tick - last_tick;
    app_input.t_ms += app_input.dt_ms;
    last_tick = this_tick;

    if (win32_should_reload_dll(&app_functions)) {
      printf("Hot reloading dll...\n");
      win32_load_dll(&app_functions);
      app_functions.load(&gl_funcs, &platform, &memory);
    }
    RECT client_rect;
    GetClientRect(hwnd, &client_rect);
    app_input.client_height = client_rect.bottom - client_rect.top;
    app_input.client_width = client_rect.right - client_rect.left;

    {
      // Lock cursor to screen
      POINT top_left = { client_rect.left, client_rect.top };
      POINT bottom_right = { client_rect.right, client_rect.bottom };

      ClientToScreen(hwnd, &top_left);
      ClientToScreen(hwnd, &bottom_right);
      RECT screen_rect = {
        .left = top_left.x,
        .top = top_left.y,
        .right = bottom_right.x,
        .bottom = bottom_right.y,
      };
      ClipCursor(&screen_rect);
    }

    win32_process_pending_messages(hwnd, is_running, *current_input, *previous_input);

    {
      /*if (current_input->r.is_pressed_this_frame()) {
          if (!is_recording) {
              auto is_success = win32_start_recording(&memory, recording);
              if (is_success) {
                  is_recording = true;
                  printf("Started recording.\n");
              } else {
                  printf("Failed to start recording.\n");
              }
          } else {
              is_recording = false;
              win32_stop_recording(recording);
              printf("Recording successfully stopped.\n");
          }
      }*/

      /*if (current_input->p.is_pressed_this_frame() && !is_recording) {
          if (!is_playing_back) {
              auto is_success = win32_init_playback(playback);
              if (is_success) {
                  printf("Started playback.\n");
                  is_playing_back = true;
                  memcpy(memory.permanent, playback.permanent_memory, Permanent_Memory_Block_Size);
                  memcpy(memory.asset, playback.asset_memory, Assets_Memory_Block_Size);
              } else {
                  printf("Failed to start playback.\n");
              }
          } else {
              win32_stop_playback(playback);
              is_playing_back = false;
          }
      }*/

      /*if (is_playing_back) {
          if (playback.current_playback_frame == playback.num_frames_recorded) {
              memcpy(memory.permanent, playback.permanent_memory, Permanent_Memory_Block_Size);
              playback.current_playback_frame = 0;
          }
          app_input = playback.input[playback.current_playback_frame++];
      } else {*/
      app_input.input = *current_input;
      //}

      /*if (is_recording) {
          if (!win32_record_frame_input(&app_input, recording)) {
              printf("Recording failed. Stopping.\n");
              win32_stop_recording(recording);
              is_recording = false;
          }
      }*/
    }

    app_functions.update_and_render(&memory, &app_input);
    {
      XAUDIO2_VOICE_STATE state;
      audio.sourceVoice->GetState(&state);
      if (state.BuffersQueued == 0) {
        printf("Howdy\n");
        auto sound_buffer = app_functions.get_sound_samples();
        win32_play_sound(audio, sound_buffer);
      }
    }
    SwapBuffers(hdc);

    curr_input_idx = curr_input_idx == 0 ? 1 : 0;
    current_input = &inputs[curr_input_idx];
    previous_input = &inputs[curr_input_idx == 0 ? 1 : 0];
    current_input->frame_clear(*previous_input);
  }

  return 0;
}
