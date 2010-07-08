#include "boxy_sequencer.h"

#include "debug.h"
#include "event_port.h"
#include "jack_midi.h"
#include "midi_out_port.h"
#include "pattern.h"


#include <stdlib.h>
#include <string.h>


struct boxy_sequencer
{
    char* basename;

    pattern*    pattern_slot[MAX_PATTERN_SLOTS];
    grbound*    grbound_slot[MAX_GRBOUND_SLOTS];
    moport*     moport_slot[MAX_MOPORT_SLOTS];

    evport_manager* ports_pattern;
    evport_manager* ports_midi_out;

    evport_manager* ports_grid;

    grid*       gr;

    evport*     gui_place_port;
    evport*     gui_remove_port;

    jack_client_t*  client;
    jtransp*        jacktransport;
};


boxyseq* boxyseq_new(int argc, char** argv)
{
    boxyseq* bs = malloc(sizeof(*bs));

    if (!bs)
        goto fail;

    bs->basename = 0;
    char* tmp = strrchr( argv[0], '/' );
    if (!(bs->basename = strdup( tmp ? tmp + 1: argv[0])))
        goto fail1;

    int i;

    for (i = 0; i < MAX_PATTERN_SLOTS; ++i)
        bs->pattern_slot[i] = 0;

    for (i = 0; i < MAX_GRBOUND_SLOTS; ++i)
        bs->grbound_slot[i] = 0;

    for (i = 0; i < MAX_MOPORT_SLOTS; ++i)
        bs->moport_slot[i] = 0;

    if (!(bs->ports_pattern = evport_manager_new("pattern")))
        goto fail2;

    if (!(bs->ports_grid = evport_manager_new("grid")))
        goto fail3;

    if (!(bs->ports_midi_out = evport_manager_new("midi_out")))
        goto fail4;

    if (!(bs->gr = grid_new()))
        goto fail5;

    bs->gui_place_port = evport_manager_evport_new(bs->ports_grid, RT_EVLIST_SORT_POS);
    bs->gui_remove_port = evport_manager_evport_new(bs->ports_grid, RT_EVLIST_SORT_REL);

    if (!bs->gui_place_port || !bs->gui_remove_port)
        goto fail6;

    grid_set_gui_place_port(bs->gr, bs->gui_place_port);
    grid_set_gui_remove_port(bs->gr, bs->gui_remove_port);

    bs->jacktransport = 0;

    return bs;

fail6:
    grid_free(bs->gr);
fail5:
    evport_manager_free(bs->ports_midi_out);
fail4:
    evport_manager_free(bs->ports_grid);
fail3:
    evport_manager_free(bs->ports_pattern);
fail2:
    free(bs->basename);
fail1:
    free(bs);
fail:
    WARNING("out of memory allocating boxyseq data\n");
    return 0;
}


void boxyseq_free(boxyseq* bs)
{
    int i;

    if (!bs)
        return;

    grid_free(bs->gr);

    evport_manager_free(bs->ports_midi_out);
    evport_manager_free(bs->ports_grid);
    evport_manager_free(bs->ports_pattern);

    for (i = 0; i < MAX_GRBOUND_SLOTS; ++i)
        grbound_free(bs->grbound_slot[i]);

    for (i = 0; i < MAX_PATTERN_SLOTS; ++i)
        pattern_free(bs->pattern_slot[i]);

    for (i = 0; i < MAX_MOPORT_SLOTS; ++i)
        moport_free(bs->moport_slot[i]);

    free(bs->basename);
    free(bs);
}


const char* boxyseq_basename(const boxyseq* bs)
{
    return bs->basename;
}


void boxyseq_set_jack_client(boxyseq* bs, jack_client_t* client)
{
    bs->client = client;
}


void boxyseq_set_jtransp(boxyseq* bs, jtransp* jacktransport)
{
    bs->jacktransport = jacktransport;
}


evport_manager* boxyseq_pattern_ports(boxyseq* bs)
{
    return bs->ports_pattern;
}



int boxyseq_pattern_new(boxyseq* bs,
                        bbt_t beats_per_bar,
                        bbt_t beat_type     )
{
    int slot = 0;

    for (slot = 0; slot < MAX_PATTERN_SLOTS; ++slot)
        if (!bs->pattern_slot[slot])
            break;

    if (slot == MAX_PATTERN_SLOTS)
    {
        WARNING("out of pattern slots\n");
        return -1;
    }

    pattern* pat;

    if (!(bs->pattern_slot[slot] = pat = pattern_new()))
        return -1;

    pdata_set_meter(pat->pd, beats_per_bar, beat_type);
    pdata_set_loop_length(  pat->pd,
                            pdata_duration_bbt_to_ticks(pat->pd,
                                                        1, 0, 0  ));

    return slot;
}


void boxyseq_pattern_free(boxyseq* bs, int slot)
{
    if (slot < 0 || slot >= MAX_PATTERN_SLOTS)
    {
        WARNING("slot value %d is out of range\n", slot);
        return;
    }

    pattern_free(bs->pattern_slot[slot]);
    bs->pattern_slot[slot] = 0;
}


pattern* boxyseq_pattern_dup(boxyseq* bs, int dest_slot, int src_slot)
{
    if (dest_slot == src_slot)
        return bs->pattern_slot[src_slot];

    if (!bs->pattern_slot[src_slot])
        return 0;

    pattern_free(bs->pattern_slot[dest_slot]);
    bs->pattern_slot[dest_slot] = pattern_dup(bs->pattern_slot[src_slot]);

    return bs->pattern_slot[dest_slot];
}


pattern* boxyseq_pattern(boxyseq* bs, int slot)
{
    if (slot < 0 || slot >= MAX_PATTERN_SLOTS)
    {
        WARNING("out of range pattern slot: %d\n", slot);
        return 0;
    }

    return bs->pattern_slot[slot];
}


int boxyseq_grbound_new(boxyseq* bs, int x, int y, int w, int h)
{
    int slot = 0;
    for (slot = 0; slot < MAX_GRBOUND_SLOTS; ++slot)
        if (!bs->grbound_slot[slot])
            break;

    if (slot == MAX_GRBOUND_SLOTS)
    {
        WARNING("out of grbound slots\n");
        return -1;
    }

    grbound* grb;

    if (!(grb = grbound_new()))
        return -1;

    fsbound* fsb = grbound_fsbound(grb);

    if (!fsbound_set_coords(fsb, x, y, w, h))
    {
        WARNING("boundary x:%d y:%d w:%d h:%d out of bounds\n",
                x, y, w, h);
        WARNING("default size will be used\n");
    }

    bs->grbound_slot[slot] = grb;

    return slot;
}


void boxyseq_grbound_free(boxyseq* bs, int slot)
{
    grbound_free(bs->grbound_slot[slot]);
    bs->grbound_slot[slot] = 0;
}


grbound* boxyseq_grbound(boxyseq* bs, int slot)
{
    return bs->grbound_slot[slot];
}


int boxyseq_moport_new(boxyseq* bs)
{
    int slot = 0;

    for (slot = 0; slot < MAX_MOPORT_SLOTS; ++slot)
        if (!bs->moport_slot[slot])
            break;

    if (slot == MAX_MOPORT_SLOTS)
    {
        WARNING("out of midi out port slots\n");
        return -1;
    }

    bs->moport_slot[slot] = moport_new(bs->client, bs->ports_midi_out);

    if (!bs->moport_slot[slot])
        return -1;

    return slot;

}


void boxyseq_moport_free(boxyseq* bs, int slot)
{
    if (slot < 0 || slot >= MAX_MOPORT_SLOTS)
    {
        WARNING("slot value %d is out of range\n", slot);
        return;
    }

    moport_free(bs->moport_slot[slot]);
    bs->moport_slot[slot] = 0;
}


moport* boxyseq_moport(boxyseq* bs, int slot)
{
    return bs->moport_slot[slot];
}


evport* boxyseq_gui_place_port(const boxyseq* bs)
{
    return bs->gui_place_port;
}


evport* boxyseq_gui_remove_port(const boxyseq* bs)
{
    return bs->gui_remove_port;
}





void boxyseq_rt_play(boxyseq* bs,
                     jack_nframes_t nframes,
                     bbt_t ph, bbt_t nph)
{
    int i;

    evport* grid_port = grid_global_input_port(bs->gr);

    for (i = 0; i < MAX_MOPORT_SLOTS; ++i)
    {
        if (bs->moport_slot[i])
            moport_rt_play_old(bs->moport_slot[i], ph, nph, bs->gr);
    }

    grid_rt_unplace(bs->gr, ph, nph);

    evport_manager_all_evport_clear_data(bs->ports_pattern);

    for (i = 0; i < MAX_PATTERN_SLOTS; ++i)
    {
        if (bs->pattern_slot[i])
            prtdata_play(bs->pattern_slot[i]->prt, ph, nph);
    }

    for (i = 0; i < MAX_GRBOUND_SLOTS; ++i)
    {
        if (bs->grbound_slot[i])
            grbound_rt_sort(bs->grbound_slot[i], grid_port);
    }

    grid_rt_place(bs->gr, ph, nph);

    for (i = 0; i < MAX_MOPORT_SLOTS; ++i)
    {
        if (bs->moport_slot[i])
        {
            moport_rt_play_new(bs->moport_slot[i], ph, nph);

            moport_rt_output_jack_midi
                (
                    bs->moport_slot[i],
                    nframes,
                    #ifdef NO_REAL_TIME
                    0
                    #else
                    jtransp_rt_frames_per_tick(bs->jacktransport)
                    #endif
                );
        }
    }

    grid_rt_block(bs->gr, ph, nph);
}

void boxyseq_empty(boxyseq* bs)
{
    int i = 0;

    for (i = 0; i < MAX_MOPORT_SLOTS; ++i)
        if (bs->moport_slot[i])
            moport_empty(bs->moport_slot[i], bs->gr);

    grid_remove_events(bs->gr);

/*
    freespace_dump(grid_freespace(bs->gr));
*/
}

void boxyseq_rt_stop(boxyseq* bs)
{
    
}




grid* boxyseq_grid(boxyseq* bs)
{
    return bs->gr;
}
