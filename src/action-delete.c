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
#include "action-delete-iface.h"
#include "messages.h"
#include "i18n.h"
#include "dir.h"
#include "hook.h"

static BOOL confirm = TRUE;
static BOOL scan = TRUE;

/*
 * Free all tail dirents. Helper for unlink_dir()
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
 * Set currently deleting item to caption on window
 *
 * @param __path - path to currently deleting item
 * @param __proc_wnd - window with different current information
 */
static void
set_deleting_item (wchar_t *__path, delete_process_window_t *__proc_wnd)
{
  wchar_t *text;
  long width;

  text = malloc ((__proc_wnd->window->position.width + 1) * sizeof (wchar_t));
  width = __proc_wnd->window->position.width -
          __proc_wnd->text->position.x - 1;
  fit_dirname (__path, width, text);
  w_text_set (__proc_wnd->text, text);
  free (text);
}

/**
 * Handler of `file unlinked` event
 *
 * @param __proc_wnd - window with different current information
 */
static void
file_unlinked (delete_process_window_t *__proc_wnd)
{
  w_progress_set_pos (__proc_wnd->progress,
                      w_progress_get_pos (__proc_wnd->progress) + 1);
}

/**
 * Unlink single file
 *
 * @param __path - path to file to be deleted
 * @param __proc_wnd - window with different current information
 * @return zero on success, non-zero otherwise
 */
static int
unlink_file (wchar_t *__path, delete_process_window_t *__proc_wnd)
{
  int res;

  set_deleting_item (__path, __proc_wnd);

  ACTION_REPEAT (res = vfs_unlink (__path),
                 action_error_retryskipcancel_ign,
                 return ACTION_CANCEL_TO_ABORT (__dlg_res_),
                 _(L"Cannot unlink file \"%ls\":\n%ls"),
                 __path, vfs_get_error (res));

  file_unlinked (__proc_wnd);

  if (res)
    {
      /* User ignored deletion */
      return ACTION_SKIP;
    }

  return ACTION_OK;
}

/**
 * Unlink directory
 *
 * @param __path - path to directory to be deleted
 * @param __proc_wnd - window with different current information
 * @param __tree - prescanned tree of items
 * @return zero on success, non-zero otherwise
 */
static int
unlink_dir (wchar_t *__path, delete_process_window_t *__proc_wnd,
            const action_listing_tree_t *__tree)
{
  int i, res, count, global_res, ignored_items = 0;
  vfs_dirent_t **eps = NULL;
  BOOL prescanned= FALSE;
  wchar_t *full_name;
  size_t len;

  /* Get listing of a directory */
  if (__tree)
    {
      /* Get listing from prescanned data */
      count = __tree->count;
      eps = __tree->dirent;

      prescanned = TRUE;

      /*
       * NOTE: If we use prescanned data, we shouldn't free()
       *       directory entries from it. They will be freed while
       *       the whole prescanned tree will be destroying
       */
    }
  else
    {
      /* Scan directory */
      ACTION_REPEAT (count = vfs_scandir (__path, &eps, 0, vfs_alphasort);
                    res = count < 0 ? count : 0,
                    action_error_retryskipcancel_ign,
                    return ACTION_CANCEL_TO_ABORT (__dlg_res_),
                    _(L"Cannot listing directory \"%ls\":\n%ls"),
                    __path, vfs_get_error (res));
    }

  len = wcslen (__path) + MAX_FILENAME_LEN + 1;
  full_name = malloc ((len + 1) * sizeof (wchar_t));

  /* Unlink children */
  global_res = ACTION_OK;
  for (i = 0; i < count; ++i)
    {
      if (!IS_PSEUDODIR (eps[i]->name))
        {
          /* Get full filename of current file or directory */
          swprintf (full_name, len, L"%ls/%ls", __path, eps[i]->name);

          if (isdir (full_name))
            {
              res = unlink_dir (full_name, __proc_wnd,
                                prescanned ? __tree->items[i] : NULL);
            }
          else
            {
              res = unlink_file (full_name, __proc_wnd);
            }
        }

      if (res == ACTION_ABORT)
        {
          FREE_REMAIN_DIRENT ();
          global_res = ACTION_ABORT;
          break;
        }

      if (res == ACTION_IGNORE || res == ACTION_SKIP)
        {
          ++ignored_items;
          res = 0;
        }

      if (!prescanned)
        {
          vfs_free_dirent (eps[i]);
        }

      /* Process accamulated queue of characters */
      hook_call (L"switch-task-hook", NULL);
      if (__proc_wnd->abort)
        {
          /* Free allocated memory */
          FREE_REMAIN_DIRENT ();
          global_res = ACTION_ABORT;
          break;
        }
    }

  if (!prescanned)
    {
      SAFE_FREE (eps);
    }

  if (global_res == ACTION_OK && ignored_items == 0)
    {
      if (prescanned && __tree->ignored_flag)
        {
          /* There are some ignored children */
          /* we can't unlink this directory */
          return ACTION_IGNORE;
        }

      set_deleting_item (__path, __proc_wnd);

      ACTION_REPEAT (res = vfs_rmdir (__path),
                     action_error_retryskipcancel_ign,
                     return ACTION_CANCEL_TO_ABORT (__dlg_res_),
                     _(L"Cannot unlink directory \"%ls\":\n%ls"),
                     __path, vfs_get_error (res));
    }

  return global_res;
}

/**
 * Iterator for make_deletion
 *
 * @param __path - path to item to be deleted
 * @param __proc_wnd - window with different current information
 * @param __tree - prescanned tree of items. This tree will be
 * send to unlink_dir().
 * @return zero on success, non-zero otherwise
 */
static int
make_deletion_iter (wchar_t *__path, delete_process_window_t *__proc_wnd,
                    const action_listing_tree_t *__tree)
{
  int res;

  /*
   * TODO: Nay be we should use ACTION_REPEAT(...) instead of stupid isdir?
   */
  if (isdir (__path))
    {
      res = unlink_dir (__path, __proc_wnd, __tree);
    }
  else
    {
      res = unlink_file (__path, __proc_wnd);
    }

  return res;
}

/**
 * Delete list of files
 *
 * @param __base_dir - base directory
 * @param __list - list of items to be deleted
 * @param __count - count of items to be deleetd
 * @return count of deleted items
 */
static unsigned long
make_deletion (const wchar_t *__base_dir,
               const file_panel_item_t **__list, unsigned long __count)
{
  int res;
  unsigned long i, source_count = __count, count = 0;
  action_listing_t listing;
  BOOL scanned = FALSE;
  wchar_t *name, *full_name;
  file_panel_item_t *item;
  delete_process_window_t *wnd;

  if (scan)
    {
      ACTION_REPEAT (res = action_get_listing (__base_dir, __list,
                                               __count, &listing,
                                               FALSE, FALSE);
                     if (res == ACTION_ABORT)
                       {
                         return 0;
                       },
                     action_error_retryskipcancel_ign,
                     return 0,
                     _(L"Cannot get listing of items:\n%ls"),
                     vfs_get_error (res));

      if (!res)
        {
          /* User can ignore some subtrees, so we need */
          /* get count of source elements from prescanned data */
          source_count = listing.tree->count;
          scanned = TRUE;
        }
    }

  /* Create and show progress window */
  wnd = action_delete_create_proc_wnd (scanned, &listing);
  w_window_show (wnd->window);

  for (i = 0; i < source_count; ++i)
    {
      if (scanned)
        {
          name = listing.tree->dirent[i]->name;
        }
      else
        {
          item = (file_panel_item_t*)__list[i];
          name = item->file->name;
        }

      /* Get full name of item to be deleted */
      full_name = wcdircatsubdir (__base_dir, name);
      res = make_deletion_iter (full_name, wnd,
                                scanned ? listing.tree->items[i] : NULL);
      free (full_name);

      if (res == ACTION_OK)
        {
          ++count;
        }
      else
        {
          if (res == ACTION_ABORT)
            {
              break;
            }
        }

      /* Process accumulated tree */
      hook_call (L"switch-task-hook", NULL);
    }

  if (scanned)
    {
      action_free_listing (&listing);
    }

  action_delete_destroy_proc_wnd (wnd);

  return count;
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
      unsigned long res;

      /* ..get full CWD and... */
      cwd = file_panel_get_full_cwd (__panel);

      /* ...make deletion */
      res = make_deletion (cwd, (const file_panel_item_t**)list, count);

      if (__panel->items.selected_count)
        {
          __panel->items.selected_count -= res;
        }

      /* Free used memory */
      free (cwd);
    }

  SAFE_FREE (list);

  /* There may be selection in source panel and it may be changed */
  /* Also, some items may have been deleted */
  /* so, we need to rescan this panel */
  file_panel_rescan (__panel);

  return ACTION_OK;
}
