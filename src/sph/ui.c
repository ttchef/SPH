
#include <sph/darray.h>
#include <sph/simulation.h>
#include <sph/ui.h>

ui_element *ui_open(ui_element *parent, ui_element *element)
{
    if (element->width.max < FLT_EPSILON)
    {
        element->width.max = FLT_MAX;
    }
    if (element->height.max < FLT_EPSILON)
    {
        element->height.max = FLT_MAX;
    }
    
    if (!parent)
    {
        void *data = SDL_malloc(sizeof(*element));
        SDL_memcpy(data, element, sizeof(*element));
        return data;
    }

    if (!parent->children)
    {
        parent->children = darray_create(sizeof(ui_element));
    }

    element->parent = parent;
    return darray_push((void *)&parent->children, element);
}

void ui_close(ui_element *element)
{
    if (!element)
    {
        return;
    }

    else if (element->width.type == UI_SIZE_FIT)
    {
        if (element->text)
        {
            element->width.value = measure_text(element->font, element->font_size, "%s", element->text);
        }
        element->width.value += element->padding.left + element->padding.right;
        element->width.value = MIN(element->width.value, element->width.max);

        if (element->width.min != 0.0f)
        {
            element->width.value = MAX(element->width.value, element->width.min);
        }
    }
    if (element->height.type == UI_SIZE_FIT)
    {
        if (element->text)
        {
            element->height.value = element->font_size;
        }

        element->height.value += element->padding.top + element->padding.bottom;
        element->height.value = MIN(element->height.value, element->height.max);

        if (element->height.min != 0.0f)
        {
            element->height.value = MAX(element->height.value, element->height.min);
        }
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
            parent->width.value += element->width.value + child_gap;
        }
        else if (parent->layout == LAYOUT_TO_BOTTOM)
        {
            parent->width.value = MAX(element->width.value, parent->width.value);
        }
    }
    if (parent->height.type == UI_SIZE_FIT)
    {
        if (parent->layout == LAYOUT_TO_RIGHT)
        {
            parent->height.value = MAX(element->height.value, parent->height.value);
        }
        else if (parent->layout == LAYOUT_TO_BOTTOM)
        {
            parent->height.value += element->height.value + child_gap;
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
            root->width.value = window_width - root->pos.x;
            root->width.value = MIN(root->width.value, root->width.max - root->pos.x);
        }
        if (root->height.type == UI_SIZE_GROW)
        {
            root->height.value = window_height - root->pos.y;
            root->height.value = MIN(root->height.value, root->height.max - root->pos.y);
        }
    }

    if (!root->children)
    {
        return;
    }

    // NOTE: Reset growable to min size
    for (u32 i = 0; i < darray_len(root->children); i++)
    {
        ui_element *child = &root->children[i];

        if (child->width.type == UI_SIZE_GROW)
        {
            child->width.value = child->width.min;
        }

        if (child->height.type == UI_SIZE_GROW)
        {
            child->height.value = child->height.min;
        }
    }

    f32 remaining_width  = root->width.value;
    f32 remaining_height = root->height.value;

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

            if (along_layout_size->value < smallest_growable_size)
            {
                smallest_growable_size = along_layout_size->value;
                smallest_growable      = i;
            }
        }

        remaining_along -= along_layout_size->value;
    }
    remaining_along -= (darray_len(root->children) - 1) * root->child_gap;

    while (growable_count && remaining_along > 0.0f)
    {
        f32 smallest        = root->layout == LAYOUT_TO_RIGHT ? root->children[smallest_growable].width.value : root->children[smallest_growable].height.value;
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

            if (along_layout_size->value < smallest)
            {
                second_smallest = smallest;
                smallest        = along_layout_size->value;
            }
            else if (along_layout_size->value > smallest)
            {
                second_smallest = MIN(second_smallest, along_layout_size->value);
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

            if (along_layout_size->value == smallest)
            {
                along_layout_size->value += size_to_add;
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
            across_layout_size->value += (remaining_across - across_layout_size->value);
        }
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
            root->width.value = window_width * root->width.value;
        }
        else
        {
            root->width.value = root->parent->width.value * root->width.value;
        }
        root->width.value = CLAMP(root->width.value, root->width.min, root->width.max);
    }
    
    if (root->height.type == UI_SIZE_PERCENT)
    {
        if (!root->parent)
        {
            root->height.value = window_height * root->height.value;
        }
        else
        {
            root->height.value = root->parent->height.value * root->height.value;
        }        
        root->height.value = CLAMP(root->height.value, root->height.min, root->height.max);
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

void ui_draw(simulation *simulation, ui_element *root)
{
    if (!root)
    {
        return;
    }

    ui_percent_resolve(root, simulation->window.width, simulation->window.height);
    ui_grow_resolve(root, simulation->window.width, simulation->window.height);

    draw_quad(simulation, root->pos, v2make(root->width.value, root->height.value), root->roundness, root->color, NULL, NULL);
    if (root->text)
    {
        draw_text(simulation, root->font, v2make(root->pos.x + root->padding.left, root->pos.y + root->padding.top), root->font_size, "%s", root->text);
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
            child->pos.x += offset;
            child->pos.y += root->padding.top;
            offset += child->width.value + root->child_gap;
        }
        else if (root->layout == LAYOUT_TO_BOTTOM)
        {
            child->pos.x += root->padding.left;
            child->pos.y += offset;
            offset += child->height.value + root->child_gap;
        }

        ui_draw(simulation, child);
    }
}

void ui_destroy(ui_element *root)
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
