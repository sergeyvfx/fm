/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Assaciative arrays support
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _hashmap_h_
#define _hashmap_h_

#include "smartinclude.h"

BEGIN_HEADER

/********
 * Type defenitions
 */

/* Forward defenitions */
struct _hashmap_t;
typedef struct _hashmap_t hashmap_t;
typedef __u32_t hash_t;

typedef hash_t (*hashmap_hash_func) (const hashmap_t *__hashmap,
                                     const void* __key);
typedef void (*hashmap_key_deleter) (void *__key);
typedef short (*hashmap_keycmp) (const void *__key1,
                                 const void *__key2);
typedef void* (*hashmap_keydup) (const void *__src);
typedef void (*hashmap_deleter) (void *__data);

typedef struct
{
  /* Key of element in array */
  void *key;

  /* Stored value */
  void *value;

  /* Pointer to next element in list */
  /* of elements, which has equal hashes */
  void *next;
} hashmap_entry_t;

struct _hashmap_t
{
  /* Function to calculate key's hash value */
  hashmap_hash_func hash_func;

  /* Default deleter for elements */
  hashmap_deleter deleter;

  /* Deleter of key */
  hashmap_key_deleter key_deleter;

  /* Comparator of keys. Needed for resolving collisions */
  hashmap_keycmp key_comparator;

  /* Duplicator of keys */
  hashmap_keydup key_duplicator;

#ifdef PROFILE_HASHMAP_LENGTH
  int count;
#endif

  /* Count of elements array. */
  /* If this value if too small, there will be lots of colflictions of */
  /* hashes. If this value is too large, there maybe amount of unused memory.*/
  /* Look for happy medium, Luke :) */
  __u32_t data_length;

  hashmap_entry_t **data;
};

/********
 * Helphul macroses
 */

#define hashmap_foreach(_hm, _key, _value) \
  { \
    __u32_t __i_=0; \
    hashmap_entry_t *__cur_; \
    for (__i_=0; __i_<(_hm)->data_length; ++__i_) \
      { \
        __cur_=(_hm)->data[__i_]; \
        while (__cur_) \
          { \
            (_key)=__cur_->key; \
            (_value)=__cur_->value;

#define hashmap_foreack_break break
#define hashmap_foreach_done \
            __cur_=__cur_->next; \
          } \
      } \
  }

/********
 * Magick consts
 */

#define HM_MAGICK_LEN 1049

/********
 *
 */

hashmap_t*
hashmap_create (hashmap_hash_func __hash_func,
                hashmap_deleter __deleter,
                hashmap_key_deleter __key_deleter,
                hashmap_keycmp __key_comparator,
                hashmap_keydup __key_duplicator,
                __u32_t __data_length);

void
hashmap_destroy (hashmap_t *__hashmap);

void
hashmap_destroy_full (hashmap_t *__hashmap, hashmap_deleter __deleter);

void
hashmap_set (hashmap_t *__hashmap, const void *__key, const void *__value);

void
hashmap_set_full (hashmap_t *__hashmap, const void *__key,
                  const void *__value, hashmap_deleter __deleter);

void*
hashmap_get (const hashmap_t *__hashmap, const void *__key);

BOOL
hashmap_isset (const hashmap_t *__hashmap, const void *__key);

void
hashmap_unset (hashmap_t *__hashmap, const void *__key);

void
hashmap_unset_full (hashmap_t *__hashmap, const void *__key,
                    hashmap_deleter __deleter);

void
hashmap_unset_all (hashmap_t *__hashmap);

void
hashmap_unset_all_full (hashmap_t *__hashmap, hashmap_deleter __deleter);

/********
 * Vrappers for wchar-ed keys
 */

/* wck means WideChar'ed Keys */

hashmap_t *
hashmap_create_wck (hashmap_deleter __deleter, __u32_t __data_length);

END_HEADER

#endif
