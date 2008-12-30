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
#include <dir.h>
#include <shared.h>

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
  SAFE_FREE (errmsg);\
}

static Tcl_Interp *interpreter = NULL;

/**
 * Prepare command for execution
 *
 * @param __format - format of command
 * @param __file_name - name of file to execute
 * @param __cwd - current working directory
 * @return pointer to a buffer with command to be executed
 * @sideeffect allocate memory for return value
 */
static wchar_t*
prepare_exec_command (const wchar_t *__format, const wchar_t *__file_name,
                      const wchar_t *__cwd)
{
  wchar_t *res, *full_name;

  full_name = wcdircatsubdir (__cwd, __file_name);

  if (wcsstr (__format, L"%n") == NULL && wcsstr (__format, L"%fn") == NULL)
    {
      /* Assume that is there is neither `%n` nor `%fn` */
      /* variable in format string, we should append full name of file */
      /* ate the end of the format string */

      size_t len = wcslen (__format) + wcslen (full_name) + 2;
      res = malloc ((len + 1) * sizeof (wchar_t));

      swprintf (res, len, L"%ls %ls", __format, full_name);
    }
  else
    {
      wchar_t *dummy;

      dummy = wcsrep ((wchar_t*)__format, L"%n", __file_name);

      res   = wcsrep (dummy, L"%fn", full_name);
      SAFE_FREE (dummy);
      dummy = res;

      res   = wcsrep (dummy, L"%cwd", __cwd);
      SAFE_FREE (dummy);
    }

  SAFE_FREE (full_name);

  return res;
}

/**
 *
 * Get file association
 *
 * @param __filename - a file name
 * @param __extcmd - a pointer of structures encapsulate file action
 * @return TCL_OK if successful, TCL_ERROR otherwise
 */
int
_get_file_associations (wchar_t *__filename, extension_action_t **__extcmd)
{
  Tcl_Obj *value, *associate =
    Tcl_GetVar2Ex (interpreter, "association", NULL, TCL_GLOBAL_ONLY);
  char *filename;

  if (associate == NULL)
    {
      return TCL_ERROR;
    }

  wcs2mbs(&filename, __filename);

  if (tcllib_fa_get_object (interpreter,
                            associate,
                            Tcl_NewStringObj (filename, -1), &value) != TCL_OK)
      {
        return TCL_ERROR;
      }

  *__extcmd = tcllib_extcmd_from_object (value);
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
  extension_action_t *ext;
  wchar_t *executecmd, *filename, *cwd;

  if (dynstruct_get_field_val (__callData, L"filename", &filename) != DYNST_OK)
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
      if (dynstruct_get_field_val (__callData, L"cwd", cwd) != DYNST_OK)
        {
          message_box (L"Hook `open-file-hook' error",
                       L"variable `cwd' not found", MB_OK|MB_CRITICAL);

          return HOOK_FAILURE;
        }

      if (!ext->opener)
        {
          wchar_t msg[1024];

          swprintf (msg, BUF_LEN (msg),
                    _(L"There is no command specified to open file \"%ls\""),
                    filename);

          MESSAGE_ERROR (msg);
          return HOOK_FAILURE;
        }

      cwd = escape_string (cwd);

      executecmd = prepare_exec_command (ext->opener, filename, cwd);

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
  static wchar_t *important_files[] = { L"associations.tcl",
                                        L"config.tcl" };
  /* static wchar_t *unimportant_files[] = { L"custom.tcl" }; */
  int i, j, count;
  wchar_t **list;

  interpreter = Tcl_CreateInterp();
  if (interpreter == NULL)
    {
      return TCL_ERROR;
    }

  if (tcllib_init_commands (interpreter) != TCL_OK)
    {
      return TCL_ERROR;
    }

  for  (i = 0; i < sizeof (important_files) / sizeof (important_files[0]); ++i)
    {
      count = get_shared_files (important_files[i], NULL, &list);

      if (count == 0)
        {
          wchar_t msg[1024];
          swprintf (msg, BUF_LEN (msg),
                    _(L"Configuration file \"%ls\" not found"),
                    important_files[i]);
          MESSAGE_ERROR (msg);
          return TCL_ERROR;
        }

      for (j = 0; j < count; ++j)
        {
          if (tcllib_load_file (list[j]) != TCL_OK)
            {
              TCL_RUNTIME_ERROR;

              /* Free memory used by unseen items */
              while (j < count)
                {
                  SAFE_FREE (list[j]);
                  ++j;
                }
              SAFE_FREE (list);

              return TCL_ERROR;
            }
          SAFE_FREE (list[j]);
        }

      SAFE_FREE (list);
    }

  hook_register (L"open-file-hook", _file_associations_hook, 1);

  return TCL_OK;
}
