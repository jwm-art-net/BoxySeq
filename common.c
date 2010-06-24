#include "common.h"

#include "debug.h"


#include <stdio.h>
#include <string.h>


const int ppqn = 2520;


inline void bbtpos_copy(bbtpos* dest, const bbtpos* src)
{
    dest->bar =     src->bar;
    dest->beat =    src->beat;
    dest->tick =    src->tick;
}


char* name_and_number(const char* name, int number)
{
    char buf[MAX_NAME_LEN];

    if (snprintf(buf, MAX_NAME_LEN,
                 "%s #%d", name, number)
             >   MAX_NAME_LEN)
    {
        buf[MAX_NAME_LEN - 1] = '\0';
    }

    char* ret = strdup(buf);

    if (!ret)
        WARNING("error creating name and number (%s %d)\n", name, number);

    return ret;
}


char* jwm_strcat_alloc(const char* str1, const char* str2)
{
    size_t l1 = strlen(str1);
    size_t l2 = strlen(str2);

    char* str = malloc(l1 + l2 + 1);

    if (!str)
        return 0;

    strcpy(str, str1);
    strcpy(str + l1, str2); 

    return str;
}

