#include "event_port.h"


#include "debug.h"
#include "llist.h"

#include <stdlib.h>
#include <string.h>


/*  how do we design the event_port data structure and methods?

    what does it do?

    it stores slightly unordered data passed to it from multiple
    sources in an orderly fashion which allows that data to be
    read by multiple readers.

    what do we need?

    _In_    we need to be able to add data to the port
            maybe from several different sources.

    _Out_   we need to read this same data from the port,
            possibly several times.

    1) what does _In_ infer?

        *   a function to write data, the port does not
            care where this function is called from or
            anything about it.

        *   the function sorts the data (via ordered list insertion).

    2) what does _Out_ infer?

        *   a function to read data, the event_port does not
            care where this function is called from or
            anything about it.

        *   there must be some kind of reset so that the same
            data can be read for multiple destinations.
            (list_goto_head).

    3) what  does 1) and 2) infer?

        *   there must be some kind of reset to clear the port
            of data ready for a new run (a new process cycle).
            (delete all nodes in the list).

    anything else?

    where does the port exist, or, what has ownership of it?

    a)  if the provider of data incoming to the port has ownership:

        _q_ how does having multiple providers work?
        _a_ it cannot work, providers must share it.

    b)  if the reader of data outcoming from the port has ownership:

        _q_ how does having multiple readers work?
        _a_ it cannot work, readers must share it.

    *** if ports are not owned by providers of data nor by
    *** readers of data...

    *** ports are stored in a central container of ports
    *** ports must be requested or associated

    *** furthermore, the UI will will need some kind of idea
    *** of all these connections.

*/


struct event_port
{
    char* name;
    int id;
    rt_evlist*  data;
};


static evport* evport_private_new(  evpool* pool,   const char* name,
                                    int id, int rt_evlist_sort_flags  )
{
    evport* port = malloc(sizeof(*port));

    if (!port)
        goto fail0;

    port->name = name_and_number(name, id);

    if (!port->name)
        goto fail1;

    port->data = rt_evlist_new(pool, rt_evlist_sort_flags);

    if (!port->data)
        goto fail2;

    port->id = id;

    return port;

fail2:
    free(port->name);

fail1:
    free(port);

fail0:
    WARNING("out of memory for new event port\n");
    return 0;
}


static void evport_free(evport* port)
{
    if (!port)
        return;

    rt_evlist_free(port->data);
    free(port->name);
    free(port);
}


event* evport_write_event(evport* port, const event* ev)
{
    return rt_evlist_event_add(port->data, ev);
}


void evport_clear_data(evport* port)
{
    rt_evlist_clear_events(port->data);
}


void evport_read_reset(evport* port)
{
    rt_evlist_read_reset(port->data);
}


int evport_read_event(evport* port, event* dest)
{
    event* ev = rt_evlist_read_event(port->data);

    if (!ev)
        return 0;

    event_copy(dest, ev);
    return 1;
}

int evport_read_and_remove_event(evport* port, event* dest)
{
    return !!rt_evlist_read_and_remove_event(port->data, dest);
}


void evport_and_remove_event(evport* port)
{
    rt_evlist_and_remove_event(port->data);
}


int evport_count(evport* port)
{
    return rt_evlist_count(port->data);
}



/*  event_port_manager

    purpose:    manages event_ports, users of event_ports request
                an evport from the manager, the manager grants the
                request.

    why?

    firstly, as shown in comment at top of file, the distribution of
    events via 'connections' requires a list data structure capable
    of a) sorting the data upon insertion, but also b) distribution
    to multiple readers. the ring buffer was going to be used for this
    but fails to meet either of those requirements.

    secondly, because events being passed around (via 'connections')
    happens in real time, the list cannot dynamically manage the memory
    required to do so. the list must use a memory pool.

    the event_port_manager provides a memory pool which all event_ports
    shall share amongst themselves.


*/





struct event_port_manager
{
    llist*  portlist;
    lnode*  cur;

    evpool* event_pool;
    int     next_port_id;
    char*   groupname;
};


static void evport_free_cb(void* data)
{
    evport_free(data);
}


evport_manager* evport_manager_new(const char* groupname)
{
    evport_manager* portman = malloc(sizeof(*portman));

    if (!portman)
        goto fail0;

    portman->event_pool = evpool_new(DEFAULT_EVPOOL_SIZE * 16);

    if (!portman->event_pool)
        goto fail1;

#ifdef EVPOOL_DEBUG
    char* tmp = jwm_strcat_alloc(groupname, "-portman");
    if (tmp)
    {
        evpool_set_origin_string(portman->event_pool, tmp);
        free(tmp);
    }
    else
        evpool_set_origin_string(portman->event_pool, groupname);
#endif

    portman->portlist = llist_new(  sizeof(evport),
                                    evport_free_cb,
                                    0, /* no evport duplication */
                                    0, /* no evport copy */
                                    0, /* no evport compare */
                                    0  /* no string representation */
                                     );
    if (!portman->portlist)
        goto fail2;

    if (!(portman->groupname = strdup(groupname)))
        goto fail3;

    portman->cur = 0;
    portman->next_port_id = 1;

    return portman;

fail3:
    llist_free(portman->portlist);
fail2:
    evpool_free(portman->event_pool);
fail1:
    free(portman);

fail0:
    WARNING("out of memory for new event port manager\n");
    return 0;
}


void evport_manager_free(evport_manager* portman)
{
    if (!portman)
        return;

    llist_free(portman->portlist);
    evpool_free(portman->event_pool);
    free(portman->groupname);
    free(portman);
}


evport* evport_manager_evport_new(  evport_manager* portman,
                                    int rt_evlist_sort_flags )
{
    evport* port = evport_private_new(  portman->event_pool,
                                        portman->groupname,
                                        portman->next_port_id,
                                        rt_evlist_sort_flags   );

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


evport* evport_manager_evport_by_id(evport_manager* portman, int id)
{
    portman->cur = llist_head(portman->portlist);
    while (portman->cur)
    {
        evport* port = lnode_data(portman->cur);

        if (port->id == id)
            return port;

        portman->cur = lnode_next(portman->cur);
    }

    return 0;
}



void evport_manager_all_evport_clear_data(evport_manager* portman)
{
    lnode* ln = llist_head(portman->portlist);

    while (ln)
    {
        evport_clear_data(lnode_data(ln));
        ln = lnode_next(ln);
    }
}
