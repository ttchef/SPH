
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

    if (element->size.type == SIZE_FIT)
    {
        ui_padding padding = element->padding;
        element->size.width += padding.left + padding.right;
        element->size.height += padding.top + padding.bottom;
    }

    ui_element *parent = element->parent;
    if (!parent)
    {
        return;
    }

    if (parent->size.type == SIZE_FIT)
    {
        f32 child_gap = (darray_len(parent->children) - 1) * parent->child_gap;

        if (parent->layout == LAYOUT_TO_RIGHT)
        {
            parent->size.width += element->size.width + child_gap;
            parent->size.height = MAX(element->size.height, parent->size.height);
        }
        else if (parent->layout == LAYOUT_TO_BOTTOM)
        {
            parent->size.width = MAX(element->size.width, parent->size.width);
            parent->size.height += element->size.height + child_gap;
        }
    }
}

void ui_draw(simulation *simulation, ui_element *root)
{
    if (!root)
    {
        return;
    }

    draw_quad(simulation, root->pos, v2make(root->size.width, root->size.height), root->color, NULL, NULL);

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
            offset += child->size.width + root->child_gap;
        }
        else if (root->layout == LAYOUT_TO_BOTTOM)
        {
            child->pos.x += root->padding.left;
            child->pos.y += offset;
            offset += child->size.height + root->child_gap;
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
