#ifndef GUI_BOUNDARY_LIST_DATA_H
#define GUI_BOUNDARY_LIST_DATA_H

/*

******************* THIS INCLUDE IS IMPLEMENTATION ONLy. *****************
** DO NOT MAKE THIS DATA STRUCTURE ACCESSIBLE OUTSIDE OF IMPLEMENTATION **
************************* THANKYOU FOR LISTENING *************************

*/

enum
{
    COL_COLOUR,     /* boundary and event base-colour   */
    COL_ACTIVE,     /* toggles active or not            */
    COL_PLAY,       /* event type to place              */
    COL_PLACE_BY,   /* row-smart or column-smart        */
    COL_HDIR,       /* left-to-right or right-to-left   */
    COL_VDIR,       /* top-to-bottom or bottom-to-top   */
    COL_SCALE,      /* musical scale                    */
    COL_KEY,        /* musical key                      */
    COL_X,          /* or MIDI note                     */
    COL_Y,          /* or MIDI velocity                 */
    COL_W,          /* boundary width                   */
    COL_H,          /* boundary height                  */

    COL_COUNT
};

struct gui_boundary_list_data
{
    GtkWidget*          window;
    GtkTreeModel*       model;

    boxyseq*            bs;
    grbound_manager*    grbman;

};



#endif
