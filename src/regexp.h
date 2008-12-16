/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Regular expressions module
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _REGEXP_H_
#define _REGEXP_H_

#include <smartinclude.h>

BEGIN_HEADER

/********
 * Type definitions
 */

struct regexp;
typedef struct regexp regexp_t;

/********
 *
 */

/* Compile regular expression */
regexp_t*
regexp_compile (const char *__regexp);

/* Free compiled regular expression descriptor */
void
regexp_free (regexp_t *__regexp);

/* Check is string matches to compiled regular expression */
BOOL
regexp_match (const regexp_t *__re, const char *__str);

/* Make sub-string replacing by compiled regular expression matching */
char*
regexp_replace (const regexp_t *__re, const char *__s, const char *__mask);

/* Check is string matches to regular expression */
BOOL
preg_match (const char *__regexp, const char *__str);

/* Make sub-string replacing by regular expression matching */
char*
preg_replace (const char *__regexp, const char *__s, const char *__mask);

/****
 * Wide-chared functions
 */

/* Compile regular expression */
regexp_t*
wregexp_compile (const wchar_t *__regexp);

/* Check is string matches to compiled regular expression */
BOOL
wregexp_match (const regexp_t *__re, const wchar_t *__str);

/* Check is string matches to regular expression */
BOOL
wpreg_match (const wchar_t *__regexp, const wchar_t *__str);

/* Make sub-string replacing by compiled regular expression matching */
wchar_t*
wregexp_replace (const regexp_t *__re,
                 const wchar_t *__s, const wchar_t *__mask);

/* Make sub-string replacing by regular expression matching */
wchar_t*
wpreg_replace (const wchar_t *__regexp,
               const wchar_t *__s, const wchar_t *__mask);

/* Escape regexp special characters in string */
char*
regexp_escape (const char *__str);

/* Escape regexp special characters in string */
wchar_t*
wregexp_escape (const wchar_t *__str);

END_HEADER

#endif
