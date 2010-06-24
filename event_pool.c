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

#ifdef EVPOOL_DEBUG
    int min_free;
    char* origin_string;
#endif

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

#ifdef EVPOOL_DEBUG
    evp->min_free = evp->count;
    evp->origin_string = 0;
#endif

    return evp;
}


void evpool_free(evpool* evp)
{
    if (!evp)
        return;

    #ifdef EVPOOL_DEBUG
    MESSAGE("pool:%p origin string:'%s'\n", evp, evp->origin_string);
    free(evp->origin_string);
    if (evp->free_count < evp->count)
        WARNING("\tmemory remaining in pool: %d... short by %d of %d\n",
                evp->free_count, evp->count - evp->free_count,
                evp->count);
    else if (evp->free_count > evp->count)
        WARNING("\tmemory remaining in pool: %d... excess by %d of %d\n",
                evp->free_count, evp->free_count - evp->count,
                evp->count);
    else
        MESSAGE("\tmemory remaining in pool: %d... ok\n",
                evp->free_count);
    MESSAGE("\tmax events allocated: %d\n", evp->count - evp->min_free);
    #endif

    free(evp->mempool);
    free(evp);
}


#ifdef EVPOOL_DEBUG
void evpool_set_origin_string(evpool* evp, const char* origin_string)
{
    if (evp->origin_string)
    {
        WARNING("pool: %p has origin already set as:'%s'\n",
                evp, evp->origin_string);
        WARNING("not changing\n");

        return;
    }

    if (!(evp->origin_string = strdup(origin_string)))
        WARNING("failed to set origin string '%s' for evpool:%p\n",
                origin_string, evp);

}
#endif

#ifdef EVPOOL_DEBUG
const char* evpool_get_origin_string(evpool* evp)
{
    return evp->origin_string;
}
#endif


static inline rt_evlink* evpool_private_event_alloc(evpool* evp)
{
    rt_evlink* evlnk = evp->memfree;

    if (!evlnk)
        return 0;

    evp->memfree = evp->memfree->next;

    --evp->free_count;

#ifdef EVPOOL_DEBUG
    if (evp->free_count < evp->min_free)
        evp->min_free = evp->free_count;
#endif

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




struct rt_event_list
{
    rt_evlink* head;
    rt_evlink* tail;

    rt_evlink* cur;

    int flags;

    evpool* pool;
    _Bool pool_managed;

#ifdef EVPOOL_DEBUG
    char* origin_string;
#endif

    int count;
};



rt_evlist*  rt_evlist_new(evpool* pool, int flags)
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
    rtevl->flags = flags;
    rtevl->count = 0;

#ifdef EVPOOL_DEBUG
    rtevl->origin_string = 0;
#endif

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

#ifdef EVPOOL_DEBUG
    free(rtevl->origin_string);
#endif

    free(rtevl);
}


int rt_evlist_count(rt_evlist* rtevl)
{
    return rtevl->count;
}


#ifdef EVPOOL_DEBUG
void rt_evlist_set_origin_string(   rt_evlist* rtevl,
                                    const char* origin_string)
{
    if (rtevl->origin_string)
    {
        WARNING("rt_evlist: %p has origin already set as:'%s'\n",
                rtevl, rtevl->origin_string);
        WARNING("not changing\n");

        return;
    }

    if (!(rtevl->origin_string = strdup(origin_string)))
        WARNING("failed to set origin string '%s' for evpool:%p\n",
                origin_string, rtevl);

    if (rtevl->pool_managed)
        evpool_set_origin_string(rtevl->pool, origin_string);

}

const char* rt_evlist_get_origin_string(rt_evlist* rtevl)
{
    return rtevl->origin_string;
}
#endif


event* rt_evlist_event_add(rt_evlist* rtevl, const event* ev)
{
    bbt_t newval, curval;
    rt_evlink* newlnk = evpool_private_event_alloc(rtevl->pool);
    rt_evlink* cur = rtevl->head;

    if (!newlnk)
    {
#ifdef EVPOOL_DEBUG
        WARNING("rt_evlist %p '%s', pool %p '%s', "
                "short of memory for event\n",
                rtevl,          rtevl->origin_string,
                rtevl->pool,    rtevl->pool->origin_string );
#else
        WARNING("rt_evlist %p, pool %p, short of memory for event\n",
                rtevl, rtevl->pool);
#endif
        return 0;
    }

    event_copy(&newlnk->ev, ev);

    if (!cur)
    {
        rtevl->head = newlnk;
        rtevl->head->next = 0;
        rtevl->head->prev = 0;
        ++rtevl->count;
        return (event*)rtevl->head;
    }

    switch (rtevl->flags)
    {
    case RT_EVLIST_SORT_POS:    newval = ev->note_pos;
        break;
    case RT_EVLIST_SORT_DUR:    newval = ev->note_dur;
        break;
    case RT_EVLIST_SORT_REL:    newval = ev->box_release;
        break;
    default:                    WARNING("ERROR: insane flags\n");
        return 0;
    }

    while(cur)
    {
        switch (rtevl->flags)
        {
        case RT_EVLIST_SORT_POS:    curval = ((event*)cur)->note_pos;
            break;
        case RT_EVLIST_SORT_DUR:    curval = ((event*)cur)->note_dur;
            break;
        case RT_EVLIST_SORT_REL:    curval = ((event*)cur)->box_release;
            break;
        default:                    WARNING("ERROR: insane flags\n");
            return 0;
        }

        if (newval < curval)
        {
            newlnk->prev = cur->prev;
            cur->prev = newlnk;
            newlnk->next = cur;

            if (cur == rtevl->head)
                rtevl->head = newlnk;

            ++rtevl->count;
            return &newlnk->ev;
        }

        if (!cur->next)
            break;

        cur = cur->next;
    }

    rtevl->tail = cur->next = newlnk;
    newlnk->prev = cur;
    newlnk->next = 0;

    ++rtevl->count;
    return &newlnk->ev;
}


void rt_evlist_clear_events(rt_evlist* rtevl)
{
    rt_evlink* evlnk = rtevl->head;

    rtevl->head = 0;
    rtevl->count = 0;

    while(evlnk)
    {
        rt_evlink* next = evlnk->next;
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
    --rtevl->count;

    return dest;
}


/*
    remove the previous event in the list.

    assumptions:
      * 1) rt_evlist_read_event was called prior to this function

      * 2) this function is only called if rt_evlist_read_event
           was successful.

    ***** FOR THIS FUNCTION TO WORK CORRECTLY BOTH THE ASSUMPTIONS
          *MUST* HOLD TRUE.

    rtevl->count is zero:
        there's nothing to remove - this function called in error

    rtevl->cur is zero:
        given assumptions, the current item is zero because the
        last item in the list was read before calling this, so,
        the last item to remove is the tail.

    rtevl->cur->prev is zero:
        given assumptions, can only assume this function called
        in error: cur would never point to the first item in list
        if rt_evlist_read_event was called prior to this function.

*/
void rt_evlist_and_remove_event(rt_evlist* rtevl)
{
    rt_evlink* rem;
/*
    MESSAGE("rtevl:%p head:%p tail:%p cur:%p\n",
            rtevl, rtevl->head, rtevl->tail, rtevl->cur);
*/
    if (!rtevl->count)
    {
        WARNING("ERROR: rt_evlist empty, nothing to remove\n");
        return;
    }

    if (!rtevl->cur) /* remove tail */
    {
        rem = rtevl->tail;

        if (rtevl->head == rtevl->tail)
            rtevl->head = rtevl->tail = 0;
        else
            rtevl->tail->prev->next = 0;

        --rtevl->count;
        evpool_private_event_free(rtevl->pool, rem);
        return;
    }

    if (rtevl->cur == rtevl->head)
    {
        WARNING("ERROR: cannot remove event previous to head\n");
        return;
    }

    rem = rtevl->cur->prev;

/*  we're being paranoid here and differentiating between the two
    error conditions on either side of this comment!
    NEITHER of which SHOULD occur.
*/

    if (!rem)
    {
        WARNING("ERROR: cannot remove non-existent previous event\n");
        return;
    }

    if (rem == rtevl->head)
    {
        rtevl->head = rtevl->cur;
        rtevl->cur->prev = 0;
        --rtevl->count;
        evpool_private_event_free(rtevl->pool, rem);
        return;
    }

    rem->prev->next = rem->next;
    rem->next->prev = rem->prev;
    --rtevl->count;
    evpool_private_event_free(rtevl->pool, rem);
}
