/*
 *
 * =============================================================================
 *  smartinclude.h
 * =============================================================================
 *
 *  Smart including of headers
 *
 *  Written (by Nazgul) under GPL
 *
*/

#ifndef _smart_include_h_
#define _smart_include_h_

#include <config.h>

#include "build-stamp.h"
#include "package.h"
#include "version.h"

#include "types.h"
#include "macrodef.h"

#define __USE_GNU
#define __USE_ISOC99
#define USE_WIDEC_SUPPORT
#define _XOPEN_SOURCE_EXTENDED

#ifdef HAVE__ATTRIBUTE__
#  define ATTR_UNUSED __attribute__((unused))
#else
#  define ATTR_UNUSED
#endif

#endif
