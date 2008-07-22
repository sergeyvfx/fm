/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Error's manage stuff
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "vfs.h"

#include <i18n.h>

#include <wchar.h>
#include <stdlib.h>

////////
//

typedef struct {
  int      errcode;
  wchar_t *desc;
} error_t;

static error_t errors[]=
{
  {VFS_OK,    L"Operation succeed"},
  {VFS_ERROR, L"Operation failed"},
  {VFS_ERR_INVLAID_ARGUMENT, L"Invalid argument passed to the function"},
  {VFS_METHOD_NOT_FOUND,     L"Method '${method-name}' of "
     "plugin '${plugin-name}' not found"},

  {VFS_PLUGIN_INVALID_FORMAT, L"Invalid plugin format"},
  {VFS_PLUGIN_INIT_ERROR,     L"Error initializating plugin"},
  {VFS_ERR_PUGIN_NOT_FOUND,   L"Plugin '${plugin-name}' not found"}
};

#define MAX_ERROR_LENGTH 1024

// Maybe big troubles with this
static wchar_t current_error[MAX_ERROR_LENGTH];

////////
// Internal stuff

/**
 * Initializes error stuff
 */
static void
init                              (void)
{
  int cmp(const void *__a, const void *__b)
    {
      error_t *a=(error_t*)__a, *b=(error_t*)__b;
      return a->errcode-b->errcode;
    };

  int count=sizeof (errors)/sizeof (error_t);
  qsort (errors, count, sizeof (error_t), cmp);
}

////////
// User's backend

/**
 * Returns an error's description by it's code
 *
 * @param __errcode - code of error for which description will be returned
 * @return description of an error
 */
wchar_t*
vfs_get_error                     (int __errcode)
{
  static BOOL initialized=FALSE;
  static int n=sizeof (errors)/sizeof (error_t);
  BOOL found=FALSE;

  if (!initialized)
    {
      init ();
      initialized=TRUE;
    }

  // Try to get error's description from VFS's errors list
  int l=0, r=n-1, m;
  while (l<=r)
    {
      m=(l+r)/2;
      if (errors[m].errcode==__errcode)
        {
          wcscpy (current_error, _(errors[m].desc));
          found=TRUE;
          break;
        } else
      if (__errcode<errors[m].errcode)
        r=m-1; else
        l=m+1;
    }

  // Try get system description
  if (!found && __errcode>VFS_ERR_COMMON)
    {
      char *s=strerror (-__errcode);
      if (s)
        {
          size_t len=strlen (s);
          wchar_t *wcs=malloc ((len+2)*sizeof (wchar_t));
          mbstowcs (wcs, s, len+1);
          wcscpy (current_error, _(wcs));
          found=TRUE;
          free (wcs);
        }
    }

  if (found)
    {
      vfs_context_format (current_error);
      return current_error;
    }

  return NULL;
}
