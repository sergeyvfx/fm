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

#ifndef _file_panel_defact_
#  error Do not include this file directly. Include file_panel-defact.h instead.
#endif

#define FPD_CHECK_PANEL_ASSOCIATED() \
  { \
    if (!__user_data) \
      { \
        MESSAGE_ERROR (L"File panel isn't associated with menu item"); \
        return -1; \
      } \
  }

/* Fill submenu with panel-related items */
int
fpd_fill_submenu (file_panel_t *__panel, w_sub_menu_t *__submenu);

/* Callback for `Sort order` menu item */
int
fpd_menu_sortorder_callback (void *__user_data);
