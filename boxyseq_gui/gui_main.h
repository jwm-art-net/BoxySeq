#ifndef GUI_MAIN_H
#define GUI_MAIN_H


#include "boxyseq_types.h"
#include "jack_process.h"


#include <gtk/gtk.h>


typedef struct gui_main_editor gui_main;


int gui_init(int* argc, char*** argv, boxyseq*);


#endif
