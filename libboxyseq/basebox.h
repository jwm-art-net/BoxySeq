#ifndef BASEBOX_H
#define BASEBOX_H


#include <stdbool.h>


typedef struct basebox_
{
    int x, y;
    int w, h;
    unsigned char r, g, b;

} basebox;


void        box_init(basebox*);
void        box_init_max_dim(basebox*); /* with max dimension */

void        box_copy(basebox* dest, const basebox* src);

bool        box_set_coords(basebox*, int x, int y, int w, int h);
bool        box_set_colour(basebox*, int r, int g, int b);


#endif
