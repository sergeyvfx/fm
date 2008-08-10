/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _iface_h_
#define _iface_h_

#include "smartinclude.h"

BEGIN_HEADER

/* Initialize interface */
int
iface_init (void);

/* Uninitialize interface */
void
iface_done (void);

int
iface_mainloop (void);

/***
 * Menu
 */

int
iface_create_menu (void);

/***
 * Actions
 */

void
iface_act_exit (void);

END_HEADER

#endif
