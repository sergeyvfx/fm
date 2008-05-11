/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Different helpers
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "util.h"
#include <wctype.h>

/**
 * Fits string to specified length.
 * If length of string is greater than length to which this string is
 * going to be fit, some characters will be replaced with
 * specified suffix.
 *
 * @param __str - string to be fit
 * @param __len - length to which string is going to be fit
 * @param __suffix - suffix which will be used to show
 * that string is truncated
 * @return pointer to new string, which must be freed
 */
wchar_t*
wcsfit                            (const wchar_t *__str, size_t __len,
                                   const wchar_t *__suffix)
{
  size_t len, in_len;
  wchar_t *str;
  size_t suff_len;

  if (!__str || !__suffix)
    return 0;

  suff_len=wcslen (__suffix);

  // If string should be smaller or equal to truncated suffix
  // it would be better if we truncate string to empty sequence
  if (__len<=suff_len)
    {
      // Use wcsdup because returned buffer must be freed
      return wcsdup (L"");
    }

  // Calculate length of new string
  in_len=wcslen (__str);
  len=MIN (__len, in_len);

  // Allocate memory for new string
  MALLOC_ZERO (str, sizeof (wchar_t)*(len+1));

  // Copy needed amount of data from input string
  wcsncpy (str, __str, in_len<=len?in_len:len-suff_len);

  // If original length is grater than new length,
  // append an ellipsis
  if (in_len>len)
    wcscat (str, __suffix);

  return str;
}
