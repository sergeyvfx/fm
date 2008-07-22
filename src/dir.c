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
 * Wide character analog for system scandir
 * Returned array and its elements must be freed
 *
 * @param __name - name of directory to scan
 * @param __res - pointer to array where result is stored
 * @return count of elements or -1 on error
 */
int
wcscandir                         (const wchar_t  *__name,
                                   dirfilter_proc  __filer,
                                   dircmp_proc     __compar,
                                   file_t       ***__res){
  char *mb_name,   // Multibyte name of directory
       *full_name;
  struct dirent **eps=0;
  int count, i;

  if (!__name || !__res)
    return -1;

  // Because of strange bag with scanning '/'
  // It should be scanned '//' instead of '/'
  if (!wcscmp (__name, L"/"))
    {
      mb_name=strdup ("//");
    } else {
      size_t len=(wcslen (__name)+1)*MB_CUR_MAX;

      // Allocate memory for multibyte buffer
      MALLOC_ZERO (mb_name, len);

      // Convert wide character name of directory to
      // multibyte string
      if (wcstombs (mb_name, __name, len)==-1)
        {
          // Some errors while converting name to wide char
          free (mb_name);
          return -1;
        }
    }

  // Scan directory
  count=scandir (mb_name, &eps, __filer,  alphasort);

  // Error scanning directory
  if (count==-1)
    return -1;

  // Allocate memory for result
  (*__res)=malloc (sizeof (file_t*)*count);

  full_name=malloc (strlen (mb_name)+MAX_FILENAME_LEN+2);

  // Convert dirent-s to file_t-s
  for (i=0; i<count; i++)
    {
      MALLOC_ZERO ((*__res)[i], sizeof (file_t));

      sprintf (full_name, "%s/%s", mb_name, eps[i]->d_name);

      // Convert multibyte item name to wide-charactered string
      if (mbstowcs ((*__res)[i]->name, eps[i]->d_name, MAX_FILENAME_LEN)==-1)
        {
          // If some error had been occurred while converting,
          // free all allocated memory and return -1
          int j;
          for (j=0; j<=i; j++)
            free ((*__res)[i]);
          free (*__res);
        }

      if (stat (full_name, &(*__res)[i]->stat)==-1)
        {
          //
          // TODO:
          //  Add handling of STAT error here
          //
        }

      if (lstat (full_name, &(*__res)[i]->lstat)==-1)
        {
          //
          // TODO:
          //  Add handling of STAT error here
          //
        }

      (*__res)[i]->type=eps[i]->d_type;

      free (eps[i]);
    }

  qsort ((*__res), count, sizeof (file_t*),
    (__compar?__compar:wcscandir_alphasort));

  // Free used variables
  SAFE_FREE (eps);

  free (mb_name);
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
wcdircatsubdir                    (const wchar_t *__name,
                                   const wchar_t *__subname)
{
  wchar_t *new_name;
  size_t new_len;

  if (!__name || !__subname)
    return 0;

  if (!wcscmp (__subname, L"."))
    return wcsdup (L"/");

  if (!wcscmp (__subname, L".."))
    return wcdirname (__name);

  // Allocate memory for new directory name
  // 2=null-terminator+directory separator
  new_len=wcslen (__name)+wcslen (__subname)+2;
  MALLOC_ZERO (new_name, sizeof (wchar_t)*new_len);

  // Build new directory
  wcscpy (new_name, __name);
  if (wcscmp (__name, L"/"))
    wcscat (new_name, L"/");
  wcscat (new_name, __subname);

  return new_name;
}

/**
 * Fits name of directory to specified length
 *
 * @param __dir_name - directory name to be fitted
 * @param __len - length to which fit the file name
 * @param __res - pointer to buffer where result will be stored
 */
void           // Fit dirname to specified length
fit_dirname                       (const wchar_t *__dir_name,
                                   long           __len,
                                   wchar_t       *__res)
{
  int i, len, ptr;
  len=wcslen (__dir_name);

  if (len>__len)
    {
      swprintf (__res, len,  L"...");
      ptr=3;
      for (i=len-__len+3; i<=len; i++)
        __res[ptr++]=__dir_name[i];
      __res[ptr]=0;
    } else
      wcscpy (__res, __dir_name);
}

/**
 * Strip non-directory suffix from file name
 *
 * @param __name - name of file name from which strip non-directory suffix
 * @return pointer to new directory name< which must be freed and
 * zero in case of errors
 */
wchar_t*
wcdirname                         (const wchar_t *__name)
{
  wchar_t *res;
  size_t i, len;
  if (!__name)
    return 0;

  len=wcslen (__name);

  // Search for last delimiter
  for (i=len-1; i>0; i--)
    if (__name[i]=='/')
      break;

  if (!i)
    return wcsdup (L"/");

  // Allocate memory for new directory name
  res=malloc (sizeof (wchar_t)*(i+1));

  // Copy data to new name
  wcsncpy (res, __name, i);
  res[i]=0;
  return res;
}

/**
 * Filter for scandir which skips all hidden files
 *
 * @param __data - pointer to a dirent structure
 * @return 0 if file is hidden, 1 if file is listingable
 */
int
scandir_filter_skip_hidden        (const struct dirent * __data)
{
  if (strcmp (__data->d_name, ".") && strcmp (__data->d_name, "..") &&
    __data->d_name[0]=='.')
    return 0;
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
wcscandir_alphasort               (const void *__a, const void *__b)
{
  file_t *a=*(file_t**)__a, *b=*(file_t**)__b;

  if (!wcscmp (a->name, L".") || !wcscmp (a->name, L".."))
    return -1;

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
wcscandir_alphasort_sep           (const void *__a, const void *__b)
{
  file_t *a=*(file_t**)__a, *b=*(file_t**)__b;

  // Directories `.` and `..` must be at the top op list
  if (!wcscmp (a->name, L".") || !wcscmp (a->name, L".."))
    return -1;

  if (!wcscmp (b->name, L".") || !wcscmp (b->name, L".."))
    return 1;

  if (S_ISDIR (a->stat.st_mode) && S_ISDIR (b->stat.st_mode))
    return wcscmp (a->name, b->name);

  if (S_ISDIR (a->stat.st_mode))
    return -1;

  if (S_ISDIR (b->stat.st_mode))
    return 1;

  return wcscmp (a->name, b->name);
}
