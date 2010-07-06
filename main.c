#include "boxy_sequencer.h"
#include "debug.h"
#include "event_buffer.h"
#include "gui_main.h"
#include "jack_midi.h"
#include "musical_scale.h"

#include <stdlib.h>





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

    if (!(jm = jmidi_new()))
        goto quit;

    if (!jmidi_startup(jm, bs))
        goto quit;

    pattern* pat1;
    pattern* pat2;

    plist* pl;
    event* ev;

    int count, steps, i, n;

    bbt_t st, dur, t;


    patslot = boxyseq_pattern_new(bs, 4, 4);
    pat1 = boxyseq_pattern(bs, patslot);

    pdata_set_loop_length(pat1->pd, ppqn * 4);

    pat1->pd->width_min = 4;
    pat1->pd->width_max = 8;
    pat1->pd->height_min = 8;
    pat1->pd->height_max = 24;

    pl = pat1->pl;

    count = 8;
    steps = 8;
    st = pat1->pd->loop_length / steps;
    dur = st / 1.25; // - pat1->pd->loop_length / (steps * 8);
    t = 0;

    for (i = 0; i < count; ++i, t += st)
    {
        ev = lnode_data(plist_add_event_new(pl, t));
        ev->note_dur = dur * 4;
        ev->box_release = dur * 6;

        ev = lnode_data(plist_add_event_new(pl, t));
        ev->note_dur = dur * 4;
        ev->box_release = dur * 6;

        ev = lnode_data(plist_add_event_new(pl, t));
        ev->note_dur = dur * 4;
        ev->box_release = dur * 6;

        if (i == 7)
        {
            ev = lnode_data(plist_add_event_new(pl, t + st / 2));
            ev->note_dur = dur * 4;
            ev->box_release = dur * 6;

            ev = lnode_data(plist_add_event_new(pl, t + st / 2));
            ev->note_dur = dur * 4;
            ev->box_release = dur * 6;

            ev = lnode_data(plist_add_event_new(pl, t + st / 2));
            ev->note_dur = dur * 4;
            ev->box_release = dur * 6;
        }

    }

    grbound* grb1;


    grboundslot = boxyseq_grbound_new(bs, 31, 60, 64, 64);
    grb1 = boxyseq_grbound(bs, grboundslot);


    evport_manager* pat_ports = boxyseq_pattern_ports(bs);

    evport* port1;


    port1 = evport_manager_evport_new(pat_ports, RT_EVLIST_SORT_POS);


    pattern_set_output_port(pat1, port1);


    grbound_set_input_port(grb1, port1);


    int moslot = boxyseq_moport_new(bs);
    moport* mo = boxyseq_moport(bs, moslot);

    grbound_midi_out_port_set(grb1, mo);



    sclist* scales = sclist_new();

    sclist_add_default_scales(scales);

    scale* sc = sclist_scale_by_name(scales, "Augmented");

    grbound_scale_binary_set(grb1, scale_as_int(sc));

    pattern_prtdata_update(pat1);

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
    if ((i % pat1->pd->loop_length) < st)
        MESSAGE("-----------ph:%d nph:%d looplen:%d\n",
                i, i + st, pat1->pd->loop_length);

    boxyseq_rt_play(bs, st, i,   i + st);
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
#endif

    jmidi_shutdown(jm);

    printf("sizeof(event):%d\n",sizeof(event));

    jmidi_free(jm);

    boxyseq_empty(bs);

    err = 0;

quit:

    boxyseq_free(bs);

    if (err)
        exit(err);

    exit(0);
}

