/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of widget scrollbar
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "widget.h"

/**
 * Destroy a scrollbar widget
 *
 * @param __scrollbar - scrollbar to be destroyed
 * @return zero on success, non-zero on failure
 */
static int
scrollbar_destructor (w_scrollbar_t *__scrollbar)
{
  if (!__scrollbar)
    {
      return -1;
    }

  free (__scrollbar);
  return 0;
}

/**
 * Draw a scrollbar
 *
 * @param __scrollbar - scrollbar to be drawn
 * @return zero on success, non-zero on failure
 */
static int
scrollbar_drawer (w_scrollbar_t *__scrollbar)
{
  scr_window_t layout = WIDGET_LAYOUT (__scrollbar);
  int i, n, offset;

  /* Widget is invisible or there is no layout */
  if (!WIDGET_VISIBLE (__scrollbar) || !layout)
    {
      return -1;
    }

  scr_wnd_attr_backup (layout);

  scr_wnd_move_caret (layout, __scrollbar->position.x,
                      __scrollbar->position.y);

  if (__scrollbar->style & WSBS_HORISONTAL)
    {
      scr_wnd_font (layout, *__scrollbar->button_font);
      scr_wnd_putch (layout, ACS_LARROW);

      n = __scrollbar->position.width - 1;
      scr_wnd_font (layout, *__scrollbar->font);
      for (i = 1; i < n; ++i)
        {
          scr_wnd_putch (layout, ACS_CKBOARD);
        }

      scr_wnd_font (layout, *__scrollbar->button_font);
      scr_wnd_putch (layout, ACS_RARROW);

      offset = (double)__scrollbar->pos / __scrollbar->size *
              (__scrollbar->position.width - 3);

      scr_wnd_move_caret (layout, __scrollbar->position.x + offset + 1,
                          __scrollbar->position.y);
      scr_wnd_putch (layout, ACS_BULLET);
    }
  else
    {
      scr_wnd_font (layout, *__scrollbar->button_font);
      scr_wnd_putch (layout, ACS_UARROW);

      n = __scrollbar->position.height - 1;
      scr_wnd_font (layout, *__scrollbar->font);
      for (i = 1; i < n; ++i)
        {
          scr_wnd_move_caret (layout, __scrollbar->position.x,
                              __scrollbar->position.y + i);
          scr_wnd_putch (layout, ACS_CKBOARD);
        }

      scr_wnd_font (layout, *__scrollbar->button_font);
      scr_wnd_move_caret (layout, __scrollbar->position.x,
                          __scrollbar->position.y + n);
      scr_wnd_putch (layout, ACS_DARROW);

      offset = (double)__scrollbar->pos / __scrollbar->size *
              (__scrollbar->position.height - 3);

      scr_wnd_move_caret (layout, __scrollbar->position.x,
                          __scrollbar->position.y + offset + 1);
      scr_wnd_putch (layout, ACS_BULLET);
    }

  scr_wnd_attr_restore (layout);

  return 0;
}

/********
 * User's backend
 */

/**
 * Create new scrollbar widget
 *
 * @param __parent - parent of a scrollbar
 * @param __x, __y - coordinates of scrollbar
 * @param __length - length of scrollbar
 * @param __size - logical size of scrollbar
 * @param __style - different styles
 * @return pointer to scrollbar object
 */
w_scrollbar_t*
widget_create_scrollbar (w_container_t *__parent, unsigned long __size,
                         int __x, int __y, int __length, unsigned int __style)
{
  w_scrollbar_t *res;
  int w, h;

  /* There is no parent, so we can't create progress bar */
  if (!__parent)
    {
      return 0;
    }

  if (__style & WSBS_HORISONTAL)
    {
      w = __length;
      h = 1;
    }
  else
    {
      w = 1;
      h = __length;
    }

  WIDGET_INIT (res, w_scrollbar_t, WT_SCROLLBAR, __parent,
               WF_NOLAYOUT | WF_UNFOCUSABE,
               scrollbar_destructor, scrollbar_drawer, __x, __y, 1, w, h);

  res->size = __size;
  res->style = __style;

  res->font = &FONT (CID_BLACK, CID_WHITE);
  res->button_font = &FONT (CID_WHITE, CID_BLACK);

  WIDGET_POST_INIT (res);

  return res;
}

/**
 * Set position of scrollbar
 *
 * @param __scrollbar - scrollbar to operate with
 * @param __pos - new walue of position
 */
void
w_scrollbar_set_pos (w_scrollbar_t *__scrollbar, unsigned long __pos)
{
  if (!__scrollbar)
    {
      return;
    }

  __scrollbar->pos = __pos;
  widget_redraw (WIDGET (__scrollbar));
}

/**
 * Set size of scrollbar
 *
 * @param __scrollbar - scrollbar to operate with
 * @param __size - new walue of size
 */
void
w_scrollbar_set_szie (w_scrollbar_t *__scrollbar, unsigned long __size)
{
  if (!__scrollbar)
    {
      return;
    }

  __scrollbar->size = __size;
  widget_redraw (WIDGET (__scrollbar));
}

/**
 * Get position of scrollbar
 *
 * @param __scrollbar - scrollbar to operate with
 * @return position of scrollbar
 */
unsigned long
w_scrollbar_get_pos (w_scrollbar_t *__scrollbar)
{
  if (!__scrollbar)
    {
      return 0;
    }

  return __scrollbar->pos;
}

/**
 * Get size of scrollbar
 *
 * @param __scrollbar - scrollbar to operate with
 * @return size of scrollbar
 */
unsigned long
w_scrollbar_get_szie (w_scrollbar_t *__scrollbar)
{
  if (!__scrollbar)
    {
      return 0;
    }

  return __scrollbar->size;
}

/**
 * Set fonts used in scrollbar
 *
 * @param __scrollbar - scrollbar to operate with
 * @param __font - default font (font of scrol area)
 * @param __button_font - font of buttons
 */
void
w_scrollbar_set_fonts (w_scrollbar_t *__scrollbar, scr_font_t *__font,
                       scr_font_t *__button_font)
{
  if (!__scrollbar)
    {
      return;
    }

  WIDGET_SAFE_SET_FONT (__scrollbar, font, __font);
  WIDGET_SAFE_SET_FONT (__scrollbar, button_font, __button_font);

  widget_redraw (WIDGET (__scrollbar));
}
