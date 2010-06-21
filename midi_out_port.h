#ifndef MIDI_OUT_PORT_H
#define MIDI_OUT_PORT_H


#include "event_port.h"

typedef struct midi_out_port moport;

moport*     moport_new(void);
void        moport_free(moport*);

void        moport_set_grid_unplace_port(moport*, evport*);


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


/* channel is stored in event at this stage */
int         moport_output(moport* midiport, const event* ev,
                                            int grb_flags );

void        moport_rt_play(moport*, bbt_t ph, bbt_t nph);


#endif
