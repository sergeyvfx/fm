/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Interface part of action 'Change owner'
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "action-chown-iface.h"
#include "actions.h"
#include "i18n.h"
#include "usergroup.h"
#include "messages.h"

#include <widgets/widget.h>

#define CHECK_ERROR(__text) \
  { \
    res = message_box (_(L"Error"), __text, MB_OKCANCEL | MB_CRITICAL); \
    if (res == MB_OK) \
      { \
        return ACTION_ERR; \
      } \
    else \
      { \
        return ACTION_ABORT; \
      } \
  }

/* Enable navigation in list when up/down arrow key */
/* pressed when edit box is focused */
#define LIST_NAVIGATE_FROM_EDIT

/**
 * Fill specified list with user names
 *
 * @param __list - list to fill
 * @param __id - id of active user
 */
static void
fill_users_list (w_list_t *__list, int __id)
{
  passwd_t **list;
  int count;

  /* Get list of users */
  if ((count = get_users_list (&list)) > 0)
    {
      int i;

      /* Sort list */
      int cmp (const void *__a, const void *__b)
      {
        passwd_t *a = *(passwd_t**)__a, *b = *(passwd_t**)__b;
        return wcscmp (a->name, b->name);
      };
      qsort (list, count, sizeof (passwd_t*), cmp);

      for (i = 0; i < count; ++i)
	{
	  w_list_append_item (__list, list[i]->name, 0);

          w_edit_add_variant (WIDGET_USER_DATA (__list), list[i]->name);

          /* Set selection to list item */
          /* and set text of edit box */
          if (list[i]->uid == __id)
            {
              w_list_set_selected (__list, i);

              w_edit_set_text (WIDGET_USER_DATA (__list), list[i]->name);
              w_edit_set_shaded (WIDGET_USER_DATA (__list), TRUE);
            }
	}
      free_users_list (list, count);
    }
}

/**
 * Fill specified list with group names
 *
 * @param __list - list to fill
 * @param __id - id of active group
 */
static void
fill_groups_list (w_list_t *__list, int __id)
{
  group_t **list;
  int count;

  /* Get list of users */
  if ((count = get_groups_list (&list)) > 0)
    {
      int i;

      /* Sort list */
      int cmp (const void *__a, const void *__b)
      {
        group_t *a = *(group_t**)__a, *b = *(group_t**)__b;
        return wcscmp (a->name, b->name);
      };
      qsort (list, count, sizeof (group_t*), cmp);

      for (i = 0; i < count; ++i)
	{
	  w_list_append_item (__list, list[i]->name, 0);

          w_edit_add_variant (WIDGET_USER_DATA (__list), list[i]->name);

          /* Set selection to list item */
          /* and set text of edit box */
          if (list[i]->gid == __id)
            {
              w_list_set_selected (__list, i);

              w_edit_set_text (WIDGET_USER_DATA (__list), list[i]->name);
              w_edit_set_shaded (WIDGET_USER_DATA (__list), TRUE);
            }
	}
      free_groups_list (list, count);
    }
}

/**
 * Callback for property_changed event from list
 *
 * @param __prop - code of changed property
 * @return zero on success, non-zero otherwise
 */
static int
list_property_changed (void *__list, int __prop)
{
  w_list_t *list = __list;

  if (__prop == W_LIST_ITEMINDEX_PROP)
    {
      /* Set text in edit box associated with this list */

      /* Get text of item */
      if (w_list_get_current_item (list))
	{
          wchar_t *text = w_list_get_current_item (list)->text;

          if (text)
            {
              /* Get associated edit box */
              w_edit_t *edt = WIDGET_USER_DATA (__list);

              w_edit_set_text (edt, text);
              w_edit_set_shaded (edt, TRUE);
            }
        }
    }

  return 0;
}

/**
 * Check validness text in edit box
 *
 * @param __edit - edit box which text will be checked
 * @return zero if text is valid, non-zero otherwise
 */
static int
check_text_validness (w_edit_t *__edit)
{
  wchar_t *text = w_edit_get_text (__edit), *item;
  w_list_t *list;
  int i, count;
  size_t len;

  if (!text)
    {
      /* Assume NULL text is valid */
      return 0;
    }

  list = WIDGET_USER_DATA (__edit);
  len = wcslen (text);

  /* Now we need overview items in list and check */
  /* if `text` if a prefix of some line */

  for (i = 0, count = w_list_items_count (list); i < count; ++i)
    {
      item = w_list_get_item (list, i)->text;
      if (wcsncmp (text, item, len) == 0)
        {
          /* Inputted text is a valid prefix of user or group name */
          /* So text is valid and we may scroll list to current item */
          /* and return success */

          w_list_set_selected (list, i);

          return 0;
        }
    }

  return -1;
}

/**
 * Handler of property_changed() callback
 *
 * @param __edit - edit box received this callback
 * @param __prop - code of changed property
 * @return result code
 */
static int
edit_property_changed (void *__edit, int __prop)
{
  if (__prop == W_EDIT_CHECKVALIDNESS_PROP)
    {
      return check_text_validness (__edit);
    }

  return 0;
}

/**
 * Check values of user and group specified by user
 *
 * @param __edt_user - edit box with user name
 * @param __edt_group - edit box with group name
 * @return zero if there are no errors, non-zero otherwise
 */
static int
check_values (w_edit_t *__edt_user, w_edit_t *__edt_group)
{
  passwd_t *pw;
  group_t *gr;
  int res;

  /* Check validness of user name */
  pw = get_user_info_by_name (w_edit_get_text (__edt_user));
  if (!pw)
    {
      widget_set_focus (WIDGET (__edt_user));

      /* There is no user with such name */
      CHECK_ERROR (_(L"Invalid name of user"));
    }
  free_user_info (pw);

  /* Check validness of user name */
  gr = get_group_info_by_name (w_edit_get_text (__edt_group));
  if (!gr)
    {
      widget_set_focus (WIDGET (__edt_group));

      /* There is no user with such name */
      CHECK_ERROR (_(L"Invalid name of group"));
    }
  free_group_info (gr);

  return 0;
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
          w_edit_t **editboxes;
          editboxes = WIDGET_USER_DATA (__window);

          if (check_values (editboxes[0], editboxes[1]))
            {
              return 1;
            }
        }
    }

  return 0;
}

/**
 * Get user's id by his name
 *
 * @param __name - name of user
 * @return user's id
 */
static int
user_id (wchar_t *__name)
{
  passwd_t *pw;
  int res;

  pw = get_user_info_by_name (__name);

  if (!pw)
    {
      return -1;
    }

  res = pw->uid;

  free_user_info (pw);

  return res;
}

/**
 * Get group's id by his name
 *
 * @param __name - name of group
 * @return group's id
 */
static int
group_id (wchar_t *__name)
{
  group_t *gr;
  int res;

  gr = get_group_info_by_name (__name);

  if (!gr)
    {
      return -1;
    }

  res = gr->gid;

  free_group_info (gr);

  return res;
}

/**
 * Handler of clicking button 'abort' on process window
 *
 * @param __button - sender button
 * @return non-zero if action has been handled, non-zero otherwise
 */
static int
abort_button_clicked (w_button_t *__button)
{
  chown_process_window_t *wnd;
  if (!__button || !WIDGET_USER_DATA (__button))
    {
      return 0;
    }

  wnd = WIDGET_USER_DATA (__button);

  wnd->abort = TRUE;

  return TRUE;
}

/**
 * Handler of keydown message for buttons on process window
 *
 * @param __button - button on which key was pressed
 * @param __ch - code of pressed key
 * @return non-zero if action has been handled, non-zero otherwise
 */
static int
button_keydown (w_button_t *__button, wint_t __ch)
{
  if (!__button || !WIDGET_USER_DATA (__button))
    {
      return 0;
    }

  if (__ch == KEY_ESC)
    {
      /* If escaped was pressed, chown operation shoud be aborted */
      chown_process_window_t *wnd;
      wnd = WIDGET_USER_DATA (__button);
      wnd->abort = TRUE;
    }

  return 0;
}

/**
 * Handler of keydown message for edit box in dialog
 *
 * @param __edit - edit box received this message
 * @param __ch - code of pressed key
 * @return non-zero if action has been handled, non-zero otherwise
 */
static int
edit_keydown (w_edit_t *__edit, wint_t __ch)
{
#ifdef LIST_NAVIGATE_FROM_EDIT
  /* If navigating in list from edit is enabled */
  /* and pressed button is an up or down arrow, */
  /* we should call list's callback and disable */
  /* handling this key in edit box's handler */
  if (__ch == KEY_UP || __ch == KEY_DOWN)
    {
      w_list_t *list;
      list = WIDGET_USER_DATA (__edit);
      WIDGET_CALL_CALLBACK (list, keydown, list, __ch);

      /* We need this call because list may want to be redrawn */
      /* but in this case cursor may be moved to different place */
      widget_redraw (WIDGET (__edit));
      return TRUE;
    }
#endif
  return FALSE;
}

/********
 * User's backend
 */

/**
 * Show dialog with different chown options
 *
 * @param __user - identificator of file's owner
 * @param __group - identificator of file's group
 * @param __rec - used to determine if it s able to make recursive chown
 * and user's decision will be written here
 * @return zero on success, non-zero otherwise
 */
int
action_chown_dialog (int *__user, int *__group, int *__rec)
{
  w_window_t *wnd;
  w_container_t *cnt;
  int left, dummy, res;
  w_button_t *btn;
  w_list_t *list;
  w_edit_t *edt_user, *edt_group, *editboxes[2];
  w_checkbox_t *cb = NULL;

  /* Create widgets */
  wnd = widget_create_window (_(L"Change owner"), 0, 0, 55, 20, WMS_CENTERED);
  WIDGET_USER_CALLBACK (wnd, property_changed) = window_property_changed;
  WIDGET_USER_DATA(wnd) = editboxes;
  cnt = WIDGET_CONTAINER (wnd);

  /* Caption and edit box for user name */
  dummy = (cnt->position.width - 2) / 2 - 1;
  widget_create_text (cnt, _(L"User name:"), 1, 1);
  edt_user = widget_create_edit (cnt, 1, 2, dummy);
  WIDGET_USER_CALLBACK (edt_user, property_changed) = edit_property_changed;
  WIDGET_USER_CALLBACK (edt_user, keydown) = (widget_keydown_proc)edit_keydown;

  /* Caption and edit box for group name */
  widget_create_text (cnt, _(L"Group name:"), dummy + 2, 1);
  edt_group = widget_create_edit (cnt, dummy + 2, 2,
                                 cnt->position.width - dummy - 3);
  WIDGET_USER_CALLBACK (edt_group, property_changed) = edit_property_changed;
  WIDGET_USER_CALLBACK (edt_group, keydown) = (widget_keydown_proc)edit_keydown;

  editboxes[0] = edt_user;
  editboxes[1] = edt_group;

  /* List of users */
  list = widget_create_list (cnt, _(L"List of users"), 1, 3, dummy,
                             cnt->position.height - 5 - ((*__rec) ? 1 : 0));
  WIDGET_USER_DATA (list) = edt_user;
  WIDGET_USER_DATA (edt_user) = list;
  WIDGET_USER_CALLBACK (list, property_changed) = list_property_changed;
  fill_users_list (list, *__user);

  /* List of Groups */
  list = widget_create_list (cnt, _(L"List of groups"), dummy + 2, 3,
                             cnt->position.width - dummy - 3,
                             cnt->position.height - 5 - ((*__rec) ? 1 : 0));
  WIDGET_USER_DATA (list) = edt_group;
  WIDGET_USER_DATA (edt_group) = list;
  WIDGET_USER_CALLBACK (list, property_changed) = list_property_changed;
  fill_groups_list (list, *__group);

  if (*__rec)
    {
      /* Recursively chown'ing is able */
      cb = widget_create_checkbox (cnt, _(L"_Recursively"),
                                   1, wnd->position.height - 3, FALSE, 0);
    }

  /* Create buttons */
  dummy = widget_shortcut_length (_(L"_Ok"));
  left = (wnd->position.width - dummy -
          widget_shortcut_length (_(L"_Cancel")) - 11) / 2;

  btn = widget_create_button (WIDGET_CONTAINER (wnd), _(L"_Ok"), left,
                              wnd->position.height - 2, WBS_DEFAULT);
  btn->modal_result = MR_OK;

  left += dummy + 7;
  btn = widget_create_button (WIDGET_CONTAINER (wnd), _(L"_Cancel"), left,
                              wnd->position.height - 2, 0);
  btn->modal_result = MR_CANCEL;

  /* Show window and overview result */
  res = w_window_show_modal (wnd);

  /* Get new identificators of user and group */
  if (res == MR_OK)
    {
      (*__user)  = user_id (w_edit_get_text (edt_user));
      (*__group) = group_id (w_edit_get_text (edt_group));

      if (*__rec)
        {
          (*__rec) = w_checkbox_get (cb);
        }
    }

  /* Free currently unused memory */
  widget_destroy (WIDGET (wnd));

  return  (res == MR_OK) ? (ACTION_OK) : (ACTION_ABORT);
}

/* Create chown'ing process window */
chown_process_window_t*
action_chown_create_proc_wnd (BOOL __total_progress)
{
  chown_process_window_t *res;
  w_button_t *btn;
  w_container_t *cnt;
  int dummy;

  MALLOC_ZERO (res, sizeof (chown_process_window_t));

  res->window = widget_create_window (_(L"Change owner"), 0, 0,
                                      50, 5 + (__total_progress ? 3 : 0),
                                      WMS_CENTERED);
  cnt = WIDGET_CONTAINER (res->window);

  if (__total_progress)
    {
      /*
       * NOTE: Real maximum progress bar's position will
       *       be set later in chown'ing stuff
       */
      res->progress = widget_create_progress (cnt, 100, 1, 4,
                                              cnt->position.width - 2, 0);
    }

  widget_create_text (cnt, _(L"Changing owner of file or directory:"), 1, 1);
  res->text = widget_create_text (cnt, L"", 1, 2);

  /* Create button */
  dummy = (cnt->position.width -
           widget_shortcut_length (_(L"_Abort")) - 6) / 2;
  btn = widget_create_button (cnt, _(L"_Abort"),
                              dummy, cnt->position.height - 2, WBS_DEFAULT);
  btn->modal_result = MR_ABORT;
  WIDGET_USER_DATA (btn) = res;
  WIDGET_USER_CALLBACK (btn, clicked) = (widget_action)abort_button_clicked;
  WIDGET_USER_CALLBACK (btn, keydown) = (widget_keydown_proc)button_keydown;

  return res;
}

/* Create chown'ing process window */
void
action_chown_destroy_proc_wnd (chown_process_window_t *__window)
{
  if (!__window)
    {
      return;
    }

  widget_destroy (WIDGET (__window->window));
  free (__window);
}
