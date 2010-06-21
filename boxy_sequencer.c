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
    evport_manager* ports_bound;

    grid*       gr;
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

    if (!(bs->ports_pattern = evport_manager_new("pattern")))
        goto fail2;

    if (!(bs->ports_bound = evport_manager_new("boundary")))
        goto fail3;

    if (!(bs->gr = grid_new()))
        goto fail4;

    return bs;

fail4:
    evport_manager_free(bs->ports_bound);
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

    evport_manager_free(bs->ports_bound);
    evport_manager_free(bs->ports_pattern);

    for (i = 0; i < MAX_GRBOUND_SLOTS; ++i)
        grbound_free(bs->grbound_slot[i]);

    for (i = 0; i < MAX_PATTERN_SLOTS; ++i)
        pattern_free(bs->pattern_slot[i]);

    free(bs->basename);
    free(bs);
}


const char* boxyseq_basename(const boxyseq* bs)
{
    return bs->basename;
}


evport_manager* boxyseq_pattern_ports(boxyseq* bs)
{
    return bs->ports_pattern;
}


evport_manager* boxyseq_bound_ports(boxyseq* bs)
{
    return bs->ports_bound;
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
        WARNING("boundary x:%d y:%d w:%d h:%d out of bounds\n", x,y,w,h);
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

    if (!(bs->moport_slot[slot] = moport_new()))
        return -1;

    moport_set_grid_unplace_port(   bs->moport_slot[slot],
                                    grid_unplace_port(bs->gr)   );

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





void boxyseq_rt_play(boxyseq* bs, bbt_t ph, bbt_t nph)
{
    int i;

    evport* grid_port = grid_input_port(bs->gr);

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

    grid_rt_place(bs->gr);

    for (i = 0; i < MAX_MOPORT_SLOTS; ++i)
    {
        if (bs->moport_slot[i])
            moport_rt_play(bs->moport_slot[i], ph, nph);
    }

    grid_rt_unplace(bs->gr, ph, nph);

}


grid* boxyseq_grid(boxyseq* bs)
{
    return bs->gr;
}
