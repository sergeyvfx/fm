/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Messages' displaying stuff
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _messages_h_
#define _messages_h_

#include "smartinclude.h"
#include "widget.h"

BEGIN_HEADER

#include <wchar.h>

//////
// Constants

// Look&feel
#define MB_CRITICAL        0x0001

// Button sets
#define MB_OK              0x0000
#define MB_OKCANCEL        0x0100
#define MB_YESNO           0x0200
#define MB_YESNOCANCEL     0x0300
#define MB_RETRYCANCEL     0x0400
#define MB_RETRYSKIPCANCEL 0x0500
#define MB_RETRYIGNCANCEL  0x0600

// Determines an index of initially focused button
#define MB_DEFBUTTON_0     0x0000
#define MB_DEFBUTTON_1     0x1000
#define MB_DEFBUTTON_2     0x2000

// Some helpers for deep-core usage
#define MB_BUTTON_CODE(_x)   ((_x>>8)%16%7)
#define MB_DEFBUTTON(_x)     ((_x>>12)%4%3)

#define MESSAGE_ERROR(_text) \
  message_box (_(L"Error"), _(_text), MB_OK|MB_CRITICAL);

//////
//

int            // Show a message box
message_box                       (const wchar_t *__caption,
                                   const wchar_t *__text,
                                   unsigned int   __flags);

END_HEADER

#endif
