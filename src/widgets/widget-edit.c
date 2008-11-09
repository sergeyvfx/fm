/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation file of widget edit
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "widget.h"
#include "util.h"

#include <wchar.h>

#define BUFFER_REALLOC_DELTA  16

#define CALL_CHECKVALIDNESS(_edit) \
  (WIDGET_CALL_USER_CALLBACK (_edit, property_changed, \
                              _edit, W_EDIT_CHECKVALIDNESS_PROP))

/* Hotkey context for all edit widgets */
static hotkey_context_t *edit_context = NULL;

/**
 * Destroy list of variants
 *
 * @param __edit - edit box in wgich list of variants will be destroyed
 */
void
free_variants (w_edit_t *__edit)
{
  unsigned long i;

  if (!__edit)
    {
      return;
    }

  for (i = 0; i < __edit->variants.count; ++i)
    {
      free (__edit->variants.strings[i]);
    }

  SAFE_FREE (__edit->variants.strings);

  __edit->variants.count = 0;
}

/**
 * Destroy an edit widget
 *
 * @param __edit - edit to be destroyed
 * @return zero on success, non-zero on failure
 */
static int
edit_destructor (w_edit_t *__edit)
{
  if (!__edit)
    {
      return -1;
    }

  SAFE_FREE (__edit->text.data);

  free_variants (__edit);

  return 0;
}

/**
 * Draw an edit
 *
 * @param __edit - edit to be drawn
 * @return zero on success, non-zero on failure
 */
static int
edit_drawer (w_edit_t *__edit)
{
  int i, printed_len, w, printed_width = 0, dummy;
  size_t len, suffix_len, text_len, cur_printed_len;
  size_t scrolled, caret_pos;

  /* Inherit layout from parent */
  scr_window_t layout = WIDGET_LAYOUT (__edit);

  /* Widget is invisible or there is no layout */
  if (!WIDGET_VISIBLE (__edit) || !layout)
    {
      return -1;
    }

  scr_wnd_attr_backup (layout);

  scr_wnd_move_caret (layout, __edit->position.x, __edit->position.y);

  if (__edit->shaded)
    {
      scr_wnd_font (layout, *__edit->shaded_font);
    }
  else
    {
      scr_wnd_font (layout, *__edit->font);
    }

  /* Calculate length of text which has to be printed */
  text_len = __edit->text.data ? wcslen (__edit->text.data) : 0;
  suffix_len = __edit->suffix ? wcslen (__edit->suffix) : 0;

  scrolled = __edit->scrolled;
  len = text_len + suffix_len;
  w = __edit->position.width;
  caret_pos = __edit->caret_pos;

  printed_len = MIN (len - scrolled, w - (caret_pos - scrolled == w ? 1 : 0));

  /* Print text */
  cur_printed_len = MIN (printed_len, text_len);
  for (i = 0; i < cur_printed_len; i++)
    {
      dummy = wcwidth (__edit->text.data[i + scrolled]);
      if (printed_width + dummy > w)
        {
          /* Printed text will not fit to width of edit box */
          break;
        }
      scr_wnd_add_wchar (layout, __edit->text.data[i + scrolled]);

      if (i + scrolled < __edit->caret_pos)
        {
          caret_pos += dummy - 1;
        }

      printed_width += dummy;
    }

  /* Print suffix */
  scr_wnd_font (layout, *__edit->suffix_font);
  if (printed_len > text_len)
    {
      cur_printed_len = printed_len - text_len;
      for (i = 0; i < cur_printed_len; ++i)
        {
          dummy = wcwidth (__edit->suffix[i]);
          if (printed_width + dummy > w)
            {
              /* Printed text will not fit to width of edit box */
              break;
            }
          scr_wnd_add_wchar (layout, __edit->suffix[i]);
          printed_width += dummy;
        }
    }

  /* Print spaces to make edit needed width */

  /* Need this to make caret be drawing with */
  /* default text font */
  scr_wnd_font (layout, *__edit->font);

  for (i = printed_width; i < __edit->position.width; i++)
    {
      scr_wnd_putch (layout, ' ');
    }

  dummy = __edit->position.x + caret_pos - scrolled;
  scr_wnd_move_caret (layout, dummy, __edit->position.y);

  scr_wnd_attr_restore (layout);

  return 0;
}

/**
 * Handler of `focused` callback (system-based)
 *
 * @param __edit - edit which catched this callback
 * @return zero if callback hasn't handled callback
 */
static int
edit_focused (w_edit_t *__edit)
{
  _WIDGET_CALL_USER_CALLBACK (__edit, focused, __edit);

  /* We shoud show a caret when edit box is focused */
  scr_show_cursor ();

  return 0;
}

/**
 * Handler of `blured` callback (system-based)
 *
 * @param __edit - edit which catched this callback
 * @return zero if callback hasn't handled callback
 */
static int
edit_blured (w_edit_t *__edit)
{
  _WIDGET_CALL_USER_CALLBACK (__edit, blured, __edit);

  /* Assume suffix should be rejected if edit box lost focus */
  __edit->suffix = NULL;
  widget_redraw (WIDGET (__edit));

  /* When edit box is blured we should hide caret */
  scr_hide_cursor ();

  return 0;
}

/**
 * Synchronize allocated buffer length with
 * stored string length
 *
 * @param __edit - edit for which buffer will be synchronized
 */
static void
edit_sync_buffer (w_edit_t *__edit)
{
  /* There is no widget or no allocated buffer */
  if (!__edit || !__edit->text.allocated)
    {
      return;
    }

  size_t txt_len = 0, alloc_len, new_len;

  if (__edit->text.data)
    {
      txt_len = wcslen (__edit->text.data);
    }

  new_len = alloc_len = __edit->text.allocated;

  /*
   * TODO:  Does we really need this checking?
   */
  if (txt_len < alloc_len)
    {
      if (alloc_len - txt_len <= 1)
        {
          /* Allocated buffer is near to be fully used */
          new_len += BUFFER_REALLOC_DELTA;
        }
      else
        if (alloc_len - txt_len > BUFFER_REALLOC_DELTA)
        {
          /* Length of current text is much shorter than allocated */
          /* buffer. It'll be better is we'll truncate buffer to */
          /* save memory */
          new_len = txt_len + BUFFER_REALLOC_DELTA;
        }

      if (new_len != alloc_len)
        {
          /* Realloc buffer */
          __edit->text.data = realloc (__edit->text.data,
                                       new_len * sizeof (wchar_t));
          __edit->text.allocated = new_len;
        }
    }
}

/**
 * Validates data needed for text scrolling in widget
 *
 * @param __edit - edit for which validate scrolling information
 */
static void
edit_validate_scrolling (w_edit_t *__edit)
{
  if (!__edit)
    {
      return;
    }

  size_t scrolled = __edit->scrolled, i;

  /* Relative position of caret */
  size_t caret_pos_rel = __edit->caret_pos - scrolled + 1;

  for (i = scrolled; i < __edit->caret_pos; ++i)
    {
      caret_pos_rel += wcwidth (__edit->text.data[i]) - 1;
    }

  /* Cursor is far too left */
  if (__edit->caret_pos < __edit->scrolled)
    {
      __edit->scrolled = __edit->caret_pos;
    }
  else
    {
      /* Cursor is far too right */
      if (caret_pos_rel >= __edit->position.width)
        {
          __edit->scrolled += caret_pos_rel - __edit->position.width;
        }
    }
}

/**
 * Make text auto-guessing to calculate suffix
 *
 * @param __edit - edit to make guessing on it
 */
static void
make_guessing (w_edit_t *__edit)
{
  wchar_t *prefix;
  unsigned long i;
  size_t len;

  if (!__edit)
    {
      return;
    }

  __edit->suffix = 0;

  prefix = w_edit_get_text (__edit);
  len = wcslen (prefix);

  /* It would be better if we calculate suffix */
  /* for non-empty prefixes only */
  if (len == 0)
    {
      return;
    }

  for (i = 0; i < __edit->variants.count; ++i)
    {
      if (!wcsncmp (prefix, __edit->variants.strings[i], len))
        {
          /* Full equivalent */
          if (len == wcslen (__edit->variants.strings[i]))
            {
              __edit->suffix = NULL;
              break;
            }

          __edit->suffix = __edit->variants.strings[i] + len;
          break;
        }
    }
}

/**
 * Handle a keydown callback
 *
 * @param __edit - edit received a callback
 * @param __ch - received character
 * @return zero if callback hasn't handled received character
 */
static int
edit_keydown (w_edit_t *__edit, wint_t __ch)
{
  _WIDGET_CALL_USER_CALLBACK (__edit, keydown, __edit, __ch);

  size_t spos, sscrolled;
  wchar_t *sbuffer = NULL;
  BOOL rollback_changes = FALSE, sshaded, reject_suffix = FALSE;

  /* Save data for rollback if validation failed */
  spos      = __edit->caret_pos;
  sscrolled = __edit->scrolled;
  sshaded   = __edit->shaded;

  /* Buffer may be unallocated, so we need this checking */
  if (__edit->text.data)
    {
      sbuffer = wcsdup (__edit->text.data);
    }

  switch (__ch)
    {
      /* Navigation */
    case KEY_LEFT:
      if (__edit->caret_pos > 0)
        {
          --__edit->caret_pos;
        }
      __edit->shaded = FALSE;
      break;
    case KEY_RIGHT:
      if (__edit->caret_pos < wcslen (__edit->text.data))
        {
          ++__edit->caret_pos;
        }
      __edit->shaded = FALSE;
      break;
    case KEY_HOME:
      /*
       * NOTE: IMHO using this stupid cycle is much more convenient
       *       than hard-core patching edit_validate_scrolling().
       *       If you'll find a grateful solving of this
       *       problem - you are welcome :)
       */
      while (__edit->caret_pos > 0)
        {
          __edit->caret_pos--;
          edit_validate_scrolling (__edit);
        }
      __edit->shaded = FALSE;
      break;
    case KEY_END:
      while (__edit->caret_pos < wcslen (__edit->text.data))
        {
          __edit->caret_pos++;
          edit_validate_scrolling (__edit);
        }
      __edit->shaded = FALSE;
      break;

    case KEY_DELETE:
    case KEY_BACKSPACE:
      {
        size_t i, n;
        n = wcslen (__edit->text.data);

        /* If pressed key is a `backspace`, we should */
        /* move caret left by one position. */
        if (__ch == KEY_BACKSPACE)
          {
            wchar_t c = __edit->text.data[n - 1], nch;
            int ch_width = wcwidth (c), dummy;

            /* Caret is already in zero position. */
            /* No chars to delete */
            if (!__edit->caret_pos)
              {
                break;
              }

            if (__edit->scrolled > 0)
              {
                /* This dark spell is needed to make cursor to */
                /* at his old position */

                /*
                 * TODO: There is some troubles in this stuff
                 */
                while (__edit->scrolled > 0)
                  {
                    nch = __edit->text.data[__edit->scrolled];
                    dummy = wcwidth (nch);
                    if (ch_width - dummy < 0)
                      {
                        break;
                      }
                    ch_width -= dummy;
                    --__edit->scrolled;
                  }
              }
            --__edit->caret_pos;
          }
        else
          {
            /* There are no chars at the right side from cursor */
            /* so, there are no chars to delete */
            if (__edit->caret_pos == n)
            {
              reject_suffix = TRUE;
              widget_redraw (WIDGET (__edit));
              break;
            }
          }

        /* Shift text */
        for (i = __edit->caret_pos; i < n - 1; i++)
          {
            __edit->text.data[i] = __edit->text.data[i + 1];
          }

        /* Set new null-terminator */
        __edit->text.data[n - 1] = 0;

        /* Send validating message */
        if (CALL_CHECKVALIDNESS (__edit))
          {
            /* We need rollback changes */
            rollback_changes = TRUE;
          }

        __edit->shaded = FALSE;
      }

      break;

    case KEY_ESC:
      /* If user hits Escape button when there is guessed suffix of string */
      /* we should cancel this guessing */
      /* And if user hits Escape button when there is no suffix, */
      /* we should do nothing */
      if (__edit->suffix)
        {
          __edit->suffix = NULL;
          widget_redraw (WIDGET (__edit));
          return TRUE;
        }
      else
        {
          return FALSE;
        }
      break;

    case KEY_RETURN:
      /* If user hits Return button when there is guessed suffix of string */
      /* we should accept guessing string to main buffer */
      /* And if user hits Return button when there is no suffix, */
      /* we should do nothing */
      if (__edit->suffix)
        {
          size_t len = wcslen (__edit->text.data),
                 suff_len = wcslen (__edit->suffix);

          /* Realloc buffer if needed */
          if (len + suff_len >= __edit->text.allocated)
            {
              __edit->text.data = realloc (__edit->text.data,
                                       (len + suff_len + 1) * sizeof (wchar_t));
              __edit->text.allocated = len + suff_len;
            }

          /* Append auto-guessed suffix to main text buffer */
          wcscat (__edit->text.data, __edit->suffix);

          /* Move caret to end of text */
          __edit->caret_pos = len + suff_len;
          edit_validate_scrolling (__edit);

          __edit->suffix = NULL;
          widget_redraw (WIDGET (__edit));
          return TRUE;
        }
      else
        {
          return FALSE;
        }
      break;

    default:

#ifdef SCREEN_NCURSESW
      /*
       * FIXME: We'd better not use specific stuff of terminal
       *        handling in widgets.
       */
      if (is_ncurses_funckey (__ch))
        {
          return FALSE;
        }
#endif

      if (iswprint (__ch))
        {
          /* Character is printable, so we have to handle this */
          /* callback ourselves */

          size_t i, len;

          /* Is edit in shaded state? */
          if (__edit->shaded)
            {
              /* If edit is in shaded state, we should clear text */
              /* and move caret to the beginning. */
              /* We should also reset count of scrolled characters */
              /* and finally, exit edit from shaded state. */

              wcscpy (__edit->text.data, L"");
              __edit->caret_pos = __edit->scrolled = 0;
              __edit->shaded = FALSE;
            }

          len = wcslen (__edit->text.data);

          /* Synchronize buffer to make sure that there is */
          /* enough memory to append character */
          edit_sync_buffer (__edit);

          /* Shift buffer */
          for (i = len; i > __edit->caret_pos; --i)
            {
              __edit->text.data[i] = __edit->text.data[i - 1];
            }

          /* Insert new character into text */
          __edit->text.data[__edit->caret_pos] = __ch;
          __edit->text.data[len + 1] = 0;

          /* Move caret to new position */
          __edit->caret_pos++;

          /* Send validating message */
          if (CALL_CHECKVALIDNESS (__edit))
            {
              /* We need rollback changes */
              rollback_changes = TRUE;
            }
        }
      else
        {
          return FALSE;
        }
    }

  /* If validator rejected buffer.. */
  if (rollback_changes)
    {
      /* ..roll-back changes */
      __edit->caret_pos = spos;
      __edit->scrolled  = sscrolled;
      __edit->shaded    = sshaded;

      /* Reject renewed buffer */
      SAFE_FREE (__edit->text.data);
      if (sbuffer)
        {
          __edit->text.data = sbuffer;
          __edit->text.allocated = wcslen (sbuffer) + 1;

          /* Need this to prevent freeing buffer later */
          sbuffer = NULL;
        }
      else
        {
          __edit->text.allocated = 0;
        }
    }
  else
    {
      if (!reject_suffix)
        {
          make_guessing (__edit);
        }
      else
        {
          __edit->suffix = NULL;
        }
    }

  SAFE_FREE (sbuffer);

  edit_validate_scrolling (__edit);
  widget_redraw (WIDGET (__edit));

  return TRUE;
}

/**
 * Sort strings in variants' list
 *
 * @param __edit - edit box in which strings will be sorted
 */
void
sort_variants (w_edit_t *__edit)
{
  int cmp (const void *__a, const void *__b)
  {
    return wcscmp (*(wchar_t**)__a, *(wchar_t**)__b);
  };
  qsort (__edit->variants.strings, __edit->variants.count,
         sizeof (wchar_t*), cmp);
}

/********
 * User's backend
 */

/**
 * Create new edit box
 *
 * @param __parent - parent of edit. Should be CONTAINER
 * @param __x, __y - coordinates of edit
 * @param __width - width of edit
 *
 * @return pointer to a edit object
 */
w_edit_t*
widget_create_edit (w_container_t *__parent,
                    int __x, int __y, int __width)
{
  w_edit_t *res;

  /* There is no parent, so we can't create edit */
  if (!__parent)
    {
      return 0;
    }

  /* Create context for edit widgets */
  edit_context = hotkey_create_context (L"exit-class-context", 0);

  WIDGET_INIT (res, w_edit_t, WT_EDIT, __parent, WF_NOLAYOUT,
               edit_context,
               edit_destructor, edit_drawer,
               __x, __y, 1, __width, 1);

  WIDGET_CALLBACK (res, keydown) = (widget_keydown_proc) edit_keydown;
  WIDGET_CALLBACK (res, focused) = (widget_action) edit_focused;
  WIDGET_CALLBACK (res, blured) = (widget_action) edit_blured;

  res->font        = FONT (CID_BLACK, CID_CYAN);
  res->shaded_font = FONT (CID_BLUE, CID_CYAN);
  res->suffix_font = FONT (CID_CYAN, CID_BLUE);

  w_edit_set_text (res, L"");

  WIDGET_POST_INIT (res);

  return res;
}

/**
 * Set text of edit box
 *
 * @param __edit - edit box to set text to
 * @param __text - text to be set
 */
void
w_edit_set_text (w_edit_t* __edit, const wchar_t *__text)
{
  if (!__edit || !__text)
    {
      return;
    }

  size_t new_len = wcslen (__text) + 1;

  if (new_len > __edit->text.allocated)
    {
      /* Length of new string is longer than length of allocated buffer, */
      /* so we need to increase this buffer */
      __edit->text.data = realloc (__edit->text.data,
                                   new_len * sizeof (wchar_t));
      __edit->text.allocated = new_len;
    }

  wcscpy (__edit->text.data, __text);

  /* Move caret to end of text */
  __edit->caret_pos = wcslen (__edit->text.data);

  edit_validate_scrolling (__edit);

  widget_redraw (WIDGET (__edit));
}

/**
 * Get text from edit box
 *
 * @param __edit - edit box from which get the text
 */
wchar_t*
w_edit_get_text (w_edit_t* __edit)
{
  if (!__edit)
    {
      return 0;
    }

  return __edit->text.data;
}

/**
 * Set fonts used in edit
 *
 * @param __edit - for which edit fonts are to be set
 * @param __font - font of default text in normal state
 * @param __shaded_font - font of text when edit is in shaded state
 */
void
w_edit_set_fonts (w_edit_t *__edit, scr_font_t *__font,
                  scr_font_t *__shaded_font, scr_font_t *__suffix_font)
{
  if (!__edit)
    {
      return;
    }

  WIDGET_SAFE_SET_FONT (__edit, font, __font);
  WIDGET_SAFE_SET_FONT (__edit, shaded_font, __shaded_font);
  WIDGET_SAFE_SET_FONT (__edit, suffix_font, __suffix_font);

  widget_redraw (WIDGET (__edit));
}

/**
 * Set shaded state of specified edit widget
 *
 * @param __edit - edit widget where shade state will be changed
 * @param __shaded - new state of shading
 */
void
w_edit_set_shaded (w_edit_t *__edit, BOOL __shaded)
{
  __edit->shaded = __shaded;
  widget_redraw (WIDGET (__edit));
}

/**
 * Get shaded state of specified edit widget
 *
 * @param __edit - edit widget where shade state will be changed
 * @return non-zero if edit is shaded, non-zero otherwise
 */
BOOL
w_edit_get_shaded (w_edit_t *__edit)
{
  return __edit->shaded;
}

/**
 * Set variants list for auto-guessing stuff
 *
 * @param __edit - edit box for which variants will be set
 * @param __strings - list of variants
 * @param __count - count of elements in list
 */
void
w_edit_set_variants (w_edit_t *__edit, wchar_t **__strings,
                     unsigned long __count)
{
  unsigned long i;

  if (!__edit || !__strings)
    {
      return;
    }

  /* Free previously used variants */
  free_variants (__edit);

  __edit->variants.strings = malloc (__count * sizeof (wchar_t*));
  for (i = 0; i < __count; ++i)
    {
      __edit->variants.strings[i] = wcsdup (__strings[i]);
    }
  __edit->variants.count = __count;

  sort_variants (__edit);

  /* We should to re-new suffix and redraw widget */
  make_guessing (__edit);
  widget_redraw (WIDGET (__edit));
}

/**
 * Add variants to list for auto-guessing stuff
 *
 * @param __edit - edit box in which variant will be added
 * @param __string - variant to add
 */
void
w_edit_add_variant (w_edit_t *__edit, wchar_t *__string)
{
  if (!__edit || !__string)
    {
      return;
    }

  __edit->variants.strings = realloc (__edit->variants.strings,
                                       (__edit->variants.count + 1) *
                                           sizeof (wchar_t*));
  __edit->variants.strings[__edit->variants.count] = wcsdup (__string);
  ++__edit->variants.count;
  sort_variants (__edit);
}

