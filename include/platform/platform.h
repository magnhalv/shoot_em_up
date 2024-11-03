#ifndef HOT_RELOAD_OPENGL_PLATFORM_H
#define HOT_RELOAD_OPENGL_PLATFORM_H

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126

#include <glad/gl.h>

#include "types.h"
#include "user_input.h"

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
};

const u32 Gl_Invalid_Id = 0;

// Functions platform MUST support
typedef u64 (*GET_FILE_LAST_MODIFIED_PROC)(const char* file_path);
typedef bool (*READ_FILE_PROC)(const char*, char*, const u64);
typedef bool (*WRITE_FILE_PROC)(const char*, const char*, const u64);
typedef u64 (*GET_FILE_SIZE_PROC)(const char*);
typedef void (*DEBUG_PRINT_READABLE_TIMESTAMP_PROC)(u64);

struct Platform {
  GET_FILE_LAST_MODIFIED_PROC get_file_last_modified;
  READ_FILE_PROC read_file;
  WRITE_FILE_PROC write_file;
  GET_FILE_SIZE_PROC get_file_size;
  DEBUG_PRINT_READABLE_TIMESTAMP_PROC debug_print_readable_timestamp;
};

const u64 Permanent_Memory_Block_Size = MegaBytes(10);
const u64 Transient_Memory_Block_Size = MegaBytes(10);
const u64 Assets_Memory_Block_Size = MegaBytes(1);
const u64 Total_Memory_Size = Permanent_Memory_Block_Size + Transient_Memory_Block_Size + Assets_Memory_Block_Size;

struct EngineMemory {
  void* permanent = nullptr; // NOTE: Must be cleared to zero
  void* transient = nullptr; // NOTE: Must be cleared to zero
  void* asset = nullptr;     // NOTE: Must be cleared to zero
};

struct EngineInput {
  i32 client_width;
  i32 client_height;
  u64 t_ms;
  u64 dt_ms;
  UserInput input;
};

// Functions application MUST support
typedef void(__cdecl* UPDATE_AND_RENDER_PROC)(EngineMemory*, EngineInput*);
typedef void(__cdecl* LOAD_PROC)(GLFunctions*, Platform*, EngineMemory*);

#endif // HOT_RELOAD_OPENGL_PLATFORM_H
