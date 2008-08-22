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
#include <unistd.h>

/********
 *
 */

#define ARG_TEST_BOOL(__arg_name, __var) \
  if (strcmp (__argv[i], __arg_name) == 0) \
    { \
      __var = TRUE; \
    }

#define OK() \
  printf (" ok.\n");

#define FAILED(_msg...) \
  { \
    printf (" failed!\n"); \
    printf (_msg); \
    if (!test_all) \
      { \
        return -1; \
      } \
  }

#define TEST(_proc,_args...) \
  printf ("  %s:", #_proc); \
  if ((res=_proc (_args))<0) \
    FAILED ("    %ls\n", vfs_get_error (res)) else \
    OK (); \

/********
 *
 */

static BOOL test_all   = FALSE,
            test_open  = FALSE,
            test_write = FALSE,
            test_read = FALSE,
            test_lseek = FALSE,
            test_close = FALSE,
            test_unlink = FALSE;

/**
 * Tester for vfs_open() and other functions,
 * related to file descriptor.
 *
 * @return zero on success, non-zero otherwise
 */
static int
vfs_open_test (void)
{
  vfs_file_t fd = 0;
  int res;
  char buf[100] = "Hello, World!";

  if (test_all || test_open || test_write || test_read ||
      test_close || test_lseek)
    {
      printf ("  vfs_open:");
      if (!(fd = vfs_open (L"localfs::/tmp/vfs.test", O_CREAT | O_RDWR,
                           &res, 0664)))
        {
          FAILED ("    %ls\n", vfs_get_error (res));
          return -1;
        }
      else
        {
          OK ();
        }
    }

  if (test_all || test_write || test_read || test_lseek)
    {
      printf ("  vfs_write:");
      if ((res = vfs_write (fd, buf, strlen (buf))) < 0)
        {
          FAILED ("    %ls\n", vfs_get_error (res));
          return -1;
        }
      else
        {
          /* Check written buffer to be equal to source buffer */
          char new_buf[100] = {0};
          int f, r;

          f = open ("/tmp/vfs.test", 0);
          if (!f)
            {
              FAILED ("    Could not open file for checking\n");
              return -1;
            }
          r = read (f, new_buf, sizeof (new_buf));
          close (f);

          if (r != strlen (buf) || memcmp (buf, new_buf, r))
            {
              FAILED ("    Written and readed buffer are not equal\n");
              return -1;
            }

          OK ();
        }
    }

  if (test_all || test_lseek)
    {
      int off = 2;
      printf ("  vfs_lseek:");
      if ((res = vfs_lseek (fd, off, SEEK_SET)) == -1)
        {
          FAILED ("    %ls\n", vfs_get_error (res));
          return -1;
        }
      else
        {
          char tmp_buf[100] = {0};

          if ((res = vfs_read (fd, tmp_buf, sizeof (tmp_buf))) < 0)
            {
              FAILED ("    Error read buffer for checking: %ls\n",
                      vfs_get_error (res));
              return -1;
            }

          if (strcmp (tmp_buf, buf + off))
            {
              FAILED ("    Got incorrect buffer\n");
              return -1;
            }

          OK ();
        }
    }

  if (test_all || test_read)
    {
      char tmp_buf[100] = {0};
      printf ("  vfs_read:");

      /* Move to start of file */
      vfs_lseek (fd, 0, SEEK_SET);
      if ((res = vfs_read (fd, tmp_buf, sizeof (tmp_buf))) < 0)
        {
          FAILED ("    %ls\n", vfs_get_error (res));
          return -1;
        }
      else
        {
          /* Compare source and read buffers */
          if (res != strlen (buf) || memcmp (buf, tmp_buf, res))
            {
              FAILED ("    Read incorrect buffer\n");
              return -1;
            }

          OK ();
        }
    }

  if (fd)
    {
      printf ("  vfs_close:");
      if ((res = vfs_close (fd)))
        {
          FAILED ("    %ls\n", vfs_get_error (res));
          return -1;
        }
      else
        {
          OK ();
        }
    }
  else
    {
      /* Just hope that this function is not buggy */
      if (fd)
        {
          vfs_close (fd);
        }
    }

  unlink ("/tmp/vfs.test");

  return 0;
}

/**
 * Tester for vfs_unlink() function
 *
 * @return zero on success, non-zero otherwise
 */
static int
vfs_unlink_test (void)
{
  int res;
  FILE *f;

  if (test_all || test_unlink)
    {
      /* Create file */
      f = fopen ("/tmp/vfs.unlink", "w");
      fprintf (f, "Hello, World!");
      fclose (f);

      printf ("  vfs_unlink:");
      if ((res = vfs_unlink (L"localfs::/tmp/vfs.unlink")))
        {
          unlink ("/tmp/vfs.unlink");
          FAILED ("    %ls\n", vfs_get_error (res));
        }
      else
        {
          struct stat st;
          if (!stat ("/tmp/vfs.unlink", &st))
            {
              FAILED ("    File still exists\n");
              return -1;
            }
          OK ();
        }
    }

  return 0;
}

/**
 * Main testing function
 *
 * @return zero on siccess, non-zero otherwise
 */
static int
test (void)
{
  printf ("* Global testing started\n");

  /* Test vfs_open() and other functions related to file descriptors */
  if (vfs_open_test ())
    {
      return -1;
    }

  if (vfs_unlink_test ())
    {
      return -1;
    }

  return 0;
}

int
main (int __argc, char **__argv)
{
  int i, res;
  BOOL load_localfs=FALSE;

  printf (">> Testing set for VFS module of ${project-name} <<\n");

  for (i = 1; i < __argc; ++i)
    {
      ARG_TEST_BOOL ("--load-localfs", load_localfs);
      ARG_TEST_BOOL ("--test-all", test_all);
      ARG_TEST_BOOL ("--test-open", test_open);
      ARG_TEST_BOOL ("--test-write", test_write);
      ARG_TEST_BOOL ("--test-read", test_read);
      ARG_TEST_BOOL ("--test-lseek", test_lseek);
      ARG_TEST_BOOL ("--test-close", test_close);
      ARG_TEST_BOOL ("--test-unlink", test_unlink);
    }

  /* Initialize all VFS stuff */
  if ((res = vfs_init ()))
    {
      printf ("* Error initializing VFS: %ls\n", vfs_get_error (res));
      return EXIT_FAILURE;
    }
  printf ("* VFS initialization succeed.\n");

  /* Load standart plugin */
  if (load_localfs || test_all)
    {
      if ((res = vfs_plugin_load (L"./plugins/liblocalfs.so")))
        {
          printf ("* Error loading plugin 'localfs': %ls\n",
                  vfs_get_error (res));
          return EXIT_FAILURE;
        }
      printf ("* Plugin 'localfs' loaded successfully\n");
    }

  res = test ();

  /* Uninitializing */
  vfs_done ();
  printf ("* VFS uninitialized\n");

  if (res != 0)
    {
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
