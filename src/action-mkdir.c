/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of action 'mkdir'
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "dir.h"
#include "actions.h"
#include "i18n.h"
#include "messages.h"

#include <widget.h>
#include <vfs/vfs.h>

#include <errno.h>

/**
 * Get directory name from user
 *
 * @return malloc'ed string where directory name is stored or
 * NULL if user canceled action
 */
static wchar_t*
get_directory_name (void)
{
  w_window_t *wnd;
  w_edit_t *edit;
  wchar_t *res = NULL;

  /* Create window */
  wnd = widget_create_window (_(L"Create a new directory"),
                              0, 0, MIN (50, SCREEN_WIDTH * 0.8), 6,
                              WMS_CENTERED);

  widget_create_text (NULL, WIDGET_CONTAINER (wnd),
                      _(L"Enter directory name:"), 1, 1);

  edit = widget_create_edit (NULL, WIDGET_CONTAINER (wnd), 1, 2,
                             wnd->position.width - 2);

  /* Create buttons */
  action_create_ok_cancel_btns (wnd);

  if (w_window_show_modal (wnd) != MR_CANCEL)
    {
      res = wcsdup (w_edit_get_text (edit));
    }

  widget_destroy (WIDGET (wnd));

  return res;
}

/**
 * Recursively creation of directory
 *
 * @param __base_dir - base directory where specified directory name will
 * be created
 * @param __dir_name - directory name to create
 * @param __orig - is __dir_name an original directory. Need to for
 * generation "Directory exists" message.
 * @return zero on success, non-zero otherwise
 */
static int
do_create_directory (const wchar_t *__base_dir, const wchar_t *__dir_name,
                     BOOL __orig)
{
  int res;
  wchar_t *parent_dir, *dummy;
  if (!__dir_name || !*__dir_name || wcscmp (__dir_name, L"/") == 0)
    {
      /* There is no directory to create */
      return 0;
    }

  /* Create parent directory */
  parent_dir = wcdirname (__dir_name);
  res = do_create_directory (__base_dir, parent_dir, FALSE);
  SAFE_FREE (parent_dir);

  if (res)
    {
      return res;
    }

  dummy = wcdircatsubdir (__base_dir, __dir_name);
  res = vfs_mkdir (dummy, 0775);
  free (dummy);

  if (res == -EEXIST && !__orig)
    {
      /* There is no error if directory is alread exists */
      /* because we use a recursively directory creation */
      res = 0;
    }

  return res;
}

/**
 * Centre cursor to created item
 *
 * @param __panel - on which panel item was created
 * @param __basedir - base directory of __dir_name
 * @param __dir_name - created directory name
 */
static void
make_centre (file_panel_t *__panel, const wchar_t *__basedir,
             const wchar_t *__dir_name)
{
  wchar_t *n_dir, *full_dir, *cwd;

  /* Get full normalized directory name */
  full_dir = wcdircatsubdir (__basedir, __dir_name);
  n_dir = vfs_normalize (full_dir);

  /* Get CWD of panel */
  cwd = file_panel_get_full_cwd (__panel);

  if (wcsncmp (cwd, n_dir, wcslen (cwd)) == 0)
    {
      wchar_t *rel_dir = n_dir + wcslen (cwd);

      /* Get name of item to select */
      if (rel_dir[0] == '/')
        {
          long i, len = 0;
          wchar_t name[MAX_FILENAME_LEN + 1] = {0};
          ++rel_dir;

          i = 0;
          while (rel_dir[i] && rel_dir[i] != '/')
            {
              name[len++] = rel_dir[i++];
            }
          name[len] = 0;

          FILE_PANEL_ACTION_CALL (__panel, centre_to_item, name);
        }
    }

  free (n_dir);
  free (full_dir);
}

/********
 * User's backend
 */

/**
 * Create directory on specified panel
 *
 * @param __panel - on which panel directory will be created
 * @return zero on success, non-zero otherwise
 */
int
action_mkdir (file_panel_t *__panel)
{
  wchar_t *dir_name, *basedir;
  int res;

  if (__panel == NULL)
    {
      return ACTION_ERR;
    }

  /* Get name of directory to create */
  dir_name = get_directory_name ();

  if (dir_name == NULL)
    {
      /* User aborted creation of a directory */
      return ACTION_ABORT;
    }

  if (dir_name[0]=='/')
    {
      size_t len;
      len = wcslen (__panel->vfs) + wcslen (VFS_PLUGIN_DELIMETER) + 1;
      basedir = malloc ((len + 1) * sizeof (wchar_t));

      if (wcscmp (__panel->vfs, VFS_LOCALFS_PLUGIN))
        {
          wcscpy (basedir, __panel->vfs);
          wcscat (basedir, VFS_PLUGIN_DELIMETER);
          wcscat (basedir, L"/");
        }
      else
        {
          wcscpy (basedir, L"/");
        }
    }
  else
    {
      basedir = file_panel_get_full_cwd (__panel);
    }

  /* Recursively creation of a directory */
  res = do_create_directory (basedir, dir_name, TRUE);

  if (res)
    {
      wchar_t msg[4096];
      swprintf (msg, BUF_LEN (msg), _(L"Error creating directory:\n%ls"),
                vfs_get_error (res));
      MESSAGE_ERROR (msg);
    }
  else
    {
      /* Rescan panel and set cursor to created item */
      file_panel_rescan (__panel);
      make_centre (__panel, basedir, dir_name);
    }

  /* Free all used memory */
  free (dir_name);
  free (basedir);

  return ACTION_OK;
}
