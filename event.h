#ifndef EVENT_H
#define EVENT_H

#include "common.h"
#include "datacb.h"


typedef enum EVENT_FLAGS
{
    EV_NOTE_ON =        0x0001,
    EV_NOTE_OFF =       0x0002,

    EV_TYPE_NOTE =      0x0010,
    EV_HAS_DURATION =   0x0040,

    EV_CHANNEL_MASK =   0xf000

} evflags;


typedef struct event_
{
    int     flags;

    int     note_pitch;
    int     note_velocity;

    bbt_t   note_pos;       /* position in time (ticks) */
    bbt_t   note_dur;       /* duration (ticks)         */

    int     box_x;      /* maybe converts to pitch       */
    int     box_y;      /* maybe converts to velocity    */

    int     box_width;
    int     box_height;

    bbt_t   box_release;    /* duration of box after note off (ticks) */

    void*   misc;   /*  FYI: misc is zero initialized when a new event
                        is created, but after that, no other function
                        declared in this file, touchs it.
                    */

} event;


event*  event_new(void);
void    event_init(event*);
event*  event_dup(const event*);
void    event_dump(const event*);
void    event_copy(event* dest, const event* src);

int     event_channel(event*);
void    event_set_channel(event*, int);

/* list manipulation callbacks */


typedef struct event_time_select
{
    bbt_t start;
    bbt_t end;

} ev_sel_time;


typedef struct event_box_select
{
    int tlx;
    int tly;
    int brx;
    int bry;

} ev_sel_coord;


typedef enum EVENT_CB_TYPE
{
    EV_POS_SET,
    EV_DUR_SET,
    EV_REL_SET,

    EV_POS_ADD,
    EV_DUR_ADD,
    EV_REL_ADD,

    EV_POS_MUL,
    EV_DUR_MUL,
    EV_REL_MUL,

    EV_POS_QUANT,
    EV_DUR_QUANT,
    EV_REL_QUANT,

    EV_X_SET,
    EV_Y_SET,
    EV_W_SET,
    EV_H_SET,

    EV_X_ADD,
    EV_Y_ADD,
    EV_W_ADD,
    EV_H_ADD

} evcb_type;


/*  events have two different roles dependent upon the container:

    * pattern
        with ordering, selection, comparison, and editing
        based upon time.

    * grid
        with ordering, selection, comparison, and editing
        based upon x,y coordinates.
*/


typedef enum EVENT_TYPE
{
    EV_TIME,
    EV_COORD,

} ev_type;


datacb_cmp  event_get_cmp_cb(ev_type);
datacb_sel  event_get_sel_cb(ev_type);
datacb_mod  event_get_mod_cb(ev_type);
datacb_str  event_get_str_cb(void);

datacb_edit event_get_edit_cb(evcb_type);


#endif
