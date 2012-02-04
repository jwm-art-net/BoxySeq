#include "debug.h"
#include "freespace_state.h"
#include "basebox.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sys/time.h>


/*************************************************************************/
/* TESTS                                                                 */
/*************************************************************************/

int test_box_within_same_sized_boundary(freespace* fs, int flags, int size);
int test_box_multiple_placement(freespace* fs, int flags, int size);
int test_box_placement_and_removal(freespace* fs, int flags);
int test_box_placement_limited_boundary(freespace* fs, int flags);


long total_searches = 0;
long total_removals = 0;





int main(int argc, char** argv)
{
    int bs;
    int placement = 0;
    int lastplacementtype = FSPLACE_ROW_SMART
                          | FSPLACE_LEFT_TO_RIGHT
                          | FSPLACE_TOP_TO_BOTTOM;

    freespace* fs = freespace_new();


    int i;
    int resx = 0;
    int resy = 0;


    char* pstr = freespace_placement_to_str(placement);

    MESSAGE("using %s placement...\n", pstr);
    free(pstr);

    basebox box;

    placement = 0
                | FSPLACE_ROW_SMART
                | FSPLACE_TOP_TO_BOTTOM
                | FSPLACE_LEFT_TO_RIGHT
                ;


    int w = 27;
    int h = 27;

    box_set_coords(&box, 0, 0, 128, 128);

    if (freespace_find(fs, &box, placement, w, h, &resx, &resy))
    {
        MESSAGE("success result x:%d, y:%d\n", resx, resy);
        freespace_remove(fs, resx, resy, w, h);
    }
    else
    {
        WARNING("guess what? it fail\n");
    }

    freespace_dump(fs,0);

    freespace_block_remove(fs, 15, 15, w, h);

    freespace_dump(fs,0);

    freespace_block_remove(fs, 25, 25, w, h);

    freespace_dump(fs,0);

    freespace_block_add(fs, 15, 15, w, h);

    freespace_dump(fs,0);

/*
    freespace_add(fs, 0, 0, w, h);

    freespace_dump(fs,0);

    freespace_remove(fs, resx, resy, w, h);

    freespace_dump(fs,0);

    freespace_block_add(fs, 25, 25, w, h);

    freespace_dump(fs,0);
*/
    goto end;


    /*---------------------------------------------------*/

/*
    int i;
    int resx = 0;
    int resy = 0;


    char* pstr = freespace_placement_to_str(placement);

    MESSAGE("using %s placement...\n", pstr);
    free(pstr);
    boxound* box = boxound_new();

    placement = 0
                | FSPLACE_ROW_SMART
                | FSPLACE_TOP_TO_BOTTOM
                | FSPLACE_LEFT_TO_RIGHT
                ;


    int w = 2;
    int h = 2;

    box_set_coords(box, 0, 0, 128, 128);

    if (freespace_find(fs, box, placement, w, h, &resx, &resy))
    {
        MESSAGE("success result x:%d, y:%d\n", resx, resy);
        freespace_remove(fs, resx, resy, w, h);
    }
    else
    {
        WARNING("guess what? it fail\n");
    }

    freespace_dump(fs);

    w = 32;
    h = 32;

    if (freespace_find(fs, box, placement, w, h , &resx, &resy))
    {
        MESSAGE("success result x:%d, y:%d\n", resx, resy);
        freespace_remove(fs, resx, resy, w, h);
    }
    else
    {
        WARNING("guess what? it fail\n");
    }

    freespace_dump(fs);

    goto end;

*/

/*
    for (i = 0; i < 2; ++i)
    {
        if (freespace_find(fs, box, placement, 1, 1, &resx, &resy))
        {
            MESSAGE("success result x:%d, y:%d\n", resx, resy);
            freespace_remove(fs, resx, resy, 1, 1 );
        }
        else
        {
            WARNING("guess what? it fail\n");
        }
    }
    
    freespace_dump(fs);

    goto end;
*/
/*
    for (placement = 0; placement <= lastplacementtype; ++placement)
    {
        if (!(placement & FSPLACE_ROW_SMART))
            continue;

        char* pstr = freespace_placement_to_str(placement);
        MESSAGE("using %s placement...\n", pstr);
        free(pstr);

        freespace_clear(fs);

        for (bs = 1; bs < 34; bs++)
        {
            if (!freespace_find(fs, box, placement, bs, bs, &resx, &resy))
            {
                WARNING("fail on:size %d\n",bs);
                break;
            }

            freespace_remove(fs, resx, resy, bs, bs);
        }

        for (bs = 1; bs < 17; bs++)
        {
            if (!freespace_find(fs, box, placement, bs, bs, &resx, &resy))
            {
                WARNING("fail2 on:size %d\n",bs);
                break;
            }

            freespace_remove(fs, resx, resy, bs, bs);
        }

        freespace_dump(fs);

    }
*/
    /*---------------------------------------------------*/

test_boundary_placement_equal_size:

    MESSAGE("\n"
            "testing box placement within boundary of same size\n"
            "==================================================\n");

    for (placement = 0; placement <= lastplacementtype; ++placement)
    {
        char* pstr = freespace_placement_to_str(placement);
        MESSAGE("using %s placement...\n", pstr);
        free(pstr);

        for (bs = 1; bs < 128; ++bs)
        {
            if (test_box_within_same_sized_boundary(fs, placement, bs))
                break;
        }

        printf("\t%s\n", (bs == 128) ? "pass" : "fail");
    }


test_multiples:

    MESSAGE("\n"
            "testing multiples of equal-sized box placement \n"
            "==============================================\n");

    for (placement = 0; placement <= lastplacementtype; ++placement)
    {
        int maxbs = 64;
        char* pstr = freespace_placement_to_str(placement);
        MESSAGE("using %s placement...\n", pstr);
        free(pstr);

        bs = 1;


        for (bs = 1; bs < maxbs; ++bs)
        {
            if (test_box_multiple_placement(fs, placement, bs))
            {
                break;
            }
        }

        printf("\t%s\n", (bs == maxbs) ? "pass" : "fail");
    }

test_placement_and_removal:

    MESSAGE("\n"
            "testing placement and removal and replacement\n"
            "=============================================\n");

    for (placement = 0; placement <= lastplacementtype; ++placement)
    {
        char* pstr = freespace_placement_to_str(placement);
        MESSAGE("using %s placement...\n", pstr);
        free(pstr);

        if (test_box_placement_and_removal(fs, placement))
            printf("\tfail\n");
        else
            printf("\tpass\n");
    }

test_limited_boundary:

    MESSAGE("\n"
            "testing box placement within limited boundary\n"
            "=============================================\n");

    for (placement = 0; placement <= lastplacementtype; ++placement)
    {
        char* pstr = freespace_placement_to_str(placement);
        MESSAGE("using %s placement...\n", pstr);
        free(pstr);

        if (test_box_placement_limited_boundary(fs, placement))
        {
            printf("\tfail\n");
        }
        else
            printf("\tpass\n");
    }

    /*---------------------------------------------------*/
end:

    MESSAGE("total freespace searches:%ld\n", total_searches);
    MESSAGE("total freespace removals:%ld\n", total_removals);

    freespace_free(fs);

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

    basebox box;

    freespace_clear(fs);

    for (x = 0; x < FSWIDTH - size; ++x)
    {
        for (y = 0; y < FSHEIGHT - size; ++y)
        {
            if (!box_set_coords(&box, x, y, size, size))
            {
                WARNING("invalid boundary coordinates "
                         "x:%d, y:%d, w:%d, h:%d", x, y, size, size);
                return -1;
            }

            total_searches++;

            if (!freespace_find(fs, &box, flags, size, size, &resx, &resy))
            {
                WARNING(   "area search for size %d failed in boundary "
                            "size %d at location x:%d, y:%d\n",
                            size, size, x, y);
                return -1;
            }

            if (resx != x || resy != y)
            {
                WARNING(   "area search for size %d returned incorrect "
                            "result location x:%d, y:%d\n",
                            size, resx, resy);
                WARNING(   "expected x:%d, y:%d\n", x, y);
                return -1;
            }
        }
    }

    return 0;
}


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

    basebox box;
    box_set_coords(&box, 0, 0, 128, 128);

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

            int xx = (flags & FSPLACE_ROW_SMART) ? x : y;
            int yy = (flags & FSPLACE_ROW_SMART) ? y : x;

            expx = (flags & FSPLACE_LEFT_TO_RIGHT)
                        ? xx * size
                        : FSWIDTH - xx * size - size;

            expy = (flags & FSPLACE_TOP_TO_BOTTOM)
                        ? yy * size
                        : FSHEIGHT - yy * size - size;

            total_searches++;

            if (!freespace_find(fs, &box, flags, size, size, &resx, &resy))
            {
                WARNING(   "area search for size %d failed at location "
                            " x:%d, y:%d\n", size, expx, expy);
                WARNING(" size:%d count:%d x:%d y:%d\n",
                            size, count, expx, expy);
                return -1;
            }

            total_removals++;

            freespace_remove(fs, resx, resy, size, size);

            if (resx != expx || resy != expy)
            {
                WARNING(   "size %d area found at x:%d, y:%d "
                            "but expected x:%d, y:%d\n",
                            size, resx, resy, expx, expy);
                WARNING(" size:%d count:%d x:%d y:%d\n",
                            size, count, x, y);
                return -1;
            }

        }
    }

    return 0;
}


/*  test_box_placement_and_removal
 *----------------------------------
 *  this test makes a series of free space search and removals and
 *  stores the location and dimensions of each. it then individually
 *  returns each stored area of used space before the final stage where
 *  it performs the original set of search and removals.
 *  the test is designed to detect errors in the code used to return
 *  used space to free space.
 *
 *  the test should not fail.
 */

int test_box_placement_and_removal(freespace* fs, int flags)
{
    int i, x[100], y[100], w[100], h[100];

    basebox box;
    box_set_coords(&box, 0, 0, FSWIDTH, FSHEIGHT);

    freespace_clear(fs);

    int maxboxes = (flags & FSPLACE_ROW_SMART) ? 100 : 98;

    for (i = 0; i < maxboxes; ++i)
    {
        w[i] = 1 + i % 25;
        h[i] = 1 + (10 + i) % 20;

        total_searches++;

        if (!freespace_find(fs, &box, flags, w[i], h[i], &x[i], &y[i]))
        {
            WARNING("area search %d for %d x %d failed\n", i, w[i], h[i]);
            return -1;
        }

        freespace_remove(fs, x[i], y[i], w[i], h[i]);
    }

    for (i = 0; i < maxboxes; ++i)
    {
        int tx, ty;

        freespace_add(fs, x[i], y[i], w[i], h[i]);

        total_searches++;

        if (!freespace_find(fs, &box, flags, w[i], h[i], &tx, &ty))
        {
            WARNING("repeat area search %d for %d x %d failed\n",
                            i, w[i], h[i]);
            return -1;
        }

        if (tx != x[i] || ty != y[i])
        {
            WARNING("repeat of area %d x %d search %d result x:%d, y:%d "
                     "turned up differing result x:%d, y:%d\n",
                        w[i], h[i], i, x[i], y[i], tx, ty);
            return -1;
        }

        total_removals++;

        freespace_remove(fs, x[i], y[i], w[i], h[i]);
    }

    return 0;
}


/*  test_box_placement_limited_boundary
 *---------------------------------------
 *  this test ensures that box placement within a boundary does not
 *  jut out.
 */
int test_box_placement_limited_boundary(freespace* fs, int flags)
{
    basebox box;

    int fsbs;
    int fsbs0 = 8, fsbs1 = 120;
    int bs = 4;


    for (fsbs = fsbs0; fsbs < fsbs1; fsbs++)
    {
        int i;
        int fsxy = (FSWIDTH - fsbs) / 2;
        int offset;

        if (bs < fsbs / 2)
            bs += 2;

        for (offset = 0; offset < fsxy * 2; offset++)
        {
            int filled = 0;
            int jut = 0;

            if (!box_set_coords(&box, offset, offset, fsbs, fsbs))
            {
                WARNING("invalid boundary coordinates "
                     "x:%d, y:%d, w:%d, h:%d\n", fsxy, fsxy, fsbs, fsbs);
                return -1;
            }

            freespace_clear(fs);

            while(!filled)
            {
                int resx = -1;
                int resy = -1;

                total_searches++;

                if (freespace_find(fs, &box, flags, bs, bs, &resx, &resy))
                {
                    total_removals++;

                    freespace_remove(fs, resx, resy, bs, bs);

                    if (resx + bs > offset + fsbs)
                    {
                        WARNING("placement x:%d, y:%d size:%d juts on"
                                 " rhs\n", resx, resy, bs);
                        WARNING("boundary x:%d, y:%d, size:%d\n",
                                    offset, offset, fsbs);
                        jut++;
                    }

                    if (resx < offset)
                    {
                        WARNING("placement x:%d, y:%d size:%d juts on"
                                " lhs\n", resx, resy, bs);
                        WARNING("boundary x:%d, y:%d, size:%d\n",
                                    offset, offset, fsbs);
                        jut++;
                    }

                    if (resy + bs > offset + fsbs)
                    {
                        WARNING("placement x:%d, y:%d size:%d juts on"
                                " bhs\n", resx, resy, bs);
                        WARNING("boundary x:%d, y:%d, size:%d\n",
                                    offset, offset, fsbs);
                        jut++;
                    }

                    if (resy < offset)
                    {
                        WARNING("placement x:%d, y:%d size:%d juts on"
                                " ths\n", resx, resy, bs);
                        WARNING("boundary x:%d, y:%d, size:%d\n",
                                    offset, offset, fsbs);
                        jut++;
                    }

                    if (jut)
                    {
                        MESSAGE("boundary size:%d box size:%d\n",fsbs,bs);
                        return -1;
                    }
                }
                else
                    ++filled;
            }
        }
    }

    return 0;
}
