#ifndef EVENT_POOL_H
#define EVENT_POOL_H


#ifdef __cplusplus
extern "C" {
#endif


#include "event.h"


/*  evpool is a memory pool for allocating events from in real time.
*/

typedef struct event_pool evpool;

evpool*     evpool_new(int count, const char* name);
void        evpool_free(evpool*);

event*      evpool_event_alloc(evpool*);
void        evpool_event_free(evpool*, event*);


/*  rt_event_list uses the event pool for memory management
*/

enum RT_EVLIST_SORT_FLAGS
{
    RT_EVLIST_SORT_POS = 0x0001,
    RT_EVLIST_SORT_DUR,
    RT_EVLIST_SORT_REL,
};


typedef struct rt_event_list rt_evlist;
typedef void (*rt_evlist_cb)(event* ev);


rt_evlist*  rt_evlist_new(evpool*, int flags, const char* name);
void        rt_evlist_free(rt_evlist*);
int         rt_evlist_count(rt_evlist*);

#ifdef EVPOOL_DEBUG
void        rt_evlist_integrity_dump(rt_evlist*, const char* from);
#endif


/* adding events copies them */
int         rt_evlist_event_add(rt_evlist*, const event*);

void        rt_evlist_clear_events(rt_evlist*);

/*  two different methods of accessing the event list:
    don't intermix them unless you enjoy confusing yourself.
*/

event*      rt_evlist_goto_first(rt_evlist*);
event*      rt_evlist_goto_next(rt_evlist*);

/* read pointers are not disrupted by adding events */
void        rt_evlist_read_reset(rt_evlist*);
event*      rt_evlist_read_event(rt_evlist*);
event*      rt_evlist_read_and_remove_event(rt_evlist* rtevl,
                                                event* dest   );

void        rt_evlist_and_remove_event(rt_evlist* rtevl);


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
