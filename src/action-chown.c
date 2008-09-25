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

#include <vfs/vfs.h>
#include <widgets/widget.h>

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
 * Set name of currently chown'ing file of directory
 *
 * @param __proc_wnd - process window descriptor
 * @param __file - name of currently chown'ing file or directory
 */
static void
set_current_file (chown_process_window_t *__proc_wnd, const wchar_t *__file)
{
  int width;
  wchar_t buf[1024];
  width = __proc_wnd->window->position.width - 2;

  fit_dirname (__file, MIN (width, BUF_LEN (buf)), buf);

  w_text_set (__proc_wnd->text, buf);
}

/**
 * Call chown() function for specified entry (file or directory)
 *
 * @param __path - path to file
 * @param __uid - user id
 * @param __gid - group id
 * @param __dir - is entry a directory?
 * @param __multiselect - are a lot of items selected?
 * This parameter used to determine which message box should be used.
 * @param __proc_wnd - process window descriptor
 * @return zero on success, non-zero otherwise
 */
static int
chown_entry (const wchar_t *__path, int __uid, int __gid,
             BOOL __dir, BOOL __multiselect,
             chown_process_window_t *__proc_wnd)
{
  wchar_t *mask;
  int res;
  int (*msg_proc) (const wchar_t*, ...);

  if (__proc_wnd->progress && !__proc_wnd->manual_total_count)
    {
      w_progress_set_pos (__proc_wnd->progress,
                          w_progress_get_pos (__proc_wnd->progress) + 1);
    }

  set_current_file (__proc_wnd, __path);

  if (__dir)
    {
      mask = _(L"Cannot change owner of directory \"%ls\":\n%ls");
    }
  else
    {
      mask = _(L"Cannot change owner of file \"%ls\":\n%ls");
    }

  if (__multiselect)
    {
      msg_proc = action_error_retryskipcancel_ign;
    }
  else
    {
      msg_proc = action_error_retrycancel;
    }

  ACTION_REPEAT (res = vfs_chown (__path, __uid, __gid),
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
 * Call chown() function for specified file
 *
 * @param __path - path to file
 * @param __uid - user id
 * @param __gid - group id
 * @param __multiselect - are a lot of items selected?
 * This parameter used to determine which message box should be used.
 * @param __proc_wnd - process window descriptor
 * @return zero on success, non-zero otherwise
 */
static int
chown_file (const wchar_t *__path, int __uid, int __gid, BOOL __multiselect,
            chown_process_window_t *__proc_wnd)
{
  return chown_entry (__path, __uid, __gid, FALSE, __multiselect, __proc_wnd);
}

/**
 * Call chown() function for specified directory
 *
 * @param __path - path to file
 * @param __uid - user id
 * @param __gid - group id
 * @param __multiselect - are a lot of items selected?
 * This parameter used to determine which message box should be used.
 * @param __proc_wnd - process window descriptor
 * @return zero on success, non-zero otherwise
 */
static int
chown_dir (const wchar_t *__path, int __uid, int __gid, BOOL __multiselect,
           chown_process_window_t *__proc_wnd)
{
  return chown_entry (__path, __uid, __gid, TRUE, __multiselect, __proc_wnd);
}

/**
 * Call chown() function for specified item
 *
 * @param __path - base path to item
 * @param __item - item for which chown() will be called
 * @param __uid - user id
 * @param __gid - group id
 * @param __multiselect - are a lot of items selected?
 * This parameter used to determine which message box should be used.
 * @para __proc_wnd - process window descriptor
 * @return zero on success, non-zero otherwise
 */
static int
do_chown_item (const wchar_t *__path, file_panel_item_t *__item,
               int __uid, int __gid, BOOL __multiselect,
               chown_process_window_t *__proc_wnd)
{
  int res;
  wchar_t *full;

  /* Get full file name */
  full = wcdircatsubdir (__path, __item->file->name);

  res = chown_entry (full, __uid, __gid,
                     S_ISDIR (__item->file->lstat.st_mode), __multiselect,
                     __proc_wnd);

  /* Free unused memory */
  free (full);

  if (res == ACTION_OK)
    {
      /* Item is now unselected */
      __item->selected = FALSE;
    }

  return res;
}

/**
 * Chown top-level items only (do not use dfs)
 *
 * @param __path - path to items
 * @param __list - list of items
 * @param __count - count of items in list
 * @param __uid - user id
 * @param __gid - group id
 * @param __proc_wnd - process window descriptor
 * @return count of items which where successfully chown'ed
 */
static unsigned long
do_toplevel_chown (const wchar_t *__path, const file_panel_item_t **__list,
                   unsigned long __count, int __uid, int __gid,
                   chown_process_window_t *__proc_wnd)
{
  unsigned long i, res = 0;
  int r;

  w_progress_set_max (__proc_wnd->progress, __count);

  for (i = 0; i < __count; ++i)
    {
      r = do_chown_item (__path, (file_panel_item_t*)__list[i],
                         __uid, __gid, __count > 1, __proc_wnd);

      if (r == ACTION_OK)
        {
          ++res;
        }

      /* User aborted chown'ing after  */
      if (r == ACTION_ABORT)
        {
          break;
        }

      /* Process accumulated queue */
      widget_process_queue ();

      if (__proc_wnd->abort)
        {
          break;
        }
    }

  return res;
}

/**
 * Recursively directory chown'ing
 *
 * @param __path - full path to item
 * @param __uid - new user id
 * @param __gid - new group id
 * @param __tree - prescanned tree
 * @param __proc_wnd - process window descriptor
 * @return zero on success, non-zero otherwise
 */
static int
chown_dir_rec (wchar_t *__path, int __uid, int __gid,
               const action_listing_tree_t *__tree,
               chown_process_window_t *__proc_wnd)
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

  /* Chown children */
  global_res = ACTION_OK;
  for (i = 0; i < count; ++i)
    {
      if (!IS_PSEUDODIR (eps[i]->name))
        {
          /* Get full filename of current file or directory */
          swprintf (full_name, len, L"%ls/%ls", __path, eps[i]->name);

          if (isdir (full_name))
            {
              res = chown_dir_rec (full_name, __uid, __gid,
                                   prescanned ? __tree->items[i] : NULL,
                                   __proc_wnd);
            }
          else
            {
              res = chown_file (full_name, __uid, __gid, TRUE, __proc_wnd);
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
      widget_process_queue ();
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

  if (global_res == ACTION_OK)
    {
      global_res = chown_dir (__path, __uid, __gid, TRUE, __proc_wnd);
    }

  return global_res;
}

/**
 * Iterator for recursively chown'ing
 *
 * @param __path - full path to item
 * @param __uid - new user id
 * @param __gid - new group id
 * @param __tree - prescanned tree
 * @param __proc_wnd - process window descriptor
 * @return zero on success, non-zero otherwise
 */
static int
do_rec_chown_iter (wchar_t *__path, int __uid, int __gid,
                   const action_listing_tree_t *__tree,
                   chown_process_window_t *__proc_wnd)
{
  int res = ACTION_OK;

  if (!isdir (__path))
    {
      /* Chown single file (regular or symlink) */

      /* Set `multiselect` flag to truth because */
      /* at least one more directory selected */
      res = chown_file (__path, __uid, __gid, TRUE, __proc_wnd);
    }
  else
    {
      res = chown_dir_rec (__path, __uid, __gid, __tree, __proc_wnd);
    }

  return res;
}

/**
 * Recursively chown'ing of items
 *
 * @param __path - path to items
 * @param __list - list of items
 * @param __count - count of items in list
 * @param __uid - user id
 * @param __gid - group id
 * @param __proc_wnd - process window descriptor
 * @return count of items which where successfully chown'ed
 */
static unsigned long
do_recursively_chown (const wchar_t *__path, const file_panel_item_t **__list,
                      unsigned long __count, int __uid, int __gid,
                      chown_process_window_t *__proc_wnd)
{
  int res;
  unsigned long i, j, source_count = __count, count = 0;
  BOOL scanned = FALSE;
  action_listing_t listing;
  wchar_t *name, *full_name;
  file_panel_item_t *item;

  if (scan)
    {
      ACTION_REPEAT (res = action_get_listing (__path, __list,
                                               __count, &listing, FALSE, TRUE);
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

          w_progress_set_max (__proc_wnd->progress, listing.count);
        }
      else
        {
          __proc_wnd->manual_total_count = TRUE;
        }
    }

  if (!scanned)
    {
      w_progress_set_max (__proc_wnd->progress, source_count);
      __proc_wnd->manual_total_count = TRUE;
    }

  item = NULL;
  j = 0;
  for (i = 0; i < source_count; ++i)
    {
      if (scanned)
        {
          name = listing.tree->dirent[i]->name;

          /* Search file panel item with specified name */
          item = (file_panel_item_t*)__list[j++];
          while ((wcscmp (item->file->name, name) != 0) && j < __count)
            {
              item = (file_panel_item_t*)__list[j++];
            }
        }
      else
        {
          item = (file_panel_item_t*)__list[j++];
          name = item->file->name;
        }

      /* Get full name of item to be deleted */
      full_name = wcdircatsubdir (__path, name);
      res = do_rec_chown_iter (full_name, __uid, __gid,
                               scanned ? listing.tree->items[i] : NULL,
                               __proc_wnd);
      free (full_name);

      if (__proc_wnd->manual_total_count && __proc_wnd->progress)
        {
          w_progress_set_pos (__proc_wnd->progress,
                              w_progress_get_pos (__proc_wnd->progress) + 1);
        }

      if (res == ACTION_OK)
        {
          if (item)
            {
              item->selected = FALSE;
              ++count;
            }
        }
      else
        {
          if (res == ACTION_ABORT)
            {
              break;
            }
        }

      /* Process accumulated queue */
      widget_process_queue ();

      if (__proc_wnd->abort)
        {
          break;
        }
    }

  if (scanned)
    {
      action_free_listing (&listing);
    }

  return count;
}

/**
 * Call chown() function for all items from list
 *
 * @param __path - path to items
 * @param __list - list of items
 * @param __count - count of items in list
 * @param __uid - user id
 * @param __gid - group id
 * @param __rec - do recursively chown'ing
 * @return count of items which where successfully chown'ed
 */
static unsigned long
do_chown (const wchar_t *__path, const file_panel_item_t **__list,
          unsigned long __count, int __uid, int __gid, BOOL __rec)
{
  unsigned long res;
  chown_process_window_t *proc_wnd;

  /* Create and show process window */
  proc_wnd = action_chown_create_proc_wnd (!__rec || scan);
  w_window_show (proc_wnd->window);

  if (!__rec)
    {
      res = do_toplevel_chown (__path, __list, __count,
                               __uid, __gid, proc_wnd);
    }
  else
    {
      res = do_recursively_chown (__path, __list, __count,
                                  __uid, __gid, proc_wnd);
    }

  action_chown_destroy_proc_wnd (proc_wnd);

  return res;
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
  int uid, gid;
  unsigned long res;
  wchar_t *cwd;
  BOOL recursively = FALSE;

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
      recursively = is_recursively_able ((const file_panel_item_t**)list,
                                         count);

      /* Show dialog to get new values of uid and git */
      if (action_chown_dialog (&uid, &gid, &recursively) == ACTION_OK)
        {
          res = do_chown (cwd, (const file_panel_item_t**)list,
                          count, uid, gid, recursively);

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
