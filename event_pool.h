#ifndef EVENT_POOL_H
#define EVENT_POOL_H


#include "event.h"


typedef struct event_pool evpool;

typedef struct rt_event_list rt_evlist;

typedef void (*rt_evlist_cb)(event* ev);


evpool*     evpool_new(int count);
void        evpool_free(evpool*);


event*      evpool_event_alloc(evpool*);
void        evpool_event_free(evpool*, event*);


rt_evlist*  rt_evlist_new(evpool*, _Bool ordered);
void        rt_evlist_free(rt_evlist*);

/* adding events copies them */
event*      rt_evlist_event_add(rt_evlist*, const event*);

void        rt_evlist_clear_events(rt_evlist*);

/*  two different methods of accessing the event list:
    don't intermix them unless you enjoy confusing yourself.
*/

event*      rt_evlist_goto_first(rt_evlist*);
event*      rt_evlist_goto_next(rt_evlist*);


void        rt_evlist_read_reset(rt_evlist*);
event*      rt_evlist_read_event(rt_evlist*);

#endif
