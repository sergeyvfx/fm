/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of action 'Change mode'
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

/* Show dialog with different chmod options */
int
action_chmod_show_dialog (unsigned int *__mask, unsigned int *__unknown_mask,
                          BOOL *__rec);

END_HEADER

#endif
