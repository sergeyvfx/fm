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

int
action_copy                       (file_panel_t *__panel);

file_panel_t*
action_choose_file_panel          (void);

END_HEADER

#endif
