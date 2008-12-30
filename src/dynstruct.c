/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Dynamic structures implementation
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "smartinclude.h"

#include <wchar.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "dynstruct.h"
#include "hashmap.h"

typedef struct field_t field_t;
struct field_t {
    wchar_t   *name;
    void      *value;
    size_t    size;
};

struct dynstruct_t {
    wchar_t    *name;
    hashmap_t  *fields;
};

void
dynstruct_field_deleter (void *__data)
{
  field_t *field = (field_t *) __data;

  SAFE_FREE (field->value);
  SAFE_FREE (field);
}

/**
 * Create dynamic structure
 *
 * @param __struct_name - a struct's name
 * @param ... - fields list a kind: field_name0, field_value0,
 * field_size0, ..., field_nameN, field_valueN, field_sizeN.
 * Fields list should terminate NULL.
 * @return pointer to new dynamic structure if successful
 *
 * @see dynstruct_add_field
 */
dynstruct_t *
dynstruct_create (const wchar_t *__struct_name, ...)
{
  dynstruct_t *dstruct;
  field_t     *field;
  wchar_t     *name;
  void        *value;
  va_list     argv;

  dstruct = (dynstruct_t *) malloc (sizeof (dynstruct_t));
  dstruct->name   = wcsdup (__struct_name);
  dstruct->fields =
    hashmap_create_wck (dynstruct_field_deleter, HM_MAGICK_LEN);

  va_start(argv, __struct_name);
    while (1)
      {
        field = (field_t *) malloc (sizeof (field_t));

        name = va_arg(argv, wchar_t *);
        if (name == NULL)
          {
            SAFE_FREE (field);
            break;
          }
        field->name = wcsdup (name);
        value = va_arg(argv, void *);
        field->size = va_arg(argv, size_t);

        if (value != NULL)
          {
            field->value = malloc(field->size);
            memcpy (field->value, value, field->size);
          }

        hashmap_set (dstruct->fields, field->name, field);
      }
  va_end(argv);

  return dstruct;
}

/**
 * Destroy dynamic structure
 *
 * @param __dstruct - a pointer to pointer on destoyed structure
 */
void
dynstruct_destroy (dynstruct_t **__dstruct)
{
  hashmap_destroy ((*__dstruct)->fields);
  SAFE_FREE ((*__dstruct)->name);
  SAFE_FREE ((*__dstruct));
}

/**
 * Add the field into dynamic structure
 *
 * @param __dstruct - a pointer on dynamic structure
 * @param __field_name - a field's name
 * @param __field_data - a field's value
 * @param __field_size - a size of field's value
 * @return DYNST_OK if successful, DYNST_FIELD_EXISTS if field with
 * __field_name already exists and DYNST_FAILURE otherwise
 */
int
dynstruct_add_field (dynstruct_t *__dstruct,
                     const wchar_t *__field_name,
                     void *__field_data, size_t __field_size)
{
  if (hashmap_isset (__dstruct->fields, __field_name) != 0)
    {
      return DYNST_FIELD_EXISTS;
    }

  void    *value = __field_data;
  field_t *field = (field_t *) malloc (sizeof (field_t));

  field->size  = __field_size;

  if (value == NULL)
    {
      SAFE_FREE (field);
      return DYNST_FAILURE;
    }

  field->name  = wcsdup (__field_name);
  field->value = malloc (field->size);
  memcpy (field->value, __field_data, field->size);

  hashmap_set (__dstruct->fields, field->name, field);

  return DYNST_OK;
}

/**
 * Get the field's value from dynamic structure
 *
 * @param __dstruct - a pointer on dynamic structure
 * @param __field_name - a field's name
 * @param __field_val - a pointer to pointer on value
 * @return DYNST_OK if successful, DYNST_FIELD_NFOUND if field not found
 */
int
dynstruct_get_field_val (const dynstruct_t *__dstruct,
                         const wchar_t *__field_name, void *__field_val)
{
  field_t *val = (field_t *) hashmap_get (__dstruct->fields, __field_name);
  if (val == NULL)
    {
      return DYNST_FIELD_NFOUND;
    }

  memcpy (__field_val, val->value, val->size);

  return DYNST_OK;
}

/**
 * Remove the field from dynamic structure
 *
 * @param __dstruct - a pointer on dynamic structure
 * @param __field_name - a name of a removed field
 * @return DYNST_OK if successful, DYNST_FIELD_NFOUND if field not found
 */
int
dynstruct_remove_field (dynstruct_t *__dstruct, const wchar_t *__field_name)
{
  if (hashmap_unset (__dstruct->fields, __field_name) != 0)
    {
      return DYNST_FIELD_NFOUND;
    }

  return DYNST_OK;
}

#ifdef NOINST_DEBUG
#include <stdio.h>
void
dynstruct_list_fields (const dynstruct_t *__dstruct)
{
  wchar_t *key;
  field_t *field;

  fwprintf (stderr, L"Listing struct `%ls':\n", __dstruct->name);
  hashmap_foreach(__dstruct->fields, key, field);
    fwprintf (stderr, L"  %ls\n", key);
  hashmap_foreach_done;
}
#endif
