/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of widget list
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _widget_list_h_
#define _widget_list_h_

#ifndef _widget_h_
#  error Do not include this file directly. Include widget.h instead.
#endif

/********
 * Constants
 */

#define W_LIST_ITEMINDEX_PROP 0x0001

/********
 * Type defenitions
 */

typedef struct
{
  wchar_t *text;
  long     tag;
  void    *data;
} w_list_item_t;

typedef struct
{
  /* Inherit from widget */
  WIDGET_MEMBERS

  wchar_t *caption;

  struct
  {
    __u32_t count;
    __u32_t current;
    w_list_item_t *data;
  } items;

  __u32_t scroll_top;

  widget_t *scrollbar;

  /* Fonts */
  scr_font_t *font;
  scr_font_t *cursor_font, *cursor_unfocused_font;
  scr_font_t *caption_font;
} w_list_t;

/********
 *
 */

/* Create new list widget */
w_list_t*
widget_create_list (const wchar_t *__name, w_container_t *__parent,
                    const wchar_t *__caption,
                    int __x, int __y, int __w, int __h);

/* Insert item to list */
w_list_item_t*
w_list_insert_item (w_list_t *__list, __u32_t __pos,
                    wchar_t *__text, __u32_t __tag);

/* Append item to list */
w_list_item_t*
w_list_append_item (w_list_t *__list, wchar_t *__text, __u32_t __tag);

/* Set fonts used in list */
void
w_list_set_fonts (w_list_t *__list, scr_font_t *__font,
                  scr_font_t *__cursor_font,
                  scr_font_t *__cursor_unfocused_font,
                  scr_font_t *__caption_font);

/* Get current item */
w_list_item_t*
w_list_get_current_item (w_list_t *__list);

/* Get count of items */
__u32_t
w_list_items_count (w_list_t *__list);

/* Get item with specified index */
w_list_item_t*
w_list_get_item (w_list_t *__list, __u32_t __index);

/* Set currently selected item */
int
w_list_set_selected (w_list_t *__list, __u32_t __index);

#endif
