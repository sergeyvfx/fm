/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Regular expressions module
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "regexp.h"
#include "util.h"

#ifdef USE_PCRE
#  include <pcre.h>
#endif

#include <wchar.h>

/********
 * Some macro-definitions
 */
#define PF_NAMED_STRING 0x0001

#define REGEXP_OK                     0
#define REGEXP_INVALID_NAMED_STRING  -1

#ifdef USE_PCRE
/* It's quite too dangerous */
#  define REGEXP_REPLACE_GLOBAL 0x10000000
#endif

/********
 * Type definitions
 */

struct regexp
{
  void *handle;
  int modifiers;
};

typedef struct
{
   char ch;
   int code;
} regexp_modifier_t;

/********
 *
 */

static regexp_modifier_t modifiers[] = {

#ifdef USE_PCRE
  /* $ matches only at end */
  /*  {'', PCRE_DOLLAR_ENDONLY}, */

  /* strict escape parsing */
  /*  {'', PCRE_EXTRA}, */

  /* handles UTF-8 chars */
  /*  {'', PCRE_UTF8}, */

  /* reverses * and *? */
  /*  {'', PCRE_UNGREEDY}, */

  /* disables capturing parents */
  /*  {'', PCRE_NO_AUTO_CAPTURE}, */

  /* case insensitive match */
  {'i', PCRE_CASELESS},

  /* multiple lines match */
  {'m', PCRE_MULTILINE},

  /* dot matches newlines */
  {'s', PCRE_DOTALL},

  /* ignore white spaces */
  {'x', PCRE_EXTENDED},

  /* all occurrences will be replaced */
  {'g', REGEXP_REPLACE_GLOBAL},
#endif

  {0,0}
};

/********
 * Internal stuff
 */

/**
 * Parsing iterator for replacing stuff
 *
 * @param __data - data to parse
 * @param __token - pointer to string where token will be placed
 * @param __flags - flags of token
 * @param __errno - number of occurred error
 * @return new shift of data
 */
static char*
replace_parser_iterator (const char *__data, char **__token,
                         int *__flags, int *__errno)
{
  int len = 0;
  char c;

  (*__flags) = 0;
  (*__errno) = REGEXP_OK;

  if (!__data || !*__data)
    {
      /* There is no data to parse */
      return NULL;
    }

  for (;;)
    {
      c = *__data;

      if (!c)
        {
          /* End of line occurred */
          break;
        }

      if (c == '$')
        {
          /* Named string */
          if (len)
            {
              /* Token is not empty - return it for correct replacing */
              break;
            }

          (*__flags) |= PF_NAMED_STRING;
          ++__data;

          for (;;)
            {
              c = *__data;
              if (!c || !(c >= '0' && c <= '9'))
                {
                  break;
                }

              *(*__token + len++) = c;
              ++__data;
            }

          if (len==0)
            {
              (*__errno) |= REGEXP_INVALID_NAMED_STRING;
            }
          break;
        }
      else
        {
          if (c=='\\')
            {
              /* Escaped character */
              if (__data[1] == 'n')
                {
                  *(*__token + len++) = '\n';
                }
              else if (__data[1] == 'r')
                {
                  *(*__token + len++) = '\r';
                }
              else if (__data[1] == 't')
                {
                  *(*__token + len++) = '\t';
                }
              else
                {
                  *(*__token + len++) = __data[1];
                }
              ++__data;
            }
          else
            {
              *(*__token+len++)=c;
            }
        }
      ++__data;
    }

  *(*__token + len++) = 0;

  return (char*)__data;
}

/**
 * Get code of modifier, specified by char
 *
 * @param __ch - specifier of modifier
 * @param __modifiers - array of known modifiers
 * @return modifier's code if found, -1 otherwise
 */
static int
get_modifier_code (char __ch, const regexp_modifier_t *__modifiers)
{
  int i = 0;

  while (__modifiers[i].ch)
    {
      if (__modifiers[i].ch == __ch)
        {
          return __modifiers[i].code;
        }
      i++;
    }

  return -1;
}

/**
 * Parse regular expression, combined with options
 *
 * @param __s - string where source regular expression is written
 * @param __regexp - pointer to buffer where regexp will be stored
 * @param __modifiers - pointer to variable, where regexp's
 * modifiers will be stored
 * @return zero on success, non-zero otherwise
 */
static int
parse_regexp (const char *__s, char **__regexp, int *__modifiers)
{
  size_t len = 0;
  int code;
  char c;
  BOOL regexp_opened = FALSE, regexp_closed = FALSE;

  (*__modifiers) = 0;

  /* Get regexp */
  for (;;)
    {
      c = *__s;

      if (!c)
        {
          /* String is over */
          break;
        }

      if (c == '/')
        {
          if (!regexp_opened)
            {
              if (len != 0)
                {
                  /* Trying to open regexp from non-zero character */
                  return -1;
                }
              regexp_opened = TRUE;
            } else
            {
              /* Caught the closing of regexp */

              /* regexp is successfully closed */
              regexp_closed = TRUE;

              /* Go to the first modifer and stop getting regexp */
              __s++;
              break;
            }
        }
      else
        {
          *(*__regexp + len++) = c;
        }

      __s++;
    }

  if (!regexp_closed)
    {
      /* Abnormal closing of regexp */
      return -1;
    }

  /* Get modifiers */
  for (;;)
    {
      c = *__s;

      if (!c)
        {
          /* String is over */
          break;
        }

      code = get_modifier_code (c, modifiers);

      if (code > 0)
        {
          *__modifiers |= code;
        }
      else
        {
          /* Invalid modifier */
          return -1;
        }

      __s++;
    }

  *(*__regexp+len)=0;

  return 0;
}

/**
 * Get vector of occurrences
 *
 * @param __re - descriptor of regular expression
 * @param __str - string to operate with
 * @param __ovector - pointer to buffer, where occurrences will be stored
 * @param __ovector_size - maximal size of output vector
 * @return count of occurrences
 */
static int
regexp_get_vector (const regexp_t *__re, const char *__str,
                   int *__ovector, int __ovector_size)
{
  int result = 0;

  if (!__re || !__str || !__ovector)
    {
      return 0;
    }

#ifdef USE_PCRE
  result = pcre_exec (__re->handle, NULL, __str, strlen (__str), 0, 0,
                      __ovector, __ovector_size);
#endif

  return result;
}

/**
 * Iterator for regexp_replace()
 *
 * @param __re - compiled regular expression
 * @param __s - source string
 * @param __mask - mask of replacement
 * @param __l - left offset of replaced sub-string
 * @param __r - right offset of replaced sub-string
 * @return replaced string
 * @sideeffect allocate memory for return value
 */
static char*
regexp_replace_iterator (const regexp_t *__re,
                         const char *__s, const char *__mask,
                         size_t *__l, size_t *__r)
{
  int i, vector_count;

  char *token, *shift, *append;
  char *out = NULL;
  char **substrings = NULL;

  size_t len, cur_size = 0;

  int ovector_size = (strlen (__s) + 1) * 2;
  int *ovector = malloc (sizeof (int) * ovector_size);
  int flags, errno;

  (*__l) = (*__r) = 0;

  /* Get the vector of matches */
  vector_count = regexp_get_vector (__re, __s, ovector, ovector_size);
  if (vector_count <= 0)
    {
      /* No matched sub-strings */
      return 0;
    }

  /* Get named sub-strings */
  substrings = malloc (vector_count * sizeof (char*));
  for (i = 0; i < vector_count; i++)
    {
      len = ovector[i * 2 + 1] - ovector[i * 2];
      substrings[i] = malloc (len + 1);
      substr (substrings[i], __s, ovector[i * 2], len);
    }

  shift = (char*)__mask;

  len = strlen (__mask);
  cur_size = len + 1;
  MALLOC_ZERO (out, cur_size);

  /* Token can't be longer than mask string */
  token = malloc (len + 1);

  while ((shift = replace_parser_iterator (shift, &token, &flags, &errno)))
    {
      if (errno == REGEXP_OK)
        {
          if (flags & PF_NAMED_STRING)
            {
              /* Numbered string */
              int index = atoi (token);
              if (index >= vector_count)
                {
                  /* If index is out of range, append empty string */
                  append = "";
                }
              else
                {
                  append = substrings[index];
                }
            }
          else
            {
              /* Just append token */
              append = token;
            }

          len = strlen (append);
          cur_size += len;
          out = realloc (out, cur_size);

          strcat (out, append);
        }
      else
        {
          SAFE_FREE (out);
          break;
        }
    }

  free (token);

  (*__l) = ovector[0];
  (*__r) = ovector[1];

  for (i = 0; i < vector_count; ++i)
    {
      free (substrings[i]);
    }

  free (substrings);
  free (ovector);

  return out;
}

/********
 * User's backend
 */

/**
 * Compile regular expression
 *
 * @param __regexp - regular expression to compile
 * @return descriptor of compiled regular expression
 * @sideeffect allocate memory for return value. Use regexp_free() to free.
 */
regexp_t*
regexp_compile (const char *__regexp)
{
  regexp_t *result = NULL;
  int modifiers;
  char *parsed_regexp = malloc (strlen (__regexp) + 1);

  if (!parse_regexp (__regexp, &parsed_regexp, &modifiers))
    {
      void *re;

#ifdef USE_PCRE
      const char *err;
      int pos;

      /* Clear non-pcre-based modifiers */
      int pcre_options = modifiers & ~REGEXP_REPLACE_GLOBAL;

      re = pcre_compile (parsed_regexp, pcre_options, &err, &pos, NULL);
#else
#  error Regular expression engine to use is not defined
#endif

      /* Error in regexp */
      if (!re)
        {
          free (parsed_regexp);
          return NULL;
        }

      /* Create descriptor */
      MALLOC_ZERO (result, sizeof (struct regexp));
      result->handle = re;
      result->modifiers = modifiers;
    }

  free (parsed_regexp);

  return result;
}

/**
 * Free compiled regular expression descriptor
 *
 * @param __regexp - regexp to free
 */
void
regexp_free (regexp_t *__regexp)
{
  if (!__regexp || !__regexp->handle)
    {
      return;
    }

  free (__regexp->handle);
  free (__regexp);
}

/**
 * Check is string matches to compiled regular expression
 *
 * @param __re - compiled regular expression to use
 * @param __str - string to check
 * @return non-zero if string matches to regular expression, zero otherwise
 */
BOOL
regexp_match (const regexp_t *__re, const char *__str)
{
  /* Assume maximal count of elements in vector to each element of string */
  /* and the whole string. */
  /* Multiplying by 2 is needed because vector will store beginning */
  /* and ending of occurrence */
  int ovector_size = (strlen (__str) + 1) * 2;

  int *ovector = malloc (sizeof (int) * ovector_size);
  int res;

  res = regexp_get_vector (__re, __str, ovector, ovector_size);

  free (ovector);

  return res > 0;
}

/**
 * Make sub-string replacing by regular compiled expression matching
 *
 * @param __re - compiled regular expression
 * @param __s - source string
 * @param __mask - mask of replacement
 * @return replaced string
 * @sideeffect allocate memory for return value
 */
char*
regexp_replace (const regexp_t *__re, const char *__s, const char *__mask)
{
  char *shift = (char*)__s;
  char *dummy, *prefix = NULL, *buf = NULL;
  size_t l, r, len, buf_len = 0, prefix_len = 0;

  for (;;)
    {
      dummy = regexp_replace_iterator (__re, shift, __mask, &l, &r);
      len = 0;

      if (dummy)
        {
          len = strlen (dummy);
        }

      if (l > 0)
        {
          /* There is non-empty prefix */

          if (prefix_len < l)
            {
              prefix_len = l;
              prefix = realloc (prefix, prefix_len + 1);
            }

          substr (prefix, shift, 0, l);
        }
      else
        {
          if (!prefix)
            {
              prefix = strdup ("");
              prefix_len = 0;
            }
          else
            {
              prefix[0] = 0;
            }
        }

      if (l + len > 0)
        {
          BOOL zerolize = 0;
          /* It there is something to append */
          buf_len += l + len;

          if (!buf)
            {
              zerolize = TRUE;
            }

          buf = realloc (buf, buf_len + 1);

          if (zerolize)
            {
              buf[0] = 0;
            }

          /* Append new prefix to buffer */
          strcat (buf, prefix);

          if (dummy)
            {
              /* Append replaced string and free it */
              strcat (buf, dummy);
              free (dummy);
            }
        }

      shift += r;

      if (!dummy || !r ||
          !TEST_FLAG (__re->modifiers, REGEXP_REPLACE_GLOBAL))
        {
          /* FINITO */
          break;
        }
    }

  SAFE_FREE (prefix);

  /* Append suffix */
  buf_len += strlen (shift);
  buf = realloc (buf, buf_len + 1);
  strcat (buf, shift);

  return buf;
}

/**
 * Check is string matches to regular expression
 *
 * @param __regexp - regular expression to use
 * @param __string - string to check
 * @return non-zero if string matches to regular expression, zero otherwise
 */
BOOL
preg_match (const char *__regexp, const char *__str)
{
  int dummy;
  regexp_t *re;

  /* Compile regexp */
  re = regexp_compile (__regexp);
  if (!re)
    {
      return FALSE;
    }

  dummy = regexp_match (re, __str);

  /* Free memory */
  regexp_free (re);

  return dummy;
}

/**
 * Make sub-string replacing by regular expression matching
 *
 * @param __re - compiled regular expression
 * @param __s - source string
 * @param __mask - mask of replacement
 * @return replaced string on success, NULL otherwise
 * @sideeffect allocate memory for return value
 */
char*
preg_replace (const char *__regexp, const char *__s, const char *__mask)
{
  regexp_t *re;
  char *result;

  re = regexp_compile (__regexp);

  if (!re)
    {
      return NULL;
    }

  result = regexp_replace (re, __s, __mask);

  regexp_free (re);

  return result;
}

/****
 * Wide-chared functions
 */

/**
 * Compile regular expression
 *
 * @param __regexp - regular expression to compile
 * @return descriptor of compiled regular expression
 * @sideeffect allocate memory for return value. Use regexp_free() to free.
 */
regexp_t*
wregexp_compile (const wchar_t *__regexp)
{
  regexp_t *result;
  char *mb_regexp;

  wcs2mbs (&mb_regexp, __regexp);
  if (mb_regexp != NULL)
    {
      result = regexp_compile (mb_regexp);
      free (mb_regexp);
      return result;
    }

  return NULL;
}

/**
 * Check is string matches to compiled regular expression
 *
 * @param __re - compiled regular expression to use
 * @param __str - string to check
 * @return non-zero if string matches to regular expression, zero otherwise
 */
BOOL
wregexp_match (const regexp_t *__re, const wchar_t *__str)
{
  BOOL result;
  char *mb_str;

  wcs2mbs (&mb_str, __str);
  if (mb_str != NULL)
    {
      result = regexp_match (__re, mb_str);
      free (mb_str);
      return result;
    }

  return FALSE;
}

/**
 * Check is string matches to regular expression
 *
 * @param __regexp - regular expression to use
 * @param __string - string to check
 * @return non-zero if string matches to regular expression, zero otherwise
 */
BOOL
wpreg_match (const wchar_t *__regexp, const wchar_t *__str)
{
  int dummy;
  regexp_t *re;

  /* Compile regexp */
  re = wregexp_compile (__regexp);
  if (!re)
    {
      return FALSE;
    }

  dummy = wregexp_match (re, __str);

  /* Free memory */
  regexp_free (re);

  return dummy;
}

/**
 * Make sub-string replacing by regular compiled expression matching
 *
 * @param __re - compiled regular expression
 * @param __s - source string
 * @param __mask - mask of replacement
 * @return replaced string
 * @sideeffect allocate memory for return value
 */
wchar_t*
wregexp_replace (const regexp_t *__re,
                 const wchar_t *__s, const wchar_t *__mask)
{
  char *s, *mask, *mb_res;

  wcs2mbs (&s, __s);
  wcs2mbs (&mask, __mask);

  if (s == NULL || mask == NULL)
    {
      SAFE_FREE (s);
      SAFE_FREE (mask);
      return NULL;
    }

  mb_res = regexp_replace (__re, s, mask);

  free (s);
  free (mask);

  if (mb_res)
    {
      wchar_t *result;
      mbs2wcs (&result, mb_res);
      free (mb_res);
      return result;
    }

  return NULL;
}

/**
 * Make sub-string replacing by regular expression matching
 *
 * @param __re - compiled regular expression
 * @param __s - source string
 * @param __mask - mask of replacement
 * @return replaced string on success, NULL otherwise
 * @sideeffect allocate memory for return value
 */
wchar_t*
wpreg_replace (const wchar_t *__regexp,
               const wchar_t *__s, const wchar_t *__mask)
{
  wchar_t *dummy;
  regexp_t *re;

  /* Compile regexp */
  re = wregexp_compile (__regexp);
  if (!re)
    {
      return FALSE;
    }

  dummy = wregexp_replace (re, __s, __mask);

  /* Free memory */
  regexp_free (re);

  return dummy;
}
