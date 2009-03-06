/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Operate on items
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "actions.h"
#include "i18n.h"
#include "dir.h"
#include "hook.h"
#include "messages.h"
#include "screen.h"

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

/********
 * Internal datatypes
 */
typedef struct
{
  w_window_t   *window;
  w_text_t     *text;
  w_progress_t *progress;

  BOOL abort;
  BOOL manual_total_count;
} process_window_t;

/********
 * Intarface
 */

/**
 * Set currently processing file or directory name
 *
 * @param __proc_wnd - descriptor of process window
 * @param __full_name - full name of processing file or directory
 */
static void
set_processing_file (process_window_t *__proc_wnd, const wchar_t *__full_name)
{
  int width;
  wchar_t buf[1024];

  width = __proc_wnd->window->position.width - 2;

  /* Set text of currently processing file/directory */
  fit_dirname (__full_name, MIN (width, BUF_LEN (buf)), buf);
  w_text_set (__proc_wnd->text, buf);
}

/**
 * Increment position of progress bar
 *
 * @param __proc_wnd - descriptor of process window
 */
static void
inc_progress_pos (process_window_t *__proc_wnd)
{
  if (__proc_wnd->progress)
    {
      w_progress_set_pos (__proc_wnd->progress,
                          w_progress_get_pos (__proc_wnd->progress) + 1);
    }
}

/**
 * Smart making of operation
 *
 * @param __operator - operation entry point
 * @param __full_name - full name of file or directory to operate with
 * @param __stat - stat information of item to operate with
 * @param __user_data - user's data to send to operator
 * @param __proc_wnd - descriptor of process window
 * @param __flags - different flags which will be sent to operator callback
 * @return operator's exit code
 */
static int
make_operation (action_operator_t __operator, const wchar_t *__full_name,
                vfs_stat_t __stat, void *__user_data,
                process_window_t *__proc_wnd, unsigned int __flags)
{
  int res = ACTION_OK;

  if (__operator)
    {
      set_processing_file (__proc_wnd, __full_name);
      res = __operator (__full_name, __stat, __user_data, __flags);
    }

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
  process_window_t *wnd;
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
      process_window_t *wnd;
      wnd = WIDGET_USER_DATA (__button);
      wnd->abort = TRUE;
    }

  return 0;
}

/**
 * Create operatinging process window
 *
 * @param __caption - caption of window
 * @param __desc - description of operation
 * @param __total_progress - create total progress bar?
 * @return descriptor of created window
 */
process_window_t*
create_proc_wnd (const wchar_t *__caption, const wchar_t *__desc,
                 BOOL __total_progress)
{
  process_window_t *res;
  w_button_t *btn;
  w_container_t *cnt;
  int dummy;

  MALLOC_ZERO (res, sizeof (process_window_t));

  res->window = widget_create_window (NULL, __caption, 0, 0,
                                      50, 5 + (__total_progress ? 3 : 0),
                                      WMS_CENTERED);
  cnt = WIDGET_CONTAINER (res->window);

  if (__total_progress)
    {
      /*
       * NOTE: Real maximum progress bar's position will
       *       be set later in chown'ing stuff
       */
      res->progress = widget_create_progress (NULL, cnt, 100, 1, 4,
                                              cnt->position.width - 2, 0);
    }

  widget_create_text (NULL, cnt, __desc, 1, 1);
  res->text = widget_create_text (NULL, cnt, L"", 1, 2);

  /* Create button */
  dummy = (cnt->position.width -
           widget_shortcut_length (_(L"_Abort")) - 6) / 2;
  btn = widget_create_button (NULL, cnt, _(L"_Abort"),
                              dummy, cnt->position.height - 2, WBS_DEFAULT);
  btn->modal_result = MR_ABORT;
  WIDGET_USER_DATA (btn) = res;
  WIDGET_USER_CALLBACK (btn, clicked) = (widget_action)abort_button_clicked;
  WIDGET_USER_CALLBACK (btn, keydown) = (widget_keydown_proc)button_keydown;

  return res;
}

/**
 * Destroy process window
 *
 * @param __window - window to be destroyed
 */
void
destroy_proc_wnd (process_window_t *__window)
{
  if (!__window)
    {
      return;
    }

  widget_destroy (WIDGET (__window->window));
  free (__window);
}

/********
 * Logick
 */

/**
 * Make operation on directory
 *
 * @param __full_name - full name of a directory
 * @param __tree - prescanned tree
 * @param __operation - action which will be called for non-directories
 * in recursively operating and for all objects in non-recursively operating
 * @param __before_rec_op - action which will be called before
 * recursively sinking
 * @param __before_rec_op - action which will be called after
 * recursively sinking
 * @param __stat - stat information about this directory. Need to optimize
 * calls of vfs_stat.
 * @param __proc_wnd - descriptor of process window
 * @param __user_data - user defined data which will be send to operator
 * @return zero on success, non-zero otherwise
 */
static int
process_directory (const wchar_t *__full_name,
                   const action_listing_tree_t *__tree,
                   action_operator_t __operation,
                   action_operator_t __before_rec_op,
                   action_operator_t __after_rec_op,
                   vfs_stat_t __stat, process_window_t *__proc_wnd,
                   void *__user_data)
{
  int i, res, count, global_res, ignored_items = 0;
  vfs_dirent_t **eps = NULL;
  BOOL prescanned = FALSE;
  wchar_t *full_name;
  size_t len;

  /*
   * TODO: Or we'd better call this handler after getting listing?
   */
  res = make_operation (__before_rec_op, __full_name, __stat,
                        __user_data, __proc_wnd, 0);
  if (res != ACTION_OK)
    {
      /* Error occurred in before-recursively handler */
      /* We'd better interrupt current level of recursion */
      return res;
    }

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
      ACTION_REPEAT (count = vfs_scandir (__full_name, &eps, 0, vfs_alphasort);
                    res = count < 0 ? count : 0,
                    action_error_retryskipcancel_ign,
                    return ACTION_CANCEL_TO_ABORT (__dlg_res_),
                    _(L"Cannot listing directory \"%ls\":\n%ls"),
                    __full_name, vfs_get_error (res));
    }

  len = wcslen (__full_name) + MAX_FILENAME_LEN + 1;
  full_name = malloc ((len + 1) * sizeof (wchar_t));

  /* Process children */
  global_res = ACTION_OK;
  for (i = 0; i < count; ++i)
    {
      if (!IS_PSEUDODIR (eps[i]->name))
        {
          vfs_stat_t stat;

          /* Get full filename of current file or directory */
          swprintf (full_name, len, L"%ls/%ls", __full_name, eps[i]->name);

          /* Stat file or directory */
          ACTION_REPEAT (res = vfs_stat (full_name, &stat),
                    action_error_retryskipcancel,
                    res = ACTION_CANCEL_TO_ABORT (__dlg_res_),
                    _(L"Cannot stat file or directory \"%ls\":\n%ls"),
                    full_name, vfs_get_error (res));

          if (!res)
            {
              if (S_ISDIR (stat.st_mode))
                {
                  res = process_directory (full_name,
                                           prescanned ? __tree->items[i]:NULL,
                                           __operation, __before_rec_op,
                                           __after_rec_op, stat, __proc_wnd,
                                           __user_data);
                }
              else
                {
                  res = make_operation (__operation, full_name, stat,
                                        __user_data, __proc_wnd, 0);
                  inc_progress_pos (__proc_wnd);
                }
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

  if (global_res == ACTION_OK)
    {
      unsigned int flags;

      flags = ignored_items > 0 ? AOF_DIR_IGNORED_ITEMS : 0;
      global_res = make_operation (__after_rec_op, __full_name, __stat,
                                   __user_data, __proc_wnd, flags);
    }

  /* Operating with directory is finished */
  /* We could increment position of progress bar */
  inc_progress_pos (__proc_wnd);

  return global_res;
}

/**
 * Call user's handlers in case of recursively operating
 *
 * @param __full_name - name of file to operate with
 * @param __operation - action which will be called for non-directories
 * in recursively operating and for all objects in non-recursively operating
 * @param __before_rec_op - action which will be called before
 * recursively sinking
 * @param __before_rec_op - action which will be called after
 * recursively sinking
 * @param __stat - stat information of file or directory
 * @param __proc_wnd - descriptor of process window
 * @param __user_data - user defined data which will be send to operator
 * @return zero on success, non-zero otherwise
 */
static int
make_recursively_call (const wchar_t *__full_name,
                       action_operator_t __operation,
                       action_operator_t __before_rec_op,
                       action_operator_t __after_rec_op, vfs_stat_t __stat,
                       process_window_t *__proc_wnd,
                       void *__user_data)
{
  int res;

  if (S_ISDIR (__stat.st_mode))
    {
      res = process_directory (__full_name, NULL, __operation, __before_rec_op,
                               __after_rec_op, __stat,
                               __proc_wnd,  __user_data);
    }
  else
    {
      res = make_operation (__operation, __full_name, __stat, __user_data,
                            __proc_wnd, 0);
      inc_progress_pos(__proc_wnd);
    }

  return res;
}

/********
 * User's backend
 */

/**
 * Operate on items from specified list
 *
 * @param __caption - caption of process window
 * @param __desc - description of operation on process window
 * @param __panel - panel for which items are belong to
 * @param __base_dir - base directory of items
 * @param __list - list of items to operate with
 * @param __count - count of items
 * @param __recursively - do recursively sinking into directories
 * @param __prescan - is prescanning allowed?
 * @param __operation - action which will be called for non-directories
 * in recursively operating and for all objects in non-recursively operating
 * @param __before_rec_op - action which will be called before
 * recursively sinking
 * @param __before_rec_op - action which will be called after
 * recursively sinking
 * @param __user_data - user defined data which will be send to operator
 * @return zero on success, non-zero otherwise
 */
int
action_operate (const wchar_t *__caption, const wchar_t *__desc,
                file_panel_t *__panel,
                const wchar_t *__base_dir, const file_panel_item_t **__list,
                unsigned long __count, BOOL __recursively, BOOL __prescan,
                BOOL __follow_symlinks,
                action_operator_t __operation,
                action_operator_t __before_rec_op,
                action_operator_t __after_rec_op,
                void *__user_data)
{
  int res;
  vfs_stat_t stat;
  unsigned long i, j, source_count = __count, count = 0;
  BOOL scanned = FALSE;
  action_listing_t listing;
  wchar_t *name, *full_name;
  file_panel_item_t *item;
  process_window_t *proc_wnd;

  /* Create and show process window */
  proc_wnd = create_proc_wnd (__caption, __desc, __recursively || __prescan);
  w_window_show (proc_wnd->window);

  if (__prescan && __recursively)
    {
      ACTION_REPEAT (res = action_get_listing (__base_dir, __list,
                                               __count, &listing, FALSE, TRUE);
                     if (res == ACTION_ABORT)
                       {
                         destroy_proc_wnd (proc_wnd);
                         return ACTION_ABORT;
                       },
                     action_error_retryskipcancel_ign,
                     destroy_proc_wnd (proc_wnd); return ACTION_ABORT,
                     _(L"Cannot get listing of items:\n%ls"),
                     vfs_get_error (res));

      if (!res)
        {
          /* User can ignore some subtrees, so we need */
          /* get count of source elements from prescanned data */
          source_count = listing.tree->count;
          scanned = TRUE;

           w_progress_set_max (proc_wnd->progress, listing.count);
        }
      else
        {
          proc_wnd->manual_total_count = TRUE;
        }
    }

  if (!scanned)
    {
      w_progress_set_max (proc_wnd->progress, source_count);
      proc_wnd->manual_total_count = TRUE;
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
      full_name = wcdircatsubdir (__base_dir, name);

      /* Get mode of file */
      if (item)
        {
          if (__follow_symlinks)
            {
              stat = item->file->stat;
            }
          else
            {
              stat = item->file->lstat;
            }
        }
      else
        {
          /*
           * TODO: We'd better use ACTION_REPEAT() here
           */
          if (__follow_symlinks)
            {
              vfs_stat (full_name, &stat);
            }
          else
            {
              vfs_lstat (full_name, &stat);
            }
        }

      if (__recursively)
        {
          res = make_recursively_call (full_name, __operation, __before_rec_op,
                                       __after_rec_op, stat,
                                       proc_wnd, __user_data);
        }
      else
        {
          res = make_operation (__operation, full_name, stat, __user_data,
                                proc_wnd, 0);
          if (proc_wnd->manual_total_count)
            {
              inc_progress_pos (proc_wnd);
            }
        }
      free (full_name);

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
      hook_call (L"switch-task-hook", NULL);

      if (proc_wnd->abort)
        {
          break;
        }
    }

  destroy_proc_wnd (proc_wnd);

  if (scanned)
    {
      action_free_listing (&listing);
    }

  if (__panel->items.selected_count >= count )
    {
      __panel->items.selected_count -= count;
    }

  return ACTION_OK;
}
