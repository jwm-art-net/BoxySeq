#include "freespace_boundary.h"
#include "freespace_state.h"


#include <stdlib.h>


struct freespace_boundary
{
    int x;
    int y;
    int w;
    int h;
};


fsbound* fsbound_new(void)
{
    fsbound* b = malloc(sizeof(*b));

    if (!b)
        return 0;

    return b;
}


fsbound* fsbound_dup(const fsbound* fsb)
{
    fsbound* b = fsbound_new();

    if (!b)
        return 0;

    fsbound_copy(b, fsb);

    return b;
}


void fsbound_copy(fsbound* dest, const fsbound* src)
{
    dest->x = src->x;
    dest->y = src->y;
    dest->w = src->w;
    dest->h = src->h;
}


void fsbound_free(fsbound* b)
{
    if (b)
        free(b);
}


void fsbound_init(fsbound* b)
{
    b->x = 0;
    b->y = 0;
    b->w = FSWIDTH;
    b->h = FSHEIGHT;
}


_Bool fsbound_set_coords(fsbound* b,  int x, int y, int w, int h)
{
    if (x > FSWIDTH || w > FSWIDTH)
        return false;

    if (y > FSHEIGHT || h > FSHEIGHT)
        return false;

    if (x + w > FSWIDTH)
        return false;

    if (y + h > FSHEIGHT)
        return false;

    b->x = x;
    b->y = y;
    b->w = w;
    b->h = h;

    return true;
}


void fsbound_get_coords(fsbound* b, int* x, int* y, int* w, int* h)
{
    *x = b->x;
    *y = b->y;
    *w = b->w;
    *h = b->h;
}

int fsbound_get_x(fsbound* b) { return b->x; }
int fsbound_get_y(fsbound* b) { return b->y; }
int fsbound_get_w(fsbound* b) { return b->w; }
int fsbound_get_h(fsbound* b) { return b->h; }

