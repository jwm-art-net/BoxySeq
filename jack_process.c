#include "jack_process.h"

#include "boxy_sequencer.h"
#include "common.h"
#include "debug.h"
#include "pattern.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jack/midiport.h>


#include "include/jack_process_data.h"


static void jack_timebase_callback( jack_transport_state_t  state,
                                    jack_nframes_t          nframes,
                                    jack_position_t*        pos,
                                    int                     new_pos,
                                    void*                   arg      );

static int  jack_process_callback(  jack_nframes_t nframes, void* arg);

static void jackdata_jack_shutdown( void *arg );

static void jd_rt_poll(jackdata* jd, jack_nframes_t nframes);


jackdata* jackdata_new(void)
{
    jackdata* jd = malloc(sizeof(*jd));

    if (!jd)
    {
        WARNING("out of memory allocating jack data\n");
        return 0;
    }

    jd->client_name = 0;
    jd->client = 0;

    jd->bs = 0;

    jd->oph = -1;
    jd->onph = 0;

    jd->is_master = 0;
    jd->is_rolling = 0;
    jd->is_valid = 0;

    jd->stopped = 1;
    jd->was_stopped = 0;
    jd->repositioned = 0;
    jd->recalc_timebase = 1;

    jd->bar = 0;
    jd->beat = 0;
    jd->tick = 0;

    jd->ftick = 0;
    jd->ftick_offset = 0;

    jd->master_beats_per_minute = 120.0f;
    jd->master_beats_per_bar =    4;
    jd->master_beat_type =        4;

    jd->beats_per_minute = 0;
    jd->beats_per_bar = 0;
    jd->beat_type = 0;

    jd->beat_length = 0;

    jd->ticks_per_period = 0;
    jd->frames_per_tick = 0;
    jd->frames_per_beat = 0;

    jd->frame_rate = 0;
    jd->frame = 0;
    jd->nframes = 0;

    jd->frame_old = 0;

    jd->ticks = 0;
    jd->minute = 0;

    jd->ticks_per_beat = 0;

    jd->tick_ratio = 0;

    return jd;
}


void jackdata_free(jackdata* jd)
{
    if (!jd)
        return;

    free(jd);
}


_Bool jackdata_startup(jackdata* jd, boxyseq* bs)
{
    #ifdef NO_REAL_TIME
    jd->client = 0;
    #else
    const char *server_name = NULL;
    jack_options_t options = JackNullOption;
    jack_status_t status = 0;

    jd->client = jack_client_open(  boxyseq_basename(bs),
                                    options,
                                    &status,
                                    server_name );

    if (!jd->client)
    {
        WARNING("jack_client_open() failed: status = 0x%2.0x\n", status);
        if (status & JackServerFailed)
            WARNING("Unable to connect to JACK server\n");
        return 0;
    }
    #endif

    boxyseq_set_jack_client(bs, jd->client);
    boxyseq_set_jackdata(bs, jd);
    jd->bs = bs;

    #ifdef NO_REAL_TIME
    jd->client_name = "BoxySeq";
    #else
    jd->client_name = jack_get_client_name(jd->client);

    if (status & JackServerStarted)
        MESSAGE("JACK server started\n");

    if (status & JackNameNotUnique)
        MESSAGE("unique name `%s' assigned\n", jd->client_name);
    #endif

    if (jack_set_timebase_callback( jd->client, 1,
                                    jack_timebase_callback, jd) == 0)
    {
        jd->is_master = 1;
    }
    else
    {
        jd->is_master = 0;
        WARNING("failed to init jack timebase callback\n");
        WARNING("running as slave\n");
    }

    #ifdef NO_REAL_TIME
    #else
    if (jack_set_process_callback(  jd->client,
                                    jack_process_callback, jd) != 0)
    {
        WARNING("failed to init jack process callback\n");
        return 0;
    }

    jack_on_shutdown(jd->client, jackdata_jack_shutdown, jd);

    if (jack_activate(jd->client))
    {
        WARNING("failed to activate jack client\n");
        return 0;
    }
    #endif

    return 1;
}


void jackdata_shutdown(jackdata* jd)
{
    if (jd->is_master && jack_release_timebase(jd->client) != 0)
        WARNING("error occurred releasing timebase\n");

    jack_client_close(jd->client);
}


jack_client_t* jackdata_client(jackdata* jd)
{
    return jd->client;
}


const char* jackdata_client_name(jackdata* jd)
{
    return jd->client_name;
}


void jackdata_transport_rewind(jackdata* jd)
{
    jack_transport_locate(jd->client, 0);
}

void jackdata_transport_play(jackdata* jd)
{
    jack_transport_start(jd->client);
}

void jackdata_transport_stop(jackdata* jd)
{
    jack_transport_stop(jd->client);
}


jack_transport_state_t
jackdata_transport_state(jackdata* jd, jack_position_t* pos)
{
    return jack_transport_query(jd->client, pos);
}


double jackdata_rt_transport_frames_per_tick(jackdata* jd)
{
    return jd->frames_per_tick;
}


static void jack_timebase_callback( jack_transport_state_t  state,
                                    jack_nframes_t          nframes,
                                    jack_position_t*        pos,
                                    int                     new_pos,
                                    void*                   arg      )
{
    jackdata* jd = (jackdata*)arg;

    if (pos->frame_rate != jd->frame_rate)
        jd->recalc_timebase = 1;

    jd->minute = (double)pos->frame / (pos->frame_rate * 60.0);

    if (new_pos || jd->recalc_timebase)
    {
        pos->valid =            JackPositionBBT;
        pos->beats_per_minute = jd->master_beats_per_minute;
        pos->beats_per_bar =    jd->master_beats_per_bar;
        pos->beat_type =        jd->master_beat_type;
        pos->ticks_per_beat =   internal_ppqn;

        jd->frames_per_beat =
            (pos->frame_rate * 60) /
                (pos->beats_per_minute / (4.0 / pos->beat_type));

        jd->frames_per_tick = jd->frames_per_beat / pos->ticks_per_beat;
        jd->ticks_per_period = nframes / jd->frames_per_tick;

        double abs_beat =
            jd->minute * pos->beats_per_minute * (4.0 / pos->beat_type);

        const double abs_tick = abs_beat * pos->ticks_per_beat;

        pos->bar =  (int32_t)(abs_beat / pos->beats_per_bar);

        pos->beat = 1 + 
            (int32_t)(abs_beat - (double)pos->bar * pos->beats_per_bar);

        jd->ftick = abs_tick - floor(abs_beat) * pos->ticks_per_beat;

        pos->tick = (int32_t)jd->ftick;

        pos->bar_start_tick = (double)pos->bar * pos->beats_per_bar
                                               * pos->ticks_per_beat;

        ++pos->bar;
        jd->recalc_timebase = 0;
    }
    else
    {
        jd->ftick += jd->ticks_per_period;
        pos->tick = (int32_t)jd->ftick;
        jd->ftick_offset = jd->ftick - pos->tick;

        while(pos->tick >= pos->ticks_per_beat)
        {
            pos->tick = pos->tick - (int32_t)pos->ticks_per_beat;

            if (++pos->beat > pos->beats_per_bar)
            {
                pos->beat = 1;
                ++pos->bar;

                pos->bar_start_tick +=
                    pos->beats_per_bar * pos->ticks_per_beat;

            }
        }

        jd->ftick = pos->tick + jd->ftick_offset;
    }
}


static void jd_rt_poll(jackdata* jd, jack_nframes_t nframes)
{
    jack_position_t         pos;
    jack_transport_state_t  jstate;

    _Bool meter_change, bpm_change, frame_rate_change;

    jstate = jack_transport_query(jd->client, &pos);

    jd->is_rolling = (jstate == JackTransportRolling);

    if (jd->is_rolling && jd->stopped)
        jd->stopped = 0;

    if (!(jd->is_valid = pos.valid & JackPositionBBT))
        return;

    meter_change = (    ((uint64_t)pos.beats_per_bar * 10000)
                     != ((uint64_t)jd->beats_per_bar * 10000)
                 ||     ((uint64_t)pos.beat_type * 10000)
                     != ((uint64_t)jd->beat_type * 10000)   );

    bpm_change = ((uint64_t)pos.beats_per_minute * 10000 !=
                  (uint64_t)jd->beats_per_minute * 10000 );

    frame_rate_change = (pos.frame_rate != jd->frame_rate);

    jd->repositioned =
        (jd->is_rolling && pos.frame != jd->frame + nframes);

    jd->frame =             pos.frame;
    jd->bar =               pos.bar;
    jd->beat =              pos.beat;

    if (meter_change)
    {
        jd->beats_per_bar =     pos.beats_per_bar;
        jd->beat_type =         pos.beat_type;
        jd->ticks_per_beat =    internal_ppqn;
    }

    /* bars and beats start at 1.. */
    pos.bar--;
    pos.beat--;


    if (meter_change || bpm_change || frame_rate_change)
    {
        jd->frame_rate =        pos.frame_rate;
        jd->beats_per_minute =  pos.beats_per_minute;
        jd->tick_ratio =        internal_ppqn / pos.ticks_per_beat;

        if (!jd->is_master)
        {
            jd->frames_per_beat =
                (jd->frame_rate * 60) /
                    (jd->beats_per_minute / (4.0 / pos.beat_type));

            jd->frames_per_tick =
                jd->frames_per_beat / internal_ppqn;

            jd->ticks_per_period =
                nframes / jd->frames_per_tick;
        }
    }

    if (jd->is_master)
        jd->tick = pos.tick;
    else    /* FIXME: with ardour as timebase master this frequently */
        jd->tick = jd->ftick = pos.tick * jd->tick_ratio;
            /* drops 2 ticks. See also: jack_midi.c:jmidi_process. */

    jd->ticks = (pos.bar * pos.beats_per_bar + pos.beat) *
                    internal_ppqn + jd->tick;


}


static int jack_process_callback(jack_nframes_t nframes, void* arg)
{
    bbt_t ph;
    bbt_t nph;
    _Bool repositioned = 0;
    jackdata* jd = (jackdata*)arg;

    jd_rt_poll(jd, nframes);

    if (!jd->is_valid)
        return 0;

    if (!jd->is_rolling)
    {
        if (!jd->stopped)
        {
            jd->stopped = 1;
            jd->was_stopped = 1;
            boxyseq_rt_clear(jd->bs);
        }
        return 0;
    }

    if (jd->repositioned && !jd->was_stopped)
    {
        repositioned = 1;
        jd->repositioned = 0;
        boxyseq_rt_clear(jd->bs);
    }

    jd->was_stopped = 0;

    ph =  (bbt_t)(0 + jd->ticks);
    nph = (bbt_t)(0 + trunc(ph + jd->ticks_per_period));

    if (ph && ph == jd->oph)
        return 0;

    if (ph != jd->onph && !repositioned)
    {
        if (jd->onph > ph)
            WARNING("duplicated %lu ticks\n",
                    (long unsigned int)(jd->onph - ph));
        else if (jd->onph == ph - 1)    /*  FIXME: for when ardour is */
            ph--;                       /*  transport master...       */
        else if (jd->onph == ph - 2)    /* NOTE: I'm not convinced this */
            ph -= 2;                    /* problem is currently fixable */
        else                            /* (certainly not by my stds). */
            WARNING("dropped %lu ticks\n",
                    (long unsigned int)(ph - jd->onph));
    }

    jd->onph = nph;

    boxyseq_rt_play(jd->bs, nframes, ph, nph);

    jd->oph = ph;

    return 0;

}


static void jackdata_jack_shutdown( void *arg )
{
    WARNING("jack_shutdown callback called\n");
    exit(1);
}

