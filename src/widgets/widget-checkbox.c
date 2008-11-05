/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of widget checkbox
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "widget.h"
#include "screen.h"
#include "util.h"

#include <wchar.h>

/* Hotkey context for all checkboxbox widgets */
static hotkey_context_t *checkbox_context = NULL;

/**
 * Destroys a checkbox widget
 *
 * @param __checkbox - checkbox to be destroyed
 * @return 0 if successful, otherwise -1
 */
static int
checkbox_destructor (w_checkbox_t *__checkbox)
{
  if (!__checkbox)
    {
      return -1;
    }

  SAFE_FREE (__checkbox->caption);

  if (WIDGET_LAYOUT (__checkbox))
    {
      scr_destroy_window (WIDGET_LAYOUT(__checkbox));
    }

  return 0;
}

/**
 * Draw a checkbox
 *
 * @param __checkbox - checkbox to be drawn
 * @return 0 if successful, otherwise -1
 */
static int
checkbox_drawer (w_checkbox_t *__checkbox)
{
  scr_window_t layout = WIDGET_LAYOUT (__checkbox);
  char ch;

  if (!__checkbox || !WIDGET_VISIBLE(__checkbox) || !layout)
    {
      return -1;
    }

  scr_wnd_attr_backup (layout);
  scr_wnd_move_caret (layout, __checkbox->position.x, __checkbox->position.y);

  if (__checkbox->caption)
    {
      scr_wnd_font (layout, __checkbox->focused ? *__checkbox->font_focus
          : *__checkbox->font);

      if (__checkbox->style & WCBS_WITH_UNDEFINED &&
          __checkbox->ischeck == WCB_STATE_UNDEFINED)
        {
          ch = '-';
        }
      else
        {
          if (__checkbox->ischeck)
            {
              ch = 'x';
            }
          else
            {
              ch = ' ';
            }
        }
      scr_wnd_printf (layout, "[%c]", ch);

      if (wcscmp (__checkbox->caption, L"") != 0)
        {
          scr_wnd_putch (layout, ' ');
          widget_shortcut_print (layout, __checkbox->caption,
             __checkbox->focused ? *__checkbox->font_focus : *__checkbox->font,
             __checkbox->focused ? *__checkbox->hotkey_focus :
                                   *__checkbox->hotkey_font);
        }
    }

  scr_wnd_attr_restore (layout);

  return 0;
}

/**
 * Turn state of checkbox
 *
 * @param __checkbox - for which checkbox state will be turned
 */
static void
checkbox_turn_state (w_checkbox_t *__checkbox)
{
  if (__checkbox->style & WCBS_WITH_UNDEFINED)
    {
      if (__checkbox->ischeck == WCB_STATE_UNDEFINED)
        {
          __checkbox->ischeck = FALSE;
        }
      else
        {
          if (__checkbox->ischeck)
            {
              __checkbox->ischeck = WCB_STATE_UNDEFINED;
            }
          else
            {
              __checkbox->ischeck = TRUE;
            }
        }
    }
  else
    {
      __checkbox->ischeck = !__checkbox->ischeck;
    }

  WIDGET_CALL_USER_CALLBACK (__checkbox, property_changed,
                             __checkbox, W_CHECKBOX_CHECKED_PROP);
}

/**
 * Handles a keydown callback
 *
 * @param __checkbox - checkbox received a callback
 * @param __ch - received character
 * @return FALSE if callback hasn't handled received character
 */
static int
checkbox_keydown (w_checkbox_t *__checkbox, wint_t __ch)
{
  _WIDGET_CALL_USER_CALLBACK (__checkbox, keydown, __checkbox, __ch);

  switch (__ch)
    {
    case KEY_SPACE:
      checkbox_turn_state (__checkbox);
      widget_redraw (WIDGET (__checkbox));
      return TRUE;
    }

  return FALSE;
}

/**
 * Handles a shortcut callback
 *
 * @param __checkbox - checkbox for which this callback sent
 * @return zero if callback hasn't handled received character,
 * non-zero otherwise
 */
static int
checkbox_shortcut (w_checkbox_t *__checkbox)
{
  _WIDGET_CALL_USER_CALLBACK (__checkbox, shortcut, __checkbox);
  widget_set_focus (WIDGET (__checkbox));

  checkbox_turn_state (__checkbox);
  widget_redraw (WIDGET (__checkbox));

  return TRUE;
}

/**
 * Create new checkbox
 *
 * @param __parent - parent of the checkbox. Should be container.
 * @param __caption - caption on checkbox
 * @param __x, __y - coordinates  of checkbox
 * @param __check - check state of checkbox
 * @param __style - style of checkbox
 * @return a pointer to checkbox object if successful, otherwise NULL
 */
w_checkbox_t*
widget_create_checkbox (w_container_t *__parent, const wchar_t *__caption,
                        int __x, int __y, BOOL __check, unsigned int __style)
{
  w_checkbox_t *widget;

  if (!__parent || !__caption)
    {
      return NULL;
    }

  /* Create context for checkbox widgets */
  if (!checkbox_context)
    {
      checkbox_context = hotkey_create_context (L"checkbox-class-context", 0);
    }

  WIDGET_INIT (widget, w_checkbox_t, WT_CHECKBOX, __parent, WF_NOLAYOUT,
               checkbox_context,
               checkbox_destructor, checkbox_drawer,
               __x, __y, 1, wcslen(__caption) + 4, 1);

  widget->style = __style;

  WIDGET_CALLBACK (widget, keydown) = (widget_keydown_proc)checkbox_keydown;
  WIDGET_CALLBACK (widget, shortcut) = (widget_action)checkbox_shortcut;

  WIDGET_SHORTCUT (widget) = widget_shortcut_key (__caption);

  widget->caption = wcsdup (__caption);

  widget->font         = FONT (CID_BLACK, CID_GREY);
  widget->font_focus   = FONT (CID_BLACK, CID_CYAN);
  widget->hotkey_font  = FONT (CID_YELLOW, CID_GREY);
  widget->hotkey_focus = FONT (CID_YELLOW, CID_CYAN);

  widget->ischeck = __check;

  WIDGET_POST_INIT (widget);

  return widget;
}

/**
 * Get check state from the checkbox
 *
 * @param __checkbox - descriptor of checkbox
 * @return WCB_STATE_UNDEFINED is state is undefined,
 * TRUE if checked, otherwise FALSE
 */
BOOL
w_checkbox_get (const w_checkbox_t *__checkbox)
{
  return __checkbox->ischeck;
}

/**
 * Set check state from the checkbox
 *
 * @param __checkbox - descriptor of checkbox
 * @param __state - new check state
 */
void
w_checkbox_set (w_checkbox_t *__checkbox, BOOL __state)
{
  if (!__checkbox)
    {
      return;
    }

  __checkbox->ischeck = __state;
  widget_redraw (WIDGET (__checkbox));

  /* Call property changed callback */
  WIDGET_CALL_USER_CALLBACK (__checkbox, property_changed,
                             __checkbox, W_CHECKBOX_CHECKED_PROP);
}

/**
 * Set colors used in checkbox
 *
 * @param __checkbox - for which checkbox fonts will be set
 * @param __font - font of normal text
 * @param __font_focus - font of text when checkbox is focused
 * @param __hotkey_font - font of shortcut character
 * @param __hotkey_focus - font of shortcut character when checkbox is focused
 */
void
w_checkbox_set_fonts (w_checkbox_t *__checkbox, scr_font_t *__font,
                      scr_font_t *__font_focus, scr_font_t *__hotkey_font,
                      scr_font_t *__hotkey_focus)
{
  if (!__checkbox)
    {
      return;
    }

  WIDGET_SAFE_SET_FONT (__checkbox, font,         __font);
  WIDGET_SAFE_SET_FONT (__checkbox, font_focus,   __font_focus);
  WIDGET_SAFE_SET_FONT (__checkbox, hotkey_font,  __hotkey_font);
  WIDGET_SAFE_SET_FONT (__checkbox, hotkey_focus, __hotkey_focus);
}
