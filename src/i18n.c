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

#include "i18n.h"

#include <libintl.h>
#include <locale.h>
#include <stdlib.h>
#include <wchar.h>

typedef struct
{
  wchar_t *origin;
  wchar_t *localize;
} i18n_string;

typedef struct
{
  size_t size;
  i18n_string *strings;
} i18n_list;

static i18n_list localization;

/**
 * Initialize localization list
 *
 * @param __list a pointer to localization list
 */
static void
i18n_list_alloc (i18n_list *__list)
{
  if (!__list)
    {
      return;
    }

  __list->size = 0;
  __list->strings = NULL;
}

/**
 * Free resource of localization list
 *
 * @param __list a pointer to localization list
 * @return zero on success, non-zero on failure
 */
static int
i18n_list_free (i18n_list *__list)
{
  __s64_t i;

  if (!__list)
    {
      return -1;
    }

  if (__list->strings)
    {
      for (i = 0; i < __list->size; ++i)
        {
          SAFE_FREE (__list->strings[i].origin);
          SAFE_FREE (__list->strings[i].localize);
        }
      SAFE_FREE (__list->strings);
    }
  __list->size = 0;

  return 0;
}

/**
 * Add new string into list
 *
 * @param __list a pointer to list localization
 * @param __origin a original text
 * @param __localize a localized text
 * @return pointer to added string
 */
static i18n_string *
i18n_list_push_back (i18n_list *__list,
                     const wchar_t *__origin, const wchar_t *__localize)
{
  __s64_t i, j;

  for (i = 0; i < __list->size; ++i)
    {
      if (wcscmp (__list->strings[i].origin, __origin) < 0)
        {
          break;
        }
    }

  ++__list->size;
  __list->strings = (i18n_string *) realloc (__list->strings,
                                             sizeof (i18n_string) *
                                                 __list->size);

  if (i != __list->size - 1)
    {
      for (j = __list->size - 1; j > i; --j)
        {
          __list->strings[j] = __list->strings[j - 1];
        }
    }

  __list->strings[i].origin = wcsdup (__origin);
  __list->strings[i].localize = wcsdup (__localize);

  return __list->strings + i;
}

/**
 * Binary search in localization list
 *
 * @param __list a pointer to localization list
 * @param __text a text to search for
 * @return index of required text or -1 if text is not found
 */
static __s32_t
i18n_list_binary_search (const i18n_list *__list, const wchar_t *__text)
{
  __s64_t first = 0, last = (__s64_t) __list->size - 1, middle;
  __s32_t retval;

  if (!__list)
    {
      return -1;
    }

  while (first <= last)
    {
      middle = (first + last) / 2;
      retval = wcscmp (__list->strings[middle].origin, __text);

      if (!retval)
        {
          return middle;
        }
      else
        {
          if (retval < 0)
            {
              last = middle - 1;
            }
          else
            {
              first = middle + 1;
            }
        }
    }

  return -1;
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

  /* init gettext */
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  i18n_list_alloc (&localization);
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
  i18n_string *string;
  __s64_t gtext_len, mbtext_len;
  __s32_t retval;
  wchar_t *wtext;
  char *mbtext, *gtext;

  if ((retval = i18n_list_binary_search (&localization, __text)) != -1)
    {
      return localization.strings[retval].localize;
    }

  mbtext_len = ((wcslen (__text) + 1) * MB_CUR_MAX);
  mbtext = (char *) malloc (mbtext_len);
  wcstombs (mbtext, __text, mbtext_len);

  gtext = gettext (mbtext);
  gtext_len = strlen (gtext) + 1;

  wtext = (wchar_t *) malloc (sizeof (wchar_t) * gtext_len);
  mbstowcs (wtext, gtext, gtext_len);

  string = i18n_list_push_back (&localization, __text, wtext);

  SAFE_FREE (wtext);
  SAFE_FREE (mbtext);

  return string->localize;
}

/**
 * Cleanup localization stuff
 */
void
i18n_release (void)
{
  i18n_list_free (&localization);
}
