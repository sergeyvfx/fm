/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Interface part of action 'Change owner'
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _ACTION_CHOWN_IFACE_H_
#define _ACTION_CHOWN_IFACE_H_

#include "smartinclude.h"

BEGIN_HEADER

#include <widgets/widget.h>

typedef struct
{
  w_window_t *window;
  w_text_t *text;
  w_progress_t *progress;

  BOOL abort;
  BOOL manual_total_count;
} chown_process_window_t;

/* Show dialog with different chown options */
int
action_chown_dialog (int *__user, int *__group, int *__rec);

/* Create chown'ing process window */
chown_process_window_t*
action_chown_create_proc_wnd (BOOL __total_progress);

/* Create chown'ing process window */
void
action_chown_destroy_proc_wnd (chown_process_window_t *__window);

END_HEADER

#endif
