#include "event_pool.h"


#include "debug.h"
#include "event_buffer.h"

#include <stdlib.h>
#include <string.h>


typedef struct rt_event_link
{
    event ev;
    struct rt_event_link* prev;
    struct rt_event_link* next;

} rt_evlink;


struct event_pool
{
    int     count;
    int     free_count;
    rt_evlink*   mempool;
    rt_evlink*   memfree;
};


struct rt_event_list
{
    rt_evlink* head;
    rt_evlink* tail;

    rt_evlink* cur;

    _Bool ordered;

    evpool* pool;
    _Bool pool_managed;
};


evpool* evpool_new(int count)
{
    if (count < 1)
        count = DEFAULT_EVPOOL_SIZE;

    evpool* evp = malloc(sizeof(*evp));

    if (!evp)
    {
        WARNING("out of memory for new evpool\n");
        return 0;
    }

    evp->mempool = malloc(sizeof(*(evp->mempool)) * (size_t)count);

    if (!evp->mempool)
    {
        WARNING("failed to allocate evpool pool\n");
        free(evp);
        return 0;
    }

    int i;

    rt_evlink* evlnk = evp->mempool;

    for (i = 0; i < count; ++i, ++evlnk)
    {
        event_init(&evlnk->ev);
        evlnk->next = evlnk + 1;
    }

    evp->mempool[count - 1].next = 0;

    evp->count = evp->free_count = count;

    evp->memfree = evp->mempool;

    return evp;
}


void evpool_free(evpool* evp)
{
    if (!evp)
        return;

    #ifdef EVPOOL_DEBUG
    MESSAGE("pool:%p\n", evp);

    if (evp->free_count < evp->count)
        WARNING("memory returned to evpool:%d short by %d of %d\n",
                evp->free_count,
                evp->count - evp->free_count, 
                evp->count);

    else if (evp->free_count > evp->count)
        WARNING("memory returned to evpool:%d excess by %d of %d\n",
                evp->free_count,
                evp->free_count - evp->count, 
                evp->count);
    #endif

    free(evp->mempool);
    free(evp);
}


static inline rt_evlink* evpool_private_event_alloc(evpool* evp)
{
    rt_evlink* evlnk = evp->memfree;

    if (!evlnk)
        return 0;

    evp->memfree = evp->memfree->next;

    --evp->free_count;

    return evlnk;
}


static inline void evpool_private_event_free(   evpool* evp,
                                                rt_evlink* evlnk )
{
    evlnk->next = evp->memfree;
    evp->memfree = evlnk;
    ++evp->free_count;
}


event* evpool_event_alloc(evpool* evp)
{
    return &(evpool_private_event_alloc(evp)->ev);
}


void evpool_event_free(evpool* evp, event* ev)
{
    evpool_private_event_free(evp, (rt_evlink*)ev);
}


rt_evlist*  rt_evlist_new(evpool* pool, _Bool ordered)
{
    rt_evlist* rtevl = malloc(sizeof(*rtevl));

    if (!rtevl)
        goto fail0;

    if (!pool)
    {
        if (!(rtevl->pool = evpool_new(DEFAULT_EVPOOL_SIZE)))
            goto fail1;

        rtevl->pool_managed = 1;
    }
    else
    {
        rtevl->pool = pool;
        rtevl->pool_managed = 0;
    }

    rtevl->head = 0;
    rtevl->tail = 0;
    rtevl->cur = 0;
    rtevl->ordered = ordered;

    return rtevl;

fail1:
    free(rtevl);

fail0:
    WARNING("out of memory for rt_evlist\n");
    return 0;
}


void rt_evlist_free(rt_evlist* rtevl)
{
    rt_evlist_clear_events(rtevl);

    if (rtevl->pool_managed)
        evpool_free(rtevl->pool);

    free(rtevl);
}


event* rt_evlist_event_add(rt_evlist* rtevl, const event* ev)
{
    rt_evlink* newlnk = evpool_private_event_alloc(rtevl->pool);

    if (!newlnk)
    {
        WARNING("rt_evlist pool short of memory for event\n");
        return 0;
    }

    event_copy(&newlnk->ev, ev);

    rt_evlink* cur = rtevl->head;

    if (!cur)
    {
        rtevl->head = newlnk;
        rtevl->head->next = 0;
        rtevl->head->prev = 0;
        return (event*)rtevl->head;
    }

    while(cur)
    {
        if (ev->note_pos < ((event*)cur)->note_pos)
        {
            newlnk->prev = cur->prev;
            cur->prev = newlnk;
            newlnk->next = cur;

            if (cur == rtevl->head)
                rtevl->head = newlnk;

            return &newlnk->ev;
        }

        if (!cur->next)
            break;

        cur = cur->next;
    }

    rtevl->tail = cur->next = newlnk;
    newlnk->prev = cur;
    newlnk->next = 0;

    return &newlnk->ev;
}


void rt_evlist_clear_events(rt_evlist* rtevl)
{
    rt_evlink* evlnk = 0;
    rt_evlink* next = rtevl->head;

    rtevl->head = 0;

    while(evlnk)
    {
        next = next->next;
        WARNING("evlnk:%p next:%p\n",evlnk,next);
        evpool_private_event_free(rtevl->pool, evlnk);
        evlnk = next;
    }
}


event* rt_evlist_goto_first(rt_evlist* rtevl)
{
    if (!(rtevl->cur = rtevl->head))
        return 0;

    return &rtevl->cur->ev;
}


event* rt_evlist_goto_next(rt_evlist* rtevl)
{
    if (!rtevl->cur)
        return 0;

    return &(rtevl->cur = rtevl->cur->next)->ev;
}


void rt_evlist_read_reset(rt_evlist* rtevl)
{
    rtevl->cur = rtevl->head;
}


event* rt_evlist_read_event(rt_evlist* rtevl)
{
    if (!rtevl->cur)
        return 0;

    event* ret = &rtevl->cur->ev;
    rtevl->cur = rtevl->cur->next;
    return ret;
}


event* rt_evlist_read_and_remove_event(rt_evlist* rtevl, event* dest)
{
    if (!rtevl->cur)
        return 0;

    #ifdef EVPOOL_DEBUG
    if (rtevl->cur != rtevl->head)
        WARNING("%p->cur != %p->head\n", rtevl,rtevl);
    #endif

    rt_evlink* evlnk = rtevl->cur;
    event_copy(dest, &rtevl->cur->ev);
    rtevl->head = rtevl->cur = rtevl->cur->next;

    evpool_private_event_free(rtevl->pool, evlnk);

    return dest;
}
