#include "pattern.h"


#include "debug.h"
#include "event_list.h"
#include "real_time_data.h"


#include <glib.h>   /* mersene twister RNG */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>


#include "include/event_pattern_data.h"


static void*    pattern_rtdata_get_cb(const void* pat);
static void     pattern_rtdata_free_cb(void* pat);


pattern* pattern_new(int id)
{
    pattern* pat = malloc(sizeof(*pat));

    if (!pat)
        goto fail0;

    char tmp[80];
    snprintf(tmp, 79, "pattern_%02d", id);
    tmp[79] = '\0';
    pat->name = strdup(tmp);

    if (!(pat->name))
        goto fail1;

    pat->events = evlist_new();

    if (!pat->events)
        goto fail2;

    pat->rt = rtdata_new(pat,   pattern_rtdata_get_cb,
                                pattern_rtdata_free_cb );
    if (!pat->rt)
        goto fail3;

    pattern_set_loop_length_bbt(pat, 1, 0 ,0);

    pattern_set_meter(pat, 4, 4);
    pattern_set_event_width_range(pat, 4, 5);
    pattern_set_event_height_range(pat, 4, 5);
/*
    pattern_set_random_seed_type(pat, SEED_TIME_SYS);
*/
    pat->evout = 0;

    return pat;

fail3:  evlist_free(pat->events);
fail2:  free(pat->name);
fail1:  free(pat);
fail0:  WARNING("out of memory for new pattern\n");
    return 0;
}


pattern* pattern_dup(const pattern* pat)
{
    static int dup_id = 0;
    pattern* dest = pattern_new(--dup_id);

    if (!dest)
        goto fail0;

    dest->events = evlist_dup(pat->events);

    if (!dest->events)
        goto fail1;

    dest->rt = rtdata_new(pat,  pattern_rtdata_get_cb,
                                pattern_rtdata_free_cb );
    if (!pat->rt)
        goto fail2;

    dest->loop_length =     pat->loop_length;

    dest->width_min =       pat->width_min;
    dest->width_max =       pat->width_max;

    dest->height_min =      pat->height_min;
    dest->height_max =      pat->height_max;

    dest->beats_per_bar =   pat->beats_per_bar;
    dest->beat_type =       pat->beat_type;
    dest->beat_ratio =      pat->beat_ratio;

/*    dest->seedtype =        pat->seedtype;
    dest->seed =            pat->seed;*/

    dest->evout =           pat->evout;

    return dest;

fail2:  evlist_free(pat->events);
fail1:  free(dest);
fail0:
    return 0;
}


void pattern_free(pattern* pat)
{
    if (!pat)
        return;

    rtdata_free(pat->rt);
    evlist_free(pat->events);
    free(pat->name);
    free(pat);
}


evlist* pattern_event_list(pattern* pat)
{
    return pat->events;
}


void pattern_set_meter(pattern* pat, float beats_per_bar, float beat_type)
{
    if (beats_per_bar <= 1 && beat_type <= 1)
        return;

    pat->beats_per_bar = beats_per_bar;
    pat->beat_type =     beat_type;
    pat->beat_ratio =    4.0 / beat_type;
}


void pattern_set_loop_length(pattern* pat, bbt_t ticks)
{
    pat->loop_length = (ticks > 0 ? ticks : 0);
}


void pattern_set_loop_length_bbt(pattern* pat,  bbt_t bar,
                                                bbt_t beat,
                                                bbt_t ticks)
{
    pat->loop_length =
        pattern_duration_bbt_to_ticks(pat, bar, beat, ticks);
}


bbt_t pattern_loop_length(pattern* pat)
{
    return pat->loop_length;
}


void pattern_set_event_width_range( pattern* pat,
                                    int width_min,
                                    int width_max  )
{
    if (!(width_min < width_max))
        return;

    pat->width_min = width_min;
    pat->width_max = width_max;
}


void pattern_set_event_height_range(pattern* pat,
                                    int height_min,
                                    int height_max  )
{
    if (!(height_min < height_max))
        return;

    pat->height_min = height_min;
    pat->height_max = height_max;
}


/*
void pattern_set_random_seed_type(pattern* pat, seed_type seedtype)
{
    pat->seedtype = seedtype;

    if (seedtype != SEED_USER)
    {
        if (seedtype == SEED_TIME_SYS)
            pat->seed = time(NULL);
        else
            pat->seed = 0;
    }
}


void pattern_set_random_seed(pattern* pat, int seed)
{
    if (pat->seedtype == SEED_USER)
        pat->seed = seed;
    else
        pat->seed = 0;
}
*/

void pattern_dump(const pattern* pat)
{
    MESSAGE("pattern: %p\n", (const void*)pat);

    if (!pat)
        return;

    MESSAGE("\tloop length: %d\n"
            "\twidth_min: %d\twidth_max: %d\n"
            "\tbeats_per_bar: %d\n\tbeat_type: %d\n"
            "\tbeat_ratio: %f\n",
            pat->loop_length,
            pat->width_min,     pat->width_max,
            pat->height_min,    pat->height_max,
            pat->beats_per_bar, pat->beat_type,
            pat->beat_ratio );

    evlist_dump_events(pat->events);
}

/*
void pattern_trigger(prtdata* prt, bbt_t start_tick, bbt_t end_tick)
{
    prt->triggered =    1;
    prt->start_tick =   start_tick;
    prt->end_tick =     end_tick;
    prt->index =        0;

    switch(prt->pd->seedtype)
    {
    case PDSEED_TIME_SYS:
        g_rand_set_seed(prt->rnd, (guint32)time(0));
        break;

    case PDSEED_TIME_TICKS:
        g_rand_set_seed(prt->rnd, (guint32)start_tick);
        break;

    case PDSEED_USER:
        g_rand_set_seed(prt->rnd, prt->pd->seed);
        break;

    default:
        WARNING("unknown seedtype for pattern\n");
        g_rand_set_seed(prt->rnd, 1);
    }
}
*/


void pattern_rt_play(pattern* pat,  bool repositioned,
                                    bbt_t ph,
                                    bbt_t nph)
{
    rt_pattern* rtpat;
    event*      events;

    bbt_t       tick;
    bbt_t       offset;
    bbt_t       nextoffset;
    bbt_t       evpos;
    int         patix;
    int         count;
    event*      ev;
    event*      evlast;
    bool       play_event;

    rtpat = rtdata_data(pat->rt);

    if (!rtpat)
    {
        WARNING("no pattern RT data\n");
        return;
    }

    events = rtpat->events;

    tick = ph % rtpat->loop_length;

    patix = ph / rtpat->loop_length;

    offset = rtpat->start_tick + (rtpat->loop_length * patix);
    nextoffset = offset + rtpat->loop_length;

    count = 1;

    rtpat->index = tick % rtpat->loop_length;
    rtpat->playing = 1;

    if (repositioned || !rtpat->evlast)
        ev = rtpat->events;
    else
        ev = rtpat->evlast;

    while (ev->pos > -1)
    {
        play_event = 0;
        evpos = ev->pos + offset;

        if (evpos >= ph && evpos < nph)
            play_event = 1;
        else
        {
            evpos = ev->pos + nextoffset;
            if (evpos >= ph && evpos < nph)
                play_event = 1;
        }

        if (play_event) /* store/restore to not modify pattern */
        {
            bbt_t opos = ev->pos;
            bbt_t odur = ev->note_dur;
            bbt_t orel = ev->box_release;

            /* FIXME:   weren't we going to have events where the
                        dimensions were specified rather than random?
            */
            if (!ev->box.w)
                ev->box.w = g_rand_int_range(rtpat->rnd, rtpat->width_min,
                                                         rtpat->width_max);
            if (!ev->box.h)
                ev->box.h = g_rand_int_range(rtpat->rnd, rtpat->height_min,
                                                         rtpat->height_max);
            ev->pos = evpos;
            ev->note_dur += evpos;
            ev->box_release += ev->note_dur;

            if (!evport_write_event(rtpat->evout, ev))
                WARNING("dropped event\n");

            ev->pos = opos;
            ev->note_dur = odur;
            ev->box_release = orel;
            evlast = ev;
        }
        ++ev;
    }
    rtpat->evlast = (ev->pos > -1) ? evlast : 0;
}


void pattern_rt_stop(pattern* pat)
{
    rt_pattern* rtpat = rtdata_data(pat->rt);

    rtpat->playing =      0;
    rtpat->start_tick =   0;
    rtpat->end_tick =     0;
    rtpat->index =        0;
}


static rt_pattern* rt_pattern_new(void)
{
    rt_pattern* rtpat = malloc(sizeof(*rtpat));

    MESSAGE("new rt_pattern...\n");

    if (!rtpat)
        goto fail0;

    rtpat->rnd = g_rand_new_with_seed((guint32)time(NULL));

    if (!rtpat->rnd)
        goto fail1;

    rtpat->playing = 0;
    rtpat->triggered = 0;

/*    rtpat->seedtype = SEED_TIME_SYS;*/

    rtpat->start_tick = rtpat->end_tick = 0;
    rtpat->index = rtpat->loop_length = 0;

    rtpat->width_min = rtpat->width_max = 0;
    rtpat->height_min = rtpat->height_max = 0;

    rtpat->events = 0;
    rtpat->evlast = 0;
    rtpat->evout = 0;

    return rtpat;

fail1:  free(rtpat);
fail0:  MESSAGE("out of memory for rt_pattern\n");
    return 0;
}


static void rt_pattern_free(rt_pattern* rtpat)
{
    if (!rtpat)
        return;

    g_rand_free(rtpat->rnd);
    free(rtpat->events);
    free(rtpat);
}


static void* pattern_rtdata_get_cb(const void* data)
{
    const pattern* pat = data;
    rt_pattern* rtpat = rt_pattern_new();

    MESSAGE("getting rt_pattern from callback\n");

    if (!rtpat)
        goto fail0;

    rtpat->events = evlist_to_array(pat->events);

    if (!rtpat->events)
        goto fail1;

    rtpat->loop_length =    pat->loop_length;

    rtpat->width_min =      pat->width_min;
    rtpat->width_max =      pat->width_max;

    rtpat->height_min =     pat->height_min;
    rtpat->height_max =     pat->height_max;

/*    rtpat->seedtype =       pat->seedtype;
    rtpat->seed =           pat->seed;*/

    rtpat->evout =          pat->evout;

    return rtpat;

fail1:  free(rtpat);
fail0:  WARNING("failed to get pattern's real time data\n");
    return 0;
}


static void pattern_rtdata_free_cb(void* data)
{
    rt_pattern_free(data);
}


void pattern_set_output_port(pattern* pat, evport* port)
{
    pat->evout = port;
}


void pattern_update_rt_data(const pattern* pat)
{
    rtdata_update(pat->rt);
}


void pattern_event_bbt( const pattern* pat,
                        const event* ev,
                        bbtpos* pos,
                        bbtpos* dur         )
{
    bbt_t ticks_per_beat;
    bbt_t ticks_per_bar;
    bbt_t tick = 0;

    ticks_per_beat = (bbt_t)(pat->beat_ratio * (float)internal_ppqn);
    ticks_per_bar = (bbt_t)(pat->beats_per_bar * (float)ticks_per_beat);

    if (pos)
    {
        tick =      ev->pos;
        pos->bar =  tick / ticks_per_bar;
        tick -=     pos->bar * ticks_per_bar;
        pos->bar++;
        pos->beat = tick / ticks_per_beat;
        tick -=     pos->beat * ticks_per_beat;
        pos->beat++;
        pos->tick = tick;
    }

    if (dur)
    {
        tick =      ev->note_dur;
        dur->bar =  tick / ticks_per_bar;
        tick -=     dur->bar * ticks_per_bar;
        dur->beat = tick / ticks_per_beat;
        tick -=     dur->beat * ticks_per_beat;
        dur->tick = tick;
    }
}


bbt_t pattern_duration_bbt_to_ticks(const pattern* pat,
                                        bbt_t bar,
                                        bbt_t beat,
                                        bbt_t tick      )
{
    bbt_t ticks_per_beat = (bbt_t)(pat->beat_ratio * internal_ppqn);

    return (bbt_t)((float)ticks_per_beat * pat->beats_per_bar * (float)bar
            + (float)ticks_per_beat * (float)beat + (float)tick);
}

