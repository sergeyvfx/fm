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
#define VFS_PLUGIN_DELIMETER L"::"

////////
// Type defenitions

struct _vfs_plugin_t;
typedef struct _vfs_plugin_t vfs_plugin_t;
typedef void *vfs_plugin_fd_t;

#include "posix.h"

#define VFS_LS(_a) L##_a

// Wrapper for calls of VFS implementation of POSIX functions
#define VFS_CALL_POSIX_FULL(_plugin,_proc,_err,_args...) \
  (((_plugin)->info._proc)?((_plugin)->info._proc (_args)):(_err))

#define VFS_CALL_POSIX(_plugin,_proc,_args...) \
 VFS_CALL_POSIX_FULL (_plugin,_proc, \
  (vfs_context_save (L"method-name", VFS_LS (#_proc), \
  L"plugin-name", (_plugin)->info.name, 0), \
  VFS_METHOD_NOT_FOUND), ##_args)

#define VFS_CALL_POSIX_PTR(_plugin,_proc,_args...) \
 VFS_CALL_POSIX_FULL (_plugin,_proc,0,##_args)

typedef int (*vfs_plugin_init_proc)     (vfs_plugin_t*);
typedef int (*vfs_plugin_onload_proc)   (void);
typedef int (*vfs_plugin_onunload_proc) (void);

typedef struct {
  wchar_t *name; // Name of plugin

  ////
  // Plugin managment
  vfs_plugin_onload_proc   onload;  // Well be called after plugin is
                                    // initialized

  vfs_plugin_onunload_proc onunload; // Well be called before plugin
                                     // will be unloaded

  ////
  // VFS-ed POSIX functions
  vfs_open_proc      open;
  vfs_close_proc     close;

  vfs_read_proc      read;
  vfs_write_proc     write;

  vfs_unlink_proc    unlink;

  vfs_mkdir_proc     mkdir;
  vfs_rmdir_proc     rmdir;

  vfs_chmod_proc     chmod;
  vfs_chown_proc     chown;

  vfs_rename_proc    rename;

  vfs_stat_proc      stat;
  vfs_stat_proc      lstat;

  vfs_scandir_proc   scandir;

  vfs_lseek_proc     lseek;

  vfs_utime_proc     utime;

  vfs_symlink_proc  symlink;
  vfs_symlink_proc  link;
  vfs_readlink_proc readlink;
} vfs_plugin_info_t;

struct _vfs_plugin_t {
  void    *dl;  // Handle to dynamic library
  wchar_t *fn;  // Filename of library

  struct {
    vfs_plugin_init_proc     init;
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
