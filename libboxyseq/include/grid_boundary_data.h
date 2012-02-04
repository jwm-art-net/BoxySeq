#ifndef INCLUDE_GRID_BOUNDARY_H
#define INCLUDE_GRID_BOUNDARY_H

/*

******************* THIS INCLUDE IS IMPLEMENTATION ONLy. *****************
** DO NOT MAKE THIS DATA STRUCTURE ACCESSIBLE OUTSIDE OF IMPLEMENTATION **
************************* THANKYOU FOR LISTENING *************************

*/


struct grid_boundary
{
    basebox     box;
    int         flags;
    int         channel;
    int         scale_bin;
    int         scale_key;

    evport*     evinput;

    moport*     midiout;

    rtdata*     rt;
};


#endif
