/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation file for widget `text`
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "widget.h"

/**
 * Destroy a text widget
 *
 * @param __text - text to be destroyed
 * @return zero on success, non-zero on failure
 */
static int
text_destructor                   (w_text_t *__text)
{
  if (!__text)
    {
      return -1;
    }

  SAFE_FREE (__text->text);

  free (__text);
  return 0;
}

/**
 * Draw a text
 *
 * @param __text - text to be drawn
 * @return zero on success, non-zero on failure
 */
static int
text_drawer                       (w_text_t *__text)
{
  scr_window_t layout=WIDGET_LAYOUT (__text);

  /* Widget is invisible or there is no layout */
  if (!WIDGET_VISIBLE (__text) || !layout)
    {
      return -1;
    }

  scr_wnd_attr_backup (layout);

  scr_wnd_move_caret (layout, __text->position.x, __text->position.y);

  /* Draw text */
  if (__text->text)
    {
      scr_wnd_font (layout, *__text->font);
      scr_wnd_printf (layout, "%ls", __text->text);
    }

  scr_wnd_attr_restore (layout);

  return 0;
}

/**
 * Clear drawed text
 *
 * @param __text - for which text widget make this operation
 */
static void
text_clear                        (w_text_t *__text)
{
  scr_window_t layout=WIDGET_LAYOUT (__text);

  /* Widget is invisible or there is no layout */
  if (!WIDGET_VISIBLE (__text) || !layout)
    {
      return;
    }

  scr_wnd_attr_backup (layout);

  scr_wnd_move_caret (layout, __text->position.x, __text->position.y);

  /* Draw text */
  if (__text->text)
    {
      size_t i, len=wcslen (__text->text);
      scr_wnd_font (layout, *__text->font);
      for (i=0; i<len; i++)
        scr_wnd_putch (layout, ' ');
    }

  scr_wnd_attr_restore (layout);
}

/********
 * User's backend
 */

/**
 * Create new text
 *
 * @param __parent - parent of text. Should be CONTAINER
 * @param __text - text
 * @param __x, __y - coordinates of text
 * @return a pointer to text object
 */
w_text_t*
widget_create_text                (w_container_t *__parent,
                                   const wchar_t *__text,
                                   int __x, int __y)
{
  w_text_t *res;

  /* There is no parent or text is null, so we can't create text */
  if (!__parent || !__text)
    return 0;

  unsigned int w;

  w=wcslen (__text);

  WIDGET_INIT (res, w_text_t, WT_TEXT, __parent, WF_NOLAYOUT | WF_UNFOCUSABE,
               text_destructor, text_drawer, __x, __y, 1, w, 1);

  res->text = wcsdup (__text);

  res->font = &FONT (CID_BLACK, CID_WHITE);

  WIDGET_POST_INIT (res);

  return res;
}

/**
 * Set font used in text
 *
 * @param __text - for which text widget fonts are to be set
 * @param __font - font of text
 */
void
w_text_set_font                   (w_text_t *__text, scr_font_t *__font)
{
  if (!__text)
    {
      return;
    }

  WIDGET_SAFE_SET_FONT (__text, font, __font);

  widget_redraw (WIDGET (__text));
}

/**
 * Set new text to widget
 *
 * @param __w_text - for which widget text will be set
 * @param __text - new text of widget
 */
void
w_text_set                        (w_text_t *__w_text, const wchar_t *__text)
{
  int w;

  if (!__w_text || !__text)
    {
      return;
    }

  text_clear (__w_text);

  SAFE_FREE (__w_text->text);
  __w_text->text=wcsdup (__text);

  /* Resize widget */
  w=wcslen (__text);
  widget_resize (WIDGET (__w_text),
    __w_text->position.x, __w_text->position.y,
    w, 1);

  widget_redraw (WIDGET (__w_text));
}
