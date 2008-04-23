/*
 *
 * =============================================================================
 *  main.c
 * =============================================================================
 *
 *  Written (by Nazgul) under General Public License.
 *
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
