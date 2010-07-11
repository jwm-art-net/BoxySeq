#ifndef EVENT_H
#define EVENT_H

#include "common.h"
#include "datacb.h"

/**
 * Flags pertaining to event type.
 */
 
typedef enum EVENT_FLAGS
{
    EV_TYPE_NOTE =      0x0001,
    EV_TYPE_BLOCK =     0x0002,
    EV_TYPEMASK =       0x000f,

    EV_STATUS_ON =      0x0010, /* or off */

    EV_CHANNEL_MASK =   0xf000,

#ifdef EVPOOL_DEBUG
    EV_IS_FREE_ERROR =  0xffff
#endif

} evflags;

/**
 * a single data structure for event
 */
typedef struct event_
{
    int     flags;
    bbt_t   pos;

    bbt_t   note_dur;
    int     note_pitch;
    int     note_velocity;

    int     box_x;          /* maybe converts to pitch       */
    int     box_y;          /* maybe converts to velocity    */
    int     box_width;
    int     box_height;
    bbt_t   box_release;    /* duration of box after note off (ticks) */

    void*   misc;           /*  all fields copied by event_copy
                                but not deep in this case...    */

} event;


event*  event_new(void);
void    event_init(event*);
event*  event_dup(const event*);
void    event_dump(const event*);
void    event_copy(event* dest, const event* src);



#define EVENT_IS_STATUS_ON( ev ) \
    ((( ev )->flags & EV_STATUS_ON ) == EV_STATUS_ON)

#define EVENT_IS_STATUS_OFF( ev ) \
    ((( ev )->flags & EV_STATUS_ON ) != EV_STATUS_ON)

#define EVENT_IS_TYPE_NOTE( ev ) \
    ((( ev )->flags & EV_TYPE_NOTE ) == EV_TYPE_NOTE)

#define EVENT_IS_TYPE_BLOCK( ev ) \
    ((( ev )->flags & EV_TYPE_NOTE ) != EV_TYPE_NOTE)


#define EVENT_SET_STATUS_ON( ev ) \
    ( ev )->flags |= EV_STATUS_ON

#define EVENT_SET_STATUS_OFF( ev ) \
    ( ev )->flags &= ~EV_STATUS_ON

#define EVENT_SET_TYPE_NOTE( ev ) \
    ( ev )->flags = ((~EV_TYPEMASK) & ( ev )->flags) | EV_TYPE_NOTE

#define EVENT_SET_TYPE_BLOCK( ev ) \
    ( ev )->flags = ((~EV_TYPEMASK) & ( ev )->flags) | EV_TYPE_BLOCK

#define EVENT_CHANNEL( ev ) \
    ((0xf000 & ( ev )->flags) >> 12)

#define EVENT_CHANNEL_SET( ev, ch ) \
    ( ev )->flags = (0xf000 & (( ch ) << 12)) + (0x0fff & ( ev )->flags);



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
