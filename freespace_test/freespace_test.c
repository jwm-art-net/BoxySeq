#include "debug.h"
#include "freespace_state.h"
#include "freespace_boundary.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sys/time.h>


/*************************************************************************/
/* TESTS                                                                 */
/*************************************************************************/

/*  test_box_multiple_placement
 *-------------------------------
 *  this test fills the free space grid with boxes of the size 'size'.
 *  the test ensures that not only is freespace found, but that it is also
 *  found at a specific location which is easily predictable.
 *
 *  the test should not fail.
 */

int test_box_multiple_placement(freespace* fs, int flags, int size)
{
    int y;
    int count = FSWIDTH / size;

    fsbound* fsb = fsbound_new();
    fsbound_set_coords(fsb, 0, 0, 128, 128);

    freespace_clear(fs);

    for (y = 0; y < count; ++y)
    {
        int x;

        for (x = 0; x < count; ++x)
        {
            int expx = 0;
            int expy = 0;
            int resx = -1;
            int resy = -1;

            if (flags & FSPLACE_ROW_SMART)
            {
                expx = (flags & FSPLACE_LEFT_TO_RIGHT)
                            ? x * size
                            : FSWIDTH - x * size - size;

                expy = (flags & FSPLACE_TOP_TO_BOTTOM)
                            ? y * size
                            : FSHEIGHT - y * size - size;
            }

            if (!freespace_find(fs, fsb, flags, size, size, &resx, &resy))
            {
                DWARNING(   "area search for size %d failed at location "
                            " x:%d, y:%d\n", size, size, x, y);
                return -1;
            }

            if (resx != expx || resy != expy)
            {
                DWARNING(   "size %d area found at x:%d, y:%d "
                            "but expected x:%d, y:%d\n",
                            size, resx, resy, expx, expy);
                return -1;
            }

            freespace_remove(fs, resx, resy, size, size);

        }
    }

    return 0;
}

/*  test_box_within_same_sized_boundary
 *---------------------------------------
 *  this test ensures box placement works in the edge-case where the area
 *  to be placed has exactly the same dimensions as the boundary.
 *  the original bug motivating the creation of this test went undetected
 *  for some time.
 *
 *  the freespace state is reset at the start of the test and at no time
 *  in this test is any free space removed, only the search is performed. 
 *
 *  the test should not fail.
 */
int test_box_within_same_sized_boundary(freespace* fs, int flags, int size)
{
    int x, y, resx, resy;

    fsbound* fsb = fsbound_new();

    freespace_clear(fs);

    for (x = 0; x < FSWIDTH - size; ++x)
    {
        for (y = 0; y < FSHEIGHT - size; ++y)
        {
            fsbound_set_coords(fsb, x, y, size, size);

            if (!freespace_find(fs, fsb, flags, size, size, &resx, &resy))
            {
                DWARNING(   "area search for size %d failed in boundary "
                            "size %d at location x:%d, y:%d\n",
                            size, size, x, y);
                return -1;
            }
        }
    }

    return 0;
}



int main(int argc, char** argv)
{
    int bs;
    int placement;
    int lastplacementtype = FSPLACE_ROW_SMART
                          | FSPLACE_LEFT_TO_RIGHT
                          | FSPLACE_TOP_TO_BOTTOM;

    freespace* fs = freespace_new();

/*

    int resx = 0;
    int resy = 0;

    placement = FSPLACE_ROW_SMART
              | FSPLACE_TOP_TO_BOTTOM
              | FSPLACE_LEFT_TO_RIGHT;

    char* pstr = freespace_placement_to_str(placement);

    DMESSAGE("using %s placement...\n", pstr);
    free(pstr);
    fsbound* fsb = fsbound_new();

    fsbound_set_coords(fsb, 0, 0, 128, 128);

    int x = 7;
    int y = 0;
    bs = 120;

    fsbound_set_coords(fsb, x, y, bs, bs);

    if (freespace_find(fs, fsb, placement, bs, bs, &resx, &resy))
    {
        DMESSAGE("success result x:%d, y:%d\n", resx, resy);
        freespace_remove(fs, resx, resy, bs, bs);
        freespace_dump(fs);
    }
    else
    {
        DWARNING("guess what? it fail\n");
    }

    goto end;
*/
/*

    for (placement = 0; placement <= lastplacementtype; ++placement)
    {
        if (!(placement & FSPLACE_ROW_SMART))
            continue;

        char* pstr = freespace_placement_to_str(placement);
        DMESSAGE("using %s placement...\n", pstr);
        free(pstr);

        freespace_clear(fs);

        for (bs = 1; bs < 34; bs++)
        {
            if (!freespace_find(fs, fsb, placement, bs, bs, &resx, &resy))
            {
                DWARNING("fail on:size %d\n",bs);
                break;
            }

            freespace_remove(fs, resx, resy, bs, bs);
        }

        for (bs = 1; bs < 17; bs++)
        {
            if (!freespace_find(fs, fsb, placement, bs, bs, &resx, &resy))
            {
                DWARNING("fail2 on:size %d\n",bs);
                break;
            }

            freespace_remove(fs, resx, resy, bs, bs);
        }

        freespace_dump(fs);

    }
*/
    freespace_clear(fs);

    MESSAGE("\n"
            "testing multiples of equal-sized box placement \n"
            "==============================================\n");

    for (placement = 0; placement <= lastplacementtype; ++placement)
    {
        if (!(placement & FSPLACE_ROW_SMART))
            continue;

        char* pstr = freespace_placement_to_str(placement);
        DMESSAGE("using %s placement...\n", pstr);
        free(pstr);

        for (bs = 1; bs < 65; ++bs)
        {
            if (test_box_multiple_placement(fs, placement, bs))
                break;
        }

        printf("\t%s\n", (bs == 65) ? "pass" : "fail");
    }



    freespace_clear(fs);

    MESSAGE("\n"
            "testing box placement within boundary of same size\n"
            "==================================================\n");

    for (placement = 0; placement <= lastplacementtype; ++placement)
    {
        if (!(placement & FSPLACE_ROW_SMART))
            continue;

        char* pstr = freespace_placement_to_str(placement);
        DMESSAGE("using %s placement...\n", pstr);
        free(pstr);

        for (bs = 1; bs < 128; ++bs)
        {
            if (test_box_within_same_sized_boundary(fs, placement, bs))
                break;
        }

        printf("\t%s\n", (bs == 128) ? "pass" : "fail");
    }

end:

    freespace_free(fs);

    return 0;
}
