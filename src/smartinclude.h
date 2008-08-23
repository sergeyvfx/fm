/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Smart including of headers
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _smart_include_h_
#define _smart_include_h_

/*
  This macroses are used in header files to ensure that the declarations
  within are properly encapsulated in an `extern "C" { .. }` block when
  included from a  C++ compiler.
 */
#ifdef __cplusplus
#  define BEGIN_HEADER extern "C" {
#  define END_HEADER }
#else
#  define BEGIN_HEADER
#  define END_HEADER
#endif

BEGIN_HEADER

#include <config.h>

#include "build-stamp.h"
#include "package.h"
#include "version.h"

/* There is some strange errors with using SIGWINCH when */
/* _XOPEN_SOURCE is defined. So, if your module needs */
/* SIGWINCH you need define NEED_SIGWINCH before includeing */
/* this header file. */
#ifndef NEED_SIGWINCH
#  define _XOPEN_SOURCE 500
#endif

#include "macrodef.h"

/* Some definitions to tell compiler use specified stuff */
#define __USE_GNU
#define __USE_ISOC99
#define __USE_FILE_OFFSET64 1
#define USE_WIDEC_SUPPORT
#define _XOPEN_SOURCE_EXTENDED

/* Attribute for unused parameter to avoid */
/* compilator's waringns */
#ifdef HAVE__ATTRIBUTE__
#  define ATTR_UNUSED __attribute__((unused))
#else
#  define ATTR_UNUSED
#endif

#include "types.h"

END_HEADER

#endif
