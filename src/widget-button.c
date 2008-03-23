/*
 *
 * ================================================================================
 *  widget-button.c
 * ================================================================================
 *
 *  Implementation file for widget `button`
 *
 *  Written (by Nazgul) under General Public License.
 *
*/

#include "widget.h"

//////
//

static int
button_destructor                 (w_button_t *__button)
{
  if (__button->caption)
    free (__button->caption);
  return 0;
}

static int
button_drawer                     (w_button_t *__button)
{
  // Inherit layout from parent
  scr_window_t layout=WIDGET_LAYOUT (__button)=WIDGET_LAYOUT (__button->parent);

  // Widget is invisible or there is no layout
  if (!WIDGET_VISIBLE (__button) || !layout)
    return 0;

  scr_wnd_attr_backup (layout);
    
  scr_wnd_move_caret (layout, __button->position.x, __button->position.y);

  if (__button->caption)
    {
      scr_wnd_font (layout, __button->focused?*__button->focused_font:*__button->font);

      scr_wnd_printf (layout, "[%s", __button->style&WBS_DEFAULT?"< ":" ");

      widget_shortcut_print (layout, __button->caption,
        __button->focused?*__button->focused_font:*__button->font,
        __button->focused?*__button->hot_focused_font:*__button->hot_font);

      scr_wnd_printf (layout, "%s]", __button->style&WBS_DEFAULT?" >":" ");
    }

  scr_wnd_attr_restore (layout);

  return 0;
}

static int
button_keydown                    (w_button_t *__button, int __ch)
{
  WIDGET_CALL_USER_CALLBACK (__button, keydown, __button, __ch);
  
  switch (__ch)
    {
    case KEY_RETURN:

      WIDGET_CALL_USER_CALLBACK (__button, clicked, __button);

      if (WIDGET_BUTTON_MODALRESULT (__button))
        {
          widget_t *cur=__button->parent;
          while (cur && WIDGET_TYPE (cur)!=WT_WINDOW)
            cur=cur->parent;
          if (cur)
            w_window_end_modal (WIDGET_WINDOW (cur), WIDGET_BUTTON_MODALRESULT (__button));
          return TRUE;
        }
      return FALSE;
    default:
      return FALSE;
    }
}

static int
button_shortcut                   (w_button_t *__button)
{
  // Try to call user's handler of shortcut..
  WIDGET_CALL_USER_CALLBACK (__button, shortcut, __button);

  // .. if we are still here emulate clicking in the button
  button_keydown (__button, KEY_RETURN);
  
  return 0;
}

//////
// User's backend

w_button_t*
widget_create_button              (w_container_t *__parent, wchar_t *__caption,
                                   int __x, int __y, unsigned int __style)
{
  w_button_t *res;

  // Parent is not a contaier-based widget
  if (!WIDGET_IS_CONTAINER (__parent))
    return 0;

  // Allocate and free memory for new window
  MALLOC_ZERO (res, sizeof (w_button_t));

  res->type=WT_BUTTON;

  res->parent=WIDGET (__parent);

  if (WIDGET_IS_CONTAINER (__parent))
    res->tab_order=WIDGET_CONTAINER_LENGTH (__parent);

  // Set methods
  res->methods.destroy = (widget_action)button_destructor;
  res->methods.draw    = (widget_action)button_drawer;

  WIDGET_CALLBACK (res, keydown)  = (widget_keydown)button_keydown;
  WIDGET_CALLBACK (res, shortcut) = (widget_action)button_shortcut;

  WIDGET_SHORTCUT (res)=widget_shortcut_key (__caption);

  if (__caption)
    res->caption=wcsdup (__caption);

  res->style=__style;

  res->position.x      = __x;
  res->position.y      = __y;
  res->position.z      = 1;
  res->position.width  = (__caption?widget_shortcut_length (__caption):0)+4+(__style&WBS_DEFAULT?2:0);
  res->position.height = 1;

  res->font         = &sf_black_on_white;
  res->focused_font = &sf_black_on_cyan;

  res->hot_font         = &sf_blue_on_white;
  res->hot_focused_font = &sf_blue_on_cyan;

  // Register widget in container
  w_container_append_child (__parent, WIDGET (res));
  
  return res;
}

void
w_button_set_fonts                (w_button_t *__button,
                                   scr_font_t *__font,     scr_font_t *__focused_font,
                                   scr_font_t *__hot_font, scr_font_t *__hot_focused_font)
{
  if (!__button)  return;

  WIDGET_SAFE_SET_FONT (__button, font,             __font);
  WIDGET_SAFE_SET_FONT (__button, focused_font,     __focused_font);
  WIDGET_SAFE_SET_FONT (__button, hot_font,         __hot_font);
  WIDGET_SAFE_SET_FONT (__button, hot_focused_font, __hot_focused_font);
}
