/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Common widgets' stuff
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "widget.h"

#include <malloc.h>

////
//

#define CONTAINER_REALLOC_DELTA 8

// static widget_t *current_widget=NULL;

////
// Deep-core stuff

/**
 * Gets next/prev focused widget
 *
 * @param __widget - container in which we want to find
 *   next/prev focused widget
 * @param __dir - direction of searching.
 *   1 - get next widget
 *  -1 - get previous widget
 *   Please, do not use other values
 * @return - next/prev widget
 */
static widget_t*
get_focused_entry                 (const widget_t *__widget, short __dir)
{
  int index;
  if (!__widget || !__widget->parent || !WIDGET_IS_CONTAINER (__widget->parent))
    return NULL;

  return w_container_widget_by_tab_order (WIDGET_CONTAINER (__widget->parent),
    ((index=__widget->tab_order+__dir)>=0?
      index:
      WIDGET_CONTAINER_LENGTH (__widget->parent)-1)%
        WIDGET_CONTAINER_LENGTH (__widget->parent));
}

// Code to operate with non-modal windows
/*void
widget_set_current_widget         (widget_t *__widget)
{
  current_widget=__widget;
}*/

/*void           // Main loop of widget stuff to manipulate with messages from user
widget_main_loop                  (void)
{
  int ch;
  scr_window_t layout;
  scr_window_t root_wnd=screen_root_wnd ();

  for (;;)
    {
      layout=current_widget&&WIDGET_LAYOUT (current_widget)?WIDGET_LAYOUT (current_widget):root_wnd;

      scr_wnd_keypad (layout, TRUE);
      ch=scr_wnd_getch (layout);

      // Redirect calls to current widget
      if (current_widget && WIDGET_CALLBACK (current_widget, keydown))
        WIDGET_CALLBACK (current_widget, keydown) (current_widget, ch);
    }
}*/

//////
//

/**
 *  Destructor of any widget
 *
 * @param __widget - widget to be destroyed
 */
void
widget_destroy                    (widget_t *__widget)
{
  if (!__widget)
    return;

  // Call destructor from object
  if (__widget->methods.destroy)
    __widget->methods.destroy (__widget);
}

/**
 * Draws widget on screen
 *
 * @param __widget - widget to bew drawn
 */
int
widget_draw                       (widget_t *__widget)
{
  if (!__widget || !__widget->methods.draw)
    return -1;

  return __widget->methods.draw (__widget);
}

/**
 * Redraws widget on screen
 *
 * @param __widget - widget to be redrawn
 */
void
widget_redraw                     (widget_t *__widget)
{
  if (!__widget)
    return;

  widget_draw (__widget);
  scr_wnd_invalidate (WIDGET_LAYOUT (__widget));
  scr_wnd_refresh (WIDGET_LAYOUT (__widget));
}

/**
 * Totally redraws all widgets
 */
void
widget_full_redraw                (void)
{
  update_panels ();
  scr_doupdate ();
}

/**
 * Call this method when root screen has been resized
 */
void
widget_on_scr_resize              (void)
{
  scr_clear ();
  widget_full_redraw ();
}

/**
 *  Sets focus to widget
 *
 * @param __widget - widget you want to set focus to
 */
void
widget_set_focus                  (widget_t *__widget)
{
  if (!__widget)
    return;

  // Update previois focused widget widget
  if (__widget->parent && WIDGET_IS_CONTAINER (__widget->parent))
    {
      w_container_t *cnt=WIDGET_CONTAINER (__widget->parent);
      if (cnt->focused_widget)
        {
          widget_t *w=cnt->focused_widget;
          w->focused=FALSE;

          // Blure da previous focused widget
          WIDGET_CALL_CALLBACK (w, blured, w);

          widget_draw (w);
        }
      cnt->focused_widget=__widget;
    }

  // Set focus to new widget and redraw
  __widget->focused=TRUE;

  if (!WIDGET_CALL_CALLBACK (__widget, focused, __widget))
    // We have to redraw widget only if widget-defined
    // callback returned zero-code
    widget_redraw (__widget);
}

////
// Different stuff

/**
 * Extracts shortcut key from etxt
 *
 * @param __text - text from which you want to extract hotkey
 * @return extracted shortcut
 */
wchar_t
widget_shortcut_key               (const wchar_t *__text)
{
  int i, n;

  if (!__text)
    return 0;

  for (i=0, n=wcslen (__text); i<n; ++i)
     if (__text[i]=='_' && i<n-1)
      {
        i++;
        if (!iswspace (__text[i]))
          return towlower (__text[i]);
      }
  return 0;
}

/**
 * Returns length of text with shortcuts
 *
 * @param __text - text whose length ypu want to calculate
 * @return length of text
 */
int
widget_shortcut_length            (const wchar_t *__text)
{
  int len=0, i, n;

  if (!__text)
    return 0;

  for (i=0, n=wcslen (__text); i<n; ++i)
    {
      if (__text[i]=='_' && i<n-1)
        {
          i++;
          if (iswspace (__text[i]))
            len++;
        }
      len++;
    }

  return len;
}

/**
 * Print text with highlighted shortcut key
 *
 * @param __layout - layout where text have to be printed
 * @param __text - text to be printed
 * @param __font - default font of text
 * @param __hot_font - font of shortcutted character
 */
void
widget_shortcut_print             (scr_window_t __layout,
                                   const wchar_t *__text,
                                   scr_font_t __font, scr_font_t __hot_font)
{
  int i, n;
  BOOL hot=FALSE, hot_printed=FALSE;

  if (!__text)
    return;

  scr_wnd_attr_backup (__layout);

  scr_wnd_font (__layout, __font);

  for (i=0, n=wcslen (__text); i<n; ++i)
    {
      if (__text[i]=='_' && i<n-1)
        {
          if (!iswspace (__text[i+1]))
            {
              hot=!hot_printed;
              i++;
            }
        }

      if (hot)
        scr_wnd_font (__layout, __hot_font);

      scr_wnd_add_wchar (__layout, __text[i]);

      if (hot) 
        {
          scr_wnd_font (__layout, __font);
          hot=FALSE;
          hot_printed=TRUE;
        }
    }

  scr_wnd_attr_restore (__layout);
}

/**
 * Returns next widget avaliable to be focused
 *
 * @param __widget - container of widgets where search of such widget
 *   have to be started
 * @return wanted widget
 */
widget_t *
widget_next_focused               (const widget_t *__widget)
{
  return get_focused_entry (__widget, 1);
}

/**
 * Returns previous widget avaliable to be focused
 *
 * @param __widget - container of widgets where search of such widget
 *   have to be started
 * @return wanted widget
 */
widget_t *
widget_prev_focused               (const widget_t *__widget)
{
  return get_focused_entry (__widget, -1);
}

////
// Container

/**
 * Appends widget to container
 *
 * @param __container - container where widget have to be added
 * @param __widget - widget which you want to add to container
 */
void
w_container_append_child          (w_container_t *__container,
                                   widget_t *__widget)
{
  // `__container` is not a contaier-based widget
  // or a `__widget` is NULL
  if (!WIDGET_IS_CONTAINER (__container) || !__widget)
    return;
  
  // Need to make array a bit bigger
  if (__container->container.length>=__container->container.alloc_length)
    {
      __container->container.alloc_length+=CONTAINER_REALLOC_DELTA;
      __container->container.data=realloc (__container->container.data,
        __container->container.alloc_length*sizeof (widget_t*));
    }
  
  __container->container.data[__container->container.length++]=__widget;
}

/**
 * Returns widget with specified tab order from container
 *
 * @param __container - container from which you want to get widget
 * @param __tab_order - tab order of widget you want to get
 * @return widget with specified tab order or NULL if there is no widget
 *   with such tab order
 */
widget_t*
w_container_widget_by_tab_order   (const w_container_t *__container,
                                   int __tab_order)
{
  int i, n;
  // `__container` is not a contaier-based widget
  if (!WIDGET_IS_CONTAINER (__container))
    return NULL;
  
  for (i=0, n=WIDGET_CONTAINER_LENGTH (__container); i<n; i++)
    if (WIDGET_CONTAINER_DATA (__container)[i] &&
        WIDGET_CONTAINER_DATA (__container)[i]->tab_order==__tab_order)
      return WIDGET_CONTAINER_DATA (__container)[i];

  return NULL;
}

/**
 * Handler of `focused` callback (system-based)
 *
 * @param __widget - widget which caucghted this callback
 * @return zero if callback hasn't handled callback
 */
int
widget_focused                    (widget_t *__widget)
{
  _WIDGET_CALL_USER_CALLBACK (__widget, focused, __widget);
  return 0;
}

/**
 * Handler of `blured` callback (system-based)
 *
 * @param __widget - widget which caucghted this callback
 * @return zero if callback hasn't handled callback
 */
int
widget_blured                     (widget_t *__widget)
{
  _WIDGET_CALL_USER_CALLBACK (__widget, blured, __widget);
  return 0;
}
