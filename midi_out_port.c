#include "midi_out_port.h"


#include "debug.h"
#include "grid_boundary.h"

#include <stdlib.h>


struct midi_out_port
{
    event note_on[16][128];
};


moport* moport_new(void)
{
    moport* mo = malloc(sizeof(*mo));

    if (!mo)
        goto fail0;

    int c, p;

    for (c = 0; c < 16; ++c)
    {
        event* ev_on = mo->note_on[c];

        for (p = 0; p < 128; ++p)
            event_init(ev_on++);
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

    int p;
    event* ev_on = mo->note_on[0];

    for (p = 0; p < 128; ++p, ++ev_on)
    {
        if (ev_on->flags & EV_TYPEMASK)
        {
            MESSAGE("note %d: ", p);

            switch(ev_on->flags & EV_STATUSMASK)
            {
            case EV_STATUS_START:   printf("start");    break;
            case EV_STATUS_PLAY:    printf("play");     break;
            case EV_STATUS_STOP:    printf("stop");     break;
            case EV_STATUS_HOLD:    printf("hold");     break;
            default:                printf("err");      break;
            }
            printf("\n");
            if (ev_on->note_pitch != p)
                WARNING("mismatched pitch: note_pitch %d note_on[%d]\n",
                        p, ev_on->note_pitch);
        }
    }

    free(mo);
}


int moport_output(moport* midiport, const event* ev, int grb_flags)
{
    if (!ev->note_dur)
        return -1;

    event* note_on = midiport->note_on[event_channel(ev)];

    int pitch = -1;

    if (grb_flags & GRBOUND_PITCH_STRICT_POS)
    {
        if (note_on[pitch].flags & EV_TYPE_NOTE)
            return -1;

        pitch = ev->box_x;
    }
    else
    {
        int startpitch, endpitch, pitchdir;

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

        while(note_on[pitch].flags & EV_TYPE_NOTE)
        {
            pitch += pitchdir;
            if (pitch == endpitch)
                break;
        }

        if (pitch == endpitch)
            return -1;
    }

    event_copy(&note_on[pitch], ev);
    note_on[pitch].note_pitch = pitch;
    note_on[pitch].flags = EV_TYPE_NOTE | EV_STATUS_START;

    return pitch;
}

void moport_rt_play(moport* midiport, bbt_t ph, bbt_t nph, grid* gr)
{
    int channel;

    for (channel = 0; channel < 16; ++channel)
    {
        int pitch;
        event* ev = midiport->note_on[channel];

        for (pitch = 0; pitch < 128; ++pitch)
        {
            switch (ev[pitch].flags & EV_STATUSMASK)
            {
            case EV_STATUS_START:
                /* FIXME: output MIDI NOTE ON msg */
                ev[pitch].flags = EV_TYPE_NOTE | EV_STATUS_PLAY;
                ev[pitch].note_dur += ph;
                /* do not break */

            case EV_STATUS_PLAY:
                if (nph >= ev[pitch].note_dur)
                {
                    ev[pitch].flags = EV_TYPE_NOTE | EV_STATUS_STOP;
                    /* FIXME: output MIDI NOTE OFF msg */
                }
                else
                    break;

            case EV_STATUS_STOP:
                ev[pitch].flags = EV_TYPE_NOTE | EV_STATUS_HOLD;
                ev[pitch].box_release += ph;
                grid_rt_unplace_event(gr, &ev[pitch]);
                ev[pitch].flags = 0;
                /* to break or not to break? */

            default:
                /* this is a bit of a non event really... */
                break;
            }
        }
    }
}

