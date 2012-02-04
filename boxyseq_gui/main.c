#include "boxy_sequencer.h"
#include "debug.h"
#include "event_buffer.h"
#include "gui_main.h"
#include "jack_process.h"
#include "musical_scale.h"


#include <stdlib.h>
#include <unistd.h>

pattern*    new_pat(pattern_manager* patman,
                    int steps,
                    float dur_r,    float rel_r,
                    int wmin,       int wmax,
                    int hmin,       int hmax)
{
    pattern* p = pattern_manager_pattern_new(patman);
    evlist* el = pattern_event_list(p);
    event*  ev = 0;

    int count = steps;
    int i;

    bbt_t   st =    (internal_ppqn * 4) / steps;
    bbt_t   dur =   st * dur_r - 1;
    bbt_t   rel =   st * rel_r - 1;
    bbt_t   t = 0;

    DMESSAGE("dur:%d rel:%d\n",dur,rel);

    pattern_set_meter(p, 4, 4);
    pattern_set_loop_length(p, internal_ppqn * 4);
    pattern_set_event_width_range(p,    wmin, wmax);
    pattern_set_event_height_range(p,   hmin, hmax);

    for (i = 0; i < count; ++i, t += st)
    {
        DMESSAGE("adding note pos:%d dur:%d rel:%d\n", t, dur, rel);
        ev = lnode_data(evlist_add_event_new(el, t));
        ev->note_dur = dur;
        ev->box_release = rel;
    }

    return p;
}


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

    pattern*    pat1 = 0;
    pattern*    pat2 = 0;
    grbound*    grb;
    moport*     mop1;
    evport*     patport1;
    evport*     patport2;

    int i;

    double r, g, b;

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
    patport1 = evport_manager_evport_new(patportman, "patport1",
                                                    RT_EVLIST_SORT_POS);

    pat1 = new_pat(patman, 4, 2.25, 0.25, 15,16,15,16);
    pattern_set_output_port(pat1, patport1);
    pattern_update_rt_data(pat1);

    pat2 = new_pat(patman, 16, 0.25, 0.25, 4, 5, 4, 5);
    pattern_set_output_port(pat2, patport1);
    pattern_update_rt_data(pat2);

    mop1 = moport_manager_moport_new(mopman);

    scales = sclist_new();

    sclist_add_default_scales(scales);

    sc = sclist_scale_by_name(scales, "Dorian");

    int scint = scale_as_int(sc);
    int keyint = note_number("F#");


    srand(time(0));

    for (i = 0; i < 1; ++i)
    {
        int x, y, w, h;

        x = rand() % 70 + 50;
        y = rand() % 70 + 50;
        w = rand() % 10 + 35;
        h = rand() % 10 + 35;

        if (x + w > 127)
            w = 127 - x;

        if (y + h > 127)
            h = 127 - y;

        grb = grbound_manager_grbound_new(grbman);
        grbound_fsbound_set(grb, x, y, w, h);
        grbound_set_input_port(grb, patport1);

        grbound_midi_out_port_set(grb, mop1);

        grbound_scale_binary_set(grb, scint);
        grbound_scale_key_set(grb, keyint);

        grbound_update_rt_data(grb);
    }



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
