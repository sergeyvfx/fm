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

#define VFS_ERR_COMMON             (-1024)
#define VFS_ERROR                  (VFS_ERR_COMMON)
#define VFS_ERR_INVLAID_ARGUMENT   (VFS_ERR_COMMON-1)
#define VFS_METHOD_NOT_FOUND       (VFS_ERR_COMMON-2)

#define VFS_ERR_PLUGIN             (VFS_ERR_COMMON-128)
#define VFS_PLUGIN_INVALID_FORMAT  (VFS_ERR_PLUGIN)
#define VFS_PLUGIN_INIT_ERROR      (VFS_ERR_PLUGIN-1)
#define VFS_ERR_PUGIN_NOT_FOUND    (VFS_ERR_PLUGIN-2)
#define VFS_ERR_INVALID_URL        (VFS_ERR_PLUGIN-3)

#endif
