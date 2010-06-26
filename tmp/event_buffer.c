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

    event*   buf;
    event*   bufend;

    event*   head;   /* write ptr */
    event*   tail;   /* read ptr */

    size_t  items;

    char*   name;   /* yes we're naming ring buffers */
};


evbuf* evbuf_new(size_t count)
{
    evbuf* rb = malloc(sizeof(*rb));

    if (!rb)
    {
        WARNING("out of memory for evbuf\n");
        return 0;
    }

    rb->count = count;

    rb->buf = malloc(sizeof(*rb->buf) * count);

    if (!rb->buf)
    {
        WARNING("out of memory for evbuf buffer\n");
        free(rb);
        return 0;
    }

    rb->bufend = rb->buf + rb->count - 1;
    rb->head = rb->buf;
    rb->tail = rb->buf;
    rb->items = 0;

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
    return rb->count - rb->items;
}


event* evbuf_write(evbuf* rb, const event* data)
{
    if (rb->items == rb->count)
        return 0;

    event* ret = rb->head;

    memcpy(rb->head, data, sizeof(event));

    if (rb->head == rb->bufend)
        rb->head = rb->buf;
    else
        ++rb->head;

    ++rb->items;

    return ret;
}


size_t evbuf_read_count(const evbuf* rb)
{
    return rb->items;
}


event* evbuf_read(evbuf* rb, event* data)
{
    if (rb->items == 0)
        return 0;

    event* ret = rb->tail;

    memcpy(data, rb->tail, sizeof(event));

    if (rb->tail == rb->bufend)
        rb->tail = rb->buf;
    else
        ++rb->tail;

    --rb->items;

    return ret;
}


void evbuf_reset(evbuf* rb)
{
    rb->head = rb->buf;
    rb->tail = rb->buf;
    rb->items = 0;
}


void evbuf_dump(const evbuf* rb)
{
    MESSAGE("evbuf:%p name:%s items:%d\n", rb, rb->name, rb->items);

    MESSAGE("max items %d\n", rb->count);
    MESSAGE("buf:%p bufend:%p head:%p tail:%p\ndata:\n",
            rb->buf, rb->bufend, rb->head, rb->tail);

    size_t items = rb->items;

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

