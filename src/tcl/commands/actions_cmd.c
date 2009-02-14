#include <tcl.h>

#include <file_panel.h>
#include <actions/actions.h>

#include "commands_list.h"

/**
 * This function implements the "copy" Tcl command
 * See the ${project-name} user documentation for details on what it does
 */
TCL_DEFUN(_tcl_actions_copy_cmd)
{
  action_copy (file_panel_get_current_panel());
  return TCL_OK;
}

/**
 * This function implements the "move" Tcl command
 * See the ${project-name} user documentation for details on what it does
 */
TCL_DEFUN(_tcl_actions_move_cmd)
{
  action_move (file_panel_get_current_panel());
  return TCL_OK;
}

/**
 * This function implements the "mkdir" Tcl command
 * See the ${project-name} user documentation for details on what it does
 */
TCL_DEFUN(_tcl_actions_mkdir_cmd)
{
  action_mkdir (file_panel_get_current_panel());
  return TCL_OK;
}

/**
 * This function implements the "delete" Tcl command
 * See the ${project-name} user documentation for details on what it does
 */
TCL_DEFUN(_tcl_actions_delete_cmd)
{
  action_delete (file_panel_get_current_panel());
  return TCL_OK;
}

/**
 * This function implements the "symlink" Tcl command
 * See the ${project-name} user documentation for details on what it does
 */
TCL_DEFUN(_tcl_actions_symlink_cmd)
{
  action_symlink (file_panel_get_current_panel());
  return TCL_OK;
}

/**
 * This function implements the "editsymlink" Tcl command
 * See the ${project-name} user documentation for details on what it does
 */
TCL_DEFUN(_tcl_actions_editsymlink_cmd)
{
  action_editsymlink (file_panel_get_current_panel());
  return TCL_OK;
}

/**
 * This function implements the "chown" Tcl command
 * See the ${project-name} user documentation for details on what it does
 */
TCL_DEFUN(_tcl_actions_chown_cmd)
{
  action_chown (file_panel_get_current_panel());
  return TCL_OK;
}

/**
 * This function implements the "chmod" Tcl command
 * See the ${project-name} user documentation for details on what it does
 */
TCL_DEFUN(_tcl_actions_chmod_cmd)
{
  action_chmod (file_panel_get_current_panel());
  return TCL_OK;
}

/**
 * This function implements the "find" Tcl command
 * See the ${project-name} user documentation for details on what it does
 */
TCL_DEFUN(_tcl_actions_find_cmd)
{
  action_find (file_panel_get_current_panel());
  return TCL_OK;
}

/**
 * This function implements the "create_file" Tcl command
 * See the ${project-name} user documentation for details on what it does
 */
TCL_DEFUN(_tcl_actions_create_file_cmd)
{
  action_create_file (file_panel_get_current_panel());
  return TCL_OK;
}

/**
 * Initialize Tcl commands for actions
 *
 * @param __interp - a pointer on Tcl interpreter
 * @return TCL_OK if successful, TCL_ERROR otherwise
 */
int
_tcl_actions_init_commands (Tcl_Interp *__interp)
{
  /*
   * TODO: should be part of file_panel module
   */
  TCL_DEFSYM_BEGIN
    TCL_DEFSYM("::actions::copy", _tcl_actions_copy_cmd),
    TCL_DEFSYM("::actions::move", _tcl_actions_move_cmd),
    TCL_DEFSYM("::actions::mkdir", _tcl_actions_mkdir_cmd),
    TCL_DEFSYM("::actions::delete", _tcl_actions_delete_cmd),
    TCL_DEFSYM("::actions::symlink", _tcl_actions_symlink_cmd),
    TCL_DEFSYM("::actions::editsymlink", _tcl_actions_editsymlink_cmd),
    TCL_DEFSYM("::actions::chown", _tcl_actions_chown_cmd),
    TCL_DEFSYM("::actions::chmod", _tcl_actions_chmod_cmd),
    TCL_DEFSYM("::actions::find", _tcl_actions_find_cmd),
    TCL_DEFSYM("::actions::create_file", _tcl_actions_create_file_cmd),
  TCL_DEFSYM_END

  TCL_DEFCREATE(__interp);
  return TCL_OK;
}
