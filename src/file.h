/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Different stuff providing file-related operations
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _file_h_
#define _file_h_

#include "smartinclude.h"

BEGIN_HEADER

#include <sys/stat.h>
#include <time.h>

/********
 * Constants
 */

#define MAX_FILENAME_LEN  256

/********
 * Type definitions
 */

typedef struct
{
  /***
   * Inherited from dirent
   */

  /* Name of file */
  wchar_t name[MAX_FILENAME_LEN];

  /* Type of file */
  unsigned char type;

  /* Stat information of file */
  struct stat stat;

  /* L-Stat information of file */
  struct stat lstat;
} file_t;

/********
 *
 */

/* Fit filename to specified length */
void
fit_filename (const wchar_t *__file_name, size_t __len, wchar_t *__res);

/* Strip directory prefix from file name */
wchar_t*
wcfilename (const wchar_t *__name);

/* Convert file mode creation mask to string */
void
umasktowcs (mode_t __mask, wchar_t *__res);

/* Convert file size to human-readable format */
#ifdef __USE_FILE_OFFSET64
__u64_t
fsizetohuman (__u64_t __size, wchar_t *__suffix);
#else
__u32_t
fsizetohuman (__u32_t __size, wchar_t *__suffix);
#endif

void
format_file_time (wchar_t *__buf, size_t __buf_size, time_t __time);

int
filename_compare (const wchar_t *__a, const wchar_t *__b);

END_HEADER

#endif
