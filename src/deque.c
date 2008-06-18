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

#include "deque.h"

#include <malloc.h>
#include <stdlib.h>

static void
swap (iterator_t *__a, iterator_t *__b)
{
    void *data;

    data = __a->data;
    __a->data = __b->data;
    __b->data = data;
}

/**
 * Initializes deque
 *
 * @return initialized empty deque
 */
deque_t *
deque_create (void)
{
    deque_t *deq = (deque_t *) malloc (sizeof (deque_t));

    deq->head = deq->tail = NULL;

#ifdef PROFILE_DEQUE_LENGTH
    deq->length = 0;
#endif

    return deq;
}

/**
 * Removes element from deque
 *
 * @param __this is deque
 * @param __item is remove element
 * @param __destroyer is destructor for __item data
 * @return 0 if successful, -1 otherwise
 */
int
deque_remove (deque_t *__this, iterator_t *__item, destroyer __destroyer)
{
    if (!__this) return -1;
    if (!__item) return -1;

    if (__item->next)
        __item->next->prev = __item->prev;

    if (__item->prev)
        __item->prev->next = __item->next;

    if (__item == __this->head)
        __this->head = __item->next;

    if (__this->tail == __item)
        __this->tail = __item->prev;

    if (__destroyer)
        __destroyer (__item->data);

#ifdef PROFILE_DEQUE_LENGTH
    --__this->length;
#endif
    free (__item);
    return 0;
}

/**
 * Removes all elements of deque
 *
 * @param __this is deque
 * @param __destroyer is destructor for element data
 * @return 0 if successful, -1 otherwise
 */
int
deque_clear (deque_t *__this, destroyer __destroyer)
{
    iterator_t *current, *t;

    if (!__this)
        return -1;

    current = __this->head;
    while (current)
    {
        t = current;
        current = current->next;
        deque_remove (__this, t, __destroyer);
    }

    return 0;
}

/**
 * Destroy deque
 *
 * @param __this is deque
 * @param __destroyer is destructor for element data
 * @return 0 if successful, -1 otherwise
 */
int
deque_destroy (deque_t *__this, destroyer __destroyer)
{
    if (!__this) return -1;
    deque_clear (__this, __destroyer);
    free (__this);

    return 0;
}

/**
 * Inserts element to tail of deque
 *
 * @param __this is deque
 * @param __data is data to insert
 * @return 0 if successful, -1 otherwise
 */
int
deque_push_back (deque_t *__this, void *__data)
{
    iterator_t *new_iter;

    if (!__this)
        return -1;

    new_iter = (iterator_t *) malloc (sizeof (iterator_t));
    new_iter->data = __data;
    new_iter->prev = __this->tail;
    new_iter->next = NULL;

    if (__this->tail)
    {
        __this->tail->next = new_iter;
        __this->tail = new_iter;
    }
    else
    {
        __this->tail = __this->head = new_iter;
    }

#ifdef PROFILE_DEQUE_LENGTH
    ++__this->length;
#endif

    return 0;
}

/**
 * Inserts element to head of deque
 *
 * @param __this is deque
 * @param __data is data to insert
 * @return 0 if successful, -1 otherwise
 */
int
deque_push_front (deque_t *__this, void *__data)
{
    iterator_t *new_iter;

    if (!__this)
        return -1;

    new_iter = (iterator_t *) malloc (sizeof (iterator_t));
    new_iter->data = __data;
    new_iter->prev = NULL;
    new_iter->next = __this->head;

    if (__this->head)
        __this->head->prev = new_iter;
    __this->head = new_iter;

    if (!__this->tail)
        __this->tail = __this->head;

#ifdef PROFILE_DEQUE_LENGTH
    ++__this->length;
#endif

    return 0;
}

/**
 * Removes element from tail of deque
 *
 * @param __this is deque
 * @return data from remove element, or NULL if deque empty
 */
void *
deque_pop_back (deque_t *__this)
{
    void *data;

    if (!__this)
        return NULL;

    data = __this->tail->data;
    __this->tail = __this->tail->prev;
    __this->tail->next = NULL;

    if (!__this->tail)
        __this->head = NULL;

#ifdef PROFILE_DEQUE_LENGTH
    --__this->length;
#endif

    return data;
}

/**
 * Removes element from head of deque
 *
 * @param __this is deque
 * @return data from remove element, or NULL if deque empty
 */
void *
deque_pop_front (deque_t *__this)
{
    iterator_t *pop_iter;

    if (!__this)
        return NULL;

    pop_iter = __this->head;
    __this->head = __this->head->next;

    if (!__this->head)
        __this->tail = NULL;

#ifdef PROFILE_DEQUE_LENGTH
    --__this->length;
#endif

    return pop_iter;
}

/**
 * Sorts an deque
 *
 * @param __this is deque
 * @param __compr is comparison function
 */
void
deque_sort (deque_t *__this, comporator __compr)
{
    iterator_t *i, *j, *current;
    if (!__this) return;

    for (i = __this->head; i != NULL; i = i->next)
    {
        current = __this->head;
        for (j = __this->head; j != __this->tail; j = j->next)
        {
            if (__compr (current->data, current->next->data) > 0)
            swap(current, current->next);
            current = current->next;
        }
    }
}

/**
 * Sorting insert in deque
 *
 * @param __this is deque
 * @param __data is data to insert
 * @param __compr comparison function
 * @return iterator inserted element if successful, NULL otherwise
 */
iterator_t *
deque_sorted_insert (deque_t *__this, void *__data, comporator __compr)
{
    iterator_t *current, *t, *prev = NULL;
    if (!__this) return NULL;

    current = __this->head;

    for (current = __this->head; current != NULL;
        prev=current, current = current->next)
    {
        if ( __compr (current->data, __data) > 0) {
            break;
        }
    }

    if (!current) {
        deque_push_back(__this, __data);
        return __this->tail;
    }

    t = (iterator_t *) malloc (sizeof(iterator_t));
    if (prev) prev->next=t;
    else __this->head=t;

    t->data = __data;
    t->next = current;
    t->prev = prev;
    current->prev = t;

#ifdef PROFILE_DEQUE_LENGTH
    ++__this->length;
#endif

    return t;
}

/**
 * Find an element in deque
 *
 * @param __this is deque
 * @param __data is search data
 * @param __compr is comparison function
 * @return iterator found element if successful, NULL otherwise
 */
iterator_t *
deque_find (deque_t *__this, void *__data, comporator __compr)
{
    iterator_t *current;
    if (!__this || !__data || !__compr) return NULL;

    for (current = __this->head; current != NULL;
        current = current->next)
    {
        if ( __compr (current->data, __data) == 0) {
            break;
        }
    }

    return current;
}
