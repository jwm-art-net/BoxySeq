#include "boxy_sequencer.h"
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

    patslot = boxyseq_pattern_new(bs, 4, 4);

    pattern* pat = boxyseq_pattern(bs, patslot);

    plist* pl = pat->pl;

    pat->pd->width_min = 2;
    pat->pd->width_max = 32;

    pat->pd->height_min = 2;
    pat->pd->height_max = 32;

    int count = 16;
    int steps = 16;
    bbt_t st = pat->pd->loop_length / steps;
    bbt_t dur = st - pat->pd->loop_length / (steps * 2);
    bbt_t t = 0;
    int i;

    for (i = 0; i < count; ++i, t += st)
    {
        event* ev = lnode_data(plist_add_event_new(pl, t));
        ev->note_dur = dur * 24;
        ev->box_release = dur * 18;
        ev = lnode_data(plist_add_event_new(pl, t+ st/4));
        ev->note_dur = dur * 25;
        ev->box_release = dur * 17;
    }


    grboundslot = boxyseq_grbound_new(bs, 0, 0, 128, 128);
    grbound* grb1 = boxyseq_grbound(bs, grboundslot);

    evport_manager* pat_ports = boxyseq_pattern_ports(bs);

    evport* port = evport_manager_evport_new(pat_ports,
                                                RT_EVLIST_SORT_POS);

    pattern_set_output_port(pat, port);

    grbound_set_input_port(grb1, port);


    int moslot = boxyseq_moport_new(bs);
    moport* mo = boxyseq_moport(bs, moslot);

    grbound_midi_out_port_set(grb1, mo);

    pattern_prtdata_update(pat);


/*******************************************
 *******************************************
 *******************************************
 *******************************************
 *******************************************
 \/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/*/
#ifdef NO_REAL_TIME
st = 128;
t = 0;
for (i = 0; i <  pat->pd->loop_length * 2; i += st)
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

