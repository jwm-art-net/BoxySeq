#include "event.h"


#include "debug.h"
#include "freespace_state.h"


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
    ev->flags = 0;

    ev->x =     -1;
    ev->y =     -1;
    ev->w =     -1;
    ev->h =     -1;

    ev->r =     0;
    ev->g =     0;
    ev->b =     0;

    ev->pos =   -1;
    ev->dur =   -1;
    ev->rel =   -1;

    ev->frame = 0;

    ev->pitch = -1;
    ev->vel =   -1;

    ev->data =  0;
}


void event_copy(event* dest, const event* src)
{
    dest->flags =   src->flags;

    dest->x = src->x;
    dest->y = src->y;
    dest->w = src->w;
    dest->h = src->h;

    dest->r = src->r;
    dest->g = src->g;
    dest->b = src->b;

    dest->pos = src->pos;
    dest->dur = src->dur;
    dest->rel = src->rel;

    dest->pitch =   src->pitch;
    dest->vel =     src->vel;

    dest->data =    src->data;
}


bool event_set_coords(event* ev, int x, int y, int w, int h)
{
    if (x == -1)
        x = ev->x;

    if (y == -1)
        y = ev->y;

    if (w == -1)
        w = ev->w;

    if (h == -1)
        h = ev->h;

    if (x < 0 || w < 1 || x > FSWIDTH)
        return false;

    if (y < 0 || h < 1 || y > FSHEIGHT)
        return false;

    if (x + w > FSWIDTH || y + h > FSHEIGHT)
        return false;

    ev->x = x;
    ev->y = y;
    ev->w = w;
    ev->h = h;

    return true;
}


bool event_set_colour(event* ev, int r, int g, int b)
{
    if (r == -1)
        r = ev->r;

    if (g == -1)
        g = ev->g;

    if (b == -1)
        b = ev->b;

    if (r < 0 || r > 255)
        return false;

    if (g < 0 || g > 255)
        return false;

    if (b < 0 || b > 255)
        return false;

    ev->r = r;
    ev->g = g;
    ev->b = b;

    return true;
}

void event_dump(const char *file,
                const char *function,
                size_t line,
                const event* ev)
{
    char tmp[20];

    event_flags_to_str(ev->flags, tmp);

    warnf( W_MESSAGE, file, function, line,
            "event: %015p  [%s] "
            "pos:%5d dur:%5d rel:%5d"
            "\tpitch:%3d vel:%3d "
            "\tx:%3d y:%3d "
            "\tw:%3d h:%3d "
            "\tdata:%p \n",
            ev,         tmp,
            ev->pos,    ev->dur,   ev->rel,
            ev->pitch,  ev->vel,
            ev->x,      ev->y,
            ev->w,      ev->h,
            ev->data );
}


void event_flags_to_str(int flags, char buf[20])
{
    const char* type = 0;
    const char* status = 0;
    unsigned int ch = flags & EV_CHANNEL_MASK;
    int r;

    switch(flags & EV_TYPE_MASK)
    {
    case EV_TYPE_NOTE:      type = "note";  break;
    case EV_TYPE_BLOCK:     type = "block"; break;
    case EV_TYPE_STATIC:    type = "static";break;
    case EV_TYPE_CLEAR:     type = "reset"; break;
    case EV_TYPE_SHUTDOWN:  type = "quit";  break;
    case EV_TYPE_BOUNDARY:  type = "bound"; break;
    
    default:
        type = "*ERR*";
        break;
    }

    switch(flags & EV_STATUS_MASK)
    {
    case EV_STATUS_ON:      status = "ON";  break;
    case EV_STATUS_CREATE:  status = "NEW"; break;
    case EV_STATUS_DESTROY: status = "DEL"; break;
    default:                status = "OFF"; break;
    }

    snprintf(buf, 20, "ch:%1x %5s | %3s", ch, type, status);
}


#ifndef NDEBUG
/*      DEBUGGING ONLY: checks event against status and type
                        represented by flags. displays event on
                        mismatch/error.
 */
bool event_is(const event* ev, int flags,
              const char *file, const char *function, size_t line)
{
    int status = flags & EV_STATUS_MASK;
    int type =   flags & EV_TYPE_MASK;
    bool ret = true;

    if (!EVENT_IS_STATUS( ev, status ))
    {
        event_dump(file, function, line, ev);
        warnf(W_WARNING, file, function, line,
                "         ^^^ event has invalid status ^^^\n");

        printf("0x%x & EV_STATUS_MASK (0x%x) == 0x%x \n",
                                flags, EV_STATUS_MASK,status);

        ret = false;
    }

    if (!EVENT_IS_TYPE( ev, type ))
    {
        if (ret == true)
            EVENT_DUMP(ev);

        warnf(W_WARNING, file, function, line,
                "        ^^^ invalid event type ^^^\n");
        ret = false;
    }

    return ret;
}
#endif /* ifndef NDEBUG */


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
    return ((ev->x >= sel->tlx &&  ev->x <= sel->brx)
         && (ev->y >= sel->tly &&  ev->y <= sel->bry));
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
        offset->x = ev->x;
        offset->y = ev->y;
    }

    ev->x -= offset->x;
    ev->y -= offset->y;
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
                             ev, ev->pos, ev->dur);
            break;

        default:
            count = snprintf(buf, DATACB_STR_SIZE,
                             "event:%p pos:%d dur:%d rel:%d w:%d h:%d",
                             ev, ev->pos, ev->dur, ev->rel, ev->w, ev->h);
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
    ev->dur = (bbt_t)n;
}

static void event_pri_rel_set_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = roundf(*(const float*)val);
    ev->rel = (bbt_t)n;
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
    ev->dur = (bbt_t)((float)ev->dur + n);
}

static void event_pri_rel_add_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = roundf(*(const float*)val);
    ev->rel = (bbt_t)((float)ev->rel + n);
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
    float n = roundf((float)ev->dur * (*(const float*)val));
    ev->dur = (bbt_t)n;
}

static void event_pri_rel_mul_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = roundf((float)ev->rel * (*(const float*)val));
    ev->rel = (bbt_t)n;
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
    float nn = roundf((float)ev->dur + n / 2);
    ev->dur = (bbt_t)(nn - fmod(nn, n));
}

static void event_pri_rel_quant_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    float nn = roundf((float)ev->rel + n / 2);
    ev->rel = (bbt_t)(nn - fmod(nn, n));
}

static void event_pri_x_set_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->x = (int)n;
}

static void event_pri_y_set_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->y = (int)n;
}

static void event_pri_w_set_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->w = (int)n;
}

static void event_pri_h_set_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->h = (int)n;
}

static void event_pri_x_add_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->x = (int)((float)ev->x + n);
}


static void event_pri_y_add_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->y = (int)((float)ev->y + n);
}

static void event_pri_w_add_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->w = (int)((float)ev->w + n);
}

static void event_pri_h_add_cb(void* data, const void* val)
{
    event* ev = (event*)data;
    float n = *(const float*)val;
    ev->h = (int)((float)ev->h + n);
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
