#ifndef INCLUDE_EVENT_PORT_DATA_H
#define INCLUDE_EVENT_PORT_DATA_H


/*

******************* THIS INCLUDE IS IMPLEMENTATION ONLy. *****************
** DO NOT MAKE THIS DATA STRUCTURE ACCESSIBLE OUTSIDE OF IMPLEMENTATION **
************************* THANKYOU FOR LISTENING *************************

*/


struct event_port
{
    char* name;
    rt_evlist*  data;
};


#endif
