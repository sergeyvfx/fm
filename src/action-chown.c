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

#include "errno.h"

#include <vfs/vfs.h>

#define CANCEL_TO_ABORT(_a) \
  ((_a) == MR_CANCEL ? ACTION_ABORT : (_a))

#define UPDATE_ID(_arg, _field) \
  { \
    if ((*_arg) == -1) \
      { \
        (*_arg) = __list[i]->file->lstat._field; \
      } \
    else \
      { \
        if ((*_arg) != __list[i]->file->lstat._field) \
          { \
            (*_arg) = -2; \
          } \
      } \
  }

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
 * Call chown() function for specified item
 *
 * @param __path - path to items
 * @param __item - item for which chown() will be called
 * @param __uid - user id
 * @param __gid - group id
 * @return zero on success, non-zero otherwise
 */
static int
do_chown_item (const wchar_t *__path, file_panel_item_t *__item,
               int __uid, int __gid)
{
  wchar_t *full, *mask;
  int res;

  /* Get full filename */
  full = wcdircatsubdir (__path, __item->file->name);

  if (S_ISDIR (__item->file->lstat.st_mode))
    {
      mask = _(L"Cannot change owner of directory \"%ls\":\n%ls");
    }
  else
    {
      mask = _(L"Cannot change owner of file \"%ls\":\n%ls");
    }

  ACTION_REPEAT (res = vfs_chown (full, __uid, __gid),
                 action_error_retrycancel,
                 free (full); return CANCEL_TO_ABORT (__dlg_res_),
                 mask, __item->file->name, vfs_get_error (res));

  /* Free unused memory */
  free (full);

  /* Item is now unselected */
  __item->selected = FALSE;

  return ACTION_OK;
}

/**
 * Call chown() function for all items from list
 *
 * @param __path - path to items
 * @param __list - list of items
 * @param __count - count of items in list
 * @param __uid - user id
 * @param __gid - group id
 * @return count of items which where successfully chown'ed
 */
static unsigned long
do_chown (const wchar_t *__path, const file_panel_item_t **__list,
          unsigned long __count, int __uid, int __gid)
{
  unsigned long i, res;
  int r;

  for (i = 0; i < __count; ++i)
    {
      r = do_chown_item (__path, (file_panel_item_t*)__list[i], __uid, __gid);

      /* User aborted chown'ing after  */
      if (r == ACTION_ABORT)
        {
          break;
        }

      ++res;
    }

  return res;
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
  int uid, gid;
  unsigned long res;
  wchar_t *cwd;

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
                         count, &uid, &gid);

  if (res == ACTION_OK)
    {
      /* Show dialog to get new values of uid and git */
      if (action_chown_dialog (&uid, &gid) == ACTION_OK)
        {
          res = do_chown (cwd, (const file_panel_item_t**)list,
                          count, uid, gid);

          /*
           * TODO: We'd better rewrite this stupid hack
           */
          if (__panel->items.selected_count)
            {
              __panel->items.selected_count -= res;
            }
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

  return ACTION_OK;
}
