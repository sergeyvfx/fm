/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Mountlist manipulating module
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "mountlist.h"

#include <deque.h>

#include <stdio.h>
#include <paths.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>

#define _PATH_PROC_MOUNTS "/proc/mounts"
#define MTAB_LINE_LENGTH 4096

#define IS_SPACE(_ch) \
  ((_ch) == ' ' || (_ch) == '\t')

#define IS_OCTAL(_ch) \
  (((_ch) & ~7) == '0')

#define GET_INT_FILED(_field) \
  { \
    if (isdigit (*s)) \
      { \
        res->_field = atoi (s); \
        while (isdigit (*s)) \
          { \
            s++; \
          } \
      } \
    else \
      { \
        res->_field = 0; \
      } \
  }

/**
 * Skip spaces in string
 *
 * @param __s - string where spaces will be skipped
 * @return pointer to first non-space character in string
 */
static char*
skip_spaces (const char *__s)
{
  if (!__s)
    {
      return NULL;
    }

  while (IS_SPACE (*__s))
    {
      ++__s;
    }

  return (char*)__s;
}

/**
 * Skip non-spaces in string
 *
 * @param __s - string where non-spaces will be skipped
 * @return pointer to first space character in string
 */
static char*
skip_nospaces (const char *__s)
{
  if (!__s)
    {
      return NULL;
    }

  while (!IS_SPACE (*__s))
    {
      ++__s;
    }

  return (char*)__s;
}

/**
 * Skip field of mount table
 *
 * @param __s - string where field will be skipped
 * @return pointer to next field in string
 */
static char*
skip_field (const char *__s)
{
  return skip_spaces (skip_nospaces (__s));
}

/**
 * Parse string of table to get field
 *
 * @sizeeffect acclocate memoty for return value
 * @return field of mount table
 */
static char*
unmangle (const char *__s)
{
  size_t i, len, ptr;
  char *s, *res;

  if (!__s)
    {
      return NULL;
    }

  s = skip_spaces (__s);
  i = ptr = 0;
  len = strlen (s);

  res = malloc ((len + 1) * sizeof (char));

  while (i < len)
    {
      /* Space is a delimeter of fields */
      if (IS_SPACE (s[i]))
        {
          break;
        }

      if (s[i] == '\\' && IS_OCTAL(s[i + 1]) &&
          IS_OCTAL(s[i + 2]) && IS_OCTAL(s[i + 3]))
        {
          /* Encoded character */
          res[ptr++] = 64 * (s[i + 1] & 7) + 8 * (s[i + 2] & 7) +
                  (s[i + 3] & 7);
          i += 4;
        }
      else
        {
          res[ptr++] = s[i];
          ++i;
        }
    }

  return res;
}

/**
 * Get field of table row and convert it to wide-char string
 *
 * @sideeffect allocate memory for return value
 * @return field of table
 */
static wchar_t*
unmangle_and_convert (const char *__s)
{
  char *s;
  wchar_t *res;

  if (!__s)
    {
      return NULL;
    }

  s = unmangle (__s);

  if (s == NULL)
    {
      /* Error parse string */
      return NULL;
    }

  /* Convert multi-byte to wide-char */
  MBS2WCS (res, s);

  /* Free temporary used memory */
  free (s);

  return res;
}

/**
 * Get mount point from specified file
 *
 * @param __stream - file to read mount point from
 * @return mountpoint's descriptor
 */
static mountpoint_t*
get_mountpoint (FILE *__stream)
{
  char buf[MTAB_LINE_LENGTH];
  char *s;
  mountpoint_t *res;

  do
    {
      if (fgets (buf, sizeof (buf), __stream) == NULL)
        {
          /* No buffer read. End of file? */
          return NULL;
        }

      s = index (buf, '\n');
      if (s == NULL)
        {
          if (feof (__stream))
            {
              /* It's just no newline at end of file */
              s = index (buf, 0);
            }
          else
            {
              /* Line too long. Assume file is corrupted */

              /* Ignore the rest of line and try to read new line*/
              continue;
            }
        }

      /* Kill EOLN characters */
      *s = 0;
      if (--s >= buf && *s == '\r')
        {
          *s = 0;
        }

      s = skip_spaces (buf);
    }
  while (*s == 0 || *s == '#');

  MALLOC_ZERO (res, sizeof (mountpoint_t));

  /* Parse row */
  res->fsname = unmangle_and_convert (s);
  s = skip_field (s);
  res->dir = unmangle_and_convert (s);
  s = skip_field (s);
  res->type = unmangle_and_convert (s);
  s = skip_field (s);
  res->opts = unmangle_and_convert (s);
  s = skip_field (s);

  GET_INT_FILED (freq);
  s = skip_spaces (s);
  GET_INT_FILED (passno);

  return res;
}

/**
 * Read list of mounted file systems from specified file
 *
 * @param __stream - file to read list from
 * @param __list - list of mounted file systems
 * @return count of filesystems
 */
static int
read_file (FILE *__stream, mountpoint_t ***__list)
{
  mountpoint_t **list = NULL;
  mountpoint_t *mp;
  int count = 0;

  while ((mp = get_mountpoint (__stream)))
    {
      list = realloc (list, (count + 1) * sizeof (mountpoint_t*));
      list[count++] = mp;
    }

  /* NULL-terminator */
  list = realloc (list, (count + 1) * sizeof (mountpoint_t*));
  list[count] = NULL;

  (*__list) = list;

  return count;
}

/********
 * User's backend
 */

/**
 * Get list of mounted file systems
 *
 * @param __list - list of mounted file systems
 * @return count of filesystems or -1 in case of error
 */
int
vfs_localfs_get_mountlist (mountpoint_t ***__list)
{
  FILE *stream;
  int res;

  /* Try to open file from _PATH_MOUNTED (often "/etc/mtab") */
  stream = fopen (_PATH_MOUNTED, "r");
  if (stream == NULL)
    {
      /* Try to open file from _PATH_PROC_MOUNTS (often "/proc/mounts") */
      stream = fopen (_PATH_PROC_MOUNTS, "r");
    }

  if (stream == NULL)
    {
      /* Fatal error - we can not read mount table! */
      return -1;
    }

  res = read_file (stream, __list);

  fclose (stream);

  return res;
}

/**
 * Free list of mounted file systems
 *
 * @param __list - list to be freed
 */
void
vfs_localfs_free_mountlist (mountpoint_t **__list)
{
  int i;
  mountpoint_t *mp;

  if (!__list)
    {
      return;
    }

  i = 0;
  while (__list[i])
    {
      mp = __list[i++];

      free (mp->fsname);
      free (mp->dir);
      free (mp->type);
      free (mp->opts);

      free (mp);
    }

  free (__list);
}
