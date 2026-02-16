#pragma once

#include <cmath>
#include <platform/types.h>

#include <core/hash.hpp>
#include <math/util.hpp>

#include "imgui.hpp"

global_variable UI_Context* global_context;

const i32 Padding_Margin_X = 10;
const i32 Padding_Margin_Y = 5;
const i32 Space_Width = 8;

auto UI_GetCodePointsTotalLength(List<CodePoint>* code_points) -> f32 {
    f32 result = 0.0f;
    for (const auto& cp : *code_points) {
        if (cp.c == ' ') {
            result += Space_Width;
        }
        else {
            result += cp.xadvance;
        }
    }
    return result;
}

auto UI_GetCodePoints(string8 str, LoadedFont* font, MemoryArena* arena) -> List<CodePoint> {
    List<CodePoint> result;
    result.init(arena, (i32)str.size);
    for (i32 i = 0; i < str.size; i++) {
        i32 code_point_idx = (i32)str.data[i] - font->code_point_first;
        CodePoint cp = font->code_points[code_point_idx];
        cp.c = str.data[i];
        result.push(cp);
    }
    return result;
}

auto UI_CreateContext(MemoryArena* arena) -> UI_Context* {
    UI_Context* result = allocate<UI_Context>(arena);
    result->frame_arenas[0] = arena->allocate_arena(MegaBytes(1));
    result->frame_arenas[1] = arena->allocate_arena(MegaBytes(1));
    result->arena = arena->allocate_arena(MegaBytes(1));
    result->style = &default_style;

    return result;
}

auto UI_SetContext(UI_Context* context) -> void {
    global_context = context;
}

auto UI_SetFont(i32 texture_id, LoadedFont* font) -> void {
    Assert(global_context);
    global_context->texture_id = texture_id;
    global_context->font = font;
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
    Assert(global_context);

    global_context->flip_and_clear_state();

    global_context->state()->parents.init(global_context->frame_arena(), 100);
    UI_Entity* entry_point = allocate<UI_Entity>(global_context->frame_arena(), 1);
    entry_point->computed_size[Axis2_X] = (f32)client_width;
    entry_point->computed_size[Axis2_Y] = (f32)client_height;
    entry_point->semantic_position[Axis2_X] = UI_Fixed(0.0f);
    entry_point->semantic_position[Axis2_Y] = UI_Fixed(0.0f);
    global_context->state()->parents.push(entry_point);

    global_context->state()->client_width = client_width;
    global_context->state()->client_height = client_height;

    Assert(global_context->prev_state());
    if (global_context->prev_state()->parents.count() > 0) {
        UI_Entity* hovered_entity = global_context->prev_state()->parents[0]->first;
        global_context->state()->active_item = 0;
        global_context->state()->hot_item = 0;
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
            if (hovered_entity && global_context->prev_state()->hot_item == hovered_entity->id) {
                global_context->state()->click_released_item = hovered_entity->id;
            }
        }
        else if (global_context->prev_state()->hot_item != 0) {
            global_context->state()->hot_item = global_context->prev_state()->hot_item;
            global_context->state()->active_item = global_context->prev_state()->active_item;
        }

        if (hovered_entity && global_context->state()->hot_item == 0) {
            global_context->state()->active_item = hovered_entity->id;

            if (mouse->left.is_pressed_this_frame()) {
                global_context->state()->hot_item = hovered_entity->id;
            }
        }
    }
}

auto calculate_children_size(UI_Entity* entity, Axis2 axis) -> f32 {
    f32 size = 0.0f;
    UI_Entity* child = entity->first;

    if (axis == Axis2_X) {
        while (child) {
            size += child->computed_size[Axis2_X];
            if (child->right) {
                size += child->margin[UI_Direction_Right];
            }
            child = child->right;
        }
    }
    else {
        while (child) {
            size += child->computed_size[Axis2_Y];
            if (child->right) {
                size += child->margin[UI_Direction_Down];
            }
            child = child->right;
        }
    }

    return size;
}

auto calculate_grow_size(UI_Entity* entity) -> void {
    {
        f32 remaning_size =
            entity->computed_size[Axis2_X] - entity->padding[UI_Direction_Left] - entity->padding[UI_Direction_Right];
        if (entity->flex_direction == UI_FlexDirection_Column) {
            f32 children_size = calculate_children_size(entity, Axis2_X);
            remaning_size -= children_size;
            if (remaning_size > 0.0f) {
                UI_Entity* child = entity->first;
                while (child) {
                    if (child->semantic_size[Axis2_X].kind == UI_SizeKind_Grow) {
                        child->computed_size[Axis2_X] += remaning_size;
                        break;
                    }
                    child = child->right;
                }
            }
        }
        else {
            UI_Entity* child = entity->first;
            while (child) {
                if (child->semantic_size[Axis2_X].kind == UI_SizeKind_Grow) {
                    child->computed_size[Axis2_X] = remaning_size;
                }
                child = child->right;
            }
        }
    }

    {
        f32 remaning_size = entity->computed_size[Axis2_Y] - entity->padding[UI_Direction_Up] - entity->padding[UI_Direction_Down];
        if (entity->flex_direction == UI_FlexDirection_Row) {
            f32 children_size = calculate_children_size(entity, Axis2_Y);
            remaning_size -= children_size;
            if (remaning_size > 0.0f) {
                UI_Entity* child = entity->first;
                while (child) {
                    if (child->semantic_size[Axis2_Y].kind == UI_SizeKind_Grow) {
                        child->computed_size[Axis2_Y] += remaning_size;
                        break;
                    }
                    child = child->right;
                }
            }
        }
        else {
            UI_Entity* child = entity->first;
            while (child) {
                if (child->semantic_size[Axis2_Y].kind == UI_SizeKind_Grow) {
                    child->computed_size[Axis2_Y] = remaning_size;
                }
                child = child->right;
            }
        }
    }
    {
        UI_Entity* child = entity->first;
        while (child) {
            calculate_grow_size(child);
            child = child->right;
        }
    }
}

auto calculate_percent_of_parent_size(UI_Entity* entity) -> void {
    {
        switch (entity->semantic_size[Axis2_X].kind) {
        case UI_SizeKind_PercentOfParent: {
            Assert(entity->parent);
            entity->computed_size[Axis2_X] =                     //
                (                                                //
                    entity->parent->computed_size[Axis2_X] -     //
                    entity->parent->padding[UI_Direction_Left] - //
                    entity->parent->padding[UI_Direction_Right]  //
                    )                                            //
                * entity->semantic_size[Axis2_X].value;
        } break;

        case UI_SizeKind_Pixels:
        case UI_SizeKind_ChildrenSum:
        case UI_SizeKind_Grow:
        case UI_SizeKind_TextContent: {

        } break;
        }
    }

    {
        switch (entity->semantic_size[Axis2_Y].kind) {
        case UI_SizeKind_PercentOfParent: {
            Assert(entity->parent);
            entity->computed_size[Axis2_Y] =                   //
                (                                              //
                    entity->parent->computed_size[Axis2_Y] -   //
                    entity->parent->padding[UI_Direction_Up] - //
                    entity->parent->padding[UI_Direction_Down] //
                    )                                          //
                * entity->semantic_size[Axis2_Y].value;
            break;
        }
        case UI_SizeKind_Pixels:
        case UI_SizeKind_Grow:
        case UI_SizeKind_ChildrenSum:
        case UI_SizeKind_TextContent: {

        } break;
        }
    }

    {
        UI_Entity* child = entity->first;
        while (child) {
            calculate_percent_of_parent_size(child);
            child = child->right;
        }
    }
}

auto calculate_size(UI_Entity* entity) -> void {
    {
        UI_Entity* child = entity->first;
        while (child) {
            calculate_size(child);
            child = child->right;
        }
    }
    {
        switch (entity->semantic_size[Axis2_X].kind) {
        case UI_SizeKind_Pixels: {
            entity->computed_size[Axis2_X] = entity->semantic_size[Axis2_X].value;
            break;
        }
        case UI_SizeKind_Grow:
        case UI_SizeKind_ChildrenSum: {
            f32 size = 0.0f;
            UI_Entity* child = entity->first;
            if (entity->flex_direction == UI_FlexDirection_Row) {
                while (child) {
                    size = hm::max(size, child->computed_size[Axis2_X]);
                    child = child->right;
                }
            }
            else {
                while (child) {
                    size += child->computed_size[Axis2_X];
                    if (child->right) {
                        size += child->margin[UI_Direction_Right];
                    }
                    child = child->right;
                }
            }
            entity->computed_size[Axis2_X] = size + entity->padding[UI_Direction_Left] + entity->padding[UI_Direction_Right];
        } break;

        case UI_SizeKind_PercentOfParent:
        case UI_SizeKind_TextContent: {

        } break;
        }
    }

    {
        switch (entity->semantic_size[Axis2_Y].kind) {
        case UI_SizeKind_Pixels: {
            entity->computed_size[Axis2_Y] = entity->semantic_size[Axis2_Y].value;
            break;
        }
        case UI_SizeKind_ChildrenSum: {
            f32 size = 0.0f;
            UI_Entity* child = entity->first;
            if (entity->flex_direction == UI_FlexDirection_Column) {
                while (child) {
                    size = hm::max(size, child->computed_size[Axis2_Y]);
                    child = child->right;
                }
            }
            else {
                while (child) {
                    size += child->computed_size[Axis2_Y];
                    if (child->right) {
                        size += child->margin[UI_Direction_Right];
                    }
                    child = child->right;
                }
            }
            entity->computed_size[Axis2_Y] = size + entity->padding[UI_Direction_Up] + entity->padding[UI_Direction_Down];
        } break;

        case UI_SizeKind_PercentOfParent:
        case UI_SizeKind_TextContent:
        case UI_SizeKind_Grow: break;
        }
    }
}

auto calculate_position_axis(UI_Entity* entity) {
    {
        f32 value = entity->semantic_position[Axis2_X].value;

        switch (entity->semantic_position[Axis2_X].kind) {
        case UI_PositionKind_Flex: {
            Assert(entity->parent);
            f32 grown = entity->parent->flex_direction == UI_FlexDirection_Column ?
                entity->computed_size[Axis2_X] + entity->margin[UI_Direction_Right] :
                0.0f;
            entity->position[Axis2_X] = entity->parent->computed_child_offset[Axis2_X];
            entity->parent->computed_child_offset[Axis2_X] += grown;
        } break;
        case UI_PositionKind_Fixed: {
            entity->position[Axis2_X] = value;
        } break;
        case UI_PositionKind_Relative: {
            Assert(entity->parent);
            entity->position[Axis2_X] = entity->parent->position[Axis2_X] + value;
        } break;
        }
    }
    {
        f32 value = entity->semantic_position[Axis2_Y].value;

        switch (entity->semantic_position[Axis2_Y].kind) {
        case UI_PositionKind_Flex: {
            Assert(entity->parent);
            f32 grown = entity->parent->flex_direction == UI_FlexDirection_Row ?
                entity->computed_size[Axis2_Y] + entity->margin[UI_Direction_Down] :
                0.0f;
            entity->position[Axis2_Y] = entity->parent->computed_child_offset[Axis2_Y];
            entity->parent->computed_child_offset[Axis2_Y] -= grown;
        } break;
        case UI_PositionKind_Fixed: {
            entity->position[Axis2_Y] = value;
        } break;
        case UI_PositionKind_Relative: {
            Assert(entity->parent);
            entity->position[Axis2_Y] = entity->parent->position[Axis2_Y] + value;
        } break;
        }
    }
}

auto calculate_positions(UI_Entity* entity) -> void {
    Assert(entity);
    calculate_position_axis(entity);
    entity->computed_child_offset[Axis2_X] = entity->position[Axis2_X] + entity->padding[UI_Direction_Right];
    entity->computed_child_offset[Axis2_Y] = entity->position[Axis2_Y] - entity->padding[UI_Direction_Down];

    {
        UI_Entity* child = entity->first;
        while (child) {
            calculate_positions(child);
            child = child->right;
        }
    }
}

auto UI_End() -> void {
    Assert(global_context->state()->parents.count() == 1);

    UI_Entity* entity = global_context->state()->parents[0];
    calculate_size(entity);
    calculate_grow_size(entity);
    calculate_percent_of_parent_size(entity);
    calculate_positions(entity);
}

auto UI_SetLayout(UI_Layout layout) -> void {
    global_context->state()->layout = layout;
}

auto UI_Generate_Render_Commands(RenderCommands* render_group) -> void {
    Assert(global_context->state()->parents.count() == 1);
    UI_Entity* entity = global_context->state()->parents[0]->first;
    StackList<UI_Entity*, 1024> next = {};
    while (entity) {
        f32 x = entity->position[Axis2_X];
        f32 y = entity->position[Axis2_Y];
        f32 el_width = entity->computed_size[Axis2_X];
        f32 el_height = entity->computed_size[Axis2_Y];
        if (entity->flags & UI_WidgetFlag_DrawBackground) {

            if (global_context->state()->layout.layout_direction == UI_Direction_Up) {
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
            auto* rendel_el = PushRenderElement(render_group, RenderEntryBitmap, entity->z_index);
            rendel_el->quad = { .bl = vec2(-0.5f, -0.5f), .tl = vec2(-0.5f, 0.5f), .tr = vec2(0.5f, 0.5f), .br = vec2(0.5f, -0.5f) };
            rendel_el->offset = vec2(x + el_width / 2, y - el_height / 2);
            rendel_el->scale = vec2(el_width, el_height);
            rendel_el->rotation = 0;
            rendel_el->texture_id = 0;
            rendel_el->color = entity->background_color;
        }

        if (entity->flags & UI_WidgetFlag_DrawText) {
            LoadedFont* font = global_context->font;
            y = y - (entity->computed_size[Axis2_Y] - font->font_height) * 0.5f - font->ascent;
            for (const CodePoint& cp : entity->text) {
                if (cp.c == ' ') {
                    x += Space_Width;
                    continue;
                }

                ivec2 uv_min = ivec2(cp.x0, cp.y0);
                ivec2 uv_max = ivec2(cp.x1, cp.y1);
                auto glyph_width = uv_max.x - uv_min.x;
                auto glyph_height = uv_max.y - uv_min.y;

                auto* el = PushRenderElement(render_group, RenderEntryBitmap, entity->z_index);
                el->uv_min = uv_min;
                el->uv_max = uv_max;

                el->quad = { .bl = vec2(-0.5f, -0.5f), .tl = vec2(-0.5f, 0.5f), .tr = vec2(0.5f, 0.5f), .br = vec2(0.5f, -0.5f) };
                // Offset with 0.5f to not make the bilinear interpolation not make the font blurry.
                el->offset = vec2((glyph_width) / 2.0f + x + cp.xoff + 0.5f,
                    (glyph_height / 2.0f) + y - (glyph_height + cp.yoff) + 0.5f);
                el->scale = vec2((f32)glyph_width, (f32)glyph_height);
                el->rotation = 0.0f;
                el->color = vec4(255.0f, 0.0f, 0.0f, 255.0f);
                el->texture_id = global_context->texture_id;

                // Offset with 0.5f to not make the bilinear interpolation not make the font blurry.
                x += floorf(cp.xadvance + 0.5f);
            }
        }

        if (entity->first) {
            if (entity->right) {
                next.push(entity->right);
            }
            entity = entity->first;
        }
        else if (entity->right) {
            entity = entity->right;
        }
        else if (next.count() > 0) {
            entity = *next.last();
            next.pop();
        }
        else {
            entity = nullptr;
        }
    }
}

auto make_element(UI_Entity* entity, UI_Entity_Status* status, UI_Position x = {}, UI_Position y = {},
    UI_Size width = {}, UI_Size height = {}) {

    entity->parent = *global_context->state()->parents.last();
    if (!entity->parent->first) {
        entity->parent->first = entity;
        entity->parent->last = entity;
    }
    else {
        UI_Entity* sibling = entity->parent->last;
        sibling->right = entity;
        entity->left = sibling;
        entity->parent->last = entity;
    }

    entity->margin[UI_Direction_Up] = 0;
    entity->margin[UI_Direction_Right] = 0;
    entity->margin[UI_Direction_Down] = 0;
    entity->margin[UI_Direction_Left] = 0;

    entity->padding[UI_Direction_Up] = 10;
    entity->padding[UI_Direction_Right] = 10;
    entity->padding[UI_Direction_Down] = 10;
    entity->padding[UI_Direction_Left] = 10;

    entity->background_color = global_context->style->background_color;

    entity->semantic_position[Axis2_X] = x;
    entity->semantic_position[Axis2_Y] = y;

    if (global_context->state()->click_released_item == entity->id) {
        status->click_released = true;
    }

    if (global_context->state()->hot_item == entity->id) {
        status->pressed = true;
    }
    else if (global_context->state()->active_item == entity->id) {
        status->hovered = true;
        if (global_context->state()->active_item != global_context->prev_state()->active_item) {
            status->first_hovered = true;
        }
    }

    entity->semantic_size[Axis2_X] = width;
    entity->semantic_size[Axis2_Y] = height;
    entity->computed_size[Axis2_X] = width.value;
    entity->computed_size[Axis2_Y] = height.value;

    entity->z_index = global_context->style->z_index;
    entity->flex_direction = global_context->style->flex_direction;
}

auto UI_PushWindow(string8 text, UI_Position x, UI_Position y, UI_Size width, UI_Size height) -> void {
    UI_Entity* window = allocate<UI_Entity>(global_context->frame_arena(), 1);
    window->id = hash64(text);
    window->name = text.data;
    window->flags = (UI_WidgetFlags)(UI_WidgetFlag_Draggable | UI_WidgetFlag_DrawBackground);

    y.value = global_context->state()->client_height - y.value;

    window->background_color = global_context->style->background_color;

    UI_Entity_Status status;
    make_element(window, &status, x, y, width, height);
    global_context->state()->parents.push(window);
}

auto get_y(f32 y, i32 client_height, UI_Direction direction) -> f32 {
    return direction == UI_Direction_Up ? y : (f32)client_height - y;
}

auto UI_PopWindow() -> void {
    Assert(global_context->state()->parents.count() > 1);
    global_context->state()->parents.pop();
}

const vec4 Button_Color = vec4(1.0f, 50.0f, 90.0f, 255.0f);
const vec4 Hover_Button_Color = vec4(10.0f, 60.0f, 100.0f, 255.0f);
const vec4 Clicked_Button_Color = vec4(20.0f, 70.0f, 120.0f, 255.0f);

auto UI_Button(string8 text) -> UI_Entity_Status {
    UI_Entity* button = allocate<UI_Entity>(global_context->frame_arena(), 1);
    UI_Entity_Status result = {};

    button->id = hash64(text);
    button->flags = (UI_WidgetFlags)(UI_WidgetFlag_Clickable | UI_WidgetFlag_DrawBackground);
    make_element(button, &result);

    button->text = UI_GetCodePoints(text, global_context->font, global_context->frame_arena());

    button->semantic_size[Axis2_X] = {
        .kind = UI_SizeKind_Pixels,
        .value = UI_GetCodePointsTotalLength(&button->text) + button->padding[UI_Direction_Left] + button->padding[UI_Direction_Right],
        .strictness = 1.0f,
    };
    button->semantic_size[Axis2_Y] = {
        .kind = UI_SizeKind_Pixels,
        .value = global_context->font->font_height + button->padding[UI_Direction_Up] + button->padding[UI_Direction_Down],
        .strictness = 1.0f,
    };

    const i32 y_dir = (i32)global_context->state()->layout.layout_direction;
    // Maybe make this a post pass step?
    if (button->left == nullptr) {
        f32* computed_rel_position = button->parent->computed_rel_position;
        button->computed_rel_position[Axis2_X] = computed_rel_position[Axis2_X] + Padding_Margin_X;
        button->computed_rel_position[Axis2_Y] = computed_rel_position[Axis2_Y] + (y_dir * Padding_Margin_X);
    }
    else {
        const UI_Entity* sib = button->left;
        button->computed_rel_position[Axis2_X] = sib->computed_rel_position[Axis2_X];
        button->computed_rel_position[Axis2_Y] = sib->computed_rel_position[Axis2_Y] + //
            (y_dir * (sib->semantic_size[Axis2_Y].value + sib->margin[UI_Direction_Down]));
    }

    if (global_context->state()->click_released_item == button->id) {
        result.click_released = true;
    }

    if (global_context->state()->hot_item == button->id) {
        button->background_color = Clicked_Button_Color;
    }
    else if (global_context->state()->active_item == button->id) {
        button->background_color = Hover_Button_Color;
    }
    else {
        button->background_color = Button_Color;
    }

    return result;
}

auto UI_Text(string8 text) -> UI_Entity_Status {
    UI_Entity* entity = allocate<UI_Entity>(global_context->frame_arena(), 1);
    UI_Entity_Status result = {};

    entity->id = hash64(text);
    entity->flags = (UI_WidgetFlags)(UI_WidgetFlag_DrawText);
    entity->text = UI_GetCodePoints(text, global_context->font, global_context->frame_arena());
    f32 width_pixels = UI_GetCodePointsTotalLength(&entity->text);
    f32 height_pixels = global_context->font->font_height;
    UI_Size width = { .kind = UI_SizeKind_Pixels, .value = width_pixels, .strictness = 1.0 };
    UI_Size height = { .kind = UI_SizeKind_Pixels, .value = height_pixels, .strictness = 1.0 };

    make_element(entity, &result, {}, {}, width, height);

    entity->margin[UI_Direction_Up] = 0.0f;
    entity->margin[UI_Direction_Right] = 0.0f;
    entity->margin[UI_Direction_Down] = 0.0f;
    entity->margin[UI_Direction_Left] = 0.0f;

    entity->padding[UI_Direction_Up] = 0.0f;
    entity->padding[UI_Direction_Right] = 0.0f;
    entity->padding[UI_Direction_Down] = 0.0f;
    entity->padding[UI_Direction_Left] = 0.0f;

    return result;
}

auto UI_Box(string8 id, UI_Size width, UI_Size height) -> UI_Entity_Status {
    UI_Entity* box = allocate<UI_Entity>(global_context->frame_arena(), 1);
    UI_Entity_Status result = {};

    box->id = hash64(id);
    box->flags = (UI_WidgetFlags)(UI_WidgetFlag_DrawBackground);

    make_element(box, &result, {}, {}, width, height);

    box->margin[UI_Direction_Up] = 0.0f;
    box->margin[UI_Direction_Right] = 0.0f;
    box->margin[UI_Direction_Down] = 0.0f;
    box->margin[UI_Direction_Left] = 0.0f;

    return result;
}

auto UI_PushStyleSize(UI_Size* width, UI_Size* height) -> void {
    global_context->style_overrides.width.push(global_context->style->width);
    global_context->style->width = *width;

    global_context->style_overrides.height.push(global_context->style->height);
    global_context->style->height = *height;
}

auto UI_PopStyleSize() -> void {
    global_context->style->width = *global_context->style_overrides.width.last();
    global_context->style_overrides.width.pop();

    global_context->style->height = *global_context->style_overrides.height.last();
    global_context->style_overrides.height.pop();
}

auto UI_PushStyleZIndex(i32 z_index) -> void {
    global_context->style_overrides.z_index.push(global_context->style->z_index);
    global_context->style->z_index = z_index;
}

auto UI_PopStyleZIndex() -> void {
    global_context->style->z_index = *global_context->style_overrides.z_index.last();
    global_context->style_overrides.z_index.pop();
}

auto UI_PushStyleBackgroundColor(vec4 color) -> void {
    global_context->style_overrides.background_colors.push(global_context->style->background_color);
    global_context->style->background_color = color;
}
auto UI_PopStyleBackgroundColor() -> void {
    global_context->style->background_color = *global_context->style_overrides.background_colors.last();
    global_context->style_overrides.background_colors.pop();
}

auto UI_PushStyleFlexDirection(UI_FlexDirection direction) -> void {
    global_context->style_overrides.flex_direction.push(global_context->style->flex_direction);
    global_context->style->flex_direction = direction;
}
auto UI_PopStyleFlexDirection() -> void {
    global_context->style->flex_direction = *global_context->style_overrides.flex_direction.last();
    global_context->style_overrides.flex_direction.pop();
}
