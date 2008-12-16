/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Interface part of find operation
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "action-find-iface.h"
#include "actions.h"
#include "i18n.h"
#include "messages.h"

#include <widgets/widget.h>

#define _GET_CHECKED(__const, __DEF_VALUE) \
  (__options->filled ? TEST_FLAG (__options->flags, __const) : (__DEF_VALUE))

#define _CHECK_CHECKBOX(__cb, __set_flag) \
  { \
    if (w_checkbox_get (__cb)) \
      { \
        SET_FLAG (flags, (__set_flag)); \
      } \
  }

typedef struct {
  w_edit_t *edt_mask, *edt_content;
  w_checkbox_t *cb_re_mask, *cb_re_content;
} opt_wnd_data_t;

/**
 * Check if specified string is a valid regular expression
 *
 * @param __regexp - string to check
 * @return zero if given string is not a valid regular epression,
 * non-zero otherwise
 */
static BOOL
check_regexp (const wchar_t *__regexp)
{
  wchar_t *dummy;
  size_t len;
  regexp_t *re;

  if (!__regexp || !*__regexp)
    {
      return TRUE;
    }

  len = wcslen (__regexp) + 2;
  dummy = malloc ((len + 1) * sizeof (wchar_t));
  swprintf (dummy, len, L"/%s/", __regexp);

  re = wregexp_compile (dummy);
  free (dummy);

  if (re)
    {
      regexp_free (re);
      return TRUE;
    }

  return FALSE;
}

/**
 * Handler of property_changed() callback
 *
 * @param __window - window received this callback
 * @param __prop - code of changed property
 * @return result code
 */
static int
window_property_changed (void *__window, int __prop)
{
  w_window_t *window = __window;

  if (__prop == W_WINDOW_CONFIRMHIDE_PROP)
    {
      if (window->modal_result == MR_OK)
        {
          opt_wnd_data_t *opts = WIDGET_USER_DATA (__window);

          if (w_checkbox_get (opts->cb_re_mask))
            {
              if (!check_regexp (w_edit_get_text (opts->edt_mask)))
                {
                  MESSAGE_ERROR (_(L"Invalid regular expression in "
                                    "file mask"));
                  widget_set_focus (WIDGET (opts->edt_mask));
                  return 1;
                }
            }

          if (w_checkbox_get (opts->cb_re_content))
            {
              if (!check_regexp (w_edit_get_text (opts->edt_content)))
                {
                  MESSAGE_ERROR (_(L"Invalid regular expression in "
                                    "file content"));
                  widget_set_focus (WIDGET (opts->edt_content));
                  return 1;
                }
            }
        }
    }

  return 0;
}

/**
 * Handler for results window's button clicked
 *
 * @param __button - descriptor of button which has been clicked
 * @return non-zero if action has been handled, zero otherwise
 */
static int
on_button_click (w_button_t *__button)
{
  w_window_t *wnd;

  wnd = WIDGET_USER_DATA (__button);

  if (wnd->show_mode == WSM_MODAL)
    {
      return FALSE;
    }

  w_window_end_modal (wnd, WIDGET_BUTTON_MODALRESULT (__button));

  return TRUE;
}

/**
 * Handler of keydown message for widgets on result window
 *
 * @param __widget - widget on which key was pressed
 * @param __ch - code of pressed key
 */
static int
button_keydown (widget_t *__widget, wint_t __ch)
{
  w_window_t *wnd;
  int modal_result = 0;

  if (!__widget || !WIDGET_USER_DATA (__widget))
    {
      return 1;
    }

  wnd = WIDGET_USER_DATA (__widget);

  if (wnd->show_mode == WSM_MODAL)
    {
      return FALSE;
    }

  if (__ch == KEY_ESC)
    {
      modal_result = MR_CANCEL;
    }
  else if (__ch == KEY_RETURN)
    {
      modal_result = AF_GOTO;
    }

  if (modal_result)
    {
      WIDGET_USER_DATA (__widget) = NULL;
      w_window_end_modal (wnd, modal_result);
      return 0;
    }

  return 0;
}

/********
 * User's backend
 */

/**
 * Show dialog with different find options
 *
 * @param __options - pointer to a structure where options will be stored
 * @return zero on success, non-zero otherwise
 */
int
action_find_show_dialog (action_find_options_t *__options)
{
  w_window_t *wnd;
  w_container_t *cnt;
  int middle, res;
  BOOL checked;

  opt_wnd_data_t wnd_data;

  w_edit_t *edt_file_mask, *edt_file_content, *edt_start_at;
  w_checkbox_t *cb_file_mask_regexp, *cb_file_mask_case_sens;
  w_checkbox_t *cb_content_regexp, *cb_content_case_sens;
  w_checkbox_t *cb_find_recursive, *cb_follow_symlinks;
  w_checkbox_t *cb_find_directories;

  wnd = widget_create_window (_(L"Find file"), 0, 0, 60, 15, WMS_CENTERED);
  cnt = WIDGET_CONTAINER (wnd);

  middle = wnd->position.width / 2 - 2;

  /* Create widgets for file masks */
  widget_create_text (cnt, _(L"A file mask or several file masks:"), 1, 1);
  edt_file_mask = widget_create_edit (cnt, 1, 2, cnt->position.width - 2);

  checked = _GET_CHECKED (AFF_MASK_REGEXP, FALSE);
  cb_file_mask_regexp = widget_create_checkbox (cnt, _(L"R_egular expression"),
                                                1, 3, checked, 0);
  checked = _GET_CHECKED (AFF_MASK_CASE_SENSITIVE, FALSE);
  cb_file_mask_case_sens = widget_create_checkbox (cnt, _(L"Case _sensitive"),
                                                   middle + 1, 3, checked, 0);
  w_edit_set_text (edt_file_mask,
                   __options->filled ? __options->file_mask :  L"*");
  w_edit_set_shaded (edt_file_mask, TRUE);

  /* Create widgets for file content */
  widget_create_text (cnt, _(L"Containing text:"), 1, 5);
  edt_file_content = widget_create_edit (cnt, 1, 6,
                                         middle - 1);

  checked = _GET_CHECKED (AFF_CONTENT_REGEXP, FALSE);
  cb_content_regexp = widget_create_checkbox (cnt, _(L"_Regular expression"),
                                              1, 7, checked, 0);

  checked = _GET_CHECKED (AFF_CONTENT_CASE_SENSITIVE, FALSE);
  cb_content_case_sens = widget_create_checkbox (cnt, _(L"_Case sensitive"),
                                                 1, 8, checked, 0);

  w_edit_set_text (edt_file_content,
                   __options->content ? __options->content :  L"");
  w_edit_set_shaded (edt_file_content, TRUE);

  /* Create widgets for start directory */
  widget_create_text (cnt, _(L"Start at:"), middle + 1, 5);
  edt_start_at = widget_create_edit (cnt, middle + 1, 6,
                                     wnd->position.width - middle - 2);

  checked = _GET_CHECKED (AFF_FIND_RECURSIVELY, TRUE);
  cb_find_recursive = widget_create_checkbox (cnt, _(L"Rec_ursively"),
                                              middle + 1, 7, checked, 0);
  w_edit_set_text (edt_start_at,
                   __options->start_at ? __options->start_at :  L".");
  w_edit_set_shaded (edt_start_at, TRUE);

  /* Misc. options */
  widget_create_text (cnt, _(L"Options:"), 1, 10);

  checked = _GET_CHECKED (AFF_FOLLOW_SYMLINKS, TRUE);
  cb_follow_symlinks = widget_create_checkbox (cnt, _(L"_Follow symlinks"),
                                               1, 11, checked, 0);

  checked = _GET_CHECKED (AFF_FIND_DIRECTORIES, FALSE);
  cb_find_directories = widget_create_checkbox (cnt, _(L"F_ind directories"),
                                                middle + 1, 11, checked, 0);

  /* Create buttons */
  action_create_ok_cancel_btns (wnd);

  wnd_data.edt_mask = edt_file_mask;
  wnd_data.edt_content = edt_file_content;
  wnd_data.cb_re_mask = cb_file_mask_regexp;
  wnd_data.cb_re_content = cb_content_regexp;

  /* Quite dangerous */
  WIDGET_USER_DATA(wnd) = &wnd_data;

  WIDGET_USER_CALLBACK (wnd, property_changed) = window_property_changed;

  res = w_window_show_modal (wnd);

  if (res == MR_OK)
    {
      unsigned long flags = 0;

      SAFE_FREE (__options->file_mask);
      SAFE_FREE (__options->content);
      SAFE_FREE (__options->start_at);

      /* Store options to a structure */
      __options->file_mask = wcsdup (w_edit_get_text (edt_file_mask));
      __options->content   = wcsdup (w_edit_get_text (edt_file_content));
      __options->start_at  = wcsdup (w_edit_get_text (edt_start_at));

      _CHECK_CHECKBOX (cb_file_mask_regexp,    AFF_MASK_REGEXP);
      _CHECK_CHECKBOX (cb_file_mask_case_sens, AFF_MASK_CASE_SENSITIVE);
      _CHECK_CHECKBOX (cb_content_regexp,      AFF_CONTENT_REGEXP);
      _CHECK_CHECKBOX (cb_content_case_sens,   AFF_CONTENT_CASE_SENSITIVE);
      _CHECK_CHECKBOX (cb_follow_symlinks,     AFF_FOLLOW_SYMLINKS);
      _CHECK_CHECKBOX (cb_find_recursive,      AFF_FIND_RECURSIVELY);
      _CHECK_CHECKBOX (cb_find_directories,    AFF_FIND_DIRECTORIES);

      __options->flags = flags;

      res = ACTION_OK;
    }
  else
    {
      res = ACTION_ABORT;
    }

  widget_destroy (WIDGET (wnd));

  return res;
}

/**
 * Create window for find results
 *
 * @return descriptor of window where find results where stored
 * @sideeffect allocate memory for return value.
 * Use action_find_free_res_wnd() to free memory.
 */
action_find_res_wnd_t*
action_find_create_res_wnd (void)
{
  action_find_res_wnd_t *result;
  w_container_t *cnt;
  w_button_t *buttons_desc[3];
  int i, count;

  static action_button_t buttons[] = {
    {L"_New search", AF_FIND_AGAIN, FALSE},
    {L"_Go to",      AF_GOTO,       TRUE},
    {L"_Cancel",     MR_CANCEL,     FALSE}
  };

  MALLOC_ZERO (result, sizeof (action_find_res_wnd_t));

  result->window = widget_create_window (_(L"Find file"),
                                         0, 0,
                                         SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4,
                                         WMS_CENTERED);
  cnt = WIDGET_CONTAINER (result->window);

  result->list = widget_create_list (cnt, _(L"Found files"), 1, 1,
                                     cnt->position.width - 2,
                                     cnt->position.height - 5);

  result->status = widget_create_text (cnt, L"",
                                       1, cnt->position.height - 4);

  count = sizeof (buttons) / sizeof (action_button_t);

  action_create_buttons (result->window, buttons, count, buttons_desc);

  WIDGET_USER_DATA (result->list) = result->window;
  WIDGET_USER_CALLBACK (result->list, keydown) =
    (widget_keydown_proc)button_keydown;

  for (i = 0; i < count; ++i)
    {
      WIDGET_USER_DATA (buttons_desc[i]) = result->window;
      WIDGET_USER_CALLBACK (buttons_desc[i], clicked) =
        (widget_action)on_button_click;
    }

  return result;
}

/**
 * Destroy window for find results
 *
 * @param __wnd - window to be destroyed
 */
void
action_find_destroy_res_wnd (action_find_res_wnd_t *__wnd)
{
  widget_destroy (WIDGET (__wnd->window));
  free (__wnd);
}
