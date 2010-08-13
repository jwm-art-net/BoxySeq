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
    evport_manager* ports_midi_out;

    grid*       gr;

    evbuf*      ui_note_on_buf;
    evbuf*      ui_note_off_buf;
    evbuf*      ui_unplace_buf;

    evbuf*      ui_input_buf;

    evlist*     ui_eventlist; /* stores collect events from buffers */

    jackdata*   jd;

    _Bool rt_quitting;
};


#endif
