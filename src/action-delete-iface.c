/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of iterface part of action 'delete'
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "action-delete-iface.h"
#include "i18n.h"

/**
 * Handler of clicking button 'abort' on process window
 *
 * @param __button - sender button
 * @return non-zero if action has been handled, non-zero otherwise
 */
static int
abort_button_clicked (w_button_t *__button)
{
  delete_process_window_t *wnd;
  if (!__button || !WIDGET_USER_DATA (__button))
    {
      return 0;
    }

  wnd = WIDGET_USER_DATA (__button);

  wnd->abort = TRUE;

  return TRUE;
}

/**
 * Handler of keydown message for buttons on process window
 *
 * @param __button - button on which key was pressed
 * @param __ch - code of pressed key
 */
static int
button_keydown (w_button_t *__button, wint_t __ch)
{
  if (!__button || !WIDGET_USER_DATA (__button))
    {
      return 0;
    }

  if (__ch == KEY_ESC)
    {
      /* If escaped was pressed, delete operation shoud be aborted */
      delete_process_window_t *wnd;
      wnd = WIDGET_USER_DATA (__button);
      wnd->abort = TRUE;
    }

  return 0;
}

/**
 * Create deletion progress window
 *
 * @param __total_progress - use total progress information
 * @param __listing - listing of items to be deleted
 * @return descriptor of progress window
 */
delete_process_window_t*
action_delete_create_proc_wnd (BOOL __total_progress,
                               const action_listing_t *__listing)
{
  int left, dummy, height = 5;
  delete_process_window_t *res;
  w_button_t *btn;
  wchar_t *pchar;

  MALLOC_ZERO (res, sizeof (delete_process_window_t));

  if (__total_progress)
    {
      height += 2;
    }

  res->window = widget_create_window (_(L"Delete"), 0, 0, 60, height,
                                      WMS_CENTERED);

  pchar = _(L"Deleting:");
  widget_create_text (WIDGET_CONTAINER (res->window), pchar, 1, 1);

  res->text = widget_create_text (WIDGET_CONTAINER (res->window), L"",
                                   wcswidth (pchar, wcslen (pchar)) + 1, 1);

  if (__total_progress)
    {
      res->progress = widget_create_progress (WIDGET_CONTAINER (res->window),
                                              __listing->count,
                                              1, 3,
                                              res->window->position.width - 2,
                                              0);
    }

  /* Create button */
  dummy = widget_shortcut_length (_(L"_Abort"));
  left = (res->window->position.width - dummy - 6) / 2;
  btn = widget_create_button (WIDGET_CONTAINER (res->window), _(L"_Abort"),
                              left, res->window->position.height - 2,
                              WBS_DEFAULT);
  btn->modal_result = MR_ABORT;
  WIDGET_USER_DATA (btn) = res;
  WIDGET_USER_CALLBACK (btn, clicked) = (widget_action)abort_button_clicked;
  WIDGET_USER_CALLBACK (btn, keydown) = (widget_keydown_proc)button_keydown;

  return res;
}

/**
 * Destroy deletion progress window
 *
 * @param __window - window to be deleted
 */
void
action_delete_destroy_proc_wnd (delete_process_window_t *__window)
{
  if (!__window)
    {
      return;
    }

  widget_destroy (WIDGET (__window->window));
  free (__window);
}
