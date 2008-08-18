/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Interface part of copy operation
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
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

/********
 * Constants
 */

/* Modal results for overwrite answers */
#define MR_COPY_APPEND       (MR_CUSTOM+1)
#define MR_COPY_REPLACE_ALL  (MR_CUSTOM+2)
#define MR_COPY_UPDATE       (MR_CUSTOM+3)
#define MR_COPY_SIZE_DIFFERS (MR_CUSTOM+4)
#define MR_COPY_NONE         (MR_CUSTOM+5)

/********
 * Type definitions
 */

typedef struct
{
  w_window_t *window;

  w_text_t *source;
  w_text_t *target;

  w_progress_t *file_progress;
  w_progress_t *bytes_progress;
  w_progress_t *count_progress;

  w_text_t *bytes_digit;
  w_text_t *count_digit;

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

  __u64_t bytes_total;
  __u64_t files_total;

  __u64_t bytes_copied;
  __u64_t files_copied;

  timeval_t timestamp;

  __u64_t prev_copied;
  timeval_t speed_timestamp;
  timeval_t prev_timestamp;
} copy_process_window_t;

/********
 *
 */

/* Create file copy process window */
copy_process_window_t*
action_copy_create_proc_wnd (BOOL __total_progress,
                             const action_listing_t *__listing);

/* Destroy file copy process window */
void
action_destroy_proc_wnd (copy_process_window_t *__window);

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
action_copy_show_dialog (const file_panel_item_t **__src_list,
                         unsigned long __count, wchar_t **__dst);

END_HEADER

#endif
