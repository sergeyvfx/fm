/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Prototypes of file actions
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _actions_h_
#define _actions_h_

#include "smartinclude.h"

BEGIN_HEADER

int
action_copy                       (const wchar_t *__src,
                                   const wchar_t *__dst);

END_HEADER

#endif
