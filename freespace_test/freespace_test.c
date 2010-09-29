#include "debug.h"
#include "freespace_state.h"
#include "freespace_boundary.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sys/time.h>


int main(int argc, char** argv)
{

    int bs;
    int as;
    int aw, ah;

    int resx;
    int resy;

    int sz;
    int placement =   FSPLACE_ROW_SMART
                    | FSPLACE_LEFT_TO_RIGHT
                    | FSPLACE_TOP_TO_BOTTOM;
    _Bool test;

    freespace* fs = freespace_new();
    fsbound* fsb = fsbound_new();

    for (bs = 1; bs < 128; ++bs)
    {
        fsbound_set_coords(fsb, 0, 0, bs, bs);
        if (!freespace_find(fs, fsb, placement, bs, bs, &resx, &resy))
            WARNING("failed to place %dx%d in %dx%d\n",bs,bs,bs,bs);
        freespace_clear(fs);
    }

    freespace_free(fs);

    fsbound_free(fsb);

    return 0;
}
