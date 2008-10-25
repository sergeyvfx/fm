/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of action 'delete'
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "actions.h"
#include "messages.h"
#include "i18n.h"
#include "dir.h"
#include "hook.h"

static BOOL confirm = TRUE;
static BOOL scan = TRUE;

/**
 * Confirm deletion operation
 *
 * @param __list - list of selected items
 * @param __count - count of items in list
 * @return non-zero if user confirmed deletion, zero otherwise
 */
static BOOL
confirm_deletion (const file_panel_item_t **__list, unsigned long __count)
{
  int res;
  wchar_t *message;

  if (!confirm)
    {
      /* Confirmation of deletion is disabled */
      /* Assume silent confirmation */
      return TRUE;
    }

  message = malloc (1024 * sizeof (wchar_t));
  action_message_formatting (__list, __count, L"Delete %ls?",
                             message, 1024);

  res = message_box (_(L"Delete"), message,
                     MB_YESNO | MB_DEFBUTTON_1 | MB_CRITICAL);

  free (message);

  return res == MR_YES;
}

/**
 * Delete operation for action_operate
 *
 * @param __full_name - full name of file to operate on
 * @param __stat - file's stat information
 * @param __flags - flags describing caller's context
 * @return zero on success, non-zero otherwise
 */
static int
delete_operation (const wchar_t *__full_name, vfs_stat_t __stat,
                  void *__user_data ATTR_UNUSED, unsigned int __flags)
{
  int res;

  if (S_ISDIR (__stat.st_mode))
    {
      if (__flags & AOF_DIR_IGNORED_ITEMS)
        {
          /* There are some ignored children */
          /* we can't unlink this directory */
          return ACTION_IGNORE;
        }

      ACTION_REPEAT (res = vfs_rmdir (__full_name),
                     action_error_retryskipcancel_ign,
                     return ACTION_CANCEL_TO_ABORT (__dlg_res_),
                     _(L"Cannot unlink directory \"%ls\":\n%ls"),
                     __full_name, vfs_get_error (res));
    }
  else
    {
      ACTION_REPEAT (res = vfs_unlink (__full_name),
                     action_error_retryskipcancel_ign,
                     return ACTION_CANCEL_TO_ABORT (__dlg_res_),
                     _(L"Cannot unlink file \"%ls\":\n%ls"),
                     __full_name, vfs_get_error (res));
    }

  if (res)
    {
      return ACTION_IGNORE;
    }

  return ACTION_OK;
}

/********
 * User's backend
 */

/**
 * Delete list of files from specified panel
 *
 * @param __panel - determines panel from which files will be deleted
 * @return zero on success, non-zero otherwise
 */
int
action_delete (file_panel_t *__panel)
{
  unsigned long count;
  file_panel_item_t **list = NULL;
  int res = ACTION_ERR;

  /* Get list of items to be deleted */
  count = file_panel_get_selected_items (__panel, &list);

  /* Cannot operate on pseydodir */
  if (!action_check_no_pseydodir ((const file_panel_item_t**)list, count))
    {
      wchar_t msg[1024];
      swprintf (msg, BUF_LEN (msg), _(L"Cannot operate on \"%ls\""),
                list[0]->file->name);
      MESSAGE_ERROR (msg);
      SAFE_FREE (list);
      return ACTION_ERR;
    }

  /* If user confirmed deletion... */
  if (confirm_deletion ((const file_panel_item_t**)list, count))
    {
      wchar_t *cwd;

      /* ..get full CWD and... */
      cwd = file_panel_get_full_cwd (__panel);

      /* ...make deletion */
      res = action_operate (_(L"Delete"),
                            _(L"Deleting"),
                            __panel, cwd, (const file_panel_item_t**)list,
                            count, TRUE, scan,
                            (action_operator_t)delete_operation,
                            NULL, (action_operator_t)delete_operation, NULL);

      /* Free used memory */
      free (cwd);
    }

  SAFE_FREE (list);

  /* There may be selection in source panel and it may be changed */
  /* Also, some items may have been deleted */
  /* so, we need to rescan this panel */
  file_panel_rescan (__panel);

  return res;
}
