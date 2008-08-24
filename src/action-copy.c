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
#include "action-copy-iface.h"
#include "messages.h"
#include "i18n.h"
#include "dir.h"
#include "util.h"
#include "timer.h"

#include <fcntl.h>
#include <errno.h>
#include <wchar.h>

/********
 * Constants and other definitions
 */

/* Size of buffer in copying operation */
#define BUF_SIZE 65536

/* Maximal size of content of symbolic link */
#define MAX_SYMLINK_CONTENT 4096

/* Period to evalute speed and ETA */
#define EVAL_SPEED_PERIOD 1.05 * 1000 * 1000

/**
 * Close file descriptors in copy_file()
 */
#define CLOSE_FD() \
  { \
    vfs_close (fd_src); \
    vfs_close (fd_dst); \
  }

/* Message about file was copied incompletely */
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
    _code=ACTION_ABORT; \
  return _code;

/**
 * Set current file name on process window from copy_file()
 */
#define COPY_SET_FN(_fn, _dst, _text) \
  { \
    fit_dirname (_fn, \
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
  REP (_act, _error_proc, return res==MR_CANCEL?ACTION_ABORT:res, \
       _error, ##_error_args)

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

/* Get file overwrite rule */
#define GET_OWR_RULE(_use_lstat) \
  ((__owr_all_rule && *__owr_all_rule)? \
    (*__owr_all_rule): \
    (action_copy_exists_dialog (__src, __dst, _use_lstat)))

/**
 * Save answer about overwriting all existent targets
 */
#define SAVE_OWR_ALL_RULE(_a) \
  if (__owr_all_rule) (*__owr_all_rule)=_a;

/*
 * Unlink symlink in copy_symlink()
 */
#define UNLINK_TARGET() \
  REP (res=vfs_unlink (__dst), error, \
  return (res==MR_CANCEL?ACTION_ABORT:res), \
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

/*
 * Free all tail dirents. Helper for copy_directory()
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

/* Update position of progress bar which shows total progress */
#define UPDATE_TOTAL_PROGRESS(_progress, _value) \
  { \
    if (__proc_wnd->_progress) \
      { \
        w_progress_set_pos (__proc_wnd->_progress, \
                            w_progress_get_pos (__proc_wnd->_progress) + \
                                    _value); \
      } \
  }

/* Set caption of text widget, which shows a digital summary information */
#define SET_DIGIT_CAPTION(_caption, _format, _args...) \
  { \
    wchar_t text[1024]; \
    swprintf (text, BUF_LEN (text), _format, _args); \
    w_text_set (__proc_wnd->_caption, text); \
  }

/* Evalute speed and ETA */
#define EVAL_SPEED() \
  { \
    CALL_DELAYED (__proc_wnd->speed_timestamp, EVAL_SPEED_PERIOD, \
                  action_copy_eval_speed, __proc_wnd); \
  }

/* Next buffer of file was copied */

/*
 * NOTE: The magick value in this macros is needed to
 *       make load of CPU not so enormous
 */

#define BUFFER_COPIED(_size) \
  { \
    static wchar_t *format = NULL; \
    if (format == NULL) \
      { \
        format = _(L"(%lld%c of %lld%c)"); \
      } \
    /* Set progress for current copying file */ \
    w_progress_set_pos (__proc_wnd->file_progress, copied); \
    __proc_wnd->bytes_copied += _size; \
    if (iteration % 8 /* Magick constant */) \
      { \
        __u64_t copied, total; \
        char cs, ts; \
        /* Update progress of total copied bytes */ \
        if (__proc_wnd->bytes_progress) \
          { \
            w_progress_set_pos (__proc_wnd->bytes_progress, \
                            __proc_wnd->bytes_copied); \
          } \
        /* Update cpations of digital information */ \
        copied = fsizetohuman (__proc_wnd->bytes_copied, &cs); \
        total = fsizetohuman (__proc_wnd->bytes_total, &ts); \
        SET_DIGIT_CAPTION (bytes_digit, format, copied, cs, total, ts); \
      } \
    /* Evalute speed and ETA */ \
    EVAL_SPEED (); \
  }

/* The while file was copied */
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
    /* Evalute speed and ETA */ \
    EVAL_SPEED (); \
  }

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

  if (s1.st_mtime > s2.st_mtime)
    {
      return TRUE;
    }

  if (s1.st_mtime < s2.st_mtime)
    {
      return FALSE;
    }

  return s1.st_mtime > s2.st_mtime;
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
                   int *__owr_all_rule, copy_process_window_t *__proc_wnd)
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
        case MR_COPY_APPEND:
          create_flags = O_WRONLY | O_APPEND;
          break;

        case MR_COPY_REPLACE_ALL:
          SAVE_OWR_ALL_RULE (MR_COPY_REPLACE_ALL);
          break;
        case MR_COPY_UPDATE:
          SAVE_OWR_ALL_RULE (MR_COPY_UPDATE);
          if (!is_newer (__src, __dst))
            {
              return ACTION_OK;
            }
          break;
        case MR_COPY_NONE:
          SAVE_OWR_ALL_RULE (MR_COPY_NONE);
          return ACTION_OK;
          break;
        case MR_COPY_SIZE_DIFFERS:
          SAVE_OWR_ALL_RULE (MR_COPY_SIZE_DIFFERS);
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
  times.actime = stat.st_atime;
  times.modtime = stat.st_mtime;

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

               /*
                * TODO: Or bytes_total may be without bytes_progress?
                */
               __proc_wnd->bytes_total -= remain;
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
              int *__owr_all_rule, copy_process_window_t *__proc_wnd)
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
            case MR_COPY_APPEND:
              /*
               * NOTE: Appending of symlinks is equal
               *       to it's replacement.
               */
              UNLINK_TARGET ();
              break;

            case MR_NO:
              return ACTION_OK;

            case MR_COPY_REPLACE_ALL:
              SAVE_OWR_ALL_RULE (MR_COPY_REPLACE_ALL);
              UNLINK_TARGET ();
              break;

            case MR_COPY_UPDATE:
              SAVE_OWR_ALL_RULE (MR_COPY_UPDATE);
              if (!is_newer (__src, __dst))
                {
                  return ACTION_OK;
                }
              UNLINK_TARGET ();
              break;

            case MR_COPY_NONE:
              SAVE_OWR_ALL_RULE (MR_COPY_NONE);
              return ACTION_OK;

            case MR_COPY_SIZE_DIFFERS:
              SAVE_OWR_ALL_RULE (MR_COPY_SIZE_DIFFERS);
              if (!is_size_differs (__src, __dst))
                {
                  return ACTION_OK;
                }
              UNLINK_TARGET ();
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
                  /* Process accamulated queue of characters */
                  COPY_PROCESS_QUEUE ();
                  continue;

                case MR_CANCEL:
                  return ACTION_ABORT;

                case MR_SKIP:
                  return ACTION_OK;
                }
            }
        }

      /* Process accamulated queue of characters */
      COPY_PROCESS_QUEUE ();

      break;
    }

  /* Create symlink */
  REP (res = vfs_symlink (content, __dst), error, return (res),
     _(L"Cannot create symbolic link \"%ls\":\n%ls"), __src,
       vfs_get_error (res));

  return ACTION_OK;
}

/**
 * Copy a special file
 *
 * @param __src - URL of source
 * @param __dst - URL of destination
 * @param __owr_all_rule - Rule for overwriting existing files
 * @param __proc_wnd - window with different current information
 * @return zero on success, non-zero otherwise
 */
static int
copy_special_file (const wchar_t *__src, const wchar_t *__dst,
                   int *__owr_all_rule, copy_process_window_t *__proc_wnd)
{
  vfs_stat_t stat, dst_stat;
  int res;

  /* Stat source file */
  REP (res = vfs_stat (__src, &stat), error, return (res),
       _(L"Cannot stat source file \"%ls\":\n%ls"), __src,
       vfs_get_error (res));

  if (S_ISCHR (stat.st_mode) || S_ISBLK (stat.st_mode) ||
      S_ISFIFO (stat.st_mode) || S_ISSOCK (stat.st_mode))
    {
      for (;;)
        {
          /* Check if file already exists */
          if ((res = vfs_stat (__dst, &dst_stat)) == VFS_OK)
            {
              res = GET_OWR_RULE (FALSE);

              /* Review user's answer */
              switch (res)
                {
                case MR_YES:
                case MR_COPY_APPEND:
                  /*
                   * NOTE: Appending of special files is equal
                   *       to it's replacement.
                   */
                  UNLINK_TARGET ();
                  break;

                case MR_NO:
                  return ACTION_OK;

                case MR_COPY_REPLACE_ALL:
                  SAVE_OWR_ALL_RULE (MR_COPY_REPLACE_ALL);
                  UNLINK_TARGET ();
                  break;

                case MR_COPY_UPDATE:
                  SAVE_OWR_ALL_RULE (MR_COPY_UPDATE);
                  if (!is_newer (__src, __dst))
                    {
                      return ACTION_OK;
                    }
                  UNLINK_TARGET ();
                  break;

                case MR_COPY_NONE:
                  SAVE_OWR_ALL_RULE (MR_COPY_NONE);
                  return ACTION_OK;

                case MR_COPY_SIZE_DIFFERS:
                  SAVE_OWR_ALL_RULE (MR_COPY_SIZE_DIFFERS);
                  /*
                   * TODO: But does it work properly?
                   */
                  if (!is_size_differs (__src, __dst))
                    {
                      return ACTION_OK;
                    }
                  UNLINK_TARGET ();
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
                  res = error (_(L"Cannot stat target file \"%ls\":\n%ls"),
                               __dst, vfs_get_error (res));

                  /* Review user's answer */
                  switch (res)
                    {
                    case MR_RETRY:
                      /* Process accamulated queue of characters */
                      COPY_PROCESS_QUEUE ();
                      continue;

                    case MR_CANCEL:
                      return ACTION_ABORT;

                    case MR_SKIP:
                      return ACTION_OK;
                    }
                }
            }

          /* Process accamulated queue of characters */
          COPY_PROCESS_QUEUE ();

          break;
        }

      /* Create special file */
      REP (res = vfs_mknod (__dst, stat.st_mode, stat.st_rdev), error,
           return res == MR_CANCEL ? ACTION_ABORT : res,
           _(L"Cannot create special file \"%ls\":\n%ls"), __src,
           vfs_get_error (res));
    }
  else
    {
      wchar_t msg[1024];
      swprintf (msg, BUF_LEN (msg),
                _(L"Cannot copy special file \"%ls\":\n%ls"),
                __src, _(L"This feature is not implemented yet"));
      MESSAGE_ERROR (msg);

      return ACTION_OK;
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
           int *__owr_all_rule, copy_process_window_t *__proc_wnd)
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
       if (res == MR_CANCEL) res = ACTION_ABORT; return res,
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
      res = copy_special_file (__src, __dst, __owr_all_rule, __proc_wnd);
      FILE_COPIED ();
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
          int *__owr_all_rule, copy_process_window_t *__proc_wnd,
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
                int *__owr_all_rule, copy_process_window_t *__proc_wnd,
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
  copy_process_window_t *wnd;
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
  res = action_copy_show_dialog (__src_list, __count, &dst);

  /*
   * TODO: Should we normalize destination?
   */

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

  wnd = action_copy_create_proc_wnd (scan_allowed, &listing);

  wnd->abs_path_prefix = (wchar_t*)__base_dir;
  w_window_show (wnd->window);

  for (i = 0; i < __count; ++i)
    {
      item = (file_panel_item_t*)__src_list[i];

      /* Make copy iteration */
      src = wcdircatsubdir (__base_dir, item->file->name);
      res = make_copy_iter (src, dst, &owr_all_rule, wnd,
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
          if (res == ACTION_ABORT)
            {
              break;
            }
        }
    }

  action_destroy_proc_wnd (wnd);

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
      SAFE_FREE (list);
      return ACTION_ERR;
    }

  /* Get second panel to start copying */
  opposite_panel = action_choose_file_panel (_(L"Copy"), _(L"Target panel"));
  if (!opposite_panel)
    {
      /* User canceled operation */
      SAFE_FREE (list);
      return ACTION_ABORT;
    }

  /* Full source and destination URLs */
  cwd = file_panel_get_full_cwd (__panel);
  dst = file_panel_get_full_cwd (opposite_panel);

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
