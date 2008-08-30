/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Working with shared directories and other data
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#define NO_XOPEN_SOURCE /* For use dirent() */

#include "shared.h"
#include "util.h"
#include "dir.h"

#include <dirent.h>
#include <stdlib.h>
#include <ctype.h>
#include <ncursesw/curses.h>

/**
 * Iterator for get_shared_files
 * Reads file from specified path
 *
 * @param __path - directory to scan
 * @param __count - count of elements in list
 * @param __list - list of files
 * @return zero on success, non-zero otherwise
 */
static int
get_shared_files_iter (wchar_t *__path, long *__count, wchar_t ***__list)
{
  int i, count;
  size_t len;
  wchar_t *s;
  struct dirent **dirent;
  char *path;

  len = wcslen (__path);
  path = malloc ((len + 1) * MB_CUR_MAX);

  if (wcstombs (path, __path, (len + 1) * MB_CUR_MAX) == -1)
    {
      free (path);
      return -1;
    }

  count = scandir (path, &dirent, 0, alphasort);

  if (count < 0)
    {
      return count;
    }

  len = wcslen (__path) + MAX_FILENAME_LEN + 1;
  s = malloc ((len + 1) * sizeof (wchar_t));

  for (i = 0; i < count; ++i)
    {
      if (!IS_MBPSEUDODIR (dirent[i]->d_name))
        {
          swprintf (s, len, L"%ls/%s", __path, dirent[i]->d_name);
          if (!isdir (s))
            {
              (*__list) = realloc ((*__list),
                                   ((*__count) + 1) * sizeof (wchar_t*));

              (*__list)[(*__count)] = wcsdup (s);
              ++(*__count);
            }
        }

      free (dirent[i]);
    }

  free (dirent);
  free (path);

  return 0;
}

/********
 * Use's backend
 */

/**
 * Get list of files in shared directory
 *
 * @param __dir - name of shared directory
 * @param __list - list of files (with absolutely pathes)
 * @return count of files or value less than zero if failed
 */
long
get_shared_files (wchar_t *__dir, wchar_t ***__list)
{
  long count = 0;

  (*__list) = NULL;

#ifndef NOINST_DEBUG
  /* We need to get file listing from ${data_dir} first */
  /* and after this from ~/.{$packacge} */

  /* NOTE: If "${PACKAGE}_HOME" environment variable is set */
  /*       we whould use it instead of ~ */

  long i;
  passwd_t *user_info;
  wchar_t *dir, *home;
  char env_home_name[64];
  size_t len;

  /* Get listing of ${data_dir} */
  get_shared_files_iter (WC_DATA_DIR, &count, __list);

  /* Get name of environment variable */
  snprintf (env_home_name, BUF_LEN (env_home_name), "%s_HOME", PACKAGE);

  /* Make name of enviroment variable be written */
  /* with capital letters only */
  for (i = 0; env_home_name[i]; i++)
    {
      env_home_name[i] = toupper (env_home_name[i]);
    }

  /* Get user info */
  if (getenv (env_home_name) == NULL)
    {
      user_info = get_user_info ();
      len = wcslen (user_info->home) + wcslen (WC_PACKAGE) + 2;
      home = malloc ((len + 1) * sizeof (wchar_t));
      swprintf (home, len + 1, L"%ls/.%ls", user_info->home, WC_PACKAGE);
      free_user_info (user_info);
    }
  else
    {
      /* We need use home directory specified in ${PACKAGE}_HOME variable */
      MBS2WCS (home, getenv (env_home_name));
    }

  /* Get listing of ~/.${package} */
  len = wcslen (home) + wcslen (__dir) + 2;
  dir = malloc ((len + 1) * sizeof (wchar_t));
  swprintf (dir, len + 1, L"%ls/%ls", home, __dir);

  /* Get listing */
  get_shared_files_iter (dir, &count, __list);

  /* Free used info */
  free (dir);
  free (home);
#else
  /* If NOINST_DEBUG is defined, we should get listing */
  /* only of fake home */

  wchar_t fake_home[] = L"../fakehome";
  wchar_t *dir;
  size_t len;

  len = wcslen (fake_home) + wcslen (__dir) + 2;
  dir = malloc ((len + 1) * sizeof (wchar_t));
  swprintf (dir, len + 1, L"%ls/%ls", fake_home, __dir);

  get_shared_files_iter (dir, &count, __list);

  free (dir);
#endif

  return count;
}
