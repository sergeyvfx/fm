/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * File bind tcl command implementation.
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include <tcl.h>
#include <util.h>

#include <hotkeys.h>
#include <regexp.h>
#include "commands_list.h"

struct bind_handle_info
{
  Tcl_Interp *interpreter;
  Tcl_Obj    *script;
};

/**
 * A closure `to pass the value' to the callback
 */
int
_tcl_bind_hotkey_closure (void *__reg_data)
{
  struct bind_handle_info *bni = __reg_data;
  return Tcl_EvalObjEx (bni->interpreter, bni->script, TCL_EVAL_GLOBAL);
}

/**
 * This function implements the "bind" Tcl command
 * See the ${project-name} user documentation for details on what it does
 */
TCL_DEFUN(_tcl_bind_cmd)
{
  struct bind_handle_info *bni;
  wchar_t *wseq, *wtag;

  char *seq, *tag;
  int retcode;

  if (objc != 4)
    {
      Tcl_WrongNumArgs (interp, 1, objv, "tag sequence script");
      return TCL_ERROR;
    }

  tag = Tcl_GetString (objv[1]);
  seq = Tcl_GetString (objv[2]);

  if (preg_match ("/<[A-Z0-9\\-]*>/ig", seq) != TRUE)
    {
      Tcl_AppendResult (interp, "binding `", seq, "' is missing", NULL);
      return TCL_ERROR;
    }

  wseq = to_widestring (preg_replace ("/<([A-Z0-9\\-]*)>/ig", seq, "$1"));

  if (preg_match ("/<<[A-Z0-9\\-]*>>/ig", tag))
    {
      /* it is widget's context. stub for future */
      wtag =
        to_widestring (preg_replace ("/<<([A-Z0-9\\-]*)>>/ig", tag, "$1"));
    }
  else
    {
      wtag = to_widestring (tag);
    }

  bni = malloc (sizeof (struct bind_handle_info));
  bni->interpreter = interp;
  /**
   * NOTE: Tcl_DuplicateObj returns a new duplicate of the original
   *       object that has refCount 0.
   */
  bni->script = Tcl_DuplicateObj (objv[3]);
  Tcl_IncrRefCount (bni->script);

  retcode =
    hotkey_bind_full (wtag, wseq, _tcl_bind_hotkey_closure, (void *) bni);

  return retcode < 0 ? TCL_ERROR : TCL_OK;
}

/**
 * Initialize Tcl commands for hotkey bind
 *
 * @param __interp - a pointer on Tcl interpreter
 * @return TCL_OK if successful, TCL_ERROR otherwise
 */
int
_tcl_bind_init_commands (Tcl_Interp *__interp)
{
  TCL_DEFSYM_BEGIN
    TCL_DEFSYM("::config::bind", _tcl_bind_cmd),
  TCL_DEFSYM_END

  TCL_DEFCREATE(__interp);

  return TCL_OK;
}
