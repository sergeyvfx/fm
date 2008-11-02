/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Internationalization and localization stuff
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "i18n.h"

#include <libintl.h>
#include <locale.h>
#include <stdlib.h>
#include <wchar.h>

#include "util.h"
#include "hashmap.h"

static hashmap_t *localization = NULL;

void
i18n_localization_deleter (void *__data)
{
  free ((wchar_t *) __data);
}

/**
 * Initialize localization stuff
 */
void
i18n_init (void)
{

#ifdef NOINST_DEBUG
#  undef LOCALEDIR
#  define LOCALEDIR "../po/locale/"
#endif

  /* Use the default locale set in the environment */
  setlocale (LC_ALL, "");

  /* Initialize gettext stuff */
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  localization = hashmap_create_wck (i18n_localization_deleter, HM_MAGICK_LEN);
}

/**
 * Return a localized text
 * Gettext wrapper
 *
 * @param __text a natural text
 * @return localized text
 */
wchar_t *
i18n_text (const wchar_t *__text)
{
  wchar_t *localize = (wchar_t *) hashmap_get (localization, __text);
  char    *mbtext, *gtext;

  if (localize != NULL)
    {
      return localize;
    }

  wcs2mbs (&mbtext, __text);
  gtext = gettext (mbtext);
  mbs2wcs (&localize, gtext);

  SAFE_FREE (mbtext);

  hashmap_set (localization, (void *) wcsdup (__text), localize);
  return localize;
}

/**
 * Cleanup localization stuff
 */
void
i18n_release (void)
{
  hashmap_destroy (localization);
  localization = NULL;
}
