#include "musical_scale.h"

#include "common.h"
#include "debug.h"
#include "llist.h"

#include <stdlib.h>
#include <string.h>


static const char* note_names[] = {
    "C",
    "C#",
    "D",
    "D#",
    "E",
    "F",
    "F#",
    "G",
    "G#",
    "A",
    "A#",
    "B"
};


int note_number(const char* name)
{
    const char* p;
    const char* chromatic_names = "C C#D D#E F F#G G#A A#B ";
    size_t n = strlen(name);

    if (!n || n > 2 || *name == ' ')
        return -1;

    if (!(p = strstr(chromatic_names, name)))
        return -1;

    return ((int)(p - chromatic_names) / 2);
}


const char* note_name(int n)
{
    return note_names[n % 12];
}


int note_to_octave(int n)
{
    return n / 12;
}


struct musical_scale
{
    char* name;
    char* bstr;
    int binary;
};


scale* scale_new(const char* name, const char* bstr)
{
    scale* sc = malloc(sizeof(*sc));

    if (!sc)
        goto fail0;

    sc->name = 0;

    if (!string_set(&sc->name, name))
        goto fail1;

    sc->bstr = 0;

    if (scale_set_by_binary_string(sc, bstr) < 0)
        goto fail2;

    return sc;

fail2:
    free(sc->name);
fail1:
    free(sc);
fail0:
    WARNING("out of memory for new scale\n");
    return 0;
}


void scale_free(scale* sc)
{
    if (!sc)
        return;

    free(sc->bstr);
    free(sc->name);
    free(sc);
}


scale* scale_dup(const scale* src)
{
    return scale_new(src->name, src->bstr);
}


scale* scale_copy(scale* dest, const scale* src)
{
    scale_name_set(dest, src->name);
    scale_set_by_binary_string(dest, src->bstr);
    return dest;
}


int scale_cmp(const scale* sc1, const scale* sc2)
{
    if (sc1->binary < sc2->binary)
        return -1;
    if (sc1->binary > sc2->binary)
        return 1;
    return 0;
}



const char* scale_name(const scale* sc)
{
    return sc->name;
}

const char* scale_as_binary_string(const scale* sc)
{
    return sc->bstr;
}

int scale_as_int(const scale* sc)
{
    return sc->binary;
}

const char* scale_name_set(scale* sc, const char* name)
{
    return string_set(&sc->name, name);
}

int scale_set_by_binary_string(scale* sc, const char* bstr)
{
    int n = binary_string_to_int(bstr);

    if (n < 0)
    {
        WARNING("invalid binary scale string\n");
        return -1;
    }

    string_set(&sc->bstr, bstr);

    return (sc->binary = n);
}

int scale_set_by_int(scale* sc, int n)
{
    char* bstr = int_to_binary_string(n, 12);

    if (!bstr)
        return -1;

    if (sc->bstr)
        free(sc->bstr);

    sc->bstr = bstr;

    return n;
}



/*-----------------------*/
/* scale llist callbacks */
/*-----------------------*/

static void scale_free_cb(void* data)
{
    scale_free(data);
}

static void* scale_dup_cb(const void* data, size_t n)
{
    return scale_dup(data);
}

static void* scale_copy_cb(void* dest, const void* src, size_t n)
{
    return scale_copy(dest, src);
}

static int scale_cmp_cb(const void* d1, const void* d2, size_t n)
{
    return scale_cmp(d1, d2);
}


static char* scale_str_cb(const void* data, int level)
{
    const scale* sc = data;
    return jwm_strcat_alloc(sc->name, sc->bstr);
}

struct scale_list
{
    llist* scales;
};


sclist* sclist_new(void)
{
    sclist* scl = malloc(sizeof(*scl));

    if (!scl)
        goto fail0;

    scl->scales = llist_new(    sizeof(scale),
                                scale_free_cb,
                                scale_dup_cb,
                                scale_copy_cb,
                                scale_cmp_cb,
                                scale_str_cb    );
    if (!scl->scales)
        goto fail1;

    return scl;

fail1:
    free(scl);
fail0:
    WARNING("out of memory for new scale list\n");
    return 0;
}


void sclist_free(sclist* scl)
{
    llist_free(scl->scales);
    free(scl);
}


void sclist_add_default_scales(sclist* scl)
{
    const char* scale_strs[][2] = {
        { "Chromatic",          "111111111111" },
        { "Major",              "101011010101" },
        { "Natural Minor",      "101101011010" },
        { "Harmonic Minor",     "101101011001" },
        { "Melodic Minor",      "101101010101" },
        { "Major Pentatonic",   "101010010100" },
        { "Minor Pentatonic",   "100101010010" },
        { "Whole Tone",         "101010101010" },
        { "Dorian",             "101101010110" },
        { "Phrygian",           "110101011010" },
        { "Lydian",             "101010110101" },
        { "Mixolydian",         "101011010110" },
        { "Locrian",            "110101101010" },
        { "Diminished",         "110110110110" },
        { "Augmented",          "101010100110" },
        { 0,                    0              }
    };

    int n = 0;

    while(scale_strs[n][0])
    {
        if (!sclist_add(scl, scale_strs[n][0], scale_strs[n][1]))
            WARNING("failed adding default scale '%s' '%s'\n",
                    scale_strs[n][0], scale_strs[n][1]);
        ++n;
    }
}


scale* sclist_add(sclist* scl, const char* name, const char* bstr)
{
    scale* sc = scale_new(name, bstr);

    if (!sc)
        return 0;

    if (!llist_add_data(scl->scales, sc))
    {
        scale_free(sc);
        return 0;
    }

    return sc;
}


scale* sclist_scale_by_name(sclist* scl, const char* name)
{
    scale* sc;
    lnode* ln = llist_head(scl->scales);

    if (!ln)
        return 0;

    while (ln)
    {
        sc = lnode_data(ln);

        if (strcmp(sc->name, name) == 0)
            return sc;

        ln = lnode_next(ln);
    }

    return 0;
}


scale* sclist_scale_by_binary_string(sclist* scl, const char* bstr)
{
    scale* sc;
    lnode* ln = llist_head(scl->scales);

    if (!ln)
        return 0;

    while (ln)
    {
        sc = lnode_data(ln);

        if (strcmp(sc->bstr, bstr) == 0)
            return sc;

        ln = lnode_next(ln);
    }

    return 0;
}


scale* sclist_scale_by_binary(sclist* scl, int n)
{
    scale* sc;
    lnode* ln = llist_head(scl->scales);

    if (!ln)
        return 0;

    while (ln)
    {
        sc = lnode_data(ln);

        if (sc->binary == n)
            return sc;

        ln = lnode_next(ln);
    }

    return 0;
}


/*
    { "Aeolian",            { 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1 
????
   aeolian.push_back(2);
   aeolian.push_back(2);
   aeolian.push_back(2);
   aeolian.push_back(2);
   aeolian.push_back(1);
   aeolian.push_back(2);
   aeolian.push_back(2);
*/

/*
scale scales[] = {
    { "Major",              7,  { 0, 2, 4, 5, 7, 9, 11 }},
    { "Natural Minor",      7,  { 0, 2, 3, 5, 7, 8, 10 }},
    { "Harmonic Minor",     7,  { 0, 2, 3, 5, 7, 8, 11 }},
    { "Melodic Minor",      7,  { 0, 2, 3, 5, 7, 9, 11 }},
    { "Major Pentatonic",   5,  { 0, 2, 4, 7, 9 }},
    { "Minor Pentatonic",   5,  { 0, 3, 5, 7, 10 }},
    { "Chromatic",          12, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }}
};
*/


