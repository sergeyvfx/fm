/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "iface.h"
#include "screen.h"

#include <widgets/widget.h>

static BOOL disabled = FALSE;

/**
 * Pause console screen handler stuff
 *
 * May be useful for stuff like process launching
 */
void
iface_screen_disable (void)
{
  disabled = TRUE;
  endwin ();
}

/**
 * Resume console screen handler stuff
 */
void
iface_screen_enable (void)
{
  screen_on_resize ();
  widget_on_scr_resize ();
  disabled = FALSE;
}

/**
 * Chec if screen is paused
 */
BOOL
iface_screen_disabled (void)
{
  return disabled;
}
