#include "midi_out_port.h"


#include "debug.h"
#include "grid_boundary.h"

#include <stdlib.h>


struct midi_out_port
{
    event start[16][128];
    event  play[16][128];
};


moport* moport_new(void)
{
    moport* mo = malloc(sizeof(*mo));

    if (!mo)
        goto fail0;

    int c, p;

    for (c = 0; c < 16; ++c)
    {
        for (p = 0; p < 128; ++p)
            mo->start[c][p].flags = 0;

        for (p = 0; p < 128; ++p)
            mo->play[c][p].flags = 0;
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


int moport_output(moport* midiport, const event* ev, int grb_flags)
{
    if (!ev->note_dur)
        return -1;

/*
    event* note_on = midiport->note_on[event_channel(ev)];
*/

    event* start = midiport->start[event_channel(ev)];
    event*  play = midiport->play[event_channel(ev)];

    int pitch = -1;

    if (grb_flags & GRBOUND_PITCH_STRICT_POS)
    {
        if (start[pitch].flags & EV_TYPE_NOTE
         ||  play[pitch].flags & EV_TYPE_NOTE )
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

        while(start[pitch].flags & EV_TYPE_NOTE
            || play[pitch].flags & EV_TYPE_NOTE )
        {
            pitch += pitchdir;
            if (pitch == endpitch)
                break;
        }

        if (pitch == endpitch)
            return -1;
    }

    event_copy(&start[pitch], ev);
    start[pitch].note_pitch = pitch;
    start[pitch].flags = EV_TYPE_NOTE | EV_STATUS_START;

    return pitch;
}

void moport_rt_play_old(moport* midiport, bbt_t ph, bbt_t nph, grid* gr)
{
    int channel, pitch;
    event* play;

    for (channel = 0; channel < 16; ++channel)
    {
        play = midiport->play[channel];

        for (pitch = 0; pitch < 128; ++pitch)
        {
            switch (play[pitch].flags & EV_STATUSMASK)
            {
            case EV_STATUS_PLAY:

                if (nph >= play[pitch].note_dur)
                {
                    play[pitch].flags = EV_TYPE_NOTE | EV_STATUS_STOP;
                    /* FIXME: output MIDI NOTE OFF msg */
                }
                else break;

            case EV_STATUS_STOP:

                play[pitch].flags = EV_TYPE_NOTE | EV_STATUS_HOLD;
                play[pitch].box_release += ph;
                grid_rt_unplace_event(gr, &play[pitch]);
                play[pitch].flags = 0;
                /* to break or not to break? */

            default:
                /* this is a bit of a non event really... */
                break;
            }
        }
    }
}

void moport_rt_play_new(moport* midiport, bbt_t ph, bbt_t nph)
{
    int channel, pitch;
    event* start;
    event* play;

    for (channel = 0; channel < 16; ++channel)
    {
        start = midiport->start[channel];
        play = midiport->play[channel];

        for (pitch = 0; pitch < 128; ++pitch)
        {
            switch (start[pitch].flags & EV_STATUSMASK)
            {
            case EV_STATUS_START:

                /* FIXME: output MIDI NOTE ON msg */
                event_copy(&play[pitch], &start[pitch]);
                play[pitch].flags = EV_TYPE_NOTE | EV_STATUS_PLAY;
                play[pitch].note_dur += ph;
                start[pitch].flags = 0;

            }
        }
    }
}

