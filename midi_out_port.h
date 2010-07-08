#ifndef MIDI_OUT_PORT_H
#define MIDI_OUT_PORT_H


#include "boxyseq.h"
#include "event_port.h"


#include <jack/jack.h>


moport*     moport_new(jack_client_t* client, evport_manager* portman);
void        moport_free(moport*);

const char* moport_name(moport*);

/*  moport_test_pitch
        an event is given a potential coordinate for where within a
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


/*  moport_start_event:

    channel is stored in event at this stage,
    returns pitch:
*/

int         moport_start_event(moport*, const event* ev,
                                        int grb_flags,
                                        int scale_bin,
                                        int scale_key  );

void        moport_rt_play_old(moport*, bbt_t ph, bbt_t nph, grid*);
void        moport_rt_play_new(moport*, bbt_t ph, bbt_t nph);

void        moport_rt_output_jack_midi(moport*, jack_nframes_t nframes,
                                                double frames_per_tick );

void        moport_empty(moport*, grid*);


#ifdef GRID_DEBUG
_Bool       moport_event_in_start(moport*, event*);
#endif

#endif
