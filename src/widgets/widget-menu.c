/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation file for widget `menu`
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "widget.h"

/********
 *
 */

/* Draw padding of submenu root item */
#define SUBMENU_PADDING() \
  scr_wnd_font (layout, focused?*__menu->focused_font:*__menu->font); \
  scr_wnd_putch (layout, ' ');

#define END_MENU_ON_CALLBACK

/* From which position sub-menus starts drawing */
#define SUMBENUS_LEFT           1

/* Padding of caption on sub-menu */
#define SUBMENU_CAPTION_PADDING 1

/* Padding for item in submenu */
#define SUBMENU_ITEM_PADDING    1

/* Gets next and previous sub-menus */
#define SUBMENU_NEXT(__sub_menu) \
  (&((__sub_menu)->menu->sub_menus.data[((__sub_menu)->index+1)% \
  (__sub_menu)->menu->sub_menus.length]))

#define SUBMENU_PREV(__sub_menu) \
  (&((__sub_menu)->menu->sub_menus.data[((__sub_menu)->index-1)>=0?\
    ((__sub_menu)->index-1):(__sub_menu)->menu->sub_menus.length-1]))

#define POP_CONTEXT(__context) \
  if (__context && hotkey_current_context () == __context) \
    { \
      hotkey_pop_context (FALSE); \
    }

/********
 *
 */

/* Hotkey context for all menu widgets */
static hotkey_context_t *menu_context = NULL;

static void
hide_menu (w_menu_t *__menu);

/********
 *
 */

/**
 * Destructor of sub-menu
 *
 * @param __submenu - submenu to be destroyed
 */
static void
submenu_destructor (w_sub_menu_t *__submenu)
{
  if (!__submenu)
    {
      return;
    }

  if (__submenu->caption)
    {
      free (__submenu->caption);
    }
}

/**
 * Destructor of menu widget
 *
 * @param __menu - menu to be destroyed
 * @return zero in success, non-zero on failure
 */
static int
menu_destructor (w_menu_t *__menu)
{
  int i;

  if (!__menu)
    {
      return -1;
    }

  /* Destroy all sub-menus */
  for (i = 0; i < __menu->sub_menus.length; i++)
    {
      submenu_destructor (&__menu->sub_menus.data[i]);
    }

  /* Hide menu before deleting to minimize blinking of screen */
  hide_menu (__menu);

  /* Destroy any panels and layouts */
  if (__menu->panel)
    {
      panel_del (__menu->panel);
    }

  if (WIDGET_LAYOUT (__menu))
    {
      scr_destroy_window (WIDGET_LAYOUT (__menu));
    }

  if (__menu->submenu_panel)
    {
      panel_del (__menu->submenu_panel);
      scr_destroy_window (__menu->submenu_layout);
    }

  if (__menu->submenu_layout)
    {
      scr_destroy_window (__menu->submenu_layout);
    }

  SAFE_FREE (__menu->sub_menus.data);

  return 0;
}

/**
 * Drawer of menu
 *
 * @param __menu - menu to be drawn
 * @return zero in success, non-zero on failure
 */
static int
menu_drawer (w_menu_t *__menu)
{
  int i;
  BOOL focused;

  if (!WIDGET_VISIBLE (__menu) || !WIDGET_LAYOUT (__menu))
    {
      return -1;
    }

  scr_window_t layout = WIDGET_LAYOUT (__menu);

  /* Backup attributes */
  scr_wnd_attr_backup (layout);

  scr_wnd_bkg (layout, *__menu->font);

  /* Move caret to position of left submenu */
  scr_wnd_move_caret (layout, SUMBENUS_LEFT, 0);

  for (i = 0; i < __menu->sub_menus.length; i++)
    {
      focused = &__menu->sub_menus.data[i] == __menu->cur_submenu;

      SUBMENU_PADDING ();

      widget_shortcut_print (layout, __menu->sub_menus.data[i].caption,
                     focused ? *__menu->focused_font : *__menu->font,
                     focused ? *__menu->hot_focused_font : *__menu->hot_font);

      SUBMENU_PADDING ();
    }

  /* We haven't been here (c) :) */
  scr_wnd_attr_restore (layout);

  return 0;
}

/**
 * Draw a specified item of sub-menu
 *
 * @param __sub_menu - sub-menu from which you want to draw an item
 * @param __index - index of item to draw
 */
void
draw_submenu_item (const w_sub_menu_t *__sub_menu, short __index)
{
  if (!__sub_menu || __index < 0 || __index >= __sub_menu->items.length)
    {
      return;
    }

  scr_window_t layout = __sub_menu->menu->submenu_layout;
  w_menu_t *menu = __sub_menu->menu;
  short i;
  BOOL separator = __sub_menu->items.data[__index].flags&SMI_SEPARATOR;

  if (!layout)
    {
      return;
    }

  scr_wnd_font (layout,
                __index == __sub_menu->cur_item_index ? *menu->focused_font :
                    *menu->font);

  /* Clear line for submenu's item */
  scr_wnd_move_caret (layout, 1, __index + 1);
  for (i = 0; i < __sub_menu->position.width - 2; i++)
    {
      scr_wnd_putch (layout, separator ? ACS_HLINE : ' ');
    }

  if (!separator)
    {
      scr_wnd_move_caret (layout, SUBMENU_ITEM_PADDING + 1, __index + 1);

      /* Draw caption of item */
      widget_shortcut_print (layout, __sub_menu->items.data[__index].caption,
              __index == __sub_menu->cur_item_index ? *menu->focused_font :
              *menu->font,
              __index == __sub_menu->cur_item_index ? *menu->hot_focused_font :
              *menu->hot_font);
    }
}

/**
 * Draw a sub menu.
 *
 * @param __sub_menu - sub-menu to be drawn
 */
void
draw_submenu (const w_sub_menu_t *__sub_menu)
{
  if (!__sub_menu)
    {
      return;
    }

  short i;
  scr_window_t layout = __sub_menu->menu->submenu_layout;

  if (!layout)
    {
      return;
    }

  scr_wnd_bkg (layout, *__sub_menu->menu->font);

  scr_wnd_border (layout);

  for (i = 0; i < __sub_menu->items.length; ++i)
    {
      draw_submenu_item (__sub_menu, i);
    }

  scr_wnd_refresh (layout);
}

/**
 * Show menu on screen
 *
 * @param __menu - menu to be shown
 */
static void
show_menu (w_menu_t *__menu)
{
  if (!__menu)
    {
      return;
    }

  WIDGET_POSITION (__menu).z = 1;

  panel_show (__menu->panel);
}

/**
 * Hide menu from screen
 *
 * @param __menu - menu to be hidden
 */
static void
hide_menu (w_menu_t *__menu)
{
  if (!__menu)
    {
      return;
    }

  WIDGET_POSITION (__menu).z = 0;

  panel_hide (__menu->submenu_panel);
  panel_hide (__menu->panel);

  __menu->unfolded = FALSE;
  __menu->focused  = FALSE;
}

/**
 * Return a position of sub-menu
 *
 * @param __sub_menu - sub-menu to get a position of
 * @return a position of specified sub-menu
 */
static widget_position_t
submenu_position (const w_sub_menu_t *__sub_menu)
{
  int i, n;
  widget_position_t res = {0, 0, 0, 0};

  res.x = SUMBENUS_LEFT;
  res.y = 1;

  res.height = 2 + __sub_menu->items.length; /* 2 is for border */
  res.width = 2;

  /* Calculate the `x` coordinate */
  for (i = 0, n = __sub_menu->menu->sub_menus.length; i < n; ++i)
    {
      if (&__sub_menu->menu->sub_menus.data[i] == __sub_menu)
        {
          break;
        }
      res.x += widget_shortcut_length (
                                    __sub_menu->menu->sub_menus.data[i].caption
                                    ) + SUBMENU_CAPTION_PADDING * 2;
    }

  /* Calculate width */
  for (i = 0, n = __sub_menu->items.length; i < n; ++i)
    {
      res.width = MAX (res.width,
                   widget_shortcut_length (__sub_menu->items.data[i].caption) +
                   2 + SUBMENU_ITEM_PADDING * 2);
    }

  return res;
}

/**
 * Return index of first focusable item in submenus
 *
 * @param __sub_menu - sub-menu where to find first focusable item
 * @return index of first focusable element
 */
static short
submenu_item_first (const w_sub_menu_t *__sub_menu)
{
  if (!__sub_menu->items.length)
    {
      return -1;
    }

  short i, n = __sub_menu->items.length;

  for (i = 0; i < n; ++i)
    {
      if (!(__sub_menu->items.data[i].flags & SMI_SEPARATOR))
        {
          return i;
        }
    }

  return -1;
}

/**
 * Return index of next focusable item in submenus
 *
 * @param __sub_menu - sub-menu where to find next focusable item
 * @return index of next focusable element
 */
static short
submenu_item_next (const w_sub_menu_t *__sub_menu)
{
  if (!__sub_menu->items.length)
    {
      return -1;
    }

  short i, n = __sub_menu->items.length,
        res = (__sub_menu->cur_item_index + 1) % __sub_menu->items.length;

  for (i = 0; i < n; ++i)
    {
      if (__sub_menu->items.data[res].flags & SMI_SEPARATOR)
        {
          res = (res + 1) % n;
        }
      else
        {
          break;
        }
    }

  if (__sub_menu->items.data[res].flags & SMI_SEPARATOR)
    {
      return -1;
    }

  return res;
}

/**
 * Return index of previous focusable item in submenus
 *
 * @param __sub_menu - sub-menu where to find previous focusable item
 * @return index of previous focusable element
 */
static short
submenu_item_prev (const w_sub_menu_t *__sub_menu)
{
  if (!__sub_menu->items.length)
    {
      return -1;
    }

  short i, n = __sub_menu->items.length,
        res = __sub_menu->cur_item_index - 1;

  if (res < 0)
    {
      res = n - 1;
    }

  for (i = 0; i < n; ++i)
    {
      if (__sub_menu->items.data[res].flags & SMI_SEPARATOR)
        {
          res = res > 0 ? res - 1 : n - 1;
        }
      else
        {
          break;
        }
    }

  if (__sub_menu->items.data[res].flags & SMI_SEPARATOR)
    {
      return -1;
    }

  return res;
}

/**
 * Unfold sub menu
 *
 * @param __sub_menu - submenu is to be unfolded
 */
static void
unfold_submenu (w_sub_menu_t *__sub_menu)
{
  if (!__sub_menu)
    {
      return;
    }

  w_menu_t *menu = __sub_menu->menu;
  widget_position_t pos = __sub_menu->position = submenu_position (__sub_menu);

  __sub_menu->cur_item_index = submenu_item_first (__sub_menu);

  /* Create a layout and panel (if necessary) */
  scr_window_t tmp = scr_create_window (pos.x, pos.y, pos.width, pos.height);
  if (!menu->submenu_layout)
    {
      menu->submenu_layout = tmp;
      menu->submenu_panel = panel_new (menu->submenu_layout);
    }
  else
    {
      /* There is no stuff to resize panels, */
      /* So, we have just to replace layout */
      scr_window_t old = menu->submenu_layout;
      menu->submenu_layout = tmp;
      panel_replace (menu->submenu_panel, tmp);
      scr_destroy_window (old);
      panel_show (menu->submenu_panel);
    }

  panel_move (menu->submenu_panel, pos.x, pos.y);

  /* Now menu is unfolded */
  menu->unfolded = TRUE;

  draw_submenu (__sub_menu);
}

/**
 * Set __sub_menu as current submenu of __menu
 *
 * @param __menu - base menu
 * @param __sub_menu - new active submenu
 */
static void
switch_to_submenu (w_sub_menu_t *__sub_menu)
{
  __sub_menu->menu->cur_submenu = __sub_menu;
  widget_redraw (WIDGET (__sub_menu->menu));

  if (__sub_menu->menu->unfolded)
    {
      unfold_submenu (__sub_menu);
    }
}

/**
 * Set focus to specified item in sub-menu
 *
 * @param __sub_menu - sub-menu where item is to be focused
 * @param __index - index of item to be focused
 */
static void
set_submenu_focused_item (w_sub_menu_t *__sub_menu, short __index)
{
  if (!__sub_menu)
    {
      return;
    }

  short sindex = __sub_menu->cur_item_index;
  __sub_menu->cur_item_index = __index;

  draw_submenu_item (__sub_menu, sindex);
  draw_submenu_item (__sub_menu, __index);

  scr_wnd_refresh (__sub_menu->menu->submenu_layout);
}

/**
 * End handling messages for menu
 *
 * @param __menu - for which menu stop handling
 */
static void
end_menu (w_menu_t *__menu)
{
  if (!__menu)
    {
      return;
    }

  /* After mapper finished, menu lost focus */
  WIDGET_CALL_CALLBACK (__menu, blured, __menu);

  if (__menu->style & WMS_HIDE_UNFOCUSED)
    {
      hide_menu (__menu);
      widget_delete_root (WIDGET (__menu));
    }
  else
    {
      widget_sink_root (WIDGET (__menu));
      panel_hide (__menu->submenu_panel);
    }

  __menu->unfolded = FALSE;
  __menu->cur_submenu = 0;
  __menu->active = FALSE;
  __menu->focused = FALSE;

  hotkey_enable ();

  POP_CONTEXT (WIDGET_USER_CONTEXT  (__menu));
  POP_CONTEXT (WIDGET_CLASS_CONTEXT (__menu));

  widget_redraw (WIDGET (__menu));
}

/**
 * Handle a keydown callback
 *
 * @param __menu - menu received a callback
 * @param __ch - received character
 * @return zero if callback hasn't handled received character
 *   non-zero otherwise
 */
static int
menu_keydown (w_menu_t *__menu, wint_t __ch)
{
  if (!__menu)
    {
      return 0;
    }

  switch (__ch)
    {
    case KEY_ESC:
    case KEY_ESC_ESC:
      if (__menu->unfolded)
        {
          panel_hide (__menu->submenu_panel);
          __menu->unfolded = FALSE;
        }
      else
        {
          end_menu (__menu);
        }
      break;

    /* Navigation */
    case KEY_LEFT:
      switch_to_submenu (SUBMENU_PREV (__menu->cur_submenu));
      break;

    case KEY_RIGHT:
      switch_to_submenu (SUBMENU_NEXT (__menu->cur_submenu));
      break;

    case KEY_RETURN:
      if (__menu->unfolded)
        {
          short cur_index;
          menu_item_callback callback;
          /* Some checking to be sure all is good */
          if (__menu->cur_submenu &&
              (cur_index = __menu->cur_submenu->cur_item_index) >= 0 &&
              cur_index < __menu->cur_submenu->items.length &&
              (callback = __menu->cur_submenu->items.data[cur_index].callback)
              )
            {
              void *user_data;
              user_data = __menu->cur_submenu->items.data[cur_index].user_data;

#ifdef END_MENU_ON_CALLBACK
              end_menu (__menu);
#endif
              callback (user_data);
            }
        }
      else
        unfold_submenu (__menu->cur_submenu);
      break;

      /* Navigation inside sub-menu */
    case KEY_DOWN:
      if (!__menu->unfolded)
        {
          unfold_submenu (__menu->cur_submenu);
        }
      else
        {
          set_submenu_focused_item (__menu->cur_submenu,
                                    submenu_item_next (__menu->cur_submenu));
        }
      break;
    case KEY_UP:
      if (__menu->unfolded)
        {
          set_submenu_focused_item (__menu->cur_submenu,
                                    submenu_item_prev (__menu->cur_submenu));
        }
      break;

    default:
      {
        short i;
        BOOL found = FALSE;
        wchar_t tmp = towlower (__ch);
        if (__menu->unfolded && __menu->cur_submenu)
          {
            w_sub_menu_item_t *item;
            for (i = 0; i < __menu->cur_submenu->items.length; ++i)
              {
                item = &__menu->cur_submenu->items.data[i];
                if (item->shortcut == tmp)
                  {
                    found = TRUE;
                    if (item->callback)
                      {
                        item->callback (item->user_data);
                      }
                  }
              }
          }

        if (!found)
          {
            for (i = 0; i < __menu->sub_menus.length; i++)
              {
                if (__menu->sub_menus.data[i].shortcut == tmp)
                  {
                    switch_to_submenu (&__menu->sub_menus.data[i]);
                    return 0;
                  }
              }
            return -1;
          }
        return 0;
      }
    }

  return 0;
}

/**
 * Handler of `focused` callback (system-based)
 *
 * @param __menu - menu which catched this callback
 * @return zero if callback hasn't handled callback
 */
static int
menu_focused (w_menu_t *__menu)
{
  /* Call user-defined handler of focused callback */
  _WIDGET_CALL_USER_CALLBACK (__menu, focused, __menu);

  /* When menu is fosued we should map all input signals */
  /* (a.k.a focused menu is a type of modal window) */

  if (__menu->active)
    {
      return 1;
    }

  __menu->active = TRUE;

  /* IDK what to do if there is no submenus */
  if (__menu->sub_menus.length)
    {
      hotkey_disable ();
      show_menu (__menu);
      widget_add_root (WIDGET (__menu));

      __menu->cur_submenu = &__menu->sub_menus.data[0];
    }

  return 0;
}

/**
 * Handler of `onresize` callback (system-based)
 *
 * @param __menu - menu which catched this callback
 * @return zero if callback hasn't handled callback
 */
static int
menu_onresize (w_menu_t *__menu)
{
  if (!__menu)
    {
      return -1;
    }

  /* Call to user-defined handler of focused callback */
  _WIDGET_CALL_USER_CALLBACK (__menu, onresize, __menu);

  /* Redraw main line of menu */
  if (__menu->layout)
    {
      scr_wnd_resize (__menu->layout, SCREEN_WIDTH, 1);
      widget_redraw (WIDGET (__menu));
    }

  /* Redraw unfolded submenu */
  if (__menu->unfolded)
    {
      widget_position_t pos = submenu_position (__menu->cur_submenu);
      scr_wnd_resize (__menu->submenu_layout, pos.width, pos.height);
      draw_submenu (__menu->cur_submenu);
    }

  return 0;
}

/********
 * User's backend
 */

/**
 * Create menu with specified style
 *
 * @param __style - style of menu. Possible values:
 *  WMS_HIDE_UNFOCUSED
 * @return pointer to new menu object
 */
w_menu_t*
widget_create_menu (unsigned int __style)
{
  w_menu_t *res;

  /* Create context for box widgets */
  if (!menu_context)
    {
      menu_context = hotkey_create_context (L"menu-class-context", 0);
    }

  WIDGET_INIT (res, w_menu_t, WT_MENU, 0, WF_ONTOP,
               menu_context,
               menu_destructor, menu_drawer,
               0, 0, 1, SCREEN_WIDTH, 1);

  res->panel = panel_new (res->layout);

  WIDGET_CALLBACK (res, focused) = (widget_action) menu_focused;
  WIDGET_CALLBACK (res, onresize) = (widget_action) menu_onresize;
  WIDGET_CALLBACK (res, keydown) = (widget_keydown_proc) menu_keydown;

  /* Look&feel */
  res->font         = FONT (CID_BLACK, CID_CYAN);
  res->focused_font = FONT (CID_WHITE, CID_BLACK);

  res->hot_font         = FONT (CID_YELLOW, CID_CYAN);
  res->hot_focused_font = FONT (CID_YELLOW, CID_BLACK);

  res->style = __style;

  WIDGET_POST_INIT (res);

  if (__style & WMS_HIDE_UNFOCUSED)
    {
      widget_delete_root (WIDGET (res));
      hide_menu (res);
    }
  else
    {
      widget_sink_root (WIDGET (res));
      widget_redraw (WIDGET (res));
      show_menu (res);
    }

  res->active = 0;
  res->unfolded = FALSE;

  return res;
}

/**
 * Append new submenu to menu
 *
 * @param __menu - menu where new submenu have to be appended
 * @param __caption - caption of submenu
 * @return pointer to new submenu
 */
w_sub_menu_t*
w_menu_append_submenu (w_menu_t *__menu, const wchar_t *__caption)
{
  w_sub_menu_t *res;

  if (!__menu)
    {
      return 0;
    }

  __menu->sub_menus.data = realloc (__menu->sub_menus.data,
                                    (__menu->sub_menus.length + 1) *
                                    sizeof (*__menu->sub_menus.data));

  res = &__menu->sub_menus.data[__menu->sub_menus.length];
  memset (res, 0, sizeof (*res));

  if (__caption)
    {
      res->caption = wcsdup (__caption);
      res->shortcut = widget_shortcut_key (__caption);
    }

  res->index = __menu->sub_menus.length;

  res->menu = __menu;

  __menu->sub_menus.length++;

  widget_redraw (WIDGET (__menu));

  return res;
}

/**
 * Append an item to submenu
 *
 * @param __sub_menu - sub-menu where you want to append an item
 * @param __caption - caption of item (shortcuts are available)
 * @param __callback - callback to be called wgen user activates this item
 * @return pointer to new created item
 */
w_sub_menu_item_t*
w_submenu_append_item (w_sub_menu_t *__sub_menu, const wchar_t *__caption,
                       menu_item_callback __callback, unsigned int __flags)
{
  short index; /* For some optimization of deference */

  if (!__sub_menu)
    return 0;

  __sub_menu->items.data = realloc (__sub_menu->items.data,
                                    ((index = __sub_menu->items.length) + 1) *
                                    sizeof (w_sub_menu_item_t));

  memset (&__sub_menu->items.data[index], 0,
          sizeof (__sub_menu->items.data[index]));

  if (__caption && !(__flags & SMI_SEPARATOR))
    {
      __sub_menu->items.data[index].caption = wcsdup (__caption);
      __sub_menu->items.data[index].shortcut = widget_shortcut_key (__caption);
    }

  __sub_menu->items.data[index].flags = __flags;
  __sub_menu->items.data[index].callback = __callback;

  __sub_menu->items.length++;

  return &__sub_menu->items.data[index];
}

/**
 * HIde menu
 *
 * @param __mennu - menu to be hidden
 */
void
w_menu_hide (w_menu_t *__menu)
{
  end_menu (__menu);
}
