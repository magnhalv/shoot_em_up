#ifndef HOT_RELOAD_OPENGL_TEXT_RENDERER_H
#define HOT_RELOAD_OPENGL_TEXT_RENDERER_H

#include "array.h"
#include "fixed_string.h"
#include "math/mat4.h"

struct Character {
    ivec2 size;
    ivec2 bearing;
    vec2 uv_start;
    vec2 uv_end;
    u32 advance{};
};

struct Font {
    FStr name;
    u32 texture_atlas;
    Array<Character> characters;
};

auto font_load(const char* path, MemoryArena& permanent_arena) -> Font*;
auto font_str_dim(const char* str, f32 scale, Font& font) -> vec2;

struct TextRenderer {
    auto init() -> void;
    auto render(const char* text, const Font& font, f32 x, f32 y, f32 scale, const mat4& ortho_projection) -> void;
};

#endif // HOT_RELOAD_OPENGL_TEXT_RENDERER_H
