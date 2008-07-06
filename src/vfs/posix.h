/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * POSIX-like defenitions of VFS plugin functions
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _vfs_posix_h_
#define _vfs_posix_h_

#include <smartinclude.h>

BEGIN_HEADER

typedef vfs_plugin_fd_t (*vfs_open_proc)   (const wchar_t *__fn,
                                            int __flags,
                                            ... /* i.e. used for mode */);

typedef int (*vfs_close_proc)  (vfs_plugin_fd_t __fd);

typedef vfs_size_t (*vfs_read_proc)   (vfs_plugin_fd_t  __fd,
                                       void            *__buf,
                                       vfs_size_t       __nbytes);

typedef vfs_size_t (*vfs_write_proc)  (vfs_plugin_fd_t  __fd,
                                       void            *__buf,
                                       vfs_size_t       __nbytes);

END_HEADER

#endif
