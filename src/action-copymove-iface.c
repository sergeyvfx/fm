/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Interface part of copy/move operations
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "action-copymove-iface.h"
#include "i18n.h"
#include "util.h"
#include "dir.h"

/********
 * Macro definitions
 */

/**
 * Create text on window with question about file existent
 */
#define EXIST_QUESTION_TEXT(_y, _format, _args...) \
  { \
    swprintf (buf, BUF_LEN (buf), _(_format), ##_args); \
    text=widget_create_text (WIDGET_CONTAINER (wnd), buf, 1, _y); \
    w_text_set_font (text, &FONT (CID_WHITE,  CID_RED)); \
  }

/**
 * Create button on window with question about file existent
 */
#define EXIST_QUESTION_BUTTON(_y, _caption, _modal_result) \
  { \
    btn=widget_create_button (WIDGET_CONTAINER (wnd), _(_caption), \
      cur_left, _y, 0); \
    btn->modal_result=_modal_result; \
    cur_left+=widget_shortcut_length (_(_caption))+5; \
    w_button_set_fonts (btn, \
      &FONT (CID_WHITE,  CID_RED), \
      &FONT (CID_BLACK,  CID_WHITE), \
      &FONT (CID_YELLOW, CID_RED), \
      &FONT (CID_YELLOW, CID_WHITE)); \
  }

/********
 * Stuff related to CopyProgressWindow
 */

/**
 * Handler of clicking button 'skip' on process window
 *
 * @param __button - sender button
 * @return non-zero if action has been handled, non-zero otherwise
 */
static int
skip_button_clicked (w_button_t *__button)
{
  copy_process_window_t *wnd;
  if (!__button | !WIDGET_USER_DATA (__button))
    {
      return 0;
    }

  wnd=WIDGET_USER_DATA (__button);

  wnd->skip = TRUE;

  return TRUE;
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
  copy_process_window_t *wnd;
  if (!__button || !WIDGET_USER_DATA (__button))
    {
      return 0;
    }

  wnd=WIDGET_USER_DATA (__button);

  wnd->abort = TRUE;

  return TRUE;
}

/**
 * Handler of keydown message for buttons on process window
 *
 * @param __button - button on which key was pressed
 * @param __ch - code of pressed key
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
      /* If escaped was pressed, copy operation shoud be aborted */
      copy_process_window_t *wnd;
      wnd=WIDGET_USER_DATA (__button);
      wnd->abort = TRUE;
    }

  return 0;
}

/********
 * Stuff for file exists dialog
 */

/**
 * Return width of file exists message
 *
 * @return width of message box
 */
static int
file_exists_msg_width (void)
{
  int res = 55, dummy;

  dummy = wcslen (_(L"Overwrite this target?")) +
          widget_shortcut_length (_(L"_Yes")) +
          widget_shortcut_length (_(L"_No")) +
          widget_shortcut_length (_(L"A_ppend")) + 17;
  res = MAX (res, dummy);

  dummy = wcslen (_(L"Overwrite all targets?")) +
          widget_shortcut_length (_(L"A_ll")) +
          widget_shortcut_length (_(L"_Update")) +
          widget_shortcut_length (_(L"Non_e")) + 17;
  res = MAX (res, dummy);

  return res;
}

/**
 * Format information of file for file_exists_question()
 *
 * @param __use_lstat - use vfs_lstat() instead of vfs_stat()
 * @param __buf - destination buffer
 * @param __buf_size - size of buffer
 * @param __url - URL of file for which information is generating
 */
static void
format_exists_file_data (BOOL __use_lstat, wchar_t *__buf,
                         size_t __buf_size, const wchar_t *__url)
{
  wchar_t date[100];
  vfs_stat_t stat;

  if (__use_lstat)
    {
      vfs_lstat (__url, &stat);
    }
  else
    {
      vfs_stat (__url, &stat);
    }

  format_file_time (date, 100, stat.st_mtime);

#ifdef __USE_FILE_OFFSET64
  swprintf (__buf, __buf_size, _(L"date %ls, size %lld bytes"),
            date, stat.st_size);
#else
  swprintf (__buf, __buf_size, _(L"date %ls, size %ld bytes"),
            date, stat.st_size);
#endif
}

/********
 * Stuff for copy dialog
 */

/**
 * Get caption for field 'To' on file copy dialog
 *
 * @param __move - are files will be moved?
 * @param __src_list - list of source items
 * @param __count - count of items to be copied
 * @param __buf - buffer where result will be stored
 * @param __buf_size - size of buffer
 */
static void
get_to_field_caption (BOOL __move, const file_panel_item_t **__src_list,
                      unsigned long __count,
                      wchar_t *__buf, size_t __buf_size)
{
  wchar_t *stencil;

  if (__move)
    {
      stencil = L"Move %ls to:";
    }
  else
    {
      stencil = L"Copy %ls to:";
    }

  action_message_formatting (__src_list, __count, stencil, __buf, __buf_size);
}

/********
 * User's backend
 */

/**
 * Create file copy process window
 *
 * @param __move - are files will be moved?
 * @param __total_progress - is total progress information avaliable
 * @param __listing - listing to get summary information
 * @return created window
 */
copy_process_window_t*
action_copy_create_proc_wnd (BOOL __move, BOOL __total_progress,
                             const action_listing_t *__listing)
{
  copy_process_window_t *res;
  w_container_t *cnt;
  w_button_t *btn;
  int left, height;
  wchar_t *pchar;

  MALLOC_ZERO (res, sizeof (copy_process_window_t))

  if (__total_progress)
    {
      height = 13;
    }
  else
    {
      height = 10;
    }

  res->window = widget_create_window (__move?_(L"Move"):_(L"Copy"), 0, 0,
                                      59, height, WMS_CENTERED);
  cnt = WIDGET_CONTAINER (res->window);

  /* Names of source and destination files */
  res->source = widget_create_text (cnt, L"", 1, 1);
  res->target = widget_create_text (cnt, L"", 1, 2);

  /* Progress bar for current file */
  widget_create_text (cnt, _(L"File"), 1, 4);
  res->file_progress = widget_create_progress (cnt, 100, 10, 4, 48, 0);

  /* Speed */
  pchar = _(L"Speed:");
  widget_create_text (cnt, pchar, 1, 6);
  res->speed_text = widget_create_text (cnt, _(L"-,--Mbps"),
                                        wcswidth (pchar, wcslen (pchar)) + 2,
                                        6);

  pchar = _(L"ETA:");
  left = 30;
  widget_create_text (cnt, pchar, left, 6);
  res->eta_text = widget_create_text (cnt, L"--:--:--",
                                   wcswidth (pchar, wcslen (pchar)) + left + 1,
                                   6);

  /* Create progess bars for displaying total progress */
  if (__total_progress)
    {
      wchar_t text[1024];

      swprintf (text, BUF_LEN (text), _(L"(%lldKb of %lldKb)"),
                res->bytes_copied, __listing->size / 1024);
      widget_create_text (cnt, _(L"Bytes"), 1, 8);
      res->bytes_digit = widget_create_text (cnt, text,
                                             2 + wcslen (_(L"Bytes")), 8);
      res->bytes_progress = widget_create_progress (cnt, __listing->size,
                                                    1, 9, 28,
                                                    WPBS_NOPERCENT);
      res->bytes_total = __listing->size;

      swprintf (text, BUF_LEN (text), _(L"(%lld of %lld)"), res->files_copied,
                __listing->count);
      widget_create_text (cnt, _(L"Count"), 30, 8);
      res->count_digit = widget_create_text (cnt, text,
                                             31 + wcslen (_(L"Count")), 8);
      res->count_progress = widget_create_progress (cnt, __listing->count,
                                                    30, 9, 28,
                                                    WPBS_NOPERCENT);
      res->files_total = __listing->count;
    }

  /* Create buttons */
  left = (cnt->position.width - widget_shortcut_length (_(L"_Skip")) -
                  widget_shortcut_length (_(L"_Abort")) - 9) / 2;

  btn = widget_create_button (cnt, _(L"_Skip"), left,
                              cnt->position.height - 2, 0);

  WIDGET_USER_DATA (btn) = res;
  WIDGET_USER_CALLBACK (btn, clicked) = (widget_action)skip_button_clicked;
  WIDGET_USER_CALLBACK (btn, keydown) = (widget_keydown_proc)button_keydown;

  left += widget_shortcut_length (_(L"_Skip")) + 5;
  btn = widget_create_button (cnt, _(L"_Abort"), left,
                              cnt->position.height - 2, 0);

  WIDGET_USER_DATA (btn) = res;
  WIDGET_USER_CALLBACK (btn, clicked) = (widget_action)abort_button_clicked;
  WIDGET_USER_CALLBACK (btn, keydown) = (widget_keydown_proc)button_keydown;

  res->prev_timestamp = res->speed_timestamp = res->timestamp = now ();
  res->move = __move;
  res->move_strategy = MOVE_STRATEGY_UNDEFINED;
  if (__move)
    {
      res->unlink_list = deque_create ();
    }

  return res;
}

/**
 * Destroy file copy process window
 *
 * @param __window - window to be destroyed
 */
void
action_copy_destroy_proc_wnd (copy_process_window_t *__window)
{
  if (!__window)
    {
      return;
    }

  widget_destroy (WIDGET (__window->window));

  /* Free list of items to unlink */
  if (__window->unlink_list)
    {
      void deleter (void *__data)
      {
        free (__data);
      }

      deque_destroy (__window->unlink_list, deleter);
    }

  free (__window);
}

/**
 * Evalute and set speed of copying and ETA
 *
 * @param __proc_wnd - descriptor of a process window
 */
void
action_copy_eval_speed (copy_process_window_t *__proc_wnd)
{
  timeval_t tv_delta;
  __u64_t time_delta;

  /* Calculate time delta */
  tv_delta = timedist (__proc_wnd->prev_timestamp,
                       __proc_wnd->speed_timestamp);
  time_delta = tv_delta.tv_sec * 1000000 + tv_delta.tv_usec;

  /* To avoid division by zero */
  if (time_delta > 0)
    {
      /* Calculate copying speed */
      double speed;
      static wchar_t *format = NULL;
      __u64_t bytes_copied, bytes_total, bytes_delta, elapsed;
      timeval_t t;
      int eta;
      wchar_t msg[100];

      if (format == NULL)
        {
          /* For some optimization */
          format = _(L"%.2lfMbps");
        }

      /* Calculate bytes delta */
      bytes_delta = __proc_wnd->bytes_copied - __proc_wnd->prev_copied;

      speed = 1000000.0 / time_delta * bytes_delta / 1024 / 1024;

      swprintf (msg, BUF_LEN (msg), format, speed);
      w_text_set (__proc_wnd->speed_text, msg);

      /*
       * TODO: In case of milliards of non-regular files or
       *       files which are very small this method is useless.
       *       Fix it!
       */

      /* Calculate ETA */
      if (__proc_wnd->bytes_progress != NULL)
        {
          /* If there is bytes_progress created */
          /* it means that we can use __proc_wnd->bytes_total */
          bytes_copied = __proc_wnd->bytes_copied;
          bytes_total = __proc_wnd->bytes_total;
        }
      else
        {
          /* We can't use __proc_wnd->bytes_total*/
          /* Count of copied bytes can be get from __proc_wnd->file_progress */
          /* And size of file can be get from __proc_wnd->file_progress */
          bytes_copied = w_progress_get_pos (__proc_wnd->file_progress);
          bytes_total = w_progress_get_max (__proc_wnd->file_progress);
        }

      /* Get elapsed count of seconds */
      t = timedist (__proc_wnd->timestamp, now ());

      /*
       * NOTE: I suppose we may ignore t.tv_usec and avoid this stupid
       *       but who knows? Maybe this field cat store more than one second?
       */

      elapsed = t.tv_sec + t.tv_usec / 1000000;

      eta = elapsed * ((double)bytes_total / bytes_copied - 1);
      swprintf (msg, BUF_LEN (msg), L"%02ld:%02ld:%02ld", eta / 3600,
                eta / 60 % 60, eta % 60);
      w_text_set (__proc_wnd->eta_text, msg);
    }

  /* Re-new stored information  */
  __proc_wnd->prev_timestamp = __proc_wnd->speed_timestamp;
  __proc_wnd->prev_copied = __proc_wnd->bytes_copied;
}

/**
 * Show question when destination file already exists
 *
 * @param __src - URL of source file
 * @param __dst - URL of destination file
 * @param __use_lstat - use vfs_lstat() instead of vfs_stat()
 * @return modal result of question
 */
int
action_copy_exists_dialog (const wchar_t *__src, const wchar_t *__dst,
                           BOOL __use_lstat)
{
  w_window_t *wnd;
  w_text_t *text;
  w_button_t *btn;
  wchar_t buf[4096], dummy[4096];
  int buttons_left, cur_left, fn_len, res;
  static int width = 0;

  if (!width)
    {
      width = file_exists_msg_width ();
    }

  /* Create question window */
  wnd = widget_create_window (_(L"File exists"), 0, 0, width,
                              13, WMS_CENTERED);
  w_window_set_fonts (wnd, &FONT (CID_WHITE, CID_RED),
                      &FONT (CID_YELLOW, CID_RED));

  fn_len = width - wcslen (_(L"Target file \"%ls\" already exists!"));
  fit_dirname (__dst, fn_len, dummy);
  EXIST_QUESTION_TEXT (1, L"Target file \"%ls\" already exists!", dummy);

  format_exists_file_data (__use_lstat, dummy, BUF_LEN (dummy), __src);
  EXIST_QUESTION_TEXT (3, L"Source: %ls", dummy);

  format_exists_file_data (__use_lstat, dummy, BUF_LEN (dummy), __dst);
  EXIST_QUESTION_TEXT (4, L"Target: %ls", dummy);

  EXIST_QUESTION_TEXT (6, L"Overwrite this target?");
  EXIST_QUESTION_TEXT (8, L"Overwrite all targets?");

  buttons_left = MAX (wcslen (_(L"Overwrite this target?")),
                      wcslen (_(L"Overwrite all targets?"))) + 2;

  cur_left = buttons_left;
  EXIST_QUESTION_BUTTON (6, L"_Yes", MR_YES);
  EXIST_QUESTION_BUTTON (6, L"_No", MR_NO);
  EXIST_QUESTION_BUTTON (6, L"A_ppend", MR_COPY_APPEND);

  cur_left = buttons_left;
  EXIST_QUESTION_BUTTON (8, L"A_ll", MR_COPY_REPLACE_ALL);
  EXIST_QUESTION_BUTTON (8, L"_Update", MR_COPY_UPDATE);
  EXIST_QUESTION_BUTTON (8, L"Non_e", MR_COPY_NONE);
  cur_left = buttons_left;
  EXIST_QUESTION_BUTTON (9, L"If _size differs", MR_COPY_SIZE_DIFFERS);

  cur_left = (wnd->position.width - widget_shortcut_length (_(L"_Abort"))
          - 4) / 2;

  EXIST_QUESTION_BUTTON (11, L"_Abort", MR_ABORT);

  res = w_window_show_modal (wnd);

  if (res == MR_CANCEL)
    {
      res = MR_ABORT;
    }

  /* Destroy any created data */
  widget_destroy (WIDGET (wnd));

  return res;
}

/**
 * Show copy dialog to confirm destination file name
 * and other additional information
 *
 * @param __move - are files will be moved?
 * @param __src_list - list of source items
 * @param __count - count of items to be copied
 * @param __dst - default destination
 * @return MR_CANCEL if user canceled copying, MR_OK otherwise
 */
int
action_copy_show_dialog (BOOL __move, const file_panel_item_t **__src_list,
                         unsigned long __count, wchar_t **__dst)
{
  int res, left, dummy;
  w_window_t *wnd;
  w_button_t *btn;
  w_edit_t *to;
  w_container_t *cnt;
  wchar_t msg[1024];

  wnd = widget_create_window (__move?_(L"Move"):_(L"Copy"),
                              0, 0, 50, 6, WMS_CENTERED);
  cnt = WIDGET_CONTAINER (wnd);

  /* Create caption for 'To' field */
  get_to_field_caption (__move, __src_list, __count, msg, BUF_LEN (msg));
  widget_create_text (cnt, msg, 1, 1);

  /* Create 'To' field */
  to = widget_create_edit (cnt, 1, 2, wnd->position.width - 2);
  w_edit_set_text (to, *__dst);

  /* Create buttons */
  dummy = widget_shortcut_length (_(L"_Ok"));
  left = (wnd->position.width - dummy -
          widget_shortcut_length (_(L"_Cancel")) - 11) / 2;

  btn = widget_create_button (cnt, _(L"_Ok"), left,
                              wnd->position.height - 2, WBS_DEFAULT);
  btn->modal_result = MR_OK;

  left += dummy + 7;
  btn = widget_create_button (cnt, _(L"_Cancel"), left,
                              wnd->position.height - 2, 0);
  btn->modal_result = MR_CANCEL;

  res = w_window_show_modal (wnd);

  /* Return values from dialog */
  *__dst = wcsdup (w_edit_get_text (to));

  widget_destroy (WIDGET (wnd));

  return res;
}

/**
 * Create a post-move information window
 * In this window will be displayed information about unlinking files
 *
 * @return descriptor of a post-move window
 */
post_move_window_t*
action_post_move_create_window (void)
{
  post_move_window_t *res;
  wchar_t *s;

  MALLOC_ZERO (res, sizeof (post_move_window_t));
  res->window = widget_create_window (_(L"Move"), 0, 0, 60, 6, WMS_CENTERED);

  s = _(L"Deleting:");
  widget_create_text (WIDGET_CONTAINER (res->window), s, 1, 1);

  res->file = widget_create_text (WIDGET_CONTAINER (res->window), L"",
                                  wcswidth (s, wcslen (s)) + 2, 1);

  res->progress = widget_create_progress (WIDGET_CONTAINER (res->window),
                                          100, 1, 3,
                                          res->window->position.width - 2,
                                          WPBS_NOPERCENT);

  return res;
}

/**
 * Destroy a post-move information window
 *
 * @param __window - window to be destroyed
 */
void
action_post_move_desstroy_window (post_move_window_t *__window)
{
  if (!__window)
    {
      return;
    }

  widget_destroy (WIDGET (__window->window));
  free (__window);
}
