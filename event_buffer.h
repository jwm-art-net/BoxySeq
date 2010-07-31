#ifndef EVENT_BUFFER_H
#define EVENT_BUFFER_H

#include "event.h"

#include <stddef.h>


typedef struct event_buffer evbuf;

evbuf*  evbuf_new(size_t count);
void    evbuf_free(evbuf*);

void    evbuf_reset(evbuf*);


/*  evbuf_write(evbuf*, const event*)   returns pointer to buffer item
                                        event parameter was copied to.
                                        or NULL if buffer full.
*/
event*  evbuf_write(evbuf*, const event*);
size_t  evbuf_write_count(const evbuf*);


/*  evbuf_read(evbuf*, event*)  returns pointer to item in buffer,
                                and writes copy to pointed to event
                                parameter. returns NULL, and does
                                not copy to param, if nothing to read.
*/
event*  evbuf_read(evbuf*, event*);
size_t  evbuf_read_count(const evbuf*);


void    evbuf_dump(const evbuf*);

#endif
