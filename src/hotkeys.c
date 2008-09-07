/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Stuff providing support of hotkeys
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "hotkeys.h"
#include "screen.h"
#include "deque.h"

#include <stdarg.h>

/*******
 *
 */

/* Length of maximum sequence for hotkey */
#define MAX_SEQUENCE_LENGTH  16

#define GET_REGISTRATION(_data) \
  { \
    va_list args; \
    va_start (args, __callback); \
    (_data)=va_arg (args, void*); \
    va_end (args); \
  }

/* Queue of incoming characters */
static wchar_t queue[MAX_SEQUENCE_LENGTH] = {0};
static short queue_ptr = 0;
hotkey_context_t *hotkey_root_context;

/*
 * TODO: We'd better use some type of hashing to make finding a hotkey faster.
 */

typedef struct
{
  wchar_t *sequence;

  /* Length of sequence*/
  short length;

  /* Data passed to hotkey registration function */
  void *reg_data;

  hotkey_callback callback;
} hotkey_t;

struct hotkey_context
{
  /* List of hotkeys' descriptors */
  hotkey_t *hotkeys;

  /* Count of hotkeys */
  int count;

  /* Different flags of context */
  unsigned int flags;
};

/* Stack of contexts */
static deque_t *contexts = NULL;

/********
 *
 */

/**
 * Check if sequence if a hot-key
 *
 * @param __sequence - pointer to sequence's descriptor
 * @param __len - length of sequence
 * @return non-zero if sequence is a hot-key
 */
static short
check_iterator (const wchar_t *__sequence, short __len)
{
  short i = 0, j = queue_ptr - __len;

  /* Just compare all characters in sequence with characters from queue*/
  for (i = 0; i < __len; i++)
    {
      if (__sequence[i] != queue[j++])
        {
          return 0;
        }
    }

  return 1;
}

/**
 * Check for a hot-key at the tail queue
 * If hot-key found, calls a callback
 *
 * @return non-zero if hot-key's callback found
 */
static short
check (void)
{
  int i;
  hotkey_context_t *context;

  deque_foreach (contexts, context);
    /* Go through all registered hot-keys and check if any is in queue */
    for (i = 0; i < context->count; i++)
      {
        if (check_iterator (context->hotkeys[i].sequence,
                            context->hotkeys[i].length))
          {
            /* Hot-key sequence is in queue. */
            context->hotkeys[i].callback (context->hotkeys[i].reg_data);
            return 1;
          }
      }

    if (context->flags & HKCF_OPAQUE)
      {
        /* We shouldn't ovweview parent contexts */
        deque_foreach_break;
      }
  deque_foreach_done

  return 0;
}

/**
 * Parse a key sequence into a numeric array of codes
 *
 * @param __sequence - sequence to parse
 * @param __res - pointer to array where to write result of parsing
 * @return length of sequence if succeed, -1 otherwise
 */
static short
parse_sequence (const wchar_t *__sequence, wchar_t *__res)
{
  short i = 0, n = wcslen (__sequence), len = 0;
  BOOL ctrl, alt;
  wchar_t dummy;

  while (i < n)
    {
      /* Sequence is too long */
      if (len >= MAX_SEQUENCE_LENGTH)
        {
          return -1;
        }

      /* Reset state */
      ctrl = alt = FALSE;

      /***
       * Parse optional prefixes
       */

      /* Alt */
      if (i < n - 1 && __sequence[i] == 'M' && __sequence[i + 1] == '-')
        {
          if (__sequence[i + 2])
            {
              i += 2;
              alt = TRUE;
            }
          else
            {
              /* Invalid META prefix */
              return -1;
            }
        }

      /* Control */
      if (i < n - 1 && __sequence[i] == 'C' && __sequence[i + 1] == '-')
        {
          if (__sequence[i + 2])
            {
              i += 2;
              ctrl = TRUE;
            }
          else
            {
              /* Invalid CONTROL prefix */
              return -1;
            }
        }

      /***
       * Check for function-key
       */
      if (i < n - 1 && __sequence[i] == 'F' &&
          __sequence[i + 1] >= '0' && __sequence[i + 1] <= '9')
        {
          short f = 0;
          i++;
          while (i < n && __sequence[i] >= '0' && __sequence[i] <= '9')
            {
              f = f * 10 + __sequence[i++] - '0';
            }

          dummy = KEY_F (f);
        }
      else
        {
          /* Simple character */
          dummy = __sequence[i++];
        }

      /***
       * Apply prefixes
       */
      if (ctrl)
        {
          dummy = CTRL (dummy);
        }

      if (alt)
        {
          dummy = ALT (dummy);
        }

      __res[len++] = dummy;

      /* Skip spaces */
      while (i < n && __sequence[i] <= ' ')
        {
          i++;
        }
    }

  return len;
}

/********
 * User's backend
 */

/**
 * Initialize hotkeys' stuff
 *
 * @return zero on success, non-zero otherwise
 */
int
hotkeys_init (void)
{
  /* Create stack of contexts */
  contexts = deque_create ();

  /* Create global context */
  hotkey_root_context = hotkey_create_context (HKCF_ACTIVE);

  return 0;
}

/**
 * Uninitialize hotkeys' stuff
 */
void
hotkeys_done (void)
{
  /* Destroy contexts */
  deque_destroy (contexts, (destroyer)hotkey_destroy_context);
}

/**
 * Create context of hotkeys
 *
 * @param __flags - flags of context
 * @return descriptor of new context
 */
hotkey_context_t*
hotkey_create_context (unsigned int __flags)
{
  hotkey_context_t *result;

  MALLOC_ZERO (result, sizeof (hotkey_context_t));

  result->flags = __flags;

  if (__flags & HKCF_ACTIVE)
    {
      /* If context is active, we should */
      /* push it into stack */
      hotkey_push_context (result);
    }

  return result;
}

/**
 * Destroy context of hotkeys
 *
 * @param __context - context to be destroyed
 */
void
hotkey_destroy_context (hotkey_context_t *__context)
{
  int i;

  if (!__context)
    {
      return;
    }

  /* Destroy list of hotkeys */
  for (i = 0; i < __context->count; ++i)
    {
      free (__context->hotkeys[i].sequence);
    }
  free (__context->hotkeys);

  /* Free momory used by context descriptor */
  free (__context);
}

/**
 * Push hotkeys context to stack of contexts
 *
 * @param __context - context to push
 */
void
hotkey_push_context (hotkey_context_t *__context)
{
  if (!__context)
    {
      return;
    }

  deque_push_front (contexts, __context);
}

/**
 * Pop hotkeys context from stack of contexts
 *
 * @param __destroy - if TRUE, context will be destroyed
 * @return pop-ed context if not destroyed, NULL otherwise
 */
hotkey_context_t*
hotkey_pop_context (BOOL __destroy)
{
  hotkey_context_t *context;

  context = deque_pop_front (contexts);

  if (__destroy)
    {
      hotkey_destroy_context (context);
      context = NULL;
    }

  return NULL;
}

/**
 * Drop specified by descriptor context from stack
 *
 * @param __context - descriptor of context to drop
 * @param __destroy - destroy after dropping
 */
void
hotkey_drop_context (hotkey_context_t *__context, BOOL __destroy)
{
  hotkey_context_t *context;
  if (!__context)
    {
      return;
    }

  deque_foreach (contexts, context)
    if (context == __context)
      {
        deque_remove (contexts, deque_foreach_iterator,
                      __destroy ? (destroyer)hotkey_destroy_context : 0);
        deque_foreach_break;
      }
  deque_foreach_done
}

/**
 * Set current context (context at the head of stack)
 *
 * @param __context - new current context descriptor
 */
void
hotkey_set_current_context (hotkey_context_t *__context)
{
  /* We should enshure that there is no */
  /* such context in stack */
  hotkey_drop_context (__context, FALSE);

  hotkey_push_context (__context);
}

/**
 * Register a hot-key at specified context
 *
 * @param __context - descriptor of context where hotkey will be registered
 * @param __sequence - hot-key sequence
 * @param __callback - callback to be called when hot-key sequence is pressed
 * @param __reg_data - registration data
 * @return zero on success, non-zero otherwise
 */
int
hotkey_register_at_context_full (hotkey_context_t *__context,
                                 const wchar_t *__sequence,
                                 hotkey_callback __callback, void *__reg_data)
{
  wchar_t dummy[MAX_SEQUENCE_LENGTH];
  short len;

  if (!__context || !__sequence)
    {
      return -1;
    }

  /* Prepare sequence */
  if ((len = parse_sequence (__sequence, dummy)) > 0)
    {
      hotkey_t *hotkey;

      /* Sequence is good, so we can allocate new hot-key descriptor */
      /* and fill it in. */
      __context->hotkeys = realloc (__context->hotkeys,
                                    sizeof (hotkey_t) *
                                      (__context->count + 1));

      hotkey = &__context->hotkeys[__context->count];
      hotkey->sequence = malloc (sizeof (wchar_t) * len);
      memcpy (hotkey->sequence, dummy, len * sizeof (wchar_t));
      hotkey->length = len;
      hotkey->reg_data = __reg_data;
      hotkey->callback = __callback;

      ++__context->count;
      return 0;
    }

  return -1;
}

/**
 * Register a hot-key at specified context
 *
 * @param __context - descriptor of context where hotkey will be registered
 * @param __sequence - hot-key sequence
 * @param __callback - callback to be called when hot-key sequence is pressed
 * @return zero on success, non-zero otherwise
 */
int
hotkey_register_at_context (hotkey_context_t *__context,
                            const wchar_t *__sequence,
                            hotkey_callback __callback)
{
  return hotkey_register_at_context_full (__context, __sequence,
                                          __callback, NULL);
}

/**
 * Register a hotkey at context from head of stack
 *
 * @param __sequence - hot-key sequence
 * @param __callback - callback to be called when hot-key sequence is pressed
 * @param __reg_data - registration data
 * @return zero on success, non-zero otherwise
 */
int
hotkey_register_full (const wchar_t *__sequence, hotkey_callback __callback,
                      void *__reg_data)
{
  hotkey_context_t *context;

  if (!contexts)
    {
      return -1;
    }

  context = (hotkey_context_t*)deque_data (deque_head (contexts));
  return hotkey_register_at_context_full (context, __sequence,
                                          __callback, __reg_data);
}

/**
 * Register a hotkey at context from head of stack
 *
 * @param __sequence - hot-key sequence
 * @param __callback - callback to be called when hot-key sequence is pressed
 * @return zero on success, non-zero otherwise
 */
int
hotkey_register (const wchar_t *__sequence, hotkey_callback __callback)
{
  return hotkey_register_full (__sequence, __callback, NULL);
}

/**
 * Release registered hot-key from specified context
 *
 * @param __context - descriptor of context from where hotkey
 * will be unregistered
 * @param __sequence - hot-key sequence to realise
 */
void
hotkey_release_from_context (hotkey_context_t *__context,
                             const wchar_t *__sequence)
{
  wchar_t dummy[MAX_SEQUENCE_LENGTH];
  short len;

  if (!__context || !__sequence)
    {
      return;
    }

  if ((len = parse_sequence (__sequence, dummy)) > 0)
    {
      short i, j;
      BOOL eq;

      /* Go through all registered hot-keys and */
      /* compare with sequence to realise. */
      for (i = 0; i < __context->count; i++)
        {
          /* Check is sequences are equal */
          eq = TRUE;
          for (j = 0; j < len; j++)
            {
              if (__context->hotkeys[i].sequence[j] != dummy[j])
                {
                  eq = FALSE;
                  break;
                }
            }

          if (eq)
            {
              /* Sequences are equal, so just destroy it. */
              free (__context->hotkeys[i].sequence);

              /* Shift registered hotkeys */
              for (j = i; j < __context->count; j++)
                {
                  __context->hotkeys[i] = __context->hotkeys[i + 1];
                }

              --__context->count;
              __context->hotkeys = realloc (__context->hotkeys,
                                            __context->count *
                                              sizeof (hotkey_t));

              break;
            }
        }
    }
}

/**
 * Release a hot-key from context at head of stack
 *
 * @param __sequence - hot-key sequence to realise
 */
void
hotkey_release (const wchar_t *__sequence)
{
  hotkey_context_t *context;

  if (!contexts)
    {
      return;
    }

  context = (hotkey_context_t*)deque_data (deque_head (contexts));
  hotkey_release_from_context (context, __sequence);
}

/**
 * Put new character to sequence
 *
 * @param __ch - character to put
 * @return non-zero if hot-key has been accepted
 */
short
hotkey_push_character (wchar_t __ch)
{
  if (queue_ptr > MAX_SEQUENCE_LENGTH - 1)
    {
      /* Shift characters in queue */
      short i;

      for (i = 0; i < MAX_SEQUENCE_LENGTH - 1; i++)
        {
          queue[i] = queue[i + 1];
        }

      queue_ptr = MAX_SEQUENCE_LENGTH - 1;
    }

  /* Store character in queue */
  queue[queue_ptr++] = __ch;

  return check ();
}
