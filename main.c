#include "boxy_sequencer.h"
#include "debug.h"
#include "gui_main.h"
#include "jack_midi.h"

#include <stdlib.h>


#include "event_buffer.h"



//#define NO_REAL_TIME



int main(int argc, char** argv)
{
    boxyseq* bs;
    jmidi*  jm;
    evbuf*  evb;

    int patslot;
    int evslot;
    int grboundslot;

    _Bool err = -1;

    if (!(bs = boxyseq_new(argc, argv)))
        exit(err);

#ifndef NO_REAL_TIME
    if (!(jm = jmidi_new()))
        goto quit;

    if (!jmidi_startup(jm, bs))
        goto quit;
#endif

    pattern* pat1;
    pattern* pat2;

    plist* pl;
    event* ev;

    int count, steps, i, n;

    bbt_t st, dur, t;


    patslot = boxyseq_pattern_new(bs, 4, 4);
    pat1 = boxyseq_pattern(bs, patslot);
    pat1->pd->width_min = 1;
    pat1->pd->width_max = 16;
    pat1->pd->height_min = 1;
    pat1->pd->height_max = 16;
    pl = pat1->pl;
    count = 16;
    steps = 16;
    st = pat1->pd->loop_length / steps;
    dur = st / 2.2; // - pat1->pd->loop_length / (steps * 8);
    t = 0;
    for (i = 0; i < count; ++i, t += st)
    {
        n = 1 + rand() % 8;
        while(n--)
        {
            ev = lnode_data(plist_add_event_new(pl, t));
            ev->note_dur = dur - 1;
            ev->box_release = dur - 1;
        }
    }

    patslot = boxyseq_pattern_new(bs, 4, 4);
    pat2 = boxyseq_pattern(bs, patslot);
    pat2->pd->width_min = 1;
    pat2->pd->width_max = 16;
    pat2->pd->height_min = 1;
    pat2->pd->height_max = 16;
    pl = pat2->pl;
    count = 32;
    steps = 32;
    st = pat2->pd->loop_length / steps;
    dur = st / 2.2; // - pat2->pd->loop_length / (steps * 8);
    t = 0;
    for (i = 0; i < count; ++i, t += st)
    {
        n = 1 + rand() % 8;
        while(n--)
        {
            ev = lnode_data(plist_add_event_new(pl, t));
            ev->note_dur = dur - 1;
            ev->box_release = dur - 1;
        }
    }


/*
        event* ev = lnode_data(plist_add_event_new(pl, t));
        ev->note_dur = dur * 34;
        ev->box_release = dur * 28;
        ev = lnode_data(plist_add_event_new(pl, t+ st/4));
        ev->note_dur = dur * 35;
        ev->box_release = dur * 27;
*/

    grbound* grb1;
    grbound* grb2;
    grbound* grb3;
    grbound* grb4;

    grboundslot = boxyseq_grbound_new(bs, 0, 32, 40,32);
    grb1 = boxyseq_grbound(bs, grboundslot);

    grboundslot = boxyseq_grbound_new(bs, 32, 32, 40, 32);
    grb2 = boxyseq_grbound(bs, grboundslot);

    grboundslot = boxyseq_grbound_new(bs, 64, 32, 40, 32);
    grb3 = boxyseq_grbound(bs, grboundslot);

    grboundslot = boxyseq_grbound_new(bs, 88, 32, 40, 32);
    grb4 = boxyseq_grbound(bs, grboundslot);

    grbound_flags_unset(grb2, FSPLACE_LEFT_TO_RIGHT);
    grbound_flags_unset(grb4, FSPLACE_LEFT_TO_RIGHT);
    grbound_flags_unset(grb3, FSPLACE_TOP_TO_BOTTOM);


    evport_manager* pat_ports = boxyseq_pattern_ports(bs);

    evport* port1;
    evport* port2;

    port1 = evport_manager_evport_new(pat_ports, RT_EVLIST_SORT_POS);
    port2 = evport_manager_evport_new(pat_ports, RT_EVLIST_SORT_POS);

    pattern_set_output_port(pat1, port1);
    pattern_set_output_port(pat2, port2);

    grbound_set_input_port(grb1, port1);
    grbound_set_input_port(grb2, port1);
    grbound_set_input_port(grb3, port1);
    grbound_set_input_port(grb4, port2);

    int moslot = boxyseq_moport_new(bs);
    moport* mo = boxyseq_moport(bs, moslot);

    grbound_midi_out_port_set(grb1, mo);
    grbound_midi_out_port_set(grb2, mo);
    grbound_midi_out_port_set(grb3, mo);
    grbound_midi_out_port_set(grb4, mo);

    pattern_prtdata_update(pat1);
    pattern_prtdata_update(pat2);



/*******************************************
 *******************************************
 *******************************************
 *******************************************
 *******************************************
 \/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/*/
#ifdef NO_REAL_TIME
st = 128;
t = 0;
for (i = 0; i <  pat1->pd->loop_length * 8; i += st)
{
    boxyseq_rt_play(bs, i,   i + st);
}
#endif
/*/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
 *******************************************
 *******************************************
 *******************************************
 *******************************************
 *******************************************/




printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");

#ifndef NO_REAL_TIME
    if (!gui_init(&argc, &argv, bs, jm))
        goto quit;

    jmidi_shutdown(jm);
#endif

    printf("sizeof(event):%d\n",sizeof(event));





    err = 0;

quit:

    boxyseq_free(bs);

    if (err)
        exit(err);

    exit(0);
}

