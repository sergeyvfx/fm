/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of common actions
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
#include "messages.h"

#define FORMAT_OUT_BUF(_params...)\
  { \
    wchar_t *format; \
    size_t len = wcslen (pre_format) + wcslen (__stencil) + 1024; \
    format = malloc ((len + 1) * sizeof (wchar_t)); \
 \
    swprintf (format, len + 1, __stencil, pre_format); \
    swprintf (__buf, __buf_size, _(format), ##_params); \
 \
    free (format); \
  }

/**
 * Format message for action
 *
 * @param __list - list of selected items
 * @param __count - count of selected items
 * @param __stencil - stencil of message
 * @param __buf - buffer where result will be saved
 * @param __buf_size - size of buffer
 */
void
action_message_formatting (const file_panel_item_t **__list,
                           unsigned long __count, const wchar_t *__stencil,
                           wchar_t *__buf, size_t __buf_size)
{
  wchar_t *pre_format;

  if (__count == 1)
    {
      /* Single file is copying */
      wchar_t *src;
      wchar_t fit_fn[32];
      file_panel_item_t *item;

      item = (file_panel_item_t*)__list[0];
      src = item->file->name;

      if (S_ISDIR (item->file->lstat.st_mode))
        {
          pre_format = L"directory \"%ls\"";
        }
      else
        {
          pre_format = L"file \"%ls\"";
        }

      fit_filename (src, BUF_LEN (fit_fn) - 1, fit_fn);

      FORMAT_OUT_BUF (fit_fn);
    }
  else
    {
      /* Coping list of file */

      BOOL only_files = TRUE, only_dirs = TRUE;
      unsigned long i;

      /* Need to determine only files, only directories or both */
      /* of files and directories are to be copied */
      for (i = 0; i < __count; i++)
        {
          if (S_ISDIR (__list[i]->file->lstat.st_mode))
            {
              only_files = FALSE;

              if (!only_dirs)
                {
                  break;
                }
            }
          else
            {
              only_dirs = FALSE;

              if (!only_files)
                {
                  break;
                }
            }
        }

      /* Determine format string */
      if (only_files)
        {
          pre_format = L"%lu files";
        }
      else
        if (only_dirs)
          {
            pre_format = L"%lu directories";
          }
        else
          {
            pre_format = L"%lu files/directories";
          }

      FORMAT_OUT_BUF (__count);
    }
}

/**
 * Enshure that pseydodirs are not selected
 *
 * @param __list - list of selected items
 * @param __count - count of items in list
 * @return non-zero if no pseydodirs are selected, zero otherwise
 */
BOOL
action_check_no_pseydodir (const file_panel_item_t **__list,
                           unsigned long __count)
{
  /*
   * NOTE: I hope that file panel cannot give access to select
   *       pseudo-directories. So, only item under cursor may be
   *       a pseudo-directory.
   *       Lets check it.
   */
  if (__count == 1 && IS_PSEUDODIR (__list[0]->file->name))
    {
      return FALSE;
    }

  return TRUE;
}

/**
 * Display an error message with buttons Retry and Cancel
 *
 * @param __text - text to display on message
 * @return result of message_box()
 */
int
action_error_retrycancel (const wchar_t *__text, ...)
{
  wchar_t buf[4096];
  PACK_ARGS (__text, buf, BUF_LEN (buf));
  return message_box (_(L"Error"), buf, MB_CRITICAL | MB_RETRYCANCEL);
}

/**
 * Display an error message with buttons Retry, Skip and cancel
 *
 * @param __text - text to display on message
 * @return result of message_box()
 */
int
action_error_retryskipcancel (const wchar_t *__text, ...)
{
  wchar_t buf[4096];
  PACK_ARGS (__text, buf, BUF_LEN (buf));
  return message_box (_(L"Error"), buf, MB_CRITICAL | MB_RETRYSKIPCANCEL);
}

/**
 * Display an error message with buttons Retry, Skip and Cancel,
 * but modal result for MR_SKIP will be replaced with MR_IGNORE.
 *
 * Need this to make to make error messages equal but with different semantic
 * of Ignore/Skip actions.
 *
 * @param __text - text to display on message
 * @return result of message_box()
 */
int
action_error_retryskipcancel_ign (const wchar_t *__text, ...)
{
  int res;
  wchar_t buf[4096];
  PACK_ARGS (__text, buf, BUF_LEN (buf));
  res = action_error_retryskipcancel (L"%ls", buf);
  if (res == MR_SKIP)
    {
      return MR_IGNORE;
    }
  return res;
}

/**
 * Create buttons `Ok` and `Cancel` on specified window
 *
 * @param __window - descri[tor of window on which buttons will be created
 */
void
action_create_ok_cancel_btns (w_window_t *__window)
{
  int dummy, left;
  w_button_t *btn;

  if (!__window)
    {
      return;
    }

  dummy = widget_shortcut_length (_(L"_Ok"));
  left = (__window->position.width - dummy -
          widget_shortcut_length (_(L"_Cancel")) - 11) / 2;

  btn = widget_create_button (NULL, WIDGET_CONTAINER (__window),
                              _(L"_Ok"), left,
                              __window->position.height - 2, WBS_DEFAULT);
  btn->modal_result = MR_OK;

  left += dummy + 7;
  btn = widget_create_button (NULL, WIDGET_CONTAINER (__window), _(L"_Cancel"),
                              left, __window->position.height - 2, 0);
  btn->modal_result = MR_CANCEL;
}

/**
 * Create an array of buttons on specified window
 *
 * @param __window - descriptor of window where buttons will be created
 * @param __buttons - array of buttons' descriptions
 * @param __count - count of buttons to create
 * @param __out - if is not NULL, pointer to array where
 * buttons' descriptors where saved
 */
void
action_create_buttons (w_window_t *__window,
                       const action_button_t *__buttons, int __count,
                       w_button_t **__out)
{
  int i, left, total;
  wchar_t *pchar;
  w_button_t *btn;

  if (!__window)
    {
      return;
    }

  left = total = 0;

  /* Get total buttons' width */
  for (i = 0; i < __count; ++i)
    {
      total += widget_shortcut_length (_(__buttons[i].caption)) + 4;

      if (__buttons[i].def)
        {
          total += 2;
        }
    }
  total += __count - 1;

  left = (__window->position.width - total) / 2;

  /* Create buttons */
  for (i = 0; i < __count; ++i)
    {
      pchar = _(__buttons[i].caption);
      btn = widget_create_button (NULL, WIDGET_CONTAINER (__window),
                                  pchar, left,
                                  __window->position.height - 2,
                                  __buttons[i].def ? WBS_DEFAULT : 0);

      if (__out)
        {
          __out[i] = btn;
        }

      btn->modal_result = __buttons[i].modal_result;
      left += widget_shortcut_length (pchar) + 5;
      if (__buttons[i].def)
        {
          left += 2;
        }
    }
}

/**
 * Check are there any selected directories
 *
 * @param __list - list of selected items
 * @param __count - count of selected items
 * @return non-zero in case at least one directory is selected and
 * zero otherwise
 */
BOOL
action_is_directory_selected (const file_panel_item_t **__list,
                              unsigned long __count)
{
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

/**
 * Centre cursor to specified item on file panel
 *
 * @param __panel - on which panel item will be centred
 * @param __item_name - full name of item to centre to
 */
void
action_centre_to_item (file_panel_t *__panel, const wchar_t *__item_name)
{
  wchar_t *n_dir, *cwd;

  /* Get full normalized item name */
  n_dir = vfs_normalize (__item_name);

  /* Get CWD of panel */
  cwd = file_panel_get_full_cwd (__panel);

  if (wcsncmp (cwd, n_dir, wcslen (cwd)) == 0)
    {
      wchar_t *rel_dir = n_dir + wcslen (cwd);

      /* Get name of item to select */
      if (rel_dir[0] == '/')
        {
          long i, len = 0;
          wchar_t name[MAX_FILENAME_LEN + 1] = {0};
          ++rel_dir;

          i = 0;
          while (rel_dir[i] && rel_dir[i] != '/')
            {
              name[len++] = rel_dir[i++];
            }
          name[len] = 0;

          FILE_PANEL_ACTION_CALL (__panel, centre_to_item, name);
        }
    }

  free (n_dir);
}
