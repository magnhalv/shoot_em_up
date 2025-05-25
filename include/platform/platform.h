#ifndef HOT_RELOAD_OPENGL_PLATFORM_H
#define HOT_RELOAD_OPENGL_PLATFORM_H

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126

#include "types.h"
#include "user_input.h"

#include <glad/gl.h>

struct GLFunctions {
    PFNGLATTACHSHADERPROC attach_shader;
    PFNGLBINDBUFFERBASEPROC bind_buffer_base;
    PFNGLBINDVERTEXARRAYPROC bind_vertex_array;
    PFNGLCLEARCOLORPROC clear_color;
    PFNGLCLEARPROC clear;
    PFNGLCOMPILESHADERPROC compile_shader;
    PFNGLCREATEBUFFERSPROC create_buffers;
    PFNGLCREATEPROGRAMPROC create_program;
    PFNGLCREATESHADERPROC create_shader;
    PFNGLCREATEVERTEXARRAYSPROC create_vertex_arrays;
    PFNGLDELETEBUFFERSPROC delete_buffers;
    PFNGLDELETEPROGRAMPROC delete_program;
    PFNGLDELETESHADERPROC delete_shader;
    PFNGLDELETEVERTEXARRAYSPROC delete_vertex_array;
    PFNGLDRAWARRAYSPROC draw_arrays;
    PFNGLENABLEPROC enable;
    PFNGLENABLEVERTEXARRAYATTRIBPROC enable_vertex_array_attrib;
    PFNGLFINISHPROC finish;
    PFNGLGETERRORPROC get_error;
    PFNGLGETPROGRAMINFOLOGPROC get_program_info_log;
    PFNGLGETSHADERINFOLOGPROC get_shader_info_log;
    PFNGLGETUNIFORMLOCATIONPROC get_uniform_location;
    PFNGLLINKPROGRAMPROC link_program;
    PFNGLNAMEDBUFFERSTORAGEPROC named_buffer_storage;
    PFNGLNAMEDBUFFERSUBDATAPROC named_buffer_sub_data;
    PFNGLPOLYGONMODEPROC polygon_mode;
    PFNGLSHADERSOURCEPROC shader_source;
    PFNGLUNIFORM4FPROC uniform_4f;
    PFNGLUNIFORM3FPROC uniform_3f;
    PFNGLUSEPROGRAMPROC use_program;
    PFNGLVERTEXARRAYATTRIBBINDINGPROC vertex_array_attrib_binding;
    PFNGLVERTEXARRAYATTRIBFORMATPROC vertex_array_attrib_format;
    PFNGLVERTEXARRAYVERTEXBUFFERPROC vertex_array_vertex_buffer;
    PFNGLVIEWPORTPROC viewport;
    PFNGLDETACHSHADERPROC detach_shader;
    PFNGLGETPROGRAMIVPROC get_programiv;
    PFNGLSTENCILOPPROC stencil_op;
    PFNGLSTENCILFUNCPROC stencil_func;
    PFNGLSTENCILMASKPROC stencil_mask;
    PFNGLDISABLEPROC disable;
    PFNGLGENFRAMEBUFFERSPROC gen_framebuffers;
    PFNGLBINDFRAMEBUFFERPROC bind_framebuffer;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC framebuffer_check_status;
    PFNGLDELETEFRAMEBUFFERSPROC delete_framebuffers;
    PFNGLGENTEXTURESPROC textures_gen;
    PFNGLBINDTEXTUREPROC texture_bind;
    PFNGLTEXIMAGE2DPROC tex_image_2d;
    PFNGLTEXPARAMETERIPROC tex_parameter_i;
    PFNGLFRAMEBUFFERTEXTURE2DPROC framebuffer_texture_2d;
    PFNGLFRAMEBUFFERRENDERBUFFERPROC framebuffer_renderbuffer;
    PFNGLRENDERBUFFERSTORAGEPROC renderbuffer_storage;
    PFNGLBINDRENDERBUFFERPROC renderbuffer_bind;
    PFNGLGENRENDERBUFFERSPROC renderbuffers_gen;
    PFNGLTEXIMAGE2DMULTISAMPLEPROC tex_image_2d_multisample;
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC renderbuffer_storage_multisample;
    PFNGLBLITFRAMEBUFFERPROC framebuffer_blit;
    PFNGLBINDBUFFERPROC bind_buffer;
    PFNGLBUFFERDATAPROC buffer_data;
    PFNGLENABLEVERTEXATTRIBARRAYPROC enable_vertex_attrib_array;
    PFNGLVERTEXATTRIBPOINTERPROC vertex_attrib_pointer;
    PFNGLBINDTEXTUREPROC bind_texture;
    PFNGLGETTEXIMAGEPROC get_tex_image;
    PFNGLBUFFERSUBDATAPROC buffer_sub_data;
    PFNGLACTIVETEXTUREPROC active_texture;
    PFNGLGENTEXTURESPROC gen_textures;
    PFNGLPIXELSTOREIPROC pixel_store_i;
    PFNGLUNIFORMMATRIX4FVPROC gl_uniform_matrix_4_fv;
    PFNGLBLENDFUNCPROC blend_func;
    PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC draw_arrays_instanced_base_instance;
    PFNGLVERTEXARRAYELEMENTBUFFERPROC vertex_array_element_buffer;
    PFNGLDRAWELEMENTSPROC draw_elements;
    PFNGLCOPYTEXSUBIMAGE2DPROC copy_tex_sub_image_2d;
    PFNGLTEXSUBIMAGE2DPROC tex_sub_image_2d;
    PFNGLGENERATEMIPMAPPROC generate_mip_map;
};

const u32 Gl_Invalid_Id = 0;

typedef struct {
    bool no_errors;
    void* platform;
} PlatformFileHandle;

typedef struct {
    u32 file_count;
    void* platform;
} PlatformFileGroup;

enum class PlatformFileType : u32 { AssetFile, Count };

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

#define PLATFORM_READ_FILE(name) bool name(PlatformFileHandle* handle, u64 offset, u64 size, void* dest)
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
const u64 Assets_Memory_Block_Size = MegaBytes(1);
const u64 Total_Memory_Size = Permanent_Memory_Block_Size + Transient_Memory_Block_Size + Assets_Memory_Block_Size;

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

// Functions application MUST support
typedef void(__cdecl* UPDATE_AND_RENDER_PROC)(EngineMemory*, EngineInput*);
typedef void(__cdecl* LOAD_PROC)(GLFunctions*, PlatformApi*, EngineMemory*);
typedef SoundBuffer(__cdecl* GET_SOUND_SAMPLES_PROC)(EngineMemory* memory, i32 num_samples);

#endif // HOT_RELOAD_OPENGL_PLATFORM_H
