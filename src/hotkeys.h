/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Stuff providing support of hotkeys
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _hotkeys_h_
#define _hotkeys_h_

#include "smartinclude.h"

BEGIN_HEADER

#include <wchar.h>

typedef void (*hotkey_callback) (void);

/* Register a hot-key */
short
hotkey_register (const wchar_t *__sequence, hotkey_callback __callback);

/* Release registered hot-key */
void
hotkey_release (const wchar_t *__sequence);

/* Put new character to sequence */
short
hotkey_push_character (wchar_t __ch);

END_HEADER

#endif
