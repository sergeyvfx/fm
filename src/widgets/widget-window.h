/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _widget_window_h_
#define _widget_window_h_

/********
 * Constants
 */

/* Window show modes */
#define WSM_DEFAULT 0x00
#define WSM_MODAL   0x01

/* Window styles */
#define WMS_CENTERED 0x01

/* Modal results */
#define MR_NONE   0x0000
#define MR_OK     0x0001
#define MR_YES    0x0002
#define MR_CANCEL 0x0003
#define MR_NO     0x0004
#define MR_ABORT  0x0005
#define MR_RETRY  0x0006
#define MR_IGNORE 0x0007
#define MR_SKIP   0x0008

#define MR_CUSTOM 0x0100

/********
 * Type defenitions
 */

typedef struct
{
  /* Inherit from widget container */
  WIDGET_CONTAINER_MEMBERS

  /* Panel of layout to manipulate with visibility */
  panel_t panel;

  /* Font for text on window */
  scr_font_t *font;

  struct
  {
    /* Text of caption */
    wchar_t *text;

    /* Font of caption */
    scr_font_t *font;
  } caption;

  unsigned int style;

  /* Some deep-core info */
  unsigned short show_mode;
  int modal_result;
} w_window_t;

/********
 *
 */

/* Deep-core stuff */
void
w_window_end_modal (w_window_t *__window, int __modal_result);

/* Creates new window */
w_window_t*
widget_create_window (const wchar_t *__caption, int __x, int __y,
                      int __w, int __h, unsigned int __style);

/* Show window */
void
w_window_show (w_window_t *__window);

/* Show modal window */
int
w_window_show_modal (w_window_t *__window);

/* Hide window */
void
w_window_hide (w_window_t *__window);

void
w_window_set_fonts (w_window_t *__window, scr_font_t *__font,
                    scr_font_t *__caption_font);

#endif
