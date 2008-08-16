/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _widget_progress_h_
#define _widget_progress_h_

#ifndef _widget_h_
#  error Do not include this file directly. Include widget.h instead.
#endif

/********
 * Constants
 */

/* Do not show percent */
#define WPBS_NOPERCENT 0x0001

#define WFPB_UPDATEONLY 0x0100

/********
 * Type defenitions
 */

typedef struct
{
  /* Inherit from widget */
  WIDGET_MEMBERS

  /* Maximal position */
  unsigned long max_pos;

  /* Current position */
  unsigned long cur_pos;

  /* Different additional styles */
  unsigned int style;

  scr_font_t *font;
  scr_font_t *background_font;
  scr_font_t *progress_font;

  /* Deep-core data */

  /* Previously drawed count of filled chars */
  int prev_count;
} w_progress_t;

/********
 *
 */

w_progress_t*
widget_create_progress (w_container_t *__parent, unsigned long __max_pos,
                        int __x, int __y, int __w, unsigned int __style);

void
w_progress_set_font (w_progress_t *__progress,
                     scr_font_t *__font,
                     scr_font_t *__background_font,
                     scr_font_t *__progress_font);

void
w_progress_set_pos (w_progress_t *__progress, unsigned long __pos);

void
w_progress_set_max (w_progress_t *__progress, unsigned long __max);

unsigned long
w_progress_get_pos (w_progress_t *__progress);

unsigned long
w_progress_get_max (w_progress_t *__progress);


#endif
