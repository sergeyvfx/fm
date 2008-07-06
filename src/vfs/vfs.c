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

#include "vfs.h"
#include "url.h"

////////
//

/**
 * Creates information for file descriptor
 *
 * @param __plugin - responsible plugin
 * @param __plugin_data - plugin's data associated with this file descriptor
 * @return created information
 */
static vfs_file_t
spawn_new_file_info               (const vfs_plugin_t *__plugin,
                                   const vfs_plugin_fd_t __plugin_data)
{
  vfs_file_t res;
  MALLOC_ZERO (res, sizeof (vfs_file_t));

  res->plugin=(vfs_plugin_t*)__plugin;
  res->plugin_data=(vfs_plugin_fd_t*)__plugin_data;

  return res;
}

/**
 * Frees allocated file information
 *
 * @param __info - information to be freed
 */
static void
free_file_info                  (vfs_file_t __info)
{
  if (!__info)
    return;
  free (__info);
}


////////
// Common stuff

/**
 * Initializes VFS stuff
 *
 * @return zero on success, non-zero otherwise
 */
int
vfs_init                          (void)
{
  int res;

  // Initialize plugins
  if ((res=vfs_plugins_init ()))
    // Error initializing plugins
    return res;

  return VFS_OK;
}

/**
 * Uninitializes VFS stuff
 */
void
vfs_done                          (void)
{
  vfs_plugins_done ();
}

////////
// VFS abstraction

/**
 * Abstraction for POSIX function open()
 *
 * @param __url - url to file to open
 * @param __flags - flags of opening file
 * @param ... - mode of new file is stored here
 * @return file descriptor if operation succeed, NULL othervise
 */
vfs_file_t
vfs_open                          (const wchar_t *__url, int __flags, ...)
{
  if (!__url)
    return NULL;

  vfs_plugin_t *plugin;
  wchar_t *path;

  if (!vfs_url_parse (__url, &plugin, &path))
    {
      int mode;
      VFS_GET_MODE (__flags, mode);
      vfs_plugin_fd_t *data;

      data=VFS_CALL_POSIX (plugin, open, path, __flags, mode);

      free (path);

      if (!data)
        return NULL;

      return spawn_new_file_info (plugin, data);
    }

  return NULL;
}

/**
 * Abstraction for POSIX function close()
 *
 * @param __file - descriptor of file to be closed
 * @return zero on success, non-zero otherwise
 */
int
vfs_close                         (vfs_file_t __file)
{
  if (!__file)
    return VFS_ERR_INVLAID_ARGUMENT;

  VFS_CALL_POSIX (__file->plugin, close, __file->plugin_data);

  free_file_info (__file);

  return VFS_OK;
}

/**
 * Abstraction for POSIX function read()
 *
 * @param __file - descriptor of file from which buffer will be read
 * @return on success, the number of read bytes written, otherwise
 * a value, less than zero is returned
 */
vfs_size_t
vfs_read                          (vfs_file_t  __file,
                                   void       *__buf,
                                   vfs_size_t  __nbytes)
{
  if (!__file)
    return VFS_ERR_INVLAID_ARGUMENT;

  return VFS_CALL_POSIX (__file->plugin, read, __file->plugin_data,
    __buf, __nbytes);
}

/**
 * Abstraction for POSIX function write()
 *
 * @param __file - descriptor of file into which buffer will be written
 * @return on success, the number of written bytes written, otherwise
 * a value, less than zero is returned
 */
vfs_size_t
vfs_write                         (vfs_file_t  __file,
                                   void       *__buf,
                                   vfs_size_t  __nbytes)
{
  if (!__file)
    return VFS_ERR_INVLAID_ARGUMENT;

  return VFS_CALL_POSIX (__file->plugin, write, __file->plugin_data,
    __buf, __nbytes);
}
