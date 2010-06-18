#include "freespace_state.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>     /* malloc */
#include <stdio.h>      /* putchar, snprintf */
#include <string.h>     /* memset */


#ifdef USE_64BIT_ARRAY
    #define FSBUFBITS 64
    #define FSBUFWIDTH 2
    #define FSDIVSHIFT 6
    typedef uint64_t fsbuf_type;
    #define TRAILING_ONES( v )  __builtin_ctzl(~( v ))
    #define TRAILING_ZEROS( v ) __builtin_ctzl( ( v ))
    #define LEADING_ONES( v )   __builtin_clzl(~( v ))
    #define LEADING_ZEROS( v )  __builtin_clzl( ( v ))
#else
#ifdef USE_32BIT_ARRAY
    #define FSBUFBITS 32
    #define FSBUFWIDTH 4
    #define FSDIVSHIFT 5
    typedef uint32_t fsbuf_type;
    #define TRAILING_ONES( v )  __builtin_ctz(~( v ))
    #define TRAILING_ZEROS( v ) __builtin_ctz( ( v ))
    #define LEADING_ONES( v )   __builtin_clz(~( v ))
    #define LEADING_ZEROS( v )  __builtin_clz( ( v ))
#else
#ifdef USE_16BIT_ARRAY
    #define FSBUFBITS 16
    #define FSBUFWIDTH 8
    #define FSDIVSHIFT 4
    typedef uint16_t fsbuf_type;
    #define TRAILING_ONES( v )  __builtin_ctz(~(0xffff0000 | ( v )))
    #define TRAILING_ZEROS( v ) __builtin_ctz(  0xffff0000 | ( v ))
    #define LEADING_ONES( v )   __builtin_clz(~( v ) << 16)
    #define LEADING_ZEROS( v )  __builtin_clz( ( v ) << 16)
#else
#ifdef USE_8BIT_ARRAY
    #define FSBUFBITS 8
    #define FSBUFWIDTH 16
    #define FSDIVSHIFT 3
    typedef uint8_t fsbuf_type;
    #define TRAILING_ONES( v )  __builtin_ctz(~(0xffffff00 | ( v )))
    #define TRAILING_ZEROS( v ) __builtin_ctz(  0xffffff00 | ( v ))
    #define LEADING_ONES( v )   __builtin_clz(~( v ) << 24)
    #define LEADING_ZEROS( v )  __builtin_clz( ( v ) << 24)
#else
    #define FSBUFBITS 1
    #define FSBUFWIDTH 128
    #define FSDIVSHIFT 0
    typedef unsigned char fsbuf_type;
    #define TRAILING_ONES(  v ) (( v ) ? 1 : 0)
    #define TRAILING_ZEROS( v ) (( v ) ? 0 : 1)
    #define LEADING_ONES( v )   (( v ) ? 1 : 0)
    #define LEADING_ZEROS( v )  (( v ) ? 0 : 1)
#endif
#endif
#endif
#endif


static const fsbuf_type fsbuf_max =   ~(fsbuf_type)0;
static const fsbuf_type fsbuf_high =  (fsbuf_type)1 << (FSBUFBITS - 1);


#ifdef FREESPACE_DEBUG
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
#endif


struct freespace_state
{
    fsbuf_type buf[FSHEIGHT][FSBUFWIDTH];
};


freespace* freespace_new(void)
{
    freespace* fs = malloc(sizeof(*fs));

    if (!fs)
        return 0;

    int y;

    for (y = 0; y < FSHEIGHT; ++y)
    {
        memset(&fs->buf[y][0], 0, sizeof(fsbuf_type) * FSBUFWIDTH);
    }

    return fs;
}


void freespace_free(freespace* fs)
{
    if (!fs)
        return;

    free(fs);
}

/*
static void fs_set_buffer(  fsbuf_type buf[FSHEIGHT][FSBUFWIDTH],
                            int x,
                            int y1,
                            int xoffset,
                            int width,
                            int height)
{
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
#ifdef FREESPACE_DEBUG0
            if (buf[y][x] & v)
                printf("**** over-writing area ****\n");
#endif
            buf[y][x] |= v;
        }

        if (width < xoffset)
            return;

        width -= xoffset;
        xoffset = FSBUFBITS;
    }
}
*/


static inline int fs_x_to_offset_index(int x, int* d)
{
    *d = (x >> FSDIVSHIFT);
    return FSBUFBITS - (x % FSBUFBITS);
}


static _Bool fs_find_row_smart_l2r(fsbuf_type buf[FSHEIGHT][FSBUFWIDTH],
                                        fsbound* bound,
                                        int flags,
                                        int width,      int height,
                                        int* resultx,   int* resulty    )
{
    int bx, by, bw, bh;

    fsbound_get_coords(bound, &bx, &by, &bw, &bh);

    if (width > bw || height > bh)
        return false;

    _Bool t2b = !!(flags & FSPLACE_TOP_TO_BOTTOM);

    const int starty =  t2b ? by : by + bh - 1;
    const int endy =    t2b ? by + bh - height : by + height - 1;
    const int diry =    t2b ? 1 : -1;

    int startx;
    int startxoffset = fs_x_to_offset_index(bx, &startx);

    int endx;
    int endxoffset  = fs_x_to_offset_index(bx + bw - width, &endx);

    int x, x1, y;
    int w, h;
    int xoffset, x1offset;

    fsbuf_type* xptr;
    fsbuf_type mask =   0;
    fsbuf_type v;

    _Bool scanning = false;

    for (y = starty; t2b ? y <= endy : y >= endy; y += diry)
    {
        scanning = false;
        xptr = &buf[y][startx];

        for (x = startx; x <= endx; ++x, ++xptr)
        {
            if(*xptr == fsbuf_max)
            {
                scanning = false;
                continue;
            }

            if (!scanning)
            {
                scanning = true;
                x1 = x;
                x1offset = xoffset = (x == startx)
                                     ? startxoffset
                                     : FSBUFBITS;
                w = width;
            }
retry:
            if (x >= endx && xoffset < endxoffset)
                break;

            if (w < xoffset)
                mask = (((fsbuf_type)1 << w) - 1) << (xoffset - w);
            else if (xoffset < FSBUFBITS)
                mask = ((fsbuf_type)1 << xoffset) - 1;
            else
                mask = fsbuf_max;

            for (h = 0; h < height; ++h)
            {
                if ((v = buf[y + h * diry][x] & mask))
                {
                    if ((xoffset = TRAILING_ZEROS(v)))
                    {
                        x1 = x;
                        x1offset = xoffset;
                        w = width;
                        goto retry;
                    }

                    scanning = false;
                    break;
                }
            }

            if (!scanning)
                continue;

            if (w <= xoffset) // ***** RESULT! *****
            {
                *resultx = x1 * FSBUFBITS + (FSBUFBITS - x1offset);
                *resulty = t2b ? y : y + 1 - height;
/*
                fs_set_buffer(buf, x1, *resulty, x1offset,
                                       width, height);
*/
                return true;
            }

            w -= xoffset;
            xoffset = FSBUFBITS;

        }
    }
    return false;
}


static _Bool fs_find_row_smart_r2l(fsbuf_type buf[FSHEIGHT][FSBUFWIDTH],
                                        fsbound* bound,
                                        int flags,
                                        int width,      int height,
                                        int* resultx,   int* resulty    )
{
    int bx, by, bw, bh;

    fsbound_get_coords(bound, &bx, &by, &bw, &bh);

    if (width > bw || height > bh)
        return false;

    _Bool t2b = !!(flags & FSPLACE_TOP_TO_BOTTOM);

    const int starty =  t2b ? by : by + bh - 1;
    const int endy =    t2b ? by + bh - height : by + height - 1;
    const int diry =    t2b ? 1 : -1;

    int startx;
    int startxoffset = fs_x_to_offset_index(bx + bw - 1, &startx) -1;

    int endx;
    int endxoffset = fs_x_to_offset_index(bx + width, &endx);

    int x, y;
    int w, h;
    int xoffset, rxoffset;

    fsbuf_type* xptr;
    fsbuf_type mask =   0;
    fsbuf_type v;

    _Bool scanning = false;

    for (y = starty; t2b ? y <= endy : y >= endy; y += diry)
    {
        scanning = false;
        xptr = &buf[y][startx];

        for (x = startx; x >= endx; --x, --xptr)
        {
            if(*xptr == fsbuf_max)
            {
                scanning = false;
                continue;
            }

            if (!scanning)
            {
                scanning = true;
                rxoffset = xoffset = (x == startx)
                                     ? startxoffset
                                     : 0;

                w = width;
            }
retry:
            xoffset = rxoffset + w;

            if (x == endx && rxoffset > endxoffset)
                break;

            if (xoffset >= FSBUFBITS)
            {
                xoffset = FSBUFBITS;
                mask = fsbuf_max << rxoffset;
            }
            else
                mask = (((fsbuf_type)1 << w) - 1) << rxoffset;

            for (h = 0; h < height; ++h)
            {
                if ((v = buf[y + h * diry][x] & mask))
                {
                    rxoffset = FSBUFBITS - LEADING_ZEROS(v);

                    if (rxoffset < FSBUFBITS)
                    {
                        w = width;
                        goto retry;
                    }

                    scanning = false;
                    break;
                }
            }

            if (!scanning)
                continue;

            if (w <= FSBUFBITS - rxoffset)
            {
                *resultx = x * FSBUFBITS + FSBUFBITS - xoffset;
                *resulty = t2b ? y : y + 1 - height;
/*
                fs_set_buffer(buf, x, *resulty, xoffset,
                                            width, height);
*/
                return true;
            }

            w -= xoffset - rxoffset;
            rxoffset = 0;
        }
    }
    return false;
}


static _Bool fs_find_col_smart(   fsbuf_type buf[FSHEIGHT][FSBUFWIDTH],
                                    fsbound* bound,
                                    int flags,
                                    int width,      int height,
                                    int* resultx,   int* resulty    )
{
    int bx, by, bw, bh;

    fsbound_get_coords(bound, &bx, &by, &bw, &bh);

    if (width > bw || height > bh)
        return false;

    _Bool t2b = !!(flags & FSPLACE_TOP_TO_BOTTOM);

    const int starty =  t2b ? by : by + bh - 1;
    const int endy =    t2b ? by + bh : by;
    const int diry =    t2b ? 1 : -1;

    _Bool l2r = !!(flags & FSPLACE_LEFT_TO_RIGHT);

    int startx;
    int startxoffset = (l2r)
                        ? fs_x_to_offset_index(bx, &startx)
                        : fs_x_to_offset_index(bx + bw - width, &startx);

    int endx;
    int endxoffset = (l2r)
                        ? fs_x_to_offset_index(bx + bw - width, &endx)
                        : fs_x_to_offset_index(bx, &endx);

    int x, y, y1;
    int w, h;
    int xoffset = startxoffset;
    int xofs;

    fsbuf_type mask[FSBUFWIDTH];

    _Bool scanning = false;

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
/*
                fs_set_buffer(buf, x, *resulty, xoffset,
                                          width, height);
*/
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


_Bool freespace_find( freespace* fs,  fsbound* bound,
                        int flags,
                        int width,      int height,
                        int* resultx,   int* resulty    )
{
    *resultx = *resulty = -1;

    if (flags & FSPLACE_ROW_SMART)
    {
        if (flags & FSPLACE_LEFT_TO_RIGHT)
            return fs_find_row_smart_l2r(   fs->buf, bound, flags,
                                            width,   height,
                                            resultx, resulty );
        else
            return fs_find_row_smart_r2l(   fs->buf, bound, flags,
                                            width,   height,
                                            resultx, resulty );
    }
    else
    {
        return fs_find_col_smart(   fs->buf, bound, flags,
                                    width,   height,
                                    resultx, resulty );
    }

    return false;
}


void freespace_remove(freespace* fs,int x,
                                    int y1,
                                    int width,
                                    int height )
{
    int xoffset = fs_x_to_offset_index(x, &x);

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
            fs->buf[y][x] |= v;
        }

        if (width < xoffset)
            return;

        width -= xoffset;
        xoffset = FSBUFBITS;
    }
}


void freespace_add( freespace* fs,  int x,
                                    int y1,
                                    int width,
                                    int height )
{
    int xoffset = fs_x_to_offset_index(x, &x);

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
            fs->buf[y][x] &= ~v;
        }

        if (width < xoffset)
            return;

        width -= xoffset;
        xoffset = FSBUFBITS;
    }
}

#ifdef FREESPACE_DEBUG
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
/*
            if (x + 1 < FSBUFWIDTH)
                putchar('|');
*/
        }
        putchar('\n');
    }
}
#endif
