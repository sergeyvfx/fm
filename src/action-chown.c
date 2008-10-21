/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of action 'Change owner'
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "actions.h"
#include "action-chown-iface.h"
#include "messages.h"
#include "dir.h"
#include "hook.h"

#include <vfs/vfs.h>
#include <widgets/widget.h>

/*
 * NOTE: Current implementation of changing owner always
 *       follows symbolic links
 */

/*
 * TODO: Add option to make this stuff not follow symbolic links
 */

#define UPDATE_ID(_arg, _field) \
  { \
    if ((*_arg) == -1) \
      { \
        (*_arg) = __list[i]->file->stat._field; \
      } \
    else \
      { \
        if ((*_arg) != __list[i]->file->stat._field) \
          { \
            (*_arg) = -2; \
          } \
      } \
  }

/*
 * Free all tail dirents. Helper for chown_dir_rec()
 */
#define FREE_REMAIN_DIRENT() \
  { \
    if (!prescanned) \
      { \
        int j; \
        for (j = i; j < count; j++) \
          { \
            vfs_free_dirent (eps[i]); \
          } \
      } \
  }

/* Datatype for operator callback */
typedef struct
{
  /* IDs of new user and group */
  int uid, gid;

  /* Are multiply files/directories selected */
  BOOL multiselect;
} op_data_t;

/* Scan directories before chown'ing */
/* (to display total progress) */
static BOOL scan = TRUE;

/**
 * Get initial values if user and group ids
 *
 * @param __path - path to items
 * @param __list - list of items
 * @param __count - count of items in list
 * @param __uid - pointer to variable, where user id will be stored
 * @param __gid - pointer to variable, where group id will be stored
 * @return zero on success, non-zero otherwise
 */
static int
get_initial_ids (const wchar_t *__path, const file_panel_item_t **__list,
                 unsigned long __count, int *__uid, int *__gid)
{
  unsigned long i;
  (*__uid) = -1;
  (*__gid) = -1;

  for (i = 0; i < __count; ++i)
    {
      UPDATE_ID (__uid, st_uid);
      UPDATE_ID (__gid, st_gid);
    }

  return ACTION_OK;
}

/**
 * Chown operation for action_operate
 *
 * @param __full_name - full name of file to operate on
 * @param __stat - file's stat information
 * @param __data - data collected from user to use
 * @return zero on success, non-zero otherwise
 */
static int
chown_operation (const wchar_t *__path, vfs_stat_t __stat,
                 op_data_t *__data, unsigned int __flags ATTR_UNUSED)
{
  wchar_t *mask;
  int res;
  int (*msg_proc) (const wchar_t*, ...);

  mask = _(L"Cannot change owner of directory \"%ls\":\n%ls");
  if (S_ISDIR (__stat.st_mode))
    {
      mask = _(L"Cannot change owner of directory \"%ls\":\n%ls");
    }
  else
    {
      mask = _(L"Cannot change owner of file \"%ls\":\n%ls");
    }

  if (__data->multiselect)
    {
      msg_proc = action_error_retryskipcancel_ign;
    }
  else
    {
      msg_proc = action_error_retrycancel;
    }

  ACTION_REPEAT (res = vfs_chown (__path, __data->uid, __data->gid),
                 msg_proc,
                 return ACTION_CANCEL_TO_ABORT (__dlg_res_),
                 mask, __path, vfs_get_error (res));

  if (res)
    {
      return ACTION_SKIP;
    }

  return ACTION_OK;
}


/**
 * Determine is it able to make recursively chown'ing
 *
 * @param __list - list of selected items
 * @param __count - count of selected items
 * @return non-zero if recursively chown'ing is able, zero otherwise
 */
BOOL
is_recursively_able (const file_panel_item_t **__list, unsigned long __count)
{
  /* Recursively chown'ing is able if */
  /* at least one selected item is a directory */

  unsigned int i;

  /* Review all items */
  for (i = 0; i < __count; ++i)
    {
      if (S_ISDIR (__list[i]->file->stat.st_mode))
        {
          /* Directory has been found */
          return TRUE;
        }
    }

  return FALSE;
}

/********
 * User's backend
 */

/**
 * Change owner of single file or group of files
 *
 * @param __panel - panel from which list of files will be get
 * @return zero on success, non-zero otherwise
 */
int
action_chown (file_panel_t *__panel)
{
  unsigned long count;
  file_panel_item_t **list = NULL;
  unsigned long res;
  wchar_t *cwd;
  BOOL recursively = FALSE;
  op_data_t op_data = {0, 0, FALSE};

  /* Get list of items to be chowned */
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

  cwd = file_panel_get_full_cwd (__panel);

  res = get_initial_ids (cwd, (const file_panel_item_t**)list,
                         count, &op_data.uid, &op_data.gid);

  /*
   * TODO: What should we do if there is wrong symbolic ling in selection?
   */

  if (res == ACTION_OK)
    {
      recursively = is_recursively_able ((const file_panel_item_t**)list,
                                         count);

      /* Show dialog to get new values of uid and git */
      if (action_chown_dialog (&op_data.uid, &op_data.gid,
                               &recursively) == ACTION_OK)
        {
          op_data.multiselect = count > 1;
          res = action_operate (_(L"Change owner"),
                                _(L"Changing owner of file or directory:"),
                                __panel, cwd, (const file_panel_item_t**)list,
                                count, recursively, scan,
                                (action_operator_t)chown_operation,
                                NULL, (action_operator_t)chown_operation,
                                &op_data);
        }
    }

  /* Free used memory */
  free (cwd);
  SAFE_FREE (list);

  /* We need rescan file panel because it can display */
  /* user/group of file */
  file_panel_rescan (__panel);

  /*
   * TODO: Maybe we should return ACTION_ERR if res is zero
   */

  return res;
}
