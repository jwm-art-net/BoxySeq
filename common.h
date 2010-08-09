#ifndef COMMON_H
#define COMMON_H


#include <stdint.h>

#define MAX_NAME_LEN 32


#define MAX_PATTERN_SLOTS 16
#define MAX_GRBOUND_SLOTS 16
#define MAX_MOPORT_SLOTS 16

#define DEFAULT_EVBUF_SIZE 256
#define DEFAULT_EVPOOL_SIZE 1024


typedef enum random_seed     /* random seed: */
{
    SEED_TIME_SYS,      /* seed using system time            */
    SEED_TIME_TICKS,    /* seed using total ticks            */
    SEED_USER,          /* seed using a user specified value */
    SEED_NONE           /* don't seed RNG system */

} seed_type;


typedef int32_t bbt_t;


typedef struct
{
    bbt_t   bar;
    bbt_t   beat;
    bbt_t   tick;

} bbtpos;


typedef struct
{
    int x;
    int y;

} coord;


void    bbtpos_copy(bbtpos* dest, const bbtpos* src);


const char* string_set(char** str_ptr, const char* new_str);
char*       name_and_number(const char* name, int number);

char*   jwm_strcat_alloc(const char* str1, const char* str2);


int     binary_string_to_int(const char*);
char*   int_to_binary_string(int, int sigbits);


/* 2520 gives int result for div by 2 ... 9 */
extern const int internal_ppqn;


#endif

