/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * implementation file of widget box
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "screen.h"
#include "widget.h"

/********
 *
 */

#define BOX_ITEM(__box, i) \
  ((w_box_item_t*)(WIDGET_CONTAINER_DATA (__box)[i]))

#define RECALC_BOX(__boc) \
  __box->evaluted = FALSE; \
  eval_sizes (__box); \
  widget_redraw (WIDGET (__box));

#define SHORTCUT_CHECKER(_w, _shortcut_key) \
  if (WIDGET_SHORTCUT (_w)==_shortcut_key) \
    { \
      if (WIDGET_CALLBACK (_w, shortcut)) \
        return WIDGET_CALLBACK (_w, shortcut) (_w); else \
        break; \
    }

/********
 *
 */

static void
eval_sizes (w_box_t *__box);

/********
 *
 */

/**
 * Destroy a box widget
 *
 * @param __box - box to be destroyed
 * @return zero on success, non-zero on failure
 */
static int
box_destructor (w_box_t *__box)
{
  if (!__box)
    {
      return -1;
    }

  /* Delete panel associated with layout */
  if (__box->panel)
    {
      panel_del (__box->panel);
    }

  /* Destroy screen layout */
  if (WIDGET_LAYOUT (__box))
    {
      scr_destroy_window (WIDGET_LAYOUT (__box));
    }

  /*  Destroy all items */
  WIDGET_CONTAINER_DELETER (__box);

  free (__box);

  return 0;
}

/**
 * Destroy a box item widget
 *
 * @param __box_item - box item to be destroyed
 * @return zero on success, non-zero on failure
 */
static int
box_item_destructor (w_box_item_t *__box_item)
{
  if (!__box_item)
    {
      return -1;
    }

  /* Call deleter inherited from container */
  WIDGET_CONTAINER_DELETER (__box_item);

  /* Destroy screen layout */
  if (WIDGET_LAYOUT (__box_item))
    {
      scr_destroy_window (WIDGET_LAYOUT (__box_item));
    }

  free (__box_item);

  return 0;
}

/**
 * Draw a box
 *
 * @param __box - box to be drawn
 * @return zero on success, non-zero on failure
 */
static int
box_drawer (w_box_t *__box)
{
  if (!WIDGET_VISIBLE (__box))
    {
      return -1;
    }

  eval_sizes (__box);

  WIDGET_CONTAINER_DRAWER (__box);

  return 0;
}

/**
 * Draw a box item
 *
 * @param __box_item - box to be drawn
 * @return zero on success, non-zero on failure
 */
static int
box_item_drawer (w_box_item_t *__box_item)
{
  if (!__box_item)
    {
      return -1;
    }

  /* Inherit layout from parent */
  scr_window_t layout = WIDGET_LAYOUT (__box_item);

  /* Widget is invisible or there is no layout */
  if (!WIDGET_VISIBLE (__box_item) || !layout)
    {
      return -1;
    }

  /* Call drawer inherited from container */
  WIDGET_CONTAINER_DRAWER (__box_item);

  return 0;
}

/**
 * Calculate sizes for items
 *
 * @param __box - box for which items sizes will be calculated
 */
static void
eval_sizes (w_box_t *__box)
{
  /* It there is no box or sizes of items have been */
  /* already evaluated */
  if (!__box || __box->evaluted)
    {
      return;
    }

  unsigned int i;
  int offset = 0,    /* Offset of current item from beginning of box */
      size = 0,      /* Size of one automatically managing item */
      cur_size,      /* Size of current item */
      dyn_size,      /* Total size of automatically managing items */
      unset_count,   /* Count of item for which size is not specified by user */
      set_count = 0, /* Count of such (see line upper) item for which size */
                     /* was set during calculating process */
      set_size = 0,  /* Total size of such items (see upper) */
      dummy;

  widget_position_t pos;
  widget_t *cur;
  scr_window_t oldwin;

  BOOL horisontal = __box->style&WBS_HORISONTAL;

  /* Total size for automatically-managed items */
  if (!horisontal)
    {
      dyn_size = __box->position.width;
    }
  else
    {
      dyn_size = __box->position.height;
    }

  /* Count of items for which size will be generated automatically */
  unset_count = 0;
  for (i = 0; i < WIDGET_CONTAINER_LENGTH (__box); i++)
    {
      if ((dummy = BOX_ITEM (__box, i)->size) < 0)
        {
          ++unset_count;
        }
      else
        {
          dyn_size -= dummy;
        }
    }

  /* Size of one item which size is not specified by user */
  if (unset_count)
    {
      size = dyn_size / unset_count;
    }

  for (i = 0; i < WIDGET_CONTAINER_LENGTH (__box); i++)
    {
      cur = WIDGET_CONTAINER_DATA (__box)[i];

      cur_size = BOX_ITEM (__box, i)->size;
      if (cur_size < 0)
        {
          cur_size = size;
          set_count++;
        }

      /* Recalculate size for last item which size is not */
      /* specified by user */
      if (set_count == unset_count && unset_count)
        {
          cur_size = dyn_size - set_size;
          unset_count = 0;
        }

      /* Set size and offset for current item */
      pos.z = cur->position.z;
      if (!horisontal)
        {
          pos.x = offset;
          pos.y = __box->position.y;
          pos.width = cur_size;
          pos.height = __box->position.height;
        }
      else
        {
          pos.x = __box->position.x;
          pos.y = offset;
          pos.width = __box->position.width;
          pos.height = cur_size;
        }
      cur->position = pos;

      /* Now we have to replace layout for widget */
      oldwin = WIDGET_LAYOUT (cur);
      WIDGET_LAYOUT (cur) = widget_create_layout (WIDGET (cur));

      /* Set size for child */
      if (WIDGET_CONTAINER_LENGTH (cur))
        {
          widget_t *w;
          w = WIDGET_CONTAINER_DATA (cur)[0];
          widget_resize (w, 0, 0, pos.width, pos.height);
        }

      /* Destroy old window */
      if (oldwin)
        {
          scr_destroy_window (oldwin);
        }

      if (BOX_ITEM (__box, i)->size < 0)
        {
          set_size += cur_size;
        }

      offset += cur_size;
    }

  __box->evaluted = TRUE;
}

/**
 * Callback for onresize action
 *
 * @param __box - box which catched this event
 * @return zero if callback hasn't handled callback
 */
static int
box_onresize (w_box_t *__box)
{
  if (!__box)
    {
      return 0;
    }

  if (!__box->parent)
    {
      __box->position.x = 0;
      __box->position.y = 0;
      __box->position.width = SCREEN_WIDTH;
      __box->position.height = SCREEN_HEIGHT;
    }

  _WIDGET_CALL_USER_CALLBACK (__box, onresize, __box);

  __box->evaluted = FALSE;

  /* There is no window's resizing stuff, so */
  /* we have to create new window and use it */
  scr_window_t oldwnd = WIDGET_LAYOUT (__box),
          newwnd = widget_create_layout (WIDGET (__box));

  WIDGET_LAYOUT (__box) = newwnd;
  panel_replace (__box->panel, newwnd);
  panels_doupdate ();

  BOOL locked = WIDGET_TEST_FLAG (__box, WF_REDRAW_LOCKED);

  if (!locked)
    {
      widget_lock_redraw (WIDGET (__box));
    }

  eval_sizes (__box);

  if (!locked)
    {
      widget_unlock_redraw (WIDGET (__box));
      widget_redraw (WIDGET (__box));
    }

  if (oldwnd)
    {
      scr_destroy_window (oldwnd);
    }

  return TRUE;
}

/**
 * Handle a keydown callback
 *
 * @param __window - window received a callback
 * @param __ch - received character
 * @return zero if callback hasn't handled received character
 *   non-zero otherwise
 */
static int
box_keydown (w_box_t *__box, wint_t __ch)
{
  widget_t *focused;

  /* Call user's callback */
  _WIDGET_CALL_USER_CALLBACK (__box, keydown, __box, __ch);

  /* If user's callback hadn't processed this callback, */
  /* make this stuff */

  if ((focused = __box->focused_widget))
    {
      /* If there is focused widget, try to redirect callback to it */
      int res = 0;
      if (WIDGET_CALLBACK (focused, keydown) &&
          (res = WIDGET_CALLBACK (focused, keydown) (focused, __ch)))
        {
          return res;
        }
    }

  switch (__ch)
    {
    /* Navigation */
    case KEY_DOWN:
    case KEY_RIGHT:
    case KEY_TAB:
      widget_set_focus (widget_next_focused (__box->focused_widget));
      return TRUE;

    case KEY_UP:
    case KEY_LEFT:
      widget_set_focus (widget_prev_focused (__box->focused_widget));
      return TRUE;

    default:
      {
        WIDGET_CONTAINER_ACTION_ITERONLY (__box, SHORTCUT_CHECKER,
                                          towlower (__ch));
        break;
      }
    }

  return FALSE;
}

/**
 * Initialize item of box
 *
 * @param __box - parent box
 * @param __item - pointer to item to initialize
 * @param __size - size of item
 */
static void
init_item (w_box_t *__box, w_box_item_t *__item, int __size)
{
  if (!__item)
    {
      return;
    }

  memset (__item, 0, sizeof (*__item));

  __item->size = __size;

  __item->parent = WIDGET (__box);
  __item->type = WT_BOX_ITEM;
  __item->position.z = 1;

  __item->methods.destroy = (widget_action) box_item_destructor;
  __item->methods.draw = (widget_action) box_item_drawer;

  WIDGET_SET_FLAG (__item, WF_CONTAINER);
}

/********
 * User's backend
 */

/**
 * Create new box
 *
 * @param __parent - parent of edit. Should be CONTAINER
 * @param __x, __y - coordinates of box
 * @param __w, __h - width and height of box
 * @param __style - style of box
 * @param __count - count of items in box
 * @return pointer to a box object
 */
w_box_t*
widget_create_box (w_container_t *__parent,
                   int __x, int __y,
                   int __w, int __h,
                   unsigned int __style,
                   unsigned int __count)
{
  int i;
  w_box_t *res;

  /* General widget initialization */
  WIDGET_INIT (res, w_box_t, WT_BOX, __parent, 0,
               box_destructor, box_drawer,
               __x, __y, 1, __w, __h);

  res->style = __style;
  res->panel = panel_new (res->layout);

  WIDGET_CALLBACK (res, onresize) = (widget_action) box_onresize;
  WIDGET_CALLBACK (res, keydown) = (widget_keydown_proc) box_keydown;

  /* Initialize items */
  w_box_item_t *item;
  for (i = 0; i < __count; i++)
    {
      item = malloc (sizeof (w_box_item_t));
      init_item (res, item, -1);
      w_container_append_child (WIDGET_CONTAINER (res), WIDGET (item));
    }

  eval_sizes (res);

  WIDGET_POST_INIT (res);

  return res;
}

/**
 * Sets size for box's item
 *
 * @param __box - box for which item belongs
 * @param __index - index of item to set size
 * @param __size - new size of item (-1 for unspecified)
 */
void
w_box_set_item_szie (w_box_t *__box, unsigned int __index,
                     int __size)
{
  if (!__box || __index >= WIDGET_CONTAINER_LENGTH (__box))
    {
      return;
    }

  BOX_ITEM (__box, __index)->size = __size;

  /* Need to re-evaluate sizes and redraw widget */
  RECALC_BOX (__box);
}

/**
 * Return widget by index
 *
 * @param __box - box for which item belongs
 * @return widget of item with specified index
 */
w_box_item_t*
w_box_item (w_box_t *__box, unsigned int __index)
{
  if (!__box || __index >= WIDGET_CONTAINER_LENGTH (__box))
    {
      return 0;
    }

  return BOX_ITEM (__box, __index);
}

/**
 * Insert new item to box
 *
 * @param __box - box in which item will be added
 * @param __index - position of new item
 * @param __size - size of new item
 * @return pointer to widget of new item
 */
w_box_item_t*
w_box_insert_item (w_box_t *__box, unsigned int __index, int __size)
{
  if (__index > WIDGET_CONTAINER_LENGTH (__box))
    {
      return 0;
    }

  w_box_item_t *item = malloc (sizeof (w_box_item_t));
  init_item (__box, item, __size);
  w_container_insert_child (WIDGET_CONTAINER (__box), WIDGET (item), __index);

  /* Need to re-evaluate sizes and redraw widget */
  RECALC_BOX (__box);

  return item;
}

/**
 * Append new item to box
 *
 * @param __box - box in which item will be added
 * @param __size - size of new item
 * @return pointer to widget of new item
 */
w_box_item_t*
w_box_append_item (w_box_t *__box, int __size)
{
  return w_box_insert_item (__box, WIDGET_CONTAINER_LENGTH (__box), __size);
}

/**
 * Deletes item from box
 *
 * @param __box - box in which item will be added
 * @param __item - item to be deleted
 */
void
w_box_delete_item (w_box_t *__box, w_box_item_t *__item)
{
  if (!__box || !__item)
    {
      return;
    }

  w_container_delete (WIDGET_CONTAINER (__box), WIDGET (__item));

  /* Need to re-evaluate sizes and redraw widget */
  RECALC_BOX (__box);
}

/**
 * Delete item, specified by index
 *
 * @param __box - box in which item will be added
 * @param __index - position of new item
 */
void
w_box_delete_by_index (w_box_t *__box, unsigned int __index)
{
  if (!__box || __index > WIDGET_CONTAINER_LENGTH (__box))
    {
      return;
    }

  w_box_item_t *item = BOX_ITEM (__box, __index);
  w_container_delete (WIDGET_CONTAINER (__box), WIDGET (item));

  /* Need to re-evaluate sizes and redraw widget */
  RECALC_BOX (__box);
}
