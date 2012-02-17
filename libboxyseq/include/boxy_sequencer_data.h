#ifndef INCLUDE_BOXY_SEQUENCER_H
#define INCLUDE_BOXY_SEQUENCER_H

/*

******************* THIS INCLUDE IS IMPLEMENTATION ONLy. *****************
** DO NOT MAKE THIS DATA STRUCTURE ACCESSIBLE OUTSIDE OF IMPLEMENTATION **
************************* THANKYOU FOR LISTENING *************************

*/


struct boxy_sequencer
{
    char* basename;

    pattern_manager*    patterns;
    grbound_manager*    grbounds;
    moport_manager*     moports;

    evport_manager* ports_pattern;

    grid*       gr;

    jack_ringbuffer_t*  ui_note_on_buf;
    jack_ringbuffer_t*  ui_note_off_buf;
    jack_ringbuffer_t*  ui_unplace_buf;
    jack_ringbuffer_t*  ui_input_buf;

    evlist*     ui_eventlist; /* stores collect events from buffers */

    jackdata*   jd;

    _Bool rt_quitting;
};


#endif
