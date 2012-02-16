#ifndef JACK_PROCESS_H
#define JACK_PROCESS_H


#ifdef __cplusplus
extern "C" {
#endif


#include "boxyseq_types.h"
#include "event.h"


#include <jack/jack.h>

#include <stdbool.h>


typedef struct jack_process_data jackdata;

jackdata*       jackdata_new(void);
void            jackdata_free(jackdata*);

bool            jackdata_startup(jackdata*, boxyseq*);
void            jackdata_shutdown(jackdata*);

jack_client_t * jackdata_client(jackdata*);
const char*     jackdata_client_name(jackdata*);


void    jackdata_transport_rewind(jackdata*);
void    jackdata_transport_play(jackdata*);
void    jackdata_transport_stop(jackdata*);

jack_transport_state_t
        jackdata_transport_state(jackdata*, jack_position_t* pos);

double  jackdata_rt_transport_frames_per_tick(jackdata*);

#ifndef NDEBUG
void    jackdata_rt_get_playhead(jackdata*, bbt_t* ph, bbt_t* nph);
#endif


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
