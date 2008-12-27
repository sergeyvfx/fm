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

#include "actions.h"
#include "action-chmod-iface.h"
#include "i18n.h"
#include "messages.h"

/* Bits of mode which displays file's permissions */
#define  PERM_BITS 07777

/* Scan directories before chown'ing */
/* (to display total progress) */
static BOOL scan = TRUE;

/* Datatype for operator callback */
typedef struct
{
  unsigned int mask;
  unsigned int unknown_mask;
} op_data_t;

static void
get_initial_masks (file_panel_item_t **__list, unsigned long __count,
                   unsigned int *__mode, unsigned int *__unknown_mask)
{
  unsigned long i;
  unsigned int cmode;

  /* Assume initial mask of set bits is equal to first selected file's mode */
  /* and all bits are known */
  (*__mode) = __list[0]->file->stat.st_mode & PERM_BITS;
  (*__unknown_mask) = 0;

  /* Overview the rest files */
  for (i = 1; i < __count; ++i)
    {
      cmode = __list[i]->file->stat.st_mode & PERM_BITS;

      (*__unknown_mask) |= (*__mode) ^ cmode;
      (*__mode) &= cmode;
    }
}

/**
 * Chmod operation for action_operate
 *
 * @param __full_name - full name of file to operate on
 * @param __stat - file's stat information
 * @param __masks - descriptor of bitmasks
 * @return zero on success, non-zero otherwise
 */
static int
chmod_operation (const wchar_t *__full_name, vfs_stat_t __stat,
                 op_data_t *__masks, unsigned int __flags ATTR_UNUSED)
{
  int res;
  vfs_mode_t mode;
  vfs_stat_t stat;
  wchar_t *format;

  mode = __masks->mask;

  if (__masks->unknown_mask)
    {
      mode = (__stat.st_mode & __masks->unknown_mask) |
             (mode & ~__masks->unknown_mask);
    }

  if (mode != (__stat.st_mode & PERM_BITS))
    {
      /* Call vfs_chmod() only if new mode is differ than */
      /* current mode of file or directory */

      /* Get format string for error message */
      if (S_ISDIR (stat.st_mode))
        {
          format = L"Cannot change mode of directory \"%ls\":\n%ls";
        }
      else
        {
          format = L"Cannot change mode of file \"%ls\":\n%ls";
        }

      ACTION_REPEAT (res = vfs_chmod (__full_name, mode),
                     action_error_retryskipcancel_ign,
                     return ACTION_CANCEL_TO_ABORT (__dlg_res_),
                     _(format),
                     __full_name, vfs_get_error (res));

      if (res)
        {
          res = ACTION_SKIP;
        }
    }
  else
    {
      res = ACTION_OK;
    }

  return res;
}

/********
 * User's backend
 */

/**
 * Change mode of single file or group of files
 *
 * @param __panel - file pinel in which items will be changed
 * @return zero on success, non-zero otherwise
 */
int
action_chmod (file_panel_t *__panel)
{
  op_data_t op_data = {0, 0};
  int res;
  unsigned long count;
  file_panel_item_t **list = NULL;
  wchar_t *cwd;
  BOOL recursively;

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

  /* Get initial values of mode mask and mask of unknown bits*/
  get_initial_masks (list, count, &op_data.mask, &op_data.unknown_mask);

  /* Recursively chown'ing is able if */
  /* at least one selected item is a directory */
  recursively =
    action_is_directory_selected ((const file_panel_item_t**)list,
                                  count);

  /* Get new masks from user */
  res = action_chmod_show_dialog (&op_data.mask, &op_data.unknown_mask,
                                  &recursively);

  if (res == ACTION_OK)
    {
      res = action_operate (_(L"Permissions"),
                            _(L"Changing permissions of file or directory"),
                            __panel, cwd, (const file_panel_item_t**)list,
                            count, recursively, scan, TRUE,
                            (action_operator_t)chmod_operation,
                            NULL, (action_operator_t)chmod_operation,
                            &op_data);
    }

  /* Free used memory */
  SAFE_FREE (list);
  free (cwd);

  /* We need ro rescan panel because it contains cached */
  /* states of files and we need to update them. */
  file_panel_rescan (__panel);

  return res;
}
