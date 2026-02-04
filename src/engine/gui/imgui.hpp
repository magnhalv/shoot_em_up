#ifndef ENGINE_IMGUI_HUGIN_H_
#define ENGINE_IMGUI_HUGIN_H_

#include <math/vec2.h>
#include <platform/types.h>
#include <platform/user_input.h>

#include <core/list.hpp>
#include <core/memory_arena.h>
#include <core/stack_list.hpp>
#include <core/string8.hpp>
#include <core/util.hpp>

#include <engine/assets.h>

#include <renderers/renderer.h>

enum UI_Direction { UI_Direction_Up = 0, UI_Direction_Right, UI_Direction_Down, UI_Direction_Left, UI_Direction_Count };

struct UI_Layout {
    UI_Direction layout_direction;
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

enum UI_SizeKind {
    UI_SizeKind_ChildrenSum = 0,
    UI_SizeKind_Pixels,
    UI_SizeKind_TextContent,
    UI_SizeKind_PercentOfParent,
};

struct UI_Size {
    UI_SizeKind kind;
    f32 value;
    f32 strictness;
};

constexpr auto UI_Pixels(f32 value) -> UI_Size {
    return UI_Size{                 //
        .kind = UI_SizeKind_Pixels, //
        .value = value,             //
        .strictness = 1.0f
    };
};

constexpr auto UI_PercentOfParent(f32 value) -> UI_Size {
    return UI_Size{                          //
        .kind = UI_SizeKind_PercentOfParent, //
        .value = value,                      //
        .strictness = 1.0f
    };
};

enum UI_PositionKind {
    UI_PositionKind_Flex = 0,
    UI_PositionKind_Fixed,
    UI_PositionKind_Relative,
};

struct UI_Position {
    UI_PositionKind kind;
    f32 value;
};

constexpr auto UI_Fixed(f32 value) -> UI_Position {
    return UI_Position{
        .kind = UI_PositionKind_Fixed, //
        .value = value                 //
    };
}

constexpr auto UI_Flex() -> UI_Position {
    return UI_Position{};
}

constexpr auto UI_Relative(f32 value) -> UI_Position {
    return UI_Position{
        .kind = UI_PositionKind_Relative, //
        .value = value                    //
    };
}

struct UI_Entity_Status {
    bool hovered;
    bool first_hovered;
    bool active;
    bool click_released;
    bool pressed;
};

enum UI_FlexDirection {
    UI_Flex_Row = 0,
    UI_Flex_Column,
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
    f32 computed_size[Axis2_Count];

    f32 margin[UI_Direction_Count];
    f32 padding[UI_Direction_Count];

    UI_FlexDirection flex_direction;
    UI_Position semantic_position[Axis2_Count];
    f32 computed_rel_position[Axis2_Count]; // Position relative to parent
    f32 position[Axis2_Count];

    f32 computed_child_offset[Axis2_Count];

    i32 z_index;

    RectangleF32 rect;

    List<CodePoint> text;

    vec4 background_color;
};

struct UI_FrameState {
    List<UI_Entity*> parents;

    UI_Layout layout;
    i32 client_width;
    i32 client_height;

    u64 hot_item;
    u64 active_item;
    u64 click_released_item;
};

i32 const UI_Max_Style_Depth = 10;
struct UI_StyleOverrides {
    StackList<vec4, UI_Max_Style_Depth> background_colors;
    StackList<UI_Size, UI_Max_Style_Depth> width;
    StackList<UI_Size, UI_Max_Style_Depth> height;
    StackList<UI_Direction, UI_Max_Style_Depth> layout_direction;
    StackList<i32, UI_Max_Style_Depth> z_index;
    StackList<UI_FlexDirection, UI_Max_Style_Depth> flex_direction;
};

struct UI_Style {
    vec4 background_color;
    UI_Size width;
    UI_Size height;
    i32 z_index;
    UI_FlexDirection flex_direction;
};

inline UI_Style default_style = {  //
    .background_color = vec4(0.0, 0.0, 0.0, 0.0f),
    .width = {
        .kind = UI_SizeKind_ChildrenSum,
        .value = 0.0f,
        .strictness = 0.0,
    },
    .height = {
        .kind = UI_SizeKind_ChildrenSum,
        .value = 0.0f,
        .strictness = 0.0,
    },
    .z_index = -1,
    .flex_direction = UI_Flex_Row
};

struct UI_Context {
    UI_FrameState frame_states[2];
    MemoryArena* frame_arenas[2];
    MemoryArena* arena;
    i32 curr_state_index;

    LoadedFont* font;
    i32 texture_id;

    UI_Style* style;
    UI_StyleOverrides style_overrides;

    auto state() -> UI_FrameState* {
        return &frame_states[curr_state_index];
    }

    auto frame_arena() -> MemoryArena* {
        return frame_arenas[curr_state_index];
    }

    auto prev_state() -> UI_FrameState* {
        i32 prev_state_index = curr_state_index == 0 ? 1 : 0;
        return &frame_states[prev_state_index];
    }

    auto flip_and_clear_state() -> void {
        curr_state_index = curr_state_index == 0 ? 1 : 0;

        frame_states[curr_state_index] = {};
        frame_arenas[curr_state_index]->clear_to_zero();
    }
};

// Why not just make gui an object? Trying some more pure functional to make it make sense.
auto UI_CreateContext(MemoryArena* arena) -> UI_Context*;
auto UI_SetContext(UI_Context* context) -> void;
auto UI_Begin(Mouse* mouse, i32 client_width, i32 client_height) -> void;
auto UI_End() -> void;
auto UI_SetLayout(UI_Layout layout) -> void;
auto UI_SetFont(i32 texture_id, LoadedFont* font) -> void;

auto UI_PushWindow(string8 text, UI_Position x = {}, UI_Position y = {}, UI_Size width = {}, UI_Size height = {}) -> void;
auto UI_PopWindow() -> void;
auto UI_Button(string8 text) -> UI_Entity_Status;
auto UI_Box(string8 id, UI_Size width = {}, UI_Size height = {}) -> UI_Entity_Status;
auto UI_Text(string8 text) -> UI_Entity_Status;

// Styling
auto UI_PushStyleSize(UI_Size* x, UI_Size* y) -> void;
auto UI_PopStyleSize() -> void;
auto UI_PushStyleZIndex(i32 z_index) -> void;
auto UI_PopStyleZIndex() -> void;
auto UI_PushStyleBackgroundColor(vec4 color) -> void;
auto UI_PopStyleBackgroundColor() -> void;
auto UI_PushStyleFlexDirection(UI_FlexDirection direction) -> void;
auto UI_PopStyleFlexDirection() -> void;

auto UI_Generate_Render_Commands(RenderGroup* render_group) -> void;

#define UI_SetSize(size_x, size_y) DeferLoop(UI_PushStyleSize(size_x, size_y), UI_PopStyleSize())
#define UI_Window(text, x, y) DeferLoop(UI_PushWindow(text, x, y), UI_PopWindow())
#define UI_WindowFull(text, x, y, width, height) DeferLoop(UI_PushWindow(text, x, y, width, height), UI_PopWindow())

#endif
