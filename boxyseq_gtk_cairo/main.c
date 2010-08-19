#include "boxy_sequencer.h"
#include "debug.h"
#include "event_buffer.h"
#include "gui_main.h"
#include "jack_process.h"
#include "musical_scale.h"


#include <stdlib.h>
#include <unistd.h>


int main(int argc, char** argv)
{
    boxyseq*    bs;
    jackdata*   jd;

    pattern_manager*    patman;
    grbound_manager*    grbman;
    moport_manager*     mopman;
    evport_manager*     patportman;

    sclist* scales;
    scale*  sc;

    pattern*    pat1;
    grbound*    grb1;
    grbound*    grb2;
    moport*     mop1;
    evport*     patport1;

    evlist* el;
    event* ev;

    int count, steps, i;

    double r, g, b;

    bbt_t st, dur, rel, t;
    _Bool err = -1;

    if (!(bs = boxyseq_new(argc, argv)))
        exit(err);

    if (!(jd = jackdata_new()))
        goto quit;

    if (!jackdata_startup(jd, bs))
        goto quit;

    patman = boxyseq_pattern_manager(bs);
    grbman = boxyseq_grbound_manager(bs);
    mopman = boxyseq_moport_manager(bs);
    patportman = boxyseq_pattern_port_manager(bs);

    pat1 = pattern_manager_pattern_new(patman);
    pattern_set_meter(pat1, 4, 4);
    pattern_set_loop_length(pat1, internal_ppqn * 4);
    pattern_set_event_width_range(pat1, 2, 10);
    pattern_set_event_height_range(pat1, 2, 10);

    el = pattern_event_list(pat1);

    count = steps = 8;
    st = (internal_ppqn * 4) / steps;
    dur = st * 8;
    rel = st * 8;
    t = 0;

    for (i = 0; i < count; ++i, t += st)
    {
        ev = lnode_data(evlist_add_event_new(el, t));
        ev->note_dur = st + rand() % dur;
        ev->box_release = st + rand() % rel;

        if (!((i + 1) % 12))
            EVENT_SET_TYPE_BLOCK( ev );
        else
        {
            ev = lnode_data(evlist_add_event_new(el,
                                (i % 3 == 0) ? t + st / 2 : t));
            ev->note_dur = st + rand() % dur;
            ev->box_release = st + rand() % rel;
        }
    }

    grb1 = grbound_manager_grbound_new(grbman);
    grb2 = grbound_manager_grbound_new(grbman);
    grbound_fsbound_set(grb1, 40, 40, 24, 32);
    grbound_fsbound_set(grb2, 64, 10, 24, 32);

    patport1 = evport_manager_evport_new(patportman, RT_EVLIST_SORT_POS);

    pattern_set_output_port(pat1, patport1);
    grbound_set_input_port(grb1, patport1);
    grbound_set_input_port(grb2, patport1);

    mop1 = moport_manager_moport_new(mopman);
    grbound_midi_out_port_set(grb1, mop1);
    grbound_midi_out_port_set(grb2, mop1);

    scales = sclist_new();

    sclist_add_default_scales(scales);

    sc = sclist_scale_by_name(scales, "Major");

    grbound_scale_binary_set(grb1, scale_as_int(sc));
    grbound_scale_binary_set(grb2, scale_as_int(sc));
    grbound_scale_key_set(grb1, note_number("C"));
    grbound_scale_key_set(grb2, note_number("C"));

    pattern_update_rt_data(pat1);
    grbound_update_rt_data(grb1);
    grbound_update_rt_data(grb2);

    grbound_manager_update_rt_data(grbman);
    pattern_manager_update_rt_data(patman);
    moport_manager_update_rt_data(mopman);
    evport_manager_update_rt_data(patportman);


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

for (i = 0; i <  looplen * 1; i += st)
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
#ifndef NO_GUI
    if (!gui_init(&argc, &argv, bs))
        goto quit;
#endif

    sclist_free(scales);

    boxyseq_shutdown(bs);

    jackdata_shutdown(jd);

    printf("sizeof(event):%ld\n",sizeof(event));

    jackdata_free(jd);

quit:

    boxyseq_free(bs);

    if (err)
        exit(err);

    exit(0);

}
