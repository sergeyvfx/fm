/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation file for widget `button`
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "widget.h"

//////
//

/**
 * Destroys a button widget
 *
 * @param __button - button to be destroyed
 * @return zero on success, non-zero on failure
 */
static int
button_destructor                 (w_button_t *__button)
{
  if (!__button)
    return -1;

  if (__button->caption)
    free (__button->caption);

  // Destroy screen layout
  if (WIDGET_LAYOUT (__button))
      scr_destroy_window (WIDGET_LAYOUT (__button));

  free (__button);
  return 0;
}

/**
 * Draws a button
 *
 * @param __button - button to be drawn
 * @return zero on success, non-zero on failure
 */
static int
button_drawer                     (w_button_t *__button)
{
  scr_window_t layout=WIDGET_LAYOUT (__button);

  // Widget is invisible or there is no layout
  if (!WIDGET_VISIBLE (__button) || !layout)
    return -1;

  scr_wnd_attr_backup (layout);

  scr_wnd_move_caret (layout, __button->position.x, __button->position.y);

  // Draw caption of button
  if (__button->caption)
    {
      scr_wnd_font (layout, __button->focused?
        *__button->focused_font:
        *__button->font);

      scr_wnd_printf (layout, "[%s", __button->style&WBS_DEFAULT?"< ":" ");

      widget_shortcut_print (layout, __button->caption,
        __button->focused?*__button->focused_font:*__button->font,
        __button->focused?*__button->hot_focused_font:*__button->hot_font);

      scr_wnd_printf (layout, "%s]", __button->style&WBS_DEFAULT?" >":" ");
    }

  scr_wnd_attr_restore (layout);

  return 0;
}

/**
 * Handles a keydown callback
 *
 * @param __button - button received a callback
 * @param __ch - received character
 * @return zero if callback hasn't handled received character
 */
static int
button_keydown                    (w_button_t *__button, wint_t __ch)
{
  _WIDGET_CALL_USER_CALLBACK (__button, keydown, __button, __ch);

  switch (__ch)
    {
    case KEY_RETURN:

      _WIDGET_CALL_USER_CALLBACK (__button, clicked, __button);

      if (WIDGET_BUTTON_MODALRESULT (__button))
        {
          widget_t *cur=__button->parent;
          while (cur && WIDGET_TYPE (cur)!=WT_WINDOW)
            cur=cur->parent;
          if (cur)
            w_window_end_modal (WIDGET_WINDOW (cur),
              WIDGET_BUTTON_MODALRESULT (__button));
          return TRUE;
        }
      return FALSE;
    default:
      return FALSE;
    }
}

 /**
  * Handles a shortcut callback
  *
  * @param __button - button for which this callback was sent
  * @return zero if callback hasn't handled received character,
  *   non-zero otherwise
  */
static int
button_shortcut                   (w_button_t *__button)
{
  // Try to call user's handler of shortcut..
  _WIDGET_CALL_USER_CALLBACK (__button, shortcut, __button);

  // .. if we are still here emulate clicking in the button
  widget_set_focus (WIDGET (__button));
  return button_keydown (__button, KEY_RETURN);
}

//////
// User's backend

/**
 * Creates new button on parent and specified with caption, position and style
 *
 * @param __parent - parent of button. Should be CONTAINER
 * @param __caption - caption on button
 * @param __x, __y - coordinates of button
 * @param __style of button. May be one or combination of:
 *   WBS_DEFAULT - default button on window
 * @return a pointer to button object
 */
w_button_t*
widget_create_button              (w_container_t *__parent,
                                   const wchar_t *__caption,
                                   int __x, int __y,
                                   unsigned int __style)
{
  w_button_t *res;

  // There is no parent or caption is null, so we can't create button
  if (!__parent || !__caption)
    return 0;

  unsigned int w;

  w=(__caption?widget_shortcut_length (__caption):0)+
    4+(__style&WBS_DEFAULT?2:0);

  WIDGET_INIT (res, w_button_t, WT_BUTTON, __parent, WF_NOLAYOUT,
               button_destructor, button_drawer,
               __x, __y, 1, w, 1);

  // Set callbacks
  WIDGET_CALLBACK (res, keydown)  = (widget_keydown_proc)button_keydown;
  WIDGET_CALLBACK (res, shortcut) = (widget_action)button_shortcut;

  WIDGET_SHORTCUT (res)=widget_shortcut_key (__caption);

  res->caption=wcsdup (__caption);

  res->style=__style;

  res->font         = &FONT (CID_BLACK, CID_WHITE);
  res->focused_font = &FONT (CID_BLACK, CID_CYAN);

  res->hot_font         = &FONT (CID_BLUE, CID_WHITE);
  res->hot_focused_font = &FONT (CID_BLUE, CID_CYAN);

  WIDGET_POST_INIT (res);

  return res;
}

/**
 * Sets fonts used in button
 *
 * @param __button - for which button fonts are to be set
 * @param __font - font of default text in normal state
 * @param __focused_font - font of default text in focused state
 * @param __font - font of highlighted text (i.e. shortcut) in normal state
 * @param __focused_font - font of highlighted text
 * (i.e. shortcut) in focused state
 */
void
w_button_set_fonts                (w_button_t *__button,
                                   scr_font_t *__font,
                                   scr_font_t *__focused_font,
                                   scr_font_t *__hot_font,
                                   scr_font_t *__hot_focused_font)
{
  if (!__button)
    return;

  WIDGET_SAFE_SET_FONT (__button, font,             __font);
  WIDGET_SAFE_SET_FONT (__button, focused_font,     __focused_font);
  WIDGET_SAFE_SET_FONT (__button, hot_font,         __hot_font);
  WIDGET_SAFE_SET_FONT (__button, hot_focused_font, __hot_focused_font);

  widget_redraw (WIDGET (__button));
}
