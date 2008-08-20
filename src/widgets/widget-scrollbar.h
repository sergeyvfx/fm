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

#ifndef _widget_scrollbar_h_
#define _widget_scrollbar_h_

#ifndef _widget_h_
#  error Do not include this file directly. Include widget.h instead.
#endif

/********
 * Constants
 */

/* Scrollbar styles */
#define WSBS_VERTICAL       0x0000
#define WSBS_HORISONTAL     0x0001

/********
 * Type defenitions
 */

typedef struct
{
  /* Inherit from simple widget */
  WIDGET_MEMBERS

  unsigned long pos;
  unsigned long size;

  scr_font_t *font;
  scr_font_t *button_font;

  /* Style of box */
  unsigned int style;
} w_scrollbar_t;

/********
 *
 */

/* Create new scrollbar widget */
w_scrollbar_t*
widget_create_scrollbar (w_container_t *__parent, unsigned long __size,
                         int __x, int __y, int __length, unsigned int __style);

/* Set position of scrollbar */
void
w_scrollbar_set_pos (w_scrollbar_t *__scrollbar, unsigned long __pos);

/* Set size of scrollbar */
void
w_scrollbar_set_szie (w_scrollbar_t *__scrollbar, unsigned long __size);

/* Get position of scrollbar */
unsigned long
w_scrollbar_get_pos (w_scrollbar_t *__scrollbar);

/* Get size of scrollbar */
unsigned long
w_scrollbar_get_szie (w_scrollbar_t *__scrollbar);

/* Set fonts used in scrollbar */
void
w_scrollbar_set_fonts (w_scrollbar_t *__scrollbar, scr_font_t *__font,
                       scr_font_t *__button_font);
#endif
