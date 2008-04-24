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

wchar_t*       // Fit string to specified length
wcsfit                            (const wchar_t *__str, size_t __len);

#endif
