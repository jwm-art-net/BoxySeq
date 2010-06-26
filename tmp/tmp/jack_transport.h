#ifndef JACK_TRANSPORT_H
#define JACK_TRANSPORT_H


#include <jack/jack.h>

#include "common.h"

typedef struct jack_transport jtransp;


jtransp*    jtransp_new(void);
void        jtransp_free(jtransp*);

_Bool       jtransp_startup(jtransp*, jack_client_t*);
_Bool       jtransp_shutdown(jtransp*);

_Bool       jtransp_is_master(jtransp*);

void        jtransp_rewind(jtransp*);

void        jtransp_play(jtransp*);

void        jtransp_stop(jtransp*);


jack_transport_state_t
            jtransp_state(jtransp*, jack_position_t* pos);


/* RT thread only: */

void        jtransp_rt_poll(jtransp*, jack_nframes_t nframes);

_Bool       jtransp_rt_is_valid(jtransp*);
_Bool       jtransp_rt_is_rolling(jtransp*);
double      jtransp_rt_ticks(jtransp*);
double      jtransp_rt_ticks_per_period(jtransp*);



#endif
