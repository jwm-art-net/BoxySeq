#include "meter.h"



void meter_master_init(meter* master, jack_position_t* pos)
{
    _Bool recalc = 0;

    master->bar =   pos->bar;
    master->beat =  pos->beat;
    master->tick =  pos->tick;

    if (master->beats_per_bar != pos->beats_per_bar
         || master->beat_type != pos->beat_type )
    {
        master->beats_per_bar = pos->beats_per_bar;
        master->beat_type =     pos->beat_type;
        master->ppqn =          pos->ticks_per_beat;

        double beat_ratio = (4.0 / master->beat_type);
        master->beat_length = master->ppqn * beat_ratio;
    }
}


void meter_master_to_internal(meter* master, meter* internal)
{
    double abs_beat = master->bar * master->beats_per_bar;
    double tratio = master->tick / master->beat_length;

    internal->bar = (bbt_t)(0 + floor(abs_beat));
    internal->beat = 1;
    internal->tick = tratio * internal->ppqn;

}

void meter_internal_to_slave(const meter* master, meter* slave)
{

}
