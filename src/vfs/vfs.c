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
#include "context.h"

///////
// Macroses

#define INIT_ITER(_proc,_args...) \
  if ((res=_proc (##_args))) \
    return res;

#define SET_ERROR(_errno) \
  if (__error) \
    (*__error)=(_errno);

// Template of function which operates with file's URL
#define _FILEOP(_proc, _params...) \
  { \
    int res; \
    if (!__url) \
      return VFS_ERR_INVLAID_ARGUMENT; \
   \
    vfs_plugin_t *plugin; \
    wchar_t *path; \
   \
    if (!(res=vfs_url_parse (__url, &plugin, &path))) \
      { \
        res=VFS_CALL_POSIX (plugin, _proc, path, ##_params); \
        free (path); \
        return res; \
      } \
   \
    return res; \
  }

// Common part of rename(),symlink() and link()
#define _RENAME_ENTRY(_proc) \
  { \
    int res; \
    if (!__old_url | !__new_path) \
      return VFS_ERR_INVLAID_ARGUMENT; \
    \
    vfs_plugin_t *plugin; \
    wchar_t *old_path; \
   \
    if (!(res=vfs_url_parse (__old_url, &plugin, &old_path))) \
      { \
        res=VFS_CALL_POSIX (plugin, _proc, old_path, __new_path); \
        free (old_path); \
        return res; \
      } \
    return res; \
  }


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

  // Initialize error context
  INIT_ITER (vfs_context_init);

  // Initialize plugins
  INIT_ITER (vfs_plugins_init);

  return VFS_OK;
}

/**
 * Uninitializes VFS stuff
 */
void
vfs_done                          (void)
{
  vfs_plugins_done ();
  vfs_context_done ();
}

////////
// VFS abstraction

/**
 * Abstraction for POSIX function open()
 * Opens and possibly creates a file
 *
 * @param __url - url of file to open
 * @param __flags - flags of opening file
 * @param ... - mode of new file is stored here
 * @return file descriptor if operation succeed, NULL othervise
 */
vfs_file_t
vfs_open                          (const wchar_t *__url,
                                   int            __flags,
                                   int           *__error,
                                   ...)
{
  if (!__url)
    {
      SET_ERROR (VFS_ERR_INVLAID_ARGUMENT);
      return NULL;
    }

  vfs_plugin_t *plugin;
  wchar_t *path;
  int res;

  SET_ERROR (0);

  if (!(res=vfs_url_parse (__url, &plugin, &path)))
    {
      int mode;
      VFS_GET_MODE (__error, mode);
      vfs_plugin_fd_t *data;

      data=VFS_CALL_POSIX_PTR (plugin, open, path, __flags, __error, mode);

      free (path);

      if (!data)
        return NULL;

      return spawn_new_file_info (plugin, data);
    } else
      SET_ERROR (res);

  return NULL;
}

/**
 * Abstraction for POSIX function close()
 * Closes a file descriptor
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
 * Reads from a file descriptor
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
 * Writes to a file descriptor
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

/**
 * Abstraction for POSIX function unlink()
 * Deletes a name and possibly the file it refers to
 *
 * @param __url - url of file to unlink
 * @return zero on success, non-zero otherwise
 */
int
vfs_unlink                        (const wchar_t *__url)
{
  _FILEOP (unlink);
}

/**
 * Abstraction for POSIX function mkdir()
 * Creates a directory
 *
 * @param __url - url of directory to be deleted
 * @param __mode - permittions to use
 * @return zero on success, non-zero otherwise
 */
int
vfs_mkdir                         (const wchar_t *__url,
                                   vfs_mode_t     __mode)
{
  _FILEOP (mkdir, __mode);
}

/**
 * Abstraction for POSIX function rmdir()
 * Deletes a directory, which must be empty
 *
 * @param __url - url of directory to be deleted
 * @return zero on success, non-zero otherwise
 */
int
vfs_rmdir                         (const wchar_t *__url)
{
  _FILEOP (rmdir);
}

/**
 * Abstraction for POSIX function rmdir()
 * Changes a permittions of a file
 *
 * @param __url - url of file for which permittions will be set
 * @return zero on success, non-zero otherwise
 */
int
vfs_chmod                         (const wchar_t *__url, vfs_mode_t __mode)
{
  _FILEOP (chmod, __mode);
}

/**
 * Abstraction for POSIX function chown()
 * Сhange ownership of a file
 *
 * @param __url - url of file for which щцтукыршз will be changed
 * @param __owner - new owner id of file
 * @param __group - new group id of file
 * @return zero on success, non-zero otherwise
 */
int
vfs_chown                         (const wchar_t *__url,
                                   vfs_uid_t      __owner,
                                   vfs_gid_t      __group)
{
  _FILEOP (chown, __owner, __group);
}

/**
 * Abstraction for POSIX function rename()
 * Changes the name or location of a file
 *
 * NOTE:
 *  Moving files to other plugins isn't supported
 *
 * @param __old_url - source url
 * @param __new_path - destination location
 * @return zero on success, non-zero otherwise
 */
int
vfs_rename                        (const wchar_t *__old_url,
                                   const wchar_t *__new_path)
{
  _RENAME_ENTRY (rename);
}

/**
 * Abstraction for POSIX function stat()
 * Gets file status
 *
 * @param __url - url of file from which status will be gotten
 * @param __stat - returned status of file
 * @return zero on success, non-zero otherwise
 */
int
vfs_stat                          (const wchar_t *__url, vfs_stat_t *__stat)
{
  _FILEOP (stat, __stat);
}

/**
 * Abstraction for POSIX function lstat()
 * Gets file status
 * If __urkl is a symbolic link, then link itself is stat-ed
 *
 * @param __url - url of file from which status will be gotten
 * @param __stat - returned status of file
 * @return zero on success, non-zero otherwise
 */
int
vfs_lstat                         (const wchar_t *__url, vfs_stat_t *__stat)
{
  _FILEOP (lstat, __stat);
}

/**
 * Abstraction for function scandir()
 * Scans a directory for matching entries
 *
 * @param __url - url to scan
 * @param __name_list - returned array of entries
 * @param __filter - filter of entries (only entries, for which value of
 * filter() returned non-zero will be stored in name list)
 * @param __compar - comparator for sorting entries
 * @return number of entries selected or value less than zero
 * if an error occurs
 */
int
vfs_scandir                       (const wchar_t  *__url,
                                   vfs_dirent_t   ***__name_list,
                                   vfs_filter_proc __filter,
                                   vfs_cmp_proc    __compar)
{
  _FILEOP (scandir, __name_list, __filter, __compar);
}

/**
 * Abstraction for POSIX function lseek()
 * Repositions read/write file offset.
 *
 * @param __file - descriptor of file in which offset will be changed.
 * @param __offset - offset to set
 * @param __whence - whence __offset if measured:
 *   SEEK_SET - The offset is set to __offset bytes.
 *   SEEK_CUR - The offset is set to its current location plus __offset bytes.
 *   SEEK_END - The offset is set to the size of the file plus __offset bytes.
 * @return if succeed, resulting offset location as measured in bytes from
 * the beginning of the file. Otherwise, a value less than zero is returned.
 */
int
vfs_lseek                         (vfs_file_t   __file,
                                   vfs_offset_t __offset,
                                   int          __whence)
{
  if (!__file)
    return VFS_ERR_INVLAID_ARGUMENT;

  return VFS_CALL_POSIX (__file->plugin, lseek, __file->plugin_data,
    __offset, __whence);
}

/**
 * Abstraction for POSIX function utime()
 * Change access and/or modification times of a file.
 *
 * @param __url - url to scan
 * @param __buf - buffer of times
 * @return zero on success, non-zero otherwise
 */
int
vfs_utime                         (const wchar_t        *__url,
                                   const struct utimbuf *__buf)
{
  _FILEOP (utime, __buf);
}

/**
 * Abstraction for POSIX function utimes()
 * Change access and/or modification times of a file.
 *
 * @param __url - url to scan
 * @param __times - buffer of times
 *  __times[0] - Access time
 *  __times[1] - Modification time
 * @return zero on success, non-zero otherwise
 */
int
vfs_utimes                        (const wchar_t        *__url,
                                   const struct timeval *__times)
{
  _FILEOP (utimes, __times);
}

/**
 * Abstraction for POSIX function symlink()
 * Creates a new symbolic link.
 *
 * @param __old_url - url of existing file
 * @param __new_url - name of symbolic link
 * @return zero on success, non-zero otherwise
 */
int
vfs_symlink                       (const wchar_t *__old_url,
                                   const wchar_t *__new_url)
{
  int res;
  vfs_plugin_t *plugin;
  wchar_t *new_url;

  if (!__old_url | !__new_url)
    return VFS_ERR_INVLAID_ARGUMENT;

  if (!(res=vfs_url_parse (__new_url, &plugin, &new_url)))
    {
      res=VFS_CALL_POSIX (plugin, symlink, __old_url, new_url);
      free (new_url);
      return res;
    }
  return res;
}

/**
 * Abstraction for POSIX function link()
 * Creates a new hard link.
 *
 * @param __old_url - url of existing file
 * @param __new_path - name of hard link
 * @return zero on success, non-zero otherwise
 */
int
vfs_link                          (const wchar_t *__old_url,
                                   const wchar_t *__new_path)
{
  _RENAME_ENTRY (link);
}

/**
 * Abstraction for POSIX function readlink()
 * Reads value of a symbolic link
 *
 * @param __url - url of symbolic link to read
 * @param __buf - buffer where result will be saved
 * @paran __bufsize - size of buffer
 * @return the count of characters placed in the buffer if succeed,
 * otherwise an error code.
 */
int
vfs_readlink                      (const wchar_t *__url,
                                   wchar_t       *__buf,
                                   size_t         __bufsize)
{
  _FILEOP (readlink, __buf, __bufsize);
}
