#ifndef PATTERN_H
#define PATTERN_H


#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>


#include "common.h"
#include "event.h"
#include "event_list.h"
#include "event_port.h"


typedef struct event_pattern pattern;


pattern*    pattern_new(int id);
pattern*    pattern_dup(const pattern*);

void        pattern_free(pattern*);

evlist*     pattern_event_list(pattern*);

void        pattern_set_meter(pattern*, float beats_per_bar,
                                        float beat_type     );

void        pattern_set_loop_length(    pattern*,   bbt_t ticks );
void        pattern_set_loop_length_bbt(pattern*,   bbt_t bar,
                                                    bbt_t beat,
                                                    bbt_t tick  );

bbt_t       pattern_loop_length(pattern*);

void        pattern_set_event_width_range(  pattern*,
                                            int width_min,
                                            int width_max  );

void        pattern_set_event_height_range( pattern*,
                                            int height_min,
                                            int height_max  );

void        pattern_set_random_seed_type(   pattern*, seed_type seedtype);
void        pattern_set_random_seed(        pattern*, uint32_t seed);


void        pattern_dump(const pattern*);


void        pattern_update_rt_data(const pattern*);

void        pattern_rt_play(    pattern*,   bool repositioned,
                                            bbt_t ph,
                                            bbt_t nph );

void        pattern_rt_stop(    pattern* );

void        pattern_set_output_port(pattern*, evport*);


/* helper functions */
void        pattern_event_bbt(  const pattern*, const event*,
                                bbtpos* pos,    bbtpos* dur );

bbt_t       pattern_duration_bbt_to_ticks(const pattern*,
                                            bbt_t bar,
                                            bbt_t beat,
                                            bbt_t tick      );


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
