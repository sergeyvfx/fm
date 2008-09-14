/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Timers' oriented stuff
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _timer_h_
#define _timer_h_

#include "smartinclude.h"

BEGIN_HEADER

#include "util.h"

/**
 * Check is timeval __to is older than __from not less than __delta usecs
 */
#define CHECK_TIME_DELTA(__from, __to, __delta) \
  (tv_usec_cmp (timedist (__from, __to), __delta) > 0)

/**
 * Call specified action if __time_delta usecs elapsed from __last_timestamp
 *
 * NOTE: __last_timestamp will be updated after __proc is called
 */
#define CALL_DELAYED(__last_timestamp, __time_delta, __proc, __args...) \
  { \
    if (CHECK_TIME_DELTA (__last_timestamp, now (), __time_delta)) \
      { \
        __proc (__args); \
        __last_timestamp = now (); \
      } \
  }

END_HEADER

#endif
