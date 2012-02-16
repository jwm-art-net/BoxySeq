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
    ev->box.x = -1;
    ev->box.y = -1;
    ev->box.w = -1;
    ev->box.h = -1;
    ev->box.r = 0;
    ev->box.g = 0;
    ev->box.b = 0;

    ev->flags =         0;
    ev->pos =           -1;
    ev->frame =         0;
    ev->note_dur =      -1;
    ev->note_pitch =    -1;
    ev->note_velocity = -1;
    ev->box_release =   -1;
    ev->grb =            0;
}


void event_copy(event* dest, const event* src)
{
    dest->box.x = src->box.x;
    dest->box.y = src->box.y;
    dest->box.w = src->box.w;
    dest->box.h = src->box.h;
    dest->box.r = src->box.r;
    dest->box.g = src->box.g;
    dest->box.b = src->box.b;

    dest->flags =           src->flags;
    dest->pos =             src->pos;
    dest->note_dur =        src->note_dur;
    dest->note_pitch =      src->note_pitch;
    dest->note_velocity =   src->note_velocity;
    dest->box_release =     src->box_release;
    dest->grb =             src->grb;
}


void event_dump(const event* ev)
{
    char tmp[20];

    event_flags_to_str(ev->flags, tmp);

    MESSAGE("event: %p  [%s] "
            "pos:%d dur:%d rel:%d"
            "\tpitch:%d vel:%d "
            "\tx:%d y:%d "
            "\tw:%d h:%d "
            "\tgrb:%p \n",

            ev,             tmp,
            ev->pos,   ev->note_dur,   ev->box_release,
            ev->note_pitch, ev->note_velocity,
            ev->box.x,      ev->box.y,
            ev->box.w,      ev->box.h,
            ev->grb );
}


void event_flags_to_str(int flags, char buf[20])
{
    const char* type = 0;
    const char* status = 0;
    int r;

    switch(flags & EV_TYPE_MASK)
    {
    case EV_TYPE_NOTE:      type = "note";  break;
    case EV_TYPE_BLOCK:     type = "block"; break;
    case EV_TYPE_CLEAR:     type = "reset"; break;
    case EV_TYPE_SHUTDOWN:  type = "quit";  break;
    default:
        type = "*ERR*";
        break;
    }

    status = (flags & EV_STATUS_ON) ? "ON" : "OFF";

    snprintf(buf, 20, "%5s | %3s", type, status);
}


#ifndef NDEBUG
/*      DEBUGGING ONLY: checks event against status and type
                        represented by flags. displays event on
                        mismatch/error.
 */
bool event_is(const event* ev, int flags,
              const char *file, const char *function, size_t line)
{
    int status = flags & EV_STATUS_ON;
    int type = flags & EV_TYPE_MASK;
    bool ret = true;

    if (!EVENT_IS_STATUS( ev, status ))
    {
        event_dump(ev);
        warnf(W_WARNING, file, function, line,
                "^^^ event has invalid status    ^^^\n");
        ret = false;
    }

    if (!EVENT_IS_TYPE( ev, type ))
    {
        if (ret == true)
            event_dump(ev);

        warnf(W_WARNING, file, function, line,
                "^^^ invalid event type ^^^\n");
        ret = false;
    }

    return ret;
}
#endif


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


static bool event_pri_sel_time_cb(const void* data, const void* crit)
{
    const event* ev = data;
    const ev_sel_time* sel = crit;
    return (ev->pos >= sel->start && ev->pos < sel->end);
}


static bool event_pri_sel_coord_cb(const void* data, const void* crit)
{
    const event* ev = data;
    const ev_sel_coord* sel = crit;
    return ((ev->box.x >= sel->tlx &&  ev->box.x <= sel->brx)
         && (ev->box.y >= sel->tly &&  ev->box.y <= sel->bry));
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
        offset->x = ev->box.x;
        offset->y = ev->box.y;
    }

    ev->box.x -= offset->x;
    ev->box.y -= offset->y;
}


static char* event_pri_str_cb(const void* data, int level)
{
    char buf[DATACB_STR_SIZE];
    char tmp[20];
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
                             ev->box.w,
                             ev->box.h);
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
    ev->box.x = (int)n;
}

static void event_pri_y_set_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->box.y = (int)n;
}

static void event_pri_w_set_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->box.w = (int)n;
}

static void event_pri_h_set_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->box.h = (int)n;
}

static void event_pri_x_add_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->box.x = (int)((float)ev->box.x + n);
}


static void event_pri_y_add_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->box.y = (int)((float)ev->box.y + n);
}

static void event_pri_w_add_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->box.w = (int)((float)ev->box.w + n);
}

static void event_pri_h_add_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->box.h = (int)((float)ev->box.h + n);
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
