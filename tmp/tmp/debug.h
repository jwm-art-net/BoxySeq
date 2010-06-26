/*
    gratiously copied:  from debug.h & debug.c
                        from non-sequencer
                        (C) 2008 Jonathan Moore Liles
    -jwm 2010
*/

#ifndef _DEBUG_H
#define _DEBUG_H

#include <stddef.h>
#include <stdio.h>

#ifndef __GNUC__
    #define __FUNCTION__ NULL
#endif


typedef enum
{
    W_MESSAGE = 0,
    W_WARNING,
} warning_t;


void
warnf ( warning_t level,
        const char *file,
        const char *function, size_t line, const char *fmt, ... );


#ifndef NDEBUG
#define DMESSAGE( fmt, ... ) \
    warnf( W_MESSAGE, __FILE__, __FUNCTION__, __LINE__, fmt, ## args )

#define DWARNING( fmt, ... ) \
    warnf( W_WARNING, __FILE__, __FUNCTION__, __LINE__, fmt, ## args )

#else

#define DMESSAGE( fmt, ... )
#define DWARNING( fmt, ... )

#endif

/* these are always defined */
#define MESSAGE( fmt, args... ) \
    warnf( W_MESSAGE,__FILE__, __FUNCTION__, __LINE__, fmt, ## args )

#define WARNING( fmt, args... ) \
    warnf( W_WARNING,__FILE__, __FUNCTION__, __LINE__, fmt, ## args )

#endif
