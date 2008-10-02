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
 * Constants
 */

/* Unknown check state */
#define WCB_STATE_UNDEFINED (-1)

/* Allow checkbox to has undefined state */
#define WCBS_WITH_UNDEFINED 0x00001

/********
 * Type definitions
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

  unsigned int style;
} w_checkbox_t;

/********
 *
 */

/* Create new checkbox */
w_checkbox_t*
widget_create_checkbox (w_container_t *__parent, const wchar_t *__caption,
                        int __x, int __y, BOOL __check, unsigned int __style);

/* Get check state from the checkbox */
BOOL
w_checkbox_get (const w_checkbox_t *__checkbox);

/* Set check state from the checkbox */
void
w_checkbox_set (w_checkbox_t *__checkbox, BOOL __state);

#endif
