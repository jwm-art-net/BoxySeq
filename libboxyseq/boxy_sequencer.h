#ifndef BOXY_SEQUENCER_H
#define BOXY_SEQUENCER_H


#include "boxyseq_types.h"
#include "common.h"
#include "event_buffer.h"
#include "event_port_manager.h"
#include "freespace_state.h"
#include "grbound_manager.h"
#include "jack_process.h"
#include "moport_manager.h"
#include "pattern_manager.h"
#include "real_time_data.h"


#include <jack/jack.h>


#include "include/boxy_sequencer_data.h"


boxyseq*    boxyseq_new(int argc, char** argv);

void        boxyseq_free(boxyseq*);

void        boxyseq_shutdown(boxyseq*);

const char* boxyseq_basename(const boxyseq*);

void        boxyseq_set_jackdata(boxyseq*, jackdata*);
jackdata*   boxyseq_jackdata(boxyseq*);


pattern_manager*    boxyseq_pattern_manager(boxyseq*);
grbound_manager*    boxyseq_grbound_manager(boxyseq*);
moport_manager*     boxyseq_moport_manager(boxyseq*);
evport_manager*     boxyseq_pattern_port_manager(boxyseq*);


evbuf*          boxyseq_ui_note_on_buf(const boxyseq*);
evbuf*          boxyseq_ui_note_off_buf(const boxyseq*);
evbuf*          boxyseq_ui_unplace_buf(const boxyseq*);


void            boxyseq_update_rt_data(const boxyseq*);

/*  rt threads stuff -------->
*/

void            boxyseq_rt_init_jack_cycle(boxyseq*, jack_nframes_t);

void            boxyseq_rt_play(boxyseq*,
                                jack_nframes_t,
                                _Bool repositioned,
                                bbt_t ph, bbt_t nph);


void            boxyseq_rt_clear(boxyseq*, jack_nframes_t nframes);

/*  UI event triggering... -->
*/

void            boxyseq_ui_trigger_clear(boxyseq*);

int             boxyseq_ui_collect_events(boxyseq*);
evlist*         boxyseq_ui_event_list(boxyseq*);

#endif
