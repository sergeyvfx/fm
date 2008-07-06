/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Defenitions of error codes
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _vfs_errcode_h_
#define _vfs_errcode_h_

#define VFS_OK                     0

#define VFS_ERR_COMMON             (-1)
#define VFS_ERROR                  (VFS_ERR_COMMON)
#define VFS_ERR_INVLAID_ARGUMENT   (VFS_ERR_COMMON-1)

#define VFS_ERR_PLUGIN             (-10)
#define VFS_PLUGIN_INVALID_FORMAT  (VFS_ERR_PLUGIN)
#define VFS_PLUGIN_INIT_ERROR      (VFS_ERR_PLUGIN-1)

#endif
