/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Tcl embed library general stuff.
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef TCL_TCLLIB_H_
#define TCL_TCLLIB_H_

#define TCL_OK 0
#define TCL_ERROR 1

#include "ext.h"

BEGIN_HEADER

int
tcllib_init (void);

int
tcllib_load_file (const wchar_t *__filename);

END_HEADER

#endif // TCL_TCLLIB_H_
