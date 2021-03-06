/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of different actions related to iface
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "iface.h"
#include "messages.h"
#include "i18n.h"
#include "util.h"

/********
 * User's backend
 */

/**
 * Prompted exiting from program
 */
void
iface_act_exit (void)
{
  if (message_box (L"fm", _(L"Are you sure you want to quit?"),
                   MB_YESNO | MB_DEFBUTTON_1) == MR_YES)
    {
      do_exit ();
    }
}
