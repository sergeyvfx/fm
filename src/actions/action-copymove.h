/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _action_copymove_h_
#define _action_copymove_h_

#include "smartinclude.h"

BEGIN_HEADER

#include "file_panel.h"

/* Copy/move list of files from specified panel */
int
action_copymove (file_panel_t *__panel, BOOL __move);

END_HEADER

#endif
