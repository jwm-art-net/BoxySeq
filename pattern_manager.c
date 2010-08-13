#include "pattern_manager.h"


#include "debug.h"
#include "llist.h"
#include "real_time_data.h"


#include <glib.h>   /* mersene twister RNG */
#include <stdlib.h>
#include <string.h>


#include "include/event_pattern_data.h" /* to take sizeof */


struct pattern_manager
{
    llist*  patlist;
    lnode*  cur;

    int next_pattern_id;

    rtdata*     rt;
};


static void* pattern_manager_rtdata_get_cb(const void* patman);
static void  pattern_manager_rtdata_free_cb(void* patman);


static void pattern_free_cb(void* data)
{
    pattern_free(data);
}


pattern_manager* pattern_manager_new(void)
{
    pattern_manager* patman = malloc(sizeof(*patman));

    if (!patman)
        goto fail0;

    patman->patlist = llist_new(    sizeof(pattern),
                                    pattern_free_cb,
                                    0, /* no duplication */
                                    0, /* no copy */
                                    0, /* no compare */
                                    0  /* no string representation */
                               );
    if (!patman->patlist)
        goto fail1;

    patman->rt = rtdata_new(patman, pattern_manager_rtdata_get_cb,
                                    pattern_manager_rtdata_free_cb);

    if (!patman->rt)
        goto fail2;

    patman->cur = 0;
    patman->next_pattern_id = 1;

    return patman;

fail2:  llist_free(patman->patlist);
fail1:  free(patman);
fail0:  WARNING("out of memory for new pattern manager\n");
    return 0;
}


void pattern_manager_free(pattern_manager* patman)
{
    if (!patman)
        return;

    rtdata_free(patman->rt);
    llist_free(patman->patlist);
    free(patman);
}


pattern* pattern_manager_pattern_new(pattern_manager* patman)
{
    pattern* pat = pattern_new(patman->next_pattern_id++);

    if (!pat)
        return 0;

    if (!llist_add_data(patman->patlist, pat))
    {
        pattern_free(pat);
        return 0;
    }

    return pat;
}


pattern* pattern_manager_pattern_first(pattern_manager* patman)
{
    return lnode_data(patman->cur = llist_head(patman->patlist));
}


pattern* pattern_manager_pattern_next(pattern_manager* patman)
{
    if (!patman->cur)
        return 0;

    return lnode_data(patman->cur = lnode_next(patman->cur));
}


static void* pattern_manager_rtdata_get_cb(const void* data)
{
    const pattern_manager* patman = data;
    return llist_to_pointer_array(patman->patlist);
}


static void  pattern_manager_rtdata_free_cb(void* data)
{
    free(data); /* we're freeing the pointer array not the ports! */
}


void pattern_manager_update_rt_data(const pattern_manager* patman)
{
    rtdata_update(patman->rt);
}


void pattern_manager_rt_play(   pattern_manager* patman,
                                _Bool repositioned,
                                bbt_t ph,
                                bbt_t nph )
{
    pattern** p = rtdata_data(patman->rt);

    if (!p)
        return;

    while(*p)
        pattern_rt_play(*p++, repositioned, ph, nph);
}
