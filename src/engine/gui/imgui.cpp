#pragma once

#include <core/hash.hpp>

#include "imgui.hpp"

struct UI_State {
    MemoryArena* curr_frame_arena;
    MemoryArena* prev_frame_arena;
    UI_Entity* parent; // TODO: Make this a stack
    UI_Entity* entry_point;
};

UI_State state{};

auto UI_Initialize(MemoryArena* arena) -> void {
    state.curr_frame_arena = arena->allocate_arena(MegaBytes(1));
    state.prev_frame_arena = arena->allocate_arena(MegaBytes(1));
}

auto UI_Begin() -> void {
    Assert(state.curr_frame_arena);
    state.entry_point = nullptr;
    MemoryArena* temp = state.curr_frame_arena;
    state.curr_frame_arena = state.prev_frame_arena;
    state.prev_frame_arena = temp;
    state.curr_frame_arena->clear();
}

auto UI_End() -> void {
    Assert(state.entry_point);
    Assert(state.parent == nullptr);
}

auto UI_Generate_Render_Commands(RenderGroup* render_group) -> void {
    Assert(state.entry_point);
    UI_Entity* entity = state.entry_point;
    while (entity) {
        vec2 bl = entity->rect.bl;
        vec2 tr = entity->rect.tr;
        vec2 tl = vec2(bl.x, tr.y);
        vec2 br = vec2(tr.x, bl.y);
        auto* rendel_el = PushRenderElement(render_group, RenderEntryBitmap);
        rendel_el->quad = { .bl = bl, .tl = tl, .tr = tr, .br = br };
        rendel_el->offset = vec2(0.0f, 0.0f);
        rendel_el->scale = vec2(1.0, 1.0);
        rendel_el->rotation = 0;
        rendel_el->bitmap_handle = BitmapId{ 0 };
        rendel_el->color = entity->background_color;

        if (entity->right) {
            entity = entity->right;
        }
        else {
            entity = entity->first;
        }
    }
}

auto UI_PushWindow(string8 text, f32 x, f32 y, f32 width, f32 height) -> void {
    Assert(state.parent == nullptr);
    UI_Entity* window = allocate<UI_Entity>(state.curr_frame_arena, 1);
    window->id = hash64(text);
    window->flags = (UI_WidgetFlags)(UI_WidgetFlag_Draggable | UI_WidgetFlag_DrawBackground);

    window->computed_rel_position[0] = x;
    window->computed_rel_position[1] = y;
    window->semantic_size[0] = {
        .kind = UI_SizeKind_Pixels,
        .value = width,
        .strictness = 1.0,
    };
    window->semantic_size[1] = {
        .kind = UI_SizeKind_Pixels,
        .value = height,
        .strictness = 1.0,
    };
    state.parent = window;

    window->rect.bl = vec2(x, y);
    window->rect.tr = vec2(x + width, y + height);
    window->background_color = vec4(255.0, 255.0, 0, 255.0);

    Assert(state.entry_point == nullptr);
    state.entry_point = window;
}

auto UI_PopWindow() -> void {
    Assert(state.parent != nullptr);
    state.parent = nullptr;
}

auto UI_Button(string8 text) -> bool {
    UI_Entity* button = allocate<UI_Entity>(state.curr_frame_arena, 1);
    button->id = hash64(text);
    button->flags = (UI_WidgetFlags)(UI_WidgetFlag_Clickable | UI_WidgetFlag_DrawBackground);

    if (state.parent) {
        button->parent = state.parent;

        if (!state.parent->first) {
            state.parent->first = button;
            state.parent->last = button;
        }
        else {
            UI_Entity* sibling = state.parent->last;
            sibling->right = button;
            button->left = sibling;
            state.parent->last = button;
        }
    }

    button->semantic_size[Axis2_X] = {
        .kind = UI_SizeKind_Pixels,
        .value = 200,
        .strictness = 1.0,
    };
    button->semantic_size[Axis2_Y] = {
        .kind = UI_SizeKind_Pixels,
        .value = 100,
        .strictness = 1.0,
    };

    const i32 padd_and_margin = 10;
    if (button->left) {
        const UI_Entity* sib = button->left;
        button->computed_rel_position[Axis2_X] = sib->computed_rel_position[Axis2_X];
        button->computed_rel_position[Axis2_Y] =
            sib->computed_rel_position[Axis2_Y] + sib->semantic_size[Axis2_Y].value + padd_and_margin;
    }
    else {
        button->computed_rel_position[Axis2_X] = button->parent->computed_rel_position[Axis2_X] + padd_and_margin;
        button->computed_rel_position[Axis2_Y] = button->parent->computed_rel_position[Axis2_Y] + padd_and_margin;
    }

    f32 x = button->computed_rel_position[Axis2_X];
    f32 y = button->computed_rel_position[Axis2_Y];
    f32 width = button->semantic_size[Axis2_X].value;
    f32 height = button->semantic_size[Axis2_Y].value;
    button->rect.bl = vec2(x, y);
    button->rect.tr = vec2(x + width, y + height);
    button->background_color = vec4(0.0, 255.0, 0, 255.0);

    return false;
}
