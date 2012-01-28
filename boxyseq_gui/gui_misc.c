#include "gui_misc.h"


cairo_pattern_t* create_diagonal_hatching_pattern(int sz)
{
    cairo_surface_t* surface;
    cairo_t* cr;
    cairo_pattern_t* pattern;

    surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, sz, sz);
    cr = cairo_create(surface);

    cairo_set_line_width(cr, sz / 2);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);

    /* make diagonal */
    cairo_move_to(cr, 0, sz);
    cairo_line_to(cr, sz, 0);
    cairo_stroke(cr);

    /* fill in join of diagonal */
    cairo_move_to(cr, -sz / 2, sz / 2);
    cairo_line_to(cr, sz / 2, -sz / 2);
    cairo_stroke(cr);

    /* fill in join of diagonal */
    cairo_move_to(cr, sz / 2, sz + sz / 2);
    cairo_line_to(cr, sz + sz / 2, sz / 2);
    cairo_stroke(cr);

    pattern = cairo_pattern_create_for_surface(surface);
    cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return pattern;
}
