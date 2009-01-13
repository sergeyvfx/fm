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


/* Show dialog with different chown options */
int
action_chown_dialog (int *__user, int *__group, int *__rec);

END_HEADER

#endif
