#ifndef EVENT_H
#define EVENT_H


#ifdef __cplusplus
extern "C" {
#endif

#include "boxyseq_types.h"
#include "common.h"
#include "datacb.h"


typedef enum EVENT_FLAGS
{
    EV_STATUS_OFF =         0x0000,

    EV_CHANNEL_MASK =       0x000f,

    EV_TYPE_NOTE =          0x0010,
    EV_TYPE_BLOCK =         0x0020,
    EV_TYPE_STATIC =        0x0030,
    EV_TYPE_CLEAR =         0x0040,
    EV_TYPE_SHUTDOWN =      0x0050,
    EV_TYPE_BOUNDARY =      0x0060,
    EV_TYPE_MASK =          0x0ff0,

    EV_STATUS_ON =          0x1000,
    EV_STATUS_CREATE =      0x2000,
    EV_STATUS_DESTROY =     0x3000,
    EV_STATUS_MASK =        0xf000,

#ifdef EVPOOL_DEBUG
    EV_IS_FREE_ERROR =      0xffff
#endif
} evflags;





typedef struct event_
{
    int     flags;

    int     x;
    int     y;
    int     w;
    int     h;

    unsigned char r, g, b;

    bbt_t   pos;
    bbt_t   dur;
    bbt_t   rel;

    uint32_t frame;

    int     pitch;
    int     vel;

    void*   data;

} event;


event*  event_new(void);
void    event_init(event*);
event*  event_dup(const event*);
void    event_copy(event* dest, const event* src);

bool    event_set_coords(event*, int x, int y, int w, int h);
bool    event_set_colour(event*, int r, int g, int b);

void    event_flags_to_str(int flags, char buf[20]);



void    event_dump(const char *file, const char *function, size_t line,
                   const event*);
#define EVENT_DUMP( ev ) \
    event_dump(__FILE__, __FUNCTION__, __LINE__, ev )



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
    ((( ev )->flags & EV_STATUS_MASK ) == evstatus)

#define EVENT_IS_STATUS_ON( ev )    \
    ((( ev )->flags & EV_STATUS_ON ) == EV_STATUS_ON)

#define EVENT_IS_STATUS_OFF( ev )   \
    (!(( ev )->flags & EV_STATUS_MASK))

#define EVENT_SET_STATUS_ON( ev )   \
    ( ev )->flags = ((~EV_STATUS_MASK) & ( ev )->flags) \
                    | EV_STATUS_ON

#define EVENT_SET_STATUS_OFF( ev )  \
    ( ev )->flags &= ~EV_STATUS_MASK

#define EVENT_SET_TYPE( ev, evtype )                    \
    ( ev )->flags = ((~EV_TYPE_MASK) & ( ev )->flags)  \
                    | (EV_TYPE_MASK & evtype)

#define EVENT_GET_TYPE( ev )        \
    (( ev )-> flags & EV_TYPE_MASK)

#define EVENT_IS_TYPE( ev, evtype ) \
    ((( ev )->flags & EV_TYPE_MASK ) == evtype)

#define EVENT_GET_CHANNEL( ev ) \
    (EV_CHANNEL_MASK & ( ev )->flags)

#define EVENT_SET_CHANNEL( ev, ch )                         \
    ( ev )->flags = ((~EV_CHANNEL_MASK) & ( ev )->flags)    \
                    | (EV_CHANNEL_MASK & ch)


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
