/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of action 'Copy'
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "actions.h"
#include "action-copymove.h"

/**
 * Copy list of files from specified panel
 *
 * @param __panel - from which panel files will be copied
 * @param __move - if FALSE, then make copying of files,
 * otherwise - move files
 * @return zero on success, non-zero otherwise
 */
int
action_copy (file_panel_t *__panel)
{
  return action_copymove (__panel, FALSE);
}
