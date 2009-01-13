/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of find file operation
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "actions.h"
#include "action-find.h"
#include "action-find-iface.h"
#include "util.h"
#include "dir.h"
#include "i18n.h"
#include "deque.h"
#include "hook.h"

#include <vfs/vfs.h>

#include <ctype.h>

#define FIND_RES_DIR  0
#define FIND_RES_ITEM 1

/* Size of buffer for buffered reading in simple content matching */
#define BUF_SIZE 524288

/* Size of buffer for regular expression matching of content */
#define RE_BUF_SIZE 524288

#define ACTION_PERFORMED(__wnd) \
  (__wnd->window->modal_result != 0)

#define USER_ACTION(__wnd) \
  (__wnd->window->modal_result)

typedef struct
{
  int type;
  wchar_t *dir;
  wchar_t *name;
} list_item_value_t;

/**
 * Free find options
 *
 * @param __options - options to be freed
 */
static void
free_find_options (action_find_options_t *__options)
{
  int i;

  SAFE_FREE (__options->file_mask);
  SAFE_FREE (__options->content);
  SAFE_FREE (__options->start_at);

  /* Destroy precompiled options */
  for (i = 0; i < __options->re_file_count; ++i)
    {
      regexp_free (__options->re_file[i]);
    }
  SAFE_FREE (__options->re_file);

  regexp_free (__options->re_content);

  SAFE_FREE (__options->mb_content);
}

/**
 * Build regular expression string from given string
 * without special characters (regexp beginning and finishing) and
 * any modifiers.
 *
 * @param __str - string to build regular expression from
 * @param __case_insens - append `case insensitive` modifier
 * @return regular expression string
 * @sideeffect allocate memory for output value
 */
static wchar_t*
build_regexp (const wchar_t *__str, BOOL __case_insens)
{
  wchar_t *regexp;
  wchar_t modifiers[8];
  size_t re_len = wcslen (__str) + 3;

  modifiers[0] = 0;

  /* Build string with modifiers */
  if (__case_insens)
    {
      /* Reserve character for modifier */
      ++re_len;
      wcscat (modifiers, L"i");
    }

  regexp = malloc ((re_len + 1) * sizeof (wchar_t));

  swprintf (regexp, re_len, L"/%ls/%ls", __str, modifiers);

  return regexp;
}

/**
 * Make buffer lowercase
 *
 * @param __buf - buffer to make lower-cased
 * @param __size - size of buffer
 */
static void
lowercase_buffer (char *__buf, size_t __size)
{
  size_t i;

  for (i = 0; i < __size; ++i)
    {
      __buf[i] = tolower (__buf[i]);
    }
}

/**
 * Precompile options for faster usage
 * (i.e. compile regular expressions, )
 *
 * @param __options - pointer to options to be precompiled
 * @return zero on success, non-zero otherwise
 */
static int
precompile_options (action_find_options_t *__options)
{
  wchar_t *file_mask, *regexp;
  int result = ACTION_OK;
  BOOL case_insens;

  /* Validate file name mask */
  if (wcscmp (__options->file_mask, L"") &&
      !TEST_FLAG (__options->flags, AFF_MASK_REGEXP))
    {
      file_mask = L"*";
    }

  /* Precompile regexp for file name mask */
  case_insens = !TEST_FLAG (__options->flags,
                            AFF_MASK_CASE_SENSITIVE);

  if (TEST_FLAG (__options->flags, AFF_MASK_REGEXP))
    {
      __options->re_file_count = 1;
      __options->re_file = malloc (sizeof (regexp_t*));
      regexp = build_regexp (__options->file_mask, case_insens);

      __options->re_file[0] = wregexp_compile (regexp);

      if (!__options->re_file[0])
        {
          /*
           * TODO: Add error handling here
           */
          result = ACTION_ERR;
        }

      free (regexp);
    }
  else
    {
      wchar_t **masks;
      long i, count;

      count = explode (__options->file_mask, L";", &masks);

      __options->re_file_count = count;
      __options->re_file = malloc (count * sizeof (regexp_t*));

      for (i = 0; i < count; ++i)
        {
          regexp = file_mask_to_regexp (masks[i], case_insens);

          __options->re_file[i] = wregexp_compile (regexp);

          if (!__options->re_file[i])
            {
              /*
               * TODO: Add error handling here
               */
              result = ACTION_ERR;
            }
          free (regexp);
        }

      free_explode_array (masks);
    }

  /* Process regexp for file's content */
  if (wcscmp (__options->content, L""))
    {
      case_insens = !TEST_FLAG (__options->flags,
                                AFF_CONTENT_CASE_SENSITIVE);

      /* We should compile regular expression for content */
      if (TEST_FLAG (__options->flags, AFF_CONTENT_REGEXP))
        {
          regexp = build_regexp (__options->content, case_insens);

          /* Compile regexp */
          __options->re_content = wregexp_compile (regexp);

          if (!__options->re_content)
            {
              /*
               * TODO: Add error handling here
               */
              result = ACTION_ERR;
            }

          free (regexp);
        }
      else
        {
          /* For speed improvement we won't use regular expressions */
          /* for searching substrings in files. */
          /* But we'd better convert wide-char content string to */
          /* multi-byte string because we wouldn't convert file's content */
          /* to multi-byte string. */
          wcs2mbs (&__options->mb_content, __options->content);

          if (case_insens)
            {
              /* If searching is case-insensitive, we should */
              /* compare buffers in the same cases. */
              lowercase_buffer (__options->mb_content,
                                strlen (__options->mb_content));
            }
        }
    }

  return result;
}

/**
 * Check any regexp from options matches file name
 *
 * @param __name - name of file to operate with
 * @param __options - finding options
 * @return zero if no regexp matches file name, non-zero otherwise
 */
static BOOL
check_name (const wchar_t *__name, const action_find_options_t *__options)
{
  int i;

  for (i = 0; i < __options->re_file_count; ++i)
    {
      if (wregexp_match (__options->re_file[i], __name))
        {
          /* File name matched by regular expression */
          return TRUE;
        }
    }

  return FALSE;
}

/**
 * Check content of file in non-regexp mode
 *
 * @param __name - name of file to check
 * @param __full_name - full name of file
 * @param __options - finding options
 * @param __res_wnd - window with search results
 * @return zero if file in unwanted, non-zero otherwise
 */
static BOOL
check_simple_content (const wchar_t *__name, const wchar_t *__full_name,
                      const action_find_options_t *__options,
                      action_find_res_wnd_t *__res_wnd)
{
  int res;
  BOOL matched = FALSE;
  vfs_file_t file;
  char *buf;
  char read_buf[BUF_SIZE];
  size_t content_len, content_hash;
  size_t i, n, h, d;
  size_t buf_len;

#ifdef __FILE_OFFSET64
  file = vfs_open (__full_name, O_LAGEFILE, 0);
#else
  file = vfs_open (__full_name, 0, 0);
#endif

  if (!file)
    {
      /* Error opening file */
      return FALSE;
    }

  content_len = strlen (__options->mb_content);

  buf = malloc (content_len + BUF_SIZE);

  /* Calculate hash for content to search */
  content_hash = 0;
  for (i = 0; i < content_len; ++i)
    {
      content_hash += (unsigned char)__options->mb_content[i];
    }

  /* Read initial buffer */
  res = vfs_read (file, buf, content_len);
  if (res == content_len)
    {
      bool case_insens;
      case_insens = !TEST_FLAG (__options->flags,
                                AFF_CONTENT_CASE_SENSITIVE);

      /* If initial buffer less than content length */
      /* we can't operate */

      buf_len = content_len;
      buf[buf_len] = 0;

      /* Calculate hash of initial buffer */
      h = 0;
      for (i = 0; i < buf_len; ++i)
        {
          if (case_insens)
            {
              buf[i] = tolower (buf[i]);
            }
          h += (unsigned char)buf[i];
        }

      for (;;)
        {
          n = buf_len - content_len;
          for (i = 0; i <= n; ++i)
            {
              /* IF hashes are equal, we can't guarantee strings are equal */
              /* We should ensure in it */
              if (h == content_hash)
                {
                  if (!strncmp (buf + i, __options->mb_content, content_len) )
                    {
                      matched = TRUE;
                      break;
                    }
                }
              else
                {
                  h -= (unsigned char)buf[i];
                  h += (unsigned char)buf[i + content_len];
                }
            }

          if (matched)
            {
              /* If file contains needed content, */
              /* we can break this stupid cycle */
              break;
            }

          hook_call (L"switch-task-hook", NULL);

          if (ACTION_PERFORMED (__res_wnd))
            {
              break;
            }

          /* Read next buffer */
          res = vfs_read (file, read_buf, BUF_SIZE);

          /* There is no new buffer to use */
          if (res <= 0)
            {
              break;
            }

          read_buf[res] = 0;

          if (case_insens)
            {
              lowercase_buffer (read_buf, res);
            }

          /* Shift overviewed buffer */
          d = buf_len - content_len + 1;
          for (i = 0; i < buf_len - d; ++i)
            {
              buf[i] = buf[i + d];
            }

          /* Append new data */
          memcpy (buf + i, read_buf, res + 1);

          h += (unsigned char)buf[content_len - 1];
          buf_len += res - d;
        }
    }

  vfs_close (file);
  free (buf);

  return matched;
}

/**
 * Check content of file in regexp mode
 *
 * @param __name - name of file to check
 * @param __full_name - full name of file
 * @param __options - finding options
 * @param __res_wnd - window with search results
 * @return zero if file in unwanted, non-zero otherwise
 */
static BOOL
check_regexp_content (const wchar_t *__name, const wchar_t *__full_name,
                      const action_find_options_t *__options,
                      action_find_res_wnd_t *__res_wnd)
{
  vfs_file_t file;
  char buf[RE_BUF_SIZE];
  char *pchar;
  int res, i;
  BOOL matched = FALSE, finito = FALSE;

#ifdef __FILE_OFFSET64
  file = vfs_open (__full_name, O_LAGEFILE, 0);
#else
  file = vfs_open (__full_name, 0, 0);
#endif

  if (file == NULL)
    {
      /* Error opening file */
      return FALSE;
    }

  for (;;)
    {
      /* Read next buffer from file */
      res = vfs_read (file, buf, RE_BUF_SIZE - 1);

      if (res <= 0)
        {
          /* Assume file is over and we should stop greping */
          break;
        }

      buf[res] = 0;
      pchar = buf;
      i = 0;

      for (;;)
        {
          /*
           * TODO: Or we should split read buffer onto separate strings?
           */

          if (regexp_match (__options->re_content, pchar))
            {
              matched = TRUE;
              break;
            }

          /* Read buffer could contain zero characters */
          /* We should overview all parts of buffer */
          /* (between all pairs of zero characters ) */
          for (; i < res; ++i)
            {
              if (buf[i] == 0)
                {
                  break;
                }
            }

          if (i >= res)
            {
              /* Buffer is over */
              break;
            }

          ++i;
          pchar = buf + i;

          hook_call (L"switch-task-hook", NULL);

          if (ACTION_PERFORMED (__res_wnd))
            {
              finito = TRUE;
              break;
            }
        }

      if (finito)
        {
          break;
        }
    }

  vfs_close (file);

  return matched;
}

/**
 * Check is regular file satisfy needed parameters
 *
 * @param __name - name of file to check
 * @param __full_name - full name of file
 * @param __options - finding options
 * @param __res_wnd - window with search results
 * @return zero if file in unwanted, non-zero otherwise
 */
static BOOL
check_regular_file (const wchar_t *__name, const wchar_t *__full_name,
                    const action_find_options_t *__options,
                    action_find_res_wnd_t *__res_wnd)
{
  /* Check file name 'validness' */
  if (!check_name (__name, __options))
    {
      /* File name is unwanted by user, so we need to */
      /* do nothing else here */
      return FALSE;
    }

  if (wcscmp (__options->content, L"") == 0)
    {
      return TRUE;
    }

  if (TEST_FLAG (__options->flags, AFF_CONTENT_REGEXP))
    {
      return check_regexp_content (__name, __full_name, __options, __res_wnd);
    }
  else
    {
      return check_simple_content (__name, __full_name, __options, __res_wnd);
    }

  return FALSE;
}

/**
 * Check is special file satisfy needed parameters
 *
 * @param __name - name of file to check
 * @param __full_name - full name of file
 * @param __options - finding options
 * @param __res_wnd - window with search results
 * @return zero if special file in unwanted, non-zero otherwise
 */
static BOOL
check_special_file (const wchar_t *__name, const wchar_t *__full_name,
                    const action_find_options_t *__options,
                    action_find_res_wnd_t *__res_wnd)
{
  /* Check it here because of small speed increasing */
  if (__options->content && __options->content[0])
    {
      /* Special files can't have content */
      /* So in case user want files with specified content */
      /* we should say that special files aren't wanted  */
      return FALSE;
    }

  /* Check directory name 'validness' */
  if (!check_name (__name, __options))
    {
      /* Special file name is unwanted by user, so we need to */
      /* do nothing else here */
      return FALSE;
    }

  return TRUE;
}

/**
 * Check is directory satisfy needed parameters
 *
 * @param __name - name of directory to check
 * @param __full_name - full name of file
 * @param __options - finding options
 * @param __res_wnd - window with search results
 * @return zero if directory in unwanted, non-zero otherwise
 */
static BOOL
check_directory (const wchar_t *__name, const wchar_t *__full_name,
                 const action_find_options_t *__options,
                 action_find_res_wnd_t *__res_wnd)
{
  /* Checking of directory is the same as checking of special file */
  return check_special_file (__name, __full_name, __options, __res_wnd);
}

/**
 * Prepare row for table
 *
 * @param __str - string for table's row to be prepared
 */
static void
prepare_row (wchar_t *__str, size_t __len)
{
  size_t i;

  for (i = 0; i < __len; ++i)
    {
      __str[i] = ' ';
    }

  __str[__len] = 0;
}

/**
 * Print table's cell in string
 *
 * @param __str - string with table's row
 * @param __cell - cell content
 * @param __pos - position of cell
 * @param __len - length of string for row
 */
static void
print_cell (wchar_t *__str, const wchar_t *__cell, size_t __pos, size_t __len)
{
  size_t len = wcslen (__cell);

  if (__pos + len >= __len)
    {
      len = __len - __pos - 1;
    }

  wcsncpy (__str + __pos, __cell, len);
  __str[__pos + len] = ' ';
}

/**
 * Store information in list value to use for navigating
 *
 * @param __dir - directory where item has been found
 * @param __name - name of item
 * @param __type - type of result row
 */
static void
store_item_value (w_list_item_t *item, int __type,
                  const wchar_t *__dir, const wchar_t *__name)
{
  list_item_value_t *value;

  MALLOC_ZERO (value, sizeof (list_item_value_t));

  value->type = __type;

  if (__dir)
    {
      value->dir = wcsdup (__dir);
    }

  if (__name)
    {
      value->name = wcsdup (__name);
    }

  item->data = value;
}

/**
 * Free items' data in list of results
 *
 * @param __list - list widget
 */
static void
free_items_values (w_list_t *__list)
{
  int i, count;
  w_list_item_t *item;
  list_item_value_t *value;

  count = w_list_items_count (__list);

  for (i = 0; i < count; ++i)
    {
      item = w_list_get_item (__list, i);
      value = item->data;

      SAFE_FREE (value->dir);
      SAFE_FREE (value->name);
      free (value);
    }
}

/**
 * Append entry to list of found items
 *
 * @param __dir - directory where item has been found
 * @param __name - name of item
 * @param __stat - stat information of item
 * @param __res_wnd - window with results
 */
static void
append_result (const wchar_t *__dir, const wchar_t *__name, vfs_stat_t __stat,
               action_find_res_wnd_t *__res_wnd)
{
  wchar_t *string, *dummy;
  size_t len, tmp_len;
  wchar_t buf[128];
  wchar_t suffix;
  w_list_item_t *item;

#ifdef __USE_FILE_OFFSET64
  __u64_t size;
  static wchar_t format[] = L"%lld %c";
#else
  __u32_t size;
  static wchar_t format[] = L"%ld %c";
#endif

  len = __res_wnd->list->position.width - 2;
  string = malloc ((len + 1) * sizeof (wchar_t));
  dummy = malloc ((len + 1) * sizeof (wchar_t));

  /* Append directory for which item belongs to */
  if (!__res_wnd->dir_opened)
    {
      size_t l;

      prepare_row (string, len);
      l = __res_wnd->list->position.width / 4 * 3 - 5;

      fit_dirname (__dir, l, dummy);
      print_cell (string, dummy, 0, len);
      print_cell (string, _(L"<DIR>"), l + 1, len);
      item = w_list_append_item (__res_wnd->list, string, FIND_RES_DIR);

      store_item_value (item, FIND_RES_DIR, __dir, NULL);

      __res_wnd->dir_opened = TRUE;
    }

  prepare_row (string, len);

  /* Name of file */
  fit_dirname (__name, __res_wnd->list->position.width / 2, dummy);
  print_cell (string, dummy, 4, len);

  /* Mode of file */
  umasktowcs (__stat.st_mode, buf);
  print_cell (string, buf, __res_wnd->list->position.width - 10, len);

  /* Modification time */
  format_file_time (buf, BUF_LEN (buf), __stat.st_mtime);
  print_cell (string, buf, __res_wnd->list->position.width - 24, len);

  /* Size of file */
  if (!S_ISDIR (__stat.st_mode))
    {
      size = fsizetohuman (__stat.st_size, &suffix);
      swprintf (buf, BUF_LEN(buf), format, size, suffix);
    }
  else
    {
      wcscpy (buf, _(L"<DIR>"));
    }
  tmp_len = wcslen (buf);
  if (buf[tmp_len - 1] == ' ')
    {
      buf[tmp_len - 1] = 0;
      --tmp_len;
    }
  print_cell (string, buf,
              __res_wnd->list->position.width - 26 - tmp_len, len);

  item = w_list_append_item (__res_wnd->list, string, FIND_RES_ITEM);

  store_item_value (item, FIND_RES_ITEM, __dir, __name);

  free (string);
  free (dummy);
}

/**
 * Set status in find results' window
 *
 * @param __res_wnd - window with results
 * @param __format - format of state
 */
static void
set_status (action_find_res_wnd_t *__res_wnd, const wchar_t *__format, ...)
{
  wchar_t buf[1024];

  PACK_ARGS(__format, buf, BUF_LEN (buf));

  w_text_set (__res_wnd->status, buf);
}

static void
set_searching_status (action_find_res_wnd_t *__res_wnd,
                      const wchar_t *__action, const wchar_t *__path)
{
  wchar_t format[1024], path[1024];
  size_t width;

  swprintf (format, BUF_LEN (format), L"%ls: %%ls", __action);

  width = wcswidth (format, wcslen (format));
  fit_dirname (__path,
               __res_wnd->window->position.width - 6 - width, path);

  set_status (__res_wnd, _(format), path);
}

/**
 * Recursive iteration for file finding
 *
 * @param __dir - directory to search file in
 * @param __rel_dir - relative director name to search file in
 * @param __options - finding options
 * @param __res_wnd - window with results
 * @return zero on success, non-zero otherwise
 */
static int
find_iteration (const wchar_t *__dir, const wchar_t *__rel_dir,
                const action_find_options_t *__options,
                action_find_res_wnd_t *__res_wnd)
{
  int i, j, count;
  vfs_dirent_t **eps = NULL;
  size_t fn_len;
  wchar_t *format, *full_name;
  vfs_stat_t stat;
  vfs_stat_proc stat_proc;
  deque_t *dirs;
  wchar_t **dir_data;

  __res_wnd->dir_opened = FALSE;

  /* Get listing of directory */

  /*
   * TODO: Add separately displaying of directories and files
   */

  count = vfs_scandir (__dir, &eps, 0, vfs_alphasort);

  if (count < 0)
    {
      /* Error getting listing */
      return ACTION_ERR;
    }

  if (TEST_FLAG(__options->flags, AFF_FIND_RECURSIVELY))
    {
      dirs = deque_create ();
    }

  /* Get function for stat'ing */
  if (TEST_FLAG(__options->flags, AFF_FOLLOW_SYMLINKS))
    {
      stat_proc = vfs_stat;
    }
  else
    {
      stat_proc = vfs_lstat;
    }

  /* Allocate memory for full file name */
  fn_len = wcslen (__dir) + MAX_FILENAME_LEN + 1;
  full_name = malloc ((fn_len + 1) * sizeof (wchar_t));

  /* Get format mask for correct directory drilling */
  if (__dir[wcslen (__dir) - 1] == '/')
    {
      format = L"%ls%ls";
    }
  else
    {
      format = L"%ls/%ls";
    }

  for (i = 0; i < count; ++i)
    {
      if (IS_PSEUDODIR (eps[i]->name))
        {
          vfs_free_dirent (eps[i]);
          continue;
        }

      set_searching_status (__res_wnd, L"Searching in", __rel_dir);

      /* Get full file name */
      swprintf (full_name, fn_len, format, __dir, eps[i]->name);

      /* Stat current node of FS */
      if (!stat_proc (full_name, &stat) == VFS_OK)
        {
          /* Error getting status of file */
          vfs_free_dirent (eps[i]);
          continue;
        }

      if (S_ISREG (stat.st_mode))
        {
          if (check_regular_file (eps[i]->name, full_name,
                                  __options, __res_wnd))
            {
              append_result (__rel_dir, eps[i]->name, stat, __res_wnd);
              ++__res_wnd->found_files;
            }
        }
      else if (S_ISDIR (stat.st_mode))
        {
          /* Of user wants directories to be found... */
          if (TEST_FLAG(__options->flags, AFF_FIND_DIRECTORIES))
            {
              if (check_directory (eps[i]->name, full_name,
                                   __options, __res_wnd))
                {
                  append_result (__rel_dir, eps[i]->name, stat, __res_wnd);
                  ++__res_wnd->found_dirs;
                }
            }

          if (TEST_FLAG(__options->flags, AFF_FIND_RECURSIVELY))
            {
              dir_data = malloc (2 * sizeof (wchar_t));
              dir_data[0] = wcsdup (eps[i]->name);
              dir_data[1] = wcsdup (full_name);
              deque_push_back (dirs, (void*)dir_data);
            }
        }
      else
        {
          if (check_special_file (eps[i]->name, full_name,
                                  __options, __res_wnd))
            {
              append_result (__rel_dir, eps[i]->name, stat, __res_wnd);
              ++__res_wnd->found_files;
            }
        }

      vfs_free_dirent (eps[i]);

      hook_call (L"switch-task-hook", NULL);

      if (ACTION_PERFORMED (__res_wnd))
        {
          /* Free remain dirents */
          for (j = i + 1; j < count; ++j)
            {
              vfs_free_dirent (eps[j]);
            }
          break;
        }
    }

  SAFE_FREE (eps);
  free (full_name);

  if (TEST_FLAG(__options->flags, AFF_FIND_RECURSIVELY) &&
      !ACTION_PERFORMED (__res_wnd))
    {
      void *data;
      wchar_t *rel_name;

      if (__rel_dir[wcslen (__rel_dir) - 1] == '/')
        {
          format = L"%ls%ls";
        }
      else
        {
          format = L"%ls/%ls";
        }

      fn_len = wcslen (__dir) + MAX_FILENAME_LEN + 1;
      rel_name = malloc ((fn_len + 1) * sizeof (wchar_t));

      deque_foreach (dirs, data);
        /* Drill relative file name */

        dir_data = data;
        if (!ACTION_PERFORMED (__res_wnd))
          {
            swprintf (rel_name, fn_len, format, __rel_dir, dir_data[0]);
            find_iteration (dir_data[1], rel_name, __options, __res_wnd);
          }
        free (dir_data);
      deque_foreach_done
    }

  return ACTION_OK;
}

/**
 * Process modal result of result list window
 *
 * @param __panel - descriptor of file panel from which find operation
 * has been called
 * @param __res_wnd - window with results
 * @param __cwd - currently working directory
 */
static int
process_modal_result (file_panel_t *__panel,
                      action_find_res_wnd_t *__res_wnd, const wchar_t *__cwd)
{
  int modal_result = __res_wnd->window->modal_result;

  if (modal_result == AF_GOTO || modal_result == MR_OK)
  {
    w_list_item_t *item;
    list_item_value_t *value;
    wchar_t *dir, *dummy;

    item = w_list_get_current_item (__res_wnd->list);

    if (!item)
      {
        return ACTION_OK;
      }

    value = item->data;

    dummy = vfs_abs_path (value->dir, __cwd);
    dir = vfs_normalize_full (dummy, FALSE);
    free (dummy);

    file_panel_set_cwd (__panel, dir);

    if (value->type == FIND_RES_ITEM)
      {
        FILE_PANEL_ACTION_CALL (__panel, centre_to_item, value->name);
      }

    free (dir);
    return ACTION_OK;
  }

  return modal_result;
}

/**
 * Find files
 *
 * @param __panel - descriptor of file panel from which find operation
 * has been called
 * @param __cwd - currently working directory
 * @param __options - finding options
 * @return zero on success, non-zero otherwise
 */
static int
make_find (file_panel_t *__panel,
           const wchar_t *__cwd, action_find_options_t *__options)
{
  action_find_res_wnd_t *wnd;
  wchar_t *dir;
  int res;

  wnd = action_find_create_res_wnd ();
  w_window_show (wnd->window);

  dir = vfs_abs_path (__options->start_at, __cwd);

  find_iteration (dir, __options->start_at, __options, wnd);

  if (TEST_FLAG (__options->flags, AFF_FIND_DIRECTORIES))
    {
      set_status (wnd, _(L"Search done. Found %ld file(s) "
                          "and %ld directories(s)"),
                  wnd->found_files, wnd->found_dirs);
    }
  else
    {
      set_status (wnd, _(L"Search done. Found %ld file(s)"), wnd->found_files);
    }

  if (!ACTION_PERFORMED (wnd) || USER_ACTION (wnd) == AF_STOP)
    {
      w_window_set_modal (wnd->window, TRUE);
    }

  res = process_modal_result (__panel, wnd, __cwd);

  free_items_values (wnd->list);
  action_find_destroy_res_wnd (wnd);

  free (dir);

  return res;
}

/********
 * User's backend
 */

/**
 * Find file operation
 *
 * @param __panel - descriptor of file panel from which find operation
 * has been called
 * @return zero on success, non-zero otherwise
 */
int
action_find (file_panel_t *__panel)
{
  action_find_options_t options;
  wchar_t *cwd;
  BOOL finito;

  if (!__panel)
    {
      return ACTION_ERR;
    }

  cwd = file_panel_get_full_cwd (__panel);

  memset (&options, 0, sizeof (options));
  do
    {
      finito = TRUE;

      /* Get options from user */
      if (action_find_show_dialog (&options) == ACTION_OK)
        {
          if (precompile_options (&options) == ACTION_OK)
            {
              options.filled = TRUE;

              /* If compilation succeed call main searching stuff */
              int res;
              res = make_find (__panel, cwd, &options);

              if (res == AF_FIND_AGAIN)
                {
                  finito = FALSE;
                }
            }
        }
    }
  while (!finito);

  free_find_options (&options);
  free (cwd);

  return ACTION_OK;
}
