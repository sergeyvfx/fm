/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of stuff related to menu
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "iface.h"
#include "widget.h"
#include "hotkeys.h"
#include "i18n.h"

///////
// Variables

static w_menu_t *menu;

////////
//

/**
 * Callback for hotkey to show the menu
 */
static void
menu_hotkey_callback              (void)
{
  widget_set_focus (WIDGET (menu));
}

/**
 * Callback for File->Exit
 */
static int
menu_exit_clicked                 (void)
{
  iface_act_exit ();
  return 0;
}

/**
 * Fills default items of menu
 */
static void
fill_menu_items                   (void)
{
  w_sub_menu_t *sm;

  // Creating of submenu 'File'
  sm=w_menu_append_submenu (menu, _(L"_File"));
  w_submenu_append_item (sm, 0, 0, SMI_SEPARATOR);
  w_submenu_append_item (sm, _(L"_Exit"), menu_exit_clicked, 0);

  // Creating of submenu 'Command'
  sm=w_menu_append_submenu (menu, _(L"_Command"));

  // Creating of submenu 'Command'
  sm=w_menu_append_submenu (menu, _(L"_Options"));

  // Creating of submenu 'Help'
  sm=w_menu_append_submenu (menu, _(L"_Help"));
  w_submenu_append_item (sm, _(L"_About..."), 0, 0);
}

////////
// User's backend

/**
 * Creates main menu
 *
 * @return zero on success, non-zero on failure
 */
int
iface_create_menu                 (void)
{
  menu=widget_create_menu (WMS_HIDE_UNFOCUSED);
  
  if (!menu)
    return -1;

  fill_menu_items ();

  // Register callback to show the menu
  hotkey_register (L"F9", menu_hotkey_callback);

  return 0;
}
