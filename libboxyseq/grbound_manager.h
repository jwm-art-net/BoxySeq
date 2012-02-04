#ifndef GRBOUND_MANAGER_H
#define GRBOUND_MANAGER_H


#ifdef __cplusplus
extern "C" {
#endif


#include "grid_boundary.h"


typedef struct grbound_manager grbound_manager;


grbound_manager*    grbound_manager_new(void);
void                grbound_manager_free(grbound_manager*);

grbound*    grbound_manager_grbound_new(grbound_manager*);
void        grbound_manager_grbound_free(grbound_manager*, grbound*);

grbound*    grbound_manager_grbound_first(grbound_manager*);
grbound*    grbound_manager_grbound_next(grbound_manager*);

void        grbound_manager_grbound_order(grbound_manager*,
                                            grbound*,
                                            int dir);

void    grbound_manager_update_rt_data(const grbound_manager*);

void    grbound_manager_rt_pull_starting(grbound_manager*,
                                            evport* grid_intersort);


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
