#ifndef DATACB_H
#define DATACB_H


#include <stddef.h>

#define DATACB_STR_SIZE 80


/*  data structure callbacks
    ========================

    callbacks for use by llist and rbuf.

    note:   in some instances, system commands can be used, these are:
            datacb_free can point to free
            datacb_copy can point to memcpy
            datacb_cmp can point to memcmp ( do not use bcmp )

            in particular, memcpy and memcmp require a third arguement.
            so the datacb_copy and datacb_cmp callback typedefs expect
            to point to a function that takes 3 arguments.

            this has implications for llist and rbuf. if the callbacks
            defined for your object (as opposed to a simple object which
            can use free, memcpy, and memcmp) require the third argument,
            'size_t n'... the size_t n should represent the size of the
            data item which the callback is designed to handle - and not
            as a specification for the size of an array of that data type.

            can the llist structure be used to create a linked list of
            arrays of which it can handle 
            
*/

/*  datacb_free:
    a callback to free data, if unset, free() will be
    used instead.
*/
typedef void    (*datacb_free)( void* data);

/*  datacb_copy:
    a callback to be used for copying data items.
    if unset, the rbuf/llist will simply use memcpy.
*/
typedef void*   (*datacb_copy)(       void* dest,
                                const void* src,
                                     size_t n  );

/*  datacb_cmp:
    a callback to compare items of data pointed to by
    two lnodes. the callback must return:
        -1  d1 < d2, 0   d1 == d2, 1   d1 > d2
    if unset, memcmp will be used.

*/
typedef int     (*datacb_cmp)(  const void* d1,
                                const void* d2,
                                     size_t n );

/*  datacb_dup:
    a callback to perform duplication of data pointed to by
    lnode. if unset, the list will simply use malloc and memcpy.
*/
typedef void*   (*datacb_dup)(  const void* data,
                                     size_t n );

/*  datacb_sel:
    a callback to determe if the selected flag of an
    lnode should be true or false.
*/
typedef _Bool   (*datacb_sel)(  const void* data,
                                const void* crit );

/*  ldatacb_edit:
    a callback which will edit the data pointed to by
    an lnode.
*/
typedef void    (*datacb_edit)(       void* data,
                                const void* val  );

/*  ldatacb_flags:
    a callback to provide a pointer to flags within
    the data pointed to be an lnode giving the llist a
    simple mechanism to automate flag setting on llists.
*/
typedef int*    (*datacb_flags)(void* data );

/*  datacb_mod
    a callback which can be used to modify data during
    copy/cut operations on, for example, a list. (ie for
    items containing positional information, you may want
    to adjust the cut/copied data to start from position
    zero. )
*/
typedef void    (*datacb_mod)(  void* data, void* init);

/*  datacb_dump
    a callback to display contents of data
*/
typedef void    (*datacb_dump)( const void* data);


/*  datacb_str
    a callback used to get a string representation of an item
    of data.
*/
typedef char*   (*datacb_str)(  const void* data, int level);


/*  datacb_rtdata
    the data structures used in boxyseq are composed of two parts,
    the first part being a representation accessed by the user interface,
    the second being a representation of the same data that is accessed
    by the REAL TIME thread.

    often they will differ in some way, preventing a straight copy of
    the ui data to rt data.

    this callback is to provide the rtdata* functions with a method of
    converting from ui data to rt data.
*/


typedef void*   (*datacb_rtdata)(const void* data);


#endif
