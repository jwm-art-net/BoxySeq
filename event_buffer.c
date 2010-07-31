#include "event_buffer.h"

#include "common.h"
#include "debug.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int evbuf_id = 0;


struct event_buffer
{
    size_t  count;
    size_t  count_mask;

    event*  buf;
    event*  bufend;

    event*  head;   /* write ptr */
    event*  tail;   /* read ptr */

    char*   name;   /*  yes we're naming ring buffers...
                        are we?
                        do we still need to do this?
                    */
};


evbuf* evbuf_new(size_t count)
{
    size_t sz = 1;
    evbuf* rb = malloc(sizeof(*rb));

    if (!rb)
    {
        WARNING("out of memory for evbuf\n");
        return 0;
    }

    for (sz = 1; sz < count; sz <<= 1)
        ;

    rb->count = sz;
    rb->count_mask = sz - 1;

    rb->buf = malloc(sizeof(*rb->buf) * rb->count);

    if (!rb->buf)
    {
        WARNING("out of memory for evbuf buffer\n");
        free(rb);
        return 0;
    }

    rb->bufend = rb->buf + rb->count - 1;
    rb->head = rb->buf;
    rb->tail = rb->buf;

    rb->name = name_and_number("buffer", ++evbuf_id);

    return rb;
}


void evbuf_free(evbuf* rb)
{
    free(rb->buf);
    free(rb->name);
    free(rb);
}


size_t evbuf_write_count(const evbuf* rb)
{
    size_t s,e,w,r;

    w = (size_t)rb->head;
    r = (size_t)rb->tail;
    s = (size_t)rb->buf;
    e = (size_t)rb->bufend;

    if (w > r)
        return ((e - w) + (r - s)) - 1;

    if (w < r)
        return (r - w) - 1;

    return rb->count - 1;
}


event* evbuf_write(evbuf* rb, const event* data)
{
    event* ret = rb->head;

    if (!evbuf_write_count(rb))
        return 0;

    memcpy(rb->head, data, sizeof(event));

    if (rb->head == rb->bufend)
        rb->head = rb->buf;
    else
        ++rb->head;

    return ret;
}


size_t evbuf_read_count(const evbuf* rb)
{
    size_t s,e,w,r;

    w = (size_t)rb->head;
    r = (size_t)rb->tail;
    s = (size_t)rb->buf;
    e = (size_t)rb->bufend;

    if (w == r)
        return 0;

    if (w > r)
        return w - r;

    return ((e - r) + (w - s)) - 1;
}


event* evbuf_read(evbuf* rb, event* data)
{
    event* ret = rb->tail;

    if (!evbuf_read_count(rb))
        return 0;

    memcpy(data, rb->tail, sizeof(event));

    if (rb->tail == rb->bufend)
        rb->tail = rb->buf;
    else
        ++rb->tail;

    return ret;
}


void evbuf_reset(evbuf* rb)
{ /* hmmm, not thread safe this, is it? */
    rb->head = rb->buf;
    rb->tail = rb->buf;
}


void evbuf_dump(const evbuf* rb)
{
    size_t items = evbuf_read_count(rb);

    MESSAGE("evbuf:%p name:%s items:%d\n", rb, rb->name, items);

    MESSAGE("max items %d\n", rb->count);
    MESSAGE("buf:%p bufend:%p head:%p tail:%p\ndata:\n",
            rb->buf, rb->bufend, rb->head, rb->tail);

    if (!items)
    {
        MESSAGE("empty\n");
        return;
    }

    event* tail = rb->tail;

    while(items)
    {
        event_dump(tail);

        if (tail == rb->bufend)
            tail = rb->buf;
        else
            ++tail;

        --items;
    }

}

