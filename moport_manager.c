#include "moport_manager.h"


#include "debug.h"
#include "llist.h"
#include "midi_out_port.h"
#include "real_time_data.h"


#include <glib.h>   /* mersene twister RNG */
#include <stdlib.h>
#include <string.h>


#include "include/midi_out_port_data.h"


struct moport_manager
{
    evport_manager* portman;

    llist*  moplist;

    rtdata*     rt;

    lnode*  cur;

    jack_client_t*  client;

    int next_moport_id;

};


static void* moport_manager_rtdata_get_cb(const void* mopman);
static void  moport_manager_rtdata_free_cb(void* mopman);


static void moport_free_cb(void* data)
{
    moport_free(data);
}


moport_manager* moport_manager_new(void)
{
    moport_manager* mopman = malloc(sizeof(*mopman));

    if (!mopman)
        goto fail0;

    if (!(mopman->portman = evport_manager_new("midi_out")))
        goto fail1;

    mopman->moplist = llist_new(    sizeof(moport),
                                    moport_free_cb,
                                    0, /* no duplication */
                                    0, /* no copy */
                                    0, /* no compare */
                                    0  /* no string representation */
                               );
    if (!mopman->moplist)
        goto fail2;

    mopman->rt = rtdata_new(mopman, moport_manager_rtdata_get_cb,
                                    moport_manager_rtdata_free_cb);

    if (!mopman->rt)
        goto fail3;

    mopman->client = 0;
    mopman->cur = 0;
    mopman->next_moport_id = 1;

    return mopman;

fail3:  llist_free(mopman->moplist);
fail2:  evport_manager_free(mopman->portman);
fail1:  free(mopman);
fail0:  WARNING("out of memory for new moport manager\n");
    return 0;
}


void moport_manager_free(moport_manager* mopman)
{
    if (!mopman)
        return;

    rtdata_free(mopman->rt);
    evport_manager_free(mopman->portman);
    llist_free(mopman->moplist);
    free(mopman);
}


void moport_manager_jack_client_set(moport_manager* mopman,
                                    jack_client_t* client  )
{
    if (mopman->client)
        return;

    mopman->client = client;
}


moport* moport_manager_moport_new(moport_manager* mopman)
{
    moport* mop = moport_new( mopman->client, mopman->portman );

    if (!mop)
        return 0;

    if (!llist_add_data(mopman->moplist, mop))
    {
        moport_free(mop);
        return 0;
    }

    return mop;
}


moport* moport_manager_moport_first(moport_manager* mopman)
{
    return lnode_data(mopman->cur = llist_head(mopman->moplist));
}


moport* moport_manager_moport_next(moport_manager* mopman)
{
    if (!mopman->cur)
        return 0;

    return lnode_data(mopman->cur = lnode_next(mopman->cur));
}


static void* moport_manager_rtdata_get_cb(const void* data)
{
    const moport_manager* mopman = data;
    return llist_to_pointer_array(mopman->moplist);
}


static void  moport_manager_rtdata_free_cb(void* data)
{
    free(data); /* we're freeing the pointer array not the ports! */
}


void moport_manager_update_rt_data(const moport_manager* mopman)
{
    rtdata_update(mopman->rt);
}


void moport_manager_rt_init_jack_cycle( moport_manager* mopman,
                                        jack_nframes_t nframes )
{
    moport** mops = rtdata_data(mopman->rt);

    if (!mops)
        return;

    while(*mops)
        moport_rt_init_jack_cycle(*mops++, nframes);
}

void moport_manager_rt_play_old( moport_manager* mopman,
                                 bbt_t ph,  bbt_t nph,
                                 grid* gr)
{
    moport** mops = rtdata_data(mopman->rt);

    if (!mops)
        return;

    while(*mops)
        moport_rt_play_old(*mops++, ph, nph, gr);
}


void    moport_manager_rt_play_new_and_output(  moport_manager* mopman,
                                                bbt_t ph,
                                                bbt_t nph,
                                                jack_nframes_t nframes,
                                                double frames_per_tick )
{
    moport** mops = rtdata_data(mopman->rt);

    if (!mops)
        return;

    while(*mops)
    {
        moport_rt_play_new(*mops, ph, nph);
        moport_rt_output_jack_midi(*mops, nframes, frames_per_tick);
        ++mops;
    }
}


void moport_manager_rt_empty(   moport_manager* mopman,
                                grid* gr,
                                jack_nframes_t nframes)
{
    moport** mops = rtdata_data(mopman->rt);

    if (!mops)
        return;

    while(*mops)
        moport_rt_empty(*mops++, gr, nframes);
}

