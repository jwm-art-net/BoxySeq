#ifndef REAL_TIME_DATA_H
#define REAL_TIME_DATA_H


#include "datacb.h"


typedef struct real_time_data rtdata;


rtdata*     rtdata_new(const void*, datacb_rtdata, datacb_free);
void        rtdata_free(rtdata*);

void*       rtdata_data(rtdata*);
void*       rtdata_update(rtdata*);



#endif
