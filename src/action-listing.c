/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Listing actions
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include <dirent.h>

#include "actions.h"
#include "deque.h"
#include "dir.h"
#include "i18n.h"
#include "messages.h"

static void
free_listing_iter (action_listing_tree_t *__tree);

/*
 * Use ACTION_REPEAT for functions like vfs_scandir() which
 * may make this stuff more friendly for user.
 */
#define USE_ACTION_REPEAT 1

/********
 * Internal stuff
 */

#ifdef USE_ACTION_REPEAT

/**
 * Display an error message with buttons Retry, Ignore and cancel
 *
 * @param __text - text to display on message
 * @return result of message_box()
 */
static int
error (const wchar_t *__text, ...)
{
  int res;
  wchar_t buf[4096];
  PACK_ARGS (__text, buf, BUF_LEN (buf));
  res = message_box (_(L"Error"), buf, MB_CRITICAL | MB_RETRYSKIPCANCEL);
  if (res == MR_SKIP)
    {
      res = MR_IGNORE;
    }

  return res;
}

#endif

/**
 * Allocate listing tree
 *
 * @param pointer to new tree
 */
static action_listing_tree_t*
allocate_listing_tree ()
{
  action_listing_tree_t *res;
  MALLOC_ZERO (res, sizeof (action_listing_tree_t));
  return res;
}

/**
 * Add item to result list
 *
 * @param __res - node of a tree where item will be added
 * @param __item - item to add
 */
static void
listing_add_item (action_listing_tree_t *__res, vfs_dirent_t *__item)
{
  if (!__res || !__item)
    {
      return;
    }

  /* Allocate memory for directory entry */
  __res->dirent = realloc (__res->dirent,
                          (__res->count + 1) * sizeof (vfs_dirent_t));
  __res->dirent[__res->count] = __item;

  /* Allocate memory for child */
  __res->items = realloc (__res->items,
                         (__res->count + 1) * sizeof (action_listing_tree_t*));
  __res->items[__res->count] = NULL;

  __res->count++;
}

/**
 * Drop last item from tree item
 * Need if user want to ignore subtree
 *
 * @param __node - node of tree from where item will be dropped
 */
static void
listing_drop_item (action_listing_tree_t *__node)
{
  /* Free used memory */
  free_listing_iter (__node->items[__node->count - 1]);
  vfs_free_dirent (__node->dirent[__node->count - 1]);

  /* Re-allocate memory for directory entry */
  __node->dirent = realloc (__node->dirent,
                          (__node->count - 1) * sizeof (vfs_dirent_t));

  /* Re-allocate memory for child */
  __node->items = realloc (__node->items,
                             (__node->count - 1) *
                                sizeof (action_listing_tree_t*));

  --__node->count;
}

/**
 * Iterator for action_get_listing()
 * Get listing tree start from specified item
 *
 * @param __path - current ditectory
 * @param __res - pointer to a structure, where result will be saved
 * @param __count - total count of files
 * @param __size - total size of files
 * @param __ignore_errors - ignore error in listing procress
 */
static int
get_listing_iter (const wchar_t *__path, action_listing_tree_t **__res,
                  __u64_t *__count, __u64_t *__size, BOOL __ignore_errors)
{
  if (isdir (__path))
    {
      long i, count;
      vfs_dirent_t **dirent;
      size_t len;
      wchar_t *cur;
      int res;

      /* Scan directory */
#ifdef USE_ACTION_REPEAT

      if (__ignore_errors)
        {
          count = vfs_scandir (__path, &dirent, 0, vfs_alphasort);
          res = count < 0 ? count : 0;
        }
      else
        {
          ACTION_REPEAT (count = vfs_scandir (__path, &dirent, 0,
                                              vfs_alphasort);
                         res = count < 0 ? count : 0,
                         error, return ACTION_ABORT,
                         _(L"Cannot get listing of directory \"%ls\":\n%ls"),
                         __path, vfs_get_error (res));
        }

       if (res)
         {
           /* If res is not null and we are here, it means that */
           /* user hit an 'Ignore' button at dialog */
           return __ignore_errors ? ACTION_OK : ACTION_IGNORE;
         }
#else
      count = vfs_scandir (__path, &dirent, 0, vfs_alphasort);
#endif

      if (count < 0)
        {
          /* Error getting content of directory */
          return __ignore_errors ? ACTION_OK : count;
        }

      (*__res) = allocate_listing_tree ();

      /* Set fields of tree */
      (*__res)->count = count;
      (*__res)->dirent = dirent;

      MALLOC_ZERO ((*__res)->items, count * sizeof (action_listing_tree_t*));

      len = wcslen (__path) + MAX_FILENAME_LEN + 1;
      cur = malloc ((len + 1) * sizeof (wchar_t));

      /* Scan children */
      i = 0;
      while (i < count)
        {
          /* Do not add pseudo-dirs '.' and '..' */
          if (!IS_PSEUDODIR (dirent[i]->name))
            {
              swprintf (cur, len, L"%ls/%ls", __path, dirent[i]->name);
              res = get_listing_iter (cur, &(*__res)->items[i],
                                      __count, __size, __ignore_errors);
              if (res == ACTION_ABORT)
                {
                  free (cur);
                  return ACTION_ABORT;
                }

              if (res == ACTION_IGNORE)
                {
                  unsigned long long j;

                  /* Free memory used by 'invalid item' */
                  vfs_free_dirent (dirent [i]);

                  /* Shift data */
                  for (j = i; j < count - 1; ++j)
                    {
                      dirent[j] = dirent[j + 1];
                    }

                  /* Make allocated array a bit less */
                  dirent = realloc (dirent,
                                    (count - 1) * sizeof (vfs_dirent_t));
                  (*__res)->dirent = dirent;

                  (*__res)->items = realloc ((*__res)->items, (count - 1) *
                          sizeof (action_listing_tree_t*));

                  /* Set ignore flag */
                  (*__res)->ignored_flag = TRUE;

                  /* Update informtion about items count */
                  --count;
                  --(*__res)->count;
                }
              else
                {
                  ++i;
                }
            }
          else
            {
              ++i;
            }
        }

      free (cur);
    }
  else
    {
      int res;
      vfs_stat_t stat;

      /* There is no children */
      if ((res = vfs_lstat (__path, &stat)) == VFS_OK)
        {
          if (S_ISREG (stat.st_mode) || S_ISLNK (stat.st_mode) ||
              S_ISCHR (stat.st_mode) || S_ISBLK (stat.st_mode) ||
              S_ISFIFO (stat.st_mode) || S_ISSOCK (stat.st_mode))
            {
              (*__count)++;

              /* There is no need to collect sizes of symbolic links */
              if (S_ISREG (stat.st_mode))
                {
                  (*__size) += stat.st_size;
                }
            }
        }
      else
        {
          return res;
        }
    }

  return 0;
}

/**
 * Iterator to free listing tree
 *
 * @param __tree - tree to free
 */
static void
free_listing_iter (action_listing_tree_t *__tree)
{
  long i;

  if (!__tree)
    {
      return;
    }

  for (i = 0; i < __tree->count; ++i)
    {
      vfs_free_dirent (__tree->dirent[i]);
      free_listing_iter (__tree->items[i]);
    }
  free (__tree->dirent);
  free (__tree->items);
  free (__tree);
}

/********
 * User's backend
 */

/**
 * Get recursively listing of items from list
 *
 * @param __base_dir - base directory
 * @param __list - list of items
 * @param __count - count of items in list
 * @param __res - pointer to a structure, where result will be saved
 * @param __ignore_errors - ignore error in listing procress
 * @return zero on success, non-zero otherwise
 */
int
action_get_listing (const wchar_t *__base_dir,
                    const file_panel_item_t **__list,
                    unsigned long __count, action_listing_t *__res,
                    BOOL __ignore_errors)
{
  unsigned long i, ptr;
  wchar_t *cur, *format;
  int res = ACTION_OK;
  size_t len;
  vfs_dirent_t *dirent;

  if (!__base_dir || !__res)
    {
      return -1;
    }

  __res->count = 0;
  __res->size = 0;
  __res->tree = allocate_listing_tree ();

  len = wcslen (__base_dir) + MAX_FILENAME_LEN + 1;
  cur = malloc ((len + 1) * sizeof (wchar_t));

  /* Get mask to evalute full path to items */
  if (__base_dir [wcslen (__base_dir)-1] == '/')
    {
      format = L"%ls%ls";
    }
  else
    {
      format = L"%ls/%ls";
    }

  ptr = 0;
  for (i = 0; i < __count; ++i)
    {
      /* Get full path of current item */
      swprintf (cur, len, format, __base_dir, __list[i]->file->name);

      /* Make pseudo direcgtory entry  */
      MALLOC_ZERO (dirent, sizeof (vfs_dirent_t));
      wcscpy (dirent->name, __list[i]->file->name);
      dirent->type = IFTODT (__list[i]->file->stat.st_mode);

      /* Add our dirent to list */
      listing_add_item (__res->tree, dirent);

      /* Get listing of item */
      res = get_listing_iter (cur, &__res->tree->items[ptr],
                              &__res->count, &__res->size, __ignore_errors);

      /* There is an error while listing */
      if (res)
        {
          if (res == ACTION_IGNORE)
            {
              listing_drop_item (__res->tree);
              res = 0;
            }
          else
            {
              free_listing_iter (__res->tree);
              break;
            }
        }
      else
        {
          ptr++;
        }
    }

  return res;
}

/**
 * Free allocated in action_get_listing listing information
 *
 * @param __self - listing information to free
 */
void
action_free_listing (action_listing_t *__self)
{
  if (!__self)
    {
      return;
    }

  free_listing_iter (__self->tree);
}
