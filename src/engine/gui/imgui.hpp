#include <math/vec2.h>
#include <platform/types.h>

#include <core/memory_arena.h>
#include <core/string8.hpp>
#include <core/util.hpp>

#include <renderers/renderer.h>

enum UI_WidgetFlags : u32 {
    UI_WidgetFlag_Clickable = (1 << 0),
    UI_WidgetFlag_DrawText = (1 << 1),
    UI_WidgetFlag_DrawBorder = (1 << 2),
    UI_WidgetFlag_DrawBackground = (1 << 3),
    UI_WidgetFlag_DrawDropShadow = (1 << 4),
    UI_WidgetFlag_Draggable = (1 << 5),
};

struct RectangleF32 {
    vec2 bl;
    vec2 tr;
};

enum Axis2 { Axis2_X = 0, Axis2_Y, Axis2_COUNT };

enum UI_SizeKind {
    UI_SizeKind_Null,
    UI_SizeKind_Pixels,
    UI_SizeKind_TextContent,
    UI_SizeKind_PercentOfParent,
    UI_SizeKind_ChildrenSum,
};

struct UI_Size {
    UI_SizeKind kind;
    f32 value;
    f32 strictness;
};

struct UI_Entity {
    u64 id;
    UI_Entity* parent;
    UI_Entity* last;  // child
    UI_Entity* first; // child
    UI_Entity* left;  // sibling
    UI_Entity* right; // sibling

    UI_WidgetFlags flags;

    UI_Size semantic_size[Axis2_COUNT];
    f32 computed_rel_position[Axis2_COUNT]; // Position relative to parent
    f32 computed_size[Axis2_COUNT];
    RectangleF32 rect;
};

auto UI_Initialize(MemoryArena* arena) -> void;
auto UI_Begin() -> void;
auto UI_End() -> void;

auto UI_PushWindow(string8 text, f32 x, f32 y, f32 width, f32 height) -> void;
auto UI_PopWindow() -> void;
auto UI_Button(string8 text) -> bool;

auto UI_Generate_Render_Commands(RenderGroup* render_group) -> void;

#define UI_Window(text, x, y, width, height) DeferLoop(UI_PushWindow(text, x, y, width, height), UI_PopWindow());
