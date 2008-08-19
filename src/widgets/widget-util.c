/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Different utilities for widgets
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "widget.h"

/**
 * Draw border insode widget
 *
 * @param __widget - widget  in which border will be drawn
 */
void
widget_draw_border (widget_t *__widget)
{
  int i, n;
  scr_window_t layout = WIDGET_LAYOUT (__widget);

  /* Widget is invisible or there is no layout */
  if (!WIDGET_VISIBLE (__widget) || !layout)
    {
      return;
    }

  if (!WIDGET_TEST_FLAG (__widget, WF_NOLAYOUT))
    {
      /* If widget has it's own layout, we can use scr_wnd_border() */
      scr_wnd_border (layout);
      return;
    }

  /* Otherwise we should draw border ourselves */

  scr_wnd_attr_backup (layout);

  /* Upper-left cornder */
  scr_wnd_move_caret (layout, __widget->position.x, __widget->position.y);
  scr_wnd_putch (layout, ACS_ULCORNER);

  /* Upper line */
  n = __widget->position.width - 1;
  for (i = 1; i < n; ++i)
    {
      scr_wnd_putch (layout, ACS_HLINE);
    }

  /* Upper-right cornder */
  scr_wnd_putch (layout, ACS_URCORNER);

  /* Side lines */
  n = __widget->position.height - 1;
  for (i = 1; i < n; ++i)
    {
      scr_wnd_move_caret (layout, __widget->position.x,
                          __widget->position.y + i);
      scr_wnd_putch (layout, ACS_VLINE);

      scr_wnd_move_caret (layout, __widget->position.x +
              __widget->position.width - 1, __widget->position.y + i);
      scr_wnd_putch (layout, ACS_VLINE);
    }

  /* Lower-left corner */
  scr_wnd_move_caret (layout, __widget->position.x, __widget->position.y +
          __widget->position.height - 1);
  scr_wnd_putch (layout, ACS_LLCORNER);

  /* Lower line */
  n = __widget->position.width - 1;
  for (i = 1; i < n; ++i)
    {
      scr_wnd_putch (layout, ACS_HLINE);
    }

  /* Lower-right corner */
  scr_wnd_putch (layout, ACS_LRCORNER);

  scr_wnd_attr_restore (layout);
}
