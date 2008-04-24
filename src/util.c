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

/**
 * Fits string to specified length.
 * If length of string is greater than length to which this string is
 * going to be fit, some characters will be replaced with `...`
 *
 * @param __str - string to be fit
 * @param __len - length to which string is going to be fit
 * @return pointer to new string, which must be freed
 */
wchar_t*
wcsfit                            (const wchar_t *__str, size_t __len)
{
  size_t len, in_len;
  wchar_t *str;

  if (!__str)
    return 0;
  
  if (__len<4)
    return wcsdup (L"");

  // Calculate length of new string
  in_len=wcslen (__str);
  len=MIN (__len, in_len);

  // Allocate memory for new string
  MALLOC_ZERO (str, sizeof (wchar_t)*(len+1));

  // Copy needed amount of data from input string
  wcsncpy (str, __str, in_len<=len?in_len:len-3);

  // If original length is grater than new length,
  // append an ellipsis
  if (in_len>len)
    wcscat (str, L"...");

  return str;
}
