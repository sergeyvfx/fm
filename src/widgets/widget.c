/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Common widgets' stuff
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "widget.h"
#include "deque.h"
#include "hook.h"
#include "messages.h"

#include <malloc.h>
#include <time.h>

/********
 *
 */

#define CONTAINER_REALLOC_DELTA 8

#define FOCUSABLE(_w) \
  (!WIDGET_TEST_FLAG (_w, WF_UNFOCUSABE))

#define DELAY (0.2*1000*10)

/* List of root widgets */
static deque_t *root_widgets = NULL;

/* Is list of root widget under destroying? */

/*
 * Need this flag because widgets_done() calls widget_destroy() for
 * all root widgets, and their destuctor may want to delete widget
 * from list of root widgets. This is not good idea, because
 * deque_destroy() will work incorrectly.
 */
static BOOL destroying_root_widgets = FALSE;

static BOOL main_loop_started = FALSE;
static BOOL stop_main_loop = FALSE;

/********
 * Deep-core stuff
 */

/***
 * Helpers for get_focused_entry
 */

/**
 * Recursively sinking in tree in specified direction
 *
 * @param __widget - current node of widget tree
 * @param __dir - direction of moving
 */
static widget_t*
get_neighbour_iter (const widget_t *__widget, short __dir)
{
  if (!__widget)
    {
      return 0;
    }

  if (!WIDGET_IS_CONTAINER (__widget))
    {
      return (widget_t*) __widget;
    }

  __dir = __dir < 0 ? -1 : 1;

  unsigned int i, length = WIDGET_CONTAINER_LENGTH (__widget);
  widget_t *widget, *cur_widget;

  i = __dir > 0 ? 0 : length - 1;

  while (i >= 0 && i < length)
    {
      cur_widget =
              w_container_widget_by_tab_order (WIDGET_CONTAINER (__widget), i);

      if ((widget = get_neighbour_iter (cur_widget, __dir)))
        {
          return widget;
        }

      i += __dir;
    }

  return 0;
}

/**
 * Search for branch from which searching of neighbour
 * may be started and starts this searching
 *
 * @param __widget - widget, describes current node of widget tree
 * @param __dir - direction of moving inside tree
 */
static widget_t*
get_neighbour (const widget_t *__widget, short __dir)
{
  if (!__widget || !__widget->parent ||
      !WIDGET_IS_CONTAINER (__widget->parent))
    {
      return 0;
    }

  __dir = __dir < 0 ? -1 : 1;

  w_container_t *parent = WIDGET_CONTAINER (__widget->parent);
  unsigned int i, length = WIDGET_CONTAINER_LENGTH (parent);
  widget_t *widget, *cur_widget;

  i = __widget->tab_order + __dir;

  /* Search for widget, from which we can */
  /* call recursively finding of searched widget */
  while (i >= 0 && i < length)
    {
      cur_widget = w_container_widget_by_tab_order (parent, i);

      if ((widget = get_neighbour_iter (cur_widget, __dir)))
        {
          return widget;
        }

      i += __dir;
    }

  /* Try to find in neighbor brunch of parent widget */
  return get_neighbour (__widget->parent, __dir);
}

/**
 * Get next/prev focused widget
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
get_focused_entry (const widget_t *__widget, short __dir)
{
  int index;
  w_container_t *cnt;
  unsigned int length;
  widget_t *widget, *base_widget;

  __dir = __dir < 0 ? -1 : 1;

  if (!__widget || !__widget->parent ||
      !WIDGET_IS_CONTAINER (__widget->parent))
    {
      return NULL;
    }

  cnt = WIDGET_CONTAINER (__widget->parent);
  length = WIDGET_CONTAINER_LENGTH (__widget->parent);
  index = __widget->tab_order + __dir;

  /* Try to get next widget in current container */
  while (index >= 0 && index < length)
    {
      /* Overview widgets in defined direction */
      /* until we meet focusable widget */

      widget=w_container_widget_by_tab_order (cnt, index);

      if (FOCUSABLE (widget))
        {
          /* Searched widget has been found */
          return widget;
        }

      index+=__dir;
    }

  /* Try to get searched widget from */
  /* neighbour branch of widget tree */
  base_widget=(widget_t*)__widget;
  for (;;)
    {
      if ((widget = get_neighbour (base_widget, __dir)))
        {
          if (FOCUSABLE (widget))
            {
              /* Searched widget has been found */
              return widget;
            }

          base_widget=widget;
        }
      else
        {
          /* There is no neighbour, which satisfies us */
          break;
        }
    }

  /* Cycling */
  while (cnt->parent && WIDGET_IS_CONTAINER (cnt->parent))
    {
      cnt = WIDGET_CONTAINER (cnt->parent);
    }

  /* Find first focusable neighour */
  widget=get_neighbour_iter (WIDGET (cnt), __dir);
  while (widget && !FOCUSABLE (widget))
    {
      widget = get_neighbour (widget, __dir);
    }

  return widget;
}

/**
 * Common part of widget_container_delete() and widget_container_drop()
 *
 * @param __widget - container from which child has to be deleted
 * @param __child - widget to be deleted
 * @param __call_deleter - is in necessary to call widget's deleter
 */
static void
w_container_delete_entry (w_container_t *__widget, widget_t *__child,
                          BOOL __call_deleter)
{
  unsigned int i;
  widget_t *w;

  if (!__widget || !__child || !WIDGET_IS_CONTAINER (__widget))
    {
      return;
    }

  for (i = 0; i < WIDGET_CONTAINER_LENGTH (__widget); i++)
    {
      if ((w = WIDGET_CONTAINER_DATA (__widget)[i]) == __child)
        {
          if (__call_deleter)
            {
              widget_destroy (WIDGET (__child));
            }
          else
            {
              if (__child->parent == WIDGET (__widget))
                {
                  __child->parent = 0;
                }
            }

          /* Shift widgets in storage */
          while (i < WIDGET_CONTAINER_LENGTH (__widget) - 1)
            {
              WIDGET_CONTAINER_DATA (__widget)[i] =
                      WIDGET_CONTAINER_DATA (__widget)[i + 1];
              i++;
            }
          WIDGET_CONTAINER_LENGTH (__widget)--;
          break;
        }
    }
}

/**
 * Widget's deleter for deque iterator
 *
 * @param __widget - widget to be deleted
 */
static void
widget_deque_deleter (void *__widget)
{
  widget_destroy (WIDGET (__widget));
}

/**
 * Compare two widgets by their addresses
 *
 * @param __w1 - first widget
 * @param __w2 - second widget
 * @return zero if this two addresses are identical,
 * -1 is first address is less than second and 1 otherwise
 */
static int
widget_ptr_cmp (const void *__w1, const void *__w2)
{
  return (long) __w2 - (long) __w1;
}

/**
 * Resets focused widget for all parent widgets,
 * which is a container
 * helper for widget_set_focus()
 *
 * @param __widget - from which node start reseting
 * @param __focused - pointer new focused widget
 */
static void
reset_focused_widget (w_container_t *__widget, widget_t *__focused)
{
  if (!__widget || !WIDGET_IS_CONTAINER (__widget))
    {
      return;
    }

  __widget->focused_widget = __focused;
  reset_focused_widget (WIDGET_CONTAINER (__widget->parent), __focused);
}

/**
 * Popup as much as possible, while
 * a focused_widget is actual
 *
 * @param __widget - from where startpopupping
 * @return
 */
static w_container_t*
get_toplevel (widget_t *__widget)
{
  if (!__widget)
    {
      return 0;
    }

  w_container_t *cur = WIDGET_CONTAINER (__widget->parent);

  if (!cur)
    {
      return WIDGET_IS_CONTAINER (__widget) ? WIDGET_CONTAINER (__widget) : 0;
    }

  if (!WIDGET_IS_CONTAINER (cur))
    {
      return 0;
    }

  while (cur->parent && WIDGET_IS_CONTAINER (cur->parent))
    {
      cur = WIDGET_CONTAINER (cur->parent);
    }

  return cur;
}

/**
 * Iterator for widget_on_scr_resize
 *
 * @param __state - stage of iteration:
 *  0 - Call onresize callbaback for no-ontop widgets
 *  1 - Call onresize callbaback for ontop widgets
 */
static void
on_scr_resize_iter (short __step)
{
  iterator_t *iter;
  widget_t *w;

  iter = root_widgets->tail;
  while (iter)
    {
      w = iter->data;
      if ((__step == 0 && !WIDGET_TEST_FLAG (w, WF_ONTOP)) ||
          (__step == 1 && WIDGET_TEST_FLAG (w, WF_ONTOP)))
        {
          WIDGET_CALL_CALLBACK (w, onresize, w);
        }
      iter = iter->prev;
    }
}

/**
 * Callback for hook "exit-hook"
 *
 * @param __call_data - calling context
 * @return HOOK_SUCCESS if succeed, HOOK_FAILURE otherwise
 */
static int
exit_hook_callback (struct dynstruct_t *__call_data ATTR_UNUSED)
{
  /* We need to stop widgets' main loop */

  stop_main_loop = TRUE;

  return HOOK_SUCCESS;
}

/**
 * Handler for hook "switch-task-hook"
 *
 * @param __call_data - calling context
 * @return HOOK_SUCCESS if succeed, HOOK_FAILURE otherwise
 */
static int
switch_task_hook_callback (struct dynstruct_t *__call_data ATTR_UNUSED)
{
  widget_process_queue ();

  return HOOK_SUCCESS;
}

/********
 *
 */

/**
 * Main loop of widget stuff to manipulate with messages from user
 */
void
widget_main_loop (void)
{
  wint_t ch;

  /* Widgets' main loop stopped before it is started */
  if (stop_main_loop)
    {
      return;
    }

  main_loop_started = TRUE;

  for (;;)
    {
      ch = widget_get_char ();

      if (ch)
        {
          widget_process_char (ch);
        }
      else
        {
          break;
        }
    }

  main_loop_started = FALSE;
}

/**
 * Process specified char is token
 *
 * @param __ch - char to process with
 */
void
widget_process_char (wint_t __ch)
{
  widget_t *w;

  if (deque_head (root_widgets))
    {
      w = deque_data (deque_head (root_widgets));
      WIDGET_CALL_CALLBACK (w, keydown, w, __ch);
    }
}

/**
 * Process queue of inputed characters
 */
void
widget_process_queue (void)
{
  wint_t ch;

  while ((ch = scr_wnd_getch (FALSE)))
    {
      widget_process_char (ch);
    }
}

/**
 * Get next character for widgets' stuff
 *
 * @param __locking - is this call is locking
 * @return code of next character
 */
wint_t
widget_get_char (void)
{
  struct timespec timestruc = {0, DELAY};
  wint_t ch;

  while (!stop_main_loop)
    {
      ch = scr_wnd_getch (FALSE);

      if (ch)
        {
          return ch;
        }
      else
        {
          nanosleep (&timestruc, 0);
        }
    }

  return 0;
}

/**
 * Initialize widgets' stuff
 *
 * @return a zero in successful, non-zero otherwise
 */
int
widgets_init (void)
{
  root_widgets = deque_create ();

  if (!root_widgets)
    {
      return -1;
    }

  hook_register (L"exit-hook", exit_hook_callback, 0);
  hook_register (L"switch-task-hook", switch_task_hook_callback, 0);

  return 0;
}

/**
 * Uninitialize widget's stuff
 */
void
widgets_done (void)
{
  hook_unhook (L"exit-hook", exit_hook_callback);
  hook_unhook (L"switch-task-hook", exit_hook_callback);

  destroying_root_widgets = TRUE;
  deque_destroy (root_widgets, widget_deque_deleter);
  root_widgets = NULL;
  destroying_root_widgets = FALSE;
}

/**
 * Adds widget to list of root widgets
 *
 * @param __widget - widget to be added
 */
void
widget_add_root (widget_t *__widget)
{
  if (!__widget || !root_widgets || destroying_root_widgets)
    {
      return;
    }

  /* Widget is already in root widgets */
  if (deque_find (root_widgets, __widget, widget_ptr_cmp))
    {
      widget_delete_root (__widget);
    }

  /* Blure previous head widget in list */
  if (deque_head (root_widgets))
    {
      widget_t *w = deque_data (deque_head (root_widgets));
      WIDGET_CALL_CALLBACK (w, blured, w);
    }

  deque_push_front (root_widgets, __widget);
}

/**
 * Deletes widget from list of root widgets.
 *
 * @param __widget - widget to be added
 */
void
widget_delete_root (widget_t *__widget)
{
  if (!__widget || !root_widgets || destroying_root_widgets)
    {
      return;
    }

  iterator_t *iter;

  if ((iter = deque_find (root_widgets, __widget, widget_ptr_cmp)))
    {
      deque_remove (root_widgets, iter, 0);
    }

  /* Focus head widget in list */
  if (deque_head (root_widgets))
    {
      widget_t *w = deque_data (deque_head (root_widgets));
      WIDGET_CALL_CALLBACK (w, focused, w);
    }
}

/**
 * Sink root widget down on stack
 *
 * @param __widget - widget to sink
 */
void
widget_sink_root (widget_t *__widget)
{
  iterator_t *iter = deque_head (root_widgets);
  if (iter && deque_data (iter) == __widget)
    {
      iter = deque_next (iter);
      if (iter)
        {
          widget_t *w = deque_data (iter);
          widget_add_root (w);
        }
    }
}

/**
 * Destructor of any widget
 *
 * @param __widget - widget to be destroyed
 */
void
widget_destroy (widget_t *__widget)
{
  if (!__widget)
    {
      return;
    }

  /*
   * TODO: Maybe we should do something with widget's hotkey context here
   */

  /* Call destructor from object */
  if (__widget->methods.destroy)
    {
      __widget->methods.destroy (__widget);
    }

   /* Destroy user's context */
   if (__widget->user_context)
     {
       hotkey_destroy_context (__widget->user_context);
     }

   /* Free allocated memory */
   SAFE_FREE (__widget);
}

/**
 * Draw widget on screen
 *
 * @param __widget - widget to bew drawn
 */
int
widget_draw (widget_t *__widget)
{
  if (!__widget || !__widget->methods.draw)
    {
      return -1;
    }

  /* Layout of widget is locked */
  if (WIDGET_TEST_FLAG (__widget, WF_REDRAW_LOCKED) ||
      !WIDGET_VISIBLE (__widget))
    {
      return 0;
    }

  return __widget->methods.draw (__widget);
}

/**
 * Redraw widget on screen
 *
 * @param __widget - widget to be redrawn
 */
int
widget_redraw (widget_t *__widget)
{
  int res;

  if (!__widget)
    {
      return 0;
    }

  /* Layout of widget is locked or widget is invisible */
  if (WIDGET_TEST_FLAG (__widget, WF_REDRAW_LOCKED) ||
      !WIDGET_VISIBLE (__widget))
    {
      return 0;
    }

  res = widget_draw (__widget);
  scr_wnd_invalidate (WIDGET_LAYOUT (__widget));
  scr_wnd_refresh (WIDGET_LAYOUT (__widget));

  return res;
}

/**
 * Totally redraws all widgets
 */
void
widget_full_redraw (void)
{
  update_panels ();
  scr_doupdate ();
}

/**
 * Call this method when root screen has been resized
 */
void
widget_on_scr_resize (void)
{
  scr_clear ();

  on_scr_resize_iter (0);
  on_scr_resize_iter (1);

  widget_full_redraw ();
}

/**
 * Set focus to widget
 *
 * @param __widget - widget you want to set focus to
 */
void
widget_set_focus (widget_t *__widget)
{
  if (!__widget)
    {
      return;
    }

  /* If widget is already focused, we should do nothing */
  if (__widget->focused)
    {
      return;
    }

  w_container_t *top_level = get_toplevel (__widget),
                *cnt = WIDGET_CONTAINER (__widget->parent);

  /* Manage previously focused widget */
  if (top_level && top_level->focused_widget)
    {
      widget_t *old_focused = top_level->focused_widget;
      w_container_t *focused_cnt;
      focused_cnt = WIDGET_CONTAINER (old_focused->parent);

      /* Zerolize focused widgets in previously focused branch */
      reset_focused_widget (focused_cnt, 0);

      /* Remove widget's contexts because it is unfocused */
      widget_pop_context (old_focused);

      /* Mark widget as unfocused */
      old_focused->focused = FALSE;

      /* Call `blured` callback */
      WIDGET_CALL_CALLBACK (old_focused, blured, old_focused);

      /* Redraw widget */
      widget_redraw (old_focused);
    }

  if (cnt && WIDGET_IS_CONTAINER (cnt))
    {
      /* Store focused widget for it's parent */
      cnt->focused_widget = __widget;

      /* Store focused widget for all other widgets */
      reset_focused_widget (WIDGET_CONTAINER (cnt->parent), __widget);
    }

  /* Set focus to new widget and redraw */
  __widget->focused = TRUE;

  /* Set widget's class context as current context */
  widget_push_context (__widget);

  if (!WIDGET_CALL_CALLBACK (__widget, focused, __widget))
    {
      /* We have to redraw widget only if widget-defined */
      /* callback returned zero-code */
      widget_redraw (__widget);
    }
}

/****
 * Different stuff
 */

/**
 * Extract shortcut key from text
 *
 * @param __text - text from which you want to extract hotkey
 * @return extracted shortcut
 */
wchar_t
widget_shortcut_key (const wchar_t *__text)
{
  int i, n;

  if (!__text)
    {
      return 0;
    }

  for (i = 0, n = wcslen (__text); i < n; ++i)
    if (__text[i] == '_' && i < n - 1)
      {
        i++;
        if (!iswspace (__text[i]))
          {
            return towlower (__text[i]);
          }
      }

  return 0;
}

/**
 * Return length of text with shortcuts
 *
 * @param __text - text whose length you want to calculate
 * @return length of text
 */
int
widget_shortcut_length (const wchar_t *__text)
{
  int len = 0, i, n;

  if (!__text)
    {
      return 0;
    }

  for (i = 0, n = wcslen (__text); i < n; ++i)
    {
      if (__text[i] == '_' && i < n - 1)
        {
          i++;
          if (iswspace (__text[i]))
            {
              len++;
            }
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
widget_shortcut_print (scr_window_t __layout, const wchar_t *__text,
                       scr_font_t __font, scr_font_t __hot_font)
{
  int i, n;
  BOOL hot = FALSE, hot_printed = FALSE;

  if (!__text)
    {
      return;
    }

  scr_wnd_attr_backup (__layout);

  scr_wnd_font (__layout, __font);

  for (i = 0, n = wcslen (__text); i < n; ++i)
    {
      if (__text[i] == '_' && i < n - 1)
        {
          if (!iswspace (__text[i + 1]))
            {
              hot = !hot_printed;
              i++;
            }
        }

      if (hot)
        scr_wnd_font (__layout, __hot_font);

      scr_wnd_add_wchar (__layout, __text[i]);

      if (hot)
        {
          scr_wnd_font (__layout, __font);
          hot = FALSE;
          hot_printed = TRUE;
        }
    }

  scr_wnd_attr_restore (__layout);
}

/**
 * Return next widget available to be focused
 *
 * @param __widget - container of widgets where search of such widget
 *   have to be started
 * @return wanted widget
 */
widget_t *
widget_next_focused (const widget_t *__widget)
{
  return get_focused_entry (__widget, 1);
}

/**
 * Return previous widget available to be focused
 *
 * @param __widget - container of widgets where search of such widget
 *   have to be started
 * @return wanted widget
 */
widget_t *
widget_prev_focused (const widget_t *__widget)
{
  return get_focused_entry (__widget, -1);
}

/**
 * Return first focusable widget in container
 *
 * @param __parent - in which container find focusable widget
 * @return seqrched widget or NULL if there is no focusable widgets
 */
widget_t*
widget_first_focusable (const w_container_t* __parent)
{
  widget_t *widget;

  widget=get_neighbour_iter (WIDGET (__parent), 1);

  while (widget && !FOCUSABLE (widget))
    {
      widget = get_neighbour (widget, 1);
    }

  return widget;
}

/****
 * Container
 */

/**
 * Append widget to container on specified position
 *
 * @param __container - container where widget have to be added
 * @param __widget - widget which you want to add to container
 * @param __pos - position of widget
 */
void
w_container_insert_child (w_container_t *__container, widget_t *__widget,
                          unsigned int __pos)
{
  unsigned int i;

  if (!__widget)
    {
      return;
    }

  if (!__container)
    {
      widget_add_root (__widget);
      return;
    }

  /* `__container` is not a container-based widget */
  /* or a `__widget` is NULL */
  if (!__container || !WIDGET_IS_CONTAINER (__container) || !__widget)
    {
      return;
    }

  /* Need to make array a bit bigger */
  if (__container->container.length >= __container->container.alloc_length)
    {
      __container->container.alloc_length += CONTAINER_REALLOC_DELTA;
      __container->container.data = realloc (__container->container.data,
                     __container->container.alloc_length * sizeof (widget_t*));
    }

  /* Shift an array */
  for (i = __container->container.length; i > __pos; i--)
    {
      __container->container.data[i] = __container->container.data[i - 1];
    }

  __widget->tab_order = WIDGET_CONTAINER_LENGTH (__container);

  /* Add widget to storage */
  __container->container.data[__pos] = __widget;
  __container->container.length++;

  __widget->parent = WIDGET (__container);

  if (WIDGET_TEST_FLAG (__container, WF_CONTAINER))
    {
      widget_resize (__widget, 0, 0, __container->position.width,
                     __container->position.height);
    }
}

/**
 * Append widget to container
 *
 * @param __container - container where widget have to be added
 * @param __widget - widget which you want to add to container
 */
void
w_container_append_child (w_container_t *__container, widget_t *__widget)
{
  if (!__widget)
    {
      return;
    }

  if (!__container)
    {
      widget_add_root (__widget);
      return;
    }

  /* `__container` is not a container-based widget */
  /* or a `__widget` is NULL */
  if (!__container || !WIDGET_IS_CONTAINER (__container))
    {
      return;
    }

  unsigned int pos = WIDGET_CONTAINER_LENGTH (__container);
  w_container_insert_child (__container, __widget, pos);
}

/**
 * Delete widget from container and calls a destroyer for it
 *
 * @param __widget - container from which child has to be deleted
 * @param __child - widget tobe deleted
 */
void
w_container_delete (w_container_t *__widget, widget_t *__child)
{
  w_container_delete_entry (__widget, __child, TRUE);
}

/**
 * Drop widget from container without destroying it
 *
 * @param __widget - container from which child has to be dropped
 * @param __child - widget to be dropped
 */
void
w_container_drop (w_container_t *__widget, widget_t *__child)
{
  w_container_delete_entry (__widget, __child, FALSE);
}

/********
 *
 */

/**
 * Return widget with specified tab order from container
 *
 * @param __container - container from which you want to get widget
 * @param __tab_order - tab order of widget you want to get
 * @return widget with specified tab order or NULL if there is no widget
 *   with such tab order
 */
widget_t*
w_container_widget_by_tab_order (const w_container_t *__container,
                                 int __tab_order)
{
  int i, n;

  /* `__container` is not a container-based widget */
  if (!WIDGET_IS_CONTAINER (__container))
    return NULL;

  for (i = 0, n = WIDGET_CONTAINER_LENGTH (__container); i < n; i++)
    {
      if (WIDGET_CONTAINER_DATA (__container)[i] &&
          WIDGET_CONTAINER_DATA (__container)[i]->tab_order == __tab_order)
        {
          return WIDGET_CONTAINER_DATA (__container)[i];
        }
    }

  return NULL;
}

/**
 * Lock layout for redraw
 *
 * @param __widget - widget which layout is to be locked
 */
void
widget_lock_redraw (widget_t *__widget)
{
  if (!__widget)
    {
      return;
    }

  WIDGET_SET_FLAG (__widget, WF_REDRAW_LOCKED);

  WIDGET_CONTAINER_ACTION_ITERONLY (__widget, widget_lock_redraw);
}

/**
 * Lock layout for redraw
 *
 * @param __widget - widget which layout is to be unlocked
 */
void
widget_unlock_redraw (widget_t *__widget)
{
  if (!__widget)
    {
      return;
    }

  WIDGET_RESET_FLAG (__widget, WF_REDRAW_LOCKED);

  WIDGET_CONTAINER_ACTION_ITERONLY (__widget, widget_unlock_redraw);
}

/**
 * Resize widget
 *
 * @param __widget - widget to be resized
 * @param __x, __y - new coordinates of widget
 * @param __w, __h - new width and height of widget
 */
void
widget_resize (widget_t *__widget, int __x, int __y, int __w, int __h)
{
  if (!__widget)
    {
      return;
    }

  /* Store new size in widget */
  __widget->position.x = __x;
  __widget->position.y = __y;
  __widget->position.width = __w;
  __widget->position.height = __h;

  WIDGET_CALL_CALLBACK (__widget, onresize, __widget);

  widget_redraw (WIDGET (__widget));
}

/**
 * Handler of `focused` callback (system-based)
 *
 * @param __widget - widget which catched this callback
 * @return zero if callback hasn't handled callback
 */
int
widget_focused (widget_t *__widget)
{
  if (!__widget)
    {
      return 0;
    }

  _WIDGET_CALL_USER_CALLBACK (__widget, focused, __widget);

  if (WIDGET_IS_CONTAINER (__widget))
    {
      w_container_t *cnt = WIDGET_CONTAINER (__widget);
      if (cnt->focused_widget)
        {
          return WIDGET_CALL_CALLBACK (cnt->focused_widget, focused,
                                       cnt->focused_widget);
        }
    }

  return 0;
}

/**
 * Handler of `blured` callback (system-based)
 *
 * @param __widget - widget which catched this callback
 * @return zero if callback hasn't handled callback
 */
int
widget_blured (widget_t *__widget)
{
  if (!__widget)
    {
      return 0;
    }

  _WIDGET_CALL_USER_CALLBACK (__widget, blured, __widget);

  if (WIDGET_IS_CONTAINER (__widget))
    {
      w_container_t *cnt = WIDGET_CONTAINER (__widget);
      if (cnt->focused_widget)
        {
          return WIDGET_CALL_CALLBACK (cnt->focused_widget, blured,
                                       cnt->focused_widget);
        }
    }

  return 0;
}

/**
 * Handler of `keydown` callback (system-based)
 *
 * @param __widget - widget which catched this callback
 * @param __ch - received character
 * @return zero if callback hasn't handled callback
 */
int
widget_keydown (widget_t *__widget, int __ch)
{
  /* Call user's callback */
  _WIDGET_CALL_USER_CALLBACK (__widget, keydown, __widget, __ch);

  return 0;
}

/**
 * Handler of `shortcut` callback (system-based)
 *
 * @param __widget - widget which catched this callback
 * @return zero if callback hasn't handled callback
 */
int
widget_shortcut (widget_t *__widget)
{
  /* Call user's callback */
  _WIDGET_CALL_USER_CALLBACK (__widget, shortcut, __widget);

  return 0;
}

/**
 * Handler of `onresize` callback (system-based)
 *
 * @param __widget - widget which catched this callback
 * @return zero if callback hasn't handled callback
 */
int
widget_onresize (widget_t *__widget)
{
  _WIDGET_CALL_USER_CALLBACK (__widget, onresize, __widget);

  /* There is no window's resizing stuff, so */
  /* we have to create new window and use it */
  scr_window_t oldwnd = WIDGET_LAYOUT (__widget),
               newwnd = widget_create_layout (WIDGET (__widget));

  WIDGET_LAYOUT (__widget) = newwnd;

  BOOL locked = WIDGET_TEST_FLAG (__widget, WF_REDRAW_LOCKED);

  if (!locked)
    {
      widget_lock_redraw (__widget);
    }

  /* But does we really need this call? */
  WIDGET_CONTAINER_ACTION_ITERONLY (__widget, WIDGET_CALL_CALLBACK,
                                    onresize, __iterator_);

  if (!locked)
    {
      widget_unlock_redraw (__widget);
      widget_redraw (__widget);
    }

  if (oldwnd && !WIDGET_TEST_FLAG (__widget, WF_NOLAYOUT))
    {
      /*
       * NOTE: Windows must be deleted in such way:
       *         1. Delete all children of window
       *         2. Delete window
       *       If window has children - it won't deleted
       *
       *       So, no children - no problems :)
       */
      scr_destroy_window (oldwnd);
    }

  return 0;
}

/**
 * Create layout for widget
 *
 * @param __widget - widget for which create layout
 * @return created layout
 */
scr_window_t
widget_create_layout (widget_t *__widget)
{
  scr_window_t res;

  if (!__widget)
    {
      return 0;
    }

  if (WIDGET_TEST_FLAG (__widget, WF_NOLAYOUT))
    {
      /* Widget doesn't have it's own layout, */
      /* so it will use parent's layout */
      if (__widget->parent)
        {
          return WIDGET_LAYOUT (__widget->parent);
        }
    }

  /* Save position to make code shorter */
  widget_position_t pos = __widget->position;

  if (!__widget->parent)
    {
      /* If there is no parent of widget, */
      /* we just create new layout window */
      res = scr_create_window (pos.x, pos.y, pos.width, pos.height);
    }
  else
    {
      /* Otherwise we should create new subwindow on parent's window */
      res = scr_create_sub_window (WIDGET_LAYOUT (__widget->parent),
                                   pos.x, pos.y, pos.width, pos.height);
    }

  return res;
}

/**
 * Check widget's visibility
 *
 * @param __widget - widget descriptor for which visibility will be checked
 * @return non-zero if widget is visible, zero otherwise
 */
BOOL
widget_visible (widget_t *__widget)
{
  widget_t *cur;

  if (!__widget)
    {
      return FALSE;
    }

  cur = __widget;
  while (cur)
    {
      if ((WIDGET_POSITION (cur)).z <= 0)
        {
          return FALSE;
        }

      cur = cur->parent;
    }

  return TRUE;
}

/**
 * Replace widget class context
 *
 * @param __widget - widget for which context will be replaced
 * @param __context - descriptor of new context
 */
void
widget_replace_class_context (widget_t *__widget, hotkey_context_t *__context)
{
  if (!__widget || __widget->class_context == __context)
    {
      return;
    }

  widget_pop_context (__widget);
  __widget->class_context = __context;
  widget_push_context (__widget);
}

/**
 * Replace widget user context
 *
 * @param __widget - widget for which context will be replaced
 * @param __context - descriptor of new context
 */
void
widget_replace_user_context (widget_t *__widget, hotkey_context_t *__context)
{

}

/**
 * Push widget's contexts
 *
 * @param __widget - widget which contexts will be push-ed
 */
void
widget_push_context (widget_t *__widget)
{
  if (!__widget || !__widget->focused || !WIDGET_VISIBLE (__widget))
    {
      return;
    }

  if (__widget->class_context)
    {
      hotkey_push_context (__widget->class_context);
    }
}

/**
 * Pop widget's contexts
 *
 * @param __widget - widget which contexts will be pop-ed
 * @return zero on success, non-zero otherwise
 */
int
widget_pop_context (widget_t *__widget)
{
  if (!__widget || !__widget->focused || !WIDGET_VISIBLE (__widget))
    {
      return -1;
    }

  if (__widget->class_context)
    {
      if (hotkey_current_context () == __widget->class_context)
        {
          hotkey_pop_context (FALSE);
          return 0;
        }
      else
        {
#ifdef DEBUG
          MESSAGE_ERROR (L"Error in pop_widget_context: unable to "
                         "pop widget's context.\n"
                         "Widget's class context is not on top of "
                         "context's stack.");
#endif
          return -1;
        }
    }

  return 0;
}
