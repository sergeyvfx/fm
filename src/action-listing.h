/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Listing action
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _action_listing_h_
#define _action_listing_h_

/********
 * Type definitions
 */

typedef struct action_tree_node {
  /* Count of items in node */
  long count;

  /* Will be TRUE when there are any ignored children */
  BOOL ignored_flag;

  /* Directory entries */
  vfs_dirent_t **dirent;

  /* Children */
  struct action_tree_node **items;
} action_listing_tree_t;

typedef struct {
  /* Total count of items */
  __u64_t count;

  /* Items' total size */
  __u64_t size;

  /* Tree of items */
  action_listing_tree_t *tree;
} action_listing_t;

/********
 *
 */

int
action_get_listing (const wchar_t *__base_dir,
                    const file_panel_item_t **__list,
                    unsigned long __count, action_listing_t *__res,
                    BOOL __ignore_errors);

void
action_free_listing (action_listing_t *__self);

END_HEADER

#endif
