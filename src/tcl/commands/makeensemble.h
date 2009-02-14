/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Create commands `ensemble' stuff implementation.
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _TCL_COMMANDS_MAKEENSEMBLE_H_
#define _TCL_COMMANDS_MAKEENSEMBLE_H_

#include <smartinclude.h>

BEGIN_HEADER

typedef struct ensemblecmd
{
  const char     *name; /* The name of the subcommand */
  Tcl_ObjCmdProc *proc; /* The implementation of the subcommand */
} ensemblecmd_t;

Tcl_Command
tcllib_make_ensemble (Tcl_Interp *interp,
                      const char *cmdname,
                      const ensemblecmd_t *subcommand);

#define TCL_DEFSYM_ENS_BEGIN(ensemble_name) \
  const ensemblecmd_t _##ensemble_name##_ensemble_cmds[] = {

#define TCL_DEFSYM_ENS_END \
    {NULL, NULL}\
  };

#define TCL_MAKE_ENSEMBLE(ensemble_name, interpreter) \
  tcllib_make_ensemble (interpreter, #ensemble_name,\
                        _##ensemble_name##_ensemble_cmds);

END_HEADER

#endif // _TCL_COMMANDS_MAKEENSEMBLE_H_
