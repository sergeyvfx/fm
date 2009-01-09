/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of stuff related to menu
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "iface.h"
#include "hotkeys.h"
#include "i18n.h"
#include "file_panel.h"
#include "hook.h"
#include "dynstruct.h"
#include "actions.h"

#include <widget.h>

#define _HOOK(_name) \
  hook_register (_name, fill_file_panel_menu_hook, 0)

#define BEGIN_MENU_DEF \
  w_sub_menu_t *sm;

#define DEFINE_MENU_ENTRY(_caption) \
  sm = w_menu_append_submenu (menu, _(_caption))

#define DEFINE_MENU_SEPARATOR \
  w_submenu_append_item (sm, 0, 0, SMI_SEPARATOR);

#define DEFINE_MENU_ITEM(_caption, _handler) \
  w_submenu_append_item (sm, _(_caption), _handler, 0);

#define DEFINE_MENU_CURRENT_PANEL_ACTION(_caption, _action,_args...) \
  int \
  menu_callback##_action (void *__reg_data ATTR_UNUSED) \
    { \
      file_panel_t *panel = current_panel; \
      _action (panel, ##_args); \
      return 0; \
    }; \
  DEFINE_MENU_ITEM (_caption, menu_callback##_action);

#define END_MENU_DEF \
  create_panel_submenu (current_panel);

/********
 * Variables
 */

static w_menu_t *menu;

/********
 *
 */

/**
 * Callback for hotkey to show the menu
 *
 * @return zero on success, non-zero otherwise
 */
static int
menu_hotkey_callback (void *__reg_data ATTR_UNUSED)
{
  widget_set_focus (WIDGET (menu));
  return 0;
}

/**
 * Callback for File->Exit
 *
 * @return zero on success, non-zero otherwise
 */
static int
menu_exit_clicked (void *__user_data ATTR_UNUSED)
{
  iface_act_exit ();
  return 0;
}

/**
 * Callback for Panel->Rescan
 *
 * @param __panel - file panel associated with menu item
 * @return zero on success, non-zero otherwise
 */
static int
menu_rescan_clicked (file_panel_t *__panel)
{
  if (!__panel)
    {
      return 1;
    }

  file_panel_rescan (__panel);

  return 0;
}

/**
 * Fill file panel's sub-menu
 *
 * @param __panel - for which panel submenu will be filled
 * @param __sub_menu - sub-menu to be filled
 */
static void
fill_panel_submenu (file_panel_t *__panel, w_sub_menu_t *__sub_menu)
{
  w_sub_menu_item_t *item;

  if (__panel)
    {
      /* Panel-specified items */
      FILE_PANEL_ACTION_CALL (__panel, fill_submenu, __sub_menu);
    }

  /* Hack to add separator if there are any */
  /* items from panel added */
  if (__sub_menu->items.length)
    {
      w_submenu_append_item (__sub_menu, 0, 0, SMI_SEPARATOR);
    }

  /* Add panel-independent items */
  item = w_submenu_append_item (__sub_menu, _(L"_Rescan"),
                                (menu_item_callback)menu_rescan_clicked, 0);
  item->user_data = __panel;
}

/**
 * Create submenu for specified file panel
 *
 * @param __panel - for which panel submenu will be created
 */
static void
create_panel_submenu (file_panel_t *__panel)
{
  static w_sub_menu_t *sm = NULL;
  static w_sub_menu_t *two_sm[] = {NULL, NULL};

  if (file_panel_get_count () != 2)
    {
      /* Remove sub-menus used to manage two panels separately */
      w_menu_remove_submenu (menu, two_sm[0]);
      w_menu_remove_submenu (menu, two_sm[1]);

      /* Create new sub-menu is needed */
      if (sm == NULL)
        {
          sm = w_menu_append_submenu (menu, _(L"_Panel"));
        }
      else
        {
          w_submenu_clear_items (sm);
        }

      fill_panel_submenu (__panel, sm);
    }
  else
    {
      /* Remove sub-menu used to manage current panel */
      w_menu_remove_submenu (menu, sm);

      /* Create new sub-menus is needed */
      if (two_sm[0] == NULL)
        {
          two_sm[0] = w_menu_insert_submenu (menu, _(L"_Left"), 0);
          two_sm[1] = w_menu_insert_submenu (menu, _(L"_Right"), 5);
        }
      else
        {
          w_submenu_clear_items (two_sm[0]);
          w_submenu_clear_items (two_sm[1]);
        }

      /*
       * FIXME: Use current and opposite panels to fill menus
       */

      fill_panel_submenu (file_panel_get_left (),  two_sm[0]);
      fill_panel_submenu (file_panel_get_right (), two_sm[1]);
    }
}

/**
 * Fill default items of menu
 */
static void
fill_menu_items (void)
{
  BEGIN_MENU_DEF

    /* Creating of submenu 'File' */
    DEFINE_MENU_ENTRY (L"_File");
    DEFINE_MENU_CURRENT_PANEL_ACTION (L"_Copy",         action_copy);
    DEFINE_MENU_CURRENT_PANEL_ACTION (L"C_hmod",        action_chmod);
    DEFINE_MENU_CURRENT_PANEL_ACTION (L"_Symlink",      action_symlink);
    DEFINE_MENU_CURRENT_PANEL_ACTION (L"Edit s_ymlink", action_editsymlink);
    DEFINE_MENU_CURRENT_PANEL_ACTION (L"Ch_own",        action_chown);
    DEFINE_MENU_CURRENT_PANEL_ACTION (L"_Rename/move",  action_move);
    DEFINE_MENU_CURRENT_PANEL_ACTION (L"_Mkdir",        action_mkdir);
    DEFINE_MENU_CURRENT_PANEL_ACTION (L"_Delete",       action_delete);
    DEFINE_MENU_SEPARATOR
    DEFINE_MENU_ITEM (L"_Exit", menu_exit_clicked);

    /* Creating of submenu 'Command' */
    DEFINE_MENU_ENTRY (L"_Command");

    DEFINE_MENU_CURRENT_PANEL_ACTION (L"_Find file", action_find);

    /* Creating of submenu 'Options' */
    DEFINE_MENU_ENTRY (L"_Options");

    /* Creating of submenu 'Help' */
    DEFINE_MENU_ENTRY (L"_Help");
    DEFINE_MENU_ITEM (L"_About...", NULL);

  END_MENU_DEF
}

/**
 * Handler for hooks "file-panel-focused-hook" and "file-panel-created-hook"
 *
 * @param __call_data - hook context data
 * @return HOOK_SUCCESS on success, HOOK_FAILURE otherwise
 */
static int
fill_file_panel_menu_hook (dynstruct_t *__call_data)
{
  file_panel_t *panel;

  if (dynstruct_get_field_val (__call_data, L"file-panel",
                               (void**)&panel) != DYNST_OK)
    {
      panel = current_panel;
    }

  create_panel_submenu (panel);

  return HOOK_SUCCESS;
}

/********
 * User's backend
 */

/**
 * Creates main menu
 *
 * @return zero on success, non-zero on failure
 */
int
iface_create_menu (void)
{
  menu = widget_create_menu (WMS_HIDE_UNFOCUSED);

  if (!menu)
    {
      return -1;
    }

  fill_menu_items ();

  _HOOK (L"file-panel-focused-hook");
  _HOOK (L"file-panel-created-hook");
  _HOOK (L"file-panel-after-destroy-hook");

  /* Register callback to show the menu */
  hotkey_bind (L".",  L"F9", menu_hotkey_callback);

  return 0;
}
