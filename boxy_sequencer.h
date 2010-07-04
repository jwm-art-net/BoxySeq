#ifndef BOXY_SEQUENCER_H
#define BOXY_SEQUENCER_H

#include "boxyseq.h"
#include "common.h"
#include "event_port.h"
#include "freespace_state.h"
#include "grid_boundary.h"
#include "jack_transport.h"
#include "pattern.h"


#include <jack/jack.h>


boxyseq*    boxyseq_new(int argc, char** argv);

void        boxyseq_free(boxyseq*);

_Bool       boxyseq_startup(boxyseq*);
void        boxyseq_shutdown(boxyseq*);

const char* boxyseq_basename(const boxyseq*);

void        boxyseq_set_jack_client(boxyseq*, jack_client_t* client);
void        boxyseq_set_jtransp(boxyseq*, jtransp* jacktransport);

/*  patterns ---------------->
*/

int             boxyseq_pattern_new(boxyseq*,
                                bbt_t beats_per_bar,
                                bbt_t beat_type    );

void            boxyseq_pattern_free(boxyseq*,  int slot);
pattern*        boxyseq_pattern_dup(boxyseq*,   int dest_slot,
                                                int src_slot);
pattern*        boxyseq_pattern(boxyseq*,       int slot);
evport_manager* boxyseq_pattern_ports(boxyseq*);

/*
_Bool       boxyseq_pattern_set_grbound(boxyseq*,   int pattern_slot,
                                                    int grbound_slot  );
*/

/*  grbounds ---------------->
*/

int             boxyseq_grbound_new(boxyseq*,   int x, int y,
                                                int w, int h  );
void            boxyseq_grbound_free(boxyseq*,  int slot);
grbound*        boxyseq_grbound(boxyseq*,       int slot);
evport_manager* boxyseq_bound_ports(boxyseq*);


/*  midi out ports ---------->
*/

int             boxyseq_moport_new(boxyseq*);
void            boxyseq_moport_free(boxyseq*,  int slot);
moport*         boxyseq_moport(boxyseq*,       int slot);


void            boxyseq_rt_play(boxyseq*,
                                jack_nframes_t,
                                bbt_t ph, bbt_t nph);

void boxyseq_empty(boxyseq*);

void            boxyseq_rt_stop(boxyseq*);

grid*   boxyseq_grid(boxyseq*);

#endif
