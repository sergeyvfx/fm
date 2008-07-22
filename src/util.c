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
#include <wchar.h>

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

  // If original length is greater than new length,
  // append an ellipsis
  if (in_len>len)
    wcscat (str, __suffix);

  return str;
}

/**
 * Copies at most n characters of string. If s is longer than n, only
 * n characters are copied, and a terminating null byte (’\0’) is added.
 *
 * @param __s - string to be duplicated
 * @param __n - number of chars to be copied
 * @return pointer to the duplicated string
 */
wchar_t*
wcsndup                           (const wchar_t *__s, size_t __n)
{
  if (__s)
    {
      wchar_t *new_str;

      // Check count of character to be copied
      __n=MIN (__n, wcslen (__s));

      new_str=malloc ((__n+1)*sizeof (wchar_t));
      wcsncpy (new_str, __s, __n);
      new_str[__n]=0;
      return new_str;
    } else
      return NULL;
}

/**
 * Replaces substring of string
 *
 * @param __str - string for which replacing will be applied
 * @param __max_len - maximum length of string
 * @param __substr - substring to be replaced
 * @param __newsubstr - substring will be replaced with this string
 */

#define S(_k) \
  ((_k<m)?(__substr[_k]):( (_k==m)?(0):(__str[_k-m-1]) ))

void
wcsrep                            (wchar_t       *__str,
                                   size_t         __max_len,
                                   const wchar_t *__substr,
                                   const wchar_t *__newsubstr)
{
  if (!__str || !__substr || !__newsubstr)
    return;

  size_t i, n=wcslen (__str), m=wcslen (__substr), k;
  size_t *f, occur,
          prev=0;

  // Result will be stroed in the same string as input
  // So, we need a temporary buffer with length equal to
  // length of input string
  wchar_t *res=malloc (sizeof (wchar_t)*(__max_len));
  size_t len_remain=__max_len;

  f=malloc ((n+m+2)*sizeof (int));
  f[1]=k=0;
  res[0]=0;

  for (i=2; i<=m+n+1; i++)
    {
      // Calculate prefix-function
      while (k>0 && S(k)!=S(i-1))
        k=f[k];
      if (S(k)==S(i-1))
        k++;
      f[i]=k;

      if (k==m)
        {
          // Substring has been found
          occur=i-2*m-1;

          // Append buffer before occurance
          wcsncat (res, __str+prev, MIN (occur-prev, len_remain));
          len_remain-=occur-prev;
          // Append new substring
          wcsncat (res, __newsubstr, MIN (m, len_remain));
          len_remain-=m;

          // To start searching new substring
          k=f[i]=0;

          // Skip occured substring
          prev=occur+m;
        }
    }

  // Append buffer after last occurandce up to the end of string
  wcsncat (res, __str+prev, MIN (i-prev, len_remain));

  wcscpy (__str, res);

  free (f);
  free (res);
}

#undef S
