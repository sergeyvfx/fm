/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of action `Create file`
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "actions.h"
#include "i18n.h"

#include <widgets/widget.h>
#include <vfs/vfs.h>

/**
 * Show dialog to get name of file to create
 *
 * @return pointer to a buffer where new file name is stored
 * @sideeffect allocate memory for output value
 */
static wchar_t*
get_file_name (void)
{
  wchar_t *res = NULL;
  w_window_t *wnd = widget_create_window (_(L"Create file"), 0, 0,
                                          MIN (50, SCREEN_WIDTH * 0.8), 6,
                                          WMS_CENTERED);
  w_edit_t *edt;

  widget_create_text (NULL, WIDGET_CONTAINER (wnd),
                      _(L"Name of file to create:"), 1, 1);

  edt = widget_create_edit (NULL, WIDGET_CONTAINER (wnd), 1, 2,
                            WIDGET_POSITION (wnd).width - 2);

  action_create_ok_cancel_btns (wnd);

  if (w_window_show_modal (wnd) == MR_OK)
    {
      res = wcsdup (w_edit_get_text (edt));
    }

  widget_destroy (WIDGET (wnd));

  return res;
}

/**
 * Create file with specified name
 *
 * @param __file_name - name of file to create
 * @return non-zero if file created, zero otherwise
 */
static BOOL
create_file (const wchar_t *__file_name)
{
  int res;
  vfs_file_t fd;

  ACTION_REPEAT (fd = vfs_open (__file_name, O_CREAT, &res, 0),
                 action_error_retrycancel,
                 return FALSE,
                 _(L"Cannot create file \"%ls\":\n%ls"), __file_name,
                 vfs_get_error (res));

  vfs_close (fd);

  return TRUE;
}

/**
 * Create file operation
 *
 * @param __panel - descriptor of file panel where file will be created
 * @return zero on success, non-zero otherwise
 */
int
action_create_file (file_panel_t *__panel)
{
  wchar_t *file_name;

  file_name = get_file_name ();

  if (file_name)
    {
      wchar_t *cwd = file_panel_get_full_cwd (__panel);
      wchar_t *full_name = vfs_abs_path (file_name, cwd);

      create_file (full_name);

      /* Rescan panel and set cursor to created item */
      file_panel_rescan (__panel);
      action_centre_to_item (__panel, full_name);

      free (file_name);
      free (cwd);
      free (full_name);
    }

  return 0;
}
