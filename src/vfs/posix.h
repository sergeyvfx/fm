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

#include "vfs.h"

#include <dirent.h>
#include <utime.h>

BEGIN_HEADER

typedef int (*vfs_filter_proc)    (const vfs_dirent_t*);
typedef int (*vfs_cmp_proc)       (const void*, const void*);

typedef vfs_plugin_fd_t (*vfs_open_proc)   (const wchar_t *__fn,
                                            int            __flags,
                                            int           *__error,
                                            ... /* i.e. used for mode */);

typedef int (*vfs_close_proc)  (vfs_plugin_fd_t __fd);

typedef vfs_size_t (*vfs_read_proc)   (vfs_plugin_fd_t  __fd,
                                       void            *__buf,
                                       vfs_size_t       __nbytes);

typedef vfs_size_t (*vfs_write_proc)  (vfs_plugin_fd_t  __fd,
                                       void            *__buf,
                                       vfs_size_t       __nbytes);

typedef int (*vfs_unlink_proc)    (const wchar_t *__fn);

typedef int (*vfs_rmdir_proc)     (const wchar_t *__path);
typedef int (*vfs_mkdir_proc)     (const wchar_t *__path,
                                   vfs_mode_t     __mode);

typedef int (*vfs_chmod_proc)     (const wchar_t *__fn, vfs_mode_t __mode);
typedef int (*vfs_chown_proc)     (const wchar_t *__fn,
                                   vfs_uid_t      __owner,
                                   vfs_gid_t      __group);

typedef int (*vfs_rename_proc)    (const wchar_t * __old_path,
                                   const wchar_t * __new_path);

typedef int (*vfs_stat_proc)      (const wchar_t *__fn,
                                   vfs_stat_t    *__stat);

typedef int (*vfs_scandir_proc)   (const wchar_t  *__path,
                                   vfs_dirent_t   ***__name_list,
                                   vfs_filter_proc __filter,
                                   vfs_cmp_proc    __compar);

typedef vfs_offset_t (*vfs_lseek_proc) (vfs_plugin_fd_t __fd,
                                        vfs_offset_t    __offset,
                                        int             __whence);

typedef int (*vfs_utime_proc)     (const wchar_t        *__fn,
                                   const struct utimbuf *__buf);

typedef int (*vfs_utimes_proc)    (const wchar_t        *__fn,
                                   const struct timeval *__times);

typedef int (*vfs_symlink_proc)   (const wchar_t *__old_path,
                                   const wchar_t *__new_path);

typedef int (*vfs_link_proc)      (const wchar_t *__old_path,
                                   const wchar_t *__new_path);

typedef int (*vfs_readlink_proc)  (const wchar_t *__fn,
                                   wchar_t       *__buf,
                                   size_t         __bufsize);

END_HEADER

#endif
