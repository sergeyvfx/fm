/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Listing actions
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "actions.h"
#include "deque.h"
#include "dir.h"

/********
 * Internal stuff
 */

/**
 * Allocate listing tree
 *
 * @param pointer to new tree
 */
static action_listing_tree_t*
allocate_listing_tree ()
{
  action_listing_tree_t *res;
  res = malloc (sizeof (action_listing_tree_t));
  res->count = 0;
  res->dirent = NULL;
  res->items = NULL;
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
 * Iterator for action_get_listing()
 * Get listing tree start from specified item
 *
 * @param __path - current ditectory
 * @param __res - pointer to a structure, where result will be saved
 * @param __count - total count of files
 * @param __size - total size of files
 */
static int
get_listing_iter (const wchar_t *__path, action_listing_tree_t **__res,
                  __u64_t *__count, __u64_t *__size)
{
  if (isdir (__path))
    {
      long i, count;
      vfs_dirent_t **dirent;
      size_t len;
      wchar_t *cur;

      /* Scan directory */
      count = vfs_scandir (__path, &dirent, 0, vfs_alphasort);

      if (count < 0)
        {
          /* Error getting content of directory */
          return count;
        }

      (*__res) = allocate_listing_tree ();

      /* Set fields of tree */
      (*__res)->count = count;
      (*__res)->dirent = dirent;

      MALLOC_ZERO ((*__res)->items, count * sizeof (action_listing_tree_t*));

      len = wcslen (__path) + MAX_FILENAME_LEN + 1;
      cur = malloc ((len + 1) * sizeof (wchar_t));

      /* Scan children */
      for (i = 0; i < count; ++i)
        {
          /* Do not add pseudo-dirs '.' and '..' */
          if (wcscmp (dirent[i]->name, L".") != 0 &&
              wcscmp (dirent[i]->name, L"..") != 0)
            {
              swprintf (cur, len, L"%ls/%ls", __path, dirent[i]->name);
              get_listing_iter (cur, &(*__res)->items[i], __count, __size);
            }
        }

      free (cur);
    }
  else
    {
      int res;
      vfs_stat_t stat;

      /* There is no children */
      if ((res=vfs_lstat (__path, &stat)) == VFS_OK)
        {
          if (S_ISREG (stat.st_mode) || S_ISLNK (stat.st_mode))
            {
              (*__count)++;
              (*__size) += stat.st_size;
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
 * @return zero on success, non-zero otherwise
 */
int
action_get_listing (const wchar_t *__base_dir,
                    const file_panel_item_t **__list,
                    unsigned long __count, actions_listing_t *__res)
{
  unsigned long i;
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
      res = get_listing_iter (cur, &__res->tree->items[i],
                              &__res->count, &__res->size);

      /* There is an error while listing */
      if (res)
        {
          break;
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
action_free_listing (actions_listing_t *__self)
{
  if (!__self)
    {
      return;
    }

  free_listing_iter (__self->tree);
}
