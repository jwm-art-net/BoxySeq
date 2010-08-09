#include "boxy_sequencer.h"
#include "debug.h"
#include "event_buffer.h"
#include "gui_main.h"
#include "jack_process.h"
#include "musical_scale.h"

#include <stdlib.h>





int main(int argc, char** argv)
{
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

    evlist* el;
    event* ev;

    int count, steps, i, n;

    bbt_t st, dur, rel, t;


    patslot = boxyseq_pattern_new(bs, 4, 4);
    pat1 = boxyseq_pattern(bs, patslot);

    pattern_set_loop_length(pat1, internal_ppqn * 4);

    pattern_set_event_width_range(pat1, 8, 12);
    pattern_set_event_height_range(pat1, 8, 12);

    el = pattern_event_list(pat1);

    count = steps = 16;
    st = (internal_ppqn * 4) / steps;
    dur = st * 4;
    rel = st * 4;
    t = 0;

    for (i = 0; i < count; ++i, t += st)
    {
        ev = lnode_data(evlist_add_event_new(el, t));
        ev->note_dur = st + rand() % dur;
        ev->box_release = st + rand() % rel;

        if (!((i + 1) % 4))
            EVENT_SET_TYPE_BLOCK( ev );
        else
        {
            ev = lnode_data(evlist_add_event_new(el,
                                (i % 5 == 0) ? t + st / 2 : t));
            ev->note_dur = st + rand() % dur;
            ev->box_release = st + rand() % rel;
        }


    }

    grbound* grb1;

    grboundslot = boxyseq_grbound_new(bs, 50, 50, 32, 32);
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

    scale* sc = sclist_scale_by_name(scales, "Harmonic Minor");

    grbound_scale_binary_set(grb1, scale_as_int(sc));
    grbound_scale_key_set(grb1, note_number("A#"));


    pattern_update_rt_data(pat1);

/*******************************************
 *******************************************
 *******************************************
 *******************************************
 *******************************************
 \/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/*/

#ifdef NO_REAL_TIME
st = 128;
t = 0;
_Bool repositioned = 1;

bbt_t looplen = pattern_loop_length(pat1);

for (i = 0; i <  looplen * 8; i += st)
{
    if ((i % looplen ) < st)
        MESSAGE("-----------ph:%d nph:%d looplen:%d\n",
                i, i + st, looplen );

    boxyseq_rt_play(bs, st, repositioned, i,   i + st);
    repositioned = 0;
}
#endif

/*/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
 *******************************************
 *******************************************
 *******************************************
 *******************************************
 *******************************************/


printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");

    if (!gui_init(&argc, &argv, bs))
        goto quit;

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
