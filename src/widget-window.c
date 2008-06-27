/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation file for widget `window`
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "widget.h"
#include <malloc.h>

///////
//

#define SHORTCUT_CHECKER(_w, _shortcut_key) \
  if (WIDGET_SHORTCUT (_w)==_shortcut_key) \
    { \
      if (WIDGET_CALLBACK (_w, shortcut)) \
        return WIDGET_CALLBACK (_w, shortcut) (_w); else \
        break; \
    }

#define CENTRE_X(_width)    ((SCREEN_WIDTH-_width)/2)
#define CENTRE_Y(_height)   ((SCREEN_HEIGHT-_height)/2)

///////
//

/**
 * Destroys a window widget
 *
 * @param __window - window to be destroyed
 * @return zero on success, non-zero on failure
 */
static int
window_destructor                 (w_window_t *__window)
{
  if (!__window)
    return -1;

  // Hide window to reduce blinks
  w_window_hide (__window);

  // Call deleter inherited from container
  WIDGET_CONTAINER_DELETER (__window);

  // Delete panel associated with layout
  if (__window->panel)
    panel_del (__window->panel);

  // Destroy screen layout
  if (WIDGET_LAYOUT (__window))
      scr_destroy_window (WIDGET_LAYOUT (__window));

  if (__window->caption.text)
    free (__window->caption.text);

  free (__window);

  return 0;
}

/**
 * Draws a window
 *
 * @param __window - a window to be drawn
 * @return zero on success, non-zero on failure
 */
static int
window_drawer                     (w_window_t *__window)
{
  if (!WIDGET_VISIBLE (__window) || !WIDGET_LAYOUT (__window))
    return -1;

  scr_window_t layout=WIDGET_LAYOUT (__window);

  scr_wnd_bkg (layout, *__window->font);

  scr_wnd_border (layout);

  if (__window->caption.text)
    {
      // Draw caption
      scr_wnd_move_caret (layout,
        (__window->position.width-wcslen (__window->caption.text)-4)/2, 0);
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

/**
 * Handles a keydown callback
 *
 * @param __window - window received a callback
 * @param __ch - received character
 * @return zero if callback hasn't handled received character
 *   non-zero otherwise
 */
static int
window_keydown                    (w_window_t *__window, wint_t __ch)
{
  widget_t *focused;

  // Call user's callback
  _WIDGET_CALL_USER_CALLBACK (__window, keydown, __window, __ch);

  // If user's callback hadn't processed this callback,
  // make this stuff

  if ((focused=__window->focused_widget))
    {
      // If there is focused widget, try to redirect callback to it
      int res=0;
      if (WIDGET_CALLBACK (focused, keydown) &&
          (res=WIDGET_CALLBACK (focused, keydown) (focused, __ch)))
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
        WIDGET_CONTAINER_ACTION_ITERONLY (__window, SHORTCUT_CHECKER,
          towlower (__ch));
        break;
      }
    }

  return FALSE;
}

/**
 * Main window procedure which caughting messages from user and
 * manipulates with them
 *
 * @param __window - window for which messages from user should be caughted
 */
static int
window_proc                       (w_window_t *__window)
{
  wint_t ch;
  BOOL finito;
  // scr_window_t layout=WIDGET_LAYOUT (__window);

  // For caughting of all function keys
  // scr_wnd_keypad (layout, TRUE);
  for (;;)
    {
      finito=FALSE;

      // Wait for next character from user
      ch=scr_wnd_getch (0);

      // Try to manage ch by common keydown callback
      if (!window_keydown (__window, ch))
        {
          switch (ch)
            {
            case KEY_ESC:
            case KEY_ESC_ESC:
              // If window is modal, then we can close it when users hits `Esc`
              if (__window->show_mode&WSM_MODAL)
                {
                  // Suggest that non-zero modal_result means that
                  // somebody wants to return this modal_result
                  // instead of MR_CANCEL
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

/**
 * Default part of window showing stuff
 *
 * @param __window - window to be shown
 * @param _show_mode - mode in which window have to be shown.
 *   Possible values:
 *     WSM_NORLAM - Normal window
 *     WSM_MODAL  - Modal window
 * @return zero or modal result of window on success.
 *   Less-zero value on failure.
 */
static int
window_show_entry                 (w_window_t *__window, int __show_mode)
{
  if (!__window)
     return -1;

  // Window is now visible
  WIDGET_POSITION (__window).z=1;

  panel_show (__window->panel);

  widget_add_root (WIDGET (__window));

  __window->show_mode    = __show_mode;
  __window->modal_result = MR_NONE;

  // widget_set_current_widget (WIDGET (__window));

  // Is it okay if we think that showing of window
  // is the same as focusing of window?
  WIDGET_CALL_CALLBACK (__window, focused, __window);

  // Set focus to first widget in window
  if (WIDGET_CONTAINER_LENGTH (__window) &&
      !WIDGET_CONTAINER_FOCUSED (__window))
    widget_set_focus (WIDGET_CONTAINER_DATA (__window)[0]);

  // Draw window
  widget_redraw (WIDGET (__window));

  if (__show_mode==WSM_MODAL)
    {
      widget_t *w;
      window_proc (__window);

      if ((w=__window->focused_widget))
        WIDGET_CALL_CALLBACK (w, blured, w);

      w_window_hide (__window);
      return __window->modal_result;
    }

  return 0;
}

static void
centre_window                     (w_window_t *__window)
{
  if (!__window)
    return;

  __window->position.x=CENTRE_X (__window->position.width);
  __window->position.y=CENTRE_Y (__window->position.height);
}

/**
 * Callback for onresize action
 *
 * @param __window - window which caughted this event
 * @return zero if callback hasn't handled callback
 */
static int
window_onresize                   (w_window_t *__window)
{
  if (!__window)
    return -1;

  _WIDGET_CALL_USER_CALLBACK (__window, onresize, __window);

  if (__window->style&WMS_CENTERED)
    centre_window (__window);

  // There is no window's resising stuff, so
  // we have to create new window and use it
  scr_window_t oldwnd = WIDGET_LAYOUT (__window),
               newwnd = widget_create_layout (WIDGET (__window));

  WIDGET_LAYOUT (__window)=newwnd;
  panel_replace (__window->panel, newwnd);
  panels_doupdate ();

  BOOL locked=WIDGET_TEST_FLAG (__window, WF_REDRAW_LOCKED);

  if (!locked)
    widget_lock_redraw (WIDGET (__window));

  WIDGET_CONTAINER_ACTION_ITERONLY (__window, WIDGET_CALL_CALLBACK,
    onresize, __iterator_);

  if (!locked)
    {
      widget_unlock_redraw (WIDGET (__window));
      widget_redraw (WIDGET (__window));
    }

  if (oldwnd)
    scr_destroy_window (oldwnd);

  return TRUE;
}

//////
// User's backend

/**
 * Creates new window with specified caption and position
 *
 * @param __caption - caption of window
 * @param __x, __y - coordinates of window
 * @param __w, __h - width and height of window
 * @return a pointer to window object
 */
w_window_t*
widget_create_window              (const wchar_t *__caption,
                                   int __x, int __y, int __w, int __h,
                                   unsigned int __style)
{
  w_window_t *res;

  if (__style&WMS_CENTERED)
    {
      __x=CENTRE_X (__w);
      __y=CENTRE_Y (__h);
    }

  WIDGET_INIT (res, w_window_t, WT_WINDOW, 0, 0,
               window_destructor, window_drawer, \
               __x, __y, 0, __w, __h);

  if (__caption)
    res->caption.text=wcsdup (__caption);

  res->panel=panel_new (res->layout);

  res->style=__style;

  // Layout parameters
  res->font         = &FONT (CID_BLACK, CID_WHITE);
  res->caption.font = &FONT (CID_BLUE,  CID_WHITE);

  WIDGET_CALLBACK (res, keydown)  = (widget_keydown_proc)window_keydown;
  WIDGET_CALLBACK (res, onresize) = (widget_action)window_onresize;

  return res;
}

/**
 * Shows modal window
 *
 * @param __window - window to be shown modally
 * @return modal result
 */
int
w_window_show_modal               (w_window_t *__window)
{
  return window_show_entry (__window, WSM_MODAL);
}

/**
 * Shows window
 *
 * @param __window - window to be shown
 */
void
w_window_show                     (w_window_t *__window)
{
  if (!__window || !WIDGET_LAYOUT (__window))
    return;

  // Window is now visible
  WIDGET_POSITION (__window).z=1;

  panel_show (__window->panel);

  WIDGET_CALL_CALLBACK (__window, focused, __window);
}

/**
 * Hides window
 *
 * @param __window - window to be hided
 */
void
w_window_hide                     (w_window_t *__window)
{
  if (!__window || !WIDGET_LAYOUT (__window))
    return;

  widget_delete_root (WIDGET (__window));

  WIDGET_CALL_CALLBACK (__window, blured, __window);

  // Window is now invisible
  WIDGET_POSITION (__window).z=0;

  panel_hide (__window->panel);
}

/**
 * Sets font of window
 *
 * @param __window - for which window change fonts
 * @param __font - font determines background of window and
 *   color of lonely text on window
 * @param __caption_font - font for caption
 */
void
w_window_set_fonts                (w_window_t *__window,
                                   scr_font_t *__font,
                                   scr_font_t *__caption_font)
{
  if (!__window)
    return;

  WIDGET_SAFE_SET_FONT (__window, font,         __font);
  WIDGET_SAFE_SET_FONT (__window, caption.font, __caption_font);

  widget_redraw (WIDGET (__window));
}

////
// Deep-core stuff

/**
 * Closes modally shown window
 *
 * @param __window - window to be closed
 */
void
w_window_end_modal                (w_window_t *__window, int __modal_result)
{
  __window->modal_result=__modal_result;
  scr_ungetch (KEY_ESC);
}
