/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Different helpers
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _util_h_
#define _util_h_

#include "smartinclude.h"

#include <wchar.h>

/********
 *
 */

/* Fit string to specified width */
wchar_t*
wcsfit (const wchar_t *__str, size_t __width, const wchar_t *__suffix);

/* Duplicate at most n characters of string */
wchar_t*
wcsndup (const wchar_t *__s, size_t __n);

/* Get sub-string of string */
void
wcssubstr (wchar_t *__dst, const wchar_t *__src, size_t __from, size_t __len);

/* Get sub-string of string */
void
substr (char *__dst, const char *__src, size_t __from, size_t __len);

/* Replace substrings of string */
wchar_t*
wcsrep (wchar_t *__str, const wchar_t *__substr, const wchar_t *__newsubstr);

/* Converted wide chararacters string to multibyte characters string */
size_t
wcs2mbs (char **__dest, const wchar_t *__source);

size_t
mbs2wcs (wchar_t **__dest, const char *__source);

/* Get the number of seconds and microseconds since the Epoch */
timeval_t
now (void);

/* Compare timeval_t and count of microsecods */
int
tv_usec_cmp (timeval_t __tv, __u64_t __usec);

/* Get difference between two timevals */
timeval_t
timedist (timeval_t __from, timeval_t __to);

/* Run the specified shell command */
int
run_shell_command (const wchar_t *__command);

/* Exit from file manager */
void
do_exit (void);

/* Escaped string */
wchar_t*
escape_string (const wchar_t *__source);

/* Expands '*' characters in pattern string by another string */
wchar_t*
pattern_rename (const wchar_t *__string, const wchar_t *__source);

/* Convert a string to an integer */
long
wtol (const wchar_t *__str);

#endif
