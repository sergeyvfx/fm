/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * File associations tcl commands implementation.
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include <tcl.h>
#include <tcl/ext.h>
#include <util.h>

#include <messages.h>

#include "commands_list.h"
#include "makeensemble.h"

/**
 * This function implements the "ext create" Tcl command
 * See the ${project-name} user documentation for details on what it does
 */
TCL_DEFUN(_tcl_create_ext_cmd)
{
  Tcl_Obj *faobject;

  faobject = tcl_fa_new_object();

  Tcl_SetObjResult (interp, faobject);
  return TCL_OK;
}

/**
 * This function implements the "ext set" Tcl command
 * See the ${project-name} user documentation for details on what it does
 */
TCL_DEFUN(_tcl_set_ext_cmd)
{
  int     index, i = 1, allocated = 0;
  char    *pattern;
  wchar_t *viewer = NULL, *editor = NULL;
  Tcl_Obj *faobject, *result;

  static const char *options[] = {
      "-view", "-edit", NULL
  };

  if (objc < 3)
    {
wrongargs:
      Tcl_WrongNumArgs(interp, 1, objv,
                       "varName ?-view viewer? ?-edit editor? pattern");
      return TCL_ERROR;
    }

  /* Gets variable from interpreter */
  faobject = Tcl_ObjGetVar2(interp, objv[1], NULL, 0);

  if (faobject == NULL)
    {
      allocated = 1;
      faobject = tcl_fa_new_object();
    }
  else if (Tcl_IsShared(faobject))
    {
      allocated = 1;
      faobject = Tcl_DuplicateObj(faobject);
    }

  while (++i < objc - 1)
    {
      if (Tcl_GetIndexFromObj (interp, objv[i],
                               options, "option", 0, &index) != TCL_OK)
        {
          if (allocated)
            {
              Tcl_DecrRefCount(faobject);
            }
          return TCL_ERROR;
        }

      if (++i >= objc -1)
        {
          goto wrongargs;
        }

      switch (index)
        {
        case 0:
          mbs2wcs (&viewer, Tcl_GetString(objv[i]));
          break;
        case 1:
          mbs2wcs (&editor, Tcl_GetString(objv[i]));
          break;
        }
    }

  pattern = Tcl_GetString(objv[i]);
  tcl_fa_put_object(interp, faobject,
      tcl_fa_new_extobject(pattern, viewer, editor));

/* INFO: can be optimized */
  result = Tcl_ObjSetVar2 (interp, objv[1], NULL, faobject, TCL_LEAVE_ERR_MSG);
  if (result == NULL)
    {
      return TCL_ERROR;
    }

  Tcl_SetObjResult (interp, result);
  return TCL_OK;
}

/**
 * Initialize Tcl commands from ext's families
 *
 * @param __interp - a pointer on Tcl interpreter
 * @return TCL_OK if successeful, TCL_ERROR otherwise
 */
int
_tcl_ext_init_commands (Tcl_Interp *__interp)
{
  const ensemblecmd_t subcommands[] = {
      {"create", _tcl_create_ext_cmd},
      {"set",    _tcl_set_ext_cmd},
      {NULL, NULL}
  };

  if (__interp == NULL)
    {
      return TCL_ERROR;
    }

  tcl_make_ensemble (__interp, "ext", subcommands);
  return TCL_OK;
}
