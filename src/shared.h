/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Working with shared directories and other data
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _shared_h_
#define _shared_h_

#include "smartinclude.h"

#include <pwd.h>

BEGIN_HEADER

/* Get list of files in shared directory */
long
get_shared_listing (const wchar_t *__dir, const wchar_t *__home_replacer,
                    wchar_t ***__list);

/* Get list of shared files */
int
get_shared_files (const wchar_t *__relative_name,
                  const wchar_t *__home_replacer, wchar_t ***__list);

END_HEADER

#endif
