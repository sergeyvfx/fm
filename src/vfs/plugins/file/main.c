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

#include <vfs/plugin.c>

#include <fcntl.h>
#include <unistd.h>

////////
// Macroses

#define FD(_a) (*(int*)(_a))

////////
//

/**
 * Opens file. Wrapper for POSIX function open()
 *
 * @param __fn - name of file to open
 * @param __flags - opening flags. See man 2 open for more info
 * @param ... - used for creation mask
 * @return plugin-based descriptor of file
 */
static vfs_plugin_fd_t
file_open                         (const wchar_t *__fn, int __flags, ...)
{
  printf ("file: Opening file %ls\n", __fn);

  size_t len=wcslen (__fn);
  char *fn=malloc ((len+1)*MB_CUR_MAX);
  int *res=NULL;

  if (wcstombs (fn, __fn, len)!=-1)
    {
      int mode;
      VFS_GET_MODE (__flags, mode);

      int fd=open (fn, __flags, mode);
      if (fd!=-1)
        {
          res=malloc (sizeof (int));
          *res=fd;
        }
    }

  free (fn);

  return res;
}

/**
 * Closes file. Wrapper for POSIX function close()
 *
 * @param __fd - descriptor of file, which will be closed
 * @return @return zero on success, non-zero otherwise
 */
static int 
file_close                        (vfs_plugin_fd_t __fd)
{
  printf ("file: Closing file %ld\n", (long int)__fd);

  if (!__fd)
    return -1;

  int res=close (FD (__fd));
  free (__fd);

  return res;
}

/**
 * Reads buffer from file. Wrapper for POSIX function read()
 *
 * @param __fd - descriptor of file from which buffer will be read
 * @param __buf - pointer to beffer where data will be stored
 * @param __nbytes - number of bytes to read
 * @param the number of bytes read if succeed, value less than zero otherwise
 */
static vfs_size_t
file_read                         (vfs_plugin_fd_t  __fd,
                                   void            *__buf,
                                   vfs_size_t       __nbytes)
{
  printf ("file: Reading from file %ld\n", (long int)__fd);

  return read (FD (__fd), __buf, __nbytes);
}

/**
 * Writes buffer to file. Wrapper for POSIX function write()
 *
 * @param __fd - descriptor of file where buffer will be written
 * @param __buf - pointer to beffer where data to be written is stored
 * @param __nbytes - number of bytes to write
 * @param the number of bytes written if succeed, value less than zero otherwise
 */
static vfs_size_t
file_write                        (vfs_plugin_fd_t  __fd,
                                   void            *__buf,
                                   vfs_size_t       __nbytes)
{
  printf ("file: Writing to file %ld\n", (long int)__fd);

  return write (FD (__fd), __buf, __nbytes);
}

////////
//

// Fill information of plugin
static vfs_plugin_info_t plugin_info = {
  L"file",

  file_open,
  file_close,

  file_read,
  file_write
};

// Initialize plugin
VFS_PLUGIN_INIT (plugin_info);
