/*
 *
 * ================================================================================
 *  widget-window.c
 * ================================================================================
 *
 *  Implementation file for widget `window`
 *
 *  Written (by Nazgul) under General Public License.
 *
*/

#include "widget.h"

#include <malloc.h>

///////
//

#define SHORTCUT_CHECKER(_w, _shortcut_key) \
  if (WIDGET_SHORTCUT (_w)==_shortcut_key) \
    { \
      if (WIDGET_CALLBACK (_w, shortcut)) \
        WIDGET_CALLBACK (_w, shortcut) (_w); else \
        break; \
    }

///////
//

static int
window_destructor                 (w_window_t *__window)
{
  if (!__window)
    return 0;

  // Destroy screen layout
  if (WIDGET_LAYOUT (__window))
      scr_destroy_window (WIDGET_LAYOUT (__window));

  if (__window->caption.text)
    free (__window->caption.text);
  
  // Call deter inherited from container
  WIDGET_CONTAINER_DELETER (__window);

  free (__window);

#ifdef WIDGET_USE_POOL
  widget_unregister_from_pool (WIDGET (__window));
#endif

  // Need this because some strange troubles with refreshing screen.
  // Actually, not all needed windows are refshed when we calls scr_refresh()
  widget_full_redraw ();

  return 0;
}

static int
window_drawer                     (w_window_t *__window)
{
  if (!WIDGET_VISIBLE (__window))
    return 0;

  scr_window_t layout=WIDGET_LAYOUT (__window);

  scr_wnd_bkg (layout, *__window->font);

  scr_wnd_border (layout);

  if (__window->caption.text)
    {
      // Draw caption
      scr_wnd_move_caret (layout, (__window->position.width-wcslen (__window->caption.text)-4)/2, 0);
      scr_wnd_putch (layout, CH_RTEE);

      scr_wnd_font   (layout, *__window->caption.font);
      scr_wnd_printf (layout, " %ls ", __window->caption.text);
      scr_wnd_font   (layout, *__window->font);

      scr_wnd_putch (layout, CH_LTEE);
    }

  // Call drawer inherited from container
  WIDGET_CONTAINER_DRAWER (__window);

  return 0;
}

static int
window_keydown                    (w_window_t *__window, int __ch)
{
  widget_t *focused;

  // Call to user's callback
  WIDGET_CALL_USER_CALLBACK (__window, keydown, __window, __ch);

  // If user's callback hadn't processed this callback,
  // make this stuff

  if ((focused=__window->focused_widget))
    {
      // If there is focused widget, try to redirect callback to it
      int res=0;
      if (WIDGET_CALLBACK (focused, keydown) && (res=WIDGET_CALLBACK (focused, keydown) (focused, __ch)))
        return res;
    }

  switch (__ch)
    {
    // Navigation
    case KEY_DOWN:
    case KEY_RIGHT:
    case KEY_TAB:
      widget_set_focus (widget_next_focused (__window->focused_widget));
      return TRUE;

    case KEY_UP:
    case KEY_LEFT:
      widget_set_focus (widget_prev_focused (__window->focused_widget));
      return TRUE;

    default:
      {
        WIDGET_CONTAINER_ACTION_ITERONLY (__window, SHORTCUT_CHECKER, towlower (__ch));
        break;
      }
    }

  return FALSE;
}

static int
window_proc                       (w_window_t *__window)
{
  wint_t ch;
  BOOL finito;
  scr_window_t layout=WIDGET_LAYOUT (__window);

  // For caughting of all function keys
  scr_wnd_keypad (layout, TRUE);
  for (;;)
    {
      finito=FALSE;

      // Wait for next character from user
      ch=scr_wnd_getch (layout);

      // Try to manage ch by common keydown callback
      if (!window_keydown (__window, ch))
        {
          switch (ch)
            {
            case KEY_ESC:
              // If window is modal, then we can close it when users hits `Esc`
              if (__window->show_mode&WSM_MODAL)
                {
                  // Suggest that non-zero modal_result means that
                  // somebody wants to return this modal_result instead of MR_CANCEL
                  if (!__window->modal_result)
                    __window->modal_result=MR_CANCEL;
                  finito=TRUE;
                }
              break;
            }
        }

      if (finito)
        break;
    }

  return 0;
}

static int
window_show_entry                 (w_window_t *__window, int __show_mode)
{
   if (!__window)
     return -1;

   widget_position_t pos=WIDGET_POSITION (__window); // For small time optimization

   // Window is now visible
   WIDGET_POSITION (__window).z=1;

  __window->show_mode    = __show_mode;
  __window->modal_result = MR_NONE;

  // widget_set_current_widget (WIDGET (__window));

  // Create layout for window
  __window->layout=scr_create_window (pos.x, pos.y, pos.width, pos.height);

  // Set focus to first widget in window
  if (WIDGET_CONTAINER_LENGTH (__window) && !WIDGET_CONTAINER_FOCUSED (__window))
    widget_set_focus (WIDGET_CONTAINER_DATA (__window)[0]);

  // Draw window
  widget_draw (WIDGET (__window));

  if (__show_mode==WSM_MODAL)
    {
      window_proc (__window);
      w_window_hide (__window);
      return __window->modal_result;
    }

  return 0;
}

//////
// User's backend

w_window_t*    // Creates new window
widget_create_window              (wchar_t *__caption, int __x, int __y, int __w, int __h)
{
  w_window_t *res;

  // Allocate and free memory for new window
  MALLOC_ZERO (res, sizeof (w_window_t));

  res->type=WT_WINDOW;

  // Set methods
  res->methods.destroy = (widget_action)window_destructor;
  res->methods.draw    = (widget_action)window_drawer;

  if (__caption)
    res->caption.text=wcsdup (__caption);

  //res->layout=scr_create_window (__x, __y, __w, __h);

  res->position.x      = __x;
  res->position.y      = __y;
  res->position.width  = __w;
  res->position.height = __h;

  // Layout parameters
  res->font         = &sf_black_on_white;
  res->caption.font = &sf_blue_on_white;

  WIDGET_CALLBACK (res, keydown)=(widget_keydown)window_keydown;

#ifdef WIDGET_USE_POOL
  widget_register_in_pool (WIDGET (res));
#endif

  return res;
}

/*void           //  Show normal window
w_window_show                     (w_window_t *__window)
{
  window_show_entry (__window, WSM_DEFAULT);
}*/

int            //  Show modal window
w_window_show_modal               (w_window_t *__window)
{
  return window_show_entry (__window, WSM_MODAL);
}

void           // Hide window
w_window_hide                     (w_window_t *__window)
{
  if (!__window || !WIDGET_LAYOUT (__window))
    return;

  // Window is now unvisible
  WIDGET_POSITION (__window).z=0;

  // widget_set_current_widget (__window->w_prev_callback);

  // Destroy layout for window
  scr_destroy_window (WIDGET_LAYOUT (__window));
  WIDGET_LAYOUT (__window)=0;

  // Redraw all windows
  widget_full_redraw ();
}

void           // Returns window from hidden state
w_window_display                  (w_window_t *__window)
{
   widget_position_t pos=WIDGET_POSITION (__window); // For small optimization

   // Window is now visible
   WIDGET_POSITION (__window).z=1;

  // Create layout for window
   __window->layout=scr_create_window (pos.x, pos.y, pos.width, pos.height);

  // Redraw all windows
  widget_full_redraw ();
}

void
w_window_set_fonts                (w_window_t *__window, scr_font_t *__font, scr_font_t *__caption_font)
{
  if (!__window)
    return;

  WIDGET_SAFE_SET_FONT (__window, font,         __font);
  WIDGET_SAFE_SET_FONT (__window, caption.font, __caption_font);
}

////
// Deep-core stuff

void
w_window_end_modal                (w_window_t *__window, int __modal_result)
{
  __window->modal_result=__modal_result;
  scr_ungetch (KEY_ESC);
}
