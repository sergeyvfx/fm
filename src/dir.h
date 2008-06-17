/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Different stuff providing directory-related operations
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _dir_h_
#define _dir_h_

#include "smartinclude.h"

BEGIN_HEADER

#include "file.h"

#include <dirent.h>

////////
// Type defenitions

typedef int (*dirfilter_proc) (const struct dirent*);
typedef int (*dircmp_proc)    (const void*, const void*);

////////
//

int            // Widecharacter analog for system scandir
wcscandir                         (const wchar_t  *__name,
                                   dirfilter_proc  __filer,
                                   dircmp_proc     __compar,
                                   file_t       ***__res);

wchar_t*       // Concatenate subdirectory to directory name
wcdircatsubdir                    (const wchar_t *__name,
                                   const wchar_t *__subname);

void           // Fit dirname to specified length
fit_dirname                       (const wchar_t *__dir_name,
                                   long           __len,
                                   wchar_t       *__res);

wchar_t*       // Strip non-directory suffix from file name
wcdirname                         (const wchar_t *__name);

int            // Filter for scandir which skips all hidden files
scandir_filter_skip_hidden        (const struct dirent * __data);

int            // Alphabetically sorter for wcscandir
wcscandir_alphasort               (const void *__a, const void *__b);

int            // Separatelly alphabetically sorter for wcscandir
wcscandir_alphasort_sep           (const void *__a, const void *__b);

END_HEADER

#endif
