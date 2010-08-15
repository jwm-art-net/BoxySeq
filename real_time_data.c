#include "real_time_data.h"


#include "debug.h"


#include <glib.h>
#include <stdlib.h>


struct real_time_data
{
    const void* data;

    datacb_rtdata   cb_rtdata_get;
    datacb_free     cb_rtdata_free;

    void* ptr;
    void* ptr_in_use;
    void* ptr_old;
    void* ptr_free;
};


rtdata* rtdata_new(const void* data,    datacb_rtdata cb_rtdata_get,
                                        datacb_free cb_rtdata_free   )
{
    rtdata* rt = malloc(sizeof(*rt));

    if (!rt)
        goto fail0;

    rt->data = data;

    rt->cb_rtdata_get =  cb_rtdata_get;
    rt->cb_rtdata_free = cb_rtdata_free;

    rt->ptr = 0;
    rt->ptr_in_use = 0;
    rt->ptr_old = 0;
    rt->ptr_free = 0;

    return rt;

fail0:
    WARNING("failed to create rtdata\n");
    return 0;
}


void rtdata_free(rtdata* rt)
{
    if (!rt)
        return;

    rt->cb_rtdata_free(rt->ptr_free);
    rt->cb_rtdata_free(rt->ptr_old);
    rt->cb_rtdata_free(rt->ptr);

    free(rt);
}


void* rtdata_update(rtdata* rt)
{
    void* ret_free = rt->ptr_free;
    void* ptr_new = rt->cb_rtdata_get(rt->data);

    #ifdef RTDATA_DEBUG
    MESSAGE("rtdata data:%p\n", rt->data);
    MESSAGE("ptr_new:%p\n", ptr_new);
    #endif

    if (!ptr_new)
        return 0;

    #ifdef RTDATA_DEBUG
    MESSAGE("rt->ptr_free:%p rt->ptr_old:%p...\n",
            rt->ptr_free, rt->ptr_old);
    MESSAGE("Updating...\n");
    #endif

    rt->ptr_free =  rt->ptr_old;
    rt->ptr_old =   g_atomic_pointer_get(&rt->ptr);

    #ifdef RTDATA_DEBUG
    MESSAGE("rt->ptr_free:%p rt->ptr_old:%p\n",
            rt->ptr_free, rt->ptr_old);
    #endif

    if (rt->ptr_free)
    {
        struct timespec req = { .tv_sec = 0, .tv_nsec = 50000 };
        struct timespec rem = { 0, 0 };

        if (g_atomic_pointer_get(&rt->ptr_in_use) == rt->ptr_free)
            nanosleep(&req, &rem);

        #ifdef RTDATA_DEBUG
        WARNING("rtdata free rt->ptr_free:%p\n", rt->ptr_free);
        #endif

        rt->cb_rtdata_free(rt->ptr_free);
        rt->ptr_free = 0; /* important to zero-out! */
    }

    g_atomic_pointer_set(&rt->ptr, ptr_new);

    return ret_free;
}


void* rtdata_data(rtdata* rt)
{
    void* ptr_new = g_atomic_pointer_get(&rt->ptr);

    g_atomic_pointer_compare_and_exchange( &rt->ptr_in_use,
                                            rt->ptr_old,
                                            ptr_new         );
    return ptr_new;
}

