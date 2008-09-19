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
 * Display an error message with buttons Retry and cancel
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
