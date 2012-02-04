#include "event_port_manager.h"


#include "debug.h"
#include "llist.h"
#include "real_time_data.h"


#include <stdlib.h>
#include <string.h>


#include "include/event_port_data.h" /* to take sizeof */


struct event_port_manager
{
    llist*  portlist;
    lnode*  cur;

    evpool* event_pool;
    int     next_port_id;
    char*   groupname;

    rtdata*     rt;
};


static void* evport_manager_rtdata_get_cb(const void* portman);
static void  evport_manager_rtdata_free_cb(void* portman);


static void evport_free_cb(void* data)
{
    evport_free(data);
}


evport_manager* evport_manager_new(const char* groupname)
{
    evport_manager* portman = malloc(sizeof(*portman));

    if (!portman)
        goto fail0;

    if (!(portman->groupname = strdup(groupname)))
        goto fail1;

    DMESSAGE("new port manager \"%s\"\n", portman->groupname);

    portman->event_pool = evpool_new(DEFAULT_EVPOOL_SIZE * 16,
                                                portman->groupname);

    if (!portman->event_pool)
        goto fail2;

    portman->portlist = llist_new(  sizeof(evport),
                                    evport_free_cb,
                                    0, /* no evport duplication */
                                    0, /* no evport copy */
                                    0, /* no evport compare */
                                    0  /* no string representation */
                                     );
    if (!portman->portlist)
        goto fail3;

    portman->rt = rtdata_new(portman,   evport_manager_rtdata_get_cb,
                                        evport_manager_rtdata_free_cb);

    if (!portman->rt)
        goto fail4;

    portman->cur = 0;
    portman->next_port_id = 1;

    return portman;

fail4:  llist_free(portman->portlist);
fail3:  evpool_free(portman->event_pool);
fail2:  free(portman->groupname);
fail1:  free(portman);
fail0:  WARNING("out of memory for new event port manager\n");
    return 0;
}


void evport_manager_free(evport_manager* portman)
{
    if (!portman)
        return;

    rtdata_free(portman->rt);
    free(portman->groupname);
    llist_free(portman->portlist);
    evpool_free(portman->event_pool);
    free(portman);
}


evport* evport_manager_evport_new(  evport_manager* portman,
                                    const char* name,
                                    int rt_evlist_sort_flags )
{
    char tmp[80];
    snprintf(tmp, 79, "%s-%s", portman->groupname, name);
    tmp[79] = '\0';

    evport* port = evport_new(  portman->event_pool,
                                tmp,
                                portman->next_port_id++,
                                rt_evlist_sort_flags    );

    if (!port)
        return 0;

    lnode* ln = llist_add_data(portman->portlist, port);

    if (!ln)
    {
        evport_free(port);
        return 0;
    }

    return port;
}


evport* evport_manager_evport_first(evport_manager* portman)
{
    return lnode_data(portman->cur = llist_head(portman->portlist));
}


evport* evport_manager_evport_next(evport_manager* portman)
{
    if (!portman->cur)
        return 0;

    return lnode_data(portman->cur = lnode_next(portman->cur));
}


static void* evport_manager_rtdata_get_cb(const void* data)
{
    const evport_manager* portman = data;
    return llist_to_pointer_array(portman->portlist);
}


static void  evport_manager_rtdata_free_cb(void* data)
{
    free(data); /* we're freeing the pointer array not the ports! */
}


void evport_manager_update_rt_data(const evport_manager* portman)
{
    rtdata_update(portman->rt);
}


void evport_manager_rt_clear_all(evport_manager* portman)
{
    evport** ports = rtdata_data(portman->rt);

    if (!ports)
        return;

    while(*ports)
        evport_clear_data(*ports++);
}

