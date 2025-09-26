#pragma once

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126

// For exit(1)
#include <stdlib.h>

#include "types.h"
#include "user_input.h"

#include <renderers/renderer.h>

const u32 Gl_Invalid_Id = 0;

#define local_persist static
#define global_variable extern

#define Assert(expr)                                                                   \
    if (!(expr)) {                                                                     \
        printf("Assertion failed: %s, file %s, line %d\n", #expr, __FILE__, __LINE__); \
        exit(1);                                                                       \
    }
#define InvalidCodePath assert(!"InvalidCodePath")
#define InvalidDefaultCase \
    default: {             \
        InvalidCodePath;   \
    } break

inline u32 safe_truncate_u64(u64 value) {
    Assert(value <= 0xFFFFFFFF);
    u32 result = (u32)value;
    return result;
}

typedef struct {
    bool has_errors;
    void* platform;
} PlatformFileHandle;

typedef struct {
    u32 file_count;
    void* platform;
} PlatformFileGroup;

enum PlatformFileType : u32 { PlatformFileType_AssetFile, PlatformFileType_Count };

// Platform API
#define PLATFORM_GET_FILE_LAST_MODIFIED(name) u64 name(const char* file_path)
typedef PLATFORM_GET_FILE_LAST_MODIFIED(platform_get_file_last_modified);

#define DEBUG_PLATFORM_READ_FILE(name) bool name(const char* path, char* buffer, const u64 buffer_size)
typedef DEBUG_PLATFORM_READ_FILE(debug_platform_read_file);

#define PLATFORM_WRITE_FILE(name) bool name(const char*, const char*, const u64)
typedef PLATFORM_WRITE_FILE(platform_write_file);

#define PLATFORM_GET_FILE_SIZE(name) u64 name(const char*)
typedef PLATFORM_GET_FILE_SIZE(platform_get_file_size);

#define PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN(name) PlatformFileGroup name(PlatformFileType type)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN(platform_get_all_files_of_type_begin);

#define PLATFORM_GET_ALL_FILES_OF_TYPE_END(name) void name(PlatformFileGroup* file_group)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_END(platform_get_all_files_of_type_end);

#define PLATFORM_OPEN_FILE(name) PlatformFileHandle name(PlatformFileGroup* file_group)
typedef PLATFORM_OPEN_FILE(platform_open_next_file);

#define PLATFORM_READ_FILE(name) void name(PlatformFileHandle* platform_file_handle, u64 offset, u64 size, void* dest)
typedef PLATFORM_READ_FILE(platform_read_file);

struct PlatformWorkQueue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(PlatformWorkQueue* Queue, void* Data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

typedef void platform_add_work_queue_entry(PlatformWorkQueue* Queue, platform_work_queue_callback* Callback, void* Data);
typedef void platform_complete_all_work(PlatformWorkQueue* Queue);

struct PlatformApi {
    platform_get_file_last_modified* get_file_last_modified;
    debug_platform_read_file* debug_read_file;
    platform_write_file* write_file;
    platform_get_file_size* get_file_size;

    platform_get_all_files_of_type_begin* get_all_files_of_type_begin;
    platform_get_all_files_of_type_end* get_all_files_of_type_end;
    platform_open_next_file* open_next_file;
    platform_read_file* read_file;

    platform_add_work_queue_entry* add_work_queue_entry;
    platform_complete_all_work* complete_all_work;
};

const u64 Permanent_Memory_Block_Size = MegaBytes(10);
const u64 Transient_Memory_Block_Size = MegaBytes(10);
const u64 Total_Memory_Size = Permanent_Memory_Block_Size + Transient_Memory_Block_Size;

struct EngineMemory {
    void* permanent = nullptr; // NOTE: Must be cleared to zero
    void* transient = nullptr; // NOTE: Must be cleared to zero
    void* asset = nullptr;     // NOTE: Must be cleared to zero

    PlatformWorkQueue* work_queue = nullptr;
};

struct EngineInput {
    i32 client_width;
    i32 client_height;
    u64 ticks;
    u64 dt_tick;
    f64 t;
    f64 dt;
    UserInput input;
};

constexpr i32 SoundSampleSize = sizeof(i16);
struct SoundBuffer {
    i16* samples;
    i32 num_samples;
    i32 sample_size_in_bytes;
    i32 samples_per_second;
    i32 tone_hz;
};

#define ENGINE_UPDATE_AND_RENDER(name) void name(EngineMemory* memory, EngineInput* app_input, RendererApi* renderer)
typedef ENGINE_UPDATE_AND_RENDER(update_and_render_fn);

#define ENGINE_LOAD(name) void name(PlatformApi* platform_api, EngineMemory* memory)
typedef ENGINE_LOAD(load_fn);

#define ENGINE_GET_SOUND_SAMPLES(name) SoundBuffer name(EngineMemory* memory, i32 num_samples)
typedef ENGINE_GET_SOUND_SAMPLES(get_sound_samples_fn);
