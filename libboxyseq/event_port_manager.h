#ifndef EVENT_PORT_MANAGER_H
#define EVENT_PORT_MANAGER_H


#ifdef __cplusplus
extern "C" {
#endif


#include "event_port.h"


typedef struct event_port_manager evport_manager;

evport_manager* evport_manager_new(const char* groupname);
void            evport_manager_free(evport_manager*);

evport*         evport_manager_evport_new(  evport_manager*,
                                            const char* name,
                                            int rt_evlist_sort_flags );

evport*         evport_manager_evport_first(evport_manager*);
evport*         evport_manager_evport_next(evport_manager*);


/*  the first of these need only be called if the second ever is.
    furthermore, if the second of these is called, then the first
    must be called after a port has been created.
*/

void            evport_manager_update_rt_data(const evport_manager*);
void            evport_manager_rt_clear_all(evport_manager*);


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
