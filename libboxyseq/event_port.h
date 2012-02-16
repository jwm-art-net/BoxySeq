#ifndef EVENT_PORT_H
#define EVENT_PORT_H


#ifdef __cplusplus
extern "C" {
#endif


#include "event_pool.h"

/*  we want to connect a pattern to several boundary within a grid.

    (each boundary in different position with different dimension)

    this was not possible using a ring buffer as the first boundary
    to read an event would claim that event and no other boundary
    would be able to read that event.

    secondly,

    we want to connect several patterns to a single boundary.

    each pattern is processed for the cycle, sequentially, one
    pattern, then the next, then the next.

    in the case where each pattern outputs more than one midi message
    in the cycle, and when using a ring buffer to output the midi data
    to, the data may end up unordered.

    MIDI data messages (using the JACK API at least) must be ordered.

    the event_port data structure is used to over come both of these
    problems.

*/


typedef struct event_port evport;

/*  generally, the event_port_manager will create and destroy
    the event ports, aswell as handle the event_pool memory
    allocation.

    evport_new and evport_free are only provided for one or
    two cases where ports outside of the event_port_manager
    are required.

    if you called evport_new with an negative id, it will be
    named ("global port %d", -id).
    positive id: ("port %d", id);
*/

evport*     evport_new( evpool* pool,   const char* name,
                                        int id,
                                        int rt_evlist_sort_flags);

void        evport_free(evport* port);

const char* evport_name(evport*);

int         evport_write_event(evport*, const event*);

void        evport_clear_data(evport*);
void        evport_read_reset(evport*);
int         evport_read_event(evport*, event* dest);
int         evport_read_and_remove_event(evport*, event* dest);
void        evport_and_remove_event(evport*);

int         evport_count(evport*);

void        evport_pre_flush_check(evport*);

#ifdef EVPORT_DEBUG
void        evport_dump(evport*);
#endif


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif

