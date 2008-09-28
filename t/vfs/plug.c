/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Some plugs to prevent liker's errors
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include <smartinclude.h>

void
iface_screen_lock (void)
{
}

void
iface_screen_unlock (void)
{
}

int
hook_call (wchar_t *__unused_name ATTR_UNUSED,
           void *__unused_data ATTR_UNUSED)
{
  return 0;
}
