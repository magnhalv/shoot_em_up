#include <cstdio>

#include <glad/gl.h>
#include <glad/wgl.h>

#include <engine/hm_assert.h>
#include <platform/platform.h>

#include "math/mat3.h"

#include <renderers/renderer.h>
#include <renderers/win32_renderer.h>

#include "../core/lib.cpp"
#include "../math/unit.cpp"

#include "gl/gl_shader.cpp"

// TODO:
// glEnable(GL_DEBUG_OUTPUT);
// glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Calls to the callback will be synchronous
// glDebugMessageCallback(MessageCallback, 0);
//
// void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
//     const GLchar* message, const void* userParam) {
//
//     if (severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_HIGH) {
//         fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
//             (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
//         // fprintf(stderr, "Exiting...\n");
//         // exit(1);
//     }
// }

typedef struct {
    u32 quad_vao;
    u32 quad_vbo;
    GLShaderProgram quad_shader;

    u32 texture_vao;
    u32 texture_vbo;
    u32 texture_ebo;
    GLShaderProgram texture_shader;

    u32 texture_handles[MaxTextureId];

} GlRendererState;

static GlRendererState state = {};

static auto clear(max_y_c client_width, max_y_c client_height, vec4 color) {
    glViewport(0, 0, client_width, client_height);
    glClearColor(color.x, color.y, color.z, color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Remove depth buffer?
}

static auto to_gl_x(f32 screen_x_cord, max_y_c screen_width) -> f32 {
    return screen_x_cord / (screen_width / 2.0f) - 1.0f;
}

static auto to_gl_y(f32 screen_y_cord, max_y_c screen_height) -> f32 {
    return screen_y_cord / (screen_height / 2.0f) - 1.0f;
}

static auto draw_quad(Quadrilateral quad, vec2 local_origin, vec2 offset, vec2 x_axis, vec2 y_axis, vec4 color,
    max_y_c screen_width, max_y_c screen_height) {
    vec2 bl = quad.bl - local_origin;
    vec2 tl = quad.tl - local_origin;
    vec2 tr = quad.tr - local_origin;
    vec2 br = quad.br - local_origin;

    bl = bl.x * x_axis + bl.y * y_axis;
    tl = tl.x * x_axis + tl.y * y_axis;
    tr = tr.x * x_axis + tr.y * y_axis;
    br = br.x * x_axis + br.y * y_axis;

    bl = bl + local_origin;
    tl = tl + local_origin;
    tr = tr + local_origin;
    br = br + local_origin;

    bl = bl + offset;
    tl = tl + offset;
    tr = tr + offset;
    br = br + offset;

    f32 quad_verticies[12] = {
        to_gl_x(bl.x, screen_width),
        to_gl_y(bl.y, screen_height),
        to_gl_x(tl.x, screen_width),
        to_gl_y(tl.y, screen_height),
        to_gl_x(tr.x, screen_width),
        to_gl_y(tr.y, screen_height),

        to_gl_x(bl.x, screen_width),
        to_gl_y(bl.y, screen_height),
        to_gl_x(tr.x, screen_width),
        to_gl_y(tr.y, screen_height),
        to_gl_x(br.x, screen_width),
        to_gl_y(br.y, screen_height),
    };

    HM_ASSERT(state.quad_vao != 0);
    glBindVertexArray(state.quad_vao);
    glNamedBufferSubData(state.quad_vbo, 0, sizeof(quad_verticies), quad_verticies);

    state.quad_shader.bind();
    state.quad_shader.set_uniform("color", color);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    state.quad_shader.unbind();
    glBindVertexArray(0);
}

static auto draw_bitmap(Quadrilateral quad, vec2 offset, vec2 scale, f32 rotation, vec4 color, max_y_c texture_id,
    max_y_c screen_width, max_y_c screen_height) {
    vec3 bl = vec3(quad.bl, 0.0f);
    vec3 tl = vec3(quad.tl, 0.0f);
    vec3 tr = vec3(quad.tr, 0.0f);
    vec3 br = vec3(quad.br, 0.0f);

    mat3 model = mat3_rotate(rotation) * mat3_scale(scale);

    bl = model * bl;
    tl = model * tl;
    tr = model * tr;
    br = model * br;

    vec3 translation = vec3(offset, 0.0);
    bl = bl + translation;
    tl = tl + translation;
    tr = tr + translation;
    br = br + translation;

    f32 vertices[] = {
        // clang-format off
        // positions                                                    // colors           // texture coords
      to_gl_x(tr.x, screen_width), to_gl_y(tr.y, screen_height), 0.0f,  color.r, color.g, color.b, color.a,   1.0f, 1.0f,   // top right
        
      to_gl_x(br.x, screen_width), to_gl_y(br.y, screen_height), 0.0f,  color.r, color.g, color.b, color.a,  1.0f, 0.0f,  // bottom right
        
      to_gl_x(bl.x, screen_width), to_gl_y(bl.y, screen_height), 0.0f,  color.r, color.g, color.b, color.a,   0.0f, 0.0f, // bottom left

      to_gl_x(tl.x, screen_width), to_gl_y(tl.y, screen_height), 0.0f,   color.r, color.g, color.b, color.a,  0.0f, 1.0f   // top left
        // clang-format on
    };
    u32* texture = &state.texture_handles[texture_id];
    HM_ASSERT(state.texture_vao != 0);
    HM_ASSERT(*texture != 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(state.texture_vao);
    glNamedBufferSubData(state.texture_vbo, 0, sizeof(vertices), vertices);
    glBindTexture(GL_TEXTURE_2D, *texture);

    state.texture_shader.bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    state.texture_shader.unbind();
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

extern "C" __declspec(dllexport) RENDERER_INIT(win32_renderer_init) {
    printf("Init opengl renderer...\n");
    {
        Win32RenderContext* win32_context = (Win32RenderContext*)context;
        win32_context->hdc = GetDC(win32_context->window);
        /*  CREATE OPEN_GL CONTEXT */
        PIXELFORMATDESCRIPTOR pfd;
        memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 24;
        pfd.cAlphaBits = 8;
        pfd.cDepthBits = 32;
        pfd.cStencilBits = 8;
        pfd.iLayerType = PFD_MAIN_PLANE;
        int pixelFormat = ChoosePixelFormat(win32_context->hdc, &pfd);
        SetPixelFormat(win32_context->hdc, pixelFormat, &pfd);

        HGLRC tempRC = wglCreateContext(win32_context->hdc);
        wglMakeCurrent(win32_context->hdc, tempRC);
        PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB_ = nullptr;
        wglCreateContextAttribsARB_ =
            (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

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
        win32_context->hglrc = wglCreateContextAttribsARB_(win32_context->hdc, nullptr, attribList);

        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(tempRC);
        wglMakeCurrent(win32_context->hdc, win32_context->hglrc);

        if (!gladLoaderLoadGL()) {
            printf("Could not initialize GLAD\n");
            exit(1);
        }

        auto _wglGetExtensionsStringEXT =
            (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");
        bool swapControlSupported = strstr(_wglGetExtensionsStringEXT(), "WGL_EXT_swap_control") != 0;

        int vsynch = 0;
        if (swapControlSupported) {
            // TODO: Remove auto?
            auto wglSwapIntervalEXT_ = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
            auto wglGetSwapIntervalEXT_ = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");

            if (wglSwapIntervalEXT_(1)) {
                vsynch = wglGetSwapIntervalEXT_();
            }
            else {
                printf("Could not enable vsync\n");
            }
        }
        else { // !swapControlSupported
            printf("WGL_EXT_swap_control not supported\n");
        }
    }

    {
        f32 quad_verticies[12] = { -1, -1, -1, 1, 1, 1, -1, -1, 1, 1, 1, -1 };
        glCreateVertexArrays(1, &state.quad_vao);
        glBindVertexArray(state.quad_vao);
        glCreateBuffers(1, &state.quad_vbo);

        glNamedBufferStorage(state.quad_vbo, sizeof(quad_verticies), quad_verticies, GL_DYNAMIC_STORAGE_BIT);
        max_y_c binding_index = 0;
        max_y_c stride = 2 * sizeof(f32);
        max_y_c offset = 0;
        glVertexArrayVertexBuffer(state.quad_vao, binding_index, state.quad_vbo, offset, stride);
        auto attrib_index = 0;

        glEnableVertexArrayAttrib(state.quad_vao, attrib_index);
        glVertexArrayAttribFormat(state.quad_vao, attrib_index, 2, GL_FLOAT, GL_FALSE, offset);
        glVertexArrayAttribBinding(state.quad_vao, attrib_index, attrib_index);
        glBindVertexArray(0);

        state.quad_shader.initialize(R"(.\shaders\basic_2d.vert)", R"(.\shaders\single_color.frag)");
    }

    // Texture VAO
    {
        u32* vao = &state.texture_vao;
        u32* vbo = &state.texture_vbo;
        u32* ebo = &state.texture_ebo;
        f32 vertices[] = {
            // clang-format off
        // positions       // colors                 // texture coords
        1.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
        1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f,   1.0f, 0.0f,  // bottom right
        0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f,   0.0f, 0.0f, // bottom left
        0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f,   0.0f, 1.0f   // top left
            // clang-format on
        };
        const u32 vbo_offset = 0;
        const u32 num_pos = 3;
        const u32 num_colors = 4;
        const u32 num_uv = 2;
        const u32 vbo_stride = (num_pos + num_colors + num_uv) * sizeof(f32);

        u32 indices[] = {
            0, 1, 3, // first triangle
            1, 2, 3  // second triangle
        };

        glCreateVertexArrays(1, vao);
        printf("New VAO: %d\n", *vao);
        glCreateBuffers(1, vbo);
        glCreateBuffers(1, ebo);

        glBindVertexArray(*vao);

        glNamedBufferStorage(*vbo, sizeof(vertices), vertices, GL_DYNAMIC_STORAGE_BIT);
        glNamedBufferStorage(*ebo, sizeof(indices), indices, GL_DYNAMIC_STORAGE_BIT);
        glVertexArrayElementBuffer(*vao, *ebo);
        const u32 VBO_bi = 0;
        glVertexArrayVertexBuffer(*vao, VBO_bi, *vbo, vbo_offset, vbo_stride);

        u32 vbo_binding_index = 0;
        u32 attrib_index = 0;
        // position attribute
        glVertexArrayAttribFormat(*vao, attrib_index, num_pos, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(*vao, attrib_index, vbo_binding_index);
        glEnableVertexArrayAttrib(*vao, attrib_index);
        attrib_index++;

        // color attribute
        glVertexArrayAttribFormat(*vao, attrib_index, num_colors, GL_FLOAT, GL_FALSE, num_pos * sizeof(f32));
        glVertexArrayAttribBinding(*vao, attrib_index, vbo_binding_index);
        glEnableVertexArrayAttrib(*vao, attrib_index);
        attrib_index++;

        // texture coord attribute
        glVertexArrayAttribFormat(*vao, attrib_index, num_uv, GL_FLOAT, GL_FALSE, (num_pos + num_colors) * sizeof(f32));
        glVertexArrayAttribBinding(*vao, attrib_index, vbo_binding_index);
        glEnableVertexArrayAttrib(*vao, attrib_index);
        attrib_index++;

        state.texture_shader.initialize(R"(.\shaders\texture_2d.vert)", R"(.\shaders\texture_2d.frag)");
    }
    {

        u32* texture = &state.texture_handles[0];
        if (*texture == 0) {
            u32 width = 1;
            u32 height = 1;
            u32 data = 0xFF0000FF;
            glGenTextures(1, texture);
            glBindTexture(GL_TEXTURE_2D, *texture);
            // set the texture wrapping parameters
            // set texture wrapping to GL_REPEAT (default wrapping method)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            // set texture filtering parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // load image, create texture and generate mipmaps
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    printf("OpenGL ready to go.\n");
}

extern "C" __declspec(dllexport) RENDERER_ADD_TEXTURE(win32_renderer_add_texture) {
    if (texture_id >= MaxTextureId) {
        InvalidCodePath;
    }

    u32* texture = &state.texture_handles[texture_id];
    // TODO: Handle 1 byte textures!
    if (*texture == 0) {
        glGenTextures(1, texture);
        glBindTexture(GL_TEXTURE_2D, *texture);
        // set the texture wrapping parameters
        // set texture wrapping to GL_REPEAT (default wrapping method)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // load image, create texture and generate mipmaps
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }
    return false;
}

extern "C" __declspec(dllexport) RENDERER_RENDER(win32_renderer_render) {
    for (u32 base_address = 0; base_address < group->push_buffer_size;) {
        RenderGroupEntryHeader* header = (RenderGroupEntryHeader*)(group->push_buffer + base_address);
        base_address += sizeof(RenderGroupEntryHeader);

        void* data = (u8*)header + sizeof(*header);
        switch (header->type) {
        case RenderCommands_RenderEntryClear: {
            RenderEntryClear* entry = (RenderEntryClear*)data;
            clear(group->screen_width, group->screen_height, entry->color);
            base_address += sizeof(*entry);
        } break;
        case RenderCommands_RenderEntryQuadrilateral: {
            auto* entry = (RenderEntryQuadrilateral*)data;
            /*draw_quad(entry->quad, entry->local_origin, entry->offset, entry->basis.x, entry->basis.y, entry->color,*/
            /*    group->screen_width, group->screen_height);*/

            base_address += sizeof(*entry);
        } break;
        case RenderCommands_RenderEntryBitmap: {
            auto* entry = (RenderEntryBitmap*)data;
            draw_bitmap(entry->quad, entry->offset, entry->scale, entry->rotation, entry->color, entry->texture_id,
                group->screen_width, group->screen_height);

            base_address += sizeof(*entry);
        } break;
        default: InvalidCodePath;
        }
    }
}

extern "C" __declspec(dllexport) RENDERER_BEGIN_FRAME(win32_renderer_begin_frame) {
}

extern "C" __declspec(dllexport) RENDERER_END_FRAME(win32_renderer_end_frame) {
    Win32RenderContext* win32_context = (Win32RenderContext*)context;
    SwapBuffers(win32_context->hdc);
}

extern "C" __declspec(dllexport) RENDERER_DELETE_CONTEXT(win32_renderer_delete_context) {
    Win32RenderContext* win32_context = (Win32RenderContext*)context;
    wglMakeCurrent(nullptr, nullptr);
    SwapBuffers(win32_context->hdc);
    ReleaseDC(win32_context->window, win32_context->hdc);
    wglDeleteContext(win32_context->hglrc);

    state = { 0 };
}
