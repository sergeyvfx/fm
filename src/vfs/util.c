/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Different utilities for Virtual File System
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "vfs.h"
#include "dir.h"

#include <deque.h>

#include <wchar.h>

/**
 * Alphabetically sorter for vfs_scandir which sorts files and directories
 *
 * @param __a - left element of array
 * @param __b - right element of array
 * @return an integer less than, equal to, or greater than zero if the
 * first argument is considered to be respectively less than, equal to,
 * or greater than the second.
 */
int
vfs_alphasort (const void *__a, const void *__b)
{
  vfs_dirent_t *a = *(vfs_dirent_t**) __a, *b = *(vfs_dirent_t**) __b;

  if (IS_PSEUDODIR (a->name))
    {
      return -1;
    }

  return wcscmp (a->name, b->name);
}

/**
 * Normalize file name
 *
 * @return malloc()'ed  normalizedfile name.
 */
wchar_t*
vfs_normalize (const wchar_t *__fn)
{
  size_t i, len, ptr = 0;
  wchar_t *res, *s, *old_s, *item;
  deque_t *items, *n_items;
  BOOL def_plugin = FALSE;
  wchar_t symlink[MAX_FILENAME_LEN + 1];

  /* Deleter for stack */
  void free_ref_data (void *__data)
    {
      SAFE_FREE (__data);
    }

#ifdef VFS_USE_DEFAULT_PLUGIN
  /* Strip default plugin name from path */
  len = wcslen (VFS_DEFAULT_PLUGIN) + wcslen (VFS_PLUGIN_DELIMETER);
  s = malloc ((len + 1) * sizeof (wchar_t));
  wcscpy (s, VFS_DEFAULT_PLUGIN);
  wcscat (s, VFS_PLUGIN_DELIMETER);

  if (wcsncmp (__fn, s, len) == 0)
    {
      __fn += len;
      def_plugin = TRUE;
    }
  free (s);
#endif

  len = wcslen (__fn);

  /* Length of file name can not become longer after trim */
  /* At this stage only. Later we should check allocated buffer's length */
  res = malloc ((len + 1) * sizeof (wchar_t));

  /* Step 1: avoid multimly slahes  */
  for (i = 0; i < len; i++)
    {
      if (__fn[i] == '/' && ptr > 0 && res[ptr - 1] == '/')
        {
          continue;
        }
      else
        {
          res[ptr++] = __fn[i];
        }
    }

  res[ptr] = 0;

  /* Step 2: process . and .. subdirectories */

  /* Get stack of path items */
  items = deque_create ();
  s = wcsdup (res);
  while (wcscmp (s, L"") != 0 && wcscmp (s, L"/") != 0)
    {
      deque_push_front (items, wcfilename (s));
      old_s = s;
      s = wcdirname (s);
      free (old_s);
    }
  free (s);

  /* Get stack of normalized items */
  n_items = deque_create ();
  deque_foreach (items, item)
    if (wcscmp (item, L".") == 0)
      {
        deque_foreach_continue;
      }

    if (wcscmp (item, L"..") == 0)
      {
        deque_pop_back (n_items);
      }
    else
      {
        deque_push_back (n_items, item);
      }
  deque_foreach_done

  /* Collect normalized path */
  s = malloc ((len + 1) * sizeof (wchar_t));
  if (res[0] == '/')
    {
      wcscpy (s, L"/");
    }
  else
    {
      wcscpy (s, L"");
    }

  i = 0;
  wcscpy (res, s);
  deque_foreach (n_items, item)
    if (i > 0)
      {
        wcscat (s, L"/");
        wcscat (res, L"/");
      }
    wcscat (s, item);
    if (i > 0 || def_plugin)
      {
        if (vfs_readlink (s, symlink, MAX_FILENAME_LEN) > 0)
          {
            item = symlink;
          }
      }
    if (item[0] == '/')
      {
        /*
         * NOTE: This could happen only if a symlink has been detected
         *       and this symlink contains an absolutely path
         */

        if (len <= wcslen (item))
          {
            len = wcslen (item);
            res = realloc (res, (len + 1) * sizeof (wchar_t));
          }

        wcscpy (res, item);
      }
    else
      {
        /* Make sure that there is enough allocated memory */
        if (len <= wcslen (res) + wcslen (item))
          {
            len += wcslen (item) + 1;
            res = realloc (res, (len + 1) * sizeof (wchar_t));
          }

        wcscat (res, item);
      }
    ++i;
  deque_foreach_done

  /* Step 3: Free used memory */
  deque_destroy (n_items, 0);
  deque_destroy (items, free_ref_data);
  free (s);

  if (wcscmp (__fn, res) == 0)
    {
      /* Result is equal to source. We can stop normalization */
    }
  else
    {
      /* After reading symlinks new pseydodirs and symlinks could appear. */
      s = vfs_normalize (res);
      free (res);
      res = s;
    }

  /* To free unused memory */
  return realloc (res, (wcslen (res) + 1) * sizeof (wchar_t));
}
