/*
 *
 * ================================================================================
 *  widget.c
 * ================================================================================
 *
 *  Common widgets' stuff
 *
 *  Written (by Nazgul) under General Public License.
 *
*/

#include "widget.h"

#include <malloc.h>

////
//

//
// TODO:
//  Need this stuff because of incorrect screen refreshing
//  (when we cast refresh() full screen become black)
//  when we want to create a lot of windows in reqursion.
//

#ifdef WIDGET_USE_POOL
#  define WIDGETS_REALLOC_DELTA 8
static widget_t **widgets;
static int widgets_len=0, widgets_ptr=0;
#endif

#define CONTAINER_REALLOC_DELTA 8

// static widget_t *current_widget=NULL;

////
// Deep-core stuff

static widget_t*
get_focused_entry                 (widget_t *__widget, short __dir)
{
  int index;
  if (!__widget || !__widget->parent || !WIDGET_IS_CONTAINER (__widget->parent))
    return NULL;

  return w_container_widget_by_tab_order (WIDGET_CONTAINER (__widget->parent),
    ((index=__widget->tab_order+__dir)>=0?index:WIDGET_CONTAINER_LENGTH (__widget->parent)-1)%WIDGET_CONTAINER_LENGTH (__widget->parent));
}

#ifdef WIDGET_USE_POOL
void
widget_register_in_pool           (widget_t *__widget)
{
  if (widgets_ptr>=widgets_len)
    {
      widgets_len+=WIDGETS_REALLOC_DELTA;
      widgets=realloc (widgets, widgets_len*sizeof (widget_t*));
    }
  widgets[widgets_ptr++]=__widget;
}

void
widget_unregister_from_pool       (widget_t *__widget)
{
  int i;
  for (i=0; i<widgets_ptr; i++)
    if (widgets[i]==__widget)
      {
        int j;
        // Shift registered data
        for (j=i; j<widgets_ptr-1; j++)
          widgets[j]=widgets[j+1];
        widgets_ptr--;

        if (!widgets_ptr)
          {
            widgets_len=0;
            free (widgets);
          }

        break;
      }
}

void
widget_redraw_pool                (void)
{
  int i;

  for (i=0; i<widgets_ptr; i++)
    {
      touchwin (WIDGET_LAYOUT (widgets[i]));
      widget_draw (WIDGET (widgets[i]));
      wrefresh (WIDGET_LAYOUT (widgets[i]));
    }
}
#endif

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

void           // Totally destroing of widget
widget_destroy                    (widget_t *__widget)
{
  if (!__widget)
    return;

  // Call destructor from object
  if (__widget->methods.destroy)
    __widget->methods.destroy (__widget);
}

void           // Draw widget on screen
widget_draw                       (widget_t *__widget)
{
  if (!__widget || !__widget->methods.draw)
    return;

  __widget->methods.draw (__widget);
}

void           // Totally redraw all widgets
widget_full_redraw                (void)
{
  screen_refresh (TRUE);

#ifdef WIDGET_USE_POOL
  widget_redraw_pool ();
#endif
}

void           // Call this method when root screen has been resized
widget_on_scr_resize              (void)
{
  scr_clear ();
  widget_full_redraw ();
}

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
          cnt->focused_widget->focused=FALSE;
          widget_draw (cnt->focused_widget);
        }
      cnt->focused_widget=__widget;
    }

  // Set focus to new widget and redraw
  __widget->focused=TRUE;
  widget_draw (__widget);
}

////
// Different stuff

wchar_t       // Extracts shortcut key from etxt
widget_shortcut_key               (wchar_t *__text)
{
  int i, n;

  for (i=0, n=wcslen (__text); i<n; ++i)
     if (__text[i]=='_' && i<n-1)
      {
        i++;
        if (!iswspace (__text[i]))
          return towlower (__text[i]);
      }
  return 0;
}

int           // Length of text with shortcuts
widget_shortcut_length            (wchar_t *__text)
{
  int len=0, i, n;

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

void           // Print text with highlighted shortcut key
widget_shortcut_print             (scr_window_t __layout, wchar_t *__text, scr_font_t __font, scr_font_t __hot_font)
{
  int i, n;
  BOOL hot=FALSE, hot_printed=FALSE;

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

widget_t *
widget_next_focused               (widget_t *__widget)
{
  return get_focused_entry (__widget, 1);
}

widget_t *
widget_prev_focused               (widget_t *__widget)
{
  return get_focused_entry (__widget, -1);
}

////
// Container

void
w_container_append_child          (w_container_t *__container, widget_t *__widget)
{
  // `__container` is not a contaier-based widget
  // or a `__widget` is NULL
  if (!WIDGET_IS_CONTAINER (__container) || !__widget)
    return;
  
  // Need to make array a bit bigger
  if (__container->container.length>=__container->container.alloc_length)
    {
      __container->container.alloc_length+=CONTAINER_REALLOC_DELTA;
      __container->container.data=realloc (__container->container.data, __container->container.alloc_length);
    }
  
  __container->container.data[__container->container.length++]=__widget;
}

widget_t*
w_container_widget_by_tab_order   (w_container_t *__container, int __tab_order)
{
  int i, n;
  // `__container` is not a contaier-based widget
  if (!WIDGET_IS_CONTAINER (__container))
    return NULL;
  
  for (i=0, n=WIDGET_CONTAINER_LENGTH (__container); i<n; i++)
    if (WIDGET_CONTAINER_DATA (__container)[i] && WIDGET_CONTAINER_DATA (__container)[i]->tab_order==__tab_order)
      return WIDGET_CONTAINER_DATA (__container)[i];

  return NULL;
}
