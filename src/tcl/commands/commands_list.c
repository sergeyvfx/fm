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

#include <tcl.h>
#include "commands_list.h"

/**
 * Initialize all Tcl's commands
 *
 * @param __interp - pointer on TCL interpreter
 * @return TCL_OK if successeful, TCL_ERROR otherwise
 */
int
tcl_init_commands (Tcl_Interp *__interp)
{
  if (_tcl_ext_init_commands (__interp) != TCL_OK)
    {
      /* TODO: added error message */
      return TCL_ERROR;
    }

  if (_tcl_iface_init_commands (__interp) != TCL_OK)
    {
      /* TODO: added error message */
      return TCL_ERROR;
    }

  return TCL_OK;
}
