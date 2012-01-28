#ifndef INCLUDE_GRID_BOUNDARY_H
#define INCLUDE_GRID_BOUNDARY_H

/*

******************* THIS INCLUDE IS IMPLEMENTATION ONLy. *****************
** DO NOT MAKE THIS DATA STRUCTURE ACCESSIBLE OUTSIDE OF IMPLEMENTATION **
************************* THANKYOU FOR LISTENING *************************

*/


struct grid_boundary
{
    int         flags;
    int         channel;
    int         scale_bin;
    int         scale_key;

    unsigned char r, g, b;

    evport*     evinput;
    fsbound*    bound;

    moport*     midiout;

    rtdata*     rt;
};


#endif
