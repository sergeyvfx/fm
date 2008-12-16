/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Interface part of find operation
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _ACTION_FIND_IFACE_H_
#define _ACTION_FIND_IFACE_H_

#include <smartinclude.h>

BEGIN_HEADER

#include "action-find.h"

#include <widgets/widget.h>

/********
 * Constants
 */

#define AF_FIND_AGAIN (MR_CUSTOM + 1)
#define AF_GOTO       (MR_CUSTOM + 2)

/********
 * Type definitions
 */

typedef struct
{
  w_window_t *window;

  w_list_t *list;

  w_text_t *status;

  BOOL dir_opened;

  long found_files, found_dirs;
} action_find_res_wnd_t;

/********
 *
 */

/* Show dialog with different find options */
int
action_find_show_dialog (action_find_options_t *__options);

/* Create window for find results */
action_find_res_wnd_t*
action_find_create_res_wnd (void);

/* Destroy window for find results */
void
action_find_destroy_res_wnd (action_find_res_wnd_t *__wnd);

END_HEADER

#endif
