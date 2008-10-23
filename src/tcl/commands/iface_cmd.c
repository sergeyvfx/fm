/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Interface tcl commands implementation.
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include <tcl.h>
#include <stdlib.h>

#include <widgets/widget.h>
#include <macrodef.h>
#include <messages.h>
#include <util.h>
#include <i18n.h>
#include <iface.h>

#include "commands_list.h"

/**
 * This function implements the "message_box" Tcl command
 * See the ${project-name} user documentation for details on what it does
 */
TCL_DEFUN(_tcl_message_box_cmd)
{
  wchar_t *message = L"", *title = L"";
  char    *result;
  int i=0, cindex, tindex, flags = MB_OK;

  static const char *options[] = {
    "-message",
    "-title",
    "-type",
    "-default",
    "-critical",
    NULL
  };

  static const char *typesString[] = {
    "ok",           "okcancel",
    "yesno",        "yesnocancel",
    "retrycancel",  "retryskipcancel",
    NULL
  };

  static const unsigned int types[] = {
    MB_OK,          MB_OKCANCEL,
    MB_YESNO,       MB_YESNOCANCEL,
    MB_RETRYCANCEL, MB_RETRYSKIPCANCEL
  };

  static const char *retvals[] = {
    "none", "ok", "yes", "cancel", "no",
    "abort", "retry", "ignore", "skip"
  };

  while (++i < objc)
    {
      /* Detecting command options */
      if (Tcl_GetIndexFromObj (interp, objv[i],
                               options, "option", 0, &cindex) != TCL_OK)
        {
          return TCL_ERROR;
        }

      /* Proccesing options value */
      switch (cindex) {
        case 0: /* -message */
          if (++i >= objc) goto wrongarg;
          MBS2WCS(message, Tcl_GetString (objv[i]));
          break;
        case 1: /* -title */
          if (++i >= objc) goto wrongarg;
          MBS2WCS(title, Tcl_GetString (objv[i]));
          break;
        case 2: /* -type */
          if (++i >= objc) goto wrongarg;
          if (Tcl_GetIndexFromObj (interp, objv[i], typesString,
                                   "-type value", 0, &tindex) != TCL_OK)
            {
              return TCL_ERROR;
            }

          flags = types[ tindex ];
          break;
        case 3: /* -default */
          if (++i >= objc) goto wrongarg;
          break;
        case 4: /* -critical */
          flags |= MB_CRITICAL;
          break;
      }
    }

  result = retvals[message_box (title, message, flags)];
  Tcl_SetObjResult (interp, Tcl_NewStringObj (result, -1));

  return TCL_OK;

wrongarg:
  Tcl_WrongNumArgs (interp, 1, objv, "?option value ...?");
  return TCL_ERROR;
}

/**
 * This function implements the "exit" Tcl command
 * See the ${project-name} user documentation for details on what it does
 */
TCL_DEFUN(_tcl_program_exit_cmd)
{
  int exitcode = 0;

  if (objc > 2)
    {
      Tcl_WrongNumArgs (interp, 1, objv, "?exitcode?");
      return TCL_ERROR;
    }
  else if (objc == 2)
    {
      if (Tcl_GetIntFromObj(interp, objv[1], &exitcode) != TCL_OK)
        {
          return TCL_ERROR;
        }
    }

  do_exit ();

  return TCL_OK;
}

/**
 * Initialize Tcl commands from ::iface and ::core namespaces
 *
 * @param __interp - a pointer on Tcl interpreter
 * @return TCL_OK if successeful, TCL_ERROR otherwise
 */
int
_tcl_iface_init_commands (Tcl_Interp *__interp)
{
  TCL_DEFSYM_BEGIN
    TCL_DEFSYM("::iface::message_box", _tcl_message_box_cmd),
    TCL_DEFSYM("::core::exit", _tcl_program_exit_cmd),
  TCL_DEFSYM_END

  Tcl_Namespace *ns_iface = NULL, *ns_core = NULL;
  ns_iface    = Tcl_CreateNamespace (__interp, "::iface", NULL, NULL);
  ns_core     = Tcl_CreateNamespace (__interp, "::core", NULL, NULL);

  if (ns_iface == NULL)
    {
      return TCL_ERROR;
    }

  if (ns_core == NULL)
    {
      return TCL_ERROR;
    }

  TCL_DEFCREATE(__interp);

  return TCL_OK;
}
