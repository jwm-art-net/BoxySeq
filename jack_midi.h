#ifndef JACK_MIDI_H
#define JACK_MIDI_H

#include "boxy_sequencer.h"
#include "event.h"
#include "jack_transport.h"

#include <jack/jack.h>


typedef struct jack_midi_data jmidi;

jmidi*          jmidi_new(void);
void            jmidi_free(jmidi*);

_Bool           jmidi_startup(jmidi*, boxyseq*);
void            jmidi_shutdown(jmidi*);

jack_client_t * jmidi_client(jmidi*);
const char*     jmidi_client_name(jmidi*);

_Bool           jmidi_is_master(jmidi*);
jtransp*        jmidi_jtransp(jmidi*);


/*  Real Time functions - prefix: midi_RT
    to be called *ONLY* from within the real time thread...
    --- that is, any function that is called from midi_process
        can call the following functions:

void    midi_RT_queue_event(int port, event*);

*/


#endif
