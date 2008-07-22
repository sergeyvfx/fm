/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Error context managing stuff
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "vfs.h"

#include <util.h>

#include <stdarg.h>
#include <wchar.h>

////////
//

typedef struct {
  wchar_t *name;
  wchar_t *value;
} vfs_context_opt_t;

vfs_context_opt_t *context=NULL;

#define MAX_VARIABLE_LEBGTH 100

////////
//

/**
 * Frees stored context
 */
static void
free_context                      (void)
{
  int i=0;

  if (!context)
    return;

  while (context[i].name)
    {
      SAFE_FREE (context[i].name);
      SAFE_FREE (context[i].value);
    }

  free (context);
  context=NULL;
}

////////
// User's backend

/**
 * Initializes context error's stuff
 *
 * @return zero on success, non-zero otherwise
 */
int
vfs_context_init                  (void)
{
  return VFS_OK;
}

/**
 * Unitializes context error's stuff
 */
void
vfs_context_done                  (void)
{
  free_context ();
}

/**
 * Saves context for formating error descriptions
 */
void
vfs_context_save                  (wchar_t *__optname1, ...)
{
  va_list args;
  va_start (args, __optname1);
  wchar_t *opt_name=__optname1,
          *opt_value;
  int count=0;

  // Free previously stored context
  free_context ();

  // Review all oprions and values
  while (opt_name)
    {
      opt_value=(wchar_t*)va_arg (args, wchar_t*);
      context=realloc (context, (count+1)*sizeof (vfs_context_opt_t));

      // Store name of option
      context[count].name = wcsdup (opt_name);

      // Store value of option
      if (opt_value)
        context[count].value = wcsdup (opt_value); else
        context[count].value = NULL;

      opt_name=(wchar_t*)va_arg (args, wchar_t*);
      count++;
    }

  va_end (args);
}

/**
 * Format string with saved context
 *
 * @param __str - string to be formated
 */
void
vfs_context_format                (wchar_t *__str)
{
  int i=0;

  // 3=three special characters+NULL-terminator  
  wchar_t dummy[MAX_VARIABLE_LEBGTH+4];

  while (context && context[i].name)
    {
      // Format full variable name (with special characters)
      swprintf (dummy, MAX_VARIABLE_LEBGTH+4, L"${%ls}", context[i].name);

      // Replace variable in string
      wcsrep (__str, MAX_VARIABLE_LEBGTH+4, dummy,
        context[i].value?context[i].value:L"");

      i++;
    }
}
