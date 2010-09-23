/************************************************************************
 ************************************************************************

    llist - a doubly linked list implementation

 ************************************************************************
 ************************************************************************

 ** llist is designed for containing one single data type per list.

 ** uses callbacks to perform copy/duplicate operations.
 ** uses callbacks to perform comparison operations.
 ** uses callbacks to perform selection operations.
 ** uses callbacks to perform editing operations.
 ** uses callbacks to perform free'ing of data items.

 ** so all data within a list must be of the same type which the
    callbacks are expecting and of the same data size.

 ** the llist is not designed to accomodate arrays, that is, you should
    not add an array directly to the list via llist_add_data by passing
    a pointer to the array. if you need to create linked lists of arrays
    then you should wrap the array in a struct.

 ** when creating a new llist, you must pass in callbacks for freeing
    data, duplicating data, comparing data, etc. In some cases you may
    use standard C system calls such as free, memcpy, memcmp. This is
    not possible for duplicating a data item however and as such, llist
    provides a helper function for this purpose: llist_datacb_dup.

 ** the only time checks for callbacks are performed is during the
    creation of a linked list via llist_new - if you pass in NULL for a
    callback and then use functionality which expects it, a segmentation
    fault WILL occurr.

 ** when adding data items to the llist via a pointer to the item passed
    to llist_add_data, the pointer must point to a dynamically allocated
    object otherwise a segmentation fault WILL occurr.

 ** recommended approach is to wrap the llist structure within a hidden
    data struct and create functions specific to the data type for
    creating the list, adding data, freeing data etc. this way, the
    actual list is not available for mis-use, and the functions to create
    a list of a given data type will always set the llist up in a uniform
    manner.

    (c) 2010 James W. Morris - GNU GPL
             http://jwm-art.net/


 ************************************************************************/

#ifndef LLIST_H
#define LLIST_H


#ifdef __cplusplus
extern "C" {
#endif


#include "datacb.h"


#include <stddef.h>
#include <stdbool.h>


typedef struct lnode lnode;
typedef struct llist llist;


/* unused yet
typedef enum llfindflags
{
    LLFIND_FWD =    0x0001,
    LLFIND_REV =    0x0002,

    LLFIND_LT =     0x0010,
    LLFIND_GT =     0x0020,
    LLFIND_EQ =     0x0030

} llfindflags;
*/


/************************************************************************
    lnode
 ************************************************************************/

void*   lnode_data(const lnode*);
lnode*  lnode_prev(const lnode*);
lnode*  lnode_next(const lnode*);

void    lnode_select(lnode*);
void    lnode_unselect(lnode*);
bool    lnode_selected(const lnode*);

/*  lnode_dump dumps contents of an lnode, prefixed by msg string
    and suffixed by the string returned by the datacb_str_cb callback.
    level is how verbose the callback should be. 0 none, 1 a little, etc.
*/
void    lnode_dump(const char* msg, const lnode*,
                   datacb_str,      int level);


/************************************************************************
    llist
 ************************************************************************/

/*  llist_new
    creates a linked list data structure. many operations on llist
    assume data added to the list actually is of 'data_size' size
    as passed to llist_new. many of these operations will use the
    callbacks passed to llist_new to perform copy/duplicate/compare
    operations and for string representation. if the copy or duplicate
    callbacks are NULL, llist assumes use of memcpy. 
*/

llist*  llist_new(  size_t data_size,
                    datacb_free,    // only if data needs ptrs free'd
                    datacb_dup,     // only if data cannot be memcpy'd
                    datacb_copy,    // only if data cannot be memcpy'd
                    datacb_cmp,     // for sorted insertion
                    datacb_str      // string representation of data.
                                    );

llist*  llist_dup(  const llist*);

void    llist_free( llist*);

lnode*  llist_head( const llist*);
lnode*  llist_tail( const llist*);
size_t  llist_lnode_count(  const llist*);
size_t  llist_data_size(const llist*);


lnode*  llist_unlink(   llist*, lnode*);
void    llist_unlink_free(llist*, lnode*);

lnode*  llist_add_data( llist*, void* data); /* sorted insertion */

lnode*  llist_select(       llist*, datacb_sel, const void* crit);
lnode*  llist_select_invert(llist*);
lnode*  llist_select_all(   llist*, bool sel);

llist*  llist_cut(      llist*, bool sel, datacb_mod, void* mod);
llist*  llist_copy(     llist*, bool sel, datacb_mod, void* mod);
void    llist_delete(   llist*, bool sel);

void    llist_paste(llist* dest,
                    const llist* src,
                    datacb_edit,
                    const void* offset     );

void    llist_edit( llist*, datacb_edit, const void* val, bool sel);

void    llist_sort( llist*); /* a poor-man's sort routine */

void    llist_set_cmp_cb(llist*, datacb_cmp);


/*  -------- FLAGS --------
*/

/*  llist_data_flags
    are functions for operating upon data in the list which uses
    flags (ie logic operations upon bits in an int). if datacb_flags
    passed to llist_new is NULL these functions do nothing.
*/
void    llist_data_flags_or(    llist*, int flags);
void    llist_data_flags_and(   llist*, int flags);
void    llist_data_flags_xor(   llist*, int flags);
void    llist_data_flags_off(   llist*, int flags);

lnode*  llist_data_flags_select(llist*, int flags);

/*  llist_data_flags_set - sets the string of characters to
    use as textual representation of the flags.
*/
void    llist_data_flags_set(   llist*,
                                datacb_flags,
                                size_t flags_bit_count,
                                const char* flags_str);

void    llist_data_flags_get(   const llist*,
                                datacb_flags *,
                                size_t * flags_bit_count,
                                const char** flags_str);


/*  llist_to_array creates an array out of the data pointed to by
    each lnode within the list. it assumes all data is of the size
    data_size - as passed to llist_new.

    the terminator if required, is a data item containing whatever
    values appropriate for your routines to distinguish it from
    normal expected data.
*/

void*   llist_to_array(const llist*, const void* terminator);

void*   llist_to_pointer_array(const llist*);

/*  llist_select_to_array preserves any existing selection
    made with the other llist_select functions. datacb_sel_cb
    and crit must not be null.
*/
void*   llist_select_to_array(  llist*,
                                datacb_sel,
                                const void* crit,
                                const void* terminator);

void*   llist_select_to_pointer_array(  llist*,
                                        datacb_sel,
                                        const void* crit );

void    llist_dump(const llist*, datacb_dump);

/*  llist_datacb_dup can be used where datacb_dup callbacks
    are required - you can use this when you'd also use
    free, and memcpy...
*/
void*   llist_datacb_dup(const void* src, size_t size);


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
