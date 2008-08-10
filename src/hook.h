/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Hooks implementation
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _hook_h_
#define _hook_h_

#include "smartinclude.h"

BEGIN_HEADER

enum
{
  HOOK_FAILURE = -1,
  HOOK_SUCCESS,
  HOOK_BREAK
};

typedef int (*hook_callback_proc) (void *__callData);

int
hook_register (const wchar_t *__name, hook_callback_proc __callback,
               int __priority);

int
hook_unregister (const wchar_t *__name);

int
hook_call (const wchar_t *__name, void *__data);

int
hook_unhook (const wchar_t *__name, hook_callback_proc __callback);

int
hooks_destroy (void);

int
hooks_init (void);

END_HEADER

#endif
