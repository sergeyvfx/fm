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

#include <stdio.h>

int
main                               (int __argc, char **__argv)
{
  if (iface_init ())
      // Non-zero exit status - some error :(
      return 1;

  iface_done ();
  
  return 0;
}
