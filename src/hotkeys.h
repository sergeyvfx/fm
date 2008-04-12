/*
 *
 * =============================================================================
 *  hotkeys.h
 * =============================================================================
 *
 *  Written (by Nazgul) under General Public License.
 *
*/

#ifndef _hotkeys_h_
#define _hotkeys_h_

#include "smartinclude.h"

#include <wchar.h>

typedef void (*hotkey_callback)  (void);

short          // Register a hot-key
hotkey_register                   (wchar_t *__sequence,
                                   hotkey_callback __callback);

void           // Realise registered hot-key
hotkey_release                    (wchar_t *__sequence);

short          // Put new character to sequence
hotkey_push_character             (wchar_t __ch);

#endif
