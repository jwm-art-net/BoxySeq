#include "freespace_state.h"

/*  #define FSDEBUG to enable debugging infomation dumps of the freespace
    state. if a problem is found that isn't covered within ../freespace_test
    then add a comprehensive test there for it. then, setup the freespace
    state with as few steps as possible to recreate the problem (ie replace
    many search-and-removals with one or two removals and then perform the
    search/remove which fails with FSDEBUG enabled to see what is
    happening...

#define FSDEBUG
*/


#include "include/freespace_boundary_data.h"


#include "debug.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>     /* malloc */
#include <stdio.h>      /* putchar, snprintf */
#include <string.h>     /* memset */


#ifdef USE_64BIT_ARRAY
    #error  use of USE_64BIT_ARRAY is broken.
    #error  use USE_32BIT_ARRAY instead.
    #define FSBUFBITS 64
    #define FSBUFWIDTH 2
    #define FSDIVSHIFT 6
    typedef uint64_t fsbuf_type;
    #define TRAILING_ZEROS( v ) __builtin_ctzl( ( v ))
    #define LEADING_ZEROS( v )  __builtin_clzl( ( v ))
#else
#ifdef USE_32BIT_ARRAY
    #define FSBUFBITS 32
    #define FSBUFWIDTH 4
    #define FSDIVSHIFT 5
    typedef uint32_t fsbuf_type;
    #define TRAILING_ZEROS( v ) __builtin_ctz( ( v ) )
    #define LEADING_ZEROS( v )  __builtin_clz( ( v ) )
#else
#ifdef USE_16BIT_ARRAY
    #define FSBUFBITS 16
    #define FSBUFWIDTH 8
    #define FSDIVSHIFT 4
    typedef uint16_t fsbuf_type;
    #define TRAILING_ZEROS( v ) __builtin_ctz(  0xffff0000 | ( v ))
    #define LEADING_ZEROS( v )  __builtin_clz( ( v ) << 16)
#else
#ifdef USE_8BIT_ARRAY
    #define FSBUFBITS 8
    #define FSBUFWIDTH 16
    #define FSDIVSHIFT 3
    typedef uint8_t fsbuf_type;
    #define TRAILING_ZEROS( v ) __builtin_ctz(  0xffffff00 | ( v ))
    #define LEADING_ZEROS( v )  __builtin_clz( ( v ) << 24)
#else
    /* Sloooowwwwww */
    #define FSBUFBITS 1
    #define FSBUFWIDTH 128
    #define FSDIVSHIFT 0
    typedef unsigned char fsbuf_type;
    #define TRAILING_ZEROS( v ) (( v ) ? 0 : 1)
    #define LEADING_ZEROS( v )  (( v ) ? 0 : 1)
#endif
#endif
#endif
#endif


static const fsbuf_type fsbuf_max =   ~(fsbuf_type)0;
static const fsbuf_type fsbuf_high =  (fsbuf_type)1 << (FSBUFBITS - 1);


static void binary_dump(const char* msg, fsbuf_type val)
{
    fsbuf_type i = FSBUFBITS;
    fsbuf_type b = val;

    printf("%20s ", msg);

    for (; i > 0; --i)
    {
        putchar((b & fsbuf_high) ? '1' : '0');
        b <<= 1;
    }

    printf(" ( %lu )\n", (unsigned long)val);
}


struct freespace_state
{
    /*  the freespace state is primarily stored in a pair of arrays. only a
        single array is required, but by optimizing with the usage of bits
        we are optimizing only for row-smart searches. the second array
        is a 90deg clockwise rotation of the first and is used for
        performing column smart searchs.
    */
    fsbuf_type row_buf[FSHEIGHT][FSBUFWIDTH];
    fsbuf_type col_buf[FSHEIGHT][FSBUFWIDTH];

    /*  so two arrays are sufficient until you realize that there are some
        cases where overlap is very definitely required (think user
        interaction via a gui). the block-areas must be allowed to overlap
        existing used space when they are put down. so two more arrays are
        needed for masking between the two types of used space:
        block-areas and "natural" areas. and with the added rotation, that
        extra two arrays becomes four.
    */
    fsbuf_type blk_row_buf[FSHEIGHT][FSBUFWIDTH];
    fsbuf_type blk_col_buf[FSHEIGHT][FSBUFWIDTH];

    fsbuf_type nat_row_buf[FSHEIGHT][FSBUFWIDTH];
    fsbuf_type nat_col_buf[FSHEIGHT][FSBUFWIDTH];

    /*  and we don't want removal of a block from a pair of intersecting
        blocks to damage the remaining block so we store the coordinates.
        because this must all be real-time safe we limit the number of
        block areas - the user can realistically manage only so many anyway.
    */
    struct blk
    {
        int x, y, w, h;
        struct blk* n;
    } blklist[MAX_BLOCK_AREAS];
};


freespace* freespace_new(void)
{
    freespace* fs = malloc(sizeof(*fs));

    if (!fs)
        return 0;

    if (FSBUFBITS == 1)
        WARNING("bit-size of freespace state array not defined!\n"
                "things won't work properly!\n");

    DMESSAGE("creating freespace grid using %d %d bit integers\n",
                                      FSBUFWIDTH, FSBUFBITS);
    freespace_clear(fs);
    freespace_block_clear(fs);

    DMESSAGE("sizeof(freespace_state):%d\n", sizeof(*fs));

    return fs;
}


void freespace_free(freespace* fs)
{
    if (!fs)
        return;

    free(fs);
}


void freespace_clear(freespace* fs)
{
    int y;

    for (y = 0; y < FSHEIGHT; ++y)
    {
        memset(&fs->row_buf[y][0], 0, sizeof(fsbuf_type) * FSBUFWIDTH);
        memset(&fs->col_buf[y][0], 0, sizeof(fsbuf_type) * FSBUFWIDTH);
    }
}


/* in accordance with LSB0 - least significant bit is numbered bit 0 */
static inline int x_to_index_offset(int x, int* offset)
{
    *offset = (FSBUFBITS - 1) - (x % FSBUFBITS);
    return x >> FSDIVSHIFT;
}


static int row_smart_l2r(fsbuf_type buf[FSHEIGHT][FSBUFWIDTH],
                        int bx, int by, int bw, int bh, int flags,
                        int width, int height, int *resultx, int *resulty)
{
    /* handle vertical direction */
    bool t2b = (flags & FSPLACE_TOP_TO_BOTTOM);
    const int y0 =  t2b ? by : by + bh - 1;
    const int y1 =  t2b ? by + bh - height : by + height - 1;
    const int ydir = t2b ? 1 : -1;

    /* get offsets and indices of boundary */
    int x0offset = 0;
    int x1offset = 0;
    int x0index = x_to_index_offset(bx, &x0offset);
    int x1index = x_to_index_offset(bx + bw - 1, &x1offset);

    /* offset and index of furthest point where placement may reach */
    int xwoffset = 0;
    int xwindex = x_to_index_offset(bx + bw - width, &xwoffset);

    /*  note: the index (horizontal) loop must test against the boundary
     *  as the loop is a state-machine with two states:
     *
     *      1) non-scanning: placement is decisivly impossible at the
     *                      location (index, offset). when in this state,
     *                      the loop should break out past (xwindex,
     *                      xwoffset) ie the last position of possible
     *                      placement.
     *
     *      2) scanning:    placement is possible at the location (index,
     *                      offset) and scanning is underway for an area
     *                      width x height. in this case the loop may need
     *                      to go right to the boundary edge (x1index,
     *                      x1offset).
     */

    int result_index;
    int result_offset;

    int y;

    #ifdef FSDEBUG
    DMESSAGE("--------------------------------------------\n"
             "x0 index:%d offset:%d\nx1 index:%d offset:%d\n",
                x0index, x0offset, x1index, x1offset);
    DMESSAGE("\nxw index:%d offset:%d\n", xwindex, xwoffset);
    DMESSAGE("\ny0: %d y1:%d ydir:%d\n",y0,y1,ydir);
    #endif

    for (y = y0; t2b ? y <= y1 : y >= y1; y += ydir)
    {
        int index;
        int offset = 0;
        int bits_remaining = 0;
        bool scanning = false;

        for (index = x0index; index <= x1index; ++index)
        {
            fsbuf_type mask = 0;
            int h;

            if (!scanning) /* check enough room remains for area at */
            {              /* (index, offset) (see note above) */
                if (index > xwindex)
                {
                    #ifdef FSDEBUG
                    DMESSAGE("index > xwindex index:%d\n", index);
                    #endif
                    break;
                }

                offset = (index == x0index) ? x0offset
                                            : FSBUFBITS - 1;

                if (index == xwindex && offset < xwoffset)
                {
                    #ifdef FSDEBUG
                    DMESSAGE("no room remains at index:%d offset:%d\n",
                                                    index, offset);
                    #endif
                    break;
                }
            }

            if (buf[y][index] == fsbuf_max)
            {
                scanning = false;
                continue;
            }

            if (!scanning)
            {
                scanning = true;
                bits_remaining = width;
                result_index = index;
                result_offset = offset;
                #ifdef FSDEBUG
                DMESSAGE("start scan index:%d offset:%d br:%d\n",
                            index,   offset, bits_remaining);
                #endif
            }
retry:
            /* set a mask for the current index collumn */
            if (bits_remaining < offset + 1)
            {   /* ie br = 4, offset = 5: 00111100 */
                mask = (((fsbuf_type)1 << bits_remaining) - 1)
                                       << (offset - bits_remaining + 1);
                #ifdef FSDEBUG
                DMESSAGE("mask... index:%d offset:%d br:%d\n",
                                        index,   offset, bits_remaining);
                binary_dump("br <= offset + 1 mask:", mask);
                #endif
            }
            else
            {   /* ie br 4, offset 2: 00000111 */
                mask = (((fsbuf_type)1 << offset) - 1) << 1 | 1;
                #ifdef FSDEBUG
                DMESSAGE("mask... index:%d offset:%d br:%d\n",
                                        index,   offset, bits_remaining);
                binary_dump("br > offset + 1 mask:", mask);
                #endif
            }

            for (h = 0; h < height; ++h)
            {
                int v = buf[y + h * ydir][index] & mask;

                if (v)
                {
                    #ifdef FSDEBUG
                    binary_dump("           mask:", mask);
                    binary_dump("obstructed by v:", v);
                    #endif
                    if ((offset = TRAILING_ZEROS(v)))
                    {
                        /* TZ needs 1 subtracting to become offset */
                        --offset;

                        if (!(index > xwindex)
                         && !(index == xwindex && offset < xwoffset))
                        {
                            result_index = index;
                            result_offset = offset;
                            bits_remaining = width;
                            #ifdef FSDEBUG
                            DMESSAGE("rety: index:%d offset:%d br:%d\n",
                                        index,   offset, bits_remaining);
                            #endif
                            goto retry;
                        }
                    }
                    #ifdef FSDEBUG
                    DMESSAGE("breaking vertical scan\n");
                    #endif
                    scanning = false;
                    break;
                }
            }

            if (!scanning)
                continue;
            #ifdef FSDEBUG
            DMESSAGE("before result test:bits_remaining:%d offset:%d\n",
                                        bits_remaining, offset);
            #endif
            if (bits_remaining <= offset + 1) /***** RESULT!! *****/
            {
                #ifdef FSDEBUG
                DMESSAGE("result index:%d offset:%d\n", result_index,
                                                        result_offset);
                #endif
                *resultx = result_index * FSBUFBITS
                                        + FSBUFBITS - 1 - result_offset;
                *resulty = t2b ? y : y + 1 - height;
                return true;
            }

            bits_remaining -= offset + 1;
            offset = FSBUFBITS - 1;
            #ifdef FSDEBUG
            DMESSAGE("after result test bits_remaining:%d offset:%d\n",
                                        bits_remaining, offset);
            #endif
        }
    }
    #ifdef FSDEBUG
    DMESSAGE("no free space\n");
    #endif
    return false;
}


static int row_smart_r2l(fsbuf_type buf[FSHEIGHT][FSBUFWIDTH],
                        int bx, int by, int bw, int bh, int flags,
                        int width, int height, int *resultx, int *resulty)
{
    /* handle vertical direction */
    bool t2b = (flags & FSPLACE_TOP_TO_BOTTOM);
    const int y0 =  t2b ? by : by + bh - 1;
    const int y1 =  t2b ? by + bh - height : by + height - 1;
    const int ydir = t2b ? 1 : -1;

    /* get offsets and indices of boundary */
    int x0offset = 0;
    int x1offset = 0;
    int x0index = x_to_index_offset(bx + bw - 1, &x0offset);
    int x1index = x_to_index_offset(bx, &x1offset);

    /* offset and index of furthest point where placement may reach */
    int xwoffset = 0;
    int xwindex = x_to_index_offset(bx + width - 1, &xwoffset);

    /*  note: the index (horizontal) loop must test against the boundary
     *  as the loop is a state-machine with two states:
     *
     *      1) non-scanning: placement is decisivly impossible at the
     *                      location (index, offset). when in this state,
     *                      the loop should break out past (xwindex,
     *                      xwoffset) ie the last position of possible
     *                      placement.
     *
     *      2) scanning:    placement is possible at the location (index,
     *                      offset) and scanning is underway for an area
     *                      width x height. in this case the loop may need
     *                      to go right to the boundary edge (x1index,
     *                      x1offset).
     */

    int y;

    #ifdef FSDEBUG
    DMESSAGE("--------------------------------------------\n"
             "x0 index:%d offset:%d\nx1 index:%d offset:%d\n",
                x0index, x0offset, x1index, x1offset);
    DMESSAGE("\nxw index:%d offset:%d\n", xwindex, xwoffset);
    DMESSAGE("\ny0: %d y1:%d ydir:%d\n",y0,y1,ydir);
    #endif

    for (y = y0; t2b ? y <= y1 : y >= y1; y += ydir)
    {
        int index;
        int offset = 0;
        int bits_remaining = 0;
        int scanning = false;

        #ifdef FSDEBUG
        DMESSAGE("\nline:%d\n",y);
        #endif

        for (index = x0index; index >= x1index; --index)
        {
            fsbuf_type mask = 0;
            int h;

            if (!scanning) /* check enough room remains for area at */
            {              /* (index, offset) (see note above) */
                if (index < xwindex)
                {
                    #ifdef FSDEBUG
                    DMESSAGE("index:%d < xwindex\n", index);
                    #endif
                    break;
                }

                offset = (index == x0index) ? x0offset
                                            : 0;

                if (index == xwindex && offset > xwoffset)
                {
                    #ifdef FSDEBUG
                    DMESSAGE("no room remains at index:%d offset:%d\n",
                                                    index, offset);
                    #endif
                    break;
                }
            }

            if (buf[y][index] == fsbuf_max)
            {
                scanning = false;
                continue;
            }

            if (!scanning)
            {
                scanning = true;
                bits_remaining = width;
                #ifdef FSDEBUG
                DMESSAGE("start scan index:%d offset:%d br:%d\n",
                            index,   offset, bits_remaining);
                #endif

            }
retry:
            if (bits_remaining < FSBUFBITS - offset)
            {   /* ie br = 4, offset = 2: 00111100 */
                mask = (((fsbuf_type)1 << bits_remaining) - 1) << offset;
                #ifdef FSDEBUG
                DMESSAGE("mask... index:%d offset:%d br:%d\n",
                                        index,   offset, bits_remaining);
                binary_dump("br <= FSBITS - offset mask:", mask);
                #endif
            }
            else
            {   /* ie br = 8, offset = 2: 11111100 */
                mask = fsbuf_max << offset;
                #ifdef FSDEBUG
                DMESSAGE("mask... index:%d offset:%d br:%d\n",
                                        index,   offset, bits_remaining);
                binary_dump("br > FSBITS - offset mask:", mask);
                #endif
            }

            for (h = 0; h < height; ++h)
            {
                int v = buf[y + h * ydir][index] & mask;

                if (v)
                {
                    #ifdef FSDEBUG
                    binary_dump("obstructed by v:", v);
                    #endif

                    offset = FSBUFBITS - LEADING_ZEROS(v);

                    if (offset < FSBUFBITS)
                    {
                        if (!(index < xwindex)
                         && !(index == xwindex && offset > xwoffset))
                        {
                            bits_remaining = width;
                            #ifdef FSDEBUG
                            DMESSAGE("rety: index:%d offset:%d br:%d\n",
                                        index,   offset, bits_remaining);
                            #endif
                            goto retry;
                        }
                    }
                    #ifdef FSDEBUG
                    DMESSAGE("breaking vertical scan\n");
                    #endif
                    scanning = false;
                    break; /* break to continue */
                }
            }

            if (!scanning)
                continue;

            if (bits_remaining <= FSBUFBITS - offset) /***** RESULT!! *****/
            {
                #ifdef FSDEBUG
                DMESSAGE("result index:%d offset:%d\n", index,
                                                        offset);
                #endif
                *resultx = index * FSBUFBITS +  (FSBUFBITS - offset)
                                                    - bits_remaining;
                *resulty = t2b ? y : y + 1 - height;
                return true;
            }

            bits_remaining -= FSBUFBITS - offset;
            offset = 0;
        }
    }

    #ifdef FSDEBUG
    DMESSAGE("no free space\n");
    #endif

    return false;
}


bool freespace_find( freespace* fs,  basebox* boundary,
                        int flags,
                        int width,      int height,
                        int* resultx,   int* resulty    )
{
    *resultx = *resulty = -1;

    #ifdef FSDEBUG
    char* pstr = freespace_placement_to_str(flags);
    DMESSAGE("\n---------------------------\n"
             "flags:%s\tarea w:%d, h:%d\n"
             "boundary x:%d,y:%d, w:%d, h:%d\n", pstr, width, height,
             boundary->x, boundary->y, boundary->w, boundary->h);
    #endif

    if (width  < 1 || width  > boundary->w
     || height < 1 || height > boundary->h)
    {
        #ifdef FSDEBUG
        DWARNING("\ninvalid area or area does not fit in boundary\n");
        #endif
        return false;
    }

    if (flags & FSPLACE_ROW_SMART)
    {
        if (flags & FSPLACE_LEFT_TO_RIGHT)
        {
            return row_smart_l2r(fs->row_buf,
                                    boundary->x, boundary->y,
                                    boundary->w, boundary->h,
                                    flags,
                                    width, height, resultx, resulty);
        }
        else
        {
            return row_smart_r2l(fs->row_buf,
                                    boundary->x, boundary->y,
                                    boundary->w, boundary->h,
                                    flags,
                                    width, height, resultx, resulty);
        }
    }
    else
    {
        /*  the row smart algorithm can be used for column-smart by
            rotating 90 degrees clockwise...
        */
        bool ret;

        int rx = FSWIDTH - boundary->y - boundary->h;
        int ry = boundary->x;

        int rflags = (flags & FSPLACE_LEFT_TO_RIGHT)
                            ? FSPLACE_TOP_TO_BOTTOM
                            : 0;

        rflags |= (flags & FSPLACE_TOP_TO_BOTTOM)
                            ? 0
                            : FSPLACE_LEFT_TO_RIGHT;

        #ifdef FSDEBUG
        pstr = freespace_placement_to_str(rflags);
        DMESSAGE("\n---------------------------\n"
             "rflags:%s\tarea w:%d, h:%d\n"
             "boundary rx:%d, ry:%d, w:%d, h:%d\n", pstr, width, height,
             rx, ry, boundary->w, boundary->h);
        #endif


        if (rflags & FSPLACE_LEFT_TO_RIGHT)
        {
            ret = row_smart_l2r(fs->col_buf,
                                    rx,     ry,
                                    boundary->h, boundary->w,
                                    rflags,
                                    height, width, resultx, resulty);
        }
        else
        {
            ret = row_smart_r2l(fs->col_buf,
                                    rx,     ry,
                                    boundary->h, boundary->w,
                                    rflags,
                                    height, width, resultx, resulty);
        }

        if (!ret)
            return false;

        /* translate result back by 'rotation' 90 anti-clockwise */

        rx = *resultx;
        *resultx = *resulty;
        *resulty = FSWIDTH - rx - height;

        return true;
    }

    WARNING("unrecognized placement flags:%d\n", flags);
    return false;
}


/*  mark_used marks an area in the freespace grid as used space. it must
    operate on two or three arrays.
 */
static void mark_used(  fsbuf_type buf_row[FSHEIGHT][FSBUFWIDTH],
                        fsbuf_type buf_col[FSHEIGHT][FSBUFWIDTH],
                        fsbuf_type aux_row[FSHEIGHT][FSBUFWIDTH],
                        fsbuf_type aux_col[FSHEIGHT][FSBUFWIDTH],
                        int x0, int y0, int w0, int h0 )
{
    int rx, ry;
    int width = w0;
    int height = h0;
    int offset = 0;
    int index = x_to_index_offset(x0, &offset);

    #ifdef FSDEBUG
    DMESSAGE("x:%d y:%d, width:%d, height:%d\n",x0,y0,width,height);
    DMESSAGE("index:%d offset:%d\n",index,offset);
    #endif

    for (; width > 0 && index < FSBUFWIDTH; ++index)
    {
        int y;
        fsbuf_type v;

        if (width < offset + 1)
            v = (((fsbuf_type)1 << width) - 1) << (offset - width + 1);
        else
            v = (((fsbuf_type)1 << offset) - 1) << 1 | 1;

        for (y = y0; y < y0 + height; ++y)
        {
            buf_row[y][index] |= v;
            aux_row[y][index] |= v;
        }

        width -= offset + 1;
        offset = FSBUFBITS - 1;
    }

    width = h0;
    height = w0;
    offset = 0;
    rx = FSWIDTH - y0 - h0;
    ry = x0;
    index = x_to_index_offset(rx, &offset);

    #ifdef FSDEBUG
    DMESSAGE("rx:%d ry:%d, width:%d, height:%d\n",rx,ry,width,height);
    DMESSAGE("index:%d offset:%d\n",index,offset);
    #endif

    for (; width > 0 && index < FSBUFWIDTH; ++index)
    {
        int y;
        fsbuf_type v;

        if (width < offset + 1)
            v = (((fsbuf_type)1 << width) - 1) << (offset - width + 1);
        else
            v = (((fsbuf_type)1 << offset) - 1) << 1 | 1;

        for (y = ry; y < ry + height; ++y)
        {
            buf_col[y][index] |= v;
            aux_col[y][index] |= v;
        }

        width -= offset + 1;
        offset = FSBUFBITS - 1;
    }
}


static void mark_unused(fsbuf_type buf_row[FSHEIGHT][FSBUFWIDTH],
                        fsbuf_type buf_col[FSHEIGHT][FSBUFWIDTH],
                        fsbuf_type aux_row[FSHEIGHT][FSBUFWIDTH],
                        fsbuf_type aux_col[FSHEIGHT][FSBUFWIDTH],
                        fsbuf_type msk_row[FSHEIGHT][FSBUFWIDTH],
                        fsbuf_type msk_col[FSHEIGHT][FSBUFWIDTH],
                        int x0, int y0, int w0, int h0 )
{
    int rx, ry;
    int width = w0;
    int height = h0;
    int offset = 0;
    int index = x_to_index_offset(x0, &offset);

    for (; width > 0 && index < FSBUFWIDTH; ++index)
    {
        int y;
        fsbuf_type v;

        if (width <= offset + 1)
            v = ~((((fsbuf_type)1 << width) - 1) << (offset - width + 1));
        else
            v = ~((((fsbuf_type)1 << offset) - 1) << 1 | 1);

        for (y = y0; y < y0 + height; ++y)
        {
            buf_row[y][index] = (buf_row[y][index] & v) | msk_row[y][index];
            aux_row[y][index] &= v;
        }

        width -= offset + 1;
        offset = FSBUFBITS - 1;
    }

    width = h0;
    height = w0;
    offset = 0;
    rx = FSWIDTH - y0 - h0;
    ry = x0;

    index = x_to_index_offset(rx, &offset);

    for (; width > 0 && index < FSBUFWIDTH; ++index)
    {
        int y;
        fsbuf_type v;

        if (width <= offset + 1)
            v = ~((((fsbuf_type)1 << width) - 1) << (offset - width + 1));
        else
            v = ~((((fsbuf_type)1 << offset) - 1) << 1 | 1);

        for (y = ry; y < ry + height; ++y)
        {
            buf_col[y][index] = (buf_col[y][index] & v) | msk_col[y][index];
            aux_col[y][index] &= v;
        }

        width -= offset + 1;
        offset = FSBUFBITS - 1;
    }
}


void freespace_remove(freespace* fs, int x0, int y0, int w0, int h0)
{
    mark_used(  fs->row_buf,        fs->col_buf,
                fs->nat_row_buf,    fs->nat_col_buf,
                x0,     y0,     w0,     h0);
}


void freespace_add(freespace* fs, int x0, int y0, int w0, int h0 )
{
    mark_unused(fs->row_buf,        fs->col_buf,
                fs->nat_row_buf,    fs->nat_col_buf,
                fs->blk_row_buf,    fs->blk_col_buf,
                x0,     y0,     w0,     h0);
}


void freespace_block_clear(freespace* fs)
{
    int y;

    for (y = 0; y < FSHEIGHT; ++y)
    {
        memset(&fs->blk_row_buf[y][0], 0, sizeof(fsbuf_type) * FSBUFWIDTH);
        memset(&fs->blk_col_buf[y][0], 0, sizeof(fsbuf_type) * FSBUFWIDTH);
        memset(&fs->nat_row_buf[y][0], 0, sizeof(fsbuf_type) * FSBUFWIDTH);
        memset(&fs->nat_col_buf[y][0], 0, sizeof(fsbuf_type) * FSBUFWIDTH);
    }

    for (y = 0; y < MAX_BLOCK_AREAS; ++y)
        fs->blklist[y].w = 0;
}


bool freespace_block_remove(freespace* fs, int x0, int y0, int w0, int h0)
{
    int i;

    for (i = 0; i < MAX_BLOCK_AREAS; ++i)
    {
        if (fs->blklist[i].w == 0)
        {
            fs->blklist[i].x = x0;
            fs->blklist[i].y = y0;
            fs->blklist[i].w = w0;
            fs->blklist[i].h = h0;

            mark_used(  fs->row_buf,        fs->col_buf,
                        fs->blk_row_buf,    fs->blk_col_buf,
                        x0,     y0,     w0,     h0);

            return true;
        }
    }

    return false;
}

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define FSMINUNDEFINED
#endif

#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define FSMAXUNDEFINED
#endif

void freespace_block_add(freespace* fs, int x0, int y0, int w0, int h0 )
{
    int i;

    for (i = 0; i < MAX_BLOCK_AREAS; ++i)
    {
        /* search for existing block which matches */

        if (fs->blklist[i].w
         && fs->blklist[i].x == x0
         && fs->blklist[i].y == y0
         && fs->blklist[i].w == w0
         && fs->blklist[i].h == h0)
        {
            int j;

            fs->blklist[i].w = 0;

            /* set the area to unused space */

            mark_unused(fs->row_buf,        fs->col_buf,
                        fs->blk_row_buf,    fs->blk_col_buf,
                        fs->nat_row_buf,    fs->nat_col_buf,
                        x0,     y0,     w0,     h0);

            /* check for intersections with other blocks */

            for (j = 0; j < MAX_BLOCK_AREAS; ++j)
            {
                if (fs->blklist[j].w == 0)
                    continue;

                if (j == i)
                    continue; /* __ */
                {
                    int ix0 = MAX(fs->blklist[j].x, x0);
                    int iy0 = MAX(fs->blklist[j].y, y0);
                    int ix1 = MIN(x0 + w0,  fs->blklist[j].x
                                          + fs->blklist[j].w);
                    int iy1 = MIN(y0 + h0,  fs->blklist[j].y
                                          + fs->blklist[j].h);

                    if (ix1 > ix0 && iy1 > iy0)
                    {
                        mark_used(  fs->row_buf,        fs->col_buf,
                                    fs->blk_row_buf,    fs->blk_col_buf,
                                    ix0,    iy0,    ix1 - ix0,  iy1 - iy0);
                    }
                }
            }
        }
    }
}

#ifdef FSMINUNDEFINED
#undef MIN
#undef FSMINUNDEFINED
#endif

#ifdef FSMAXUNDEFINED
#undef MAX
#undef FSMAXUNDEFINED
#endif


void freespace_dump(freespace* fs, int buf)
{
    int x, y;

    for (x = 0; x < FSBUFWIDTH; ++x)
    {
        if (FSBUFBITS > 8)
        {
            int i;
            for (i = FSBUFBITS - 1; i > 12; --i)
                putchar(' ');

            printf("...98");
        }
        printf("76543210%c", x < FSBUFWIDTH - 1 ? '_' : '\n');
    }

    for (y = 0; y < FSHEIGHT; ++y)
    {
        for (x = 0; x < FSBUFWIDTH; ++x)
        {
            fsbuf_type b;
            fsbuf_type i = FSBUFBITS;

            switch(buf)
            {
            case 1:     b = fs->col_buf[y][x];      break;
            case 2:     b = fs->blk_row_buf[y][x];  break;
            case 3:     b = fs->blk_col_buf[y][x];  break;
            case 4:     b = fs->nat_row_buf[y][x];  break;
            case 5:     b = fs->nat_col_buf[y][x];  break;
            default:    b = fs->row_buf[y][x];      break;
            }

            for(; i != 0; --i, b <<= 1)
                putchar(b & fsbuf_high ? '#' : ' ');

            if (x + 1 < FSBUFWIDTH)
                putchar('|');

        }
        putchar('\n');
    }
}


char* freespace_placement_to_str(int flags)
{
    char* org = (flags & FSPLACE_ROW_SMART)
                    ? "row-smart" :  "collumn-smart";

    char* horiz = (flags & FSPLACE_LEFT_TO_RIGHT)
                    ? "left-to-right" : "right-to-left";
                    
    char* vert = (flags & FSPLACE_TOP_TO_BOTTOM)
                    ? "top-to-bottom" : "bottom-to-top";

    char buf[80];

    snprintf(buf, 79, "%s, %s, %s",   org, horiz, vert);

    return strdup(buf);
}

