#ifndef GUI_MISC_H
#define GUI_MISC_H


#include <cairo.h>


enum DIR
{
    LEFT =  0x0001,
    RIGHT = 0x0002,
    UP =    0x0003,
    DOWN =  0x0004
};

cairo_pattern_t* create_diagonal_hatching_pattern(int size);


#endif
