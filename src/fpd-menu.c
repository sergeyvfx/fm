/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Menu for default file panel
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "file_panel-defact.h"
#include "i18n.h"
#include "messages.h"

#include <widgets/widget.h>

#define _ITEM(_caption, _callback) \
  item = w_submenu_append_item (__submenu, _(_caption), _callback, 0); \
  if (!item) \
    { \
      return -1; \
    } \
  item->user_data=__panel;

#define _SEPARATOR() \
  w_submenu_append_item (__submenu, 0, 0, SMI_SEPARATOR)


/**
 * Callback for `Sort order` menu item
 *
 * @param __user_data - associated file panel
 * @return zero if callback hasn't handled received character
 */
static int
menu_close_callback (void *__user_data)
{
  file_panel_t *panel;

  if (file_panel_get_count () <= 1)
    {
      MESSAGE_ERROR (_(L"You cannot close the only file panel"));
      return 0;
    }

  FPD_CHECK_PANEL_ASSOCIATED ();

  panel = __user_data;

  file_panel_destroy (panel);

  return 0;
}

/********
 * User's backend
 */

/**
 * Fill submenu with panel-related items
 *
 * @param __panel - for which panel this method has been called
 * @param __submenu - in which submenu items will be added
 * @return zero on success, non-zero on failure
 */
int
fpd_fill_submenu (file_panel_t *__panel, w_sub_menu_t *__submenu)
{
  w_sub_menu_item_t *item;

  if (!__panel || !__submenu)
    {
      return -1;
    }

  _ITEM (L"_Listing mode...", 0);
  _ITEM (L"_Sort order...", fpd_menu_sortorder_callback);

  if (file_panel_get_count () > 1)
    {
      _SEPARATOR ();
      _ITEM (L"_Close panel", menu_close_callback);
    }

  return 0;
}
