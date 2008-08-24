/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of plugin `file` for VFS
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#define NO_XOPEN_SOURCE /* For use dirent() */

#include "mountlist.h"

#include <dirent.h>
#include <vfs/plugin.c>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>

/*******
 * Macroses
 */

#define FD(_a) (*(int*)(_a))

#define ACTUAL_ERRCODE(_a) \
  ((_a)?(-errno):(0))

/* Template of function which operates with file's name */
#define _FILEOP(_proc, _params...) \
  { \
    if (!__fn) \
      return VFS_ERR_INVLAID_ARGUMENT; \
  \
    size_t len=wcslen (__fn); \
    char *fn=malloc ((len+1)*MB_CUR_MAX); \
    int res=VFS_ERROR; \
  \
    if (wcstombs (fn, __fn, (len+1)*MB_CUR_MAX)!=-1) \
      res=ACTUAL_ERRCODE (_proc (fn, ##_params)); \
  \
    free (fn);\
    return res; \
  }

/* Common part of rename(),symlink() and link() */
#define _RENAME_ENTRY(_proc) \
  if (!__old_path || !__new_path) \
    return VFS_ERR_INVLAID_ARGUMENT; \
 \
  size_t olen=wcslen (__old_path), nlen=wcslen (__new_path); \
  char *opath=malloc ((olen+1)*MB_CUR_MAX); \
  char *npath=malloc ((nlen+1)*MB_CUR_MAX); \
  int res=VFS_ERROR; \
 \
  if (wcstombs (opath, __old_path, (olen+1)*MB_CUR_MAX)!=-1 && \
      wcstombs (npath, __new_path, (nlen+1)*MB_CUR_MAX)!=-1) \
    res=ACTUAL_ERRCODE (_proc (opath, npath)); \
 \
  free (opath); \
  free (npath); \
  return res;


#define SET_ERROR(_errno) \
  if (__error) \
    (*__error)=(_errno);

/********
 * Helpers
 */

/**
 * Check is specified prefix is a prefix of path
 *
 * NOTE: Path is not a prefix of itself
 * NOTE: / is a prefix of any path
 *
 * @param __prefix - prefix to check
 * @param __path - in this path prefix will be checked
 * @return non-zero if __prefix is a prefix of __path and
 * zero otherwise
 */
static BOOL
localfs_is_prefix (const wchar_t *__prefix, const wchar_t *__path)
{
  size_t len;

  if (wcsstr (__path, __prefix) != __path)
    {
      /* __prefix is not a prefix of string __path */
      /* We can return FALSE */

      return FALSE;
    }

  len = wcslen (__prefix);

  /* It should be a directory delimeter afger substring __prefix */
  /* to make it a path prefix */
  if ((__path[len] && __path[len] == '/') || !wcscmp (__prefix, L"/"))
    {
      return TRUE;
    }

  return FALSE;
}

/********
 *
 */

/**
 * Open file. Wrapper for POSIX function open()
 *
 * @param __fn - name of file to open
 * @param __flags - opening flags. See man 2 open for more info
 * @param ... - used for creation mask
 * @return plugin-based descriptor of file
 */
static vfs_plugin_fd_t
localfs_open (const wchar_t *__fn, int __flags, int *__error, ...)
{
  if (!__fn)
    {
      SET_ERROR (VFS_ERR_INVLAID_ARGUMENT);
      return NULL;
    }

  size_t len = wcslen (__fn);
  char *fn = malloc ((len + 1) * MB_CUR_MAX);
  int *res = NULL;

  SET_ERROR (0);

  if (wcstombs (fn, __fn, (len + 1) * MB_CUR_MAX) != -1)
    {
      int mode;
      VFS_GET_MODE (__error, mode);

      int fd = open (fn, __flags, mode);

      if (fd != -1)
        {
          res = malloc (sizeof (int));
          *res = fd;
        }
      else
        {
          SET_ERROR (ACTUAL_ERRCODE (errno));
        }
    }
  else
    {
      SET_ERROR (VFS_ERROR);
    }

  free (fn);

  return res;
}

/**
 * Close file. Wrapper for POSIX function close()
 *
 * @param __fd - descriptor of file, which will be closed
 * @return @return zero on success, non-zero otherwise
 */
static int
localfs_close (vfs_plugin_fd_t __fd)
{
  if (!__fd)
    {
      return -1;
    }

  int res = ACTUAL_ERRCODE (close (FD (__fd)));
  free (__fd);

  return res;
}

/**
 * Read buffer from file. Wrapper for POSIX function read()
 *
 * @param __fd - descriptor of file from which buffer will be read
 * @param __buf - pointer to beffer where data will be stored
 * @param __nbytes - number of bytes to read
 * @param the number of bytes read if succeed, value less than zero otherwise
 */
static vfs_size_t
localfs_read (vfs_plugin_fd_t __fd, void *__buf, vfs_size_t __nbytes)
{
  return read (FD (__fd), __buf, __nbytes);
}

/**
 * Write buffer to file. Wrapper for POSIX function write()
 *
 * @param __fd - descriptor of file where buffer will be written
 * @param __buf - pointer to beffer where data to be written is stored
 * @param __nbytes - number of bytes to write
 * @param the number of bytes written if succeed, value less than zero otherwise
 */
static vfs_size_t
localfs_write (vfs_plugin_fd_t __fd, void *__buf, vfs_size_t __nbytes)
{
  return write (FD (__fd), __buf, __nbytes);
}

/**
 * Delete a name and possibly the file it refers to
 * Wrapper for POSIX function unlink()
 *
 * @param __fn - name of file to be deleted
 * @return zero on success, non-zero otherwise
 */
static int
localfs_unlink (const wchar_t *__fn)
{
  _FILEOP (unlink);
}

/**
 * Create a directory. Wrapper for POSIX function rmdir()
 *
 * @param __fn - name of directory to be created
 * @param __mode - permittions to use
 * @return zero on success, non-zero otherwise
 */
static int
localfs_mkdir (const wchar_t *__fn, vfs_mode_t __mode)
{
  _FILEOP (mkdir, __mode);
}

/**
 * Delete empty directory. Wrapper for POSIX function rmdir()
 *
 * @param __fn - name of directory to be deleted
 * @return zero on success, non-zero otherwise
 */
static int
localfs_rmdir (const wchar_t *__fn)
{
  _FILEOP (rmdir);
}

/**
 * Change a permittions of a file. Wrapper for POSIX function chmod()
 *
 * @param __fn - name of file for which permittons will be set
 * @return zero on success, non-zero otherwise
 */
static int
localfs_chmod (const wchar_t *__fn, vfs_mode_t __mode)
{
  _FILEOP (chmod, __mode);
}

/**
 * Change ownership of a file. Wrapper for POSIX function chown()
 *
 * @param __fn - name of file for which ownership will be set
 * @param __owner - new owner id of file
 * @param __group - new group id of file
 * @return zero on success, non-zero otherwise
 */
static int
localfs_chown (const wchar_t *__fn, vfs_uid_t __owner, vfs_gid_t __group)
{
  _FILEOP (chown, __owner, __group);
}

/**
 * Change the name or location of a file.
 * Wrapper for POSIX function rename()
 *
 * @param __old_path - source location
 * @param __new_path - destination location
 * @return zero on success, non-zero otherwise
 */
static int
localfs_rename (const wchar_t *__old_path, const wchar_t *__new_path)
{
  _RENAME_ENTRY (rename);
}

/**
 * Get file status. Wrapper for POSIX function stat()
 *
 * @param __fn - name of file from which status will be gotten
 * @param __stat - returned status of file
 * @return zero on success, non-zero otherwise
 */
static int
localfs_stat (const wchar_t *__fn, vfs_stat_t *__stat)
{
  _FILEOP (stat, __stat);
}

/**
 * Get file status. Wrapper for POSIX function stat()
 * If __fn is a symbolic link, then link itself is stat-ed
 *
 * @param __fn - name of file from which status will be gotten
 * @param __stat - returned status of file
 * @return zero on success, non-zero otherwise
 */
static int
localfs_lstat (const wchar_t *__fn, vfs_stat_t *__stat)
{
  _FILEOP (lstat, __stat);
}

/**
 * Scan a directory for matching entries. Wrapper for function scandir()
 *
 * @param __path - path to scan
 * @param __name_list - returned array of entries
 * @param __filter - filter of entries (only entries, for which value of
 * filter() returned non-zero will be stored in name list)
 * @param __compar - comparator for sorting entries
 * @return number of entries selected or value less than zero
 * if an error occurs
 */
int
localfs_scandir (const wchar_t *__path, vfs_dirent_t ***__name_list,
                 vfs_filter_proc __filter, vfs_cmp_proc __compar)
{
  if (!__path || !__name_list)
    {
      return VFS_ERR_INVLAID_ARGUMENT;
    }

  size_t len = wcslen (__path);
  char *path = malloc ((len + 1) * MB_CUR_MAX);
  int res = VFS_ERROR;

  (*__name_list) = 0;

  if (wcstombs (path, __path, (len + 1) * MB_CUR_MAX) != -1)
    {
      struct dirent **eps;

      /* Get multibyte-ed names of enries */
      res = scandir (path, &eps, 0, 0);
      if (res > 0)
        {
          int i, count = 0;
          vfs_dirent_t tmp;

          count = 0;
          (*__name_list) = 0;

          for (i = 0; i < res; i++)
            {
              /* Fill coomon part */
              tmp.type = eps[i]->d_type;

              /* Convert file name */
              if (mbstowcs (tmp.name, eps[i]->d_name,
                            VFS_MAX_FILENAME_LEN) == -1)
                {
                  /* If some error had been occurred while converting, */
                  /* free all allocated memory and return -1 */
                  int j;

                  for (j = 0; j < count; j++)
                    {
                      free ((*__name_list)[j]);
                    }
                  free (*__name_list);

                  for (j = i; j < res; j++)
                    {
                      free (eps[j]);
                    }
                  free (eps);

                  return VFS_ERROR;
                }

              /* Apply filter */
              if (!__filter || __filter (&tmp))
                {
                  (*__name_list) = realloc (*__name_list,
                                         (count + 1) * sizeof (vfs_dirent_t*));
                  MALLOC_ZERO ((*__name_list)[count], sizeof (vfs_dirent_t));
                  *(*__name_list)[count] = tmp;
                  count++;
                }

              free (eps[i]);
            }
          SAFE_FREE (eps);

          res = count;

          /* Sort items */
          if (__compar)
            {
              qsort ((*__name_list), res, sizeof (vfs_dirent_t*), __compar);
            }
        }
      else
        {
          res = ACTUAL_ERRCODE (res);
        }
    }

  free (path);
  return res;
}

/**
 * Reposition read/write file offset. Wrapper for POSIX function lseek()
 *
 * @param __fd - descriptor of file in which offset will be changed.
 * @param __offset - offset to set
 * @param __whence - whence __offset if measured:
 *   SEEK_SET - The offset is set to __offset bytes.
 *   SEEK_CUR - The offset is set to its current location plus __offset bytes.
 *   SEEK_END - The offset is set to the size of the file plus __offset bytes.
 * @return if succeed, resulting offset location as measured in bytes from
 * the beginning of the file. Otherwise, a value less than zero is returned.
 */
vfs_offset_t
localfs_lseek (vfs_plugin_fd_t __fd, vfs_offset_t __offset, int __whence)
{
  vfs_offset_t res = lseek (FD (__fd), __offset, __whence);

  if (res == -1)
    {
      res = ACTUAL_ERRCODE (res);
    }

  return res;
}

/**
 * Change access and/or modification times of a file.
 * Wrapper for POSIX function utime()
 *
 * @param __fn - name of file for which times will be changed
 * @param __buf - buffer of times
 * @return @return zero on success, non-zero otherwise
 */
int
localfs_utime (const wchar_t *__fn, const struct utimbuf *__buf)
{
  if (!__fn | !__buf)
    {
      return VFS_ERR_INVLAID_ARGUMENT;
    }

  _FILEOP (utime, __buf);
}

/**
 * Change access and/or modification times of a file.
 * Wrapper for POSIX function utime()
 *
 * @param __fn - name of file for which times will be changed
 * @param __times - buffer of times
 *  __times[0] - Access time
 *  __times[1] - Modification time
 * @return @return zero on success, non-zero otherwise
 */
int
localfs_utimes (const wchar_t *__fn, const struct timeval *__times)
{
  if (!__fn | !__times)
    {
      return VFS_ERR_INVLAID_ARGUMENT;
    }

  _FILEOP (utimes, __times);
}

/**
 * Create a new symbolic link.
 * Wrapper for POSIX function symlink()
 *
 * @param __old_path - name of existing file
 * @param __new_path - name of symbolic link
 * @return zero on success, non-zero otherwise
 */
static int
localfs_symlink (const wchar_t *__old_path, const wchar_t *__new_path)
{
  _RENAME_ENTRY (symlink);
}

/**
 * Create a new hard link.
 * Wrapper for POSIX function link()
 *
 * @param __old_path - name of existing file
 * @param __new_path - name of рфкв link
 * @return zero on success, non-zero otherwise
 */
static int
localfs_link (const wchar_t *__old_path, const wchar_t *__new_path)
{
  _RENAME_ENTRY (link);
}

/**
 * Read value of a symbolic link
 * Wrapper for POSIX function readlink()
 *
 * @param __fn - name of symlink to read
 * @param __buf - buffer where whalue of link will be saved
 * @param __bufsize - size of buffer
 */
static int
localfs_readlink (const wchar_t *__fn, wchar_t *__buf, size_t __bufsize)
{
  if (!__fn || !__buf || !__bufsize)
    {
      return VFS_ERR_INVLAID_ARGUMENT;
    }

  size_t len = wcslen (__fn);
  char *fn = malloc ((len + 1) * MB_CUR_MAX);
  int res = VFS_ERROR;

  /* Conver input widechar filename to multibyte */
  if (wcstombs (fn, __fn, (len + 1) * MB_CUR_MAX) != -1)
    {
      char *mbbuf = malloc (__bufsize + 1);
      res = readlink (fn, mbbuf, __bufsize);

      if (res >= 0)
        {
          mbbuf[res] = 0;
          mbstowcs (__buf, mbbuf, __bufsize);
        }
      else
        {
          res = -errno;
        }

      free (mbbuf);
    }

  free (fn);
  return res;
}

/**
 * Create a special or ordinary file
 * Wrapper for POSIX function mknod()
 *
 * @param __fn - terget file name
 * @param __mode - permittions and type of file
 * @param __dev - specifies the major and minor numbers of the newly
   created device
 */
static int
localfs_mknod (const wchar_t *__fn, vfs_mode_t __mode, vfs_dev_t __dev)
{
  if (!__fn)
    {
      return VFS_ERR_INVLAID_ARGUMENT;
    }

  _FILEOP (mknod, __mode, __dev);
}

/**
 * Get strategy for 'move' operation
 *
 * NOTE: Paths should be normalized
 *
 * @param __src_path - path to source
 * @param __dst_path - path to destination
 * @return VFS_RS_RENAME in case of vfs_rename() will be optimal way for
 * moving. VFS_RS_COPY if we should use stupid copy+unlink for moving.
 * And a value less than zero in case of error.
 */
static int
localfs_move_strategy (const wchar_t *__src_path, const wchar_t *__dst_path)
{

  /*
   * Algorithm: Get list of mounted file systems. Sort it by mount
   *            directory field in reverse mode. Review this list and check:
   *            if source and destination has current directory from list
   *            as a path prefix, then we can say, that source and destination
   *            are on the the same filesystem and we can use vfs_rename().
   *            If only one path from source and destination has current
   *            directory from list as a parent, then source and destination
   *            are on different filesystems. If neither source nor destination
   *            has current path from list as a parent, we should start
   *            reviewing of next path from list
   */

  mountpoint_t **list;
  int i, count, res = VFS_MS_COPY;
  BOOL is_src_prefix, is_dst_prefix;

  /* Comparator for sorter */
  int cmp(const void *__a, const void *__b)
  {
    mountpoint_t *a = *(mountpoint_t**)__a,
                 *b = *(mountpoint_t**)__b;
    return (int)wcslen(b->dir) - (int)wcslen(a->dir);
  }

  /* Get list of mounted filesystems */
  if ((count = vfs_localfs_get_mountlist (&list))<0)
    {
      /* Error occured while getting list of mounted file systems */
      return -1;
    }

  /* Sort list of file systems */
  qsort (list, count, sizeof (mountpoint_t*), cmp);

  /* Re-view list of mounted file systems */
  for (i = 0; i < count; ++i)
    {
      is_src_prefix = localfs_is_prefix (list[i]->dir, __src_path);
      is_dst_prefix = localfs_is_prefix (list[i]->dir, __dst_path);

      if (is_src_prefix && is_dst_prefix)
        {
          res = VFS_MS_RENAME;
          break;
        }

      if (is_src_prefix || is_dst_prefix)
        {
          res = VFS_MS_COPY;
          break;
        }
    }

  vfs_localfs_free_mountlist (list);

  return res;
}

/********
 *
 */

/* Fill information of plugin */
static vfs_plugin_info_t plugin_info = {
  L"localfs",

  0,
  0,

  localfs_open,
  localfs_close,

  localfs_read,
  localfs_write,

  localfs_unlink,

  localfs_mkdir,
  localfs_rmdir,

  localfs_chmod,
  localfs_chown,

  localfs_rename,

  localfs_stat,
  localfs_lstat,

  localfs_scandir,

  localfs_lseek,

  localfs_utime,
  localfs_utimes,

  localfs_symlink,
  localfs_link,
  localfs_readlink,

  localfs_mknod,

  localfs_move_strategy
};

/* Initialize plugin */
VFS_PLUGIN_INIT (plugin_info);
