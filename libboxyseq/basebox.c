#include "basebox.h"
#include "freespace_state.h"


void box_init(basebox* box)
{
    box->x = 0;
    box->y = 0;
    box->w = 0;
    box->h = 0;
    box->r = 0;
    box->g = 0;
    box->b = 0;
}


void box_init_max_dim(basebox* box)
{
    box->x = 0;
    box->y = 0;
    box->w = FSWIDTH;
    box->h = FSHEIGHT;
    box->r = 0;
    box->g = 0;
    box->b = 0;
}


void box_copy(basebox* dest, const basebox* src)
{
    dest->x = src->x;
    dest->y = src->y;
    dest->w = src->w;
    dest->h = src->h;
    dest->r = src->r;
    dest->g = src->g;
    dest->b = src->b;
}


bool box_set_coords(basebox* box, int x, int y, int w, int h)
{
    if (x == -1)
        x = box->x;

    if (y == -1)
        y = box->y;

    if (w == -1)
        w = box->w;

    if (h == -1)
        h = box->h;

    if (x < 0 || w < 1 || x > FSWIDTH)
        return false;

    if (y < 0 || h < 1 || y > FSHEIGHT)
        return false;

    if (x + w > FSWIDTH || y + h > FSHEIGHT)
        return false;

    box->x = x;
    box->y = y;
    box->w = w;
    box->h = h;

    return true;
}


bool box_set_colour(basebox* box, int r, int g, int b)
{
    if (r == -1)
        r = box->r;

    if (g == -1)
        g = box->g;

    if (b == -1)
        b = box->b;

    if (r < 0 || r > 255)
        return false;

    if (g < 0 || g > 255)
        return false;

    if (b < 0 || b > 255)
        return false;

    box->r = r;
    box->g = g;
    box->b = b;

    return true;
}
