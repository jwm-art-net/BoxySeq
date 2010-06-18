#ifndef GUI_MAIN_H
#define GUI_MAIN_H


#include "boxy_sequencer.h"
#include "jack_midi.h"


#include <gtk/gtk.h>



int     gui_init(int* argc, char*** argv, boxyseq*, jmidi*);


#endif
