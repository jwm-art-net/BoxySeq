#include "llist.h"

#include "debug.h"

#include <stdlib.h>
#include <string.h>


typedef enum LNFLAGS
{
    LNF_SELECTED =  0x0001, /* public selection */
    LNF_ASELECTED = 0x0002  /* private selection */

} lnflags;


struct lnode
{
    void* data;

    struct lnode* prev;
    struct lnode* next;

    lnflags flags;
};


struct llist
{
    lnode*  head;
    lnode*  tail;

    size_t  data_size;
    size_t  lnode_count;
    size_t  sel_count;
    size_t  asel_count;

    datacb_free     cb_free;
    datacb_dup      cb_dup;
    datacb_copy     cb_copy;
    datacb_cmp      cb_cmp;
    datacb_str      cb_str;
    datacb_flags    cb_flags;

    const char*     flags_str;
    size_t          flags_bit_count;
};


/************************************************************************
    lnode private implementation
 ************************************************************************/

static lnode* lnode_pri_new(void* data,
                            lnode* prev,
                            lnode* next,
                            llist* ll )
{
    lnode* ln = malloc(sizeof(*ln));

    if (!ln)
    {
        WARNING("out of memory allocating lnode\n", 1);
        return 0;
    }

    ln->data = data;
    ln->prev = prev;
    ln->next = next;

    if (prev)
        prev->next = ln;

    if (next)
        next->prev = ln;

    ln->flags = 0;

    #ifdef LLIST_DEBUG
    lnode_dump("new", ln, ll->cb_str, 1);
    #endif

    ll->lnode_count++;

    return ln;
}


/************************************************************************
    lnode public implementation
 ************************************************************************/

inline lnode*   lnode_prev(const lnode* ln)     { return ln->prev;     }
inline lnode*   lnode_next(const lnode* ln)     { return ln->next;     }
inline void*    lnode_data(const lnode* ln)     { return ln->data;     }


inline void lnode_select(lnode* ln)
{
    ln->flags |= LNF_SELECTED;
}


inline void lnode_unselect(lnode* ln)
{
    ln->flags &= (unsigned)~LNF_SELECTED;
}


inline _Bool lnode_selected(const lnode* ln)
{
    return ln->flags & LNF_SELECTED;
}


void
lnode_dump(const char* msg, const lnode* ln,
                datacb_str cb_str, int level)
{
    if (!ln)
    {
        MESSAGE("%s lnode:NULL\n");
        return;
    }

    char* lnf = strdup("--");

    lnf[0] = (char)(ln->flags & LNF_SELECTED  ? 'S' : '-');
    lnf[1] = (char)(ln->flags & LNF_ASELECTED ? 'A' : '-');

    if (cb_str)
    {
        char* str = cb_str(ln->data, level);
        MESSAGE("%s lnode:%p p:%p n:%p f: %s data:%p [ %s ]\n",
                msg, ln, ln->prev, ln->next, lnf,
                ln->data, str);
        free(str);
    }
    else
        MESSAGE("%s lnode:%p p:%p n:%p f:%s data:%p\n",
                msg, ln, ln->prev, ln->next, lnf, ln->data);

    free(lnf);
}


/************************************************************************
    llist private implementation
 ************************************************************************/


static void llist_pri_copy_misc(llist* dest, const llist* src)
{
    dest->cb_flags =        src->cb_flags;
    dest->flags_str =       src->flags_str;
    dest->flags_bit_count = src->flags_bit_count;
}


static lnode*
llist_pri_add_at_tail(llist* ll, lnode* ln)
{
    if (!ll->head)
    {
        ll->head = ll->tail = ln;
        return ln;
    }

    #ifdef LLIST_DEBUG
    if (ll->cb_cmp)
    {
        if (ll->cb_cmp(ln->data, ll->tail->data, ll->data_size) < 0)
        {
            WARNING("adding lnode %p at tail of llist %p"
                    "breaks order\n", ln, ll);
            return 0;
        }
    }
    #endif

    ll->tail = ln;
    return ln;
}


static lnode*
llist_pri_link_at_tail(llist* ll, lnode* ln)
{
    #ifdef LLIST_DEBUG
    MESSAGE("re-linking lnode:%p at tail of llist:%p\n", ln, ll);
    #endif

    #ifdef LLIST_DEBUG
    if (ll->tail && ll->cb_cmp)
    {
        if (ll->cb_cmp(ln->data, ll->tail->data, ll->data_size) < 0)
        {
            WARNING("adding lnode %p at tail of llist %p"
                    "breaks order\n", ln, ll);
            return 0;
        }
    }
    #endif

    ll->lnode_count++;

    if (!ll->head)
    {
        ll->head = ll->tail = ln;
        ln->prev = ln->next = 0;
        return ln;
    }

    ln->prev = ll->tail;
    ll->tail->next = ln;
    ll->tail = ln;
    ln->next = 0;

    return ln;
}


static lnode*
llist_pri_select_flags( llist* ll,
                        lnflags lnf,
                        datacb_sel cb_sel, const void* crit)
{
    if (!ll->lnode_count || !cb_sel)
        return 0;

    lnode* ln = ll->head;
    lnode* first = 0;

    size_t selcount = 0;

    while(ln)
    {
        if (cb_sel(ln->data, crit))
        {
            ++selcount;
            ln->flags |= lnf;
            if (!first)
                first = ln;
        }
        else
            ln->flags &= ~lnf;

        ln = ln->next;
    }

    if (lnf & LNF_SELECTED)
        ll->sel_count = selcount;
    if (lnf & LNF_ASELECTED)
        ll->asel_count = selcount;

    return first;
}


static lnode*
llist_pri_select_flags_invert(llist* ll, lnflags lnf)
{
    if (ll->lnode_count == 0)
        return 0;

    lnode* ln = ll->head;
    lnode* first = 0;

    size_t selcount = 0;

    while(ln)
    {
        ln->flags ^= lnf;

        if (ln->flags & lnf)
        {
            ++selcount;
            if (!first)
                first = ln;
        }

        ln = ln->next;
    }

    if (lnf & LNF_SELECTED)
        ll->sel_count = selcount;
    if (lnf & LNF_ASELECTED)
        ll->asel_count = selcount;

    return first;
}


static lnode*
llist_pri_select_flags_all(llist* ll, lnflags lnf, _Bool sel)
{
    lnode* ln = ll->head;

    while(ln)
    {
        if (sel)
            ln->flags |= lnf;
        else
            ln->flags &= ~lnf;

        ln = ln->next;
    }

    if (lnf & LNF_SELECTED)
        ll->sel_count = (sel ? ll->lnode_count : 0);
    if (lnf & LNF_ASELECTED)
        ll->asel_count = (sel ? ll->lnode_count : 0);

    return ll->head;
}

static void*
llist_pri_to_array( const llist* ll,
                    size_t count,
                    lnflags lnf,
                    const lnode* first,
                    const void* terminator)
{
    if (terminator)
        ++count;

    if (!count || !ll->cb_copy)
        return 0;

    #ifdef LLIST_DEBUG
    MESSAGE("making array from %d lnodes%s within llist %p\n",
            count, (terminator ? " (inc terminator)" : ""), ll);
    #endif

    void* arr = malloc(ll->data_size * count);

    if (!arr)
    {
        WARNING("out of memory copying llist to array\n");
        return 0;
    }

    void* i = arr;
    void* iend = (void*)((char*)i + ll->data_size * 
                         (count - (size_t)(terminator ? 1 : 0)));

    const lnode* ln = first;

    while(ln && i <= iend)
    {
        if (!lnf || (ln->flags & lnf))
        {
            ll->cb_copy(i, ln->data, ll->data_size);
            i = (void*)((char*)i + ll->data_size);
        }
        ln = ln->next;
    }

    if (terminator)
        ll->cb_copy(i, terminator, ll->data_size);

    return arr;
}

/************************************************************************
    llist public implementation
 ************************************************************************/

llist*
llist_new(  size_t data_size,   datacb_free  cb_free,
                                datacb_dup   cb_dup,
                                datacb_copy  cb_copy,
                                datacb_cmp   cb_cmp,
                                datacb_str   cb_str    )
{
    if (!data_size)
    {
        WARNING("cannot have list with zero data sized elements\n");
        return 0;
    }

    llist* ll = malloc(sizeof(*ll));

    if (!ll)
    {
        WARNING("out of memory for new llist\n");
        return 0;
    }

    ll->data_size = data_size;

    ll->head = ll->tail = 0;

    ll->lnode_count = 0;

    ll->cb_free =  cb_free;
    ll->cb_dup =   cb_dup;
    ll->cb_copy =  cb_copy;
    ll->cb_cmp =   cb_cmp;
    ll->cb_str =   cb_str;

    ll->cb_flags = 0;
    ll->flags_str = 0;
    ll->flags_bit_count = 0;

    #ifdef LLIST_DEBUG
    if (!cb_free)
        WARNING("no callback provided to free list data\n");
    if (!cb_dup)
        WARNING("no callback provided to duplicate list data\n");
    if (!cb_copy)
        WARNING("no callback provided to copy list data\n");
    if (!cb_cmp)
        WARNING("no callback provided to compare list data\n");
    #endif

    return ll;
}


llist*
llist_dup(const llist* ll)
{
    llist* newll = llist_new(   ll->data_size,
                                ll->cb_free,
                                ll->cb_dup,
                                ll->cb_copy,
                                ll->cb_cmp,
                                ll->cb_str      );

    if (!newll || !ll->cb_dup)
        return 0;

    lnode* ln = ll->head;

    void* newdata = 0;

    while(ln)
    {
        newdata = ll->cb_dup(ln->data, ll->data_size);

        if (!newdata)
        {
            WARNING("out of memory copying data\n");
            llist_free(newll);
            return 0;
        }

        llist_pri_add_at_tail(newll,
                lnode_pri_new(newdata, newll->tail, 0, newll));

        ln = ln->next;
    }

    llist_pri_copy_misc(newll, ll);

    return newll;
}


void
llist_free(llist* ll)
{
    lnode* ln = ll->head;
    lnode* n = 0;

    while(ln)
    {
        ll->cb_free(ln->data);
        n = ln->next;
        free(ln);
        ln = n;
    }

    free(ll);
}


lnode*  llist_head(const llist* ll)         { return ll->head; }
lnode*  llist_tail(const llist* ll)         { return ll->tail; }
size_t  llist_lnode_count(const llist* ll)  { return ll->lnode_count; }
size_t  llist_data_size(const llist* ll)    { return ll->data_size; }


lnode*
llist_unlink(llist* ll, lnode* ln)
{
    lnode* prev = ln->prev;
    lnode* next = ln->next;

    #ifdef LLIST_DEBUG
    lnode_dump("unlinking", ln, ll->cb_str, 1);
    #endif

    if (ln == ll->head)
    {
        ll->head = next;
        if (next)
            next->prev = 0;
    }

    if (ln == ll->tail)
    {
        ll->tail = prev;
        if (prev)
            prev->next = 0;
    }

    if (prev)
        prev->next = next;

    if (next)
        next->prev = prev;

    ll->lnode_count--;

    return ln;
}


void llist_unlink_free(llist* ll, lnode* ln)
{
    lnode* remln = llist_unlink(ll, ln);

    if (remln != ln)
        return;

    ll->cb_free(ln->data);
    free(ln);
}

lnode*
llist_add_data(llist* ll, void* data)
{
    if (!data)
    {
        WARNING("llist_add_data will not add NULL data\n");
        return 0;
    }

    if (!ll->head)
        return ll->head = ll->tail = lnode_pri_new(data, 0, 0, ll);

    lnode* newln = 0;
    lnode* ln = ll->head;

    if (ll->cb_cmp)
    {
        while(ln)
        {
            if (ll->cb_cmp(data, ln->data, ll->data_size) < 0)
            {
                if (!(newln = lnode_pri_new(data, ln->prev, ln, ll)))
                    return 0;

                if (ln == ll->head)
                    ll->head = newln;

                return newln;
            }
            ln = ln->next;
        }
    }

    if (!(newln = lnode_pri_new(data, ll->tail, 0, ll)))
        return 0;

    ll->tail = newln;

    return newln;
}


inline lnode*
llist_select(llist* ll, datacb_sel cb_sel, const void* crit)
{
    return llist_pri_select_flags(ll, LNF_SELECTED, cb_sel, crit);
}


inline lnode*
llist_select_invert(llist* ll)
{
    return llist_pri_select_flags_invert(ll, LNF_SELECTED);
}


inline lnode*
llist_select_all(llist* ll, _Bool sel)
{
    return llist_pri_select_flags_all(ll, LNF_SELECTED, sel);
}


llist*
llist_cut(llist* ll, _Bool sel, datacb_mod cb_mod, void* mod)
{
    llist* newll = llist_new(   ll->data_size,
                                ll->cb_free,
                                ll->cb_dup,
                                ll->cb_copy,
                                ll->cb_cmp,
                                ll->cb_str      );

    if (!newll)
        return 0;

    llist_pri_copy_misc(newll, ll);

    lnode* ln = ll->head;
    lnode* next = 0;
    int sel_count = 0;

    while(ln)
    {
        next = ln->next;

        if ((sel && (ln->flags & LNF_SELECTED)) || !sel)
        {
            llist_pri_link_at_tail(newll, llist_unlink(ll, ln));

            if (cb_mod)
                cb_mod(ln->data, mod);

            ln->flags = 0;

            ++sel_count;
        }

        ln = next;
    }

    if (sel_count)
        return newll;

    free(newll);
    return 0;
}


llist*
llist_copy(llist* ll, _Bool sel, datacb_mod cb_mod, void* mod)
{
    if (!ll->cb_dup)
        return 0;

    llist* newll = llist_new(   ll->data_size,
                                ll->cb_free,
                                ll->cb_dup,
                                ll->cb_copy,
                                ll->cb_cmp,
                                ll->cb_str      );
    if (!newll)
        return 0;

    llist_pri_copy_misc(newll, ll);

    lnode* ln = ll->head;
    int sel_count = 0;

    void* newdata = 0;

    while(ln)
    {
        if ((sel && (ln->flags & LNF_SELECTED)) || !sel)
        {
            newdata = ll->cb_dup(ln->data, ll->data_size);

            if (!newdata)
            {
                llist_free(newll);
                return 0;
            }

            if (cb_mod)
                cb_mod(newdata, mod);

            llist_pri_add_at_tail(newll,
                    lnode_pri_new(newdata, newll->tail, 0, newll));
            ++sel_count;
        }

        ln = ln->next;
    }

    if (sel_count)
        return newll;

    free(newll);
    return 0;
}


void
llist_delete(llist* ll, _Bool sel)
{
    if (!ll->cb_free)
        return;

    lnode* ln = ll->head;
    lnode* n = 0;

    while(ln)
    {
        n = ln->next;

        if ((sel && (ln->flags & LNF_SELECTED)) || !sel)
        {
            ll->cb_free(ln->data);
            free(llist_unlink(ll, ln));
        }

        ln = n;
    }
}

void
llist_paste(llist* dest, const llist* src,
                         datacb_edit cb_edit,
                         const void* offset     )
{
    if (!src->cb_dup)
        return;

    if (src == dest)
    {
        WARNING("cannot paste list into itself\n");
        return;
    }

    if (dest->data_size != src->data_size)
    {
        WARNING("cannot paste list into list, data sizes differ\n");
        return;
    }

    void* newdata = 0;
    lnode* newln = 0;
    lnode* srcln = src->head;
    lnode* destln = dest->head;

    /*  remember:   this only appears to be broken if the list
                    is broken already - by some other function.
    */

    while(srcln)
    {
        newdata = src->cb_dup(srcln->data, src->data_size);

        if (!newdata)
            return;

        if (cb_edit)
            cb_edit(newdata, offset);

        while(destln)
        {
            if (dest->cb_cmp(destln->data, newdata, dest->data_size) > 0)
                break;

            destln = destln->next;
        }

        if (!destln)
        {
            if (!(newln = lnode_pri_new(newdata, dest->tail, 0, dest)))
                goto bail;

            if (!llist_pri_add_at_tail(dest, newln))
                goto bail;
        }
        else
        {
            newln = lnode_pri_new(newdata, destln->prev, destln, dest);
            if (!newln)
                goto bail;

            if (destln == dest->head)
                dest->head = newln;
        }

        srcln = srcln->next;
    }
    return;

bail:
    if (dest->cb_free)
        dest->cb_free(newdata);
    else
        free(newdata);
}


void
llist_edit(llist* ll, datacb_edit cb_edit, const void* val, _Bool sel)
{
    lnode* ln = ll->head;

    while(ln)
    {
        if ((sel && (ln->flags & LNF_SELECTED)) || !sel)
            cb_edit(ln->data, val);

        ln = ln->next;
    }
}

void
llist_dump(const llist* ll, datacb_dump cb_dump)
{
    MESSAGE("llist: %p\n\tlnode_count: %d\n\thead: %p\n\ttail: %p\n",
            ll, ll->lnode_count, ll->head, ll->tail);

    lnode* ln = ll->head;

    _Bool showflags = (ll->cb_flags && ll->flags_str);

    char*   fbuf = 0;

    if (showflags)
    {
        fbuf = malloc(ll->flags_bit_count + 1);

        if (!fbuf)
            showflags = 0;

        fbuf[ll->flags_bit_count] = 0;
    }

    while(ln)
    {
        if (cb_dump)
            cb_dump(ln->data);
        else
            lnode_dump("", ln, ll->cb_str, 1);

        if (showflags)
        {
            int* f = ll->cb_flags(ln->data);
            size_t i;
            int mb = 1;

            for (i = 0; i < ll->flags_bit_count; ++i, mb *= 2)
                if (*f & mb)
                    fbuf[i] = ll->flags_str[i];
                else
                    fbuf[i] = '-';

            MESSAGE("data flags:%s\n", fbuf);
        }
        ln = ln->next;
    }

    MESSAGE("-- end of llist --\n");

    if (showflags)
        free(fbuf);
}


void
llist_set_cmp_cb(llist* ll, datacb_cmp cb_cmp)
{
    ll->cb_cmp = cb_cmp;
}


void
llist_sort(llist* ll)
{
    if (!ll->cb_cmp)
        return;

    lnode* ln = ll->head;
    lnode* n = 0;
    lnode* p = 0;
    lnode* next = 0;
    lnode* prev = 0;

    #ifdef LLIST_DEBUG
    MESSAGE("sorting llist:%p\n", (void*)ll);
    #endif

    while(ln)
    {
        next = n = ln->next;

        if (n)
        {
            while(ll->cb_cmp(ln->data, n->data, ll->data_size) > 0)
            {
                #ifdef LLIST_DEBUG
                if (ll->cb_str)
                {
                    char* lnstr = ll->cb_str(ln->data, 1);
                    char* nstr = ll->cb_str(n->data, 1);
                    MESSAGE("ln:%p %s > n:%p %s\n",
                        ln, lnstr, n,  nstr);
                    free(lnstr);
                    free(nstr);
                }
                else
                    MESSAGE("ln:%p > n:%p\n",ln,n);
                #endif
                n = n->next;
                if (!n)
                    break;
            }

            if (!n)
            {
                llist_unlink(ll, ln);
                ll->lnode_count++;
                ln->prev = ll->tail;
                ln->next = 0;
                ll->tail->next = ln;
                ll->tail = ln;
                #ifdef LLIST_DEBUG
                MESSAGE("ln:%p moved to tail\n",ln);
                #endif
            }
            else if (n != next)
            {
                llist_unlink(ll, ln);
                ll->lnode_count++;
                n->prev->next = ln;
                ln->prev = n->prev;
                n->prev = ln;
                ln->next = n;
                #ifdef LLIST_DEBUG
                MESSAGE("ln:%p moved before n:%p\n",ln,n);
                #endif
            }
        }

        prev = p = ln->prev;

        if (p)
        {
            while(ll->cb_cmp(ln->data, p->data, ll->data_size) < 0)
            {
                #ifdef LLIST_DEBUG
                if (ll->cb_str)
                {
                    char* lnstr = ll->cb_str(ln->data, 1);
                    char* pstr = ll->cb_str(p->data, 1);
                    MESSAGE("ln:%p %s < p:%p %s\n",
                        ln, lnstr, p,  pstr);
                    free(lnstr);
                    free(pstr);
                }
                else
                    MESSAGE("ln:%p < p:%p\n",ln,p);
                #endif
                p = p->prev;
                if (!p)
                    break;
            }

            if (!p)
            {
                llist_unlink(ll, ln);
                ll->lnode_count++;
                ll->head->prev = ln;
                ln->next = ll->head;
                ln->prev = 0;
                ll->head = ln;
                #ifdef LLIST_DEBUG
                MESSAGE("ln:%p moved to head\n",ln);
                #endif
            }
            else if (p != prev)
            {
                llist_unlink(ll, ln);
                ll->lnode_count++;
                ln->prev = p;
                p->next->prev = ln;
                ln->next = p->next;
                p->next = ln;
                #ifdef LLIST_DEBUG
                MESSAGE("ln:%p moved after p:%p\n",ln,p);
                #endif
            }
        }
        ln = next;
    }
}


static _Bool llist_pri_has_flags_cb(llist* ll)
{
    if (!ll->cb_flags)
    {
        WARNING("llist %p lacks flag setting capabilities\n", ll);
        return 0;
    }

    return 1;
}

void llist_data_flags_or(llist* ll, int flags)
{
    if (!llist_pri_has_flags_cb(ll))
        return;

    lnode* ln;

    for (ln = ll->head; ln != 0; ln = ln->next)
        *(ll->cb_flags(ln->data)) |= flags;
}


void llist_data_flags_and(llist* ll, int flags)
{
    if (!llist_pri_has_flags_cb(ll))
        return;

    lnode* ln;

    for (ln = ll->head; ln != 0; ln = ln->next)
        *(ll->cb_flags(ln->data)) &= flags;
}


void llist_data_flags_xor(llist* ll, int flags)
{
    if (!llist_pri_has_flags_cb(ll))
        return;

    lnode* ln;

    for (ln = ll->head; ln != 0; ln = ln->next)
        *(ll->cb_flags(ln->data)) ^= flags;
}


void llist_data_flags_off(llist* ll, int flags)
{
    if (!llist_pri_has_flags_cb(ll))
        return;

    lnode* ln;
    int* f;

    for (ln = ll->head; ln != 0; ln = ln->next)
    {
        f = ll->cb_flags(ln->data);
        *f ^= (*f & flags);
    }
}


lnode* llist_data_flags_select(llist* ll, int flags)
{
    if (!llist_pri_has_flags_cb(ll))
        return 0;

    lnode* ln;
    lnode* first = 0;
    ll->sel_count = 0;

    for(ln = ll->head; ln != 0; ln = ln->next)
    {
        if (*((int*)ll->cb_flags(ln->data)) && flags)
        {
            lnode_select(ln);
            ++ll->sel_count;

            if (!first)
                first = ln;
        }
    }

    return first;
}


void    llist_data_flags_set(   llist* ll,
                                datacb_flags flags_cb,
                                size_t flags_bit_count,
                                const char* flags_str)
{
    ll->cb_flags = flags_cb;
    ll->flags_bit_count = flags_bit_count;
    ll->flags_str = flags_str;
}


void    llist_data_flags_get(   const llist* ll,
                                datacb_flags * flags_cb,
                                size_t * flags_bit_count,
                                const char** flags_str)
{
    if (flags_cb)
        *flags_cb = ll->cb_flags;

    if (flags_bit_count)
        *flags_bit_count = ll->flags_bit_count;

    if (flags_str)
        *flags_str = ll->flags_str;
}



inline void*
llist_to_array(const llist* ll, const void* terminator)
{
    return llist_pri_to_array(  ll,
                                ll->lnode_count,
                                0, /* no selection */
                                ll->head,
                                terminator );
}


inline void*
llist_select_to_array(  llist* ll,
                        datacb_sel sel_cb,
                        const void* crit,
                        const void* terminator)
{
    lnode* first = llist_pri_select_flags(  ll,
                                            LNF_ASELECTED,
                                            sel_cb,
                                            crit    );

    return llist_pri_to_array(  ll,
                                ll->asel_count,
                                LNF_ASELECTED,
                                first,
                                terminator );
}


void* llist_datacb_dup(const void* src, size_t size)
{
    void* dest = malloc(size);

    if (!dest)
    {
        WARNING("out of memory for data duplication\n");
        return 0;
    }

    memcpy(dest, src, size);

    return dest;
}
