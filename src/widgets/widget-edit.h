/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
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
 * Constants
 */

#define W_EDIT_CHECKVALIDNESS_PROP 0x0001

/********
 * Type definitions
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

  /* Font of shaded style */
  scr_font_t *shaded_font;

  /* Font of auto-guessing suffix */
  scr_font_t *suffix_font;

  /* Is edit in shaded state? */
  BOOL shaded;

  /* Variants for auto-guessing */
  struct
  {
    wchar_t **strings;
    unsigned long count;
  } variants;

  /* Text suffix from auto-guessing stuff */
  wchar_t *suffix;

} w_edit_t;

/********
 *
 */

/* Create new edit box */
w_edit_t*
widget_create_edit (w_container_t *__parent, int __x, int __y, int __width);

/* Get text of edit box */
void
w_edit_set_text (w_edit_t* __exit, const wchar_t *__text);

/* Get text from edit box */
wchar_t*
w_edit_get_text (w_edit_t* __edit);

/* Set fonts used in edit */
void
w_edit_set_fonts (w_edit_t *__edit, scr_font_t *__font,
                  scr_font_t *__shaded_font, scr_font_t *__suffix_font);

/* Set shaded state of specified edit widget */
void
w_edit_set_shaded (w_edit_t *__edit, BOOL __shaded);

/* Set variants list for auto-guessing stuff */
void
w_edit_set_variants (w_edit_t *__edit, wchar_t **__strings,
                     unsigned long __count);

/* Add variants to list for auto-guessing stuff */
void
w_edit_add_variant (w_edit_t *__edit, wchar_t *__string);

#endif
