/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Prototypes of file actions
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _actions_h_
#define _actions_h_

#include "smartinclude.h"

BEGIN_HEADER

#include "file_panel.h"
#include "deque.h"
#include <vfs/vfs.h>

/********
 * Constants
 */

/* Error codes */
#define ACTION_OK     0
#define ACTION_ERR   -1
#define ACTION_ABORT  1

/********
 *
 */

#include <action-listing.h>

/********
 *
 */

/* Copy list of files from specified panel */
int
action_copy (file_panel_t *__panel);

/* Move/rename list of files from specified panel */
int
action_move (file_panel_t *__panel);

/* Create directory on specified panel */
int
action_mkdir (file_panel_t *__panel);

/* Chooses file panel for action */
file_panel_t*
action_choose_file_panel (const wchar_t *__caption,
                          const wchar_t *__short_msg);

END_HEADER

#endif
