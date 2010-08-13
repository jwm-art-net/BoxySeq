#ifndef INCLUDE_MIDI_OUT_PORT_DATA_H
#define INCLUDE_MIDI_OUT_PORT_DATA_H

/*

******************* THIS INCLUDE IS IMPLEMENTATION ONLy. *****************
** DO NOT MAKE THIS DATA STRUCTURE ACCESSIBLE OUTSIDE OF IMPLEMENTATION **
************************* THANKYOU FOR LISTENING *************************

*/


struct midi_out_port
{
    event start[16][128];
    event  play[16][128];

    evport* intersort;

    jack_port_t*    jack_out_port;
    void*           jport_buf;

};


#endif
