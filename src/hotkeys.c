/*
 *
 * =============================================================================
 *  hotkeys.c
 * =============================================================================
 *
 *  Written (by Nazgul) under General Public License.
 *
*/

#include "hotkeys.h"
#include "screen.h"

////////
//

// Length of maximum sequence for hotkey
#define MAX_SEQUENCE_LENGTH  3

// Queue of incoming characters
static wchar_t queue[MAX_SEQUENCE_LENGTH]={0};
static short queue_ptr=0;

// List of all registered hot-keys

//
// TODO:
//  We'd better use some typy of hashing to make
//  findning a hotkey faster.
//

static struct {
  wchar_t         *sequence;
  short           length;     // Length of sequence
  hotkey_callback callback;
} *hotkeys=0;

static short hotkey_count=0;

////////
//

/**
 * Checks if sequence if a hot-key
 *
 * @param __sequence - pointer to sequence's descriptor
 * @param __len - length of sequence
 *
 * @return non-zero if sequence is a hot-key
 */
static short
check_iterator                    (wchar_t *__sequence, short __len)
{
  short i=0, j=queue_ptr-__len;

  // Just compare all characters in sequence with
  // characters from queue
  for (i=0; i<__len; i++)
    if (__sequence[i]!=queue[j++]) {
      return 0;
    }

  return 1;
}

/**
 * Checks for a hot-key at the tail queue
 * If hot-key found, calls a callback
 *
 * @return non-zero if hot-key's callback found
 */
static short
check                             (void)
{
  short i;

  // Go through all registered hot-keys and
  // check if any is in queue
  for (i=0; i<hotkey_count; i++)
    {
      if (check_iterator (hotkeys[i].sequence, hotkeys[i].length))
        {
          // Hot-key sequence is in queue.
          hotkeys[i].callback ();
          return 1;
        }
    }

  return 0;
}

/**
 * Parses a key sequense into a numeric array of codes
 *
 * @oaram __sequence - sequence to parse
 * @param __res - pointer to array where to write result of parsing
 * @return length of sequence if succseed, -1 otherwise
 */
static short
parse_sequence                    (wchar_t *__sequence, wchar_t *__res)
{
  short i=0, n=wcslen (__sequence), len=0;
  BOOL ctrl, alt;
  wchar_t dummy;

  while (i<n)
    {
      // Sequence is too long
      if (len>=MAX_SEQUENCE_LENGTH)
        return -1;

      // Reset state
      ctrl=alt=FALSE;

      ////
      // Parse optional prefixes

      // Alt
      if (i<n-1 && __sequence[i]=='M' && __sequence[i+1]=='-')
        {
          alt=TRUE;
          i+=2;
        }

      // Control
      if (__sequence[i]=='^')
        {
          if (__sequence[i+1])
            {
              ctrl=TRUE;
              i++;
            } else
              return -1; /* Invalid CONTROL prefix */
        }

      ////
      // Check for function-key
      if (i<n-1 && __sequence[i]=='F' &&
          __sequence[i+1]>='0' && __sequence[i+1]<='9')
        {
          short f=0;
          i++;
          while (i<n && __sequence[i]>='0' && __sequence[i]<='9')
            f=f*10+__sequence[i++]-'0';
          
          dummy=KEY_F(f);
        } else
          dummy=__sequence[i++]; // Simple character

      ////
      // Apply prefixes
      if (ctrl)
        dummy=CTRL (dummy);

      if (alt)
        dummy=ALT (dummy);

      __res[len++]=dummy;

      // Skip spaces
      while (i<n && __sequence[i]<=' ')
        i++;
    }

  return len;
}

////////
// User's backend

/**
 * Registers a hotkey
 *
 * @param __sequence - hot-key sequence
 * @param __callback - callback to be called when hot-key sequence is pressed
 * @return zero on success
 */
short
hotkey_register                   (wchar_t *__sequence,
                                   hotkey_callback __callback)
{
  wchar_t dummy[MAX_SEQUENCE_LENGTH];
  short len;

  // Prepare sequence
  if ((len=parse_sequence (__sequence, dummy))>0)
    {
      // Sequence is good, so we can allocate new hot-key descriptor
      // and fill it in.
      hotkeys=realloc (hotkeys, sizeof (*hotkeys)*(hotkey_count+1));

      hotkeys[hotkey_count].sequence=malloc (sizeof (wchar_t)*len);
      memcpy (hotkeys[hotkey_count].sequence, dummy, len*sizeof (wchar_t));
      hotkeys[hotkey_count].length=len;
      hotkeys[hotkey_count].callback=__callback;

      hotkey_count++;
      return 0;
    }

  return -1;
}

/**
 * Realises a hot-key.
 *
 * @param __sequence - hot-key sequence to realise
 */
void
hotkey_realise                    (wchar_t *__sequence)
{
  wchar_t dummy[MAX_SEQUENCE_LENGTH];
  short len;

  if ((len=parse_sequence (__sequence, dummy))>0)
    {
      short i, j;
      BOOL eq;

      // Go through all registered hot-keys and
      // compare with sequence to realise.
      for (i=0; i<hotkey_count; i++)
        {
          // Check is sequences are equal
          eq=TRUE;
          for (j=0; j<len; j++)
            if (hotkeys[i].sequence[j]!=dummy[j])
              {
                eq=FALSE;
                break;
              }

          if (eq)
            {
              // Sequences are equal, so just destroy it.
              free (hotkeys[i].sequence);

              // Shift registered hotkeys
              for (j=i; j<hotkey_count; j++)
                hotkeys[i]=hotkeys[i+1];

              hotkey_count--;
              hotkeys=realloc (hotkeys, hotkey_count*sizeof (*hotkeys));

              break;
            }
        }
    }
}

/**
 * Puts new character to sequence
 *
 * @param __ch - character to put
 * @return non-zero if hot-key has been accepted
 */
short
hotkey_push_character             (wchar_t __ch)
{
  if (queue_ptr>MAX_SEQUENCE_LENGTH-1)
    {
      // Shift characters in queue
      short i;

      for (i=0; i<MAX_SEQUENCE_LENGTH-1; i++)
        queue[i]=queue[i+1];

      queue_ptr=MAX_SEQUENCE_LENGTH-1;
    }

  // Store character in queue
  queue[queue_ptr++]=__ch;

  return check ();
}
