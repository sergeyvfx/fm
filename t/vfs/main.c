/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * VFS testing stuff
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include <vfs/vfs.h>
#include <stdlib.h>
#include <fcntl.h>

////////
//

#define OK() \
  printf (" ok.\n");

#define FAILED(_msg...) \
  { \
    printf (" failed!\n"); \
    printf (_msg); \
  }

#define TEST(_proc,_args...) \
  printf ("  %s:", #_proc); \
  if ((res=_proc (_args))<0) \
    FAILED ("    %ls\n", vfs_get_error (res)) else \
    OK (); \

////////
//

/**
 * Main testing function
 */
static void
test                              (void)
{
  int res;
  vfs_file_t fd;
  char buf[100]="Hello, World!";

  printf ("* Global testing started\n");

  printf ("  vfs_open:");
  if (!(fd=vfs_open (L"file::/tmp/vfs.test", O_CREAT | O_RDWR, &res)))
    FAILED ("    %ls\n", vfs_get_error (res)) else
    OK ();

  TEST (vfs_write, fd, buf, strlen (buf));
  printf ("    Written buffer: %s\n", buf);
  TEST (vfs_lseek, fd, 0, SEEK_SET);
  memset (buf, 0, sizeof (buf));
  TEST (vfs_read, fd, buf, sizeof (buf));
  printf ("    Read buffer: %s\n", buf);

  printf ("  vfs_close:");
  if ((res=vfs_close (fd)))
    FAILED ("    %ls\n", vfs_get_error (res)) else
    OK ();

  TEST (vfs_unlink, L"file::/tmp/vfs.test");
}

int
main                              (int __argc, char **__argv)
{
  int res;

  printf (">> Testing set for VFS module of ${project-name} <<\n");

  // Initialize all VFS stuff
  if ((res=vfs_init ()))
    {
      printf ("* Error initializing VFS: %ls\n", vfs_get_error (res));
      return EXIT_FAILURE;
    }
  printf ("* VFS initialization succeed.\n");

  // Load standart plugin
  if ((res=vfs_plugin_load (L"./plugins/libfile.so")))
    {
      printf ("* Error loading plugin 'file': %ls\n", vfs_get_error (res));
      return EXIT_FAILURE;
    }
  printf ("* Plugin 'file' loaded successfully\n");

  test ();

  // Uninitializing
  vfs_done ();
  printf ("* VFS uninitialized\n");

  return EXIT_SUCCESS;
}
