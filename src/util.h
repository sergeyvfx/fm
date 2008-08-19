/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Different helpers
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _util_h_
#define _util_h_

#include "smartinclude.h"

#include <wchar.h>

/* Fit string to specified width */
wchar_t*
wcsfit (const wchar_t *__str, size_t __width, const wchar_t *__suffix);

/* Copies at most n characters of string */
wchar_t*
wcsndup (const wchar_t *__s, size_t __n);

/* Replace substrings of string */
void
wcsrep (wchar_t *__str, size_t __max_len,
        const wchar_t *__substr, const wchar_t *__newsubstr);

/* Get the number of seconds and microseconds since the Epoch */
timeval_t
now (void);

/* Compare timeval_t and count of microsecods */
int
tv_usec_cmp (timeval_t __tv, __u64_t __usec);

/* Get difference between two timevals */
timeval_t
timedist (timeval_t __from, timeval_t __to);

#endif
