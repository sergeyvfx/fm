/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Different MACRO definitions
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _macrodef_h_
#define _macrodef_h_

BEGIN_HEADER

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#define SAFE_FREE(__a) \
  if (__a) { free (__a); __a=0; }

#define MALLOC_ZERO(__ptr,__size) \
  { \
    __ptr=malloc (__size); \
    memset (__ptr, 0, __size); \
  }

#ifndef MIN
#  define MIN(__a,__b) \
  ((__a)<(__b)?(__a):(__b))
#endif

#ifndef MAX
#  define MAX(__a,__b) \
  ((__a)>(__b)?(__a):(__b))
#endif

#define PACK_ARGS(__text,__buf,__size) \
  va_list ap;\
  wcscpy (__buf, L"");\
  va_start (ap, __text);\
  vswprintf (__buf, __size, __text, ap); \
  va_end (ap);

#define SET_FLAG(__flags, __f)   (__flags)|=(__f)
#define TEST_FLAG(__flags, __f)  ((__flags)&(__f))
#define CLEAR_FLAG(__flags, __f) ((__flags)&=~(__f))

#define MBS2WCS(_res,_src) \
  { \
    size_t len=strlen (_src); \
    _res=malloc (sizeof (wchar_t)*(len+2)); \
    mbstowcs (_res, _src, len+1); \
  }

/* Length of static wide-string buffer */
#define BUF_LEN(_buf) \
  sizeof (_buf)/sizeof (wchar_t)

END_HEADER

#endif
