#ifndef REAL_TIME_DATA_H
#define REAL_TIME_DATA_H


#include "datacb.h"


typedef struct real_time_data rtdata;


/*  this is a very primitive update mechanism... the datacb_rtdata
    callback is used to provide a copy of the data for RT use. whether
    the RT data is a plain duplicate or is a modified version is of no
    importance here. the datacb_free callback is used to free this data.

    the update uses GLIB atomic functions - g_atomic_pointer_get,
    g_atomic_pointer_set, and g_atomic_pointer_compare_and_exchange.

    in this early stage, this mechanism is only filled-out enough to
    provide the update mechanism - it has the potential to offer full(?)
    UNDO functionality, but here, were the UNDO functionality actually
    implemented (it wouldn't take much), it would only offer one level of
    UNDO.
*/


rtdata*     rtdata_new(const void*, datacb_rtdata, datacb_free);
void        rtdata_free(rtdata*);

void*       rtdata_data(rtdata*);
void*       rtdata_update(rtdata*);



#endif
