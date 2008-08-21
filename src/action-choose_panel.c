/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Choose file panel action
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "actions.h"
#include "deque.h"
#include "i18n.h"

#include <widget.h>

/**
 * Fill list with file panels
 *
 * @param __list - list which will be filled
 */
static void
fill_list (w_list_t *__list)
{
  int i;
  deque_t *panels;
  wchar_t *text, *cwd;
  file_panel_t *panel;
  w_list_item_t *item;
  size_t len;

  if (!__list)
    {
      return;
    }

  i = 0;
  panels = file_panel_get_list ();

  deque_foreach (panels, panel);
    /* Prepare text of list item */
    cwd = file_panel_get_full_cwd (panel);
    len = wcslen (cwd) + 64;
    text = malloc ((len + 1) * sizeof (wchar_t));
    swprintf (text, len, L"%d. %ls", i + 1, cwd);

    /* Append item */
    item = w_list_append_item (__list, text, 0);
    item->data = panel;

    free (text);
    free (cwd);
    i++;
  deque_foreach_done
}

/**
 * Show window with list of panels
 *
 * @param __caption - caption of a window
 * @param __short_msg - short message which will be used as list's caption
 * @return selected file panel
 */
static file_panel_t*
show_list (const wchar_t *__caption, const wchar_t *__short_msg)
{
  int modal_result, w_ok, w_cancel, button_left;
  w_window_t *wnd;
  w_list_t *list;
  file_panel_t *res;
  wchar_t *cpt_ok, *cpt_cancel;
  w_button_t *btn;

  /* Create window */
  wnd = widget_create_window (__caption, 0, 0,
                              SCREEN_WIDTH * 0.7, SCREEN_HEIGHT * 0.8,
                              WMS_CENTERED);
  list = widget_create_list (WIDGET_CONTAINER (wnd), __short_msg, 1, 1,
                             wnd->position.width - 2,
                             wnd->position.height - 3);

  /* Create buttons */
  cpt_ok = _(L"_Ok");
  w_ok = widget_shortcut_length (cpt_ok) + 6;
  cpt_cancel = _(L"_Cancel");
  w_cancel = widget_shortcut_length (cpt_cancel) + 4;

  button_left = (wnd->position.width - (w_ok  + w_cancel + 2)) / 2;

  btn = widget_create_button (WIDGET_CONTAINER (wnd), cpt_ok,
                              button_left, wnd->position.height - 2,
                              WBS_DEFAULT);
  btn->modal_result = MR_OK;
  button_left += w_ok + 1;

  btn = widget_create_button (WIDGET_CONTAINER (wnd), cpt_cancel,
                              button_left, wnd->position.height - 2, 0);
  btn->modal_result = MR_CANCEL;

  /* Fill list with items */
  fill_list (list);

  /* Display iwndow */
  modal_result = w_window_show_modal (wnd);

  if (modal_result == MR_CANCEL)
    {
      /* User canceled action */
      res = NULL;
    }
  else
    {
      /* Get panel from item's data */
      w_list_item_t *item;
      item = w_list_get_current_item (list);
      if (item)
        {
          res = item->data;
        }
      else
        {
          res = NULL;
        }
    }

  /* Destoy all user variables */
  widget_destroy (WIDGET (wnd));

  return res;
}

/********
 * User's backend
 */

/**
 * Chooses file panel for action
 *
 * @param __caption - caption of a window with file panels list
 * @param __short_msg - short message which will be used as list's caption
 * @return
 *  - If there is only one panel or user canceled operation, NULL returnef
 *  - If there is only two file panels, opposite to curretn will be returned
 *  - If there is more than two file panels, user'll see a list of
 */
file_panel_t*
action_choose_file_panel (const wchar_t *__caption,
                          const wchar_t *__short_msg)
{
  int count = file_panel_get_count ();
  deque_t *panels = file_panel_get_list ();

  if (count <= 1)
    {
      return NULL;
    }

  if (count == 2)
    {
      /* Return oppsite file panel */
      if (deque_data (deque_head (panels)) != current_panel)
        {
          return deque_data (deque_head (panels));
        }
      else
        {
          return deque_data (deque_tail (panels));
        }
    }
  else
    {
      return show_list (__caption, __short_msg);
    }

  return NULL;
}
