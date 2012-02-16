#include "grbound_manager.h"


#include "debug.h"
#include "llist.h"
#include "real_time_data.h"


#include <glib.h>   /* mersene twister RNG */
#include <stdlib.h>
#include <string.h>


#include "include/grid_boundary_data.h" /* to take sizeof */


struct grbound_manager
{
    llist*  grblist;
    lnode*  cur;

    int next_grbound_id;

    rtdata*     rt;

};


static void* grbound_manager_rtdata_get_cb(const void* grbman);
static void  grbound_manager_rtdata_free_cb(void* grbman);


static void grbound_free_cb(void* data)
{
    grbound_free(data);
}


grbound_manager* grbound_manager_new(void)
{
    grbound_manager* grbman = malloc(sizeof(*grbman));

    if (!grbman)
        goto fail0;

    grbman->grblist = llist_new(    sizeof(grbound),
                                    grbound_free_cb,
                                    0, /* no duplication */
                                    0, /* no copy */
                                    0, /* no compare */
                                    0  /* no string representation */
                               );
    if (!grbman->grblist)
        goto fail1;

    grbman->rt = rtdata_new(grbman, grbound_manager_rtdata_get_cb,
                                    grbound_manager_rtdata_free_cb);

    if (!grbman->rt)
        goto fail2;

    grbman->cur = 0;
    grbman->next_grbound_id = 1;

    return grbman;

fail2:  llist_free(grbman->grblist);
fail1:  free(grbman);
fail0:  WARNING("out of memory for new grid boundary manager\n");
    return 0;
}


void grbound_manager_free(grbound_manager* grbman)
{
    if (!grbman)
        return;

    rtdata_free(grbman->rt);
    llist_free(grbman->grblist);
    free(grbman);
}


grbound* grbound_manager_grbound_new(grbound_manager* grbman)
{
    grbound* grb = grbound_new();

    grbman->next_grbound_id++;

    if (!grb)
        return 0;

    if (!llist_add_data(grbman->grblist, grb))
    {
        grbound_free(grb);
        return 0;
    }

    return grb;
}


grbound* grbound_manager_grbound_first(grbound_manager* grbman)
{
    return lnode_data(grbman->cur = llist_head(grbman->grblist));
}


grbound* grbound_manager_grbound_next(grbound_manager* grbman)
{
    if (!grbman->cur)
        return 0;

    grbman->cur = lnode_next(grbman->cur);

    return (grbman->cur) ? lnode_data(grbman->cur) : 0;
}


void grbound_manager_grbound_order(grbound_manager* grbman,
                                                grbound* grb,
                                                int dir)
{
    lnode* ln = llist_head(grbman->grblist);

    while (ln && lnode_data(ln) != grb)
        ln = lnode_next(ln);

    if (!ln)
        return;

    llist_order_node(grbman->grblist, ln, dir);
}


static void* grbound_manager_rtdata_get_cb(const void* data)
{
    const grbound_manager* grbman = data;
    return llist_to_pointer_array(grbman->grblist);
}


static void  grbound_manager_rtdata_free_cb(void* data)
{
    free(data); /* we're freeing the pointer array not the ports! */
}


void grbound_manager_update_rt_data(const grbound_manager* grbman)
{
    rtdata_update(grbman->rt);
}


void grbound_manager_rt_pull_starting(grbound_manager* grbman,
                                            evport* grid_intersort)
{
    grbound** grb = rtdata_data(grbman->rt);

    if (!grb)
        return;

    while(*grb)
        grbound_rt_pull_starting(*grb++, grid_intersort);
}

#ifndef NDEBUG
void    grbound_manager_rt_check_incoming(grbound_manager* grbman,
                                                bbt_t ph, bbt_t nph)
{
    grbound** grb = rtdata_data(grbman->rt);

    if (!grb)
        return;

    while(*grb)
        grbound_rt_check_incoming(*grb++, ph, nph);
}
#endif

void grbound_manager_rt_empty_incoming(grbound_manager* grbman)
{
    grbound** grb = rtdata_data(grbman->rt);

    if (!grb)
        return;

    while(*grb)
        grbound_rt_empty_incoming(*grb++);
}


void grbound_manager_dump_events(grbound_manager* grbman)
{
    grbound* grb = grbound_manager_grbound_first(grbman);

    DMESSAGE("grbound manager:%p\n", grbman);
    DMESSAGE("ports:\n");

    if (!grb)
    {
        DMESSAGE("none\n");
        return;
    }

    do
    {
        grbound_dump_events(grb);
        grb = grbound_manager_grbound_next(grbman);
    } while(grb);
}


/*
void grbound_manager_rt_sort(grbound_manager* grbman, evport* grid_port)
{
    grbound** grb = rtdata_data(grbman->rt);

    if (!grb)
        return;

    while(*grb)
        grbound_rt_sort(*grb++, grid_port);
}
*/
