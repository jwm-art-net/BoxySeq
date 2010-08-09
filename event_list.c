#include "event_list.h"


#include "debug.h"


#include <stdlib.h>
#include <string.h>


struct event_list
{
    llist*  ll;
    void*   misc;
};


static bbt_t        evlist_default_duration = 0;
static signed char  evlist_default_width = 0;
static signed char  evlist_default_height = 0;


static void evlist_pri_dump_cb(const void*);


evlist* evlist_new(void)
{
    evlist* el = malloc(sizeof(*el));

    if (!el)
        goto fail0;

    el->ll = llist_new( sizeof(event),
                        free,
                        llist_datacb_dup,
                        memcpy,
                        event_get_cmp_cb(EV_TIME),
                        event_get_str_cb()  );

    if (!el->ll)
    {
        free(el);
        return 0;
    }

    el->misc = 0;

    return el;


fail0:  WARNING("out of memory for new event list\n");

    return 0;
}


evlist*  evlist_dup(const evlist* el)
{
    evlist* new_el = evlist_new();

    if (!new_el)
        return 0;

    if (!(new_el->ll = llist_dup(el->ll)))
    {
        free(new_el);
        return 0;
    }

    return new_el;
}


void evlist_free(evlist* el)
{
    if (!el)
        return;
    llist_free(el->ll);
    free(el);
}


size_t evlist_event_count(const evlist* el)
{
    return llist_lnode_count(el->ll);
}


lnode* evlist_head(const evlist* el)
{
    return llist_head(el->ll);
}


lnode* evlist_tail(const evlist* el)
{
    return llist_tail(el->ll);
}


lnode* evlist_add_event(evlist* el, event* ev)
{
    #ifdef EVLIST_DEBUG
    MESSAGE("adding event to evlist %p position %d\n",
            (void*)el, ev->pos);
    #endif

    return llist_add_data(el->ll, ev);
}


lnode* evlist_add_event_new(evlist* el, bbt_t start_tick)
{
    event* ev = event_new();

    if (!ev)
        return 0;

    EVENT_SET_TYPE_NOTE( ev );

    ev->pos =  start_tick;
    ev->note_dur =  evlist_default_duration;

    ev->box_release =   0;
    ev->box_width =     evlist_default_width;
    ev->box_height =    evlist_default_height;

    #ifdef EVLIST_DEBUG
    MESSAGE("pos:%d dur:%d w:%d h:%d\n",ev->pos,
                                        ev->note_dur,
                                        ev->box_width,
                                        ev->box_height  );
                                        
    #endif

    lnode* ln = evlist_add_event(el, ev);

    if (!ln)
        free(ev);

    return ln;
}


lnode* evlist_add_event_copy(evlist* el, event* ev)
{
    event* evcopy = event_new();

    if (!ev)
        return 0;

    event_copy(evcopy, ev);

    lnode* ln = evlist_add_event(el, evcopy);

    if (!ln)
        free(evcopy);

    return ln;
}


lnode* evlist_unlink(evlist* el, lnode* ln)
{
    #ifdef EVLIST_DEBUG
    MESSAGE("unlinking lnode:%p from evlist:%p\n",
            (void*)el, (void*)ln);
    #endif

    return llist_unlink(el->ll, ln);
}


void evlist_unlink_free(evlist* el, lnode* ln)
{
    llist_unlink_free(el->ll, ln);
}


lnode* evlist_select(const evlist* el, bbt_t start_tick, bbt_t end_tick)
{
    #ifdef EVLIST_DEBUG
    MESSAGE("selecting within evlist:%p positions %d - %d\n",
            (const void*)el, start_tick, end_tick);

    if (start_tick >= end_tick)
    {
        WARNING("selection bounds error\n");
        return 0;
    }
    #endif

    ev_sel_time sel = { .start = start_tick, .end = end_tick };

    return llist_select(el->ll, event_get_sel_cb(EV_TIME), &sel);
}


lnode* evlist_invert_selection(const evlist* el)
{
    #ifdef EVLIST_DEBUG
    MESSAGE("inverting selection within evlist:%p\n", (const void*)el);
    #endif

    return llist_select_invert(el->ll);
}


lnode* evlist_select_all(const evlist* el, _Bool sel)
{
    #ifdef EVLIST_DEBUG
    MESSAGE("select all within evlist:%p\n", (const void*)el);
    #endif

    return llist_select_all(el->ll, sel);
}


evlist* evlist_cut(evlist* el, _Bool sel)
{
    #ifdef EVLIST_DEBUG
    MESSAGE("cutting from evlist:%p select:%d\n", (const void*)el, sel);
    #endif

    bbt_t mod = -1;

    llist* newll = llist_cut(el->ll,
                                sel, event_get_mod_cb(EV_TIME), &mod);

    if (!newll)
        return 0;

    evlist* new_el = malloc(sizeof(*new_el));

    if (!new_el)
    {
        MESSAGE("out of memory creating evlist\n");
        return 0;
    }

    new_el->ll = newll;
    new_el->misc = 0;

    return new_el;
}


evlist* evlist_copy(const evlist* el, _Bool sel)
{
    bbt_t mod = -1;

    #ifdef EVLIST_DEBUG
    MESSAGE("copying from evlist:%p select:%d\n", (const void*)el, sel);
    MESSAGE("mod:%p %d\n", &mod, mod);
    #endif

    llist* newll = llist_copy(el->ll,
                                 sel, event_get_mod_cb(EV_TIME), &mod);

    if (!newll)
        return 0;

    evlist* new_el = malloc(sizeof(*new_el));

    if (!new_el)
    {
        MESSAGE("out of memory creating evlist\n");
        return 0;
    }

    new_el->ll = newll;
    new_el->misc = 0;

    return new_el;
}


void evlist_delete(evlist* el, _Bool sel)
{
    #ifdef EVLIST_DEBUG
    MESSAGE("deleting within evlist:%p select:%d\n", (const void*)el, sel);
    #endif

    llist_delete(el->ll, sel);
}


void evlist_paste(evlist* dest, float offset, const evlist* src)
{
    #ifdef EVLIST_DEBUG
    MESSAGE("inserting evlist:%p into evlist:%p\n",
            (void*)dest, (const void*)src);
    #endif

    llist_paste(dest->ll,
                src->ll,
                event_get_edit_cb(EV_POS_ADD),
                &offset );
}


void evlist_edit(evlist* el, evcb_type ecb, float val, _Bool sel)
{
    #ifdef EVLIST_DEBUG
    MESSAGE("editing evlist:%p cb_type:%d val:%f sel:%d\n",
            (void*)el, ecb, val, sel);
    #endif

    datacb_edit cb = event_get_edit_cb(ecb);

    if (!cb)
        return;

    llist_edit(el->ll, cb, &val, sel);

    if ((ecb == EV_POS_ADD || ecb == EV_POS_MUL) && sel)
        llist_sort(el->ll);
}


void evlist_dump_list(const evlist* el)
{
    MESSAGE("evlist:%p\n", el);

    if (!el)
        return;

    MESSAGE("%d events:\n", llist_lnode_count(el->ll));
    llist_dump(el->ll, 0);
}


void evlist_dump_events(const evlist* el)
{
    MESSAGE("evlist:%p\n", el);

    if (!el)
        return;

    MESSAGE("%d events:\n", llist_lnode_count(el->ll));
    llist_dump(el->ll, evlist_pri_dump_cb);
}


event* evlist_to_array(const evlist* el)
{
    lnode* ln = llist_tail(el->ll);

    ev_sel_time sel = { .start = 0, .end = 1 };

    if (ln)
        sel.end = ((event*)lnode_data(ln))->pos + 1;

    event terminator;
    event_init(&terminator);

    event* ev_arr = llist_select_to_array(  el->ll,
                                            event_get_sel_cb(EV_TIME),
                                            &sel,
                                            &terminator);
    return ev_arr;
}


/*
void evlist_set_default_duration(bbt_t d)
{
    if (d > 0)
        evlist_default_duration = d;
}


void evlist_set_default_width(signed char w)
{
    if (w > -1)
        evlist_default_width = w;
}


void evlist_set_default_height(signed char h)
{
    if (h > -1)
        evlist_default_height = h;
}
*/


static void evlist_pri_dump_cb(const void* data)
{
    event_dump(data);
}

