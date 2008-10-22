/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of action 'Change mode'
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "action-chmod-iface.h"
#include "actions.h"
#include "i18n.h"
#include "screen.h"
#include "util.h"

#include <widgets/widget.h>
#include <wchar.h>
#include <ctype.h>

#define _CREATE_CHECKBOX(_bit, _x, _y, _text) \
  res[11 - (_bit)] = create_checkbox (__window, __bitmask, __mask,      \
                                       __unknown_mask, _bit, _x, _y, _text)


/* Information, associated with checkbox */
typedef struct
{
  /* Text for displayig checkbox's state is changed */
  w_text_t *text;

  /* Number of bit which is managed by this checkbox */
  unsigned int manage_bit;

  /* Initial state of checkbox */
  BOOL initial_state;

  /* Edit box where numeric bitmask is entering */
  w_edit_t *bitmask;

  /* Need for correct synchronizing mode in editbox and checkboxes */
  BOOL not_update_text;
} checkbox_info_t;

/********
 * Implementation
 */

/**
 * Convert mode mask to alpha
 *
 * @param __mask - mask of set bits
 * @param __unknown_mask - mask of bits which set is unknown
 * @param __out - buffer to store result
 */
static void
mask2alpha (unsigned int __mask, unsigned int __unknown_mask, wchar_t *__out)
{
  wchar_t buf[5] = {0};
  short i;

  __mask = __mask & ~__unknown_mask;

  for (i = 0; i < 4; ++i)
    {
      buf[3 - i] = (__mask >> (i * 3)) % 8 + '0';
    }

  wcscpy (__out, buf);
}

/**
 * Convert alpha bit mask to number
 *
 * @param __mask - mask to convert
 * @return converted bit mask
 */
static unsigned int
alpha2mask (const wchar_t *__mask)
{
  unsigned int res = 0;
  size_t i, n;

  n = wcslen (__mask);
  for (i = 0; i < n; ++i)
    {
      res = res * 8 + __mask[i] - '0';
    }

  return res;
}

/**
 * Handler of property changed callback for checkboxes
 *
 * @param __checkbox - describes whose property has been changed
 * @param __prop - code of property which has been changed
 * @return non-zero if callback is handled, zero otherwise
 */
static int
checkbox_property_changed (w_checkbox_t *__checkbox, int __prop)
{
  if (__prop == W_CHECKBOX_CHECKED_PROP)
    {
      checkbox_info_t *info = WIDGET_USER_DATA (__checkbox);
      BOOL state = w_checkbox_get (__checkbox);
      w_edit_t *edt;
      unsigned int mask, bit;
      wchar_t buf[32];

      /* Change checkbox's changed indicator */
      if (info->initial_state != state && state != WCB_STATE_UNDEFINED)
        {
          w_text_set (info->text, L"*");
        }
      else
        {
          w_text_set (info->text, L"");
        }

      if (!info->not_update_text)
        {
          /* Update text in bitmask edit box */
          edt = info->bitmask;
          bit = 1 << info->manage_bit;
          mask = alpha2mask (w_edit_get_text (edt));

          if (state == TRUE)
            {
              mask |= bit;
            }
          else
            {
              mask &= ~bit;
            }

          mask2alpha (mask, 0, buf);
          w_edit_set_text (edt, buf);
        }

      return TRUE;
    }

  return FALSE;
}

static w_checkbox_t*
create_checkbox (w_window_t *__window, w_edit_t *__bitmask,
                 unsigned int __mask, unsigned int __unknown_mask,
                 unsigned int __bit, int __x, int __y,
                 const wchar_t *__caption)
{
  checkbox_info_t *info;
  w_checkbox_t *cb;
  int state;

  state = WCB_STATE_UNDEFINED;
  if ((__unknown_mask >> __bit) % 2 == 0)
    {
      /* If bit is known */
      state = (__mask >> __bit) % 2;
    }

  /* Create information, associated with checkbox */
  MALLOC_ZERO (info, sizeof (checkbox_info_t));
  info->initial_state = state;
  info->text = widget_create_text (WIDGET_CONTAINER (__window), L"",
                                   __x - 1, __y);
  info->manage_bit = __bit;
  info->bitmask = __bitmask;

  /* Create checkbox */
  cb = widget_create_checkbox (WIDGET_CONTAINER (__window), __caption,
                               __x, __y, state, WCBS_WITH_UNDEFINED);
  WIDGET_USER_CALLBACK (cb, property_changed) =
    (widget_propchanged_proc)checkbox_property_changed;
  WIDGET_USER_DATA (cb) = info;

  return cb;
}

/**
 * Create checkboxes for mode bits
 *
 * @param __window - for which window check boxes will be belongs to
 * @param __bitmask - edit box where bitmask is entering
 * @param __mask - mask of set bits
 * @param __unknown_mask - mask of bits which set is unknown
 * @return array of checkboxes
 */
static w_checkbox_t**
create_checkboxes (w_window_t *__window, w_edit_t *__bitmask,
                   unsigned int __mask, unsigned int __unknown_mask)
{
  int offset, offset_left = 0, i, width, j, bit, left;
  w_checkbox_t **res = NULL;
  wchar_t *pchar;
  wchar_t *header_column[] = {L"Owner", L"Group", L"Other", NULL};
  wchar_t *header_row[] = {L"Read", L"Write", L"Execute", NULL};

  MALLOC_ZERO (res, 13 * sizeof (w_checkbox_t*));

  offset = 4;

  /* Create highest bits */
  _CREATE_CHECKBOX (11, 4, offset++, _(L"Set user ID on execution"));
  _CREATE_CHECKBOX (10, 4, offset++, _(L"Set group ID on execution"));
  _CREATE_CHECKBOX (9,  4, offset++, _(L"Sticky bit"));

  offset += 2;

  /**
   * Create checkboxes for permissions matrix
   */

  /* Create captions */
  i = 0;
  while (header_column[i])
    {
      pchar = _(header_column[i]);
      offset_left = MAX (offset_left, wcswidth (pchar, wcslen (pchar)) + 4);
      widget_create_text (WIDGET_CONTAINER (__window), pchar,  2, offset + i);
      ++i;
    }

  j = 0;
  left = offset_left;
  while (header_row[j])
    {
      pchar = _(header_row[j]);
      widget_create_text (WIDGET_CONTAINER (__window), pchar,
                          left, offset - 1);
      left += wcswidth (pchar, wcslen (pchar)) + 2;
      ++j;
    }

  i = 0;
  bit = 8;
  while (header_column[i])
    {
      j = 0;
      left = offset_left;
      while (header_row[j])
        {
          pchar = _(header_row[j]);
          width = wcswidth (pchar, wcslen (pchar));
          _CREATE_CHECKBOX (bit, left + (width - 3) / 2, offset, L"");

          left += width + 2;
          --bit;
          ++j;
        }

      ++i;
      ++offset;
    }

  return res;
}

/**
 * Return dimensions of dialog window
 *
 * @param __recursively - is recursively chmoding is allowed
 * @return dialog's dimensions
 */
static widget_position_t
get_dialog_dimensions (BOOL __recursively)
{
  widget_position_t res;
  int i = 0;
  static BOOL initialized = FALSE;
  static int width, height;

  if (!initialized)
    {
      wchar_t *header_column[] = {L"Owner", L"Group", L"Other", NULL};
      wchar_t *header_row[] = {L"Read", L"Write", L"Execute", NULL};
      wchar_t *nonmatrix_strings[] = {L"Set user ID on execution",
                                      L"Set group ID on execution",
                                      L"Sticky bit"};
      wchar_t *pchar;
      int w, t;

      /*
       * NOTE: Dimensions of dialog is based on set of different captions
       *       which can't be changed during working. So we could
       *       use pre-calculation.
       */

      width = wcswidth (_(L"Permissions"), wcslen (_(L"Permissions"))) + 6;

      width = MAX (width, wcswidth (_(L"Ok"), wcslen (_(L"Ok"))) +
                   wcswidth (_(L"Cancel"), wcslen (_(L"Cancel"))) + 13);
      width = MAX (width,
                   wcswidth (_(L"Enter octal permissions:"),
                             wcslen (_(L"Enter octal permissions:"))) + 10);
      width = MAX (width, wcswidth (_(L"Additional options:"),
                                    wcslen (_(L"Additional options:"))));
      width = MAX (width, widget_shortcut_length (L"_Recursively") + 6);

      height = 15;

      /* Calculate width of matrix */
      w = 0;
      i = 0;
      while (header_column[i])
        {
          pchar = _(header_column[i]);
          t = wcswidth (pchar, wcslen (pchar)) + 4;
          w = MAX (w, t);
          ++i;
        }

      i = 0;
      while (header_row[i])
        {
          pchar = _(header_row[i]);
          w += wcswidth (pchar, wcslen (pchar)) + 2;
          ++i;
        }

      width = MAX (width, w);

      /* Calculate width of non-matrix texts */
      i = 0;
      while (nonmatrix_strings[i])
        {
          pchar = _(nonmatrix_strings[i]);
          t = wcswidth (pchar, wcslen (pchar)) + 10;
          width = MAX (width, t);
          ++i;
        }

      initialized = TRUE;
    }

  res.width = width;
  res.height = height;

  /* Ability of recursively chmoding is not constant */
  /* so we can't use this stuff in pre-calculating */
  if (__recursively)
    {
      res.height += 3;
    }

  return res;
}

/**
 * Destroy array of checkboxes
 *
 * @param __array - array to destroy
 */
static void
destroy_checkboxes_array (w_checkbox_t **__array)
{
  int i = 0;

  /*
   * NOTE: Widget's will be destroyed with dialog.
   *       So, we just only need to destroy information associated with
   *       checkboxes and free memory used by array.
   */

  /* Free information associated with checkboxes */
  while (__array[i])
    {
      free (WIDGET_USER_DATA (__array[i]));
      ++i;
    }

  /* Free memory used by array */
  free (__array);
}

/**
 * Handler of keydown callback for editbox where octal mask is editiong
 *
 * @param __edit - descriptor of editbox
 * @param __ch - pressed key
 * @return non-zero if callback is handled, zero otherwise
 */
static int
bitmask_edt_keydown (w_edit_t *__edit, wint_t __ch)
{
  if (isprint (__ch))
    {
      /* Deny entering printable non-numbers */
      /* And set limit on text's length */
      if (__ch < '0' || __ch > '9' ||
            (wcslen (w_edit_get_text (__edit)) >= 4 &&
            !w_edit_get_shaded (__edit))
         )
        {
          return TRUE;
        }
    }

  return FALSE;
}

/**
 * Handler of property changed callback for bitmask editor
 *
 * @param __edit - describes whose property has been changed
 * @param __prop - code of property which has been changed
 * @return non-zero if callback is handled, zero otherwise
 */
static int
bitmask_edt_property_changed (w_edit_t *__edit, int __prop)
{
  if (__prop == W_EDIT_CHECKVALIDNESS_PROP)
    {
      unsigned int mode, bit;
      int i = 0;
      w_checkbox_t **arr;
      checkbox_info_t *info;
      BOOL checked;
      mode = alpha2mask (w_edit_get_text (__edit));

      /* Check for validness */
      if (mode > 07777)
        {
          return TRUE;
        }

      /* Synchronize checkboxes with value in edit box */
      arr = WIDGET_USER_DATA (__edit);
      while (arr[i] != NULL)
        {
          info = WIDGET_USER_DATA (arr[i]);

          bit = 1 << info->manage_bit;
          checked = (mode & bit) ? TRUE : FALSE;

          info->not_update_text = TRUE;
          w_checkbox_set (arr[i], checked);
          info->not_update_text = FALSE;

          ++i;
        }
    }
  return FALSE;
}

/**
 * Update masks of set and unknown bits from array of checkboxes
 *
 * @param __arr - array of check boxes
 * @param __mask - mask of set bits of mode
 * @param __unknown_mask - mask of unknown bits in mode
 */
static void
update_masks (w_checkbox_t **__arr, unsigned int *__mask,
              unsigned int *__unknown_mask)
{
  int i, bit, state;
  checkbox_info_t *info;

  /* Reset bits */
  (*__mask) = 0;
  (*__unknown_mask) = 0;

  i = 0;
  while (__arr[i])
    {
      info = WIDGET_USER_DATA (__arr[i]);
      state = w_checkbox_get (__arr[i]);
      bit = 1 << info->manage_bit;

      if (state)
        {
          /* Set or unknown */
          if (state == TRUE)
            {
              (*__mask) |= bit;
            }
          else
            {
              (*__unknown_mask) |= bit;
            }
        }

      ++i;
    }
}

/********
 * User's backend
 */

/**
 * Show dialog with different chmod options
 *
 * @param __mask - mask of set bits
 * @param __unknown_mask - mask of bits which set is unknown
 * @param __rec - used to determine if it s able to make recursive chown
 * and user's decision will be written here
 * @return zero on success, non-zero otherwise
 */
int
action_chmod_show_dialog (unsigned int *__mask, unsigned int *__unknown_mask,
                          BOOL *__rec)
{
  w_window_t *wnd;
  widget_position_t dim;
  w_checkbox_t **cb_arr;
  w_checkbox_t *recursively;
  w_edit_t *octal;
  wchar_t *pchar, buf[64];
  int dummy, res;

  dim = get_dialog_dimensions (*__rec);

  wnd = widget_create_window (_(L"Permissions"), 0, 0,
                              dim.width, dim.height, WMS_CENTERED);

  /* Create edit box to enter permission by number */
  pchar = _(L"Enter octal permissions:");
  widget_create_text (WIDGET_CONTAINER (wnd), pchar, 1, 1);
  dummy = wcswidth (pchar, wcslen (pchar));
  octal = widget_create_edit (WIDGET_CONTAINER (wnd),
                              dummy + 2, 1, wnd->position.width - dummy - 3);
  WIDGET_USER_CALLBACK (octal, keydown) =
    (widget_keydown_proc)bitmask_edt_keydown;
  WIDGET_USER_CALLBACK (octal, property_changed) =
    (widget_propchanged_proc)bitmask_edt_property_changed;

  mask2alpha (*__mask, *__unknown_mask, buf);
  w_edit_set_text (octal, buf);
  w_edit_set_shaded (octal, TRUE);

  /* Create checkboxes to manage permissions' bits */
  widget_create_text (WIDGET_CONTAINER (wnd),
                      _(L"Or use checkboxes to build it:"), 1, 3);
  cb_arr = create_checkboxes (wnd, octal, *__mask, *__unknown_mask);

  WIDGET_USER_DATA (octal) = cb_arr;

  if (*__rec)
    {
      widget_create_text (WIDGET_CONTAINER (wnd), _(L"Additional options:"),
                          1, wnd->position.height - 5);
      recursively = widget_create_checkbox (WIDGET_CONTAINER (wnd),
                                            _(L"_Recursively"),
                                            4, wnd->position.height - 4,
                                            FALSE, 0);
    }

  /* Create buttons */
  action_create_ok_cancel_btns (wnd);

  /* Show window */
  res = w_window_show_modal (wnd);

  if (res == MR_OK)
    {
      update_masks (cb_arr, __mask, __unknown_mask);
      if (*__rec)
        {
          (*__rec) = w_checkbox_get (recursively);
        }
      res = ACTION_OK;
    }
  else
    {
      res = ACTION_ABORT;
    }

  /* Free used memory */
  destroy_checkboxes_array (cb_arr);
  widget_destroy (WIDGET (wnd));

  return res;
}
