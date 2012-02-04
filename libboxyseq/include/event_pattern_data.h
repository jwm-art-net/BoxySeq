#ifndef INCLUDE_EVENT_PATTERN_H
#define INCLUDE_EVENT_PATTERN_H


/*

******************* THIS INCLUDE IS IMPLEMENTATION ONLy. *****************
** DO NOT MAKE THIS DATA STRUCTURE ACCESSIBLE OUTSIDE OF IMPLEMENTATION **
************************* THANKYOU FOR LISTENING *************************

*/


struct event_pattern
{
    char*   name;

    bbt_t   loop_length;

    int     width_min;
    int     width_max;

    int     height_min;
    int     height_max;

    float   beats_per_bar;
    float   beat_type;
    double  beat_ratio;

/*    seed_type   seedtype;
    int         seed;*/

    evlist*     events;

    rtdata*     rt;

    evport*     evout;
};


typedef struct rt_event_pattern
{
    bbt_t   loop_length;

    int     width_min;
    int     width_max;

    int     height_min;
    int     height_max;

/*    seed_type   seedtype;
    int         seed;*/

    event*  events;

    evport* evout;

    _Bool   playing;
    _Bool   triggered;

    GRand*  rnd;

    bbt_t   start_tick;
    bbt_t   end_tick;
    bbt_t   index;

    event*  evlast;

} rt_pattern;


#endif
