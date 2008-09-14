/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of widget list
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "widget.h"

#include <util.h>

#define ITEMS_PER_PAGE(__list) \
  (__list->position.height - 2)

/**
 * Destroy a list widget
 *
 * @param __list - list to be destroyed
 * @return zero on success, non-zero on failure
 */
static int
list_destructor (w_list_t *__list)
{
  if (!__list)
    {
      return -1;
    }

  widget_destroy (__list->scrollbar);

  SAFE_FREE (__list->items.data);

  free (__list);
  return 0;
}

/**
 * Draw a list
 *
 * @param __list - list to be drawn
 * @return zero on success, non-zero on failure
 */
static int
list_drawer (w_list_t *__list)
{
  scr_window_t layout = WIDGET_LAYOUT (__list);
  __u32_t i, n, items_width, cur_width;
  wchar_t *text;

  /* Widget is invisible or there is no layout */
  if (!WIDGET_VISIBLE (__list) || !layout)
    {
      return -1;
    }

  scr_wnd_attr_backup (layout);

  widget_draw_border (WIDGET (__list));

  /* Draw caption */
  if (__list->caption)
    {
      text = wcsfit (__list->caption, __list->position.width - 4, L"...");

      scr_wnd_move_caret (layout, __list->position.x + 1, __list->position.y);
      scr_wnd_font (layout, *__list->caption_font);

      scr_wnd_putch (layout, ' ');
      scr_wnd_add_nstr (layout, text, wcslen (text));
      scr_wnd_putch (layout, ' ');

      free (text);
    }

  /* Draw items */
  items_width = __list->position.width - 2;
  n = MIN (__list->items.count, ITEMS_PER_PAGE (__list));

  for (i = 0; i < n; ++i)
    {
      if (i + __list->scroll_top == __list->items.current)
        {
          if (__list->focused)
            {
              scr_wnd_font (layout, *__list->cursor_font);
            }
          else
            {
              scr_wnd_font (layout, *__list->cursor_unfocused_font);
            }
        }
      else
        {
          scr_wnd_font (layout, *__list->font);
        }

      scr_wnd_move_caret (layout, __list->position.x + 1,
                          __list->position.y + i +1);

      text = __list->items.data[i + __list->scroll_top].text;
      text = wcsfit (text, items_width, L"...");
      cur_width = wcswidth (text, wcslen (text));

      scr_wnd_add_nstr (layout, text, wcslen (text));

      while (cur_width < items_width)
        {
          scr_wnd_putch (layout, ' ');
          ++cur_width;
        }

      free (text);
    }

  if (__list->items.count > ITEMS_PER_PAGE (__list))
    {
      /* Draw scrollbar only if items more than lines pre page */
      widget_draw (__list->scrollbar);
    }

  scr_wnd_attr_restore (layout);

  return 0;
}

/**
 * Handle a keydown callback
 *
 * @param __list - list received a callback
 * @param __ch - received character
 * @return zero if callback hasn't handled received character
 */
static int
list_keydown (w_list_t *__list, wint_t __ch)
{
  int sindex;

  _WIDGET_CALL_USER_CALLBACK (__list, keydown, __list, __ch);

  /* Save currently selected item index to determine */
  /* is it needed to call property_changed callback */
  sindex = __list->items.current;

  switch (__ch)
    {
    case KEY_UP:
      if (__list->items.current > 0)
        {
          --__list->items.current;
          if (__list->items.current < __list->scroll_top)
            {
              --__list->scroll_top;
            }
        }
      break;
    case KEY_DOWN:
      if (__list->items.current < __list->items.count - 1)
        {
          ++__list->items.current;
          if (__list->items.current > __list->scroll_top +
                  ITEMS_PER_PAGE (__list) - 1)
            {
              ++__list->scroll_top;
            }
        }
      break;
    case KEY_HOME:
      __list->scroll_top =  __list->items.current = 0;
      break;
    case KEY_END:
      __list->items.current = __list->items.count - 1;
      __list->scroll_top = __list->items.count - ITEMS_PER_PAGE (__list);
      break;
    case KEY_NPAGE:
      __list->items.current += ITEMS_PER_PAGE (__list);
      if (__list->items.current >= __list->items.count)
        {
          __list->items.current = __list->items.count - 1;
          __list->scroll_top = __list->items.count - ITEMS_PER_PAGE (__list);
        }
      else
        {
          __list->scroll_top += ITEMS_PER_PAGE (__list);
          __list->scroll_top = MIN (__list->scroll_top, __list->items.count -
                   ITEMS_PER_PAGE (__list));
        }
      break;
    case KEY_PPAGE:
      if (__list->items.current >= ITEMS_PER_PAGE (__list))
        {
          __list->items.current -= ITEMS_PER_PAGE (__list);
          if (__list->scroll_top >= ITEMS_PER_PAGE (__list))
            {
              __list->scroll_top -= ITEMS_PER_PAGE (__list);
            }
          else
            {
              __list->scroll_top = 0;
            }
        }
      else
        {
          __list->items.current = __list->scroll_top = 0;
        }
      break;
    default:
      return FALSE;
    }

  /* Update position of scrollbar */
  w_scrollbar_set_pos ((w_scrollbar_t*)__list->scrollbar,
                        __list->items.current);

  widget_redraw (WIDGET (__list));

  if (sindex != __list->items.current)
    {
      /* Item index has been changed */
      WIDGET_CALL_USER_CALLBACK (__list, property_changed,
                                 __list, W_LIST_ITEMINDEX_PROP);
    }

  return TRUE;
}

/**
 * Handler of onresize callback
 *
 * @param __list - descriptor of a list which catched this callback
 */
static int
list_onresize (w_list_t *__list)
{
  if (!__list)
    {
      return FALSE;
    }

  widget_onresize (WIDGET (__list));
  widget_onresize (__list->scrollbar);

  return TRUE;
}

/********
 * User's backend
 */

/**
 * Create new list
 *
 * @param __parent - parent of list. Should be CONTAINER
 * @param __x, __y - coordinates of list
 * @param __w, __h - dimensions of list
 * @return a pointer to list object
 */
w_list_t*
widget_create_list (w_container_t *__parent, const wchar_t *__caption,
                    int __x, int __y, int __w, int __h)
{
  w_list_t *res;

  /* There is no parent, so we can't create progress bar */
  if (!__parent)
    {
      return 0;
    }

  WIDGET_INIT (res, w_list_t, WT_LIST, __parent, WF_NOLAYOUT,
               list_destructor, list_drawer, __x, __y, 1, __w, __h);

  WIDGET_CALLBACK (res, keydown)  = (widget_keydown_proc)list_keydown;
  WIDGET_CALLBACK (res, onresize) = (widget_action)list_onresize;

  if (__caption)
    {
      res->caption = wcsdup (__caption);
    }

  res->scrollbar = (widget_t*)widget_create_scrollbar (WIDGET (res), 0,
                                            __x + __w - 1,
                                            __y + 1, __h - 2, 0);

  res->font = &FONT (CID_BLACK, CID_WHITE);
  res->cursor_font = &FONT (CID_BLACK, CID_CYAN);
  res->cursor_unfocused_font = &FONT (CID_BLUE, CID_CYAN);
  res->caption_font = &FONT (CID_BLUE, CID_WHITE);

  WIDGET_POST_INIT (res);

  return res;
}

/**
 * Insert item to list
 *
 * @param __list - where item will be inserted
 * @param __pos - position of item
 * @param __text - text of item
 * @param __tag - tag, associated with item
 * @return pointer to new item
 */
w_list_item_t*
w_list_insert_item (w_list_t *__list, __u32_t __pos,
                    wchar_t *__text, __u32_t __tag)
{
  __u32_t i;

  if (!__list || !__text || __pos>__list->items.count)
    {
      return NULL;
    }

  /* Allocate memory for new item */
  __list->items.data = realloc (__list->items.data,
                                (__list->items.count + 1) *
                                    sizeof (w_list_item_t));

  /* Shift array */
  for (i = __list->items.count; i > __pos; --i)
    {
      __list->items.data[i] = __list->items.data[i - 1];
    }

  /* Fill fields of new item */
  __list->items.data[__pos].text = wcsdup (__text);
  __list->items.data[__pos].tag = __tag;

  /* Update size of scrollbar */
  w_scrollbar_set_size ((w_scrollbar_t*)__list->scrollbar,
                        __list->items.count);

  ++__list->items.count;

  /* Redraw widget */
  widget_redraw (WIDGET (__list));

  return &__list->items.data[__pos];
}

/**
 * Append item to list
 *
 * @param __list - where item will be appended
 * @param __text - text of item
 * @param __tag - tag, associated with item
 */
w_list_item_t*
w_list_append_item (w_list_t *__list, wchar_t *__text, __u32_t __tag)
{
  if (!__list)
    {
      return NULL;
    }

  return w_list_insert_item (__list, __list->items.count, __text, __tag);
}

/**
 * Set fonts used in list
 *
 * @param __list - list to oerate with
 * @param __font - font for default items
 * @param __cursor_font - font for cursor and selected item
 * @param __cursor_unfocused_font - font for cursor and selected item when
 * widget is not focused
 * @param __caption_font - font of caption
 */
void
w_list_set_fonts (w_list_t *__list, scr_font_t *__font,
                  scr_font_t *__cursor_font,
                  scr_font_t *__cursor_unfocused_font,
                  scr_font_t *__caption_font)
{
  if (!__list)
    {
      return;
    }

  WIDGET_SAFE_SET_FONT (__list, font, __font);
  WIDGET_SAFE_SET_FONT (__list, cursor_font, __cursor_font);
  WIDGET_SAFE_SET_FONT (__list, cursor_unfocused_font,
                        __cursor_unfocused_font);
  WIDGET_SAFE_SET_FONT (__list, caption_font, __caption_font);

  widget_redraw (WIDGET (__list));
}

/**
 * Get current item
 *
 * @param __list - from which list item will be gotten
 * @reutrn currently selected item
 */
w_list_item_t*
w_list_get_current_item (w_list_t *__list)
{
  if (__list == NULL || __list->items.count == 0)
    {
      return NULL;
    }

  return &__list->items.data[__list->items.current];
}
