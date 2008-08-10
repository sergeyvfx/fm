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

/*
 * !!WARINIG!!
 *  This will work properly ONLY if parent context
 *  has an implementation of hash maps
 *  wcsndup must be also implemented in parent context
 */

BEGIN_HEADER

#include "errcode.h"

#include <sys/stat.h>

#define VFS_MAX_FILENAME_LEN 256

/********
 * Common types
 */

typedef size_t vfs_size_t;
typedef off_t vfs_offset_t;
typedef mode_t vfs_mode_t;
typedef uid_t vfs_uid_t;
typedef gid_t vfs_gid_t;
typedef struct stat vfs_stat_t;

typedef struct
{
  unsigned char type;
  wchar_t name[VFS_MAX_FILENAME_LEN];
} vfs_dirent_t;

/********
 * Plugins
 */

#include "plugin.h"

/********
 * Type defenitions
 */

typedef struct
{
  vfs_plugin_t *plugin;
  vfs_plugin_fd_t plugin_data;
} *vfs_file_t;

/********
 * Macros
 */
#define VFS_GET_MODE(_last, _mode) \
  { \
    va_list args; \
    va_start (args, _last); \
    (_mode)=va_arg (args, int); \
    va_end (args); \
  }

#define vfs_free_dirent(_a) \
  SAFE_FREE (_a)

/********
 * Common stuff
 */

int
vfs_init (void);

void
vfs_done (void);

/********
 * VFS abstraction
 */

vfs_file_t
vfs_open (const wchar_t *__url, int __flags, int *__error, ...);

int
vfs_close (vfs_file_t __file);

vfs_size_t
vfs_read (vfs_file_t __file, void *__buf, vfs_size_t __nbytes);

vfs_size_t
vfs_write (vfs_file_t __file, void *__buf, vfs_size_t __nbytes);

int
vfs_unlink (const wchar_t *__url);

int
vfs_mkdir (const wchar_t *__path, vfs_mode_t __mode);

int
vfs_rmdir (const wchar_t *__url);

int
vfs_chmod (const wchar_t *__url, vfs_mode_t __mode);

int
vfs_chown (const wchar_t *__url, vfs_uid_t __owner, vfs_gid_t __group);

int
vfs_rename (const wchar_t *__old_url, const wchar_t *__new_path);

int
vfs_stat (const wchar_t *__url, vfs_stat_t *__stat);

int
vfs_lstat (const wchar_t *__url, vfs_stat_t *__stat);

int
vfs_scandir (const wchar_t *__url, vfs_dirent_t ***__name_list,
             vfs_filter_proc __filter, vfs_cmp_proc __compar);

int
vfs_lseek (vfs_file_t __file, vfs_offset_t __offset, int __whence);

int
vfs_utime (const wchar_t *__url, const struct utimbuf *__buf);

int
vfs_utimes (const wchar_t *__url, const struct timeval *__times);

int
vfs_symlink (const wchar_t *__old_url, const wchar_t *__new_url);

int
vfs_link (const wchar_t *__old_url, const wchar_t *__new_path);

int
vfs_readlink (const wchar_t *__url, wchar_t *__buf, size_t __bufsize);


/********
 * Different utilities
 */

int
vfs_alphasort (const void *__a, const void *__b);

/********
 *
 */

#include "context.h"

wchar_t*
vfs_get_error (int __errcode);

END_HEADER

#endif
