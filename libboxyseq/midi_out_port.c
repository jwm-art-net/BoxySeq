#include "midi_out_port.h"


#include "debug.h"
#include "freespace_state.h"
#include "grid_boundary.h"
#include "musical_scale.h"
#include "box_grid.h"


#include <jack/midiport.h>
#include <stdlib.h>
#include <string.h>


#include "include/midi_out_port_data.h"


moport* moport_new(jack_client_t* client,   int port_id,
                                            evport_manager* portman)
{
    moport* mo = malloc(sizeof(*mo));

    if (!mo)
        goto fail0;

    char tmp[80];
    snprintf(tmp, 79, "port-%2d", port_id);
    mo->name = strdup(tmp);

    #ifndef NO_REAL_TIME
    mo->jack_out_port = jack_port_register( client,
                                            mo->name,
                                            JACK_DEFAULT_MIDI_TYPE,
                                            JackPortIsOutput,
                                            0);
    if (!mo->jack_out_port)
    {
        WARNING("failed to register jack port\n");
        goto fail2;
    }
    #else
    mo->jack_out_port = 0;
    #endif

    mo->jport_buf = 0;

    int c, p;

    for (c = 0; c < 16; ++c)
    {
        for (p = 0; p < 128; ++p)
             mo->play[c][p].flags = 0;
    }

    return mo;

fail2:  free(mo->name);
fail1:  free(mo);
fail0:  WARNING("out of memory for new midi out port\n");
    return 0;
}


void moport_free(moport* mo)
{
    if (!mo)
        return;

    free(mo->name);
    free(mo);
}


int moport_rt_push_event_pitch(moport* midiport,
                                    const event* ev,
                                    int grb_flags,
                                    int scale_bin,
                                    int scale_key )
{
    if (ev->note_dur == ev->pos)
        return -1;

    #ifndef NDEBUG
    EVENT_IS(ev, EV_STATUS_ON | EV_TYPE_NOTE);
    #endif

    event*  play =  midiport->play[EVENT_GET_CHANNEL(ev)];

    int pitch = ev->box.x;

    if (grb_flags & GRBOUND_PITCH_STRICT_POS)
    {
        if (!scale_note_is_valid(scale_bin, scale_key, pitch))
            return -1;

        if (play[pitch].flags)
            return -1;
    }
    else
    {
        int startpitch, endpitch;
        int pitchdir = 1;

        startpitch = endpitch = pitch;

        if (grb_flags & FSPLACE_LEFT_TO_RIGHT)
            endpitch += ev->box.w;
        else
        {
            pitch = startpitch += ev->box.w;
            pitchdir = -1;
        }

        while(pitch != endpitch)
        {
            if (scale_note_is_valid(scale_bin, scale_key, pitch)
             && !play[pitch].flags)
            {
                goto output;
            }

            pitch += pitchdir;
        }

        return -1;
    }

output:

    event_copy(&play[pitch], ev);
    play[pitch].note_pitch = pitch;
    EVENT_SET_STATUS_ON( &play[pitch] );

    return pitch;
}


void moport_rt_pull_ending(moport* midiport, bbt_t ph, bbt_t nph,
                                             evport* grid_intersort)
{
    int channel, pitch;
    event* play;

    for (channel = 0; channel < 16; ++channel)
    {
        play = midiport->play[channel];

        for (pitch = 0; pitch < 128; ++pitch)
        {
            if (!play[pitch].flags)
                continue;

            if (play[pitch].note_dur < nph)
            {
                if (play[pitch].note_dur >= ph)
                {
                    play[pitch].pos = play[pitch].note_dur;
                    EVENT_SET_STATUS_OFF( &play[pitch] );

                    if (!evport_write_event(grid_intersort, &play[pitch]))
                    {
                        WARNING("failed to write to grid intersort\n");
                    }
                }

                play[pitch].flags = 0;
            }
        }
    }
}


void moport_rt_init_jack_cycle(moport* midiport, jack_nframes_t nframes)
{
    midiport->jport_buf = jack_port_get_buffer( midiport->jack_out_port,
                                                nframes);
    jack_midi_clear_buffer(midiport->jport_buf);
}


void moport_rt_output_jack_midi_event(  moport* midiport,
                                        event* ev,
                                        bbt_t ph,
                                        jack_nframes_t nframes,
                                        double frames_per_tick )
{
    unsigned char* buf;
    void* jport_buf = midiport->jport_buf;
    jack_nframes_t pos = (ev->pos - ph) * frames_per_tick;

    if (EVENT_IS_STATUS_ON( ev ))
    {
        buf = jack_midi_event_reserve(jport_buf, pos, 3);

        if (buf)
        {
            buf[0] = (unsigned char)(0x90 | EVENT_GET_CHANNEL( ev ));
            buf[1] = (unsigned char)ev->note_pitch;
            buf[2] = (unsigned char)ev->note_velocity;
        }
        else
            WARNING("ph:%d note-ON event pos: %d "
                    "was not reserved by JACK\n", ph, ev->pos);
    }
    else if(EVENT_IS_STATUS_OFF( ev ))
    {
        buf = jack_midi_event_reserve(jport_buf, pos, 3);

        if (buf)
        {
                buf[0] = (unsigned char)(0x80 | EVENT_GET_CHANNEL( ev ));
                buf[1] = (unsigned char)ev->note_pitch;
                buf[2] = (unsigned char)ev->note_velocity;
        }
        else
            WARNING("ph:%d note-OFF event pos: %d "
                    "was not reserved by JACK\n", ph, ev->pos);
    }
}


void moport_rt_pull_playing_and_empty(  moport* midiport,
                                        bbt_t ph, bbt_t nph,
                                        evport* grid_intersort)
{
    int channel, pitch;

    for (channel = 0; channel < 16; ++channel)
    {
        event* play = midiport->play[channel];

        for (pitch = 0; pitch < 128; ++pitch)
        {
            if (play[pitch].flags)
            {
                play[pitch].pos = 0;
                play[pitch].note_dur = 1;
                play[pitch].box_release = 2;/*ph;*/

                EVENT_SET_STATUS_OFF( &play[pitch] );

                if (!evport_write_event(grid_intersort, &play[pitch]))
                {
                    WARNING("failed to write play-event "
                            "to grid intersort\n");
                }

                play[pitch].flags = 0;
            }
        }
    }
}


void moport_event_dump(moport* midiport)
{
    int channel, pitch;
    DMESSAGE("midiport:%p\n",midiport);
    DMESSAGE("moport dump: \"%s\"\n", midiport->name);
    DMESSAGE("playing:\n");

    for (channel = 0; channel < 16; ++channel)
    {
        event* play = midiport->play[channel];

        for (pitch = 0; pitch < 128; ++pitch)
        {
            if (play[pitch].flags)
            {
                event_dump(&play[pitch]);
                #ifndef NDEBUG
                EVENT_IS(&play[pitch], EV_STATUS_ON | EV_TYPE_NOTE);
                #endif
            }

        }
    }
}

