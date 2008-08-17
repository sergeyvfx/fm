/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of action 'copy'
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "actions.h"
#include "messages.h"
#include "i18n.h"
#include "dir.h"
#include "file.h"

#include <widget.h>

#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <wchar.h>

/********
 * Constants and other definitions
 */

/* Size of buffer in copying operation */
#define BUF_SIZE 65536

/* Maximal size of content of symbolic link */
#define MAX_SYMLINK_CONTENT 4096

/* Length of static wide-string buffer */
#define BUF_LEN(_buf) \
  sizeof (_buf)/sizeof (wchar_t)

/**
 * Close file descriptors in copy_file()
 */
#define CLOSE_FD() \
  { \
    vfs_close (fd_src); \
    vfs_close (fd_dst); \
  }

#define INCOMPLETE_MESSAGE() \
  message_box (_(L"Question"), \
               _(L"Incomplete file was retrieved. Keep it?"), \
               MB_YESNO | MB_CRITICAL)

/**
 * Close file descriptors and return a value from copy_regular_file()
 */
#define COPY_RETERR(_code) \
  CLOSE_FD (); \
  /* Check is file retrieved completely */ \
  if (_code && fd_dst) \
    { \
      if (INCOMPLETE_MESSAGE () == MR_NO) \
        { \
          vfs_unlink (__dst); \
        } \
      else \
        { \
          /* Set access and modification time of new file */ \
          vfs_utime (__dst, &times); \
        } \
    } \
  if (_code==MR_CANCEL) \
    _code=MR_ABORT; \
  return _code;

/**
 * Set current file name in copy_file()
 */
#define COPY_SET_FN(_fn, _dst, _text) \
  { \
    fit_filename (_fn, \
      __proc_wnd->window->position.width+1-wcslen (_(_text L": %ls")), fn); \
    swprintf (msg, BUF_LEN (msg), _(_text L": %ls"), fn); \
    w_text_set (__proc_wnd->_dst, msg); \
  }

/**
 * Make action and if it failed asks to retry this action
 */
#define REP(_act, _error_proc, _error_act, _error, _error_args...) \
  do { \
    _act; \
    /* Error while making action */ \
    if (res) \
      { \
        res=_error_proc (_error, ##_error_args); \
        /* Review user's decision */ \
        if (res==MR_IGNORE) \
          { \
            /* User ignored error */ \
            break; \
          } \
        if (res!=MR_RETRY) \
          { \
            /* User doen't want to continue trying */ \
            _error_act; \
          } \
      } else { \
        break; \
      } \
  } while (TRUE);

#define COPY_FILE_REP(_act, _error_proc, _error, _error_args...) \
  REP (_act, _error_proc, COPY_RETERR (res), _error, ##_error_args)

#define COPY_DIR_REP(_act, _error_proc, _error, _error_args...) \
  REP (_act, _error_proc, return res, _error, ##_error_args)

/**
 * Open file descriptor
 * If some errors occurred, asks user what to do
 */
#define OPEN_FD(_fd, _url, _flags, _res, _error, _error_args...) \
  COPY_FILE_REP (_fd=vfs_open (_url, _flags, &_res, 0), \
  error, _error, ##_error_args)

/**
 * Allocate memory for full file name in directory listing cycle
 */
#define ALLOC_FN(_s, _len, _src) \
  { \
    _len=wcslen (_src)+MAX_FILENAME_LEN+1; \
    _s=malloc ((_len+1)*sizeof (wchar_t)); \
  }

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

/* Get file overwrite rule */
#define GET_OWR_RULE(_use_lstat) \
  ((__owr_all_rule && *__owr_all_rule)? \
    (*__owr_all_rule): \
    (file_exists_question (__src, __dst, _use_lstat)))

/**
 * Save answer about overwriting all existent targets
 */
#define SAVE_OWR_ALL_RULE(_a) \
  if (__owr_all_rule) (*__owr_all_rule)=_a;

/*
 * Unlink symlink in copy_symlink()
 */
#define SYMLINK_UNLINK() \
  REP (res=vfs_unlink (__dst), error, \
  return (res==MR_CANCEL?MR_ABORT:res), \
   _(L"Cannot unlink target file \"%ls\":\n%ls"), __dst, \
  vfs_get_error (res))

/*
 * Copy process aborted?
 */
#define PROCESS_ABORTED() \
  (__proc_wnd->skip || __proc_wnd->abort)

/*
 * Process queue in copy_regular_file().
 */
#define COPY_PROCESS_QUEUE() \
  widget_process_queue (); \
  if (PROCESS_ABORTED()) \
    { \
      break; \
    }

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

#define UPDATE_TOTAL_PROGRESS(_progress, _value) \
  { \
    if (__proc_wnd->_progress) \
      { \
        w_progress_set_pos (__proc_wnd->_progress, \
                            w_progress_get_pos (__proc_wnd->_progress) + \
                                    _value); \
      } \
  }

#define SET_DIGIT_CAPTION(_caption, _format, _args...) \
  { \
    wchar_t text[1024]; \
    swprintf (text, BUF_LEN (text), _format, _args); \
    w_text_set (__proc_wnd->_caption, text); \
  }

#define BUFFER_COPIED(_size) \
  { \
    static wchar_t *format = NULL; \
    if (format == NULL) \
      { \
        format = _(L"(%lldKb of %lldKb)"); \
      } \
    w_progress_set_pos (__proc_wnd->file_progress, copied); \
    UPDATE_TOTAL_PROGRESS (bytes_progress, _size); \
    __proc_wnd->bytes_copied += _size; \
    if (iteration % 8 /* Magick constant */) \
      { \
        SET_DIGIT_CAPTION (bytes_digit, format, \
            __proc_wnd->bytes_copied / 1024, __proc_wnd->bytes_total / 1024); \
      } \
  }

#define FILE_COPIED() \
  { \
    static wchar_t *format = NULL; \
    if (format == NULL) \
      { \
        format = _(L"(%lld of %lld)"); \
      } \
    UPDATE_TOTAL_PROGRESS (count_progress, 1); \
    ++__proc_wnd->files_copied; \
    SET_DIGIT_CAPTION (count_digit, format, \
        __proc_wnd->files_copied, __proc_wnd->files_total); \
  }

/**
 * Modal results for overwrite answers
 */
#define MR_APPEND       (MR_CUSTOM+1)
#define MR_ALL          (MR_CUSTOM+2)
#define MR_UPDATE       (MR_CUSTOM+3)
#define MR_SIZE_DIFFERS (MR_CUSTOM+4)
#define MR_MY_NONE      (MR_CUSTOM+5)

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
} process_window_t;

/********
 * Global variables
 */

/* Recursively scanning before copying */
static BOOL scan = TRUE;

/********
 * Internal stuff
 */

/**
 * Display an error message with buttons Retry, Skip and cancel
 *
 * @param __text - text to display on message
 * @return result of message_box()
 */
static int
error (const wchar_t *__text, ...)
{
  wchar_t buf[4096];
  PACK_ARGS (__text, buf, BUF_LEN (buf));
  return message_box (_(L"Error"), buf, MB_CRITICAL | MB_RETRYSKIPCANCEL);
}

/**
 * Display an error message with buttons Retry, Ignore and cancel
 *
 * @param __text - text to display on message
 * @return result of message_box()
 */
static int
error2 (const wchar_t *__text, ...)
{
  wchar_t buf[4096];
  PACK_ARGS (__text, buf, BUF_LEN (buf));
  return message_box (_(L"Error"), buf, MB_CRITICAL | MB_RETRYIGNCANCEL);
}

/**
 * Return real destination filename
 *
 * @param __src - URL of source
 * @param __dst - URL of destination
 * @return real destination filename
 */
static wchar_t*
get_real_dst (const wchar_t *__src, const wchar_t *__dst)
{
  wchar_t *rdst, *fn = wcfilename (__src);
  rdst = wcdircatsubdir (__dst, fn);
  free (fn);
  return rdst;
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

  format_file_time (date, 100, stat.st_mtim.tv_sec);

#ifdef __USE_FILE_OFFSET64
  swprintf (__buf, __buf_size, _(L"date %ls, size %lld bytes"),
            date, stat.st_size);
#else
  swprintf (__buf, __buf_size, _(L"date %ls, size %ld bytes"),
            date, stat.st_size);
#endif
}

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
 * Show question when destination file already exists
 *
 * @param __src - URL of source file
 * @param __dst - URL of destination file
 * @param __use_lstat - use vfs_lstat() instead of vfs_stat()
 * @return modal result of question
 */
static int
file_exists_question (const wchar_t *__src,
                      const wchar_t *__dst,
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
  wnd = widget_create_window (_(L"File exists"), 0, 0, width, 13, WMS_CENTERED);
  w_window_set_fonts (wnd, &FONT (CID_WHITE, CID_RED),
                      &FONT (CID_YELLOW, CID_RED));

  fn_len = width - wcslen (_(L"Target file \"%ls\" already exists!"));
  fit_filename (__dst, fn_len, dummy);
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
  EXIST_QUESTION_BUTTON (6, L"A_ppend", MR_APPEND);

  cur_left = buttons_left;
  EXIST_QUESTION_BUTTON (8, L"A_ll", MR_ALL);
  EXIST_QUESTION_BUTTON (8, L"_Update", MR_UPDATE);
  EXIST_QUESTION_BUTTON (8, L"Non_e", MR_MY_NONE);
  cur_left = buttons_left;
  EXIST_QUESTION_BUTTON (9, L"If _size differs", MR_SIZE_DIFFERS);

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
 * Handler of clicking button 'skip' on process window
 *
 * @param __button - sender button
 * @return non-zero if action has been handled, non-zero otherwise
 */
static int
skip_button_clicked (w_button_t *__button)
{
  process_window_t *wnd;
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
  process_window_t *wnd;
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
      process_window_t *wnd;
      wnd=WIDGET_USER_DATA (__button);
      wnd->abort = TRUE;
    }

  return 0;
}

/**
 * Create file copy process window
 *
 * @param __total_progress - is total progress information avaliable
 * @param __listing - listing to get summary information
 * @return created window
 */
static process_window_t*
create_process_window (BOOL __total_progress,
                       const action_listing_t *__listing)
{
  process_window_t *res;
  w_container_t *cnt;
  w_button_t *btn;
  int buttons_left, height;

  MALLOC_ZERO (res, sizeof (process_window_t))

  if (__total_progress)
    {
      height = 11;
    }
  else
    {
      height = 8;
    }

  res->window = widget_create_window (_(L"Copy"), 0, 0,
                                      59, height, WMS_CENTERED);
  cnt = WIDGET_CONTAINER (res->window);

  res->source = widget_create_text (cnt, L"", 1, 1);
  res->target = widget_create_text (cnt, L"", 1, 2);

  res->file_progress = widget_create_progress (cnt, 100, 10, 4, 53, 0);

  widget_create_text (cnt, _(L"File"), 1, 4);

  /* Create progess bars for displaying total progress */
  if (__total_progress)
    {
      wchar_t text[1024];

      swprintf (text, BUF_LEN (text), _(L"(%lldKb of %lldKb)"),
                res->bytes_copied, __listing->size / 1024);
      widget_create_text (cnt, _(L"Bytes"), 1, 6);
      res->bytes_digit = widget_create_text (cnt, text,
                                             2 + wcslen (_(L"Bytes")), 6);
      res->bytes_progress = widget_create_progress (cnt, __listing->size,
                                                    1, 7, 28,
                                                    WPBS_NOPERCENT);
      res->bytes_total = __listing->size;

      swprintf (text, BUF_LEN (text), _(L"(%lld of %lld)"), res->files_copied,
                __listing->count);
      widget_create_text (cnt, _(L"Count"), 30, 6);
      res->count_digit = widget_create_text (cnt, text,
                                             31 + wcslen (_(L"Count")), 6);
      res->count_progress = widget_create_progress (cnt, __listing->count,
                                                    30, 7, 28,
                                                    WPBS_NOPERCENT);
      res->files_total = __listing->count;
    }

  /* Create buttons */
  buttons_left = (cnt->position.width - widget_shortcut_length (_(L"_Skip")) -
                  widget_shortcut_length (_(L"_Abort")) - 9) / 2;

  btn = widget_create_button (cnt, _(L"_Skip"), buttons_left,
                              cnt->position.height - 2, 0);

  WIDGET_USER_DATA (btn) = res;
  WIDGET_USER_CALLBACK (btn, clicked) = (widget_action)skip_button_clicked;
  WIDGET_USER_CALLBACK (btn, keydown) = (widget_keydown_proc)button_keydown;

  buttons_left += widget_shortcut_length (_(L"_Skip")) + 5;
  btn = widget_create_button (cnt, _(L"_Abort"), buttons_left,
                              cnt->position.height - 2, 0);

  WIDGET_USER_DATA (btn) = res;
  WIDGET_USER_CALLBACK (btn, clicked) = (widget_action)abort_button_clicked;
  WIDGET_USER_CALLBACK (btn, keydown) = (widget_keydown_proc)button_keydown;

  return res;
}

/**
 * Destroy file copy process window
 *
 * @param __window - window to be destroyed
 */
static void
destroy_process_window (process_window_t *__window)
{
  if (!__window)
    {
      return;
    }

  widget_destroy (WIDGET (__window->window));
  free (__window);
}

/**
 * Get caption for field 'To'
 *
 * @param __src_list - list of source items
 * @param __count - count of items to be copied
 * @param __buf - buffer where result will be stored
 * @param __buf_size - size of buffer
 */
static void
get_to_field_caption (const file_panel_item_t **__src_list,
                      unsigned long __count,
                      wchar_t *__buf, size_t __buf_size)
{
  wchar_t *format;

  if (__count == 1)
    {
      /* Single file is copying */
      wchar_t *src;
      wchar_t fit_fn[20];
      file_panel_item_t *item;

      item = (file_panel_item_t*)__src_list[0];
      src = item->file->name;

      if (S_ISDIR (item->file->lstat.st_mode))
        {
          format = _(L"Copy directory \"%ls\" to:");
        }
      else
        {
          format = _(L"Copy file \"%ls\" to:");
        }

      fit_filename (src, BUF_LEN (fit_fn), fit_fn);
      swprintf (__buf, __buf_size, format, fit_fn);
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
          if (S_ISDIR (__src_list[i]->file->lstat.st_mode))
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
          format = _(L"Copy %lu files to:");
        }
      else
        if (only_dirs)
          {
            format = _(L"Copy %lu directories to:");
          }
        else
          {
            format = _(L"Copy %lu files/directories to:");
          }

      swprintf (__buf, __buf_size, format, __count);
    }
}

/**
 * Show copy dialog to confirm destination file name
 * and other additional information
 *
 * @param __src_list - list of source items
 * @param __count - count of items to be copied
 * @param __dst - default destination
 * @return MR_CANCEL if user canceled copying, MR_OK otherwise
 */
static int
show_copy_dialog (const file_panel_item_t **__src_list,
                  unsigned long __count,
                  wchar_t **__dst)
{
  int res, left, dummy;
  w_window_t *wnd;
  w_button_t *btn;
  w_edit_t *to;
  w_container_t *cnt;
  wchar_t msg[1024];

  wnd = widget_create_window (_(L"Copy"), 0, 0, 50, 6, WMS_CENTERED);
  cnt = WIDGET_CONTAINER (wnd);

  /* Create caption for 'To' field */
  get_to_field_caption (__src_list, __count, msg, BUF_LEN (msg));
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
 * Compare sizes of two files
 *
 * @param __src - URL of source file
 * @param __dst - URL of destination file
 * @return zero if sizes are equal, non-zero otherwise
 */
static BOOL
is_size_differs (const wchar_t *__src, const wchar_t *__dst)
{
  vfs_stat_t s1, s2;

  vfs_lstat (__src, &s1);
  vfs_lstat (__dst, &s2);

  return s1.st_size != s2.st_size;
}

/**
 * Compare modification times of two files
 *
 * @param __src - URL of source file
 * @param __dst - URL of destination file
 * @return zero if source is older than destination, non-zero otherwise
 */
static BOOL
is_newer (const wchar_t *__src, const wchar_t *__dst)
{
  vfs_stat_t s1, s2;

  vfs_lstat (__src, &s1);
  vfs_lstat (__dst, &s2);

  if (s1.st_mtim.tv_sec > s2.st_mtim.tv_sec)
    {
      return TRUE;
    }

  if (s1.st_mtim.tv_sec < s2.st_mtim.tv_sec)
    {
      return FALSE;
    }

  return s1.st_mtim.tv_nsec > s2.st_mtim.tv_nsec;
}

/**
 * Is total progress info displaying avaliable
 *
 * @param __src_list - list of items to be copied
 * @param __count - count of items in list
 * @return is total progress info displaying avaliable
 */
static BOOL
total_progress_avaliable (const file_panel_item_t **__src_list,
                          unsigned long __count)
{
  if (!__src_list || !scan)
    {
      return FALSE;
    }

  if (__count > 1)
    {
      return TRUE;
    }

  return S_ISDIR (__src_list[0]->file->lstat.st_mode);
}

/**
 * Copy a regular file
 *
 * @param __src - URL of source
 * @param __dst - URL of destination
 * @param __owr_all_rule - Rule for overwriting existing files
 * @param __proc_wnd - window with different current information
 * @return zero on success, non-zero otherwise
 */
static int
copy_regular_file (const wchar_t *__src, const wchar_t *__dst,
                   int *__owr_all_rule, process_window_t *__proc_wnd)
{
  vfs_file_t fd_src = 0, fd_dst = 0;
  int res, create_flags = O_WRONLY | O_CREAT | O_TRUNC;
  vfs_stat_t stat;
  char buffer[BUF_SIZE];
  vfs_size_t remain, copied, read, written;
  struct utimbuf times;
  __u64_t iteration = 0;

  /* Open descriptor of source file */
  OPEN_FD (fd_src, __src, O_RDONLY, res,
           _(L"Cannot open source file \"%ls\":\n%ls"),
           __src, vfs_get_error (res));

  /* Check is file already exists */
  if (!vfs_stat (__dst, &stat))
    {
      res = GET_OWR_RULE (FALSE);

      switch (res)
        {
        case MR_NO:
          return ACTION_OK;
          break;
        case MR_APPEND:
          create_flags = O_WRONLY | O_APPEND;
          break;

        case MR_ALL:
          SAVE_OWR_ALL_RULE (MR_ALL);
          break;
        case MR_UPDATE:
          SAVE_OWR_ALL_RULE (MR_UPDATE);
          if (!is_newer (__src, __dst))
            {
              return ACTION_OK;
            }
          break;
        case MR_MY_NONE:
          SAVE_OWR_ALL_RULE (MR_MY_NONE);
          return ACTION_OK;
          break;
        case MR_SIZE_DIFFERS:
          SAVE_OWR_ALL_RULE (MR_SIZE_DIFFERS);
          if (!is_size_differs (__src, __dst))
            return ACTION_OK;
          break;

        case MR_CANCEL:
        case MR_ABORT:
          return ACTION_ABORT;
          break;
        }
    }

  /* Stat of source file */
  COPY_FILE_REP (res = vfs_stat (__src, &stat), error,
                 _(L"Cannot stat source file \"%ls\":\n%ls"),
                 __src, vfs_get_error (res));

  remain = stat.st_size;

  /* Save access and modification time */
  times.actime = stat.st_atim.tv_sec;
  times.modtime = stat.st_mtim.tv_sec;

  /* Create destination file */
  OPEN_FD (fd_dst, __dst, create_flags, res,
           _(L"Cannot create target file \"%ls\":\n%ls"),
           __dst, vfs_get_error (res));

  /* Set mode of destination file */
  COPY_FILE_REP (res = vfs_chmod (__dst, stat.st_mode), error2,
                 _(L"Cannot chmod target file \"%ls\":\n%ls"),
                 __dst, vfs_get_error (res));

  copied = 0;
  w_progress_set_max (__proc_wnd->file_progress, remain);

  /* Copy content of file */
  while (remain > 0)
    {
      /* Read buffer from source file */
      COPY_FILE_REP (read = vfs_read (fd_src, buffer, MIN (remain, BUF_SIZE));
                     res = read < 0 ? read : 0;,
                     error,
                     _(L"Cannot read source file \"%ls\":\n%ls"),
                     __src, vfs_get_error (res));

     /*
      * NOTE: Reading of buffer and it's writting may be long.
      *       So, need to process after both of this operations.
      */

      /* Process accamulated queue of characters */
      COPY_PROCESS_QUEUE ();

      /* Write buffer to destination file */
      COPY_FILE_REP (written = vfs_write (fd_dst, buffer, read);
                     res = written < 0 ? written : 0;,
                     error,
                     _(L"Cannot write target file \"%ls\":\n%ls"),
                     __dst, vfs_get_error (res));

      copied += written;
      BUFFER_COPIED (written);

      /* Process accamulated queue of characters */
      COPY_PROCESS_QUEUE ();

      remain -= read;
      ++iteration;
    }

  /* Set access and modification time of new file */
  vfs_utime (__dst, &times);

  /* Close descriptors  */
  CLOSE_FD ();

  /* Operation has been broken */
  if (PROCESS_ABORTED ())
    {
      if (remain != 0)
        {
          /* If file hasn't been fully copied */
          if (INCOMPLETE_MESSAGE () == MR_NO)
            {
              vfs_unlink (__dst);
            }

          /* Update limit of max position in bytes progress */
          if (__proc_wnd->bytes_progress)
            {
               w_progress_set_max (__proc_wnd->bytes_progress,
                     w_progress_get_max (__proc_wnd->bytes_progress) - remain);
            }
        }

      /* Reset skip flag */
      __proc_wnd->skip = FALSE;

      if (__proc_wnd->abort)
        {
          return ACTION_ABORT;
        }
    }

  return ACTION_OK;
}

/**
 * Copy a symbolic link
 *
 * @param __src - URL of source
 * @param __dst - URL of destination
 * @param __owr_all_rule - Rule for overwriting existing files
 * @param __proc_wnd - window with different current information
 * @return zero on success, non-zero otherwise
 */
static int
copy_symlink (const wchar_t *__src, const wchar_t *__dst,
              int *__owr_all_rule, process_window_t *__proc_wnd)
{
  vfs_stat_t stat;
  wchar_t content[MAX_SYMLINK_CONTENT];
  int res;

  /* Read content of source symbolic link */  
  vfs_readlink (__src, content, BUF_LEN (content));

  for (;;)
    {
      /* Check for file existment */
      if (!(res = vfs_lstat (__dst, &stat)))
        {
          /* File has been successfully stat'ed */
          if (S_ISLNK (stat.st_mode))
            {
              /* If found file is a symbolic link, */
              /* read it and compare it's content with content */
              /* of source symbolic link */

              wchar_t e_content[MAX_SYMLINK_CONTENT];
              vfs_readlink (__dst, e_content, BUF_LEN (e_content));

              /* Compare contents */
              if (!wcscmp (content, e_content))
                {
                  /* Contents are equal, so we can think, that */
                  /* symbolic link has been copied successfully */
                  return ACTION_OK;
                }
            }

          /* Symlinks are different or target is not a symlink */
          res = GET_OWR_RULE (TRUE);

          /* Review user's answer */
          switch (res)
            {
            case MR_YES:
            case MR_APPEND:
              /*
               * NOTE: Appending of symlinks is equal
               *       to it's replacement.
               */
              SYMLINK_UNLINK ();
              break;

            case MR_NO:
              return ACTION_OK;

            case MR_ALL:
              SAVE_OWR_ALL_RULE (MR_ALL);
              SYMLINK_UNLINK ();
              break;

            case MR_UPDATE:
              SAVE_OWR_ALL_RULE (MR_UPDATE);
              if (!is_newer (__src, __dst))
                {
                  return ACTION_OK;
                }
              SYMLINK_UNLINK ();
              break;

            case MR_MY_NONE:
              SAVE_OWR_ALL_RULE (MR_MY_NONE);
              return ACTION_OK;

            case MR_SIZE_DIFFERS:
              SAVE_OWR_ALL_RULE (MR_SIZE_DIFFERS);
              if (!is_size_differs (__src, __dst))
                {
                  return ACTION_OK;
                }
              SYMLINK_UNLINK ();
              break;

            case MR_ABORT:
              return ACTION_ABORT;
            }
        }
      else
        {
          if (res != -ENOENT)
            {
              /* Error STAT'ing*/
              res = error (_(L"Cannot stat target file \"%ls\":\n%ls"), __dst,
                           vfs_get_error (res));

              /* Review user's answer */
              switch (res)
                {
                case MR_RETRY:
                  continue;

                case MR_CANCEL:
                  return ACTION_ABORT;

                case MR_SKIP:
                  return ACTION_OK;
                }
            }
        }

      /* Create symlink only if 'File not found' error returned */
      if (!vfs_symlink (content, __dst))
        {
          return ACTION_OK;
        }

      /* Process accamulated queue of characters */
      COPY_PROCESS_QUEUE ();
    }

  return ACTION_OK;
}

/**
 * Copy a single file
 *
 * @param __src - URL of source
 * @param __dst - URL of destination
 * @param __owr_all_rule - Rule for overwriting existing files
 * @param __proc_wnd - window with different current information
 * @return zero on success, non-zero otherwise
 */
static int
copy_file (const wchar_t *__src, const wchar_t *__dst,
           int *__owr_all_rule, process_window_t *__proc_wnd)
{
  int res;
  size_t prefix_len;
  vfs_stat_t stat;
  wchar_t msg[1024], fn[1024];

  /* Initialize current info on screen */
  w_progress_set_pos (__proc_wnd->file_progress, 0);

  /* +1 becase I want to skip directory delimeter too */
  prefix_len = wcslen (__proc_wnd->abs_path_prefix) + 1;

  COPY_SET_FN (__src + prefix_len, source, L"Source");
  COPY_SET_FN (__dst, target, L"Target");

  /* Check is file copying to itself */
  if (!filename_compare (__src, __dst))
    {
      wchar_t buf[4096];
      swprintf (buf, BUF_LEN (buf),
                _(L"Cannot copy \"%ls\" to itself"), __src);

      MESSAGE_ERROR (buf);
      return ACTION_ERR;
    }

  /* Stat source file to determine it's type */
  REP (res = vfs_lstat (__src, &stat), error,
       if (res == MR_CANCEL) res = MR_ABORT; return res,
       _(L"Cannot stat source file \"%ls\":\n%ls"),
       __src, vfs_get_error (res));

  if (S_ISREG (stat.st_mode))
    {
      res = copy_regular_file (__src, __dst, __owr_all_rule, __proc_wnd);
      FILE_COPIED ();
    }
  else
    if (S_ISLNK (stat.st_mode))
    {
      res = copy_symlink (__src, __dst, __owr_all_rule, __proc_wnd);
      FILE_COPIED ();
    }
  else
    {
      /*
       * TODO: Need to implement coping non-regular files
       */

      swprintf (msg, BUF_LEN (msg),
                _(L"Cannot copy special file \"%ls\":\n%ls"),
                __src, _(L"This feature is not implemented yet"));
      MESSAGE_ERROR (msg);

      res = ACTION_OK;
    }

  /* Process accamulated queue of characters */
  widget_process_queue ();
  if (__proc_wnd->abort)
    {
      return ACTION_ABORT;
    }

  return res;
}

/**
 * Recursively copy directory
 *
 * @param __src - URL of source
 * @param __dst - URL of destination
 * @param __owr_all_rule - Rule for overwriting existing files
 * @param __proc_wnd - window with different current information
 * @param __tree - prescanned tree
 * @return zero on success, non-zero otherwise
 */
static int
copy_dir (const wchar_t *__src, const wchar_t *__dst,
          int *__owr_all_rule, process_window_t *__proc_wnd,
          const action_listing_tree_t *__tree)
{
  vfs_dirent_t **eps = NULL;
  vfs_stat_t stat;
  int count, i, res = ACTION_OK;
  wchar_t *full_name, *full_dst;
  size_t fn_len, dst_len;
  BOOL prescanned = FALSE;

  /* Stat source directory */
  COPY_DIR_REP (res = vfs_stat (__src, &stat);, error,
                _(L"Cannot stat source directory \"%ls\":\n%ls"),
                __src, vfs_get_error (res));

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
      COPY_DIR_REP (count = vfs_scandir (__src, &eps, 0, vfs_alphasort);
                    res = count < 0 ? count : 0,
                    error,
                    _(L"Cannot listing source directory \"%ls\":\n%ls"),
                    __src, vfs_get_error (res));
    }

  /*
   * NOTE: Destination directory must be created AFTER listing of source one.
   *       It depends on posibility that destination directory will
   *       be created inside source.
   */

  /* Create destination directory */
  COPY_DIR_REP (res = vfs_mkdir (__dst, 0); if (res == -EEXIST) res = 0;, error,
                _(L"Cannot create target directory \"%ls\":\n%ls"),
                __dst, vfs_get_error (res));

  /* Set mode of destination directory */
  COPY_DIR_REP (res = vfs_chmod (__dst, stat.st_mode), error,
                _(L"Cannot chmod target directory \"%ls\":\n%ls"),
                __dst, vfs_get_error (res));

  /* Allocate memories for new strings */
  ALLOC_FN (full_name, fn_len, __src);
  ALLOC_FN (full_dst, dst_len, __dst);

  /* Get contents of source directory */
  for (i = 0; i < count; i++)
    {
      if (!IS_PSEUDODIR (eps[i]->name))
        {
          /* Get full filename of current file or directory and */
          /* it's destination */
          swprintf (full_name, fn_len, L"%ls/%ls", __src, eps[i]->name);
          swprintf (full_dst, dst_len, L"%ls/%ls", __dst, eps[i]->name);

          if (!isdir (full_name))
            {
              res = copy_file (full_name, full_dst, __owr_all_rule, __proc_wnd);
            }
          else
            {
              res = copy_dir (full_name, full_dst, __owr_all_rule, __proc_wnd,
                              prescanned ? __tree->items[i] : NULL);
            }

          /* Copying has been aborted */
          if (res == ACTION_ABORT)
            {
              /* Free allocated memory */
              FREE_REMAIN_DIRENT ();
              break;
            }
        }

      /* Process accamulated queue of characters */
      if (PROCESS_ABORTED ())
        {
          /* Free allocated memory */
          FREE_REMAIN_DIRENT ();

          if (__proc_wnd->abort)
            {
              res = ACTION_ABORT;
            }

          break;
        }

      if (!prescanned)
        {
          vfs_free_dirent (eps[i]);
        }
    }

  /* Free used variables */
  if (!prescanned)
    {
      SAFE_FREE (eps);
    }

  free (full_name);
  free (full_dst);

  return res;
}

/**
 * Iterator for make_copy()
 *
 * @param __src - full source URL
 * @param __dst - full destination URL
 * @param __owr_all_rule - Rule for overwriting existing files
 * @param __proc_wnd - window with different current information
 * @param __tree - prescanned tree of items. This tree will be
 * send to copy_dir().
 * @return zero on success, non-zero otherwise
 */
static int
make_copy_iter (const wchar_t *__src, const wchar_t *__dst,
                int *__owr_all_rule, process_window_t *__proc_wnd,
                const action_listing_tree_t *__tree)
{
  int res = ACTION_ERR;
  wchar_t *rdst = NULL; /* Real destination */

  if (isdir (__src))
    {
      vfs_stat_t stat;

      /* Get full destination directory name */
      if (vfs_stat (__dst, &stat))
        {
          rdst = wcsdup (__dst);
        }
      else
        {
          if (isdir (__dst))
            {
              rdst = get_real_dst (__src, __dst);
            }
          else
            {
              /*
               * TODO: Add error handling here
               */
              return ACTION_ERR;
            }
        }

      /* Copy directory */
      res = copy_dir (__src, rdst, __owr_all_rule, __proc_wnd, __tree);
    }
  else
    {
      /* Get full destination filename */
      if (isdir (__dst))
        {
          rdst = get_real_dst (__src, __dst);
        }
      else
        {
          rdst = wcsdup (__dst);
        }

      /* Copy single file */
      res = copy_file (__src, rdst, __owr_all_rule, __proc_wnd);
    }

  SAFE_FREE (rdst);

  return res;
}

/**
 * Copy file or directory
 *
 * @param __base_dir - base directpry
 * @param __src_list - list of source items
 * @param __count - count of items to be copied
 * @param __dst - URL of destination
 * @return count of copied items
 */
static unsigned long
make_copy (const wchar_t *__base_dir, const file_panel_item_t **__src_list,
           unsigned long __count, const wchar_t *__dst)
{
  process_window_t *wnd;
  int res, owr_all_rule = 0;
  wchar_t *dst = (wchar_t*) __dst, *src;
  unsigned long i, count = 0;
  file_panel_item_t *item;
  BOOL scan_allowed;
  action_listing_t listing;

  if (!__base_dir || !*__src_list || !dst)
    {
      return 0;
    }

  memset (&listing, 0, sizeof (listing));

  /* Get customized settings from user */
  res = show_copy_dialog (__src_list, __count, &dst);

  /* User canceled copying */
  if (res == MR_CANCEL)
    {
      SAFE_FREE (dst);
      return 0;
    }

  scan_allowed = total_progress_avaliable (__src_list, __count);

  /* Get listing of items */
  if (scan_allowed)
    {
      if ((res = action_get_listing (__base_dir, __src_list,
                                   __count, &listing)) != ACTION_OK)
        {
          /* There was an error listing items */
          if (res == ACTION_ABORT)
            {
              /* User just aborted scanning */
              scan_allowed = FALSE;
            }
          else
            {
              return res;
            }
        }
    }

  wnd = create_process_window (scan_allowed, &listing);

  wnd->abs_path_prefix = (wchar_t*)__base_dir;
  w_window_show (wnd->window);

  for (i = 0; i < __count; ++i)
    {
      item = (file_panel_item_t*)__src_list[i];

      /* Make copy iteration */
      src = wcdircatsubdir (__base_dir, item->file->name);
      res = make_copy_iter (src, __dst, &owr_all_rule, wnd,
                            scan_allowed ? listing.tree->items[i] : NULL);
      free (src);

      if (res == 0)
        {
          /* In case of successfull copying */
          /* we need free selection from copied item */
          item->selected = FALSE;
          count++;
        }
      else
        {
          if (res == MR_ABORT)
            {
              break;
            }
        }
    }

  destroy_process_window (wnd);

  /* Free listing information */
  if (scan_allowed)
    {
      action_free_listing (&listing);
    }

  free (dst);

  return count;
}

/********
 * User's backend
 */

/**
 * Copy list of files from specified panel
 *
 * @param __panel - from which panel files will be copied
 * @return zero on success, non-zero otherwise
 */
int
action_copy (file_panel_t *__panel)
{
  file_panel_t *opposite_panel;
  wchar_t *dst, *cwd;
  file_panel_item_t **list = NULL;
  unsigned long count;

  if (!__panel)
    {
      return ACTION_ERR;
    }

  /* Check file panels count */
  if (file_panel_get_count () <= 1)
    {
      MESSAGE_ERROR (_(L"File copying may be start at least "
                        "with two file panels"));
      return ACTION_ERR;
    }

  /* Get second panel to start copying */
  opposite_panel = action_choose_file_panel ();
  if (!opposite_panel)
    {
      /* User canceled operation */
      return ACTION_ABORT;
    }

  /* Full source and destination URLs */
  cwd = file_panel_get_full_cwd (__panel);
  dst = file_panel_get_full_cwd (opposite_panel);

  /* Get list of items to be copied */
  count = file_panel_get_selected_items (__panel, &list);

  /*
   * NOTE: I hope that file panel cannot give access to select
   *       pseudo-directories. So, only item under cursor may be
   *       a pseudo-directory.
   *       Lets check it.
   */
  if (count == 1 && IS_PSEUDODIR (list[0]->file->name))

    {
      wchar_t msg[1024];
      swprintf (msg, BUF_LEN (msg), _(L"Cannot operate on \"%ls\""),
                list[0]->file->name);
      MESSAGE_ERROR (msg);
      return ACTION_ERR;
    }

  /* Copy files */
  count = make_copy (cwd, (const file_panel_item_t**)list, count, dst);

  /* If there selection_count==0 it means that */
  /* item under cursor has been copied and we */
  /* we shouldn't change selected_count */
  if (__panel->items.selected_count)
    {
      __panel->items.selected_count -= count;
    }

  SAFE_FREE (list);
  free (dst);
  free (cwd);

  /* There may be selection in source panel and it may be changed */
  /* so, we need to redraw it */
  file_panel_redraw (__panel);

  /* There is new items on opposite panel */
  /* so, we need to rescan it */
  file_panel_rescan (opposite_panel);

  return ACTION_OK;
}
