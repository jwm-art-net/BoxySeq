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

/*
    double  ticks_per_beat;
*/

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

    tr->is_master = tr->is_rolling = tr->is_valid = 0;

    tr->bar = tr->beat = tr->tick = 0;

    tr->master_beats_per_minute = 145.0f;
    tr->master_beats_per_bar =    4;
    tr->master_beat_type =        4;
    tr->beats_per_minute = 0;
    tr->beats_per_bar = tr->beat_type = 0;
/*
    tr->ticks_per_beat = 0;
*/
    tr->ticks_per_period = tr->frames_per_tick = 0;
    tr->frame_rate = tr->ticks = tr->frame = tr->nframes = 0;
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

    tr->is_rolling =    jstate == JackTransportRolling;
    tr->is_valid =      pos.valid & JackPositionBBT;
    tr->bar =           pos.bar;
    tr->beat =          pos.beat;
    tr->tick =          pos.tick;

    /* bars and beats start at 1.. */
    pos.bar--;
    pos.beat--;

    _Bool recalc_ticks_per_period = 0;

    if (pos.beats_per_minute != tr->beats_per_minute)
    {
//        signal_tempo_change(pos.beats_per_minute);
        recalc_ticks_per_period = 1;
    }

    if (pos.beats_per_bar != tr->beats_per_bar)
    {
//        signal_bpb_change( pos.beats_per_bar);
    }

    if (pos.beat_type != tr->beat_type)
    {
//        signal_beat_change(pos.beat_type);
    }

    if (pos.frame_rate != tr->frame_rate)
        recalc_ticks_per_period = 1;

/*
    tr->ticks_per_beat =    pos.ticks_per_beat;
*/

    tr->beats_per_bar =     pos.beats_per_bar;
    tr->beat_type =         pos.beat_type;
    tr->beats_per_minute =  pos.beats_per_minute;
    tr->frame =             pos.frame;
    tr->frame_rate =        pos.frame_rate;

    if (recalc_ticks_per_period)
    {
        const double frames_per_beat = 
                        tr->frame_rate * 60 / tr->beats_per_minute;

        tr->frames_per_tick = frames_per_beat / (double)ppqn;
        tr->ticks_per_period = nframes / tr->frames_per_tick;
    }

    double abs_tick = ((double)pos.bar * pos.beats_per_bar
                                      + (double)pos.beat)
                               * pos.ticks_per_beat + pos.tick;

    /* scale Jack's ticks to our ticks */
    const double pulses_per_tick = ppqn / pos.ticks_per_beat;

    tr->ticks = abs_tick * pulses_per_tick;
    tr->tick = (bbt_t)(tr->tick * pulses_per_tick);

/*
    tr->ticks_per_beat = ppqn;
*/

}


/*
void TempoMap::get_beat(unsigned long frame, double& bpm, double& beat) const {
    TempoChange* tc = const_cast<CDTree<TempoChange*>*>(m_frame2tc)->get(frame);
    bpm = double(tc->bpm);
    beat = tc->beat + bpm * double(frame - tc->frame) / (60 * m_frame_rate);
    //beat = int32_t(beat_d);
    //tick = int32_t((beat_d - int(beat_d)) * ticks_per_beat);
  }

 void Song::get_timebase_info(unsigned long frame, unsigned long framerate,
			       double ticks_per_beat, double& bpm, 
			       int32_t& beat, int32_t& tick,
			       double& frame_offset) const {
    double beat_d;
    m_tempo_map.get_beat(frame, bpm, beat_d);
    beat = int32_t(beat_d);
    double tick_d = (beat_d - beat) * ticks_per_beat;
    tick = int32_t(tick_d);
    double frames_per_tick = framerate * 60 / (bpm * ticks_per_beat);
    frame_offset = (tick_d - tick) * frames_per_tick;
  }

 void Sequencer::jack_timebase_callback(jack_transport_state_t state, 
					 jack_nframes_t nframes, 
					 jack_position_t* pos, 
					 int new_pos) {
    pos->beats_per_bar = 4;
    pos->ticks_per_beat = 10000;
    
    
    // can't pass references to pos-> members directly since it's packed
    int32_t beat, tick;
    double bpm, frame_offset;
    m_song.get_timebase_info(pos->frame, pos->frame_rate, pos->ticks_per_beat,
			     bpm, beat, tick, frame_offset);
    pos->beats_per_minute = bpm;
    pos->bar_start_tick = frame_offset;

    // if we are standing still or if we just relocated, calculate 
    // the new position
    if (new_pos || state != JackTransportRolling) {
      pos->beat = beat;
      pos->tick = tick;
    }
    // otherwise, just increase the BBT by a period
    else {
      double db = nframes * pos->beats_per_minute / (pos->frame_rate * 60.0);
      pos->beat = m_last_beat + int32_t(db);
      pos->tick = m_last_tick + int32_t((db - int(db)) * pos->ticks_per_beat);
      if (pos->tick >= pos->ticks_per_beat) {
	pos->tick -= int32_t(pos->ticks_per_beat);
	++pos->beat;
      }
    }
  
    m_last_beat = pos->beat;
    m_last_tick = pos->tick;

    pos->bar = int32_t(pos->beat / pos->beats_per_bar);
    pos->beat %= int(pos->beats_per_bar);
    pos->valid = JackPositionBBT;
    
    // bars and beats start from 1 by convention (but ticks don't!)
    ++pos->bar;
    ++pos->beat;
  }

  
*/


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

        double min = (double)pos->frame / (pos->frame_rate * 60.0);

        double abs_tick = min * pos->beats_per_minute
                              * pos->ticks_per_beat;

        double abs_beat = abs_tick / pos->ticks_per_beat;

        pos->bar =  (int32_t)(abs_beat / pos->beats_per_bar);
        pos->beat = (int32_t)(abs_beat - 
                            ((float)pos->bar * pos->beats_per_bar) + 1);

        pos->tick = (int32_t)(abs_tick - 
                             (abs_beat * pos->ticks_per_beat));

        pos->bar_start_tick =(float)pos->bar * pos->beats_per_bar
                                             * pos->ticks_per_beat;

        pos->bar++;

        tr->recalc_timebase = 0;
    }
    else
    {
        pos->tick += (int32_t)(nframes * pos->ticks_per_beat
                                       * pos->beats_per_minute
                                       / (pos->frame_rate * 60.0));

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
