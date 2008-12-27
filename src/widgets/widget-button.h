/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _widget_button_h_
#define _widget_button_h_

#ifndef _widget_h_
#  error Do not include this file directly. Include widget.h instead.
#endif

/********
 * Constants
 */

/* Button styles */
#define WBS_NONE    0x0000
#define WBS_DEFAULT 0x0001

/********
 * Type defenitions
 */

typedef struct
{
  /* Inherit from widget */
  WIDGET_MEMBERS

  /* Caption in button */
  wchar_t *caption;

  /* Font for normal style */
  scr_font_t *font;

  /* Font for hotkey in normal state */
  scr_font_t *focused_font;

  /* Font for shortcut style */
  scr_font_t *hot_font;

  /* Font for hotkey in normal state */
  scr_font_t *hot_focused_font;

  unsigned int style;

  /* Modal result code for window */
  unsigned short modal_result;
} w_button_t;

/********
 *
 */

w_button_t*
widget_create_button (const wchar_t *name, w_container_t *__parent,
                      const wchar_t *__caption,
                      int __x, int __y, unsigned int __style);

void
w_button_set_fonts (w_button_t *__button,
                    scr_font_t *__font,
                    scr_font_t *__focused_font,
                    scr_font_t *__hot_font,
                    scr_font_t *__hot_focused_font);

#endif
