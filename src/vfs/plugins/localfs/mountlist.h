/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Mountlist manipulating module
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _vfs_localfs_mountlist_h_
#define _vfs_localfs_mountlist_h_

#include <smartinclude.h>

BEGIN_HEADER

typedef struct
{
  wchar_t *fsname;
  wchar_t *dir;
  wchar_t *type;
  wchar_t *opts;
  int freq;
  int passno;
} mountpoint_t;

/* Get list of mounted file systems */
int
vfs_localfs_get_mountlist (mountpoint_t ***__list);

/* Free list of mounted file systems */
void
vfs_localfs_free_mountlist (mountpoint_t **__list);

END_HEADER

#endif
