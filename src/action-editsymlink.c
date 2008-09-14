/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "actions.h"
#include "messages.h"
#include "dir.h"
#include "file.h"

#include <widgets/widget.h>
#include <vfs/vfs.h>

#define CANCEL_TO_ABORT(_a) \
  ((_a) == MR_CANCEL ? ACTION_ABORT : (_a))

/**
 * Display an error message with buttons Retry and Cancel
 *
 * @param __text - text to display on message
 * @return result of message_box()
 */
static int
error (const wchar_t *__text, ...)
{
  int res;
  wchar_t buf[4096];
  PACK_ARGS (__text, buf, BUF_LEN (buf));
  res = message_box (_(L"Error"), buf, MB_CRITICAL | MB_RETRYCANCEL);
  return res;
}

/**
 * Show dialog to user and get new content of link
 *
 * @param __item - name of item from file panel
 * @param __content - filename of symlink
 * @return zero on succss, non-zero otherwise
 */
static int
show_dialog (const wchar_t *__item, wchar_t **__content)
{
  int res, left, dummy;
  w_window_t *wnd;
  w_edit_t *e_cnt;
  w_button_t *btn;
  wchar_t *buf, fit[20];

  wnd = widget_create_window (_(L"Edit symlink"), 0, 0, 60, 6, WMS_CENTERED);

  buf = malloc (1024 * sizeof (wchar_t));
  fit_filename (__item, 20, fit);
  swprintf (buf, 1024, _(L"Symlink \"%ls\" pints to:"), fit);
  widget_create_text (WIDGET_CONTAINER (wnd), buf, 1, 1);
  e_cnt = widget_create_edit (WIDGET_CONTAINER (wnd), 1, 2,
                              wnd->position.width - 2);
  w_edit_set_text (e_cnt, *__content);
  w_edit_set_shaded (e_cnt, TRUE);
  free (buf);

  /* Create buttons */
  dummy = widget_shortcut_length (_(L"_Ok"));
  left = (wnd->position.width - dummy -
          widget_shortcut_length (_(L"_Cancel")) - 11) / 2;

  btn = widget_create_button (WIDGET_CONTAINER (wnd), _(L"_Ok"), left,
                              wnd->position.height - 2, WBS_DEFAULT);
  btn->modal_result = MR_OK;

  left += dummy + 7;
  btn = widget_create_button (WIDGET_CONTAINER (wnd), _(L"_Cancel"), left,
                              wnd->position.height - 2, 0);
  btn->modal_result = MR_CANCEL;

  /* Show window */
  res = w_window_show_modal (wnd);

  /* Get text from edit widget */

  /* Free previous content */
  free ((wchar_t*)*__content);

  *__content = wcsdup (w_edit_get_text (e_cnt));

  widget_destroy (WIDGET (wnd));

  if (res == MR_OK)
    {
      return ACTION_OK;
    }

  return ACTION_ABORT;
}

/**
 * Main logick for editing symbolic links
 *
 * @param __item - name of item from file panel
 * @param __filename - filename of symlink
 * @return zero on success, non-zero otherwise
 */
static int
make_editsymlink (const wchar_t *__item, const wchar_t *__filename)
{
  int res;
  wchar_t *content, *prev;

  if (!__filename)
    {
      /* Invalid argument */
      return ACTION_ERR;
    }

  /* Read content of symbolic link */
  content = malloc (4096 * sizeof (wchar_t));
  ACTION_REPEAT (res = vfs_readlink (__filename, content, 4096);
                 if (res >= 0)
                   {
                     res = 0;
                   },
                 error,
                 return CANCEL_TO_ABORT (__dlg_res_),
                 _(L"Cannot read content of symbolic link \"%ls\":\n%ls"),
                 __filename, vfs_get_error (res));

  prev = wcsdup (content);
  if (show_dialog (__item, &content) != ACTION_OK)
    {
      free (content);
      return ACTION_ABORT;
    }

  /* If content of symlink hasn't been changed */
  /* we should do nothing */
  if (wcscmp (prev, content) != 0)
    {
      /*
       * TODO: Could it be done smarter?
       */

      /* Unlink old symlink */
      ACTION_REPEAT (res = vfs_unlink (__filename), error,
                     res = CANCEL_TO_ABORT (__dlg_res_); goto __error_;,
                     _(L"Cannot unlink symbolic link \"%ls\":\n%ls"),
                     __filename, vfs_get_error (res));

      /* Create new symlink */
      ACTION_REPEAT (res = vfs_symlink (content, __filename), error,
                     res = CANCEL_TO_ABORT (__dlg_res_); goto __error_;,
                     _(L"Cannot create symbolic link \"%ls\":\n%ls"),
                     __filename, vfs_get_error (res));

    }

  /* Free used memory */
  free (content);
  free (prev);

  return ACTION_OK;
__error_:
  free (content);
  free (prev);

  return res;
}

/********
 * User's backend
 */

/**
 * Edit content of existing symbolic link
 *
 * @param __panel - source panel
 * @return zero on success, non-zero otherwise
 */
int
action_editsymlink (file_panel_t *__panel)
{
  file_panel_item_t *item;
  wchar_t *cwd, *full;
  int res;

  if (!__panel)
    {
      return ACTION_ERR;
    }

  /* Get item under cursor */
  item = &__panel->items.data[__panel->items.current];

  if (!item)
    {
      /* No item to edit */
      return ACTION_ERR;
    }

  if (!S_ISLNK (item->file->lstat.st_mode))
    {
      /* We can change content only of symbolic link */

      wchar_t *msg = malloc (1024 * sizeof (wchar_t));

      /*
       * TODO: Should we use fit_filename() here?
       */
      swprintf (msg, 1024, _(L"\"%ls\" is not a symbolic link"),
               item->file->name);

      MESSAGE_ERROR (msg);

      free (msg);

      return ACTION_ERR;
    }

  /* Full CWD URL */
  cwd = file_panel_get_full_cwd (__panel);

  /* Full symbolic file name */
  full = wcdircatsubdir (cwd, item->file->name);

  res = make_editsymlink (item->file->name, full);

  /* Free used memory */
  free (cwd);
  free (full);

  /* Now we should rescan panels */
  file_panel_rescan (__panel);

  return res;
}
