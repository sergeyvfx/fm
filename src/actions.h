/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Prototypes of different actions
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _actions_h_
#define _actions_h_

#include "smartinclude.h"

BEGIN_HEADER

#include "file_panel.h"
#include "deque.h"
#include <vfs/vfs.h>

/********
 * Constants
 */

/* Error codes */
#define ACTION_OK     0
#define ACTION_ERR   -1
#define ACTION_ABORT  1
#define ACTION_IGNORE 2
#define ACTION_SKIP   3

/********
 * Macro definitions
 */

#include <action-listing.h>


/**
 * Make action and if it failed asks to retry this action
 *
 * NOTE: You need to have defined integer variable res.
 *       This variable will be used to control _act's exit status
 *
 * @param _act - action to make
 * @param _error_proc - procedure which displays an error message
 * @param _error_act - action, which will be made made in case of error
 * @param _error_args - arguments, which will be passed to _error_proc
 */
#define ACTION_REPEAT(_act, _error_proc, _error_act, _error, _error_args...) \
  do { \
    _act; \
    /* Error while making action */ \
    if (res) \
      { \
        int __dlg_res_ = _error_proc (_error, ##_error_args); \
        /* Review user's decision */ \
        if (__dlg_res_ == MR_IGNORE) \
          { \
            /* User ignored error */ \
            break; \
          } \
        if (__dlg_res_ != MR_RETRY) \
          { \
            /* User doen't want to continue trying */ \
            _error_act; \
            break; \
          } \
      } else { \
        break; \
      } \
  } while (TRUE);

/********
 *
 */

/**
 * Helpers
 */

/* Format message for action */
void
action_message_formatting (const file_panel_item_t **__list,
                           unsigned long __count, const wchar_t *__stencil,
                           wchar_t *__buf, size_t __buf_size);

/* Enshure that pseydodirs are not selected */
BOOL
action_check_no_pseydodir (const file_panel_item_t **__list,
                           unsigned long __count);

/**
 *
 */

/* Copy list of files from specified panel */
int
action_copy (file_panel_t *__panel);

/* Move/rename list of files from specified panel */
int
action_move (file_panel_t *__panel);

/* Create directory on specified panel */
int
action_mkdir (file_panel_t *__panel);

/* Delete list of files from specified panel */
int
action_delete (file_panel_t *__panel);

/* Make a symlink to file from specified panel */
int
action_symlink (file_panel_t *__panel);

/* Edit content of existing symbolic link */
int
action_editsymlink (file_panel_t *__panel);

/* Change owner of single file or group of files */
int
action_chown (file_panel_t *__panel);

/* Chooses file panel for action */
file_panel_t*
action_choose_file_panel (const wchar_t *__caption,
                          const wchar_t *__short_msg);

/**
 * Helpers
 */

int
action_error_retrycancel (const wchar_t *__text, ...);

END_HEADER

#endif
