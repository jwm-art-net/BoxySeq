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


void    moport_manager_update_rt_data(const moport_manager*);


/*  moport_manager_rt_init_jack_cycle is called at the beginning of every
 *  JACK process cycle.  currently done by boxyseq_rt_init_jack_cycle.
 */
void    moport_manager_rt_init_jack_cycle(  moport_manager*,
                                            jack_nframes_t nframes );

void    moport_manager_rt_pull_ending(  moport_manager*,
                                        bbt_t ph,
                                        bbt_t nph,
                                        evport* grid_intersort);

void    moport_manager_rt_pull_playing_and_empty(   moport_manager*,
                                                    bbt_t ph, bbt_t nph,
                                                    evport* grid_intersort);


void    moport_manager_dump_events(moport_manager*);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
