/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of iterface part of action 'delete'
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _action_delete_iface_h_
#define _action_delete_iface_h_

#include "smartinclude.h"

BEGIN_HEADER

#include <widgets/widget.h>
#include "actions.h"

typedef struct
{
  w_window_t *window;
  w_progress_t *progress;
  w_text_t *text;

  BOOL abort;
} delete_process_window_t;

/* Create deletion progress window */
delete_process_window_t*
action_delete_create_proc_wnd (BOOL __total_progress,
                               const action_listing_t *__listing);

/* Destroy deletion progress window */
void
action_delete_destroy_proc_wnd (delete_process_window_t *__window);

END_HEADER

#endif
