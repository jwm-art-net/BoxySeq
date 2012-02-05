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

    char* name;

    #ifdef EVPOOL_DEBUG
    int min_free;
    #endif
};


evpool* evpool_new(int count, const char* name)
{
    if (count < 1)
        count = DEFAULT_EVPOOL_SIZE;

    evpool* evp = malloc(sizeof(*evp));

    DMESSAGE("new event pool \"%s\"\n", name);

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
        #ifdef EVPOOL_DEBUG
        evlnk->ev.flags = EV_IS_FREE_ERROR;
        #endif
    }

    evp->mempool[count - 1].next = 0;
    evp->count = evp->free_count = count;
    evp->memfree = evp->mempool;
    evp->name = strdup(name);


    #ifdef EVPOOL_DEBUG
    evp->min_free = evp->count;
    #endif


    return evp;
}


void evpool_free(evpool* evp)
{
    if (!evp)
        return;

    #ifdef EVPOOL_DEBUG
    MESSAGE("pool:%p name:'%s'\n", evp, evp->name);

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
    free(evp->name);
    free(evp);
}


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
    evlnk->ev.flags = 0;
#endif

    return evlnk;
}


static inline void evpool_private_event_free(   evpool* evp,
                                                rt_evlink* evlnk )
{
#ifdef EVPOOL_DEBUG
    evlnk->ev.flags = EV_IS_FREE_ERROR;
#endif

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
    bool pool_managed;

    char* name;

    int count;
};



#ifdef EVPOOL_DEBUG

void rt_evlist_integrity_dump(rt_evlist* rtevl, const char* from)
{
    rt_evlink* fwd_track[500];
    rt_evlink* rev_track[500];

    int i;
    int fwd_count = 0;
    int rev_count = 0;

    rt_evlink* fwd = rtevl->head;
    rt_evlink* rev = rtevl->tail;

    if (!fwd && !rev && !rtevl->cur)
        return;

    if (!rtevl->head)
    {
        WARNING("head is null but tail set\n");
        goto fail;
    }

    if (!rtevl->tail)
    {
        WARNING("tail is null but head set\n");
        goto fail;
    }

    for (i = 0; i < 500; ++i)
        fwd_track[i] = rev_track[i] = 0;

    while (fwd && fwd != rtevl->tail)
    {
        if (fwd->ev.flags == EV_IS_FREE_ERROR)
        {
            WARNING("link %d: is free\n", fwd);
            goto fail;
        }

        for (i = 0; i < fwd_count; ++i)
            if (fwd_track[i] == fwd)
            {
                WARNING("duplicate of link %d: %p\n", i, fwd);
                goto fail;
            }

        fwd_track[fwd_count++] = fwd;
        fwd = fwd->next;
    }

    if (!fwd)
    {
        WARNING("forward check failed: tail not reached\n");
        goto fail;
    }

    fwd_track[fwd_count] = fwd;

    while (rev && rev != rtevl->head)
    {
        if (rev->ev.flags == EV_IS_FREE_ERROR)
        {
            WARNING("link %d: is free\n", rev);
            goto fail;
        }

        for (i = 0; i < rev_count; ++i)
            if (rev_track[i] == rev)
            {
                WARNING("duplicate of link %d: %p\n", i, fwd);
                goto fail;
            }

        rev_track[rev_count++] = rev;
        rev = rev->prev;
    }

    if (!rev)
    {
        WARNING("reverse check failed: head not reached\n");
        goto fail;
    }

    rev_track[rev_count] = rev;

    bool fail = 0;

    if (fwd_count != rev_count)
    {
        WARNING("forward event count %d != reverse event count %d\n",
                fwd_count, rev_count);
        ++fail;
    }

    int last = (fwd_count > rev_count) ? fwd_count : rev_count;

    int f, r;

    for (f = 0, r = last; f < last && r > 0; ++f, --r)
    {
        if (fwd_track[f] != rev_track[r])
        {
            if (fwd_track[f + 1] == rev_track[r])
            {
                WARNING("extraneous link (no. %d) %p in fwd iteration ("
                        "or link missing in rev iteration)\n",
                        f, fwd_track[f]);
                ++fail;
                ++f;
            }
            else if (fwd_track[f] == rev_track[r - 1])
            {
                WARNING("extraneous link (no. %d) %p in rev iteration ("
                        "or link missing in fwd iteration)\n",
                        last - r, rev_track[r]);
                ++fail;
                --r;
            }
        }
    }

    if (fail)
    {
        WARNING("%d fails\n", fail);
        goto fail;
    }

    return;

fail:
    WARNING("***** integrity checks  for [%s] failed *****\n",
                                            rtevl->name);

    fwd = rtevl->head;
    fwd_count = 0;

    while (fwd)
    {
        MESSAGE("fwd: %3d: %p prev: %p next: %p\n",
                fwd_count++, fwd, fwd->prev, fwd->next);
        fwd = fwd->next;
    }

    rev = rtevl->tail;

    while (rev && rev_count > -5)
    {
        MESSAGE("rev: %3d: %p prev: %p next: %p\n",
                rev_count--, rev, rev->prev, rev->next);
        rev = rev->prev;
    }
    WARNING("CHECK ABORTED: called from %s\n", from);
}
#endif


rt_evlist*  rt_evlist_new(evpool* pool, int flags, const char* name)
{
    rt_evlist* rtevl = malloc(sizeof(*rtevl));

    if (!rtevl)
        goto fail0;

    DMESSAGE("new RT event list \"%s\"\n", name);

    if (!pool)
    {
        if (!(rtevl->pool = evpool_new(DEFAULT_EVPOOL_SIZE, name)))
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

    rtevl->name = strdup(name);

    return rtevl;

fail1:
    free(rtevl);

fail0:
    WARNING("out of memory for rt_evlist\n");
    return 0;
}


void rt_evlist_free(rt_evlist* rtevl)
{
    #ifdef EVPOOL_DEBUG999
    rt_evlist_integrity_dump(rtevl, __FUNCTION__);
    #endif

    rt_evlist_clear_events(rtevl);

    if (rtevl->pool_managed)
        evpool_free(rtevl->pool);

    free(rtevl->name);
    free(rtevl);
}


int rt_evlist_count(rt_evlist* rtevl)
{
    return rtevl->count;
}


int rt_evlist_event_add(rt_evlist* rtevl, const event* ev)
{
    #ifdef EVPOOL_DEBUG999
    rt_evlist_integrity_dump(rtevl, __FUNCTION__);
    #endif

    bbt_t newval, curval;
    rt_evlink* newlnk = evpool_private_event_alloc(rtevl->pool);
    rt_evlink* cur = rtevl->head;

    if (!newlnk)
    {
#ifdef EVPOOL_DEBUG
        WARNING("rt_evlist %p '%s', pool %p '%s', "
                "short of memory for event\n",
                rtevl,          rtevl->name,
                rtevl->pool,    rtevl->pool->name );
#else
        WARNING("rt_evlist %p, pool %p, short of memory for event\n",
                rtevl, rtevl->pool);
#endif
        return 0;
    }

    event_copy(&newlnk->ev, ev);

    if (!cur) /* no head, list is empty */
    {
        rtevl->head = rtevl->tail = newlnk;
        rtevl->head->next = 0;
        rtevl->head->prev = 0;
        rtevl->cur = rtevl->head;
        ++rtevl->count;

        return 1;
        /* ---------------------------------- */
    }

    switch (rtevl->flags)
    {
    case RT_EVLIST_SORT_POS:    newval = ev->pos;
        break;

    case RT_EVLIST_SORT_DUR:

        if ((newval = ev->note_dur) == -1)
            goto add_at_tail;

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
        case RT_EVLIST_SORT_POS:    curval = ((event*)cur)->pos;
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

            if (cur->prev)
                cur->prev->next = newlnk;

            cur->prev = newlnk;
            newlnk->next = cur;

            if (cur == rtevl->head)
                rtevl->head = newlnk;

            ++rtevl->count;

            return 1;
        }

        cur = cur->next;
    }

add_at_tail:

    rtevl->tail->next = newlnk;
    newlnk->prev = rtevl->tail;
    newlnk->next = 0;
    rtevl->tail = newlnk;

    ++rtevl->count;

    return 1;
}


void rt_evlist_clear_events(rt_evlist* rtevl)
{
    #ifdef EVPOOL_DEBUG999
    rt_evlist_integrity_dump(rtevl, __FUNCTION__);
    #endif

    rt_evlink* evlnk = rtevl->head;

    rtevl->head = rtevl->tail = 0;
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
    #ifdef EVPOOL_DEBUG999
    rt_evlist_integrity_dump(rtevl, __FUNCTION__);
    #endif

    if (!(rtevl->cur = rtevl->head))
        return 0;

    return &rtevl->cur->ev;
}


event* rt_evlist_goto_next(rt_evlist* rtevl)
{
    #ifdef EVPOOL_DEBUG999
    rt_evlist_integrity_dump(rtevl, __FUNCTION__);
    #endif
    if (!rtevl->cur)
        return 0;

    return &(rtevl->cur = rtevl->cur->next)->ev;
}


void rt_evlist_read_reset(rt_evlist* rtevl)
{
    #ifdef EVPOOL_DEBUG999
    rt_evlist_integrity_dump(rtevl, __FUNCTION__);
    #endif

    rtevl->cur = rtevl->head;
}


event* rt_evlist_read_event(rt_evlist* rtevl)
{
    #ifdef EVPOOL_DEBUG999
    rt_evlist_integrity_dump(rtevl, __FUNCTION__);
    #endif

    if (!rtevl->cur)
        return 0;

    event* ret = &rtevl->cur->ev;
    rtevl->cur = rtevl->cur->next;
    return ret;
}


event* rt_evlist_read_and_remove_event(rt_evlist* rtevl, event* dest)
{
    #ifdef EVPOOL_DEBUG999
    rt_evlist_integrity_dump(rtevl, __FUNCTION__);
    #endif

    if (!rtevl->cur)
        return 0;

    #ifdef EVPOOL_DEBUG
    if (rtevl->cur != rtevl->head)
        WARNING("%p->cur != %p->head\n", rtevl,rtevl);
    #endif

    rt_evlink* evlnk = rtevl->cur;
    event_copy(dest, &rtevl->cur->ev);
    rtevl->head = rtevl->cur = rtevl->cur->next;

    if (rtevl->tail == evlnk)
        rtevl->tail = 0;

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
    #ifdef EVPOOL_DEBUG999
    rt_evlist_integrity_dump(rtevl, __FUNCTION__);
    #endif

    rt_evlink* rem = 0;
/*
    WARNING("rtevl:%p head:%p tail:%p cur:%p\n",
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
        {
            DMESSAGE(   "rtevl->tail:%p\n"
                        "rtevl->tail->prev:%p\n",
                        rtevl->tail, rtevl->tail->prev);

            if (!rtevl->tail->prev)
                rt_evlist_integrity_dump(rtevl, __FUNCTION__);

            rtevl->tail->prev->next = 0;
        }

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
