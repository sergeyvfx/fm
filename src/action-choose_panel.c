/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Choose file panel action
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
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
    if (panel != current_panel)
      {
        /* Do not include current panel to list */

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
      }
    i++;
  deque_foreach_done
}

/**
 * Get window dimenstions
 *
 * @param __caption - caption of a window
 * @param __short_msg - short message which will be used as list's caption
 * @return dimenstions of a window
 */
static widget_position_t
window_dimensions (const wchar_t *__caption, const wchar_t *__short_msg)
{
  deque_t *panels;
  wchar_t *cpt_ok, *cpt_cancel, *cwd;
  int w_ok, w_cancel;
  file_panel_t *panel;

  widget_position_t res = {0, 0, 0, 0, 0};

  cpt_ok = _(L"_Ok");
  w_ok = widget_shortcut_length (cpt_ok) + 6;
  cpt_cancel = _(L"_Cancel");
  w_cancel = widget_shortcut_length (cpt_cancel) + 4;

  res.width = 20;
  if (__caption)
    {
      res.width = MAX (res.width, wcswidth (__caption,
                                            wcslen (__caption)) + 4);
    }

  if (__short_msg)
    {
      res.width = MAX (res.width, wcswidth (__short_msg,
                                            wcslen (__short_msg)) + 8);
    }

  res.width = MAX (res.width, w_ok + w_cancel + 10);

  res.height = 7;
  res.height = MAX (res.height, file_panel_get_count () + 4);

  panels = file_panel_get_list ();

  deque_foreach (panels, panel);
    cwd = file_panel_get_full_cwd (panel);
    res.width = MAX (res.width, wcswidth (cwd, wcslen (cwd)) + 7);
    free (cwd);
  deque_foreach_done

  /* Limit dimensions of window */
  res.width = MIN (res.width, SCREEN_WIDTH * 0.7);
  res.height = MIN (res.height, SCREEN_HEIGHT * 0.8);

  return res;
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
  int modal_result;
  w_window_t *wnd;
  w_list_t *list;
  file_panel_t *res;
  widget_position_t pos;

  /* Create window */
  pos = window_dimensions (__caption, __short_msg);

  wnd = widget_create_window (__caption, 0, 0, pos.width, pos.height,
                              WMS_CENTERED);
  list = widget_create_list (NULL, WIDGET_CONTAINER (wnd), __short_msg, 1, 1,
                             wnd->position.width - 2,
                             wnd->position.height - 3);

  /* Create buttons */
  action_create_ok_cancel_btns (wnd);

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
