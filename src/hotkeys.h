/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Stuff providing support of hotkeys
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _hotkeys_h_
#define _hotkeys_h_

#include "smartinclude.h"

BEGIN_HEADER

#include <wchar.h>

/********
 *
 */

/* Context will be pushed to stack of contexts */
#define HKCF_ACTIVE 0x0001

/* Context is 'opaque'. it means that while registered hotkey is searching */
/* it will be the last reviewing context */
#define HKCF_OPAQUE 0x0002

/* Do not append context to hashmap of contexts */
#define HKCF_NOTINMAP 0x0004

/********
 * Type definitions
 */
typedef int (*hotkey_callback) (void *__reg_data);

struct hotkey_context;
typedef struct hotkey_context hotkey_context_t;

/********
 *
 */

/* Initialize hotkeys' stuff */
int
hotkeys_init (void);

/* Uninitialize hotkeys' stuff */
void
hotkeys_done (void);

/* Create context of hotkeys */
hotkey_context_t*
hotkey_create_context (const wchar_t *__name, unsigned int __flags);

/* Destroy context of hotkeys */
void
hotkey_destroy_context (hotkey_context_t *__context);

/* Push hotkeys context to stack of contexts */
void
hotkey_push_context (hotkey_context_t *__context);

/* Pop hotkeys context from stack of contexts */
hotkey_context_t*
hotkey_pop_context (BOOL __destroy);

/* Drop context from stack */
void
hotkey_drop_context (hotkey_context_t *__context, BOOL __destroy);

/* Set current context (context at the head of stack) */
void
hotkey_set_current_context (hotkey_context_t *__context);

/* Register a hot-key at specified context */
int
hotkey_register_at_context (hotkey_context_t *__context,
                            const wchar_t *__sequence,
                            hotkey_callback __callback);

/* Register a hot-key at specified context */
int
hotkey_register_at_context_full (hotkey_context_t *__context,
                                 const wchar_t *__sequence,
                                 hotkey_callback __callback,
                                 void *__reg_data);

/* Register a hotkey at context from head of stack */
int
hotkey_register (const wchar_t *__sequence, hotkey_callback __callback);

/* Register a hotkey at context from head of stack */
int
hotkey_register_full (const wchar_t *__sequence, hotkey_callback __callback,
                      void *__reg_data);

/* Release registered hot-key from specified context */
void
hotkey_release_from_context (hotkey_context_t *__context,
                             const wchar_t *__sequence);

/* Release registered hot-key from context at head of stack */
void
hotkey_release (const wchar_t *__sequence);

/* Put new character to sequence */
short
hotkey_push_character (wchar_t __ch);

/* Get current hotkey context */
hotkey_context_t*
hotkey_current_context (void);

/* Bind a hotkey at context */
int
hotkey_bind (const wchar_t *__context_name,
             const wchar_t *__sequence, hotkey_callback __callback);

/* Bind a hotkey at context */
int
hotkey_bind_full (const wchar_t *__context_name,
                  const wchar_t *__sequence, hotkey_callback __callback,
                  void *__reg_data);

END_HEADER

#endif
