#pragma once

#include <core/hash.hpp>
#include <math/util.hpp>

#include "imgui.hpp"

struct UI_State {
    UI_Entity* parent; // TODO: Make this a stack
    UI_Entity* entry_point;

    UI_Layout layout;
    i32 client_width;
    i32 client_height;

    u64 hot_item;
    u64 active_item;
    u64 clicked_item;
};

MemoryArena* frame_arena = nullptr;
MemoryArena* prev_frame_arena = nullptr;

UI_State* state;
UI_State* prev_state;
LoadedFont* g_font;
i32 g_texture_id;

const i32 padd_and_margin = 10;

auto UI_GetCodePointsTotalLength(List<CodePoint>* code_points) -> f32 {
    f32 result = 0.0f;
    for (const auto& cp : *code_points) {
        result += cp.xadvance;
    }
    return result;
}

auto UI_GetCodePoints(string8 str, LoadedFont* font, MemoryArena* arena) -> List<CodePoint> {
    List<CodePoint> result;
    result.init(arena, str.length);
    for (u32 i = 0; i < str.length; i++) {
        i32 code_point_idx = (i32)str.data[i] - font->code_point_first;
        result.push(font->code_points[code_point_idx]);
    }
    return result;
}

auto UI_Initialize(MemoryArena* arena) -> void {
    frame_arena = arena->allocate_arena(MegaBytes(1));
    state = allocate<UI_State>(frame_arena);
    prev_frame_arena = arena->allocate_arena(MegaBytes(1));
}

auto UI_SetFont(i32 texture_id, LoadedFont* font) -> void {
    g_texture_id = texture_id;
    g_font = font;
}

auto UI_is_inside(i32 mouse_x, i32 mouse_y, UI_Entity* entity) -> bool {
    f32 min_x = entity->rect.bl.x;
    f32 max_x = entity->rect.tr.x;
    f32 min_y = entity->rect.bl.y;
    f32 max_y = entity->rect.tr.y;
    return hm::in_range((f32)mouse_x, min_x, max_x) && hm::in_range((f32)mouse_y, min_y, max_y);
}

auto UI_Begin(Mouse* mouse, i32 client_width, i32 client_height) -> void {
    i32 mouse_y = client_height - mouse->client_y;
    i32 mouse_x = mouse->client_x;
    Assert(frame_arena);
    Assert(prev_frame_arena);

    prev_state = state;

    MemoryArena* temp = frame_arena;
    frame_arena = prev_frame_arena;
    prev_frame_arena = temp;
    frame_arena->clear_to_zero();

    state = allocate<UI_State>(frame_arena);

    state->client_width = client_width;
    state->client_height = client_height;

    Assert(prev_state);
    UI_Entity* hovered_entity = prev_state->entry_point;
    state->active_item = 0;
    state->hot_item = 0;
    if (hovered_entity && UI_is_inside(mouse_x, mouse_y, hovered_entity)) {
        while (hovered_entity) {

            if (hovered_entity->first == nullptr) {
                break;
            }
            else {
                UI_Entity* child = hovered_entity->first;
                UI_Entity* active_child = nullptr;
                while (child) {
                    if (UI_is_inside(mouse_x, mouse_y, child)) {
                        active_child = child;
                        break;
                    }
                    child = child->right;
                }
                if (active_child != nullptr) {
                    hovered_entity = active_child;
                }
                else {
                    break;
                }
            }
        }
    }
    else {
        hovered_entity = nullptr;
    }

    if (mouse->left.is_released_this_frame()) {
        if (hovered_entity && prev_state->hot_item == hovered_entity->id) {
            state->clicked_item = hovered_entity->id;
        }
    }
    else if (prev_state->hot_item != 0) {
        state->hot_item = prev_state->hot_item;
        state->active_item = prev_state->active_item;
    }

    if (hovered_entity && state->hot_item == 0) {
        state->active_item = hovered_entity->id;

        if (mouse->left.is_pressed_this_frame()) {
            state->hot_item = hovered_entity->id;
        }
    }
}

auto calculate_size(UI_Entity* entity, Axis2 axis) -> void {
    switch (entity->semantic_size[axis].kind) {
    case UI_SizeKind_Pixels: {
        entity->computed_size[axis] = entity->semantic_size[axis].value;
        break;
    }
    case UI_SizeKind_ChildrenSum:
    case UI_SizeKind_Null: {
        UI_Entity* child = entity->first;
        i32 direction = axis == Axis2_Y ? state->layout.layout_direction : 1;
        f32 size = direction > 0 ? 0.0f : state->client_height;
        while (child) {
            calculate_size(child, axis);
            f32 child_size = child->computed_rel_position[axis] + (direction * child->computed_size[Axis2_X]);
            size = direction > 0 ? hm::max(child_size, size) : hm::min(child_size, size);
            child = child->right;
        }
        entity->computed_size[axis] = size + entity->padding[UI_Direction_Down];
    }

    case UI_SizeKind_TextContent:
    case UI_SizeKind_PercentOfParent: {
        break;
    }
    }
}

auto UI_End() -> void {
    Assert(state->entry_point);
    Assert(state->parent == nullptr);

    UI_Entity* entity = state->entry_point;
    calculate_size(entity, Axis2_X);
    calculate_size(entity, Axis2_Y);
}

auto UI_SetLayout(UI_Layout layout) -> void {
    state->layout = layout;
}

auto UI_Generate_Render_Commands(RenderGroup* render_group) -> void {
    Assert(state->entry_point);
    UI_Entity* entity = state->entry_point;
    while (entity) {

        f32 x = entity->computed_rel_position[Axis2_X];
        f32 y = entity->computed_rel_position[Axis2_Y];
        f32 el_width = entity->computed_size[Axis2_X];
        f32 el_height = entity->computed_size[Axis2_Y];
        {

            if (state->layout.layout_direction == LayoutDirection_BottomToTop) {
                entity->rect.bl = vec2(x, y);
                entity->rect.tr = vec2(x + el_width, y + el_height);
            }
            else {
                entity->rect.bl = vec2(x, y - el_height);
                entity->rect.tr = vec2(x + el_width, y);
            }

            vec2 bl = entity->rect.bl;
            vec2 tr = entity->rect.tr;
            vec2 tl = vec2(bl.x, tr.y);
            vec2 br = vec2(tr.x, bl.y);
            auto* rendel_el = PushRenderElement(render_group, RenderEntryBitmap);
            rendel_el->quad = { .bl = vec2(-0.5f, -0.5f), .tl = vec2(-0.5f, 0.5f), .tr = vec2(0.5f, 0.5f), .br = vec2(0.5f, -0.5f) };
            rendel_el->offset = vec2(x + el_width / 2, y - el_height / 2);
            rendel_el->scale = vec2(el_width, el_height);
            rendel_el->rotation = 0;
            rendel_el->texture_id = 0;
            rendel_el->color = entity->background_color;
        }

        f32 padding_y = entity->padding[UI_Direction_Down];
        f32 padding_x = entity->padding[UI_Direction_Left];
        f32 advance = x + padding_x;
        for (const CodePoint& cp : entity->text) {

            ivec2 uv_min = ivec2(cp.x0 - 1, g_font->bitmap_height - cp.y0 - 1);
            ivec2 uv_max = ivec2(cp.x1 - 1, g_font->bitmap_height - cp.y1 - 1);
            i32 width = uv_max.x - uv_min.x;
            i32 height = uv_min.y - uv_max.y; // TODO: Need to revert this shit

            if (width == 0 && height == 0) {
                // Space
                advance += 8;
                continue;
            }

            auto* el = PushRenderElement(render_group, RenderEntryBitmap);
            el->uv_min = uv_min;
            el->uv_max = uv_max;
            el->quad = { .bl = vec2(-0.5f, -0.5f), .tl = vec2(-0.5f, 0.5f), .tr = vec2(0.5f, 0.5f), .br = vec2(0.5f, -0.5f) };
            el->offset = vec2(advance + width / 2.0f, y + height / 2.0f - (g_font->font_height / 2) - padding_y);
            el->scale = vec2((f32)width, (f32)height);

            el->rotation = 0.0f;
            el->color = vec4(255.0f, 0.0f, 0.0f, 255.0f);
            el->texture_id = g_texture_id;

            advance += cp.xadvance;
        }

        if (entity->right) {
            entity = entity->right;
        }
        else {
            entity = entity->first;
        }
    }
}

auto UI_PushWindow(string8 text, f32 x, f32 y, f32 width, f32 height) -> void {
    Assert(state->parent == nullptr);
    UI_Entity* window = allocate<UI_Entity>(frame_arena, 1);
    window->id = hash64(text);
    window->flags = (UI_WidgetFlags)(UI_WidgetFlag_Draggable | UI_WidgetFlag_DrawBackground);

    if (state->layout.layout_direction == LayoutDirection_TopToBottom) {
        y = state->client_height - y;
    }

    window->position[Axis2_X] = x;
    window->position[Axis2_Y] = y;

    if (width > 0 && height > 0) {
        window->semantic_size[Axis2_X] = {
            .kind = UI_SizeKind_Pixels,
            .value = width,
            .strictness = 1.0,
        };
        window->semantic_size[Axis2_Y] = {
            .kind = UI_SizeKind_Pixels,
            .value = height,
            .strictness = 1.0,
        };
    }
    else {
        window->semantic_size[Axis2_X] = {
            .kind = UI_SizeKind_ChildrenSum,
            .value = 0,
            .strictness = 1.0,
        };
        window->semantic_size[Axis2_Y] = {
            .kind = UI_SizeKind_ChildrenSum,
            .value = 0,
            .strictness = 1.0,
        };
    }

    window->padding[UI_Direction_Up] = padd_and_margin;
    window->padding[UI_Direction_Right] = padd_and_margin;
    window->padding[UI_Direction_Down] = padd_and_margin;
    window->padding[UI_Direction_Left] = padd_and_margin;

    window->computed_rel_position[Axis2_X] = x;
    window->computed_rel_position[Axis2_Y] = y;

    state->parent = window;

    window->background_color = vec4(255.0, 255.0, 0, 255.0);

    Assert(state->entry_point == nullptr);
    state->entry_point = window;
}

auto get_y(f32 y, i32 client_height, LayoutDirection direction) -> f32 {
    return direction == LayoutDirection_BottomToTop ? y : (f32)client_height - y;
}

auto UI_PopWindow() -> void {
    Assert(state->parent != nullptr);
    state->parent = nullptr;
}

const vec4 Button_Color = vec4(1.0f, 50.0f, 90.0f, 255.0f);
const vec4 Hover_Button_Color = vec4(10.0f, 60.0f, 100.0f, 255.0f);
const vec4 Clicked_Button_Color = vec4(20.0f, 70.0f, 120.0f, 255.0f);

auto UI_Button(string8 text) -> UI_Entity_Status {
    UI_Entity* button = allocate<UI_Entity>(frame_arena, 1);
    UI_Entity_Status result = {};

    button->id = hash64(text);
    button->flags = (UI_WidgetFlags)(UI_WidgetFlag_Clickable | UI_WidgetFlag_DrawBackground);

    if (state->parent) {
        button->parent = state->parent;

        if (!state->parent->first) {
            state->parent->first = button;
            state->parent->last = button;
        }
        else {
            UI_Entity* sibling = state->parent->last;
            sibling->right = button;
            button->left = sibling;
            state->parent->last = button;
        }
    }

    button->text = UI_GetCodePoints(text, g_font, frame_arena);

    button->margin[UI_Direction_Up] = padd_and_margin;
    button->margin[UI_Direction_Right] = padd_and_margin;
    button->margin[UI_Direction_Down] = padd_and_margin;
    button->margin[UI_Direction_Left] = padd_and_margin;

    button->padding[UI_Direction_Up] = padd_and_margin;
    button->padding[UI_Direction_Right] = padd_and_margin;
    button->padding[UI_Direction_Down] = padd_and_margin;
    button->padding[UI_Direction_Left] = padd_and_margin;

    button->semantic_size[Axis2_X] = {
        .kind = UI_SizeKind_Pixels,
        .value = UI_GetCodePointsTotalLength(&button->text) + button->padding[UI_Direction_Left] + button->padding[UI_Direction_Right],
        .strictness = 1.0f,
    };
    button->semantic_size[Axis2_Y] = {
        .kind = UI_SizeKind_Pixels,
        .value = 100,
        .strictness = 1.0f,
    };

    const i32 y_dir = (i32)state->layout.layout_direction;
    // Maybe make this a post pass step?
    if (button->left == nullptr) {
        f32* computed_rel_position = button->parent->computed_rel_position;
        button->computed_rel_position[Axis2_X] = computed_rel_position[Axis2_X] + padd_and_margin;
        button->computed_rel_position[Axis2_Y] = computed_rel_position[Axis2_Y] + (y_dir * padd_and_margin);
    }
    else {
        const UI_Entity* sib = button->left;
        button->computed_rel_position[Axis2_X] = sib->computed_rel_position[Axis2_X];
        button->computed_rel_position[Axis2_Y] = sib->computed_rel_position[Axis2_Y] + //
            (y_dir * (sib->semantic_size[Axis2_Y].value + sib->margin[UI_Direction_Down]));
    }

    if (state->clicked_item == button->id) {
        result.clicked = true;
    }

    if (state->hot_item == button->id) {
        button->background_color = Clicked_Button_Color;
    }
    else if (state->active_item == button->id) {
        button->background_color = Hover_Button_Color;
    }
    else {
        button->background_color = Button_Color;
    }

    return result;
}
