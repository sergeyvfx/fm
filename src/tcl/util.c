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

#include <tcl.h>
#include <string.h>
#include "util.h"

/**
 * Tcl specific strdup
 *
 * @see man strdup
 */
char *
ckstrdup(const char *s)
{
  if (s == NULL)
    {
      return NULL;
    }

  size_t length = strlen(s);
  char *newstring = ckalloc(length + 1);

  strncpy (newstring, s, length);
  newstring [length] = '\0';

  return newstring;
}
