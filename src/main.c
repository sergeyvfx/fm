/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Main implementation file of this project
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "iface.h"
#include "i18n.h"

#include <stdio.h>
#include <stdlib.h>

int
main                               (int __argc, char **__argv)
{
  i18n_init();

  if (iface_init ()) {
      i18n_release();
      return EXIT_FAILURE;
  }

  iface_done ();
  i18n_release();

  return EXIT_SUCCESS;
}
