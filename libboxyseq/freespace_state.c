#include "freespace_state.h"


//#define FSDEBUG

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

    printf(" ( %lu )\n", (unsigned long long)val);
}


struct freespace_state
{
    fsbuf_type buf[FSHEIGHT][FSBUFWIDTH];
};


freespace* freespace_new(void)
{
    freespace* fs = malloc(sizeof(*fs));

    if (!fs)
        return 0;

    if (FSBUFBITS == 1)
        WARNING("bit-size of freespace state array not defined!\n"
                "things won't work properly!\n");

    int y;

    DMESSAGE("creating freespace grid using %d %d bit integers\n",
                                      FSBUFWIDTH, FSBUFBITS);

    for (y = 0; y < FSHEIGHT; ++y)
        memset(&fs->buf[y][0], 0, sizeof(fsbuf_type) * FSBUFWIDTH);

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
        memset(&fs->buf[y][0], 0, sizeof(fsbuf_type) * FSBUFWIDTH);
}


static inline int x_to_index_offset(int x, int* offset)
{
    *offset = (FSBUFBITS - 1) - (x % FSBUFBITS);
    return x >> FSDIVSHIFT;
}


static bool fs_find_row_smart_l2r(fsbuf_type buf[FSHEIGHT][FSBUFWIDTH],
                                        fsbound* fsb,
                                        int flags,
                                        int width,      int height,
                                        int* resultx,   int* resulty    )
{
    /* handle vertical direction */
    bool t2b = (flags & FSPLACE_TOP_TO_BOTTOM);
    const int y0 =  t2b ? fsb->y : fsb->y + fsb->h - 1;
    const int y1 =  t2b ? fsb->y + fsb->h - height : fsb->y + height - 1;
    const int ydir = t2b ? 1 : -1;

    /* get offsets and indices of boundary */
    int x0offset = 0;
    int x1offset = 0;
    int x0index = x_to_index_offset(fsb->x, &x0offset);
    int x1index = x_to_index_offset(fsb->x + fsb->w - 1, &x1offset);

    /* offset and index of furthest point where placement may reach */
    int xwoffset = 0;
    int xwindex = x_to_index_offset(fsb->x + fsb->w - width, &xwoffset);

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



static bool fs_find_row_smart_r2l(fsbuf_type buf[FSHEIGHT][FSBUFWIDTH],
                                        fsbound* fsb,
                                        int flags,
                                        int width,      int height,
                                        int* resultx,   int* resulty    )
{
    /* handle vertical direction */
    bool t2b = (flags & FSPLACE_TOP_TO_BOTTOM);
    const int y0 =  t2b ? fsb->y : fsb->y + fsb->h - 1;
    const int y1 =  t2b ? fsb->y + fsb->h - height : fsb->y + height - 1;
    const int ydir = t2b ? 1 : -1;

    /* get offsets and indices of boundary */
    int x0offset = 0;
    int x1offset = 0;
    int x0index = x_to_index_offset(fsb->x + fsb->w - 1, &x0offset);
    int x1index = x_to_index_offset(fsb->x, &x1offset);

    /* offset and index of furthest point where placement may reach */
    int xwoffset = 0;
    int xwindex = x_to_index_offset(fsb->x + width - 1, &xwoffset);

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
                result_index = index;
                result_offset = offset;
                #ifdef FSDEBUG
                DMESSAGE("start scan index:%d offset:%d br:%d\n",
                            index,   offset, bits_remaining);
                #endif

            }
retry:
            if (bits_remaining < FSBUFBITS - offset)
            {   /* ie br = 4, offset = 2: 00111100 */
                mask = ((fsbuf_type)1 << bits_remaining) - 1 << offset;
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


static bool fs_find_col_smart(   fsbuf_type buf[FSHEIGHT][FSBUFWIDTH],
                                    fsbound* fsb,
                                    int flags,
                                    int width,      int height,
                                    int* resultx,   int* resulty    )
{
    bool t2b = !!(flags & FSPLACE_TOP_TO_BOTTOM);

    const int starty = t2b ? fsb->y : fsb->y + fsb->h - 1;
    const int endy = t2b ? fsb->y + fsb->h : fsb->y;
    const int diry = t2b ? 1 : -1;

    bool l2r = !!(flags & FSPLACE_LEFT_TO_RIGHT);

    int startx, endx;
    int startxoffset = 0;
    int endxoffset = 0;

    startx = (l2r)
            ? x_to_index_offset(fsb->x, &startxoffset)
            : x_to_index_offset(fsb->x + fsb->w - width, &startxoffset);

    endx = (l2r)
            ? x_to_index_offset(fsb->x + fsb->w - width, &endxoffset)
            : x_to_index_offset(fsb->x, &endxoffset);

    int x, y, y1;
    int w, h;
    int xoffset = startxoffset;
    int xofs;

    fsbuf_type mask[FSBUFWIDTH];

    bool scanning = false;

    *resultx = -1;
    *resulty = -1;

    int mx;
    int mxtestend = (l2r) ? endx : startx;
    int mendx;

    x = startx;

    while((l2r) ? x <= endx : x >= endx)
    {
        w = width;
        mx = x;
        xofs = xoffset;

        while (w >= 0 && mx <= mxtestend )
        {
            if (w < xofs)
                mask[mx] = (((fsbuf_type)1 << w) - 1) << (xofs - w);
            else if (xofs < FSBUFBITS)
                mask[mx] = ((fsbuf_type)1 << xofs) - 1;
            else
                mask[mx] = fsbuf_max;

            w -= xofs;
            xofs = FSBUFBITS;
            ++mx;
        }

        mendx = mx;

        if (w > 0)
            return false;

        scanning = false;

        for (y = starty; t2b ? y < endy : y >= endy; y += diry)
        {
            if (!scanning)
            {
                y1 = y;
                h = height;
                scanning = true;
            }

            for (mx = x; mx < mendx; ++mx)
            {
                if (buf[y][mx] & mask[mx])
                {
                    scanning = false;
                    break;
                }
                
            }

            if (!scanning)
                continue;

            --h;

            if (!h)
            {
                *resultx = x * FSBUFBITS + (FSBUFBITS - xoffset);
                *resulty = t2b ? y1 : y1 + 1 - height;

                return true;
            }
        }

        if (x == endx && xoffset == endxoffset)
            return false;

        if (l2r)
        {
            if (xoffset > 0)
                --xoffset;
            else
            {
                xoffset = FSBUFBITS;
                ++x;
            }
        }
        else
        {
            if (xoffset < FSBUFBITS)
                ++xoffset;
            else
            {
                xoffset = 0;
                --x;
            }
        }
    }
    return false;
}


bool freespace_find( freespace* fs,  fsbound* fsb,
                        int flags,
                        int width,      int height,
                        int* resultx,   int* resulty    )
{
    *resultx = *resulty = -1;

    if (width < 1 || width > fsb->w || height < 1 || height > fsb->h)
        return false;

    if (flags & FSPLACE_ROW_SMART)
    {
        if (flags & FSPLACE_LEFT_TO_RIGHT)
        {
            return fs_find_row_smart_l2r(   fs->buf, fsb, flags,
                                            width,   height,
                                            resultx, resulty );
        }
        else
        {
            return fs_find_row_smart_r2l(   fs->buf, fsb, flags,
                                            width,   height,
                                            resultx, resulty );
        }
    }
    else
    {
        return fs_find_col_smart(   fs->buf, fsb, flags,
                                    width,   height,
                                    resultx, resulty );
    }

    WARNING("unrecognized placement flags:%d\n", flags);
    return false;
}


void freespace_remove(freespace* fs,int x,
                                    int y0,
                                    int width,
                                    int height )
{
    int offset = 0;
    int index = x_to_index_offset(x, &offset);
#ifdef FSDEBUG
DMESSAGE("x:%d y:%d, width:%d, height:%d\n",x,y0,width,height);
DMESSAGE("index:%d offset:%d\n",index,offset);
#endif
    for (; width > 0 && index < FSBUFWIDTH; ++index)
    {
        int y;
        fsbuf_type v;
#ifdef FSDEBUG
DMESSAGE("looping!:width:%d\n",width);
#endif
        if (width < offset + 1)
        {
            v = (((fsbuf_type)1 << width) - 1) << (offset - width + 1);
        }
        else
        {
            v = (((fsbuf_type)1 << offset) - 1) << 1 | 1;
        }
#ifdef FSDEBUG
binary_dump("v:\n",v);
#endif
        for (y = y0; y < y0 + height; ++y)
            fs->buf[y][index] |= v;

        width -= offset + 1;
        offset = FSBUFBITS - 1;
    }
}


void freespace_add( freespace* fs,  int x,
                                    int y0,
                                    int width,
                                    int height )
{
    int offset = 0;
    int index = x_to_index_offset(x, &offset);

    for (; width > 0 && index < FSBUFWIDTH; ++index)
    {
        int y;
        fsbuf_type v;

        if (width <= offset + 1)
        {
            v = (((fsbuf_type)1 << width) - 1) << (offset - width + 1);
        }
        else
        {
            v = (((fsbuf_type)1 << offset) - 1) << 1 | 1;
        }

        for (y = y0; y < y0 + height; ++y)
            fs->buf[y][index] &= ~v;

        width -= offset + 1;
        offset = FSBUFBITS - 1;
    }
}


bool freespace_test( freespace* fs, int state,
                                    int x,      int y1,
                                    int width,  int height )
{
    int xoffset = 0;
    x = x_to_index_offset(x, &xoffset);

    fsbuf_type v;
    int y;

    for (; width > 0 && x < FSBUFWIDTH; ++x)
    {
        if (width < xoffset)
            v = (((fsbuf_type)1 << width) - 1) << (xoffset - width);
        else if (xoffset < FSBUFBITS)
            v = ((fsbuf_type)1 << xoffset) - 1;
        else
            v = fsbuf_max;

        for (y = y1; y < y1 + height; ++y)
        {
            if (!!(fs->buf[y][x] & v) == state)
                return false;
        }

        if (width < xoffset)
            return true;

        width -= xoffset;
        xoffset = FSBUFBITS;
    }

    return true;
}


void freespace_dump(freespace* fs)
{
    int x, y;

    for (y = 0; y < FSHEIGHT; ++y)
    {
        for (x = 0; x < FSBUFWIDTH; ++x)
        {
            fsbuf_type i = FSBUFBITS;
            fsbuf_type b = fs->buf[y][x];

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

