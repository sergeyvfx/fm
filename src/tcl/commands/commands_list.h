/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Tcl commands general stuff implementation.
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _TCL_COMMANDS_COMMANDS_LIST_H_
#define _TCL_COMMANDS_COMMANDS_LIST_H_

#include <smartinclude.h>

BEGIN_HEADER

#define TCL_DEFUN(Fname) \
  static int \
  Fname(ClientData clientData, \
        Tcl_Interp *interp, \
        int objc, \
        Tcl_Obj *const objv[])

#define TCL_DEFSYM_BEGIN \
  struct {\
    const char *name;\
    Tcl_ObjCmdProc *proc;\
  } *__iter_cmd, __cmd_declare_[] = {

#define TCL_DEFSYM(Sname, Fname) \
        {Sname, Fname}

#define TCL_DEFSYM_END \
    {NULL, NULL}\
  };

#define TCL_DEFCREATE(interpreter) \
  { \
    __iter_cmd = __cmd_declare_; \
    while(__iter_cmd->name != NULL) \
      {\
        Tcl_CreateObjCommand(interpreter, \
                             __iter_cmd->name, \
                             __iter_cmd->proc, \
                             NULL, NULL); \
        ++__iter_cmd; \
      } \
    __iter_cmd = NULL; \
  }

int
tcllib_init_commands (Tcl_Interp *);

/* Private function */

int
_tcl_ext_init_commands (Tcl_Interp *);

int
_tcl_iface_init_commands (Tcl_Interp *);

int
_tcl_bind_init_commands (Tcl_Interp *);

int
_tcl_actions_init_commands (Tcl_Interp *);

END_HEADER

#endif // _TCL_COMMANDS_COMMANDS_LIST_H_
