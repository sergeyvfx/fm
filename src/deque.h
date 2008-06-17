/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Deques
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _deque_h_
#define _deque_h_

#include "smartinclude.h"

BEGIN_HEADER

typedef struct _Iter iterator_t;
struct _Iter {
    void *data;
    iterator_t *next;
    iterator_t *prev;
};

typedef struct _Deq deque_t;
struct _Deq {
    iterator_t *head, *tail;
#ifdef PROFILE_DEQUE_LENGTH
    unsigned long long length;
#endif
};

typedef void (*destroyer) (void *);
typedef int (*comporator) (const void *, const void *);

#define deque_tail(self) (self)->tail
#define deque_head(self) (self)->head

#ifdef PROFILE_DEQUE_LENGTH
    #define deque_length(self) (self)->length
#endif

#define deque_next(self) (self)->next
#define deque_prev(self) (self)->prev
#define deque_data(self) (self)->data

#define deque_foreach(self, data) \
    { \
        deque_t     *__deq_ = self;\
        iterator_t  *__cur_, *__next_;\
        __cur_ = deque_head(__deq_);\
        while (__cur_)\
        {\
            data = deque_data (__cur_);\
            __next_ = deque_next (__cur_);

#define deque_foreach_break break
#define deque_foreach_done \
            __cur_ = __next_; \
        }\
    }

deque_t *
deque_create(void);

int
deque_destroy (deque_t *__this, destroyer __destroyer);

int
deque_clear (deque_t *__this, destroyer __destroyer);

int
deque_remove (deque_t *__this, iterator_t *__item, destroyer __destroyer);

int
deque_push_back (deque_t *__this, void *__data);

int
deque_push_front (deque_t *__this, void *__data);

void *
deque_pop_back (deque_t *__this);

void *
deque_pop_front (deque_t *__this);

void
deque_sort (deque_t *__this, comporator __compr);

iterator_t *
deque_sorted_insert (deque_t *__this, void *__data, comporator __compr);

iterator_t *
deque_find (deque_t *__this, void *__data, comporator __compr);

END_HEADER

#endif
