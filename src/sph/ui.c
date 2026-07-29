
#include <sph/darray.h>
#include <sph/simulation.h>
#include <sph/ui.h>

ui_element *ui_open(ui_element *parent, ui_element *element)
{
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

    if (element->width.type == UI_SIZE_FIT)
    {
        element->width.value += element->padding.left + element->padding.right;
    }
    if (element->height.type == UI_SIZE_FIT)
    {
        element->height.value += element->padding.top + element->padding.bottom;
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

static void ui_grow_resolve(ui_element *root)
{
    // TODO: handle parent grow as a special case later

    if (!root->children)
    {
        return;
    }

    f32 remaining_width  = root->width.value;
    f32 remaining_height = root->height.value;

    remaining_width -= root->padding.left + root->padding.right;
    remaining_height -= root->padding.top + root->padding.bottom;

    for (u32 i = 0; i < darray_len(root->children); i++)
    {
        ui_element *child = &root->children[i];

        if (child->width.type == UI_SIZE_GROW)
        {
            continue;
        }

        remaining_width -= child->width.value;
    }
    remaining_width -= (darray_len(root->children) - 1) * root->child_gap;

    for (u32 i = 0; i < darray_len(root->children); i++)
    {
        ui_element *child = &root->children[i];

        if (child->width.type == UI_SIZE_GROW)
        {
            child->width.value += remaining_width;
        }
        if (child->height.type == UI_SIZE_GROW)
        {
            child->height.value += (remaining_height - child->height.value);
        }
    }
}

void ui_draw(simulation *simulation, ui_element *root)
{
    if (!root)
    {
        return;
    }

    ui_grow_resolve(root);

    draw_quad(simulation, root->pos, v2make(root->width.value, root->height.value), root->color, NULL, NULL);

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
    if (!root || !root->children)
    {
        return;
    }

    for (u32 i = 0; i < darray_len(root->children); i++)
    {
        ui_destroy(&root->children[i]);
    }
    darray_destroy(root->children);
    SDL_free(root);
}
