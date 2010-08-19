#ifndef SCALE_H
#define SCALE_H


#define scale_note_is_valid( scale_binary, key, note ) \
    ( (scale_binary) & (1 << (12 - (( (note) + (key) ) % 12))))


int             note_number(const char*);
const char*     note_name(int);
int             note_to_octave(int);

typedef struct scale_list sclist;
typedef struct musical_scale scale;


scale*      scale_new(const char* name, const char* binary);
void        scale_free(scale*);
scale*      scale_dup(const scale*);
scale*      scale_copy(scale* dest, const scale* src);
int         scale_cmp(const scale*, const scale*);

const char* scale_name(const scale*);
const char* scale_as_binary_string(const scale*);
int         scale_as_int(const scale*);
void        scale_as_rgb(const scale*, double* r, double* g, double* b);

const char* scale_name_set(scale*, const char*);
int         scale_set_by_binary_string(scale*, const char*);
int         scale_set_by_int(scale*, int);

sclist*     sclist_new(void);
void        sclist_free(sclist*);
void        sclist_add_default_scales(sclist*);

scale*      sclist_add(sclist*, const char* name, const char* binary);

scale*      sclist_scale_by_binary(sclist*, int);
scale*      sclist_scale_by_binary_string(sclist*, const char*);
scale*      sclist_scale_by_name(sclist*, const char*);


#endif
