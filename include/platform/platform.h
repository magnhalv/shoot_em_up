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

const i32 SCREEN_WIDTH = 960;
const i32 SCREEN_HEIGHT = 540;

const u32 WORKER_THREAD_COUNT = 4;
const u32 TOTAL_THREAD_COUNT = WORKER_THREAD_COUNT + 1;

const u32 Gl_Invalid_Id = 0;

typedef struct {
    bool has_errors;
    void* platform;
} PlatformFileHandle;

typedef struct {
    u32 file_count;
    void* platform;
} PlatformFileGroup;

#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
#unsed COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#include <immintrin.h>

#if COMPILER_MSVC
#include <intrin.h>
#elif COMPILER_LLVM
#include <x86intrin.h>
#else
#error SEE/NEON is not availbe for this compiler.
#endif

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

#define PLATFORM_PRINT_STACK_TRACE(name) void name()
typedef PLATFORM_PRINT_STACK_TRACE(platform_print_stack_trace);

struct PlatformWorkQueue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(PlatformWorkQueue* queue, void* data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

typedef void platform_add_work_queue_entry(PlatformWorkQueue* Queue, platform_work_queue_callback* Callback, void* Data);
typedef void platform_complete_all_work(PlatformWorkQueue* Queue);

struct DebugTable;
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

    platform_print_stack_trace* print_stack_trace;

    PlatformWorkQueue* work_queue = nullptr;
#if HOMEMADE_DEBUG
    DebugTable* debug_table;
    f32 frame_duration_ms;
    f32 frame_target_ms;
    i64 frame_duration_clock_cycles;
    i64 total_frame_duration_clock_cycles;
    u32 main_thread_id;
#endif

    u32 thread_ids[TOTAL_THREAD_COUNT];
};

global_variable PlatformApi* Platform = nullptr;

const u64 Permanent_Memory_Block_Size = MegaBytes(10);
const u64 Transient_Memory_Block_Size = MegaBytes(20);
const u64 Debug_Memory_Block_Size = MegaBytes(20);
const u64 Total_Memory_Size = Permanent_Memory_Block_Size + Transient_Memory_Block_Size + Debug_Memory_Block_Size;
const u64 Renderer_Permanent_Memory_Size = MegaBytes(10);
const u64 Renderer_Transient_Memory_Size = MegaBytes(10);
const u64 Renderer_Total_Memory_Size = Renderer_Permanent_Memory_Size + Renderer_Transient_Memory_Size;

#define Assert(expr)                                                                   \
    if (!(expr)) {                                                                     \
        printf("Assertion failed: %s, file %s, line %d\n", #expr, __FILE__, __LINE__); \
        abort();                                                                       \
    }
#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase \
    default: {             \
        InvalidCodePath;   \
    } break

inline u32 safe_truncate_u64(u64 value) {
    Assert(value <= 0xFFFFFFFF);
    u32 result = (u32)value;
    return result;
}

#if COMPILER_MSVC
inline auto atomic_add_u64(u64 volatile* value, u64 addend) -> u64 {
    u64 original_value = _InterlockedExchangeAdd64((i64 volatile*)value, addend);
    return original_value;
}

inline auto atomic_exchange_u64(u64 volatile* value, u64 exchange) -> u64 {
    u64 orignal_value = _InterlockedExchange64((i64 volatile*)value, exchange);
    return orignal_value;
}

inline auto get_thread_id() -> u32 {
    u8* ThreadLocalStorage = (u8*)__readgsqword(0x30);
    u32 ThreadID = *(u32*)(ThreadLocalStorage + 0x48);

    return (ThreadID);
}
#else
#error Unsupported architecture
#endif
