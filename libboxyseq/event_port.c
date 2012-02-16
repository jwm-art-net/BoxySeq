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


#include "include/event_port_data.h"


evport* evport_new( evpool* pool,   const char* name,
                    int id,         int rt_evlist_sort_flags  )
{
    char tmp[80];
    evport* port = malloc(sizeof(*port));

    if (!port)
        goto fail0;

    snprintf(tmp, 79, "%s-port_%02d", name, id);
    tmp[79] = '\0';

    port->name = strdup(tmp);

    DMESSAGE("new event port \"%s\"\n",port->name);

    port->data = rt_evlist_new(pool, rt_evlist_sort_flags, port->name);

    if (!port->data)
        goto fail2;

    return port;

fail2:
    free(port->name);

    free(port);

fail0:
    WARNING("out of memory for new event port\n");
    return 0;
}


const char* evport_name(evport* ev)
{
    return ev->name;
}


void evport_free(evport* port)
{
    if (!port)
        return;

    #ifdef EVPOOL_DEBUG
    evport_dump(port);
    #endif

    if (rt_evlist_count(port->data))
    {
        event ev;

        rt_evlist_read_reset(port->data);

        while(rt_evlist_read_and_remove_event(port->data, &ev))
            ;
    }

    rt_evlist_free(port->data);
    free(port->name);
    free(port);
}


int evport_write_event(evport* port, const event* ev)
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



void evport_pre_flush_check(evport* port)
{
    event* ev;

    DMESSAGE("pre-flush check..\n");
    rt_evlist_read_reset(port->data);

    rt_evlist_integrity_dump(port->data, __FUNCTION__);

    rt_evlist_read_reset(port->data);

    while((ev = rt_evlist_read_event(port->data)))
    {
        if (EVENT_IS_STATUS_ON( ev ))
        {
            DWARNING("pre-flushing \07\n");
            event_dump(ev);
            rt_evlist_and_remove_event(port->data);
        }
        else
        {
            ev->pos = 0;
            ev->note_dur = 1;
            ev->box_release = 2;
        }
    }

    rt_evlist_integrity_dump(port->data, __FUNCTION__);

    DMESSAGE("pre-flush complete...\n");
}


#ifdef EVPORT_DEBUG
void evport_dump(evport* port)
{
    MESSAGE("port %s contains: %d events\n",
            port->name, rt_evlist_count(port->data));

    rt_evlist_integrity_dump(port->data, __FUNCTION__);

    if (rt_evlist_count(port->data))
    {
        event* ev;

        rt_evlist_read_reset(port->data);

        while((ev = rt_evlist_read_event(port->data)))
            event_dump(ev);
    }
}
#endif

