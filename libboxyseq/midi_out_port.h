#ifndef MIDI_OUT_PORT_H
#define MIDI_OUT_PORT_H


#ifdef __cplusplus
extern "C" {
#endif


#include "boxyseq_types.h"
#include "event_port_manager.h"


#include <jack/jack.h>


moport*     moport_new(jack_client_t* client,   int port_id,
                                                evport_manager* portman);
void        moport_free(moport*);



/*  moport_rt_event_get_pitch
 *-----------------------------
 *  determines the pitch if possible of an event placed in the grid.
 *
 *  returns values:
 *      -1 if it is not possible for the event to generate any pitch
 *      the event pitch
 *
 *  the pitch is determined by a number of factors:
 *      1) is boundary flag GRBOUND_PITCH_STRICT_POS is set then
 *          only the x coordinate of the event is taken as the pitch
 *          and if 
 *  an event is given a potential coordinate for where within a
        boundary (within the grid) it might be placed.
        before it is actually placed, we need to check if there are
        notes on this channel already playing at that pitch.

        an event when placed has width (and of course height) which
        represents the range of pitches which it might actually output.

        we need to test which if any of these pitches are not already
        playing in the channel, and return the first available pitch
        which can be played.

        or, if no pitch is available in the channel, return -1 to
        indicate the note will not play, and consequently act
        appropriately, place the box but as a blocker - not a note,
        or, discard the event entirely.
*/


/*  moport_rt_push_event_pitch
 *------------------------------
 *  final determination of event note-on status. the event has already been
 *  placed when this is called. this determines if the x placement is valid
 *  within the key/scale (width may or may not be taken into account) and if
 *  the resultant pitch is already active on that midi port/channel.
 *  returns pitch on success, -1 on failure.
 */

int         moport_rt_push_event_pitch( moport*, const event* ev,
                                        int grb_flags,
                                        int scale_bin,
                                        int scale_key  );

void        moport_rt_init_jack_cycle(  moport*,  jack_nframes_t nframes);

void        moport_rt_pull_ending(      moport*, bbt_t ph, bbt_t nph,
                                                evport* grid_intersort);

void        moport_rt_process_new(      moport*, bbt_t ph, bbt_t nph);

void        moport_rt_output_jack_midi_event(moport*, event*,
                                                bbt_t ph,
                                                jack_nframes_t nframes,
                                                double frames_per_tick );

void        moport_rt_pull_playing_and_empty(   moport*,
                                                bbt_t ph, bbt_t nph,
                                                evport* grid_intersort);

void        moport_rt_empty(moport*, grid*, jack_nframes_t nframes);


void        moport_event_dump(moport*);

#ifdef GRID_DEBUG
#include <stdbool.h>
bool       moport_event_in_start(moport*, event*);
#endif


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
