/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * URL parsers,etc..
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _vfs_url_h_
#define _vfs_url_h_

#include <smartinclude.h>

BEGIN_HEADER

#include "vfs.h"

int
vfs_url_parse                     (const wchar_t *__url,
                                   vfs_plugin_t **__plugin,
                                   wchar_t      **__path);

END_HEADER

#endif
