/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Common part of copy and move operations
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "actions.h"
#include "action-copymove-iface.h"
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
      int t = INCOMPLETE_MESSAGE (); \
      if (t == MR_NO || t == MR_CANCEL) \
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

#define COPY_FILE_REP(_act, _error_proc, _error, _error_args...) \
  ACTION_REPEAT (_act, _error_proc, COPY_RETERR (__dlg_res_), \
  _error, ##_error_args)

#define COPY_DIR_REP(_act, _error_proc, _error, _error_args...) \
  ACTION_REPEAT (_act, _error_proc, \
       return ACTION_CANCEL_TO_ABORT (__dlg_res_),     \
       _error, ##_error_args)

/**
 * Open file descriptor
 * If some errors occurred, asks user what to do
 */
#define OPEN_FD(_fd, _url, _flags, _res, _error, _error_args...) \
  COPY_FILE_REP (_fd=vfs_open (_url, _flags, &_res, 0), \
  action_error_retryskipcancel, _error, ##_error_args)

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
 * Unlink target file
 */
#define UNLINK_TARGET() \
  ACTION_REPEAT (res = vfs_unlink (__dst), action_error_retryskipcancel, \
  return ACTION_CANCEL_TO_ABORT (__dlg_res_), \
   _(L"Cannot unlink target file \"%ls\":\n%ls"), __dst, \
  vfs_get_error (res))

/*
 * Unlink source file
 */
#define UNLINK_SOURCE() \
  ACTION_REPEAT (res=vfs_unlink (__src), action_error_retryskipcancel, \
  return ACTION_CANCEL_TO_ABORT (__dlg_res_), \
  _(L"Cannot unlink source file \"%ls\":\n%ls"), __src, \
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

#define SET_TOTAL_BYTES_CAPTION() \
  { \
    if (__proc_wnd->bytes_digit) \
      { \
        static wchar_t *format = NULL; \
        if (format == NULL) \
          { \
            format = _(L"(%lld%c of %lld%c)"); \
          } \
        __u64_t copied, total; \
        char cs, ts; \
        copied = fsizetohuman (__proc_wnd->bytes_copied, &cs); \
        total = fsizetohuman (__proc_wnd->bytes_total, &ts); \
        cs = !cs ? 'b' : cs; \
        ts = !ts ? 'b' : ts; \
        SET_DIGIT_CAPTION (bytes_digit, format, copied, cs, total, ts); \
    } \
  }

/* Next buffer of file was copied */

/*
 * NOTE: The magick value in this macros is needed to
 *       make load of CPU not so enormous
 */

#define BUFFER_COPIED(_size) \
  { \
    /* Set progress for current copying file */ \
    w_progress_set_pos (__proc_wnd->file_progress, copied); \
    __proc_wnd->bytes_copied += _size; \
    if (iteration % 8 /* Magick constant */) \
      { \
        /* Update progress of total copied bytes */ \
        if (__proc_wnd->bytes_progress) \
          { \
            w_progress_set_pos (__proc_wnd->bytes_progress, \
                            __proc_wnd->bytes_copied); \
          } \
        /* Update cpations of digital information */ \
        SET_TOTAL_BYTES_CAPTION (); \
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

/* Update limit of max position in bytes progress */
#define REDUCE_TOTAL_BYTES(_size) \
  { \
    if (__proc_wnd->bytes_progress) \
      { \
         w_progress_set_max (__proc_wnd->bytes_progress, \
               w_progress_get_max (__proc_wnd->bytes_progress) - _size); \
 \
         /* \
          * TODO: Or bytes_total may be without bytes_progress? \
          */ \
         __proc_wnd->bytes_total -= _size; \
 \
          /* Update information at text widget */ \
          SET_TOTAL_BYTES_CAPTION (); \
      } \
  }

#define CAN_USE_RENAME() \
  (__proc_wnd->move && __proc_wnd->move_strategy == VFS_MS_RENAME)

 /* Check is file copying to itself */
 #define CHECK_THE_SAME() \
   { \
      if (!filename_compare (__src, __dst)) \
        { \
          wchar_t buf[4096]; \
          swprintf (buf, BUF_LEN (buf), \
                    _(L"Cannot copy \"%ls\" to itself"), __src); \
          MESSAGE_ERROR (buf); \
          return ACTION_ERR; \
        } \
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
 * Makes a vfs_rename() opertaion
 *
 * @param __src - source path
 * @param __dst - destination path
 * @param __proc_wnd - window with different current information
 * @return zero on success, non-zero otherwise
 */
static int
make_rename (const wchar_t *__src, const wchar_t *__dst,
             copy_process_window_t *__proc_wnd)
{
  int res;

  ACTION_REPEAT (res = vfs_rename (__src, __dst),
                 action_error_retryskipcancel_ign,
                 return ACTION_CANCEL_TO_ABORT (__dlg_res_),
                 _(L"Cannot move \"%ls\":\n%ls"), __src, vfs_get_error (res));

  /* Process accamulated queue of characters */
  widget_process_queue ();;

  if (res)
    {
      return ACTION_IGNORE;
    }

  return ACTION_OK;
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
  BOOL append = FALSE, target_exists = FALSE;

  /* Check is file already exists */
  if (!vfs_stat (__dst, &stat))
    {
      res = GET_OWR_RULE (FALSE);
      target_exists = TRUE;

      switch (res)
        {
        case MR_NO:
          return ACTION_SKIP;
          break;
        case MR_COPY_APPEND:
          append = TRUE;
          create_flags = O_WRONLY | O_APPEND;
          break;

        case MR_COPY_REPLACE_ALL:
          SAVE_OWR_ALL_RULE (MR_COPY_REPLACE_ALL);
          break;
        case MR_COPY_UPDATE:
          SAVE_OWR_ALL_RULE (MR_COPY_UPDATE);
          if (!is_newer (__src, __dst))
            {
              return ACTION_SKIP;
            }
          break;
        case MR_COPY_NONE:
          SAVE_OWR_ALL_RULE (MR_COPY_NONE);
          return ACTION_SKIP;
          break;
        case MR_COPY_SIZE_DIFFERS:
          SAVE_OWR_ALL_RULE (MR_COPY_SIZE_DIFFERS);
          if (!is_size_differs (__src, __dst))
            {
              return ACTION_SKIP;
            }
          break;

        case MR_CANCEL:
        case MR_ABORT:
          return ACTION_ABORT;
          break;
        }
    }

  /* Stat source file */
  COPY_FILE_REP (res = vfs_stat (__src, &stat), action_error_retryskipcancel,
                 _(L"Cannot stat source file \"%ls\":\n%ls"),
                 __src, vfs_get_error (res));

  if (CAN_USE_RENAME ())
    {
      /* We should use vfs_rename() instead of copy+unlink */

      if (!append)
        {
          /* Can't use vfs_rename to append file as a tail to existing file */

          /* Unlink target to avoid possible troubles */
          if (target_exists)
            {
              UNLINK_TARGET ();
            }

          res = make_rename (__src, __dst, __proc_wnd);

          /* Update limit of max position in bytes progress */
          REDUCE_TOTAL_BYTES (stat.st_size);

          return res;
        }
    }

  /* Open descriptor of source file */
  OPEN_FD (fd_src, __src, O_RDONLY, res,
           _(L"Cannot open source file \"%ls\":\n%ls"),
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
  COPY_FILE_REP (res = vfs_chmod (__dst, stat.st_mode),
                 action_error_retryskipcancel_ign,
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
                     action_error_retryskipcancel,
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
                     action_error_retryskipcancel,
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
          int t = INCOMPLETE_MESSAGE ();
          /* If file hasn't been fully copied */
          if (t == MR_NO || t == MR_CANCEL)
            {
              vfs_unlink (__dst);
            }

          /* Update limit of max position in bytes progress */
          REDUCE_TOTAL_BYTES (remain);
        }

      /* Reset skip flag */
      __proc_wnd->skip = FALSE;

      if (__proc_wnd->abort)
        {
          return ACTION_ABORT;
        }
    }

  /* Unlink source file only if file will be moved, it will moved by */
  /* copy+unlink strategy and it was _fully_ moved */
  if (__proc_wnd->move && remain == 0 &&
      (__proc_wnd->move_strategy == VFS_MS_COPY || append))
    {
      /* Source will not be unlinked immediately */
      /* This may be helpful if user canceled moving */
      /* Just add this file to list of items to be unlinked */

      deque_push_back (__proc_wnd->unlink_list, wcsdup (__src));
      ++__proc_wnd->unlink_count;
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
              return ACTION_SKIP;

            case MR_COPY_REPLACE_ALL:
              SAVE_OWR_ALL_RULE (MR_COPY_REPLACE_ALL);
              UNLINK_TARGET ();
              break;

            case MR_COPY_UPDATE:
              SAVE_OWR_ALL_RULE (MR_COPY_UPDATE);
              if (!is_newer (__src, __dst))
                {
                  return ACTION_SKIP;
                }
              UNLINK_TARGET ();
              break;

            case MR_COPY_NONE:
              SAVE_OWR_ALL_RULE (MR_COPY_NONE);
              return ACTION_SKIP;

            case MR_COPY_SIZE_DIFFERS:
              SAVE_OWR_ALL_RULE (MR_COPY_SIZE_DIFFERS);
              if (!is_size_differs (__src, __dst))
                {
                  return ACTION_SKIP;
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
              res = action_error_retryskipcancel (_(L"Cannot stat target "
                                                 L"file \"%ls\":\n%ls"), __dst,
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

  if (CAN_USE_RENAME ())
    {
      make_rename (__src, __dst, __proc_wnd);
    }
  else
    {
      /* Create symlink */
      ACTION_REPEAT (res = vfs_symlink (content, __dst),
                     action_error_retryskipcancel,
                     return ACTION_CANCEL_TO_ABORT (__dlg_res_),
                     _(L"Cannot create symbolic link \"%ls\":\n%ls"), __src,
                     vfs_get_error (res));

      if (__proc_wnd->move)
        {
          /* Symlinks (like files) shouldn't be unlimked immediately */

          deque_push_back (__proc_wnd->unlink_list, wcsdup (__src));
          ++__proc_wnd->unlink_count;
        }
    }

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
  ACTION_REPEAT (res = vfs_stat (__src, &stat), action_error_retryskipcancel,
                 return ACTION_CANCEL_TO_ABORT (__dlg_res_),
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
                  return ACTION_SKIP;

                case MR_COPY_REPLACE_ALL:
                  SAVE_OWR_ALL_RULE (MR_COPY_REPLACE_ALL);
                  UNLINK_TARGET ();
                  break;

                case MR_COPY_UPDATE:
                  SAVE_OWR_ALL_RULE (MR_COPY_UPDATE);
                  if (!is_newer (__src, __dst))
                    {
                      return ACTION_SKIP;
                    }
                  UNLINK_TARGET ();
                  break;

                case MR_COPY_NONE:
                  SAVE_OWR_ALL_RULE (MR_COPY_NONE);
                  return ACTION_SKIP;

                case MR_COPY_SIZE_DIFFERS:
                  SAVE_OWR_ALL_RULE (MR_COPY_SIZE_DIFFERS);
                  /*
                   * TODO: But does it work properly?
                   */
                  if (!is_size_differs (__src, __dst))
                    {
                      return ACTION_SKIP;
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
                  res = action_error_retryskipcancel (_(L"Cannot stat target "
                                                   L"file \"%ls\":\n%ls"),
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
                      return ACTION_SKIP;
                    }
                }
            }

          /* Process accamulated queue of characters */
          COPY_PROCESS_QUEUE ();

          break;
        }

      if (CAN_USE_RENAME ())
        {
          /* Rename special file */
          make_rename (__src, __dst, __proc_wnd);
        }
      else
        {
          /* Create special file */
          ACTION_REPEAT (res = vfs_mknod (__dst, stat.st_mode, stat.st_rdev),
                         action_error_retryskipcancel,
                         return ACTION_CANCEL_TO_ABORT (__dlg_res_),
                         _(L"Cannot create special file \"%ls\":\n%ls"), __src,
                         vfs_get_error (res));

          if (__proc_wnd->move)
            {
              /* Special files (like regilar files) shouldn't be */
              /* unlimked immediately */

              deque_push_back (__proc_wnd->unlink_list, wcsdup (__src));
              ++__proc_wnd->unlink_count;
            }
        }
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
  CHECK_THE_SAME ();

  /* Stat source file to determine it's type */
  ACTION_REPEAT (res = vfs_lstat (__src, &stat), action_error_retryskipcancel,
                 return ACTION_CANCEL_TO_ABORT (__dlg_res_),
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
  int count, i, res, global_res, ignored_items = 0;
  wchar_t *full_name, *full_dst;
  size_t fn_len, dst_len;
  BOOL prescanned = FALSE;
  int move_strategy = MOVE_STRATEGY_UNDEFINED;

  /* Check is file copying to itself */
  CHECK_THE_SAME ();

  if (__proc_wnd->move)
    {
      /* Directory is moving */
      move_strategy = vfs_move_strategy (__src, __dst);
      if (move_strategy == VFS_MS_RENAME)
        {
          BOOL can_rename;

          can_rename = TRUE;

          /* Stat target directory */
          /* We need there because it is the simplest way to make */
          /* replacing files in destination directory not boring */
          /* We just don't rename directory, but it's children */
          /* will be easily renamed. */
          COPY_DIR_REP (res = vfs_stat (__dst, &stat);
                        if (res != -ENOENT)
                          {
                            can_rename = FALSE;
                          }
                        else
                          {
                            res = 0;
                          },
                       action_error_retryskipcancel,
                        _(L"Cannot stat target directory \"%ls\":\n%ls"),
                        __src, vfs_get_error (res));

          if (can_rename)
            {
              res = make_rename (__src, __dst, __proc_wnd);
              return res;
            }
        }
    }

  /* Stat source directory */
  COPY_DIR_REP (res = vfs_stat (__src, &stat);, action_error_retryskipcancel,
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
                    action_error_retryskipcancel,
                    _(L"Cannot listing source directory \"%ls\":\n%ls"),
                    __src, vfs_get_error (res));
    }

  /*
   * NOTE: Destination directory must be created AFTER listing of source one.
   *       It depends on posibility that destination directory will
   *       be created inside source.
   */

  /* Create destination directory */
  COPY_DIR_REP (res = vfs_mkdir (__dst, 0);
                if (res == -EEXIST) res = 0;, action_error_retryskipcancel,
                _(L"Cannot create target directory \"%ls\":\n%ls"),
                __dst, vfs_get_error (res));

  /* Set mode of destination directory */
  COPY_DIR_REP (res = vfs_chmod (__dst, stat.st_mode),
                action_error_retryskipcancel,
                _(L"Cannot chmod target directory \"%ls\":\n%ls"),
                __dst, vfs_get_error (res));

  /* Allocate memories for new strings */
  ALLOC_FN (full_name, fn_len, __src);
  ALLOC_FN (full_dst, dst_len, __dst);

  move_strategy = MOVE_STRATEGY_UNDEFINED;

  /* Review contents of source directory */
  global_res = ACTION_OK;
  for (i = 0; i < count; i++)
    {
      if (!IS_PSEUDODIR (eps[i]->name))
        {
          /* Get full filename of current file or directory and */
          /* it's destination */
          swprintf (full_name, fn_len, L"%ls/%ls", __src, eps[i]->name);

          swprintf (full_dst, dst_len, L"%ls/%ls", __dst,
                    eps[i]->name);

          /* Make copying/moving */
          if (!isdir (full_name))
            {
              int s_strategy;

              /* We need this because local move_strategy may be */
              /* truth, when move_strategy from window's descriptor */
              /* may be false. */
              if (__proc_wnd->move)
                {
                  s_strategy = __proc_wnd->move_strategy;

                  /* We need this because previously move_strategy */
                  /* was calclated for parent directory, not */
                  /* for child. It is not the same. Because parent */
                  /* may be a mount-point */
                  if (move_strategy == MOVE_STRATEGY_UNDEFINED)
                    {
                      move_strategy = vfs_move_strategy (full_name,
                                                         full_dst);
                    }
                  __proc_wnd->move_strategy = move_strategy;
                }

              res = copy_file (full_name, full_dst,
                               __owr_all_rule, __proc_wnd);

              if (__proc_wnd->move)
                {
                  __proc_wnd->move_strategy = s_strategy;
                }
            }
          else
            {
              res = copy_dir (full_name, full_dst, __owr_all_rule,
                             __proc_wnd,
                             prescanned ? __tree->items[i] : NULL);

            }

          if (res == ACTION_IGNORE || res == ACTION_SKIP)
            {
              ++ignored_items;
              res = 0;
            }

          /* Copying has been aborted */
          if (res == ACTION_ABORT)
            {
              /* Free allocated memory */
              FREE_REMAIN_DIRENT ();
              global_res = ACTION_ABORT;
              break;
            }
        }

      /* Process accamulated queue of characters */
      widget_process_queue ();
      if (PROCESS_ABORTED ())
        {
          /* Free allocated memory */
          FREE_REMAIN_DIRENT ();

          if (__proc_wnd->abort)
            {
              global_res = ACTION_ABORT;
            }

          break;
        }

      if (prescanned)
        {
          vfs_free_dirent (eps[i]);
        }
    }

  if (__proc_wnd->move)
    {
      /* If all it's content has been successfully moved, */
      /* we should unlink source directory */

      if (global_res == ACTION_OK && ignored_items == 0)
        {
          deque_push_back (__proc_wnd->unlink_list, wcsdup (__src));
          ++__proc_wnd->unlink_count;
        }
    }

  /* Free used variables */
  if (!prescanned)
    {
      SAFE_FREE (eps);
    }

  free (full_name);
  free (full_dst);

  return global_res;
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

  __proc_wnd->move_strategy = MOVE_STRATEGY_UNDEFINED;

  if (__proc_wnd->move)
    {
      /* Get move strategy */
      __proc_wnd->move_strategy = vfs_move_strategy (__src, __dst);
    }

  /*
   * TODO: Nay be we should use ACTION_REPEAT(...) instead of stupid isdir?
   */
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

      /* Copy/move single file */
      res = copy_file (__src, rdst, __owr_all_rule, __proc_wnd);
    }

  SAFE_FREE (rdst);

  return res;
}

/**
 * Unlink all items from list of items to be unlinked
 *
 * @param __proc_wnd - window with different current information
 * @return zero on sucess, non-zero otherwise
 */
static int
make_unlink (copy_process_window_t *__proc_wnd)
{
  post_move_window_t *wnd;
  wchar_t *path, fit_path[1024], *format;
  int fit_width, res;
  __int64_t count = 0;

  int (*proc) (const wchar_t *);

  if (!__proc_wnd || !__proc_wnd->unlink_list)
    {
      return -1;
    }

  /* Hide copying progress window */
  w_window_hide (__proc_wnd->window);

  /* Create and show new progress window */
  wnd = action_post_move_create_window ();
  w_window_show (wnd->window);

  w_progress_set_max (wnd->progress, __proc_wnd->unlink_count);

  fit_width = wnd->window->position.width - wnd->file->position.x - 1;

  /* Unlink each item from list */
  deque_foreach (__proc_wnd->unlink_list, path);
    fit_dirname (path, fit_width, fit_path);
    w_text_set (wnd->file, fit_path);

    /* Get format for error and unlinking function */
    if (isdir (path))
      {
        format = _(L"Cannot unlink source directory \"%ls\":\n%ls");
        proc = vfs_rmdir;
      }
    else
      {
        format = _(L"Cannot unlink source file \"%ls\":\n%ls");
        proc = vfs_unlink;
      }

    ACTION_REPEAT (res = proc (path), action_error_retryskipcancel_ign,
                   action_post_move_desstroy_window (wnd);
                   if (ACTION_CANCEL_TO_ABORT (__dlg_res_) == ACTION_ABORT)
                     {
                       return ACTION_ABORT;
                     },
                   format, path, vfs_get_error (res))

    w_progress_set_pos (wnd->progress, ++count);
  deque_foreach_done

   /* Free used memory */
  action_post_move_desstroy_window (wnd);

  return ACTION_OK;
}

/**
 * Copy file or directory
 *
 * @param __move - if FALSE, then make copying of files,
 * otherwise - move files
 * @param __base_dir - base directpry
 * @param __src_list - list of source items
 * @param __count - count of items to be copied
 * @param __dst - URL of destination
 * @return count of copied items
 */
static unsigned long
make_copy (BOOL __move, const wchar_t *__base_dir,
           const file_panel_item_t **__src_list, unsigned long __count,
           const wchar_t *__dst)
{
  copy_process_window_t *wnd;
  int res, owr_all_rule = 0;
  wchar_t *dst, *src, *dummy = (wchar_t*) __dst, *item_name;
  unsigned long i, count = 0, source_count;
  file_panel_item_t *item;
  BOOL scan_allowed;
  action_listing_t listing;

  if (!__base_dir || !*__src_list || !__dst)
    {
      return 0;
    }

  memset (&listing, 0, sizeof (listing));

  /* Get customized settings from user */
  res = action_copy_show_dialog (__move, __src_list, __count, &dummy);

  /* Count of source items */
  source_count = __count;

  /*
   * TODO: Should we normalize destination?
   */

  /* User canceled copying */
  if (res == MR_CANCEL)
    {
      SAFE_FREE (dummy);
      return 0;
    }

  scan_allowed = total_progress_avaliable (__src_list, __count);

  /* Get listing of items */
  if (scan_allowed)
    {
      ACTION_REPEAT (res = action_get_listing (__base_dir, __src_list,
                                     __count, &listing, __move);
                     if (res == ACTION_ABORT)
                       {
                         return 0;
                       },
                     action_error_retryskipcancel_ign,
                     /* There was an error getting full listing of items */
                     /* If user will answer Cancel to error dialog, none */
                     /* of files will be copied/moved */
                     /* But if user will answer Ignore prescanning will be */
                     /* disabled and some items will be copied/moved */
                     return 0,
                     _(L"Cannot get listing of items:\n%ls"),
                     vfs_get_error (res));

      if (res)
        {
          /* res may be non-zero here only in case */
          /* user hit an Ignore button */
          scan_allowed = FALSE;
        }
      else
        {
          /* User can ignore some subtrees, so we need */
          /* get count of source elements from prescanned data */
          source_count = listing.tree->count;
        }
    }

  /* Get absolute destination path */
  if (dummy[0] == '/')
    {
      dst = dummy;
    }
  else
    {
      dst = wcdircatsubdir (__base_dir, dummy);
      free (dummy);
    }

  wnd = action_copy_create_proc_wnd (__move, scan_allowed, &listing);

  wnd->abs_path_prefix = (wchar_t*)__base_dir;
  w_window_show (wnd->window);

  item = NULL;
  for (i = 0; i < source_count; ++i)
    {
      if (scan_allowed)
        {
          item_name = listing.tree->dirent[i]->name;
        }
      else
        {
          item = (file_panel_item_t*)__src_list[i];
          item_name = item->file->name;
        }

      /* Make copy iteration */
      src = wcdircatsubdir (__base_dir, item_name);
      res = make_copy_iter (src, dst, &owr_all_rule, wnd,
                            scan_allowed ? listing.tree->items[i] : NULL);
      free (src);

      if (res == 0)
        {
          /* In case of successfull copying */
          /* we need free selection from copied item */
          if (item != NULL)
            {
              /* item may be NULL only in case prescanning is disabled */
              item->selected = FALSE;
              count++;
            }
        }
      else
        {
          if (res == ACTION_ABORT)
            {
              break;
            }
        }

      /* Maybe this will help to grow up interactvity */
      widget_process_queue ();
      if (wnd->abort)
        {
          break;
        }
    }

  if (__move)
    {
      make_unlink (wnd);
    }

  action_copy_destroy_proc_wnd (wnd);

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
 * Copy/move list of files from specified panel
 *
 * @param __panel - from which panel files will be copied
 * @param __move - if FALSE, then make copying of files,
 * otherwise - move files
 * @return zero on success, non-zero otherwise
 */
int
action_copymove (file_panel_t *__panel, BOOL __move)
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

  if (!action_check_no_pseydodir ((const file_panel_item_t**)list, count))
    {
      wchar_t msg[1024];
      swprintf (msg, BUF_LEN (msg), _(L"Cannot operate on \"%ls\""),
                list[0]->file->name);
      MESSAGE_ERROR (msg);
      SAFE_FREE (list);
      return ACTION_ERR;
    }

  /* Get second panel to start copying */
  opposite_panel = action_choose_file_panel (__move?_(L"Move"):_(L"Copy"),
                                             _(L"Target panel"));
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
  count = make_copy (__move, cwd, (const file_panel_item_t**)list, count, dst);

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
  /* so, we need to rescan it */
  file_panel_rescan (__panel);

  /* There is new items on opposite panel */
  /* so, we also need to rescan it */
  file_panel_rescan (opposite_panel);

  return ACTION_OK;
}
