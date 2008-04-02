/*
 *
 * =============================================================================
 *  types.h
 * =============================================================================
 *
 *  Written (by Nazgul) under General Public License.
 *
*/

#ifndef _types_h_
#define _types_h_

#include "smartinclude.h"

#ifdef LINUX
#  include <sys/types.h>
#endif

#ifndef BOOL

#  define BOOL int

#  ifndef FALSE
#    define FALSE 0
#  endif

#  ifndef TRUE
#    define TRUE 1
#  endif

#endif

#ifndef DWORD
#  define DWORD unsigned long long
#endif

typedef unsigned char      __u8_t;
typedef unsigned int       __u16_t;
typedef unsigned long      __u32_t;
typedef unsigned long long __u64_t;

typedef struct timeval timeval_t;

#endif
