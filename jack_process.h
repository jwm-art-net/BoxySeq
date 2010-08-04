#ifndef JACK_PROCESS_H
#define JACK_PROCESS_H

#include "boxyseq.h"
#include "event.h"

#include <jack/jack.h>


typedef struct jack_process_data jackdata;

jackdata*       jackdata_new(void);
void            jackdata_free(jackdata*);

_Bool           jackdata_startup(jackdata*, boxyseq*);
void            jackdata_shutdown(jackdata*);

jack_client_t * jackdata_client(jackdata*);
const char*     jackdata_client_name(jackdata*);


void    jackdata_transport_rewind(jackdata*);
void    jackdata_transport_play(jackdata*);
void    jackdata_transport_stop(jackdata*);

jack_transport_state_t
        jackdata_transport_state(jackdata*, jack_position_t* pos);




double  jackdata_rt_transport_frames_per_tick(jackdata*);

#endif
