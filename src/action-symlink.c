/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of action 'Make symlink'
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "actions.h"
#include "i18n.h"
#include "messages.h"
#include "dir.h"
#include "i18n.h"

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
 * Show dialog to prove content and filename of symbolik link
 *
 * @param __content - content of symlink
 * @param __fn - file name of symlink
 * @return zero on success, non-zero otherwise
 */
static int
show_dialog (wchar_t **__content, wchar_t **__fn)
{
  int res, left, dummy;
  w_window_t *wnd;
  w_edit_t *e_cnt, *e_fn;
  w_button_t *btn;

  wnd = widget_create_window (_(L"Symbolic link"), 0, 0, 60, 9, WMS_CENTERED);

  widget_create_text (WIDGET_CONTAINER (wnd), _(L"Existing filename "
                      "(filename symlink will point to):"), 1, 1);
  e_cnt = widget_create_edit (WIDGET_CONTAINER (wnd), 1, 2,
                              wnd->position.width - 2);
  w_edit_set_text (e_cnt, *__content);
  w_edit_set_shaded (e_cnt, TRUE);

  widget_create_text (WIDGET_CONTAINER (wnd), _(L"Symbolic link filename:"),
                      1, 4);
  e_fn = widget_create_edit (WIDGET_CONTAINER (wnd), 1, 5,
                             wnd->position.width - 2);
  w_edit_set_text (e_fn, *__fn);
  w_edit_set_shaded (e_fn, TRUE);

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

  /* Get texts from edit widgets */

  /* Free previous content */
  free ((wchar_t*)*__content);
  free ((wchar_t*)*__fn);

  *__content = wcsdup (w_edit_get_text (e_cnt));
  *__fn = wcsdup (w_edit_get_text (e_fn));

  widget_destroy (WIDGET (wnd));

  if (res == MR_OK)
    {
      return ACTION_OK;
    }

  return ACTION_ABORT;
}

/**
 * This function calls vfs_symlink() in repeated mode.
 * Need to make make_symlink() a bit shorter.
 *
 * @param __content - content of symlink
 * @param __filename symlink filename
 * @return zero on success, non-zero otherwise
 */
static int
do_make_symlink (const wchar_t *__content, const wchar_t *__filename)
{
  int res;

  ACTION_REPEAT (res = vfs_symlink (__content, __filename),
                 error,
                 return CANCEL_TO_ABORT (__dlg_res_),
                 _(L"Cannot create symbolic link \"%ls\" "
                   L"with content \"%ls\":\n%ls"),
                 __filename, __content, vfs_get_error (res));

  return ACTION_OK;
}

/**
 * Logick of making symlink
 *
 * @param __item - name of item under cursor
 * @param __base_dir - base directory
 * @param __opposite_dir - directory name at opposite panel
 * @return zero on success, non-zero otherwise
 */
static int
make_symlink (const wchar_t *__item, const wchar_t *__base_dir,
              const wchar_t *__oppsite_dir)
{
  int res;
  wchar_t *content, *filename;

  /* Get default content and filename of link */
  content = wcdircatsubdir (__base_dir, __item);
  filename = wcdircatsubdir (__oppsite_dir, __item);

  res = show_dialog (&content, &filename);

  if (res)
    {
      /* Free used memory */
      free (content);
      free (filename);
      return res;
    }

  /*
   * TODO: Add appending base directory if filename is
   *       relative here.
   */

  res = do_make_symlink (content, filename);

  /* Free used memory */
  free (content);
  free (filename);

  return res;
}

/********
 * User's backend
 */

/**
 * Make a symlink to file from specified panel
 *
 * @param __panel - source panel
 * @return zero on success, non-zero otherwise
 */
int
action_symlink (file_panel_t *__panel)
{
  file_panel_t *opposite_panel;
  file_panel_item_t *item;
  wchar_t *cwd, *dst;
  int res;

  /*
   * We'll make here only some general checking and initialization.
   * All logick will be made in make_symlink()
   */

  if (!__panel)
    {
      return ACTION_ERR;
    }

  /* Check file panels count */
  if (file_panel_get_count () <= 1)
    {
      MESSAGE_ERROR (_(L"Smymbolic link may be made at least "
                        "with two file panels"));
      return ACTION_ERR;
    }

  /* Get second panel to operate */
  opposite_panel = action_choose_file_panel (_(L"Symbolic link"),
                                             _(L"Target panel"));

  if (!opposite_panel)
    {
      /* User aborted action */
      return ACTION_ABORT;
    }

  /* Get item under cursor */
  item = &__panel->items.data[__panel->items.current];

  if (!item)
    {
      return ACTION_ERR;
    }

  /* Full source and destination URLs */
  cwd = file_panel_get_full_cwd (__panel);
  dst = file_panel_get_full_cwd (opposite_panel);

  res = make_symlink (item->file->name, cwd, dst);

  free (cwd);
  free (dst);

  /* We should redraw both of the panels, because */
  /* they can have the same CWD in which symlink has been created */
  file_panel_rescan (__panel);
  file_panel_rescan (opposite_panel);

  return res;
}
