#ifndef INCLUDE_JACK_PROCESS_DATA_H
#define INCLUDE_JACK_PROCESS_DATA_H

/*

******************* THIS INCLUDE IS IMPLEMENTATION ONLy. *****************
** DO NOT MAKE THIS DATA STRUCTURE ACCESSIBLE OUTSIDE OF IMPLEMENTATION **
************************* THANKYOU FOR LISTENING *************************

*/


struct jack_process_data
{
    const char*     client_name;
    jack_client_t*  client;

    boxyseq*        bs;

    bbt_t   oph;
    bbt_t   onph;

    _Bool is_master;
    _Bool is_rolling;
    _Bool is_valid;

    _Bool stopped;
    _Bool was_stopped;
    _Bool repositioned;
    _Bool   recalc_timebase;


    bbt_t  bar;
    bbt_t  beat;
    bbt_t  tick;

    double  ftick;
    double  ftick_offset;

    double  master_beats_per_minute;
    float   master_beats_per_bar;
    float   master_beat_type;

    double  beats_per_minute;
    float   beats_per_bar;
    float   beat_type;

    double  beat_length;

    double  ticks_per_period;
    double  frames_per_tick;
    double  frames_per_beat;

    jack_nframes_t frame_rate;
    jack_nframes_t frame;
    jack_nframes_t nframes;

    jack_nframes_t frame_old;


    double  ticks;
    double  minute;

    double ticks_per_beat;

    double tick_ratio;
};


#endif
