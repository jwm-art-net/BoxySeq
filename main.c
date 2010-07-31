#include "boxy_sequencer.h"
#include "debug.h"
#include "event_buffer.h"
#include "gui_main.h"
#include "jack_process.h"
#include "musical_scale.h"

#include <stdlib.h>





int main(int argc, char** argv)
{
/*
    rt_evlist* rtl = rt_evlist_new(0, RT_EVLIST_SORT_POS);

    int i;

    event ev;
    event* ev_p;

    for (i = 0; i < 15; ++i)
    {
        ev.pos = rand() % 16;
        ev_p = rt_evlist_event_add(rtl, &ev);
        if (!ev_p)
            WARNING("poo pah\n");
        ev_p = rt_evlist_event_add(rtl, &ev);
        if (!ev_p)
            WARNING("poo pah\n");
    }

    rt_evlist_read_reset(rtl);

    while(rt_evlist_read_and_remove_event(rtl, &ev))
        event_dump(&ev);

    rt_evlist_free(rtl);

*/
    boxyseq*    bs;
    jackdata*   jd;
    evbuf*      evb;

    int patslot;
    int evslot;
    int grboundslot;

    _Bool err = -1;

    if (!(bs = boxyseq_new(argc, argv)))
        exit(err);

    if (!(jd = jackdata_new()))
        goto quit;

    if (!jackdata_startup(jd, bs))
        goto quit;

    pattern* pat1;
    pattern* pat2;

    plist* pl;
    event* ev;

    int count, steps, i, n;

    bbt_t st, dur, rel, t;


    patslot = boxyseq_pattern_new(bs, 4, 4);
    pat1 = boxyseq_pattern(bs, patslot);

    pdata_set_loop_length(pat1->pd, internal_ppqn * 4);

    pat1->pd->width_min = 4;
    pat1->pd->width_max = 12;
    pat1->pd->height_min = 4;
    pat1->pd->height_max = 8;

    pl = pat1->pl;

    count = steps = 12;
    st = (internal_ppqn * 4) / steps;
    dur = st * 2;
    rel = st * 2;
    t = 0;

/*  DO NOT test infinite durations by adding an event with infinite
    duration (ie ev.note_dur = -1) to pattern - the pattern is not
    designed (nor will be altered) to handle them in this way.
*/

    for (i = 0; i < count; ++i, t += st)
    {
        ev = lnode_data(plist_add_event_new(pl, t));
        ev->note_dur = dur;
        ev->box_release = rel;

        if (!((i + 1) % 4))
            EVENT_SET_TYPE_BLOCK( ev );
/*
        if ((i % 5) == 0)
        {
            ev = lnode_data(plist_add_event_new(pl, t));
            ev->note_dur = dur;
            ev->box_release = rel;
            ev = lnode_data(plist_add_event_new(pl, t));
            ev->note_dur = dur;
            ev->box_release = rel;
        }
*/
    }

    grbound* grb1;


    grboundslot = boxyseq_grbound_new(bs, 32, 60, 32, 66);
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

    scale* sc = sclist_scale_by_name(scales, "Major");

    grbound_scale_binary_set(grb1, scale_as_int(sc));
    grbound_scale_key_set(grb1, note_number("F"));


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
    if (!gui_init(&argc, &argv, bs, jd))
        goto quit;
#endif

    boxyseq_shutdown(bs);

    jackdata_shutdown(jd);

    printf("sizeof(event):%d\n",sizeof(event));

    jackdata_free(jd);

quit:

    boxyseq_free(bs);

    if (err)
        exit(err);

    exit(0);

}
