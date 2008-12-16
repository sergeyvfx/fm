/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation of find file operation
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _ACTION_FIND_H_
#define _ACTION_FIND_H_

#include <smartinclude.h>

BEGIN_HEADER

#include "regexp.h"

#define AFF_MASK_REGEXP            0x0001
#define AFF_MASK_CASE_SENSITIVE    0x0002
#define AFF_CONTENT_REGEXP         0x0004
#define AFF_CONTENT_CASE_SENSITIVE 0x0008
#define AFF_FIND_RECURSIVELY       0x0010
#define AFF_FOLLOW_SYMLINKS        0x0020
#define AFF_FIND_DIRECTORIES       0x0040

typedef struct
{
  BOOL filled;

  /* Initial options */
  wchar_t *file_mask;

  wchar_t *content;
  wchar_t *start_at;

  unsigned long flags;

  /* Precompiled options */
  regexp_t **re_file;
  int re_file_count;
  regexp_t *re_content;

  char *mb_content;
} action_find_options_t;

END_HEADER

#endif
