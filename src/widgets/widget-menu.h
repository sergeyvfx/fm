/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _widget_menu_h_
#define _widget_menu_h_

#ifndef _widget_h_
#  error Do not include this file directly. Include widget.h instead.
#endif

/********
 * Constants
 */

/* Menu styles */
#define WMS_NONE           0x0000
#define WMS_HIDE_UNFOCUSED 0x0001

/* Sub-menu's item styles */
#define SMI_NONE           0x0000
#define SMI_SEPARATOR      0x0001

/********
 * Type defenitions
 */

struct w_menu_t_entry;
typedef int (*menu_item_callback) (void *__user_data);

typedef struct
{
  /* Caption of item */
  wchar_t *caption;

  wchar_t shortcut;
  unsigned int flags;
  void *user_data;
  menu_item_callback callback;
} w_sub_menu_item_t;

typedef struct
{
  /* Caption of sub-menu */
  wchar_t *caption;

  wchar_t shortcut;

  struct
  {
    w_sub_menu_item_t **data;
    unsigned short length;
  } items;

  /* Menu which owns this submenu */
  struct w_menu_t_entry *menu;

  /* Index of submenu in parent's container */
  short index;

  /* Position of sub-menu */
  widget_position_t position;

  /* Index of currently selected item */
  short cur_item_index;
} w_sub_menu_t;

typedef struct w_menu_t_entry
{
  /* Inherit from widget */
  WIDGET_MEMBERS

  /* Panel of layout to manipulate with visibility */
  panel_t panel;

  /* Font to draw menu line */
  scr_font_t *font;

  /* Font to draw shortcut */
  scr_font_t *hot_font;

  /* Font to draw submenu caption in case it's focused */
  scr_font_t *focused_font;

  /* Font to draw focused shortcut */
  scr_font_t *hot_focused_font;

  /* Style of menu */
  unsigned int style;

  struct
  {
    w_sub_menu_t **data;
    unsigned short length;
  } sub_menus;

  /****
   * Internal usage
   */

  /* Pointer to currently selected submenu */
  w_sub_menu_t *cur_submenu;

  /* Layout to draw a submemu's items */
  scr_window_t submenu_layout;

  /* Panel to manipulate with submenu's layout */
  panel_t submenu_panel;

  /* Shows if any sub-menu is unfolded */
  BOOL unfolded;

  /* Is menu activate? */
  BOOL active;
} w_menu_t;

/********
 *
 */

/* Create menu with specified style */
w_menu_t*
widget_create_menu (unsigned int __style);

/* Insert new submenu to menu */
w_sub_menu_t*
w_menu_insert_submenu (w_menu_t *__menu, const wchar_t *__caption,
                       unsigned short __pos);

/* Append new submenu to menu */
w_sub_menu_t*
w_menu_append_submenu (w_menu_t *__menu, const wchar_t *__caption);

/* Remove sub-menu from menu */
void
w_menu_remove_submenu (w_menu_t *__menu, w_sub_menu_t *__sub_menu);

/* Hide menu */
void
w_menu_hide (w_menu_t *__menu);

/* Append an item to submenu */
w_sub_menu_item_t*
w_submenu_append_item (w_sub_menu_t *__sub_menu, const wchar_t *__caption,
                       menu_item_callback __callback, unsigned int __flags);

/* Remove all items from sub-menu */
void
w_submenu_clear_items (w_sub_menu_t *__sub_menu);

#endif
