/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Virtual File System support
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */
 
#ifndef _vfs_h_
#define _vfs_h_

#include <smartinclude.h>

//
// !!WARINIG!!
//  This will work properly ONLY if parent context
//  has an implementation of hash maps
//  wcsndup must be also implemented in parent context
//

BEGIN_HEADER

#include "errcode.h"

typedef size_t vfs_size_t;
typedef vfs_size_t vfs_offset_t;

////////
// Plugins

#include "plugin.h"

////////
// Type defenitions

typedef struct {
  vfs_plugin_t    *plugin;
  vfs_plugin_fd_t  plugin_data;
} *vfs_file_t;

////////
// Macros
#define VFS_GET_MODE(_last, _mode) \
  { \
    va_list args; \
    va_start (args, __flags); \
    (_mode)=va_arg (args, int); \
    va_end (args); \
  }

////////
// Common stuff

int
vfs_init                          (void);

void
vfs_done                          (void);

////////
// VFS abstraction

vfs_file_t
vfs_open                          (const wchar_t *__url, int __flags, ...);

int
vfs_close                         (vfs_file_t __file);

vfs_size_t
vfs_read                          (vfs_file_t  __file,
                                   void       *__buf,
                                   vfs_size_t  __nbytes);

vfs_size_t
vfs_write                         (vfs_file_t  __file,
                                   void       *__buf,
                                   vfs_size_t  __nbytes);

END_HEADER

#endif
