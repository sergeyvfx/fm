/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Interface part of copy/move operations
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _action_copy_iface_h_
#define _action_copy_iface_h_

#include "smartinclude.h"

BEGIN_HEADER

#include "actions.h"
#include <widget.h>

#include <wchar.h>

#include "deque.h"

/********
 * Constants
 */

/* Modal results for overwrite answers */
#define MR_COPY_APPEND       (MR_CUSTOM+1)
#define MR_COPY_REPLACE_ALL  (MR_CUSTOM+2)
#define MR_COPY_UPDATE       (MR_CUSTOM+3)
#define MR_COPY_SIZE_DIFFERS (MR_CUSTOM+4)
#define MR_COPY_NONE         (MR_CUSTOM+5)

#define MOVE_STRATEGY_UNDEFINED (-1)

/********
 * Type definitions
 */

typedef struct
{
  /* Widget of window */
  w_window_t *window;

  /* Captions for source and destination file names */
  w_text_t *source;
  w_text_t *target;

  /* Progress bars for current file, total copied bytes and */
  /* total copied count */
  w_progress_t *file_progress;
  w_progress_t *bytes_progress;
  w_progress_t *count_progress;

  /* Captions for displaying copied bytes and count */
  /* as numbers */
  w_text_t *bytes_digit;
  w_text_t *count_digit;

  /* Captions for copying speed and ETA */
  w_text_t *speed_text;
  w_text_t *eta_text;

  /*
   * NOTE: This descriptor is very convenient to send additional
   *       information to deep-core functions and leave their
   *       parameter lists short.
   */

  /* Skip copying current file */
  BOOL skip;

  /* Abort copying operation */
  BOOL abort;

  /* Prefix of absolute directory names of sources */
  wchar_t *abs_path_prefix;

  /* Count of total bytes and files to copy */
  __u64_t bytes_total;
  __u64_t files_total;

  /* Count of processed bytes and files */
  __u64_t bytes_copied;
  __u64_t files_copied;

  /* Timestamp for evaluting speed and ETA */
  timeval_t timestamp;

  /* Different internal info  */
  __u64_t prev_copied;
  timeval_t speed_timestamp;
  timeval_t prev_timestamp;

  /* Data for moving operation */
  BOOL move;
  BOOL move_strategy;

  /* List of items (files/directories) to be unlinked after moving */
  deque_t *unlink_list;
  __u64_t unlink_count;
} copy_process_window_t;

typedef struct
{
  w_window_t *window;
  w_text_t *file;
  w_progress_t *progress;
} post_move_window_t;

/********
 *
 */

/* Create file copy process window */
copy_process_window_t*
action_copy_create_proc_wnd (BOOL __move, BOOL __total_progress,
                             const action_listing_t *__listing);

/* Destroy file copy process window */
void
action_copy_destroy_proc_wnd (copy_process_window_t *__window);

/* Evalute and set speed of copying and ETA */
void
action_copy_eval_speed (copy_process_window_t *__proc_wnd);

/* Show question when destination file already exists */
int
action_copy_exists_dialog (const wchar_t *__src, const wchar_t *__dst,
                           BOOL __use_lstat);

/* Show copy dialog to confirm destination file name */
/* and other additional information */
int
action_copy_show_dialog (BOOL __move, const file_panel_item_t **__src_list,
                         unsigned long __count, wchar_t **__dst);

/* Create a post-move information window */
post_move_window_t*
action_post_move_create_window (void);

/* Destroy a post-move information window */
void
action_post_move_desstroy_window (post_move_window_t *__window);

END_HEADER

#endif
