#include "midi_out_port.h"


#include "debug.h"
#include "grid_boundary.h"
#include "musical_scale.h"


#include <jack/midiport.h>
#include <stdlib.h>


struct midi_out_port
{
    event start[16][128];
    event  play[16][128];

    evport* intersort;

    jack_port_t*    jack_out_port;
    void*           jport_buf;

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

    mo->jport_buf = 0;

    int c, p;

    for (c = 0; c < 16; ++c)
    {
        for (p = 0; p < 128; ++p)
            mo->start[c][p].flags =
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
/*
    int c, p;

    MESSAGE("midi out port data:\n");

    for (c = 0; c < 16; ++c)
    {
        int count = 0;
        MESSAGE("channel: %d\n", c);
        MESSAGE("start events...\n");

        for (p = 0; p < 128; ++p)
            if (mo->start[c][p].flags)
            {
                ++count;
                event_dump(&mo->start[c][p]);
            }

        MESSAGE("total: %d\n", count);

        count = 0;

        MESSAGE("play events...\n");

        for (p = 0; p < 128; ++p)
            if (mo->play[c][p].flags)
            {
                ++count;
                event_dump(&mo->play[c][p]);
            }

        MESSAGE("total: %d\n", count);
    }
*/
    free(mo);
}


const char* moport_name(moport* mo)
{
    return evport_name(mo->intersort);
}


int moport_start_event(moport* midiport, const event* ev,
                                            int grb_flags,
                                            int scale_bin,
                                            int scale_key )
{
    if (ev->note_dur == ev->pos)
        return -1;

    event* start = midiport->start[EVENT_CHANNEL(ev)];
    event*  play =  midiport->play[EVENT_CHANNEL(ev)];

    int pitch = ev->box_x;

    if (grb_flags & GRBOUND_PITCH_STRICT_POS)
    {
        if (!scale_note_is_valid(scale_bin, scale_key, pitch))
            return -1;

        if (start[pitch].flags || play[pitch].flags)
            return -1;
    }
    else
    {
        int startpitch, endpitch;
        int pitchdir = 1;

        startpitch = endpitch = pitch;

        if (grb_flags & FSPLACE_LEFT_TO_RIGHT)
            endpitch += ev->box_width;
        else
        {
            pitch = startpitch += ev->box_width;
            pitchdir = -1;
        }

        while( !scale_note_is_valid(scale_bin, scale_key, pitch)
            || start[pitch].flags
            || play[pitch].flags   )
        {
            pitch += pitchdir;
            if (pitch == endpitch)
                return -1;
        }
    }

    event_copy(&start[pitch], ev);
    start[pitch].note_pitch = pitch;

    EVENT_SET_STATUS_ON( &start[pitch] );

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
            if (play[pitch].flags)
            {
                if (play[pitch].note_dur < nph)
                {
                    play[pitch].pos = play[pitch].note_dur - ph;
                    EVENT_SET_STATUS_OFF( &play[pitch] );

                    if (!evport_write_event(midiport->intersort,
                                                    &play[pitch]))
                    {
                        WARNING("failed to write event to intersort\n");
                    }

                    if (!grid_rt_unplace_event(gr, &play[pitch]))
                        WARNING("old event output to unplace failed\n");

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

    for (channel = 0; channel < 16; ++channel)
    {
        start = midiport->start[channel];
        play = midiport->play[channel];

        for (pitch = 0; pitch < 128; ++pitch)
        {
            if (start[pitch].flags)
            {
                start[pitch].pos -= ph;

                if (evport_write_event(midiport->intersort,
                                                &start[pitch]))
                {
                    event_copy(&play[pitch], &start[pitch]);
                }
                else
                    WARNING("new event output to intersort failed\n");

                start[pitch].flags = 0;
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


void moport_rt_output_jack_midi(moport* midiport, jack_nframes_t nframes,
                                                  double frames_per_tick )
{
    event ev;
    unsigned char* buf;
    void* jport_buf = midiport->jport_buf;

    evport_read_reset(midiport->intersort);

    #ifdef NO_REAL_TIME
    if (!evport_count(midiport->intersort))
        return;
    MESSAGE("midi out port:\n");
    while(evport_read_and_remove_event(midiport->intersort, &ev))
        event_dump(&ev);
    return;
    #endif

    while(evport_read_and_remove_event(midiport->intersort, &ev))
    {
        if (EVENT_IS_STATUS_ON( &ev ))
        {
            buf = jack_midi_event_reserve(  jport_buf,
                    (jack_nframes_t)((double)ev.pos * frames_per_tick),
                                            3 );
            if (buf)
            {
                buf[0] = (unsigned char)(0x90 | EVENT_CHANNEL( &ev ));
                buf[1] = (unsigned char)ev.note_pitch;
                buf[2] = (unsigned char)ev.note_velocity;
            }
            #ifdef GRID_DEBUG
            else
                WARNING("note-ON event was not reserved by JACK\n");
            #endif
        }
        else if(EVENT_IS_STATUS_OFF( &ev ))
        {
            buf = jack_midi_event_reserve(jport_buf,
                (jack_nframes_t)((double)ev.pos * frames_per_tick),
                                            3 );
            if (buf)
            {
                buf[0] = (unsigned char)(0x80 | EVENT_CHANNEL( &ev ));
                buf[1] = (unsigned char)ev.note_pitch;
                buf[2] = (unsigned char)ev.note_velocity;
            }
            #ifdef GRID_DEBUG
            else
                WARNING("note-OFF event was not reserved by JACK\n");
            #endif
        }
    }
}


void moport_empty(moport* midiport, grid* gr, jack_nframes_t nframes)
{
    int channel;
    int pitch;
    event  ev;
    event* play;
    unsigned char* buf;
    void* jport_buf = midiport->jport_buf;

    for (channel = 0; channel < 16; ++channel)
    {
        play = midiport->play[channel];

        for (pitch = 0; pitch < 128; ++pitch)
        {
            if (play[pitch].flags)
            {
                EVENT_SET_STATUS_OFF( &play[pitch] );

                if (!evport_write_event(midiport->intersort,
                                                    &play[pitch]))
                {
                    WARNING("failed to write event to intersort\n");
                }

                grid_remove_event(gr, &play[pitch]);
                play[pitch].flags = 0;
            }
        }
    }

    evport_read_reset(midiport->intersort);

    while(evport_read_and_remove_event(midiport->intersort, &ev))
    {
        if (EVENT_IS_STATUS_OFF( &ev ))
        {
            buf = jack_midi_event_reserve(jport_buf,(jack_nframes_t)0, 3);

            if (buf)
            {
                buf[0] = (unsigned char)(0x80 | EVENT_CHANNEL( &ev ));
                buf[1] = (unsigned char)ev.note_pitch;
                buf[2] = (unsigned char)ev.note_velocity;
            }
            #ifdef GRID_DEBUG
            else
                WARNING("note-OFF event was not reserved by JACK\n");
            #endif
        }
    }
}


#ifdef GRID_DEBUG
_Bool moport_event_in_start(moport* mo, event* ev)
{
    int c, p;
    event* start;
    for (c = 0; c < 16; ++c)
    {
        start = mo->start[c];
        for (p = 0; p < 128; ++p)
        {
            if (start[p].flags)
            {
                if (start[p].pos ==             ev->pos
                 && start[p].note_dur ==        ev->note_dur
                 && start[p].note_pitch ==      ev->note_pitch
                 && start[p].note_velocity ==   ev->note_velocity
                 && start[p].box_x ==           ev->box_x
                 && start[p].box_y ==           ev->box_y
                 && start[p].box_release ==     ev->box_release)
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}
#endif
