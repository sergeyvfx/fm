/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Different stuff providing file-related operations
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "file.h"

#include <wchar.h>

////////
//

#define FILE_TRUNCATOR '~'

////////
//

/**
 * Fits filename to specified length
 *
 * @param __file_name - file name to be fited
 * @param __len - length to which fit the file name
 * @param __res - pointer to buffer where result will be stored
 */
void
fit_filename                      (const wchar_t *__file_name,
                                   long           __len,
                                   wchar_t       *__res)
{
  size_t len;
  if ((len=wcslen (__file_name))<=__len)
    {
      // Do not call any specific stuff
      wcscpy (__res, __file_name);
  } else {
    // Cast truncation
    size_t i, t=__len/2-(__len%2?0:1), ptr=0;

    // Copy prefix
    for (i=0; i<t; i++)
      __res[ptr++]=__file_name[i];

    __res[ptr++]=FILE_TRUNCATOR;

    for (i=len-__len+ptr; i<len; i++)
      __res[ptr++]=__file_name[i];

    __res[ptr]=0;
  }
}

/**
 * Strip directory prefix from file name
 *
 * @param __name - name of file from which strip directory prefix
 * @return pointer to new file name, which must be freed and
 * zero in case of errors
 */
wchar_t*
wcfilename                        (const wchar_t *__name)
{
  size_t i, len=wcslen (__name), ptr=0;
  wchar_t *res;
  
  if (!__name)
    return 0;

  if (!wcscmp (__name, L"/"))
    return wcsdup (L"/");
  
  // Search last '/'
  for (i=len-1; i>=0; i--)
    if (__name[i]=='/')
      break;

  // Allocate memory for new filename
  res=malloc (sizeof (wchar_t)*(len-i+1));

  // Copy data to new filename
  i++;
  while (i<len)
    res[ptr++]=__name[i++];

  res[ptr]=0;
  
  return res;
}

/**
 * Converts file mode creation mask to string
 *
 * @param __mask - mode creation mask to be converted to string
 * @param __res - pointer to string where save result of converting
 */
void
umasktowcs                        (mode_t __mask, wchar_t *__res)
{
  if (!__res)
    return;

  // Permissions for owner
  __res[0]=__mask&S_IRUSR?'r':'-';
  __res[1]=__mask&S_IWUSR?'w':'-';
  __res[2]=__mask&S_IXUSR?'x':'-';

  // Permissions for group
  __res[3]=__mask&S_IRGRP?'r':'-';
  __res[4]=__mask&S_IWGRP?'w':'-';
  __res[5]=__mask&S_IXGRP?'x':'-';

  // Permissions for ootthers
  __res[6]=__mask&S_IROTH?'r':'-';
  __res[7]=__mask&S_IWOTH?'w':'-';
  __res[8]=__mask&S_IXOTH?'x':'-';
  
  // Null-terminator
  __res[9]=0;
}
