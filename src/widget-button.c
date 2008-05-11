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

  free (__button);
  return 0;
}

/**
 * Draws a button
 *
 * @param __button - button to be drawed
 * @return zero on success, non-zero on failure
 */
static int
button_drawer                     (w_button_t *__button)
{
  // Inherit layout from parent
  scr_window_t layout=WIDGET_LAYOUT (__button)=
    WIDGET_LAYOUT (__button->parent);

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
  return button_keydown (__button, KEY_RETURN);
}

//////
// User's backend

/**
 * Creates new button on parent and specified with caption, position and style
 *
 * @param __parent - parent of button. Shoudld be CONTAINER
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

  // Allocate and zerolize memory for new button
  MALLOC_ZERO (res, sizeof (w_button_t));

  res->type=WT_BUTTON;

  res->parent=WIDGET (__parent);

  if (WIDGET_IS_CONTAINER (__parent))
    res->tab_order=WIDGET_CONTAINER_LENGTH (__parent);

  // Set methods
  res->methods.destroy = (widget_action)button_destructor;
  res->methods.draw    = (widget_action)button_drawer;

  // Set callbacks
  WIDGET_CALLBACK (res, keydown)  = (widget_keydown)button_keydown;
  WIDGET_CALLBACK (res, shortcut) = (widget_action)button_shortcut;
  WIDGET_CALLBACK (res, focused)  = (widget_action)widget_focused;
  WIDGET_CALLBACK (res, blured)   = (widget_action)widget_blured;

  WIDGET_SHORTCUT (res)=widget_shortcut_key (__caption);

  res->caption=wcsdup (__caption);

  res->style=__style;

  res->position.x      = __x;
  res->position.y      = __y;
  res->position.z      = 1;
  res->position.width  = (__caption?widget_shortcut_length (__caption):0)+
    4+(__style&WBS_DEFAULT?2:0);
  res->position.height = 1;

  res->font         = &sf_black_on_white;
  res->focused_font = &sf_black_on_cyan;

  res->hot_font         = &sf_blue_on_white;
  res->hot_focused_font = &sf_blue_on_cyan;

  // Register widget in container
  w_container_append_child (__parent, WIDGET (res));

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
