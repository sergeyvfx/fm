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
tcllib_init_commands (Tcl_Interp *__interp)
{
  typedef int (*init_commands_t) (Tcl_Interp *);

  init_commands_t commands[] = {
      _tcl_ext_init_commands, _tcl_iface_init_commands,
      _tcl_bind_init_commands, _tcl_actions_init_commands,

      NULL /* Terminate NULL */
  };

  init_commands_t *p = commands;

  for (; *p != NULL; ++p)
    {
      if ((*p) (__interp) != TCL_OK)
        {
          return TCL_ERROR;
        }
    }

  return TCL_OK;
}
