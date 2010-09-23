#ifndef EVENT_LIST_H
#define EVENT_LIST_H


#ifdef __cplusplus
extern "C" {
#endif


#include "event.h"
#include "llist.h"


typedef struct event_list evlist;


evlist* evlist_new(void);
evlist* evlist_dup(const evlist*);
void    evlist_free(evlist*);

size_t  evlist_event_count(const evlist*);


lnode*  evlist_head(const evlist*);
lnode*  evlist_tail(const evlist*);


lnode*  evlist_add_event(       evlist*, event*);
lnode*  evlist_add_event_new(   evlist*, bbt_t start_tick);
lnode*  evlist_add_event_copy(  evlist*, event*);


lnode*  evlist_unlink(          evlist*, lnode*);
void    evlist_unlink_free(     evlist*, lnode*);


lnode*  evlist_select(    const evlist*,
                                bbt_t start_tick,
                                bbt_t end_tick);


lnode*  evlist_invert_selection(  const evlist*);
lnode*  evlist_select_all(        const evlist*, bool sel);


evlist* evlist_cut(         evlist*,    bool sel);
evlist* evlist_copy(  const evlist*,    bool sel);
void    evlist_delete(      evlist*,    bool sel);

void    evlist_paste(   evlist* dest, float offset, const evlist* src);
void    evlist_edit(    evlist*, evcb_type, float n, bool sel);


void    evlist_dump_list(const evlist*);
void    evlist_dump_events(const evlist*);


event*  evlist_to_array(const evlist*);


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
