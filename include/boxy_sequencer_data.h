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

    pattern*    pattern_slot[MAX_PATTERN_SLOTS];
    grbound*    grbound_slot[MAX_GRBOUND_SLOTS];
    moport*     moport_slot[MAX_MOPORT_SLOTS];

    evport_manager* ports_pattern;
    evport_manager* ports_midi_out;

    rtdata*     rt;

    grid*       gr;

    evbuf*      ui_note_on_buf;
    evbuf*      ui_note_off_buf;
    evbuf*      ui_unplace_buf;

    evbuf*      ui_input_buf;

    evlist*     ui_eventlist; /* stores collect events from buffers */

    jackdata*       jd;
};


typedef struct rt_boxy_sequencer
{
    pattern*    pattern_slot[MAX_PATTERN_SLOTS];
    grbound*    grbound_slot[MAX_GRBOUND_SLOTS];
    moport*     moport_slot[MAX_MOPORT_SLOTS];

    _Bool quitting;

} rt_boxyseq;


#endif
