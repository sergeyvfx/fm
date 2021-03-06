/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Different helpers
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "util.h"
#include "dir.h"
#include "iface.h"
#include "hook.h"
#include "signals.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <vfs/vfs.h>
#include <unistd.h>

#define EXPLODE_APPEND_STRING(__s) \
  { \
    (*__out) = realloc (*__out, (count + 1) * sizeof (wchar_t*)); \
    (*__out)[count] = __s ? wcsdup (__s) : NULL; \
     ++count; \
  }

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
 * Duplicate at most n characters of string. If s is longer than n, only
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
 * Get sub-string of string
 *
 * @param __src - source string
 * @param __from - from character
 * @param __len - length of sub-string
 */
void
wcssubstr (wchar_t *__dst, const wchar_t *__src, size_t __from, size_t __len)
{
  if (!__src || !__len)
    {
      return;
    }

  /*
   * TODO: Are we need any checking here?
   */

  wcsncpy (__dst, __src + __from, __len);
  __dst[__len] = 0;
}

/**
 * Get sub-string of string
 *
 * @param __src - source string
 * @param __from - from character
 * @param __len - length of sub-string
 */
void
substr (char *__dst, const char *__src, size_t __from, size_t __len)
{
  if (!__src || !__len)
    {
      return;
    }

  /*
   * TODO: Are we need any checking here?
   */

  strncpy (__dst, __src + __from, __len);
  __dst[__len] = 0;
}

/**
 * Convert a wide character string to a multibyte string
 *
 * @param __dest - string to convertion
 * @param __source - string from convertion
 * @return number of bytes convertion string,
 *         not including the terminating null byte
 */
size_t
wcs2mbs (char **__dest, const wchar_t *__source)
{
  size_t len = (wcslen (__source) + 1) * MB_CUR_MAX;
  *__dest = malloc (len);
  return wcstombs (*__dest, __source, len);
}

/**
 * Sideeffect analog wcs2mbs
 *
 * @param __string - string from convertion
 * @return allocated multibyte characters string
 */
char *
to_multibyte (const wchar_t *__string)
{
  char *result;
  size_t length = wcs2mbs (&result, __string);
  return length > 0 ? result : NULL;
}

/**
 * Convert a multibyte string to a wide character string
 *
 * @see wcs2mbs
 */
size_t
mbs2wcs (wchar_t **__dest, const char *__source)
{
  size_t len = strlen (__source);

  *__dest = malloc (sizeof (wchar_t) * (len + 2));
  return mbstowcs (*__dest, __source, len + 1);
}

/**
 * Sideeffect analog mbs2wcs
 *
 * @param __string - string from convertion
 * @return allocated wide characters string
 */
wchar_t *
to_widestring (const char *__string)
{
  wchar_t *result;
  size_t length = mbs2wcs (&result, __string);
  return length > 0 ? result : NULL;
}

/**
 * Replace substring of string
 *
 * @param __str - string for which replacing will be applied
 * @param __substr - substring to be replaced
 * @param __newsubstr - substring will be replaced with this string
 * @return string with replaced sub-strings
 * @sideeffect allocate memory for result
 */

#define S(_k) \
  ((_k<m)?(__substr[_k]):( (_k==m)?(0):(__str[_k-m-1]) ))

wchar_t*
wcsrep (wchar_t *__str, const wchar_t *__substr, const wchar_t *__newsubstr)
{
  if (!__str || !__substr || !__newsubstr)
    {
      return NULL;
    }

  size_t i, n = wcslen (__str), m = wcslen (__substr), k;
  size_t l = wcslen (__newsubstr);
  size_t *f, occur,
          prev = 0;

  /* Assume result's length is equal to length of input string */
  /* It would be enlarged if needed */
  size_t alloc_len = wcslen (__str), res_len = 0;
  wchar_t *res = malloc ((alloc_len + 1) * sizeof (wchar_t));

  f = malloc ((n + m + 2) * sizeof (size_t));
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

          res_len += (occur - prev) + l;

          /* Enlarge buffer if needed */
          if (res_len >= alloc_len)
            {
              alloc_len += res_len;
              res = realloc (res, (alloc_len + 1) * sizeof (wchar_t));
            }

          /* Append buffer before occurance */
          wcsncat (res, __str + prev, occur - prev);

          /* Append new substring */
          wcscat (res, __newsubstr);

          /* To start searching new substring */
          k = f[i] = 0;

          /* Skip occured substring */
          prev = occur + m;
        }
    }

  /* Enlarge buffer */
  res_len += i - prev;
  if (res_len >= alloc_len)
    {
      res = realloc (res, (res_len + 1) * sizeof (wchar_t));
    }

  /* Append buffer after last occurrence up to the end of string */
  wcscat (res, __str + prev);

  free (f);

  return res;
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

/**
 * Run the specified shell command
 *
 * @param __command - a name executed command
 * @return see man execl(3) and man fork(2)
 */
int
run_shell_command (const wchar_t *__command)
{
  char  *command;
  int   result = 0, status;
  pid_t pid;

  wcs2mbs (&command, __command);

  iface_screen_lock ();
    if ((pid = fork ()) == 0)
      {
        result = execl (getenv ("SHELL"), "sh", "-c", command, NULL);
      }

    if (pid != -1)
      {
        waitpid (pid, &status, WUNTRACED | WCONTINUED);
      }
    else
      {
        result = -1;
      }

    /*
     * FIXME: execl() replaces my handlers of signals
     */
    signals_hook ();

  iface_screen_unlock ();

  free (command);
  return result;
}

/**
 * Exit from file manager
 */
void
do_exit (void)
{
  hook_call (L"exit-hook", NULL);
}

/**
 * Escaped `bad' characters in a string
 *
 * @param __source - a source string
 * @return escaped string
 */
wchar_t *
escape_string (const wchar_t *__source)
{
  const wchar_t *special = L" $`\"\\!()[]", *in = __source;
  wchar_t *out, *result;
  size_t  escaped_symbol = 0;

  while (*in != L'\0')
    {
      if (wcschr (special, *in++))
        {
          ++escaped_symbol;
        }
    }

  if (escaped_symbol == 0)
    {
      return wcsdup (__source);
    }

  result = malloc (sizeof (wchar_t) * (wcslen(__source) + escaped_symbol + 1));

  in  = __source;
  out = result;

  while (*in != L'\0')
    {
      if (wcschr (special, *in))
        {
          *out++ = L'\\';
        }
      *out++ = *in++;
    }

  *out = L'\0';

  return result;
}

/**
 * Expands '*' characters in pattern string by another string
 *
 * If there is a '*.' substring in pattern, '*' will be replaced with base file
 * name from string (string with dropped file extension)
 * If there is a '.*' substring in pattern, '*' will be replaced with file
 * extension from string.
 * All the rest '*' characters in pattern will be replaced by the whole string.
 *
 * @param __pattern - string with pattern
 * @param __string - string to expand '*' characters
 * @return pointer to string with expanded '*' characters
 * @sizeeffect allocate memory for return value
 */
wchar_t*
pattern_rename (const wchar_t *__pattern, const wchar_t *__string)
{
  wchar_t *res = NULL;
  size_t i, n, m, prev, base_len, ext_len, size, buf_len;
  wchar_t *base, *ext, *buf;
  int rep_iter = 0;

  m = wcslen (__string);

  /* Get pointer to base name and extension and thir lengths */
  base = (wchar_t*)__string;
  base_len = m;

  ext = (wchar_t*)(__string + base_len);
  ext_len = 0;
  while (ext >= __string && *ext != '.')
    {
      --ext;
      ++ext_len;
    }

  if (*ext == '.')
    {
      base_len -= ext_len;
      ++ext;
      --ext_len;
    }

  size = prev = 0;

  /* Expand all '*' characters */
  for (i = 0, n = wcslen (__pattern); i < n; ++i)
    {
      if (__pattern[i] == '*')
        {
          if (i < n - 1 && __pattern[i + 1] == '.')
            {
              buf = base;
              buf_len = base_len;
            }
          else if (i > 0 && __pattern[i - 1] == '.')
            {
              buf = ext;
              buf_len = ext_len;
            }
          else
            {
              buf = (wchar_t*)__string;
              buf_len = m;
            }

          /* Allocate meory */
          size += (i - prev + 1 + buf_len) * sizeof (wchar_t);
          if (!res)
            {
              MALLOC_ZERO (res, size);
            }
          else
            {
              res = realloc (res, size);
            }

          /* Append skipped part of string and expand '*' character */
          wcsncat (res, __pattern + prev, i - prev);
          wcsncat (res, buf, buf_len);

          prev = i + 1;
          ++rep_iter;
        }
    }

  if (!res)
    {
      /* Theree is no '*' characters in string */
      /* so, we can just return duplicate of string */
      return wcsdup (__pattern);
    }

  /* Allocate memory for tail */
  size += (n - prev + 1) * sizeof (wchar_t);
  res = realloc (res, size);

  /* Append tail */
  wcscat (res, __pattern + prev);

  return res;
}

/**
 * Convert a string to an integer
 *
 * @param __str - string to be converted
 * @return long integer value
 */
long
wtol (const wchar_t *__str)
{
  long res;
  wchar_t *stopwcs;

  res = wcstol (__str, &stopwcs, 10);

  return res;
}

/**
 * Convert file mask to regular expression
 *
 * @param __mask - file mask to be converted
 * @return non-compiled regular expression on success, NULL otherwise
 * @sideeffect allocate memory for return value
 */
wchar_t*
file_mask_to_regexp (wchar_t *__mask, BOOL __case_insens)
{
  wchar_t *regexp;
  size_t regexp_len;
  const wchar_t *special = L"^$+[]()/.";
  wchar_t *in = __mask, *out;
  wchar_t modifiers[8] = {0};

  /* Calculate how much memory we should allocate for regexp */

  /* 4 because of starting and finishing slashes and */
  /* string beginning and finishing matchers */
  regexp_len = 4;
  while (*in != L'\0')
    {
      /* `*` will be replaced with `.*` */
      if (*in == '*')
        {
          regexp_len += 2;
          ++in;
          continue;
        }

      if (wcschr (special, *in))
        {
          ++regexp_len;
        }

      ++regexp_len;
      ++in;
    }

  if (__case_insens)
    {
      wcscat (modifiers, L"i");
      ++regexp_len;
    }

  /* Build regexp string */
  regexp = malloc ((regexp_len + 1) * sizeof (wchar_t));
  wcscpy (regexp, L"/^");

  in  = __mask;
  out = regexp + 2;

  while (*in != L'\0')
    {
      /* Replace `*` with `.*` */
      if (*in == '*')
        {
          *out++ = L'.';
          *out++ = L'*';
          ++in;
          continue;
        }

      /* Replace `?` with `.` */
      if (*in == '?')
        {
          *out++ = L'.';
          ++in;
          continue;
        }

      /* Escape regexp special characters */
      if (wcschr (special, *in))
        {
          *out++ = L'\\';
        }
      *out++ = *in++;
    }

  *out = L'\0';
  wcscat (out, L"$/");
  wcscat (out, modifiers);

  return regexp;
}

/**
 * Split string by separator
 *
 * @param __s - source string
 * @param __sep - separator string
 * @param __out - pointer to array of string
 * @sideeffect allocate memory for return value.
 * Use free_explode_array to free it.
 */
long
explode (const wchar_t *__s, const wchar_t *__sep, wchar_t ***__out)
{
  wchar_t *token;
  size_t i, n, count, token_len, sep_len;

  (*__out) = 0;

  token = malloc ((wcslen (__s) + 1) * sizeof (wchar_t));

  count = 0;
  token_len = 0;
  sep_len = wcslen (__sep);

  i = 0;
  n = wcslen (__s);
  while (i < n)
    {
      if (wcsncmp (__s + i, __sep, sep_len) == 0)
        {
          /* Current token is a separator */
          /* We should append gotten token to array and skip separator */

          /* Set null-terminator */
          token[token_len] = 0;

          /* Append string to array */
          EXPLODE_APPEND_STRING (token);

          token_len = 0;
          i += sep_len;
        }
      else
        {
          token[token_len++] = __s[i++];
        }
    }

  if (token_len > 0)
    {
      /* Set null-terminator */
      token[token_len] = 0;

      /* Append string to array */
      EXPLODE_APPEND_STRING (token);
    }

  free (token);

  /* Append null-terminator of array */
  EXPLODE_APPEND_STRING (NULL);

  return count - 1;
}

/**
 * Free memory used by result of explode() function
 *
 * @param __self - array to be freed
 */
void
free_explode_array (wchar_t **__self)
{
  int i = 0;

  while (__self[i])
    {
      free (__self[i++]);
    }

  free (__self);
}
