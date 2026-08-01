
#include <sph/app.h>
#include <sph/darray.h>
#include <sph/ui.h>

static ui_context ui_ctx;

static void ui_destroy(ui_element *root)
{
    if (!root)
    {
        return;
    }

    if (root->children)
    {
        for (u32 i = 0; i < darray_len(root->children); i++)
        {
            ui_destroy(&root->children[i]);
        }

        darray_destroy(root->children);
    }

    if (!root->parent)
    {
        SDL_free(root);
    }
}

static void ui_mouse_over_elements(ui_element *root, v2 mouse_pos, ui_id **pointer_over_ids)
{
    if (!root)
    {
        return;
    }

    if (root->pos.x < mouse_pos.x && root->pos.y < mouse_pos.y &&
        root->pos.x + root->width.min_max.min > mouse_pos.x &&
        root->pos.y + root->height.min_max.min > mouse_pos.y)
    {
        darray_push((void **)pointer_over_ids, &root->id);
    }

    if (!root->children)
    {
        return;
    }

    for (u32 i = 0; i < darray_len(root->children); i++)
    {
        ui_mouse_over_elements(&root->children[i], mouse_pos, pointer_over_ids);
    }
}

static void ui_data_collect(ui_element *root, ui_previous_data **world_positions)
{
    if (!root)
    {
        return;
    }

    ui_previous_data pos = {
        .id   = root->id,
        .pos  = v2make(root->pos.x, root->pos.y),
        .size = v2make(root->width.min_max.min, root->height.min_max.min),
    };

    darray_push((void **)world_positions, &pos);

    if (!root->children)
    {
        return;
    }

    for (u32 i = 0; i < darray_len(root->children); i++)
    {
        ui_data_collect(&root->children[i], world_positions);
    }
}

void ui_update(input *input)
{
    if (!ui_ctx.pointer_over_ids)
    {
        ui_ctx.pointer_over_ids = darray_create(sizeof(ui_id));
    }
    if (!ui_ctx.previous_data)
    {
        ui_ctx.previous_data = darray_create(sizeof(ui_previous_data));
    }

    darray_len_set(ui_ctx.pointer_over_ids, 0);
    darray_len_set(ui_ctx.previous_data, 0);

    ui_mouse_over_elements(ui_ctx.root, input->mouse_pos, &ui_ctx.pointer_over_ids);
    ui_data_collect(ui_ctx.root, &ui_ctx.previous_data);

    ui_ctx.element_count = 0;

    if (ui_ctx.root)
    {
        ui_destroy(ui_ctx.root);
        ui_ctx.root = NULL;
    }
}

ui_element *ui_open(ui_element *parent, ui_element element)
{
    if (element.layout == LAYOUT_TO_RIGHT && element.child_align == 0)
    {
        element.child_align = TOP;
    }

    if (element.width.type != UI_SIZE_PERCENT && element.width.min_max.max == 0.0f)
    {
        element.width.min_max.max = FLT_MAX;
    }
    if (element.height.type != UI_SIZE_PERCENT && element.height.min_max.max == 0.0f)
    {
        element.height.min_max.max = FLT_MAX;
    }

    element.id = ui_ctx.element_count++;

    if (!parent)
    {
        void *data = SDL_malloc(sizeof(element));
        SDL_memcpy(data, &element, sizeof(element));

        if (ui_ctx.root)
        {
            SDL_Log("[UI] More than one root element.. quitting.");
            assert(0);
        }
        ui_ctx.root = data;

        if (ui_ctx.open_elements.count + 1 < ARRAY_COUNT(ui_ctx.open_elements.elements))
        {
            ui_ctx.open_elements.elements[ui_ctx.open_elements.count++] = data;
        }

        return data;
    }

    if (!parent->children)
    {
        parent->children = darray_create(sizeof(ui_element));
    }

    element.parent            = parent;
    ui_element *child_address = darray_push((void **)&parent->children, &element);

    if (ui_ctx.open_elements.count + 1 < ARRAY_COUNT(ui_ctx.open_elements.elements))
    {
        ui_ctx.open_elements.elements[ui_ctx.open_elements.count++] = child_address;
    }
    return child_address;
}

void ui_close(ui_element *element)
{
    if (!element)
    {
        return;
    }

    else if (element->width.type == UI_SIZE_FIT)
    {
        f32 min = element->width.min_max.min;
        if (element->text.chars)
        {
            element->width.min_max.min = measure_text(element->text.font, element->text.font_size, "%s", element->text.chars);
        }
        element->width.min_max.min += element->padding.left + element->padding.right;
        element->width.min_max.min = CLAMP(element->width.min_max.min, min, element->width.min_max.max);
    }
    if (element->height.type == UI_SIZE_FIT)
    {
        f32 min = element->height.min_max.min;
        if (element->text.chars)
        {
            element->height.min_max.min = element->text.font_size;
        }
        element->height.min_max.min += element->padding.top + element->padding.bottom;
        element->height.min_max.min = CLAMP(element->height.min_max.min, min, element->height.min_max.max);
    }

    ui_element *parent = element->parent;
    if (!parent)
    {
        return;
    }

    f32 child_gap = (darray_len(parent->children) - 1) * parent->child_gap;
    if (parent->width.type == UI_SIZE_FIT)
    {
        if (parent->layout == LAYOUT_TO_RIGHT)
        {
            parent->width.min_max.min += element->width.min_max.min + child_gap;
        }
        else if (parent->layout == LAYOUT_TO_BOTTOM)
        {
            parent->width.min_max.min = MAX(element->width.min_max.min, parent->width.min_max.min);
        }
    }
    if (parent->height.type == UI_SIZE_FIT)
    {
        if (parent->layout == LAYOUT_TO_RIGHT)
        {
            parent->height.min_max.min = MAX(element->height.min_max.min, parent->height.min_max.min);
        }
        else if (parent->layout == LAYOUT_TO_BOTTOM)
        {
            parent->height.min_max.min += element->height.min_max.min + child_gap;
        }
    }
}

static void ui_grow_resolve(ui_element *root, u32 window_width, u32 window_height)
{
    // NOTE: special case if element is the root (only element without parent)
    if (!root->parent)
    {
        if (root->width.type == UI_SIZE_GROW)
        {
            root->width.min_max.min = window_width - root->pos.x;
            root->width.min_max.min = MIN(root->width.min_max.min, root->width.min_max.max - root->pos.x);
        }
        if (root->height.type == UI_SIZE_GROW)
        {
            root->height.min_max.min = window_height - root->pos.y;
            root->height.min_max.min = MIN(root->height.min_max.min, root->height.min_max.max - root->pos.y);
        }
    }

    if (!root->children)
    {
        return;
    }

    f32 remaining_width  = root->width.min_max.min;
    f32 remaining_height = root->height.min_max.min;

    remaining_width -= root->padding.left + root->padding.right;
    remaining_height -= root->padding.top + root->padding.bottom;

    f32 remaining_along  = root->layout == LAYOUT_TO_RIGHT ? remaining_width : remaining_height;
    f32 remaining_across = root->layout == LAYOUT_TO_RIGHT ? remaining_height : remaining_width;

    f32 smallest_growable_size = FLT_MAX;
    u32 smallest_growable      = 0;
    u32 growable_count         = 0;

    // NOTE: All sizes of regular sized elements
    for (u32 i = 0; i < darray_len(root->children); i++)
    {
        ui_element *child             = &root->children[i];
        ui_size    *along_layout_size = root->layout == LAYOUT_TO_RIGHT ? &child->width : &child->height;

        if (along_layout_size->type == UI_SIZE_GROW)
        {
            ++growable_count;

            if (along_layout_size->min_max.min < smallest_growable_size)
            {
                smallest_growable_size = along_layout_size->min_max.min;
                smallest_growable      = i;
            }
        }

        remaining_along -= along_layout_size->min_max.min;
    }
    remaining_along -= (darray_len(root->children) - 1) * root->child_gap;

    while (growable_count && remaining_along > 0.0f)
    {
        f32 smallest        = root->layout == LAYOUT_TO_RIGHT ? root->children[smallest_growable].width.min_max.min : root->children[smallest_growable].height.min_max.min;
        f32 second_smallest = FLT_MAX;
        f32 size_to_add     = remaining_along;

        for (u32 i = 0; i < darray_len(root->children); i++)
        {
            ui_element *child             = &root->children[i];
            ui_size    *along_layout_size = root->layout == LAYOUT_TO_RIGHT ? &child->width : &child->height;

            if (along_layout_size->type != UI_SIZE_GROW)
            {
                continue;
            }

            if (along_layout_size->min_max.min < smallest)
            {
                second_smallest = smallest;
                smallest        = along_layout_size->min_max.min;
            }
            else if (along_layout_size->min_max.min > smallest)
            {
                second_smallest = MIN(second_smallest, along_layout_size->min_max.min);
                size_to_add     = second_smallest - smallest;
            }
        }

        size_to_add = MIN(size_to_add, (f32)remaining_along / (f32)growable_count);

        if (size_to_add < 0.1f)
        {
            break;
        }

        for (u32 i = 0; i < darray_len(root->children); i++)
        {
            ui_element *child             = &root->children[i];
            ui_size    *along_layout_size = root->layout == LAYOUT_TO_RIGHT ? &child->width : &child->height;

            if (along_layout_size->type != UI_SIZE_GROW)
            {
                continue;
            }

            if (along_layout_size->min_max.min == smallest)
            {
                along_layout_size->min_max.min += size_to_add;
                remaining_along -= size_to_add;
            }
        }
    }

    for (u32 i = 0; i < darray_len(root->children); i++)
    {
        ui_element *child              = &root->children[i];
        ui_size    *across_layout_size = root->layout == LAYOUT_TO_RIGHT ? &child->height : &child->width;

        if (across_layout_size->type == UI_SIZE_GROW)
        {
            across_layout_size->min_max.min += (remaining_across - across_layout_size->min_max.min);
        }
    }

    for (u32 i = 0; i < darray_len(root->children); i++)
    {
        ui_grow_resolve(&root->children[i], window_width, window_height);
    }
}

static void ui_percent_resolve(ui_element *root, u32 window_width, u32 window_height)
{
    if (!root)
    {
        return;
    }

    if (root->width.type == UI_SIZE_PERCENT)
    {
        if (!root->parent)
        {
            root->width.min_max.min = window_width * root->width.percent;
        }
        else
        {
            f32 child_gap = 0.0f;
            if (root->parent->layout == LAYOUT_TO_RIGHT)
            {
                child_gap = (darray_len(root->parent->children) - 1) * root->parent->child_gap;
            }
            root->width.min_max.min = root->width.percent * (root->parent->width.min_max.min - root->parent->padding.left - root->padding.right - child_gap);
        }
    }

    if (root->height.type == UI_SIZE_PERCENT)
    {
        if (!root->parent)
        {
            root->height.min_max.min = window_height * root->height.percent;
        }
        else
        {
            f32 child_gap = 0.0f;
            if (root->parent->layout == LAYOUT_TO_BOTTOM)
            {
                child_gap = (darray_len(root->parent->children) - 1) * root->parent->child_gap;
            }
            root->height.min_max.min = root->height.percent * (root->parent->height.min_max.min - root->parent->padding.bottom - root->padding.top - child_gap);
        }
    }

    if (!root->children)
    {
        return;
    }

    for (u32 i = 0; i < darray_len(root->children); i++)
    {
        ui_percent_resolve(&root->children[i], window_width, window_height);
    }
}

static void ui_draw_helper(app *app, ui_element *root)
{
    if (!root)
    {
        return;
    }

    draw_quad(app, root->pos, v2make(root->width.min_max.min, root->height.min_max.min), root->roundness, root->color, NULL, NULL);
    if (root->text.chars)
    {
        // TODO: Fix this
        v2 text_pos = root->pos;
        text_pos.y -= root->text.font_size * 0.25f;

        draw_text(app, root->text.font, text_pos, root->text.font_size, "%s", root->text.chars);
    }

    if (!root->children)
    {
        return;
    }

    f32 offset = root->layout == LAYOUT_TO_RIGHT ? root->padding.left : root->padding.top;

    for (u32 i = 0; i < darray_len(root->children); i++)
    {
        ui_element *child = &root->children[i];

        child->pos = v2add(root->pos, child->pos);

        if (root->layout == LAYOUT_TO_RIGHT)
        {
            switch (root->child_align)
            {
            case TOP:
            {
                child->pos.y += root->padding.top;
            }
            break;
            case BOTTOM:
            {
                child->pos.y += root->height.min_max.min - root->padding.bottom - child->height.min_max.min;
            }
            break;
            case CENTER:
            {
                child->pos.y += (root->height.min_max.min - root->padding.top - root->padding.bottom) * 0.5f - child->height.min_max.min * 0.5f + root->padding.top;
            }
            break;
            default:
            {
                SDL_Log("[UI] Unkown child alignement.");
            }
            break;
            }
            child->pos.x += offset;
            offset += child->width.min_max.min + root->child_gap;
        }
        else if (root->layout == LAYOUT_TO_BOTTOM)
        {
            switch (root->child_align)
            {
            case LEFT:
            {
                child->pos.x += root->padding.left;
            }
            break;
            case RIGHT:
            {
                child->pos.x += root->width.min_max.min - child->width.min_max.min - root->padding.right;
            }
            break;
            case CENTER:
            {
                child->pos.x += (root->width.min_max.min - root->padding.left - root->padding.right) * 0.5f - child->width.min_max.min * 0.5f + root->padding.left;
            }
            break;
            default:
            {
                SDL_Log("[UI] Unkown child alignement.");
            }
            break;
            }
            child->pos.y += offset;
            offset += child->height.min_max.min + root->child_gap;
        }

        ui_draw_helper(app, child);
    }
}

void ui_draw(app *app)
{
    ui_element *root = ui_ctx.root;

    ui_percent_resolve(root, app->window.width, app->window.height);
    ui_grow_resolve(root, app->window.width, app->window.height);
    ui_draw_helper(app, root);
}

ui_context *ui_context_get(void)
{
    return &ui_ctx;
}

ui_element *ui_open_elements_pop(void)
{
    if (ui_ctx.open_elements.count == 0)
    {
        SDL_Log("[UI] Tried to pop open elements with a count of 0.");
        return NULL;
    }

    return ui_ctx.open_elements.elements[--ui_ctx.open_elements.count];
}

ui_element *ui_open_elements_peek(i32 offset)
{
    if (ui_ctx.open_elements.count < (u32)SDL_abs(offset - 1))
    {
        return NULL;
    }

    return ui_ctx.open_elements.elements[ui_ctx.open_elements.count + offset - 1];
}

bool ui_hovered(ui_id id)
{
    for (u32 i = 0; i < darray_len(ui_ctx.pointer_over_ids); i++)
    {
        if (id == ui_ctx.pointer_over_ids[i])
        {
            return true;
        }
    }

    return false;
}

v2 ui_world_position(ui_id id, bool size)
{
    for (u32 i = 0; i < darray_len(ui_ctx.previous_data); i++)
    {
        if (id == ui_ctx.previous_data[i].id)
        {
            if (size)
            {
                return ui_ctx.previous_data[i].size;
            }
            else
            {
                return ui_ctx.previous_data[i].pos;
            }
        }
    }

    return v2zero();
}
