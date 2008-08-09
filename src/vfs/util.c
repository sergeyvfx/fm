/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Different utilities for Virtual File System
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "vfs.h"

#include <wchar.h>

/**
 * Alphabetically sorter for vfs_scandir which sorts files and directories
 *
 * @param __a - left element of array
 * @param __b - right element of array
 * @return an integer less than, equal to, or greater than zero if the
 * first argument is considered to be respectively less than, equal to,
 * or greater than the second.
 */
int
vfs_alphasort                     (const void *__a, const void *__b)
{
  vfs_dirent_t *a=*(vfs_dirent_t**)__a, *b=*(vfs_dirent_t**)__b;

  if (!wcscmp (a->name, L".") || !wcscmp (a->name, L".."))
    return -1;

  return wcscmp (a->name, b->name);
}
