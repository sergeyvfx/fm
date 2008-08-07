/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Choose file panel action
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "actions.h"
#include "deque.h"

/**
 * Chooses file panel for action
 *
 * @return
 *  - If there is only one panel or user canceled operation, NULL returnef
 *  - If there is only two file panels, opposite to curretn will be returned
 *  - If there is more than two file panels, user'll see a list of
 */
file_panel_t*
action_choose_file_panel          (void)
{
  int count=file_panel_get_count ();
  deque_t *panels=file_panel_get_list ();

  if (count<=1)
    return NULL;

  if (count==2)
    {
      /* Return oppsite file panel  */
      if (deque_data (deque_head (panels))!=current_panel)
        return deque_data (deque_head (panels)); else
        return deque_data (deque_tail (panels));
    } else {
      /*
       * TODO:
       *  Add displaying of file panels list here
       */
    }

  return NULL;
}
