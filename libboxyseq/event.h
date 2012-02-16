#ifndef EVENT_H
#define EVENT_H


#ifdef __cplusplus
extern "C" {
#endif

#include "basebox.h"
#include "boxyseq_types.h"
#include "common.h"
#include "datacb.h"


/**
 * Flags pertaining to event type.
 */
 
typedef enum EVENT_FLAGS
{
    EV_TYPE_NOTE =          0x0001,
    EV_TYPE_BLOCK =         0x0002,
    EV_TYPE_STATIC =        0x0003, /* static block */

/*
    EV_TYPE_BLOCKED_NOTE =  0x0003,
*/

    EV_TYPE_CLEAR =         0x000d, /* from RT thread to clear GUI */
    EV_TYPE_SHUTDOWN =      0x000e, /* from GUI thread to clear RT */

    EV_TYPE_MASK =          0x000f,

    EV_STATUS_ON =          0x0010, /* or off */

    EV_CHANNEL_MASK =       0xf000,

#ifdef EVPOOL_DEBUG
    EV_IS_FREE_ERROR =      0xffff
#endif

} evflags;

#define EV_STATUS_OFF (!EV_STATUS_ON)

/**
 * a single data structure for event
 */
typedef struct event_
{
    basebox box;
    int     flags;
    bbt_t   pos;

    uint32_t frame;

    bbt_t   note_dur;
    int     note_pitch;
    int     note_velocity;

    bbt_t   box_release;    /* duration of box after note off (ticks) */

    grbound* grb;

} event;


event*  event_new(void);
void    event_init(event*);
event*  event_dup(const event*);
void    event_dump(const event*);
void    event_copy(event* dest, const event* src);

void    event_flags_to_str(int flags, char buf[20]);

#ifndef NDEBUG
/*      DEBUGGING ONLY: checks event against status and type
                        represented by flags. displays event on
                        mismatch/error.
 */
bool    event_is(const event*, int flags,
                 const char *file, const char *function, size_t line);
#define EVENT_IS( event, flags ) \
    event_is( event, flags, __FILE__, __FUNCTION__, __LINE__)
#endif

#define EVENT_IS_STATUS( ev, evstatus ) \
    ((( ev )->flags & EV_STATUS_ON ) == evstatus)

#define EVENT_IS_STATUS_ON( ev )    \
    ((( ev )->flags & EV_STATUS_ON ) == EV_STATUS_ON)

#define EVENT_IS_STATUS_OFF( ev )   \
    ((( ev )->flags & EV_STATUS_ON ) != EV_STATUS_ON)

#define EVENT_SET_STATUS_ON( ev )   \
    ( ev )->flags |= EV_STATUS_ON

#define EVENT_SET_STATUS_OFF( ev )  \
    ( ev )->flags &= ~EV_STATUS_ON

#define EVENT_SET_TYPE( ev, evtype )                    \
    ( ev )->flags = ((~EV_TYPE_MASK) & ( ev ) ->flags)  \
                    | (EV_TYPE_MASK & evtype)

#define EVENT_GET_TYPE( ev )        \
    (( ev )-> flags & EV_TYPE_MASK)

#define EVENT_IS_TYPE( ev, evtype ) \
    ((( ev )->flags & EV_TYPE_MASK ) == evtype)

#define EVENT_GET_CHANNEL( ev )     \
    ((0xf000 & ( ev )->flags) >> 12)

#define EVENT_SET_CHANNEL( ev, ch ) \
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


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
