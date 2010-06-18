#include "midi_out_port.h"


#include "debug.h"
#include "grid_boundary.h"

#include <stdlib.h>


struct midi_out_port
{
    evport* evinput;

    event note_on[16][128];
    event note_off[16][128];
};


moport* moport_new(void)
{
    moport* mo = malloc(sizeof(*mo));

    if (!mo)
        goto fail0;

    mo->evinput = 0;

    int c, p;

    for (c = 0; c < 16; ++c)
    {
        event* ev_on = mo->note_on[c];
        event* ev_off = mo->note_off[c];

        for (p = 0; p < 128; ++p)
        {
            event_init(ev_on++);
            event_init(ev_off++);
        }
    }

    return mo;

fail0:
    WARNING("out of memory for new midi out port\n");
    return 0;
}


void moport_free(moport* mo)
{
    if (!mo)
        return;

    free(mo);
}


int moport_test_pitch(moport* midiport, event* ev, int grb_flags)
{
    event* note_on = midiport->note_on[event_channel(ev)];

    if (grb_flags & GRBOUND_PITCH_STRICT_POS)
    {
        if (note_on[ev->box_x].flags & EV_NOTE_ON)
        {
            if (grb_flags & GRBOUND_BLOCK_ON_NOTE_FAIL)
                return -2; /* ev->flags &= ~EV_TYPE_NOTE; */
            else
                return -1;
        }

        return ev->box_x;
    }
    else
    {
        int pitch, startpitch, endpitch, pitchdir;

        if (grb_flags & FSPLACE_LEFT_TO_RIGHT)
        {
            startpitch = pitch = ev->box_x;
            endpitch = pitch + ev->box_width;
            pitchdir = 1;
        }
        else
        {
            startpitch = pitch = ev->box_x + ev->box_width;
            endpitch = ev->box_x;
            pitchdir = -1;
        }

        while(note_on[pitch].flags & EV_NOTE_ON)
        {
            pitch += pitchdir;
            if (pitch == endpitch)
                break;
        }

        if (pitch < endpitch)
            return pitch;

        if (grb_flags & GRBOUND_BLOCK_ON_NOTE_FAIL)
            return -2; /* ev.flags &= ~EV_TYPE_NOTE; */
    }

    return -1;
}

