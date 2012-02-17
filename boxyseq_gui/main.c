#include "boxy_sequencer.h"
#include "debug.h"
#include "gui_main.h"
#include "jack_process.h"
#include "musical_scale.h"


#include <stdlib.h>
#include <unistd.h>

pattern*    new_pat(pattern_manager* patman,
                    int ch,
                    int steps,
                    int count,
                    float offset_ratio,
                    int simul,
                    float dur_r,    float rel_r,
                    int wmin,       int wmax,
                    int hmin,       int hmax)
{
    pattern* p = pattern_manager_pattern_new(patman);
    evlist* el = pattern_event_list(p);

    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
    int i;

    bbt_t   st =    (internal_ppqn * 4) / steps;
    bbt_t   dur =   st * dur_r - 1;
    bbt_t   rel =   st * rel_r - 1;
    bbt_t   t = st * offset_ratio;

    random_rgb(&r, &g, &b);

    DMESSAGE("dur:%d rel:%d\n",dur,rel);

    pattern_set_meter(p, 4, 4);
    pattern_set_loop_length(p, internal_ppqn * 4);
    pattern_set_event_width_range(p,    wmin, wmax);
    pattern_set_event_height_range(p,   hmin, hmax);

    for (i = 0; i < count; ++i, t += st)
    {
        int j;

        DMESSAGE("adding %d note(s) at pos:%d dur:%d rel:%d\n",
                        simul, t, dur, rel);

        for (j = 0; j < simul; ++j)
        {
            event* ev = lnode_data(evlist_add_event_new(el, t));
            EVENT_SET_TYPE( ev, EV_TYPE_NOTE );
            EVENT_SET_CHANNEL( ev, ch );
            ev->note_dur = dur;
            ev->box_release = rel;
            ev->box.r = r;
            ev->box.g = g;
            ev->box.b = b;
            event_dump( ev );
        }
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

    pattern*    pat0 = 0;
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

/*    boxyseq_ui_place_static_block(bs, 32, 32, 64, 64);*/

    patman = boxyseq_pattern_manager(bs);
    grbman = boxyseq_grbound_manager(bs);
    mopman = boxyseq_moport_manager(bs);
    patportman = boxyseq_pattern_port_manager(bs);


/*
                    int evtype,
                    int ch,
                    int steps,
                    int count,
                    float offset_ratio,b
                    int simul,
                    float dur_r,    float rel_r,
                    int wmin,       int wmax,
                    int hmin,       int hmax)
*/


    patport1 = evport_manager_evport_new(patportman, "patport1",
                                                    RT_EVLIST_SORT_POS);

    pat0 = new_pat(patman, 2, 6, 6, 0.0,    1, 1.25, 12.05, 4,5,4,5);
    pattern_set_output_port(pat0, patport1);
    pattern_update_rt_data(pat0);

    pat1 = new_pat(patman, 1, 12, 12, 0.1,  1, 0.5, 11.4, 5,6,5,6);
    pattern_set_output_port(pat1, patport1);
    pattern_update_rt_data(pat1);

    pat2 = new_pat(patman, 0, 18, 18, 0.0,  1, 2.75, 10.85, 6,7,6,7);
    pattern_set_output_port(pat2, patport1);
    pattern_update_rt_data(pat2);

    mop1 = moport_manager_moport_new(mopman);

    scales = sclist_new();

    sclist_add_default_scales(scales);

    sc = sclist_scale_by_name(scales, "Major");

    int scint = scale_as_int(sc);
    int keyint = note_number("C#");


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

    jackdata_free(jd);

quit:

    boxyseq_free(bs);

    if (err)
        exit(err);

    exit(0);

}
