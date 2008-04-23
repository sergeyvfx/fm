/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Internationalization and localization stuff
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _i18n_h_
#define _i18n_h_

#include "smartinclude.h"

#define _(x) i18n_text(x)

void
i18n_init (void);

void
i18n_release (void);

wchar_t *
i18n_text (const wchar_t *);

#endif
