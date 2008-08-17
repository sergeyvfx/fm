/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Different stuff providing directory-related operations
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "dir.h"

#include <wchar.h>
#include <stdlib.h>
#include <dirent.h>

/**
 * Wrapper of VFS function vfs_scandir()
 * Returned array and its elements must be freed
 *
 * @param __name - name of directory to scan
 * @param __res - pointer to array where result is stored
 * @return count of elements or -1 on error
 */
int
wcscandir (const wchar_t *__url, vfs_filter_proc __filer,
           dircmp_proc __compar, file_t ***__res)
{
  vfs_dirent_t **eps = NULL;
  wchar_t *full_name;
  size_t fn_len;
  int count, i;

  if (!__url || !__res)
    {
      return -1;
    }

  /* Do not use VFS-related sorting, because */
  /* our comparator may want STAT information */
  count = vfs_scandir (__url, &eps, __filer, 0);

  /* Error scanning directory */
  if (count < 0)
    {
      return count;
    }

  fn_len = wcslen (__url) + MAX_FILENAME_LEN + 1;
  full_name = malloc ((fn_len + 1) * sizeof (wchar_t));

  /* Allocate memory for result */
  (*__res) = malloc (sizeof (file_t*) * count);

  /* Get stat information */
  for (i = 0; i < count; i++)
    {
      MALLOC_ZERO ((*__res)[i], sizeof (file_t));

      swprintf (full_name, fn_len, L"%ls/%ls", __url, eps[i]->name);
      wcscpy ((*__res)[i]->name, eps[i]->name);
      (*__res)[i]->type = eps[i]->type;

      /* Get STAT information of file */
      vfs_stat (full_name, &(*__res)[i]->stat);
      vfs_lstat (full_name, &(*__res)[i]->lstat);

      vfs_free_dirent (eps[i]);
    }

  /* Free used variables */
  SAFE_FREE (eps);

  qsort ((*__res), count, sizeof (file_t*),
         (__compar ? __compar : wcscandir_alphasort));

  free (full_name);

  return count;
}

/**
 * Concatenate subdirectory to directory name
 *
 * @param __name - name of directory name to which append
 * a subdirectory name
 * @param __subname - name of subdirectory which will be concatenated
 * to directory name
 * @return pointer to new directory name, which must be freed and
 * zero in case of errors
 */
wchar_t*
wcdircatsubdir (const wchar_t *__name, const wchar_t *__subname)
{
  wchar_t *new_name;
  size_t new_len;

  if (!__name || !__subname)
    {
      return 0;
    }

  if (!wcscmp (__subname, L"."))
    {
      return wcsdup (L"/");
    }

  if (!wcscmp (__subname, L".."))
    {
      return wcdirname (__name);
    }

  /* Allocate memory for new directory name */
  /* 2=null-terminator+directory separator */
  new_len = wcslen (__name) + wcslen (__subname) + 2;
  MALLOC_ZERO (new_name, sizeof (wchar_t) * new_len);

  /* Build new directory */
  wcscpy (new_name, __name);
  if (__name[wcslen (__name) - 1] != '/')
    {
      wcscat (new_name, L"/");
    }
  wcscat (new_name, __subname);

  return new_name;
}

/**
 * Fit name of directory to specified width
 *
 * @param __dir_name - directory name to be fitted
 * @param __width - width to which fit the directory name
 * @param __res - pointer to buffer where result will be stored
 */
void
fit_dirname (const wchar_t *__dir_name, size_t __width, wchar_t *__res)
{
  size_t i, len, ptr, cur_width;
  int ch_wid;
  len = wcslen (__dir_name);

  if (wcswidth (__dir_name, len) > __width)
    {
      wcscpy (__res, L"...");
      ptr = wcslen (__res);

      /* Get character from which copying is 'safe' */
      cur_width = wcswidth (__res, ptr);
      i = len - 1;
      for (;;)
        {
          ch_wid = wcwidth (__dir_name[i]);
          if (cur_width + ch_wid >= __width)
            {
              break;
            }

          cur_width += ch_wid;

          if (i == 0)
            {
              break;
            }
          --i;
        }

      /* Copy tail */
      while (i < len)
        {
          __res[ptr++] = __dir_name[i];
          ++i;
        }

      __res[ptr] = 0;
    }
  else
    {
      wcscpy (__res, __dir_name);
    }
}

/**
 * Strip non-directory suffix from file name
 *
 * @param __name - name of file name from which strip non-directory suffix
 * @return pointer to new directory name, which must be freed, and
 * zero in case of errors
 */
wchar_t*
wcdirname (const wchar_t *__name)
{
  wchar_t *res;
  size_t i, len;

  if (!__name)
    {
      return 0;
    }

  len = wcslen (__name);

  /* Search for last delimiter */
  for (i = len - 1; i > 0; i--)
    {
      if (__name[i] == '/')
        {
          break;
        }
    }

  if (!i)
    {
      return wcsdup (L"/");
    }

  /* Allocate memory for new directory name */
  res = malloc (sizeof (wchar_t)*(i + 1));

  /* Copy data to new name */
  wcsncpy (res, __name, i);
  res[i] = 0;
  return res;
}

/**
 * Filter for scandir which skips all hidden files
 *
 * @param __data - pointer to a dirent structure
 * @return 0 if file is hidden, 1 if file is listingable
 */
int
scandir_filter_skip_hidden (const vfs_dirent_t * __data)
{
  if (!IS_PSEUDODIR (__data->name) && __data->name[0] == '.')
    {
      return 0;
    }

  return 1;
}

/**
 * Alphabetically sorter for wcscandir which sorts files and directories
 *
 * @param __a - left element of array
 * @param __b - right element of array
 * @return an integer less than, equal to, or greater than zero if the
 * first argument is considered to be respectively less than, equal to,
 * or greater than the second.
 */
int
wcscandir_alphasort (const void *__a, const void *__b)
{
  file_t *a = *(file_t**) __a, *b = *(file_t**) __b;

  if (IS_PSEUDODIR (a->name))
    {
      return -1;
    }

  return wcscmp (a->name, b->name);
}

/**
 * Alphabetically sorter for wcscandir which sorts files and
 * directories separately
 *
 * @param __a - left element of array
 * @param __b - right element of array
 * @return an integer less than, equal to, or greater than zero if the
 * first argument is considered to be respectively less than, equal to,
 * or greater than the second.
 */
int
wcscandir_alphasort_sep (const void *__a, const void *__b)
{
  file_t *a = *(file_t**) __a, *b = *(file_t**) __b;

  /* Directories `.` and `..` must be at the top op list */
  if (IS_PSEUDODIR (a->name))
    {
      return -1;
    }

  if (IS_PSEUDODIR (b->name))
    {
      return 1;
    }

  if (S_ISDIR (a->stat.st_mode) && S_ISDIR (b->stat.st_mode))
    {
      return wcscmp (a->name, b->name);
    }

  if (S_ISDIR (a->stat.st_mode))
    {
      return -1;
    }

  if (S_ISDIR (b->stat.st_mode))
    {
      return 1;
    }

  return wcscmp (a->name, b->name);
}

/**
 * Checks is specified URL is a directory
 *
 * @param __url - URL to check
 * @return non-zero if specified URL is a directory and zero otherwise
 */
BOOL
isdir (const wchar_t *__url)
{
  vfs_stat_t stat;

  if (vfs_lstat (__url, &stat))
    {
      /*
       * TODO: Add error handling here
       */
      return FALSE;
    }

  return S_ISDIR (stat.st_mode);
}
