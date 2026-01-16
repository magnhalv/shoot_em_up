#ifndef ENGINE_IMGUI_HUGIN_H_
#define ENGINE_IMGUI_HUGIN_H_

#include <math/vec2.h>
#include <platform/types.h>
#include <platform/user_input.h>

#include <core/memory_arena.h>
#include <core/string8.hpp>
#include <core/util.hpp>

#include <engine/assets.h>
#include <engine/list.h>

#include <renderers/renderer.h>

enum LayoutDirection { LayoutDirection_TopToBottom = -1, LayoutDirection_BottomToTop = 1 };

struct UI_Layout {
    LayoutDirection layout_direction;
};

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

enum Axis2 { Axis2_X = 0, Axis2_Y, Axis2_Count };
enum UI_Direction { UI_Direction_Up = 0, UI_Direction_Right, UI_Direction_Down, UI_Direction_Left, UI_Direction_Count };

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

struct UI_Entity_Status {
    bool hovered;
    bool active;
    bool clicked;
    bool pressed;
};

struct UI_Entity {
    u64 id;
    UI_Entity* parent;
    UI_Entity* last;  // child
    UI_Entity* first; // child
    UI_Entity* left;  // sibling
    UI_Entity* right; // sibling

    UI_WidgetFlags flags;

    UI_Size semantic_size[Axis2_Count];
    f32 margin[UI_Direction_Count];
    f32 padding[UI_Direction_Count];

    f32 computed_rel_position[Axis2_Count]; // Position relative to parent
    f32 position[Axis2_Count];
    f32 computed_size[Axis2_Count];

    RectangleF32 rect;

    List<CodePoint> text;

    vec4 background_color;
};

auto UI_Initialize(MemoryArena* arena) -> void;
auto UI_Begin(Mouse* mouse, i32 client_width, i32 client_height) -> void;
auto UI_End() -> void;
auto UI_SetLayout(UI_Layout layout) -> void;
auto UI_SetFont(i32 texture_id, LoadedFont* font) -> void;

auto UI_PushWindow(string8 text, f32 x = 0, f32 y = 0, f32 width = 0, f32 height = 0) -> void;
auto UI_PopWindow() -> void;
auto UI_Button(string8 text) -> UI_Entity_Status;

auto UI_Generate_Render_Commands(RenderGroup* render_group) -> void;

#define UI_Window(text, x, y) DeferLoop(UI_PushWindow(text, x, y, 0, 0), UI_PopWindow())
#define UI_WindowFull(text, x, y, width, height) DeferLoop(UI_PushWindow(text, x, y, width, height), UI_PopWindow())

#endif
