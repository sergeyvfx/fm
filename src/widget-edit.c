/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation file of widget edit
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "widget.h"
#include "util.h"

//////
//

#define BUFFER_REALLOC_DELTA  16

//////
//

/**
 * Destroys an edit widget
 *
 * @param __edit - edit to be destroyed
 * @return zero on success, non-zero on failure
 */
static int
edit_destructor                   (w_edit_t *__edit)
{
  if (!__edit)
    return -1;

  // Destroy screen layout
  if (WIDGET_LAYOUT (__edit))
      scr_destroy_window (WIDGET_LAYOUT (__edit));

  free (__edit);
  return 0;
}

/**
 * Draws a button
 *
 * @param __button - button to be drawn
 * @return zero on success, non-zero on failure
 */
static int
edit_drawer                       (w_edit_t *__edit)
{
  int i, printed_len, w;
  size_t len, scrolled, caret_pos;

  // Inherit layout from parent
  scr_window_t layout=WIDGET_LAYOUT (__edit);

  // Widget is invisible or there is no layout
  if (!WIDGET_VISIBLE (__edit) || !layout)
    return -1;

  scr_wnd_attr_backup (layout);

  scr_wnd_move_caret (layout, __edit->position.x, __edit->position.y);
  scr_wnd_font (layout, *__edit->font);

  // Calculate length of text which has to be printed
  scrolled  =__edit->scrolled;
  len       = wcslen (__edit->text.data);
  w         = __edit->position.width;
  caret_pos = __edit->caret_pos;

  printed_len=MIN (len-scrolled, w-(caret_pos-scrolled==w?1:0));

  // Print text
  for (i=0; i<printed_len; i++)
    scr_wnd_add_wchar (layout, __edit->text.data[i+scrolled]);

  // Print spaces to make edit needed width
  for (i=printed_len; i<__edit->position.width; i++)
    scr_wnd_putch (layout, ' ');

  scr_wnd_move_caret (layout, __edit->position.x+caret_pos-scrolled,
    __edit->position.y);

  scr_wnd_attr_restore (layout);

  return 0;
}

/**
 * Handler of `focused` callback (system-based)
 *
 * @param __edit - edit which caucghted this callback
 * @return zero if callback hasn't handled callback
 */
static int
edit_focused                      (w_edit_t *__edit)
{
  _WIDGET_CALL_USER_CALLBACK (__edit, focused, __edit);

  // We shoud show a caret when edit box is focused
  scr_show_curser ();

  return 0;
}

/**
 * Handler of `blured` callback (system-based)
 *
 * @param __edit - edit which caucghted this callback
 * @return zero if callback hasn't handled callback
 */
static int
edit_blured                       (w_edit_t *__edit)
{
  _WIDGET_CALL_USER_CALLBACK (__edit, blured, __edit);

  // When edit box is blured we should hide caret
  scr_hide_curser ();

  return 0;
}

/**
 * Synchronizes allocated buffer length with
 * stored string length
 *
 * @param __edit - edit for which buffer will be synchronized
 */
static void
edit_sync_buffer                  (w_edit_t *__edit)
{
  // There is no widget or no allocated buffer
  if (!__edit || !__edit->text.allocated)
    return;

  size_t txt_len=0, alloc_len, new_len;

  if (__edit->text.data)
    txt_len=wcslen (__edit->text.data);

  new_len = alloc_len = __edit->text.allocated;

  //
  // TODO:
  //  Does we really need this checking?
  //
  if (txt_len<alloc_len)
    {
      if (alloc_len-txt_len<=1)
        {
          // Allocated buffer is near to be fully used
          new_len+=BUFFER_REALLOC_DELTA;
        } else
      if (alloc_len-txt_len>BUFFER_REALLOC_DELTA)
        {
          // Length of current text is much shorter than allocated
          // buffer. It'll be better is we'll truncate buffer to
          // save memory
          new_len=txt_len+BUFFER_REALLOC_DELTA;
       }

      if (new_len!=alloc_len)
        {
          // Realloc buffer
          __edit->text.data=realloc (__edit->text.data,
              new_len*sizeof (wchar_t));
          __edit->text.allocated=new_len;
        }
    }
}

/**
 * Validates data needed for text scrolling in widget
 *
 * @param __edit - edit for which validate scrolling information
 */
static void
edit_validate_scrolling           (w_edit_t *__edit)
{
  if (!__edit)
    return;

  size_t scrolled      = __edit->scrolled;
  // Relative position of caret
  size_t caret_pos_rel = __edit->caret_pos-scrolled+1;

  // Cursor is far too left
  if (__edit->caret_pos<__edit->scrolled)
    __edit->scrolled=__edit->caret_pos; else

  // Cursor is far too right
  if (caret_pos_rel>=__edit->position.width)
    __edit->scrolled+=caret_pos_rel-__edit->position.width;
}

/**
 * Handles a keydown callback
 *
 * @param __edit - edit received a callback
 * @param __ch - received character
 * @return zero if callback hasn't handled received character
 */
static int
edit_keydown                      (w_edit_t *__edit, wint_t __ch)
{
  _WIDGET_CALL_USER_CALLBACK (__edit, keydown, __edit, __ch);

  switch (__ch)
    {
    // Navigation
    case KEY_LEFT:
      if (__edit->caret_pos>0)
        --__edit->caret_pos;
      break;
    case KEY_RIGHT:
      if (__edit->caret_pos<wcslen (__edit->text.data))
        ++__edit->caret_pos;
      break;
    case KEY_HOME:
      __edit->caret_pos=0;
      break;
    case KEY_END:
      __edit->caret_pos=wcslen (__edit->text.data);
      break;

    case KEY_DELETE:
    case KEY_BACKSPACE:
      {
        size_t i, n;
        n=wcslen (__edit->text.data);

        // If pressed key is a `backspace`, we should
        // move caret left by one position.
        if (__ch==KEY_BACKSPACE)
          {
            // Caret is already in zero position.
            // No chars to delete
            if (!__edit->caret_pos)
              break;

            --__edit->caret_pos;
          } else
            // There are no chars at the right side from cursor
            // so, there are no chars to delete
            if (__edit->caret_pos==n)
              break;

        // Shift text
        for (i=__edit->caret_pos; i<n-1; i++)
          __edit->text.data[i]=__edit->text.data[i+1];

        // Set new null-terminator
        __edit->text.data[n-1]=0;
      }

      break;

    default:

#ifdef SCREEN_NCURSESW
      //
      // FIXME:
      //   We'd better not use specific stuff of terminal
      //   handling in widgets.
      //
      if (is_ncurses_funckey (__ch))
        return FALSE;
#endif

      if (iswprint (__ch))
        {
          // Character is printable, so we have to handle this
          // callback ourselves

          size_t i, len=wcslen (__edit->text.data);

          // Synchronize buffer to make sure that there is
          // enough memory to append character
          edit_sync_buffer (__edit);

          // Shift buffer
          for (i=len; i>__edit->caret_pos; --i)
            __edit->text.data[i]=__edit->text.data[i-1];

          // Insert new character into text
          __edit->text.data[__edit->caret_pos]=__ch;
          __edit->text.data[len+1]=0;

          // Move caret to new position
          __edit->caret_pos++;
        } else
          return FALSE;
    }

  edit_validate_scrolling (__edit);
  widget_redraw (WIDGET (__edit));

  return TRUE;
}

//////
// User's backend

/**
 * Creates new edit box
 *
 * @param __parent - parent of edit. Should be CONTAINER
 * @param __x, __y - coordinates of edit
 * @param __width - width of edit
 *
 * @return pointer to a edit object
 */
w_edit_t*
widget_create_edit                (w_container_t *__parent,
                                   int __x, int __y, int __width)
{
  w_edit_t *res;

  // There is no parent, so we can't create edit
  if (!__parent)
    return 0;

  WIDGET_INIT (res, w_edit_t, WT_EDIT, __parent, WF_NOLAYOUT,
               edit_destructor, edit_drawer,
               __x, __y, 1, __width, 1);

  WIDGET_CALLBACK (res, keydown) = (widget_keydown_proc)edit_keydown;
  WIDGET_CALLBACK (res, focused) = (widget_action)edit_focused;
  WIDGET_CALLBACK (res, blured)  = (widget_action)edit_blured;

  res->font = &FONT (CID_BLACK, CID_CYAN);

  w_edit_set_text (res, L"");

  WIDGET_POST_INIT (res);

  return res;
}

/**
 * Sets text of edit box
 *
 * @param __edit - edit box to set text to
 * @param __text - text to be set
 */
void
w_edit_set_text                   (w_edit_t* __edit, wchar_t *__text)
{
  if (!__edit || !__text)
    return;

  size_t new_len=wcslen (__text)+1;

  if (new_len>__edit->text.allocated)
    {
      // Length of new string is longer than length of allocated buffer,
      // so we need to increase this buffer
      __edit->text.data=realloc (__edit->text.data, new_len*sizeof (wchar_t));
      __edit->text.allocated=new_len;
    }

  wcscpy (__edit->text.data, __text);

  // Move caret to end of text
  __edit->caret_pos=wcslen (__edit->text.data);

  edit_validate_scrolling (__edit);

  widget_redraw (WIDGET (__edit));
}

/**
 * Gets text from edit box
 *
 * @param __edit - edit box from which get the text
 */
wchar_t*
w_edit_get_text                   (w_edit_t* __edit)
{
  if (!__edit)
    return 0;

  return __edit->text.data;
}

/**
 * Sets fonts used in edit
 *
 * @param __edit - for which edit fonts are to be set
 * @param __font - font of default text in normal state
 */
void
w_edit_set_fonts                  (w_edit_t *__edit, scr_font_t *__font)
{
  if (!__edit)
    return;

  WIDGET_SAFE_SET_FONT (__edit, font, __font);

  widget_redraw (WIDGET (__edit));
}
