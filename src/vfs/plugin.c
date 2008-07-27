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

#include "vfs.h"

#include <wchar.h>
#include <dlfcn.h>
#include <stdlib.h>

#include <hashmap.h>

////////
// Macroses

// Helper for load_symbols()
#define _LOAD_SYMBOL(_sym, _name) \
  __plugin->procs._sym=dlsym (__plugin->dl, VFS_PLUGIN_INIT_PROC); \
  if (!__plugin->procs._sym) \
    return VFS_ERR_PLUGIN_FORMAT;

#define _CALL_HANDLER(_plugin, _callback, _args...) \
  if ((_plugin)->info._callback) \
    (_plugin)->info._callback (##_args);

////////
// Variables

static hashmap_t *plugins=NULL;

////////
// Internal stuff

/**
 * Frees allocated memory and opened descriptors
 *
 * @param __plugin - plugin to bre freed.
 */
static void
free_plugin                      (vfs_plugin_t *__plugin)
{
  if (!__plugin)
    return;

  SAFE_FREE (__plugin->fn);
  if (__plugin->dl)
    dlclose (__plugin->dl);
  SAFE_FREE (__plugin);
}

/**
 * Unloads plugin from system
 *
 * @param __plugin - plugin to be unloaded
 */
static void
unload_plugin                    (vfs_plugin_t *__plugin)
{
  _CALL_HANDLER (__plugin, onunload);
  free_plugin (__plugin);
}

/**
 * Loads symbols from library to plugin descriptor
 *
 * @param __plugin - descriptor of plugin
 * @return zero on success, non-zero otherwise
 */
static int
load_symbols                     (vfs_plugin_t *__plugin)
{
  if (!__plugin || !__plugin->fn)
    return VFS_ERR_INVLAID_ARGUMENT;

  char *mb_name;
  size_t len=(wcslen (__plugin->fn)+2)*MB_CUR_MAX;

  // Allocate memory for multibyte buffer
  MALLOC_ZERO (mb_name, len);

  // Convert wide character file name of library to multibyte string
  if (wcstombs (mb_name, __plugin->fn, (len+1)*MB_CUR_MAX)==-1)
    {
      // Some errors while converting name to wide char
      free (mb_name);
      return VFS_ERROR;
    }

  // Empty library is a parent process
  if (!strcmp (mb_name, ""))
    return VFS_ERR_INVLAID_ARGUMENT;

  // Open library
  __plugin->dl=dlopen (mb_name, RTLD_LAZY);
  if (!__plugin->dl)
    {
      char *mb_err=dlerror ();
      if (mb_err && strlen (mb_err))
        {
          wchar_t *wc_err;
          MBS2WCS (wc_err, mb_err);
          vfs_context_save (L"dl-error", wc_err, 0);
          free (wc_err);
        }
      return VFS_ERR_PLUGIN_LOAD;
    }

  // Getting symbols
  _LOAD_SYMBOL (init, VFS_PLUGIN_INIT_PROC);

  return VFS_OK;
}

/**
 * Creates plugin descriptor from file name of library
 *
 * @param __file_name - name of file, which is a dynamic library
 * @param __error - parameter to report an error code
 * @return new plugin descriptor
 */
static vfs_plugin_t*
plugin_from_file                  (const wchar_t *__file_name,
                                   int *__error)
{
  vfs_plugin_t *plugin;
  int err;

  if (!__file_name)
    return NULL;

  MALLOC_ZERO (plugin, sizeof (vfs_plugin_t));

  //
  // TODO:
  //  But does we really need this information?
  //
  plugin->fn=wcsdup (__file_name);

  if ((err=load_symbols (plugin)))
    goto __failure_;

  // Symbols are loaded
  if (plugin->procs.init (plugin)!=VFS_OK)
    {
      // Error initialising plugin
      err=VFS_ERR_PLUGIN_INIT;
      goto __failure_;
    }

  return plugin;

__failure_:
  free_plugin (plugin);

  // Set error code
  if (__error)
    (*__error)=err;
  return NULL;
}

static void
plugin_deleter                    (void *__plugin)
{
  unload_plugin (__plugin);
}

////////
// User's backend

int
vfs_plugins_init                  (void)
{
  plugins=hashmap_create_wck (plugin_deleter, HM_MAGICK_LEN);

  return VFS_OK;
}

void
vfs_plugins_done                  (void)
{
  hashmap_destroy (plugins);
}

/**
 * Loads VFS plugin
 *
 * @param __file_name - name of file, which is a dynamic library
 * @return zero on success, non-zero otherwise
 */
int
vfs_plugin_load                   (const wchar_t *__file_name)
{
  vfs_plugin_t *plugin;
  int err=0;

  if (!__file_name)
    return VFS_ERR_INVLAID_ARGUMENT;

  plugin=plugin_from_file (__file_name, &err);

  if (!plugin)
    {
      // Some error occured - return error code
      return err;
    }

  _CALL_HANDLER (plugin, onload);

  // Register plugin in hash map
  hashmap_set (plugins, plugin->info.name, plugin);

  return VFS_OK;
}

/**
 * Unloads plugin with specified name
 *
 * @param __plugin_name - name of plugin to be unloaded
 * @return zero on success, non-zero otherwise
 */
int
vfs_plugin_unload                 (const wchar_t *__plugin_name)
{
  unload_plugin (vfs_plugin_by_name (__plugin_name));
  return VFS_OK;
}

/**
 * Returns descriptor of plugin with specified name
 *
 * @param __plugin_name - name of plugin
 * @return descriptor of plugin if succeed, NULL otherwise
 */
vfs_plugin_t*
vfs_plugin_by_name                (const wchar_t *__plugin_name)
{
  if (!__plugin_name)
    return NULL;

  return hashmap_get (plugins, __plugin_name);
}
