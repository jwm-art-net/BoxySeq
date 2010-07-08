#include "pattern.h"

#include "debug.h"

#include <glib.h>   /* for random number using mersene twister */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

struct plist
{
    llist* ll;

    pattern* pat;
};


struct prtdata
{
    _Bool  playing;    /* states */
    _Bool  triggered;

    GRand* rnd;

    bbt_t  start_tick; /* processing */
    bbt_t  end_tick;
    bbt_t  index;

    pdata* volatile pd;
    pdata* volatile pd_in_use;
    pdata*          pd_old;
    pdata*          pd_free;

    event* volatile ev_arr;
    event* volatile ev_arr_in_use;
    event*          ev_arr_old;
    event*          ev_arr_free;

    evport* evoutput;
};


/* ---------------------------------------------------------------------
 * * * * * * PDATA * * * * */

static pdata*  pdata_pri_new(void);


pdata* pdata_dup(const pdata* pd)
{
    pdata* newpd = malloc(sizeof(*pd));

    if (!newpd)
    {
        WARNING("failed to duplicate pdata:%p\n",
                (const void*)pd);
        return 0;
    }

    return memcpy(newpd, pd, sizeof(*pd));
}


void pdata_set_meter(pdata* pd, bbt_t beats_per_bar, bbt_t beat_type)
{
    if (beats_per_bar <= 1 && beat_type <= 1)
        return;

    pd->beats_per_bar = beats_per_bar;
    pd->beat_type =     beat_type;
    pd->beat_ratio =    4.0 / beat_type;
}


void pdata_set_loop_length(pdata* pd, bbt_t ticks)
{
    pd->loop_length = ticks;
}


void pdata_get_event_bbt(const pdata* pd,
                         const event* ev,
                         bbtpos* pos,
                         bbtpos* dur         )
{
    bbt_t ticks_per_beat = (bbt_t)(pd->beat_ratio * ppqn);
    bbt_t ticks_per_bar =  pd->beats_per_bar * ticks_per_beat;
    bbt_t tick = 0;

    if (pos)
    {
        tick =      ev->pos;
        pos->bar =  tick / ticks_per_bar;
        tick -=     pos->bar * ticks_per_bar;
        pos->bar++;
        pos->beat = tick / ticks_per_beat;
        tick -=     pos->beat * ticks_per_beat;
        pos->beat++;
        pos->tick = tick;
    }

    if (dur)
    {
        tick =      ev->note_dur;
        dur->bar =  tick / ticks_per_bar;
        tick -=     dur->bar * ticks_per_bar;
        dur->beat = tick / ticks_per_beat;
        tick -=     dur->beat * ticks_per_beat;
        dur->tick = tick;
    }
}


bbt_t pdata_duration_bbt_to_ticks(const pdata* pd,
                                        bbt_t bar,
                                        bbt_t beat,
                                        bbt_t tick      )
{
    bbt_t ticks_per_beat = (bbt_t)(pd->beat_ratio * ppqn);
    return (ticks_per_beat * pd->beats_per_bar * bar
            + ticks_per_beat * beat + tick);
}


void pdata_dump(const pdata* pd)
{
    MESSAGE("pdata: %p\n", (const void*)pd);

    if (!pd)
        return;

    MESSAGE("\tbar_trig: %d\n\tloop length: %d\n"
            "\twidth_min: %d\twidth_max: %d\n"
            "\tbeats_per_bar: %d\n\tbeat_type: %d\n"
            "\tbeat_ratio: %f\n",
            pd->bar_trig,       pd->loop_length,
            pd->width_min,      pd->width_max,
            pd->height_min,     pd->height_max,
            pd->beats_per_bar,  pd->beat_type,
            pd->beat_ratio);
}


static pdata* pdata_pri_new(void)
{
    pdata* pd = malloc(sizeof(*pd));

    if (!pd)
    {
        MESSAGE("out of memory for pdata\n");
        return 0;
    }

    memset(pd, 0, sizeof(*pd));

    pd->width_min = pd->height_min = 4;
    pd->width_max = pd->height_max = 16;

    return pd;
}


/* ---------------------------------------------------------------------
 * * * * * * PLIST * * * * */

static bbt_t        plist_default_duration = 0;
static signed char  plist_default_width = 0;
static signed char  plist_default_height = 0;


plist* plist_new(void)
{
    plist* pl = malloc(sizeof(*pl));

    if (!pl)
    {
        WARNING("out of memory for new plist\n");
        return 0;
    }

    pl->ll = llist_new( sizeof(event),
                        free,
                        llist_datacb_dup,
                        memcpy,
                        event_get_cmp_cb(EV_TIME),
                        event_get_str_cb()  );

    if (!pl->ll)
    {
        free(pl);
        return 0;
    }

    pl->pat = 0;

    return pl;
}


static event* plist_pri_to_array(const plist* pl)
{
    lnode* ln = llist_tail(pl->ll);

    ev_sel_time sel = { .start = 0, .end = 1 };

    if (ln)
        sel.end = ((event*)lnode_data(ln))->pos + 1;

    event terminator;
    event_init(&terminator);

    event* ev_arr = llist_select_to_array(  pl->ll,
                                            event_get_sel_cb(EV_TIME),
                                            &sel,
                                            &terminator);
    return ev_arr;
}


static void plist_pri_dump_cb(const void* data)
{
    event_dump(data);
}


size_t plist_event_count(const plist* pl)
{
    return llist_lnode_count(pl->ll);
}


lnode* plist_head(const plist* pl)
{
    return llist_head(pl->ll);
}


lnode* plist_tail(const plist* pl)
{
    return llist_tail(pl->ll);
}


plist*  plist_dup(const plist* pl)
{
    plist* newpl = plist_new();

    if (!newpl)
        return 0;

    if (!(newpl->ll = llist_dup(pl->ll)))
    {
        free(newpl);
        return 0;
    }

    return newpl;
}


void plist_free(plist* pl)
{
    if (!pl)
        return;
    llist_free(pl->ll);
    free(pl);
}


void plist_set_default_duration(bbt_t d)
{
    if (d > 0)
        plist_default_duration = d;
}


void plist_set_default_width(signed char w)
{
    if (w > -1)
        plist_default_width = w;
}


void plist_set_default_height(signed char h)
{
    if (h > -1)
        plist_default_height = h;
}


lnode* plist_add_event(plist* pl, event* ev)
{
    #ifdef PATTERN_DEBUG
    MESSAGE("adding event to plist %p position %d\n",
            (void*)pl, ev->pos);
    #endif

    return llist_add_data(pl->ll, ev);
}


lnode* plist_add_event_new(plist* pl, bbt_t start_tick)
{
    event* ev = event_new();

    if (!ev)
        return 0;

    ev->pos =  start_tick;
    ev->note_dur =  plist_default_duration;

    ev->box_release =   0;
    ev->box_width =     plist_default_width;
    ev->box_height =    plist_default_height;

    #ifdef PATTERN_DEBUG
    MESSAGE("pos:%d dur:%d w:%d h:%d\n",ev->pos,
                                        ev->note_dur,
                                        ev->box_width,
                                        ev->box_height  );
                                        
    #endif

    lnode* ln = plist_add_event(pl, ev);

    if (!ln)
        free(ev);

    return ln;
}


lnode* plist_add_event_copy(plist* pl, event* ev)
{
    event* evcopy = event_new();

    if (!ev)
        return 0;

    event_copy(evcopy, ev);

    lnode* ln = plist_add_event(pl, evcopy);

    if (!ln)
        free(evcopy);

    return ln;
}

lnode* plist_unlink(plist* pl, lnode* ln)
{
    #ifdef PATTERN_DEBUG
    MESSAGE("unlinking lnode:%p from plist:%p\n",
            (void*)pl, (void*)ln);
    #endif

    return llist_unlink(pl->ll, ln);
}

void plist_unlink_free(plist* pl, lnode* ln)
{
    llist_unlink_free(pl->ll, ln);
}


lnode* plist_select(const plist* pl, bbt_t start_tick, bbt_t end_tick)
{
    #ifdef PATTERN_DEBUG
    MESSAGE("selecting within plist:%p positions %d - %d\n",
            (const void*)pl, start_tick, end_tick);

    if (start_tick >= end_tick)
    {
        WARNING("selection bounds error\n");
        return 0;
    }
    #endif

    ev_sel_time sel = { .start = start_tick, .end = end_tick };

    return llist_select(pl->ll, event_get_sel_cb(EV_TIME), &sel);
}


lnode* plist_invert_selection(const plist* pl)
{
    #ifdef PATTERN_DEBUG
    MESSAGE("inverting selection within plist:%p\n", (const void*)pl);
    #endif

    return llist_select_invert(pl->ll);
}


lnode* plist_select_all(const plist* pl, _Bool sel)
{
    #ifdef PATTERN_DEBUG
    MESSAGE("select all within plist:%p\n", (const void*)pl);
    #endif

    return llist_select_all(pl->ll, sel);
}


plist* plist_cut(plist* pl, _Bool sel)
{
    #ifdef PATTERN_DEBUG
    MESSAGE("cutting from plist:%p select:%d\n", (const void*)pl, sel);
    #endif

    bbt_t mod = -1;

    llist* newll = llist_cut(pl->ll,
                                sel, event_get_mod_cb(EV_TIME), &mod);

    if (!newll)
        return 0;

    plist* newpl = malloc(sizeof(*newpl));

    if (!newpl)
    {
        MESSAGE("out of memory creating plist\n");
        return 0;
    }

    newpl->ll = newll;
    newpl->pat = 0;

    return newpl;
}


plist* plist_copy(const plist* pl, _Bool sel)
{
    bbt_t mod = -1;

    #ifdef PATTERN_DEBUG
    MESSAGE("copying from plist:%p select:%d\n", (const void*)pl, sel);
    MESSAGE("mod:%p %d\n", &mod, mod);
    #endif

    llist* newll = llist_copy(pl->ll,
                                 sel, event_get_mod_cb(EV_TIME), &mod);

    if (!newll)
        return 0;

    plist* newpl = malloc(sizeof(*newpl));

    if (!newpl)
    {
        MESSAGE("out of memory creating plist\n");
        return 0;
    }

    newpl->ll = newll;
    newpl->pat = 0;

    return newpl;
}


void plist_delete(plist* pl, _Bool sel)
{
    #ifdef PATTERN_DEBUG
    MESSAGE("deleting within plist:%p select:%d\n", (const void*)pl, sel);
    #endif

    llist_delete(pl->ll, sel);
}


void plist_paste(plist* dest, float offset, const plist* src)
{
    #ifdef PATTERN_DEBUG
    MESSAGE("inserting plist:%p into plist:%p\n",
            (void*)dest, (const void*)src);
    #endif

    llist_paste(dest->ll,
                src->ll,
                event_get_edit_cb(EV_POS_ADD),
                &offset );
}


void plist_edit(plist* pl, evcb_type ecb, float val, _Bool sel)
{
    #ifdef PATTERN_DEBUG
    MESSAGE("editing plist:%p cb_type:%d val:%f sel:%d\n",
            (void*)pl, ecb, val, sel);
    #endif

    datacb_edit cb = event_get_edit_cb(ecb);

    if (!cb)
        return;

    llist_edit(pl->ll, cb, &val, sel);

    if ((ecb == EV_POS_ADD || ecb == EV_POS_MUL) && sel)
        llist_sort(pl->ll);
}


void plist_dump(const plist* pl)
{
    MESSAGE("plist:%p\n", pl);
    if (!pl)
        return;
    MESSAGE("%d events:\n", llist_lnode_count(pl->ll));
    llist_dump(pl->ll, 0);
}


void plist_dump_events(const plist* pl)
{
    MESSAGE("plist:%p\n", pl);
    if (!pl)
        return;
    MESSAGE("%d events:\n", llist_lnode_count(pl->ll));
    llist_dump(pl->ll, plist_pri_dump_cb);
}


/* ---------------------------------------------------------------------
 * * * * * THE RTDATA STRUCT IS THE ONLY DATA THE RT THREAD * * * * *
 * * * * * SHOULD OPERATE ON. THE RTDATA FUNCTIONS ARE THE  * * * * *
 * * * * * ONLY FUNCTIONS THE RT THREAD SHOULD CALL.        * * * * */

static prtdata*  prtdata_pri_new(void);
static void     prtdata_pri_free(prtdata*);


void prtdata_trigger(prtdata* prt, bbt_t start_tick, bbt_t end_tick)
{
    prt->triggered =    1;
    prt->start_tick =   start_tick;
    prt->end_tick =     end_tick;
    prt->index =        0;

    switch(prt->pd->seedtype)
    {
    case PDSEED_TIME_SYS:
        g_rand_set_seed(prt->rnd, (guint32)time(0));
        break;

    case PDSEED_TIME_TICKS:
        g_rand_set_seed(prt->rnd, (guint32)start_tick);
        break;

    case PDSEED_USER:
        g_rand_set_seed(prt->rnd, prt->pd->seed);
        break;

    default:
        WARNING("unknown seedtype for pattern\n");
        g_rand_set_seed(prt->rnd, 1);
    }

}


void prtdata_play(prtdata* prt, bbt_t ph, bbt_t nph)
{
    pdata* pd =         prt->pd;
    event* ev_arr =     prt->ev_arr;

    if (!pd || !ev_arr)
        return;

    prt->pd_in_use =    pd;
    prt->ev_arr_in_use =ev_arr;

    bbt_t   tick =       ph - prt->start_tick;
    int     num_played = tick / pd->loop_length;

    bbt_t   offset = prt->start_tick + (pd->loop_length * num_played);
    bbt_t   nextoffset = offset + pd->loop_length;

    int count = 1;

    prt->index = tick % pd->loop_length;

    prt->playing = 1;

    event* ev = ev_arr;
    event* ev_out;

    event tmp;

    int n;
    bbt_t evpos;

    _Bool play_event;

    while (ev->pos > -1)
    {
        play_event = 0;
        evpos = ev->pos + offset;

        if (evpos >= ph && evpos < nph)
            play_event = 1;
        else
        {
            evpos = ev->pos + nextoffset;
            if (evpos >= ph && evpos < nph)
                play_event = 1;
        }

        if (play_event)
        {
            ev->box_width = g_rand_int_range(   prt->rnd,
                                                pd->width_min,
                                                pd->width_max  );

            ev->box_height = g_rand_int_range(  prt->rnd,
                                                pd->height_min,
                                                pd->height_max  );

            if ((ev_out = evport_write_event(prt->evoutput, ev)))
            {
                ev_out->pos = evpos;
                ev_out->note_dur += evpos;
                ev_out->box_release += ev_out->note_dur;
            }
            else
                WARNING("dropped event\n");
        }
        ++ev;
    }
}


void prtdata_stop(prtdata* prt)
{
    prt->playing =      0;
    prt->start_tick =   0;
    prt->end_tick =     0;
    prt->index =        0;
}


static prtdata* prtdata_pri_new(void)
{
    prtdata* prt = malloc(sizeof(*prt));

    if (!prt)
        goto fail;

    memset(prt, 0, sizeof(*prt));

    prt->rnd = g_rand_new();

    return prt;

fail:
    free(prt);
    MESSAGE("out of memory for prtdata\n");
    return 0;
}


static void prtdata_pri_free(prtdata* prt)
{
    if (!prt)
        return;
    free(prt->pd_free);
    free(prt->pd_old);
    free(prt->pd);
    free(prt->ev_arr_free);
    free(prt->ev_arr_old);
    free(prt->ev_arr);
    free(prt->rnd);
    free(prt);
}


/* ---------------------------------------------------------------------
 * * * * * * PATTERN * * * * */


pattern* default_pattern = 0;

static int pattern_id = 0;


pattern* pattern_new(void)
{
    pattern* pat = malloc(sizeof(*pat));

    if (!pat)
    {
        WARNING("out of memory allocating pattern structure\n");
        return 0;
    }

    pat->pd = pdata_pri_new();
    pat->pl = plist_new();
    pat->prt = prtdata_pri_new();

    if (!pat->pd || !pat->pl || !pat->prt)
    {
        free(pat->pd);
        free(pat->pl);
        free(pat->prt);
        free(pat);
        return 0;
    }

    pat->pl->pat = pat;

    pat->name = name_and_number("pattern", ++pattern_id);;

    return pat;
}


pattern* pattern_dup(const pattern* src)
{
    pattern* dest = pattern_new();

    if (!dest)
    {
        WARNING("pattern %p not duplicated\n",(const void*)src);
        return 0;
    }

    memcpy(dest->pd, src->pd, sizeof(*src->pd));
    plist_paste(dest->pl, 0, src->pl);

    dest->pl->pat = dest;

    pattern_prtdata_update(dest);

    return dest;
}


void pattern_free(pattern* pat)
{
    if (!pat)
        return;
    free(pat->pd);
    pat->pl->pat = 0;
    plist_free(pat->pl);
    prtdata_pri_free(pat->prt);
    free(pat->name);
    free(pat);
}


void pattern_dump(const pattern* pat)
{
    MESSAGE("pattern: %p\n\tname:%s\n", (const void*)pat, pat->name);
    pdata_dump(pat->pd);
    plist_dump(pat->pl);
}


void pattern_set_output_port(pattern* pat, evport* port)
{
    pat->prt->evoutput = port;
}


void pattern_prtdata_update(const pattern* pat)
{
    struct timespec req = { .tv_sec = 0, .tv_nsec = 50 };
    struct timespec rem = { 0, 0 };

    #ifdef PATTERN_DEBUG
    MESSAGE("updating RT data for %s\n", pat->name);
    #endif

    prtdata* prt = pat->prt;

    pdata* pd_free =     prt->pd_free;
    event* ev_arr_free = prt->ev_arr_free;

    pdata* pd_copy =     pdata_dup(pat->pd);
    event* ev_arr_copy = plist_pri_to_array(pat->pl);

    #ifdef PATTERN_DEBUG
    MESSAGE("RT pnote array:\n");
    {
        event* ev = ev_arr_copy;
        while(ev->pos != -1)
        {
            event_dump(ev);
            ev++;
        }
    }
    #endif

    prt->pd_free =     prt->pd_old;
    prt->ev_arr_free = prt->ev_arr_old;

    prt->pd_old =     prt->pd;
    prt->ev_arr_old = prt->ev_arr;

    prt->pd =     pd_copy;
    prt->ev_arr = ev_arr_copy;

    /*  a (hopefully) very quick tiny little spinlock just to make
        sure a quick sucession of updates does not free the data
        currently in use by the RT thread
    */
    if (pd_free || ev_arr_free)
    {
        while(prt->pd_in_use == pd_free
           || prt->ev_arr_in_use == ev_arr_free)
        {
            nanosleep(&req, &rem);
        }

        free(pd_free);
        free(ev_arr_free);
    }
}

