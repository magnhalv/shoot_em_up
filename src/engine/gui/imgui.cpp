#pragma once

#include <core/hash.hpp>

#include "imgui.hpp"

struct UI_State {
    MemoryArena* arena;
    UI_Entity* parent; // TODO: Make this a stack
    UI_Entity* entry_point;
};

UI_State state{};

auto UI_Initialize(MemoryArena* arena) -> void {
    state.arena = arena->allocate_arena(MegaBytes(1));
}

auto UI_Begin() -> void {
    Assert(state.arena);
    state.entry_point = nullptr;
}

auto UI_End() -> void {
    Assert(state.entry_point);
    Assert(state.parent == nullptr);
}

auto UI_Generate_Render_Commands(RenderGroup* render_group) -> void {
    Assert(state.entry_point);
    UI_Entity* entity = state.entry_point;
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
}

auto UI_PushWindow(string8 text, f32 x, f32 y, f32 width, f32 height) -> void {
    Assert(state.parent == nullptr);
    UI_Entity* window = allocate<UI_Entity>(state.arena, 1);
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

    Assert(state.entry_point == nullptr);
    state.entry_point = window;
}

auto UI_PopWindow() -> void {
    Assert(state.parent != nullptr);
    state.parent = nullptr;
}

auto UI_Button(string8 text) -> bool {
    UI_Entity* button = allocate<UI_Entity>(state.arena, 1);
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

    button->semantic_size[0] = {
        .kind = UI_SizeKind_Pixels,
        .value = 200,
        .strictness = 1.0,
    };
    button->semantic_size[1] = {
        .kind = UI_SizeKind_Pixels,
        .value = 100,
        .strictness = 1.0,
    };

    return false;
}
