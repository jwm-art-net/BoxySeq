#ifndef MOPORT_MANAGER_H
#define MOPORT_MANAGER_H


#ifdef __cplusplus
extern "C" {
#endif


#include "midi_out_port.h"


#include <jack/jack.h>


typedef struct moport_manager moport_manager;

moport_manager* moport_manager_new(void);
void            moport_manager_free(moport_manager*);

void            moport_manager_jack_client_set( moport_manager*,
                                                jack_client_t*   );

moport*         moport_manager_moport_new(moport_manager*);

moport*         moport_manager_moport_first(moport_manager*);
moport*         moport_manager_moport_next(moport_manager*);


/*  the first of these need only be called if the second ever is.
    furthermore, if the second of these is called, then the first
    must be called after a port has been created.
*/

void    moport_manager_update_rt_data(const moport_manager*);

void    moport_manager_rt_init_jack_cycle(  moport_manager*,
                                            jack_nframes_t nframes );

void    moport_manager_rt_play_old( moport_manager*,
                                    bbt_t ph,
                                    bbt_t nph,
                                    grid* );

void    moport_manager_rt_play_new_and_output(  moport_manager*,
                                                bbt_t ph,
                                                bbt_t nph,
                                                jack_nframes_t nframes,
                                                double frames_per_tick );

void    moport_manager_rt_empty(moport_manager*,
                                grid*,
                                jack_nframes_t nframes);


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
