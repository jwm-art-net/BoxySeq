#include "boxy_sequencer.h"
#include "gui_main.h"
#include "jack_midi.h"

#include <stdlib.h>


#include "event_buffer.h"


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

    patslot = boxyseq_pattern_new(bs, 4, 4);

    pattern* pat = boxyseq_pattern(bs, patslot);

    plist* pl = pat->pl;

    int steps = 4;
    int st = pat->pd->loop_length / steps;
    int dur = st - pat->pd->loop_length / (steps * 2);
    int t = 0;
    int i;

    for (i = 0; i < steps; ++i, t+= st)
    {
        event* ev = lnode_data(plist_add_event_new(pl, t));
        ev->note_dur = dur;
        ev->box_release = dur;
    }


    grboundslot = boxyseq_grbound_new(bs, 0, 0, 128, 128);
    grbound* grb1 = boxyseq_grbound(bs, grboundslot);

    evport_manager* pat_ports = boxyseq_pattern_ports(bs);

    evport* port = evport_manager_evport_new(pat_ports);

    pattern_set_output_port(pat, port);

    grbound_set_input_port(grb1, port);

    pattern_prtdata_update(pat);


/*******************************************
 *******************************************
 *******************************************
 *******************************************
 *******************************************
 \/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/*/
/*
    boxyseq_rt_play(bs, 0,   128);
    boxyseq_rt_play(bs, 128, 256);
*/
/*/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
 *******************************************
 *******************************************
 *******************************************
 *******************************************
 *******************************************/




printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");

    if (!gui_init(&argc, &argv, bs, jm))
        goto quit;

    jmidi_shutdown(jm);

    printf("sizeof(event):%d\n",sizeof(event));





    err = 0;

quit:

    boxyseq_free(bs);

    if (err)
        exit(err);

    exit(0);
}

