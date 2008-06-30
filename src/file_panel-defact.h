/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Default actions' handlers for file panel
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _file_panel_defact_
#define _file_panel_defact_

#include "smartinclude.h"
#include "dir.h"

BEGIN_HEADER

//
// NOTE:
//  To reduce lengths of symbols from this module,
//  prefix `file_panel_defact_` reduced to `fpd_`.
//

typedef struct {
  struct {
    dircmp_proc    comparator;
    dirfilter_proc filter;
  } dir;
} fpd_data_t;

int
fpd_sortorder_menu_callback       (void *__user_data);

END_HEADER

#endif
