/*
 *
 * ================================================================================
 *  messages.h
 * ================================================================================
 *
 *  Written (by Nazgul) under General Public License.
 *
*/

#ifndef _messages_h_
#define _messages_h_

#include "smartinclude.h"

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

#define MB_DEFBUTTON_0     0x0000
#define MB_DEFBUTTON_1     0x1000
#define MB_DEFBUTTON_2     0x2000

// Some helpers for deep-core usage
#define MB_BUTTON_CODE(_x)   ((_x>>8)%16%6)
#define MB_DEFBUTTON(_x)     ((_x>>12)%4%3)

//////
//

int
message_box                       (wchar_t *__caption, wchar_t *__text, unsigned int __flags);

#endif
