#ifndef METER_H
#define METER_H


#include "common.h"


#include <jack/jack.h>


typedef struct meter
{
    float   beats_per_bar;
    float   beat_type;
    double  beat_length;

    bbt_t   bar;
    bbt_t   beat;
    bbt_t   tick;


    bbt_t   bar_start_tick;

    double  ppqn;

    

} meter;


void    meter_master_init(meter* master, jack_position_t*);
void    meter_master_to_slave(const meter* master, meter* slave);

#endif
