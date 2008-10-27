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

#include <stdlib.h>
#include <screen.h>

#include <hook.h>
#include <dynstruct.h>
#include <messages.h>
#include <util.h>
#include <iface.h>

#include <sys/wait.h>
#include <unistd.h>

#include "ext.h"
#include "tcllib.h"
#include "commands/commands_list.h"

#define TCL_RUNTIME_ERROR \
{\
  wchar_t *errmsg;\
  mbs2wcs (&errmsg, Tcl_GetStringResult(interpreter));\
  message_box (L"Tcl Runtime Error", errmsg, MB_OK|MB_CRITICAL);\
  free (errmsg);\
}

static Tcl_Interp *interpreter = NULL;

/**
 *
 * Get file association
 *
 * @param __filename - a file name
 * @param __extcmd - a pointer of structures encapsulate file action
 * @return TCL_OK if successful, TCL_ERROR otherwise
 */
int
_get_file_associations (wchar_t *__filename, extension_t **__extcmd)
{
  Tcl_Obj *value, *associate =
    Tcl_GetVar2Ex (interpreter, "association", NULL, TCL_GLOBAL_ONLY);
  char *filename;

  if (associate == NULL)
    {
      return TCL_ERROR;
    }

  wcs2mbs(&filename, __filename);

  if (tcl_fa_get_object (interpreter,
                         associate,
                         Tcl_NewStringObj (filename, -1), &value) != TCL_OK)
      {
        return TCL_ERROR;
      }

  *__extcmd = tcl_extcmd_from_object (value);
  SAFE_FREE (filename);

  return TCL_OK;
}

/**
 *
 * File associations hook
 * For details see about hook's on user documentation
 */
int
_file_associations_hook (dynstruct_t *__callData)
{
  extension_t *ext;
  wchar_t     *executecmd, *filename, *cwd;
  size_t      slength;

  if (dynstruct_get_field_val (__callData,
                               L"filename", (void *) &filename) != DYNST_OK)
    {
      message_box (L"Hook `open-file-hook' error",
                   L"variable `filename' not found", MB_OK|MB_CRITICAL);

      return HOOK_FAILURE;
    }

  filename = escape_string (filename);

  if (_get_file_associations (filename, (void *) &ext) != TCL_OK)
    {
      executecmd = filename;
    }
  else
    {
      if (dynstruct_get_field_val (__callData,
                                   L"cwd", (void *) &cwd) != DYNST_OK)
        {
          message_box (L"Hook `open-file-hook' error",
                       L"variable `cwd' not found", MB_OK|MB_CRITICAL);

          return HOOK_FAILURE;
        }

      cwd = escape_string (cwd);

      slength = wcslen (ext->viewer) + wcslen (filename) + wcslen (cwd) + 3;
      executecmd = malloc (sizeof (wchar_t) * (slength + 1));

      swprintf (executecmd, slength,
                L"%ls %ls/%ls", ext->viewer, cwd, filename);
      SAFE_FREE (filename);
      SAFE_FREE (cwd);
    }

  if (run_shell_command (executecmd) == -1)
    {
      return HOOK_FAILURE;
    }

  SAFE_FREE (executecmd);

  return HOOK_SUCCESS;
}

/**
 *
 * Load file's contents into TCL interpreter
 *
 * @param __filename - a name loads file
 * @return TCL_OK if successful, TCL_ERROR otherwise
 */
int
tcllib_load_file (const wchar_t *__filename)
{
  char *filename;

  wcs2mbs(&filename, __filename);
  if (Tcl_EvalFile (interpreter, filename) != TCL_OK)
    {
      return TCL_ERROR;
    }
  SAFE_FREE (filename);

  return TCL_OK;
}

/**
 *
 * Initialize Tcl embed library
 *
 * @return TCL_OK if successful, TCL_ERROR otherwise
 */
int
tcllib_init (void)
{
  /* NOTE: Keep order, config.tcl be loaded last */
  static wchar_t *important_files[] = { L"tcl/associations.tcl",
                                        L"tcl/config.tcl" };
  /* static wchar_t *unimportant_files[] = { L"custom.tcl" }; */
  int i;

  interpreter = Tcl_CreateInterp();
  if (interpreter == NULL)
    {
      return TCL_ERROR;
    }

  if (tcl_init_commands (interpreter) != TCL_OK)
    {
      return TCL_ERROR;
    }

  for  (i = 0; i < sizeof (important_files) / sizeof (important_files[0]); ++i)
    {
      if (tcllib_load_file (important_files[i]) != TCL_OK)
        {
          TCL_RUNTIME_ERROR;
          return TCL_ERROR;
        }
    }

  hook_register (L"open-file-hook", _file_associations_hook, 1);

  return TCL_OK;
}
