#include "event.h"


#include "debug.h"


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


event* event_new(void)
{
    event* ev = malloc(sizeof(*ev));

    if (!ev)
    {
        WARNING("out of memory allocating event\n");
        return 0;
    }

    event_init(ev);

    return ev;
}


void event_init(event* ev)
{
    ev->flags =         0;
    ev->pos =           -1;
    ev->note_dur =      -1;
    ev->note_pitch =    -1;
    ev->note_velocity = -1;
    ev->box_x =         -1;
    ev->box_y =         -1;
    ev->box_width =     -1;
    ev->box_height =    -1;
    ev->box_release =   -1;
    ev->misc =          0;
}


void event_copy(event* dest, const event* src)
{
    dest->flags =           src->flags;
    dest->pos =             src->pos;
    dest->note_dur =        src->note_dur;
    dest->note_pitch =      src->note_pitch;
    dest->note_velocity =   src->note_velocity;
    dest->box_x =           src->box_x;
    dest->box_y =           src->box_y;
    dest->box_width =       src->box_width;
    dest->box_height =      src->box_height;
    dest->box_release =     src->box_release;
    dest->misc =            src->misc;
}

/*
int event_channel(const event* ev)
{
    return (0xf000 & ev->flags) >> 12;
}

void event_channel_set(event* ev, int ch)
{
    ev->flags = (0xf000 & (ch << 12)) + (0x0fff & ev->flags);
}

*/

void event_flag_set(event* ev, int f)
{
    if (!f)
    {
        ev->flags = 0;
        return;
    }

    if (f & EV_TYPEMASK)
        ev->flags = (0xfff0 & ev->flags) + (0x000f & f);

    if (f & EV_STATUSMASK)
        ev->flags = (0xff0f & ev->flags) + (0x00f0 & f);
}


void event_dump(const event* ev)
{
    MESSAGE("event: %p  |  flags %d  |  "
            "pos:%6d  |  dur:%6d  |  rel:%6d  |  "
            "pitch:%3d  |  vel:%3d  |  "
            "x:%3d  |  y:%3d  |  "
            "w:%3d  |  h:%3d  |  "
            "misc:%p \n",

            ev,             ev->flags,
            ev->pos,   ev->note_dur,   ev->box_release,
            ev->note_pitch, ev->note_velocity,
            ev->box_x,      ev->box_y,
            ev->box_width,  ev->box_height,
            ev->misc );
}


static int event_pri_cmp_time_cb(const void* d1,
                                 const void* d2, size_t n)
{
    #ifdef PATTERN_DEBUG
    if (n != sizeof(event))
        WARNING("cmp size != sizeof(event)\n");
    #endif

    const event* ev1 = d1;
    const event* ev2 = d2;

    if (ev1->pos == ev2->pos)
        return 0;

    if (ev1->pos > ev2->pos)
        return 1;

    return -1;
}


static int event_pri_cmp_coord_cb(const void* d1,
                                  const void* d2, size_t n)
{
    #ifdef PATTERN_DEBUG
    if (n != sizeof(event))
        WARNING("cmp size != sizeof(event)\n");
    #endif

    const event* ev1 = d1;
    const event* ev2 = d2;

    if (ev1->pos == ev2->pos)
        return 0;

    if (ev1->pos > ev2->pos)
        return 1;

    return -1;
}


static _Bool event_pri_sel_time_cb(const void* data, const void* crit)
{
    const event* ev = data;
    const ev_sel_time* sel = crit;
    return (ev->pos >= sel->start && ev->pos < sel->end);
}


static _Bool event_pri_sel_coord_cb(const void* data, const void* crit)
{
    const event* ev = data;
    const ev_sel_coord* sel = crit;
    return ((ev->box_x >= sel->tlx &&  ev->box_x <= sel->brx)
         && (ev->box_y >= sel->tly &&  ev->box_y <= sel->bry));
}


static void event_pri_mod_time_cb(void* data, void* init)
{
    event* ev = data;
    bbt_t* offset = init;

    if (*offset == -1)
        *offset = ev->pos;

    ev->pos -= *offset;
}


static void event_pri_mod_coord_cb(void* data, void* init)
{
    event* ev = data;
    coord* offset = init;

    if (offset->x == -1)
    {
        offset->x = ev->box_x;
        offset->y = ev->box_y;
    }

    ev->box_x -= offset->x;
    ev->box_y -= offset->y;
}


static char* event_pri_str_cb(const void* data, int level)
{
    char buf[DATACB_STR_SIZE];
    const event* ev = data;
    int count;

    switch(level)
    {
        case 0:
            return 0;

        case 1:
            count = snprintf(buf, DATACB_STR_SIZE,
                             "event:%p pos:%d",
                             ev, ev->pos);
            break;

        case 2:
            count = snprintf(buf, DATACB_STR_SIZE,
                             "event:%p pos:%d dur:%d",
                             ev, ev->pos, ev->note_dur);
            break;

        default:
            count = snprintf(buf, DATACB_STR_SIZE,
                             "event:%p pos:%d dur:%d rel:%d w:%d h:%d",
                             ev, ev->pos, ev->note_dur,
                             ev->box_release,
                             ev->box_width,
                             ev->box_height);
    }

    if (count >= DATACB_STR_SIZE)
        buf[DATACB_STR_SIZE - 1] = '\0';

    return strdup(buf);
}


static void event_pri_pos_set_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = roundf(*(const float*)val);
    ev->pos = (bbt_t)n;
}

static void event_pri_dur_set_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = roundf(*(const float*)val);
    ev->note_dur = (bbt_t)n;
}

static void event_pri_rel_set_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = roundf(*(const float*)val);
    ev->box_release = (bbt_t)n;
}

static void event_pri_pos_add_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = roundf(*(const float*)val);
    ev->pos = (bbt_t)((float)ev->pos + n);
}

static void event_pri_dur_add_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = roundf(*(const float*)val);
    ev->note_dur = (bbt_t)((float)ev->note_dur + n);
}

static void event_pri_rel_add_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = roundf(*(const float*)val);
    ev->box_release = (bbt_t)((float)ev->box_release + n);
}

static void event_pri_pos_mul_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = roundf((float)ev->pos * (*(const float*)val));
    ev->pos = (bbt_t)n;
}

static void event_pri_dur_mul_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = roundf((float)ev->note_dur * (*(const float*)val));
    ev->note_dur = (bbt_t)n;
}

static void event_pri_rel_mul_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = roundf((float)ev->box_release * (*(const float*)val));
    ev->box_release = (bbt_t)n;
}

static void event_pri_pos_quant_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    float nn = roundf((float)ev->pos + n / 2);
    ev->pos = (bbt_t)(nn - fmod(nn, n));
}

static void event_pri_dur_quant_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    float nn = roundf((float)ev->note_dur + n / 2);
    ev->note_dur = (bbt_t)(nn - fmod(nn, n));
}

static void event_pri_rel_quant_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    float nn = roundf((float)ev->box_release + n / 2);
    ev->box_release = (bbt_t)(nn - fmod(nn, n));
}

static void event_pri_x_set_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->box_x = (int)n;
}

static void event_pri_y_set_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->box_y = (int)n;
}

static void event_pri_w_set_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->box_width = (int)n;
}

static void event_pri_h_set_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->box_height = (int)n;
}

static void event_pri_x_add_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->box_x = (int)((float)ev->box_x + n);
}


static void event_pri_y_add_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->box_y = (int)((float)ev->box_y + n);
}

static void event_pri_w_add_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->box_width = (int)((float)ev->box_width + n);
}

static void event_pri_h_add_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->box_height = (int)((float)ev->box_height + n);
}

datacb_cmp event_get_cmp_cb(ev_type type)
{
    if (type == EV_COORD)
        return event_pri_cmp_coord_cb;
    return event_pri_cmp_time_cb;
}


datacb_sel event_get_sel_cb(ev_type type)
{
    if (type == EV_COORD)
        return event_pri_sel_coord_cb;
    return event_pri_sel_time_cb;
}


datacb_mod event_get_mod_cb(ev_type type)
{
    if (type == EV_COORD)
        return event_pri_mod_coord_cb;
    return event_pri_mod_time_cb;
}


datacb_str event_get_str_cb(void)
{
    return event_pri_str_cb;
}


datacb_edit event_get_edit_cb(evcb_type t)
{
    switch(t)
    {
    case EV_POS_SET:    return event_pri_pos_set_cb;
    case EV_DUR_SET:    return event_pri_dur_set_cb;
    case EV_REL_SET:    return event_pri_rel_set_cb;

    case EV_POS_ADD:    return event_pri_pos_add_cb;
    case EV_DUR_ADD:    return event_pri_dur_add_cb;
    case EV_REL_ADD:    return event_pri_rel_add_cb;

    case EV_POS_MUL:    return event_pri_pos_mul_cb;
    case EV_DUR_MUL:    return event_pri_dur_mul_cb;
    case EV_REL_MUL:    return event_pri_rel_mul_cb;

    case EV_POS_QUANT:  return event_pri_pos_quant_cb;
    case EV_DUR_QUANT:  return event_pri_dur_quant_cb;
    case EV_REL_QUANT:  return event_pri_rel_quant_cb;

    case EV_X_SET:      return event_pri_x_set_cb;
    case EV_Y_SET:      return event_pri_y_set_cb;
    case EV_W_SET:      return event_pri_w_set_cb;
    case EV_H_SET:      return event_pri_h_set_cb;

    case EV_X_ADD:      return event_pri_x_add_cb;
    case EV_Y_ADD:      return event_pri_y_add_cb;
    case EV_W_ADD:      return event_pri_w_add_cb;
    case EV_H_ADD:      return event_pri_h_add_cb;

    default:
        WARNING("unknown event edit cb type\n");
        return 0;
    }
}
