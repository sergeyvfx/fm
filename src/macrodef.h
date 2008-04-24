/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Deifferent MACRO defenitions
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _macrodef_h_
#define _macrodef_h_

#include "smartinclude.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>

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
  strcpy (__buf, "");\
  va_start (ap, __text);\
  vsnprintf (__buf, __size, __text, ap); \
  va_end (ap);

#define SET_FLAG(__flags, __f)   (__flags)|=(__f)
#define TEST_FLAG(__flags, __f)  ((__flags)&(__f))
#define CLEAR_FLAG(__flags, __f) ((__flags)&=~(__f))

#endif
