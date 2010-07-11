#include "jack_transport.h"

#include "jack_midi.h"
#include "common.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
    *some* code here is based upon NON-Sequencer transport code...
*/

static void jtransp_timebase_cb(    jack_transport_state_t  state,
                                    jack_nframes_t          nframes,
                                    jack_position_t*        pos,
                                    int                     new_pos,
                                    void*                   arg);


struct jack_transport
{
    _Bool is_master;
    _Bool is_rolling;
    _Bool is_valid;

    bbt_t  bar;
    bbt_t  beat;
    bbt_t  tick;

    double  master_beats_per_minute;
    float   master_beats_per_bar;
    float   master_beat_type;

    double  beats_per_minute;
    float   beats_per_bar;
    float   beat_type;

    double  ticks_per_period;
    double  frames_per_tick;

    double  frame_rate;
    double  ticks;
    double  frame;
    double  nframes;

    jack_client_t*  client;
    _Bool   recalc_timebase;
};


jtransp* jtransp_new(void)
{
    jtransp* tr = malloc(sizeof(*tr));

    if (!tr)
    {
        WARNING("out of memory allocating jack transport data\n");
        return 0;
    }

    tr->is_master = 0;
    tr->is_rolling = 0;
    tr->is_valid = 0;

    tr->bar = 0;
    tr->beat = 0;
    tr->tick = 0;

    tr->master_beats_per_minute = 120.0f;
    tr->master_beats_per_bar =    4;
    tr->master_beat_type =        4;

    tr->beats_per_minute = 0;
    tr->beats_per_bar = 0;
    tr->beat_type = 0;

    tr->ticks_per_period = 0;
    tr->frames_per_tick = 0;
    tr->frame_rate = 0;

    tr->ticks = 0;
    tr->frame = 0;
    tr->nframes = 0;

    tr->client = 0;
    tr->recalc_timebase = 1;

    return tr;
}


void jtransp_free(jtransp* tr)
{
    if (!tr)
        return;

    free(tr);
}


_Bool jtransp_startup(jtransp* tr, jack_client_t* client)
{
    if (!client)
        return 0;

    tr->client = client;

    if (jack_set_timebase_callback( client, 1,
                                    jtransp_timebase_cb, tr) == 0)
    {
        tr->is_master = 1;
    }
    else
    {
        tr->is_master = 0;
        WARNING("failed to init jack timebase callback\n");
        WARNING("running as slave\n");
    }

    return 1;
}


_Bool jtransp_shutdown(jtransp* tr)
{
    if (tr->is_master && jack_release_timebase(tr->client) != 0)
    {
        WARNING("error occurred releasing timebase\n");
        return 0;
    }

    return 1;
}


_Bool jtransp_is_master(jtransp* tr)
{
    return tr->is_master;
}


void jtransp_rewind(jtransp* tr)
{
    jack_transport_locate(tr->client, 0);
}


void jtransp_play(jtransp* tr)
{
    jack_transport_start(tr->client);
}


void jtransp_stop(jtransp* tr)
{
    jack_transport_stop(tr->client);
}


jack_transport_state_t jtransp_state(jtransp* tr, jack_position_t* pos)
{
    return jack_transport_query(tr->client, pos);
}


void jtransp_rt_poll(jtransp* tr, jack_nframes_t nframes)
{
    jack_position_t         pos;
    jack_transport_state_t  jstate;

    jstate = jack_transport_query(tr->client, &pos);

    tr->is_rolling = jstate == JackTransportRolling;

    if (!(tr->is_valid = pos.valid & JackPositionBBT))
        return;

    tr->bar =  pos.bar;
    tr->beat = pos.beat;
    tr->tick = pos.tick;

    /* bars and beats start at 1.. */
    pos.bar--;
    pos.beat--;

    _Bool recalc_ticks_per_period = 0;

    if (pos.frame_rate != tr->frame_rate
     || pos.beats_per_minute != tr->beats_per_minute)
    {
        recalc_ticks_per_period = 1;
    }

    tr->beats_per_bar =     pos.beats_per_bar;
    tr->beat_type =         pos.beat_type;
    tr->beats_per_minute =  pos.beats_per_minute;
    tr->frame =             pos.frame;
    tr->frame_rate =        pos.frame_rate;


    if (recalc_ticks_per_period)
    {
        const double frames_per_beat =
                (tr->frame_rate * 60.0f) / tr->beats_per_minute;

        tr->frames_per_tick = frames_per_beat / (double)ppqn;
        tr->ticks_per_period = nframes / tr->frames_per_tick;
    }

    const double abs_tick = 
        ((double)pos.bar * pos.beats_per_bar + (double)pos.beat)
            * pos.ticks_per_beat + pos.tick;

    /* scale Jack's ticks to our ticks */
    const double pulses_per_tick = ppqn / pos.ticks_per_beat;

    tr->ticks = abs_tick * pulses_per_tick;
    tr->tick = (bbt_t)(tr->tick * pulses_per_tick);

}


static void jtransp_timebase_cb(    jack_transport_state_t  state,
                                    jack_nframes_t          nframes,
                                    jack_position_t*        pos,
                                    int                     new_pos,
                                    void*                   arg)
{
    jtransp* tr = (jtransp*)arg;

    if (new_pos || tr->recalc_timebase)
    {
        pos->valid =            JackPositionBBT;
        pos->beats_per_minute = tr->master_beats_per_minute;
        pos->beats_per_bar =    tr->master_beats_per_bar;
        pos->beat_type =        tr->master_beat_type;
        pos->ticks_per_beat =   ppqn;

        const double minute =
                        (double)pos->frame / (pos->frame_rate * 60.0);

        const double abs_beat = minute * pos->beats_per_minute;
        const double abs_tick = abs_beat * ppqn;

        pos->bar =  (int32_t)(abs_beat / pos->beats_per_bar);

        pos->beat = (int32_t)(abs_beat - 
                            (double)pos->bar * pos->beats_per_bar + 1);

        pos->tick = (int32_t)(abs_tick - abs_beat * ppqn);

        pos->bar_start_tick = (double)pos->bar * pos->beats_per_bar
                                              * pos->ticks_per_beat;
        ++pos->bar;
        tr->recalc_timebase = 0;
    }
    else
    {
        pos->tick += (int32_t)
            (nframes * pos->ticks_per_beat * pos->beats_per_minute
                            / ((double)pos->frame_rate * 60.0f));

        while(pos->tick >= pos->ticks_per_beat)
        {
            pos->tick -= (int32_t)pos->ticks_per_beat;

            if (++pos->beat > pos->beats_per_bar)
            {
                pos->beat = 1;
                ++pos->bar;
                pos->bar_start_tick += pos->beats_per_bar
                                     * pos->ticks_per_beat;
            }
        }
    }
}


_Bool jtransp_rt_is_valid(jtransp* tr)
{
    return tr->is_valid;
}


_Bool jtransp_rt_is_rolling(jtransp* tr)
{
    return tr->is_rolling;
}


double jtransp_rt_ticks(jtransp* tr)
{
    return tr->ticks;
}


double jtransp_rt_ticks_per_period(jtransp* tr)
{
    return tr->ticks_per_period;
}


double jtransp_rt_frames_per_tick(jtransp* tr)
{
    return tr->frames_per_tick;
}
