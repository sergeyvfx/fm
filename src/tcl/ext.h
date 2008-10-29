/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * File association implementation
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _TCL_EXT_H_
#define _TCL_EXT_H_

#include <smartinclude.h>
#include <tcl.h>

BEGIN_HEADER

typedef struct extension_action_t
{
  wchar_t *editor;
  wchar_t *viewer;
  wchar_t *opener;

} extension_action_t;

Tcl_Obj *
tcllib_fa_new_object(void);

Tcl_Obj *
tcllib_fa_new_extobject (const char    *__pattern,
                         const wchar_t *__viewer,
                         const wchar_t *__editor,
                         const wchar_t *__opener);

int
tcllib_fa_get_object (Tcl_Interp *, Tcl_Obj *, Tcl_Obj *, Tcl_Obj **);

extension_action_t *
tcllib_extcmd_from_object (Tcl_Obj *);

int
tcllib_fa_put_object(Tcl_Interp *, Tcl_Obj *, Tcl_Obj *);

END_HEADER

#endif
