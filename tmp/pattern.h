#ifndef PATTERN_H
#define PATTERN_H

#include "common.h"
#include "event.h"
#include "event_port.h"
#include "llist.h"


/* ---------------------------------------------------------------------
 * * * * * * PDATA * * * * */

typedef enum pdseed     /* random seed: */
{
    PDSEED_TIME_SYS,    /* seed using system time            */
    PDSEED_TIME_TICKS,  /* seed using total ticks            */
    PDSEED_USER         /* seed using a user specified value */

} pdseed;


typedef struct pdata
{
    int     bar_trig;
    bbt_t   loop_length;

    /* bounds for box dimensions */
    signed char width_min,  width_max;
    signed char height_min, height_max;

    bbt_t   beats_per_bar;
    bbt_t   beat_type;
    double  beat_ratio; /* ie beat_type * r = quarter note */

    pdseed      seedtype;
    uint32_t    seed;

} pdata;


pdata*  pdata_dup(const pdata*);

void    pdata_set_meter(pdata*, bbt_t beats_per_bar, bbt_t beat_type);
void    pdata_set_loop_length(pdata*, bbt_t ticks);
void    pdata_get_event_bbt(const pdata*,
                            const event*,
                            bbtpos* pos,
                            bbtpos* dur );

bbt_t   pdata_duration_bbt_to_ticks(const pdata*,
                                    bbt_t bar,
                                    bbt_t beat,
                                    bbt_t tick      );

void    pdata_dump(const pdata*);


/* ---------------------------------------------------------------------
 * * * * * * PLIST * * * * *
    there is no method to create a plist - it's an opaque data structure.
    only by creating a pattern, is a plist created.
 */

typedef struct plist plist;

/* plist_free won't work on the plist created by pattern_new */

plist*  plist_dup(const plist*);

void    plist_free(plist*);

size_t  plist_event_count(const plist*);

lnode*  plist_head(const plist*);
lnode*  plist_tail(const plist*);

void    plist_set_default_duration(bbt_t);
void    plist_set_default_width(signed char);
void    plist_set_default_height(signed char);

lnode*  plist_add_event(plist*, event*);
lnode*  plist_add_event_new( plist*, bbt_t start_tick);

lnode*  plist_unlink(   plist*, lnode*);

lnode*  plist_select(   const plist*,
                        bbt_t start_tick,
                        bbt_t end_tick);

lnode*  plist_invert_selection(const plist*);
lnode*  plist_select_all(      const plist*, _Bool sel);

plist*  plist_cut(    plist*,       _Bool sel);
plist*  plist_copy(   const plist*, _Bool sel);
void    plist_delete( plist*,       _Bool sel);
void    plist_paste(  plist* dest, float offset, const plist* src);
void    plist_edit(   plist*, evcb_type, float n, _Bool sel);

void    plist_dump(const plist*);
void    plist_dump_events(const plist*);


/* ---------------------------------------------------------------------
 * * * * * THE RTDATA STRUCT IS THE ONLY DATA THE RT THREAD * * * * *
 * * * * * SHOULD OPERATE ON. THE RTDATA FUNCTIONS ARE THE  * * * * *
 * * * * * ONLY FUNCTIONS THE RT THREAD SHOULD CALL.        * * * * */

typedef struct prtdata prtdata;

void    prtdata_trigger( prtdata*, bbt_t start_tick, bbt_t end_tick);
void    prtdata_play(    prtdata*, bbt_t start_tick, bbt_t end_tick);
void    prtdata_stop(    prtdata* );

/* * * * * rtdata_dump ONLY FOR DEBUGGING && !within RT thread  * * */
void    prtdata_dump(    prtdata* );


/* ---------------------------------------------------------------------
 * * * * * * PATTERN * * * * */

typedef struct pattern_
{
    char*   name;

    pdata*  pd;
    plist*  pl;

    prtdata* prt;
} pattern;


pattern*    pattern_new(void);
pattern*    pattern_dup(const pattern*);

void        pattern_free(pattern*);
void        pattern_dump(const pattern*);

void        pattern_set_output_port(pattern*, evport*);

void        pattern_prtdata_update(const pattern*);

#endif
