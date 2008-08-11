/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _widget_edit_h_
#define _widget_edit_h_

#ifndef _widget_h_
#  error Do not include this file directly. Include widget.h instead.
#endif

/********
 * Type defenitions
 */

typedef struct
{
  /* Inherit from widget */
  WIDGET_MEMBERS

  struct
  {
    /* Caption in edit box */
    wchar_t *data;

    /* Allocated buffer size */
    size_t allocated;
  } text;

  /* How much characters where scrolled */
  /* due to widget's width is limited */
  size_t scrolled;

  /* Position of caret */
  size_t caret_pos;

  /* Font for normal style */
  scr_font_t *font;
} w_edit_t;

/********
 *
 */

w_edit_t*
widget_create_edit (w_container_t *__parent, int __x, int __y, int __width);

void
w_edit_set_text (w_edit_t* __exit, wchar_t *__text);

wchar_t*
w_edit_get_text (w_edit_t* __edit);

void
w_edit_set_fonts (w_edit_t *__edit, scr_font_t *__font);

#endif
