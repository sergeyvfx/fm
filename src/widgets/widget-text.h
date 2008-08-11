/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _widget_text_h_
#define _widget_text_h_

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

  /* Text */
  wchar_t *text;

  /* Font of text */
  scr_font_t *font;
} w_text_t;

/********
 *
 */

w_text_t*
widget_create_text (w_container_t *__parent, const wchar_t *__text,
                    int __x, int __y);

void
w_text_set_font (w_text_t *__text, scr_font_t *__font);

void
w_text_set (w_text_t *__w_text, const wchar_t *__text);

#endif
