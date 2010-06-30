#include "midi_out_port.h"


#include "debug.h"
#include "grid_boundary.h"

#include <jack/midiport.h>
#include <stdlib.h>


struct midi_out_port
{
    event start[16][128];
    event  play[16][128];

    evport* intersort;

    jack_port_t* jack_out_port;

};


moport* moport_new(jack_client_t* client, evport_manager* portman)
{
    moport* mo = malloc(sizeof(*mo));

    if (!mo)
        goto fail0;

    mo->intersort = evport_manager_evport_new(  portman,
                                                RT_EVLIST_SORT_POS  );

    if (!mo->intersort)
        goto fail1;

    #ifndef NO_REAL_TIME
    mo->jack_out_port = jack_port_register( client,
                                            evport_name(mo->intersort),
                                            JACK_DEFAULT_MIDI_TYPE,
                                            JackPortIsOutput,
                                            0);
    if (!mo->jack_out_port)
    {
        WARNING("failed to register jack port\n");
        goto fail1;
    }
    #else
    mo->jack_out_port = 0;
    #endif

    int c, p;

    for (c = 0; c < 16; ++c)
    {
        for (p = 0; p < 128; ++p)
            mo->start[c][p].flags = 0;

        for (p = 0; p < 128; ++p)
            mo->play[c][p].flags = 0;
    }

    return mo;

fail1:
    free(mo);
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


const char* moport_name(moport* mo)
{
    return evport_name(mo->intersort);
}


int moport_start_event(moport* midiport, const event* ev, int grb_flags)
{
    if (!ev->note_dur)
        return -1;

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

    start[pitch].flags &= ~EV_STATUSMASK;
    start[pitch].flags |= EV_STATUS_START;

    return pitch;
}


void moport_rt_play_old(moport* midiport, bbt_t ph, bbt_t nph, grid* gr)
{
    int channel, pitch;
    event* play;
    event* out;

    for (channel = 0; channel < 16; ++channel)
    {
        play = midiport->play[channel];

        for (pitch = 0; pitch < 128; ++pitch)
        {
            if ((play[pitch].flags & EV_STATUSMASK) == EV_STATUS_PLAY)
            {
                if (play[pitch].note_dur >= ph
                 && play[pitch].note_dur < nph)
                {
                    out = evport_write_event(midiport->intersort,
                                                    &play[pitch] );
                    if (out)
                    {
                        out->pos = out->note_dur - ph;
                        out->flags &= ~EV_STATUSMASK;
                        out->flags |= EV_STATUS_OFF;
                    }

                    play[pitch].flags &= ~EV_STATUSMASK;
                    play[pitch].flags |= EV_STATUS_OFF;
                    out = grid_rt_unplace_event(gr, &play[pitch]);
                    play[pitch].flags = 0;
                }
            }
        }
    }
}


void moport_rt_play_new(moport* midiport, bbt_t ph, bbt_t nph)
{
    int channel, pitch;
    event* start;
    event* play;
    event* out;

    for (channel = 0; channel < 16; ++channel)
    {
        start = midiport->start[channel];
        play = midiport->play[channel];

        for (pitch = 0; pitch < 128; ++pitch)
        {
            if ((start[pitch].flags & EV_STATUSMASK) == EV_STATUS_START)
            {
                out = evport_write_event(midiport->intersort,
                                                &start[pitch] );
                if (out)
                {
                    out->pos -= ph;
                    out->flags &= ~EV_STATUSMASK;
                    out->flags |= EV_STATUS_PLAY;
                }

                event_copy(&play[pitch], &start[pitch]);
                play[pitch].flags &= ~EV_STATUSMASK;
                play[pitch].flags |= EV_STATUS_PLAY;
                start[pitch].flags = 0;
            }
        }
    }
}


void moport_rt_output_jack_midi(moport* midiport, jack_nframes_t nframes,
                                                  double frames_per_tick )
{
    event ev;
    unsigned char* buf;
    void* jport_buf = jack_port_get_buffer( midiport->jack_out_port,
                                            nframes );

    evport_read_reset(midiport->intersort);
    jack_midi_clear_buffer(jport_buf);

    while(evport_read_and_remove_event(midiport->intersort, &ev))
    {
        if ((ev.flags & EV_STATUSMASK) == EV_STATUS_PLAY)
        {
            buf = jack_midi_event_reserve(  jport_buf,
                    (jack_nframes_t)((double)ev.pos * frames_per_tick),
                                            3 );
            if (buf)
            {
                buf[0] = 0x90 | event_channel(&ev);
                buf[1] = (unsigned char)ev.note_pitch;
                buf[2] = (unsigned char)ev.note_velocity;
            }
        }
        else if((ev.flags & EV_STATUSMASK) == EV_STATUS_OFF)
        {
            buf = jack_midi_event_reserve(jport_buf,
                (jack_nframes_t)((double)ev.pos * frames_per_tick),
                                            3 );
            if (buf)
            {
                buf[0] = 0x80 | event_channel(&ev);
                buf[1] = (unsigned char)ev.note_pitch;
                buf[2] = (unsigned char)ev.note_velocity;
            }
        }
    }
}
