#ifndef PATTERN_MANAGER_H
#define PATTERN_MANAGER_H


#include "pattern.h"


typedef struct pattern_manager pattern_manager;


pattern_manager*    pattern_manager_new(void);
void                pattern_manager_free(pattern_manager*);

pattern*    pattern_manager_pattern_new(pattern_manager*);
void        pattern_manager_pattern_free(pattern_manager*, pattern*);

pattern*    pattern_manager_pattern_first(pattern_manager*);
pattern*    pattern_manager_pattern_next(pattern_manager*);

void    pattern_manager_update_rt_data(const pattern_manager*);

void    pattern_manager_rt_play(    pattern_manager*,
                                    _Bool repositioned,
                                    bbt_t ph,
                                    bbt_t nph );

#endif
