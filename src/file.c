/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Different stuff providing file-related operations
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "file.h"
#include "deque.h"
#include "dir.h"

#include <wchar.h>
#include <math.h>

/********
 *
 */

#define FILE_TRUNCATOR '.'
#define FILE_TRUNCATOR_MIN_LEN 3

#define DIGITS_IN_HUMAN_SIZE 7

/* Default Formats of date strings for recent and older dates */
#define DEFAULT_DATE_FORMAT     L"%b %d %H:%M"
#define DEFAULT_OLD_DATE_FORMAT L"%b %d  %Y"

/* Default timedists of recent date (in days) */
#define DEFAULT_RECENT_PAST     (24*30*6)
#define DEFAULT_RECENT_FUTURE   (24*30*6)

/********
 *
 */

/**
 * Fit filename to specified width
 *
 * @param __file_name - file name to be fitted
 * @param __width - width to which fit the file name
 * @param __res - pointer to buffer where result will be stored
 */
void
fit_filename (const wchar_t *__file_name, size_t __width, wchar_t *__res)
{
  size_t len;

  if (__width <= 0)
    {
      /* Wanted length of file name is too short */
      *__res = 0;
      return;
    }

  len = wcslen (__file_name);
  if (wcswidth (__file_name, len) <= __width)
    {
      /* Do not call any specific stuff */
      wcscpy (__res, __file_name);
    }
  else
    {
      size_t i, j, ptr = 0, mid = __width / 2, cur_width = 0, suff_ptr = 0;
      int ch_wid;
      wchar_t *suffix;

      /* Copy left part of truncated name */
      for (i = 0; i < len; i++)
        {
          ch_wid = wcwidth (__file_name[i]);
          if (cur_width + ch_wid > mid)
            {
              break;
            }
          cur_width += ch_wid;
          __res[ptr++] = __file_name[i];
        }

      /* Collect right part of truncated file name */
      /* NOTE: Suffix will be collected in reverse mode */
      suffix = malloc (sizeof (wchar_t) * (len + 1));
      for (j = len - 1; j > i; --j)
        {
          ch_wid = wcwidth (__file_name[j]);
          if (__width - (cur_width + ch_wid) < FILE_TRUNCATOR_MIN_LEN)
            {
              break;
            }
          cur_width += ch_wid;
          suffix[suff_ptr++] = __file_name[j];
        }

      /* Copy truncator */
      for (j = 0; j < __width - cur_width; ++j)
        {
          __res[ptr++] = FILE_TRUNCATOR;
        }

      /* Copy suffix */
      if (suff_ptr>0)
        {
          --suff_ptr;
          for (;;)
            {
              __res[ptr++] = suffix[suff_ptr];
              if (suff_ptr == 0)
                {
                  break;
                }
              --suff_ptr;
            }
        }

      free (suffix);
      __res[ptr] = 0;
    }
}

/**
 * Strip directory prefix from file name
 *
 * @param __name - name of file from which strip directory prefix
 * @return pointer to new file name, which must be freed and
 * zero in case of errors
 */
wchar_t*
wcfilename (const wchar_t *__name)
{
  size_t i, len = wcslen (__name), ptr = 0;
  wchar_t *res;

  if (!__name)
    {
      return 0;
    }

  if (!wcscmp (__name, L"/"))
    {
      return wcsdup (L"/");
    }

  /* Search last '/' */
  for (i = len - 1; i >= 0; i--)
    {
      if (__name[i] == '/')
        {
          break;
        }
    }

  /* Allocate memory for new filename */
  res = malloc (sizeof (wchar_t)*(len - i + 1));

  /* Copy data to new filename */
  i++;
  while (i < len)
    {
      res[ptr++] = __name[i++];
    }

  res[ptr] = 0;

  return res;
}

/**
 * Convert file mode creation mask to string
 *
 * @param __mask - mode creation mask to be converted to string
 * @param __res - pointer to string where save result of converting
 */
void
umasktowcs (mode_t __mask, wchar_t *__res)
{
  if (!__res)
    {
      return;
    }

  /* Permissions for owner */
  __res[0] = __mask & S_IRUSR ? 'r' : '-';
  __res[1] = __mask & S_IWUSR ? 'w' : '-';
  __res[2] = __mask & S_IXUSR ? 'x' : '-';

  /* Permissions for group */
  __res[3] = __mask & S_IRGRP ? 'r' : '-';
  __res[4] = __mask & S_IWGRP ? 'w' : '-';
  __res[5] = __mask & S_IXGRP ? 'x' : '-';

  /* Permissions for others */
  __res[6] = __mask & S_IROTH ? 'r' : '-';
  __res[7] = __mask & S_IWOTH ? 'w' : '-';
  __res[8] = __mask & S_IXOTH ? 'x' : '-';

  /* Null-terminator */
  __res[9] = 0;
}

/**
 * Convert file size to human-readable format
 *
 * @param __size - original size of file
 * @param __suffix - returned suffix of human-readable size
 * @return human-readable size of file (without suffix)
 */
#ifdef __USE_FILE_OFFSET64
__u64_t
fsizetohuman (__u64_t __size, char *__suffix)
#else

__u32_t
fsizetohuman (__u32_t __size, char *__suffix)
#endif
{
  static char suffixes[] = {'\0', 'K', 'M', 'G', 'T'};
  __u64_t res = __size;
  short suff_ptr = 0;
  double log10 = log (10);

  while (log (res) / log10 > DIGITS_IN_HUMAN_SIZE)
    {
      res /= 1024;
      suff_ptr++;
    }

  if (__suffix)
    {
      (*__suffix) = suffixes[suff_ptr];
    }

  return res;
}

/**
 * Format time of file
 * This function uses different date format string for recent and
 * old files
 *
 * @param __buf - buffer for result
 * @param __buf_size - size of buffer
 * @param __time - time of file
 */
void
format_file_time (wchar_t *__buf, size_t __buf_size, time_t __time)
{
  wchar_t *format;
  struct tm tm;
  time_t now;

  now = time (0);

  /* Convert time_t to struct tm */
  gmtime_r (&__time, &tm);

  /* Get format for date string */
  if (__time < now)
    {
      format = ((now - __time) < DEFAULT_RECENT_PAST * 3600) ?
              DEFAULT_DATE_FORMAT : DEFAULT_OLD_DATE_FORMAT;
    }
  else
    {
      format = ((__time - now) < DEFAULT_RECENT_FUTURE * 3600) ?
              DEFAULT_DATE_FORMAT : DEFAULT_OLD_DATE_FORMAT;
    }

  wcsftime (__buf, __buf_size, format, &tm);
}

/**
 * Trim file name
 * (Deletes duplicated '/' from file name)
 * Returned buffer must be freed
 *
 * @return truncated file name.
 */
wchar_t*
filename_normalize (const wchar_t *__fn)
{
  size_t i, len, ptr = 0;
  wchar_t *res, *s, *old_s, *item;
  deque_t *items, *n_items;
  BOOL abs = FALSE;

  len = wcslen (__fn);

  void free_ref_data (void *__data)
    {
      SAFE_FREE (__data);
    }

  /* Length of file name can not become longer after trim */
  res = malloc ((len + 1) * sizeof (wchar_t));

  /* Step 1: avoid multimly slahes  */
  for (i = 0; i < len; i++)
    {
      if (__fn[i] == '/' && ptr > 0 && res[ptr - 1] == '/')
        {
          continue;
        }
      else
        {
          res[ptr++] = __fn[i];
        }
    }

  res[ptr] = 0;

  /* Step 2: process . and .. subdirectories */

  /* Get stack of path items */
  items = deque_create ();
  s = wcsdup (res);
  while (wcscmp (s, L"") != 0 && wcscmp (s, L"/") != 0)
    {
      deque_push_front (items, wcfilename (s));
      old_s = s;
      s = wcdirname (s);
      free (old_s);
    }
  free (s);

  /* Get stack of normalized items */
  n_items = deque_create ();
  deque_foreach (items, item)
    if (wcscmp (item, L".") == 0)
      {
        deque_foreach_continue;
      }

    if (wcscmp (item, L"..") == 0)
      {
        deque_pop_back (n_items);
      }
    else
      {
        deque_push_back (n_items, item);
      }
  deque_foreach_done

  /* Collect normalized path */
  if (res[0] == '/')
    {
      wcscpy (res, L"/");
      abs = TRUE;
    }
  else
    {
      wcscpy (res, L"");
    }

  i = 0;
  deque_foreach (n_items, item)
    if (!(abs && i == 0))
      {
        wcscat (res, L"/");
      }
    wcscat (res, item);
    ++i;
  deque_foreach_done

  /* Step 3: Free used memory */
  deque_destroy (n_items, 0);
  deque_destroy (items, free_ref_data);

  return res;
}

/**
 * Compare two file names
 *
 * @return value of wcscmp() with trimed file names as arguments
 */
int
filename_compare (const wchar_t *__a, const wchar_t *__b)
{
  int res;
  wchar_t *a, *b;
  a = filename_normalize (__a);
  b = filename_normalize (__b);

  res = wcscmp (a, b);

  free (a);
  free (b);

  return res;
}
