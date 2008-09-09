/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Dynamic structures implementation
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _DYNSTRUCT_H_
#define _DYNSTRUCT_H_

BEGIN_HEADER

typedef struct dynstruct_t dynstruct_t;
struct dynstruct_t;

enum {
    DYNST_FAILURE = -254,
    DYNST_FIELD_NFOUND,
    DYNST_FIELD_EXISTS,
    DYNST_OK = 0
};

dynstruct_t *
dynstruct_create (const wchar_t *, ...);

void
dynstruct_destroy (dynstruct_t **);

int
dynstruct_add_field (dynstruct_t *, const wchar_t *, void *, size_t );

int
dynstruct_get_field_val (const dynstruct_t *, const wchar_t *, void **);

int
dynstruct_remove_field (dynstruct_t *, const wchar_t *);

#ifdef NOINST_DEBUG

void
dynstruct_list_fields (const dynstruct_t *);

#endif

END_HEADER

#endif
