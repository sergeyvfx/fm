/*
 *
 * =============================================================================
 *  macrodef.h
 * =============================================================================
 *
 *  Deifferent MACRO defenitions
 *
 *  Written (by Nazgul) under GPL
 *
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

#endif
