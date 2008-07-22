/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Error context managing stuff
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _vfs_context_h_
#define _vfs_context_h_

#include "smartinclude.h"

////////
//

int
vfs_context_init                  (void);

void
vfs_context_done                  (void);

void
vfs_context_save                  (wchar_t *__optname1, ...);

void
vfs_context_format                (wchar_t *__format);

#endif
