#include "common.h"

#include "debug.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
const int internal_ppqn = 24;
const int internal_ppqn = 720720;     it's important to have 1/4 notes
                                      evenly divisible by 16 isn't it?
                                      ...but not that important.
*/


const int internal_ppqn = 2520;     /* divisible by 1-10 */


inline void bbtpos_copy(bbtpos* dest, const bbtpos* src)
{
    dest->bar =     src->bar;
    dest->beat =    src->beat;
    dest->tick =    src->tick;
}

const char* string_set(char** str_ptr, const char* new_str)
{
    size_t n = strlen(new_str);
    char* str = malloc(n + 1);

    if (!str)
    {
        WARNING("failed to set string.\n");
        return 0;
    }

    strcpy(str, new_str);

    if (*str_ptr)
        free(*str_ptr);

    return (*str_ptr = str);
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


int binary_string_to_int(const char* bstr)
{
    int n = 0;

    while(*bstr)
    {
        n <<= 1;
        switch(*bstr)
        {
            case '0':           break;
            case '1':   ++n;    break;
            default:    return -1;
        }
        ++bstr;
    }

    return n;
}


char* int_to_binary_string(int n, int sigbits)
{
    int max;
    char* p;
    char* bstr;

    if (sigbits < 0)
        return 0;

    if (!(bstr = malloc((size_t)sigbits + 1)))
        return 0;

    p = bstr;
    max = 1 << (sigbits ? sigbits - 1 : 0);

    while(sigbits)
    {
        *p++ = (n & max) ? '1' : '0';
        n <<= 1;
        --sigbits;
    }

    *p = '\0';

    return bstr;
}


int lcg_rand(int *x, int max)
{
    int a = 134775813;
    int c = 1;
    int m = 4294967296;
    *x = ((*x * a) + c) % m;
    return (max) ? *x % max : x;
}


void random_rgb(unsigned char* r, unsigned char* g, unsigned char* b)
{
    char n = rand() % 6;

    char high = 205 + rand() % 50;
    char mid = 100 + rand() % 25;
    char dark = rand() % 85;

    switch(n)
    {
    case 0: *r = high;  *g = mid;   *b = dark;  break;
    case 1: *r = mid;   *g = dark;  *b = high;  break;
    case 2: *r = dark;  *g = high;  *b = mid;   break;
    case 3: *r = high;  *g = dark;  *b = mid;   break;
    case 4: *r = dark;  *g = mid;   *b = high;  break;
    case 5: *r = mid;   *g = high;  *b = dark;  break;
    }
}

