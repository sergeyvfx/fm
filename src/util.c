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
 * Fit string to specified width.
 * If width of string is greater than with to which this string is
 * going to be fit, some characters will be replaced with
 * specified suffix.
 *
 * @param __str - string to be fit
 * @param __width - width to which string is going to be fit
 * @param __suffix - suffix which will be used to show
 * that string is truncated
 * @return pointer to new string, which must be freed
 */
wchar_t*
wcsfit (const wchar_t *__str, size_t __width, const wchar_t *__suffix)
{
  size_t len, in_len, i, copy_len;
  wchar_t *str;
  size_t suff_width, cur_width;

  if (!__str || !__suffix)
    {
      return 0;
    }

  suff_width = wcswidth (__suffix, wcslen (__suffix));

  /* If string should be smaller or equal to truncated suffix */
  /* it would be better if we truncate string to empty sequence */
  if (__width <= suff_width)
    {
      /* Use wcsdup because returned buffer must be freed */
      return wcsdup (L"");
    }

  /* Calculate length of new string */
  in_len = wcslen (__str);
  if (__width < wcswidth (__str, wcslen (__str)))
    {
      len = 0;
      cur_width = 0;
      for (i = 0; i < in_len; ++i)
        {
          if (cur_width + wcwidth (__str[i]) > __width - suff_width)
            {
              break;
            }
          cur_width += wcwidth (__str[i]);
          len++;
        }

      /* Length of string to copy from source string */
      copy_len = len;

      /* Length of allocated string */
      len += wcslen (__suffix);
    }
  else
    {
      copy_len = len = in_len;
    }

  /* Allocate memory for new string */
  MALLOC_ZERO (str, sizeof (wchar_t)*(len + 1));

  /* Copy needed amount of data from input string */
  wcsncpy (str, __str, copy_len);

  /* If original length is greater than new length, */
  /* append an ellipsis */
  if (in_len > len)
    {
      wcscat (str, __suffix);
    }

  return str;
}

/**
 * Copy at most n characters of string. If s is longer than n, only
 * n characters are copied, and a terminating null byte (’\0’) is added.
 *
 * @param __s - string to be duplicated
 * @param __n - number of chars to be copied
 * @return pointer to the duplicated string
 */
wchar_t*
wcsndup (const wchar_t *__s, size_t __n)
{
  if (__s)
    {
      wchar_t *new_str;

      /* Check count of character to be copied */
      __n = MIN (__n, wcslen (__s));

      new_str = malloc ((__n + 1) * sizeof (wchar_t));
      wcsncpy (new_str, __s, __n);
      new_str[__n] = 0;
      return new_str;
    }
  else
    {
      return NULL;
    }
}

/**
 * Replace substring of string
 *
 * @param __str - string for which replacing will be applied
 * @param __max_len - maximum length of string
 * @param __substr - substring to be replaced
 * @param __newsubstr - substring will be replaced with this string
 */

#define S(_k) \
  ((_k<m)?(__substr[_k]):( (_k==m)?(0):(__str[_k-m-1]) ))

void
wcsrep (wchar_t *__str, size_t __max_len,
        const wchar_t *__substr, const wchar_t *__newsubstr)
{
  if (!__str || !__substr || !__newsubstr)
    {
      return;
    }

  size_t i, n = wcslen (__str), m = wcslen (__substr), k;
  size_t l = wcslen (__newsubstr);
  size_t *f, occur,
          prev = 0;

  /* Result will be stroed in the same string as input */
  /* So, we need a temporary buffer with length equal to */
  /* length of input string */
  wchar_t *res = malloc (sizeof (wchar_t)*(__max_len));
  size_t len_remain = __max_len;

  f = malloc ((n + m + 2) * sizeof (int));
  f[1] = k = 0;
  res[0] = 0;

  for (i = 2; i <= m + n + 1; i++)
    {
      /* Calculate prefix-function */
      while (k > 0 && S (k) != S (i - 1))
        {
          k = f[k];
        }

      if (S (k) == S (i - 1))
        {
          k++;
        }

      f[i] = k;

      if (k == m)
        {
          /* Substring has been found */
          occur = i - 2 * m - 1;

          /* Append buffer before occurance */
          wcsncat (res, __str + prev, MIN (occur - prev, len_remain));
          len_remain -= occur - prev;

          /* Append new substring */
          wcsncat (res, __newsubstr, MIN (l, len_remain));
          len_remain -= l;

          /* To start searching new substring */
          k = f[i] = 0;

          /* Skip occured substring */
          prev = occur + m;
        }
    }

  /* Append buffer after last occurandce up to the end of string */
  wcsncat (res, __str + prev, MIN (i - prev, len_remain));

  wcscpy (__str, res);

  free (f);
  free (res);
}
#undef S

/**
 * Get the number of seconds and microseconds since the Epoch
 *
 * @return the number of seconds and microseconds since the Epoch
 */
timeval_t
now (void)
{
  timeval_t res;
  gettimeofday (&res, 0);
  return res;
}

/**
 * Compare timeval_t and count of microsecods
 *
 * @param __tv - input timeval
 * @param __usec - count of microsecods
 * @return -1 if timeval is less than usec, 1 if timeval is greaterthan
 * usec and zero it they are equal
 */
int
tv_usec_cmp (timeval_t __tv, __u64_t __usec)
{
  __u64_t sec, usec;
  sec = __usec / 1000000;
  usec = __usec % 1000000;

  if (__tv.tv_sec > sec)
    {
      return 1;
    }
  if (__tv.tv_sec < sec)
    {
      return -1;
    }
  if (__tv.tv_usec > usec)
    {
      return 1;
    }
  if (__tv.tv_usec < usec)
    {
      return -1;
    }
  return 0;
}

/**
 * Get difference between two timevals
 *
 * @param __from -from which timeval difference will be measured
 * @param __to - to which timewal difference will be measured
 * @return difference between this two timevals, or {0, 0} if
 * 'from' timeval is later, that 'to' timeval
 */
timeval_t
timedist (timeval_t __from, timeval_t __to)
{
  timeval_t res = {0, 0};

  if ((__from.tv_sec > __to.tv_sec) ||
      (__from.tv_sec == __to.tv_sec && __from.tv_usec > __to.tv_usec)
      )
    {
      return res;
    }

  res.tv_sec = __to.tv_sec - __from.tv_sec;

  if (__to.tv_usec >= __from.tv_usec)
    {
      res.tv_usec = __to.tv_usec - __from.tv_usec;
    }
  else
    {
      res.tv_sec--;
      res.tv_usec = __to.tv_usec + 1000000 - __from.tv_usec;
    }

  return res;
}
