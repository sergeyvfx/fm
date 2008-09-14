/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _widget_checkbox_h_
#define _widget_checkbox_h_

#ifndef _widget_h_
#  error Do not include this file directly. Include widget.h instead.
#endif

/********
 * Type defenitions
 */

typedef struct
{
  WIDGET_MEMBERS

  scr_font_t *font;
  scr_font_t *font_focus;

  scr_font_t *hotkey_font;
  scr_font_t *hotkey_focus;

  wchar_t *caption;
  BOOL ischeck;
} w_checkbox_t;

/********
 *
 */

w_checkbox_t*
widget_create_checkbox (w_container_t *__parent, const wchar_t *__caption,
                        int __x, int __y, BOOL __check);

BOOL
w_checkbox_get (const w_checkbox_t *__checkbox);

#endif
