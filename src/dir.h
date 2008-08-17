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
#include <vfs/vfs.h>

#include <dirent.h>

/********
 * Macro defenitions
 */

/* Check if _dir is a name of pseudo-directory */
#define IS_PSEUDODIR(_dir) \
  (wcscmp (_dir, L".") == 0 || wcscmp (_dir, L"..") == 0)

/********
 * Type definitions
 */

typedef int (*dircmp_proc) (const void*, const void*);

/********
 *
 */

/* Wrapper of VFS function vfs_scandir() */
int
wcscandir (const wchar_t *__name, vfs_filter_proc __filer,
           dircmp_proc __compar, file_t ***__res);

/* Concatenate subdirectory to directory name */
wchar_t*
wcdircatsubdir (const wchar_t *__name, const wchar_t *__subname);

/* Fit dirname to specified length */
void
fit_dirname (const wchar_t *__dir_name, long __len, wchar_t *__res);

/* Strip non-directory suffix from file name */
wchar_t*
wcdirname (const wchar_t *__name);

/* Filter for scandir which skips all hidden files */
int
scandir_filter_skip_hidden (const vfs_dirent_t * __data);

/* Alphabetically sorter for wcscandir */
int
wcscandir_alphasort (const void *__a, const void *__b);

/* Separately alphabetically sorter for wcscandir */
int
wcscandir_alphasort_sep (const void *__a, const void *__b);

/* Is URL points to a directory? */
BOOL
isdir (const wchar_t *__url);

END_HEADER

#endif
