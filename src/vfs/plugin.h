/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Plugin support for VFS
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _vfs_plugin_h_
#define _vfs_plugin_h_

#include <smartinclude.h>

BEGIN_HEADER

////////
//

#define VFS_PLUGIN_INIT_ENTRY __vfs_lugin_init_entry__
#define VFS_PLUGIN_INIT_PROC "__vfs_lugin_init_entry__"

////////
// Type defenitions

struct _vfs_plugin_t;
typedef struct _vfs_plugin_t vfs_plugin_t;
typedef void *vfs_plugin_fd_t;

#include "posix.h"

// Wrapper for calls of VFS implementation of POSIX functions
#define VFS_CALL_POSIX(_plugin,_proc,_args...) \
  (((_plugin)->info._proc)?((_plugin)->info._proc (_args)):0)

typedef struct {
  wchar_t *name; // Name of plugin

  ////
  // VFS-ed POSIX functions
  vfs_open_proc  open;
  vfs_close_proc close;

  vfs_read_proc  read;
  vfs_write_proc write;
} vfs_plugin_info_t;

typedef int (*vfs_plugin_init_proc) (vfs_plugin_t*);

struct _vfs_plugin_t {
  void    *dl;  // Handle to dynamic library
  wchar_t *fn;  // Filename of library

  struct {
    vfs_plugin_init_proc init;
  } procs;

  vfs_plugin_info_t info;
};

////////
//

int
vfs_plugins_init                  (void);

void
vfs_plugins_done                  (void);

int
vfs_plugin_load                   (const wchar_t *__file_name);

int
vfs_plugin_unload                 (const wchar_t *__plugin_name);

vfs_plugin_t*
vfs_plugin_by_name                (const wchar_t *__plugin_name);

////////
// For in-plugin usage
#define VFS_PLUGIN_INIT(_info) \
  int VFS_PLUGIN_INIT_ENTRY (vfs_plugin_t *__plugin) \
  { \
    __plugin->info=_info; \
    return VFS_OK; \
  }

END_HEADER

#endif
