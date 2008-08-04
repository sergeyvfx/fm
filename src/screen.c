/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * terminal handling abstraction layer
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "screen.h"
#include "hotkeys.h"

////
//

#ifdef SCREEN_NCURSESW
static WINDOW *root_wnd = NULL;
BOOL curs_wis = TRUE;
#  define MY_KEYS (KEY_MAX+1)
#endif

int ESCDELAY = 0;  // Disable pause when escape is pressed
static int mode=0; // Mode of screen

// Default fonts
scr_font_t screen_fonts[SCREEN_MAX_FONTS];

#ifdef SCREEN_NCURSESW
#  include <time.h>

#  define INIT_COLOR(__font,__pair_no,__fore,__back,__bold) \
  init_pair (__pair_no,  __fore,  __back); \
  __font.color_pair  = __pair_no; \
__font.bold          = __bold;

#  define _INIT_COLOR(__fore, __back, __bold) \
  { \
    INIT_COLOR (screen_fonts[CID (__fore, __back)],\
      pair_no,__fore,__back,__bold);\
    pair_no++;\
  }

#  define DELAY (0.2*1000*10)

#endif

//////
//

/**
 *  Initializes default fonts
 */
static void
define_default_fonts              (void)
{
#ifdef SCREEN_NCURSESW

  int colors[][3]=
   {
     {CID_BLACK,  CID_WHITE, FALSE},
     {CID_BLUE,   CID_WHITE, FALSE},
     {CID_YELLOW, CID_WHITE, TRUE},

     {CID_CYAN,   CID_BLUE,  TRUE},
     {CID_YELLOW, CID_BLUE,  TRUE},
     {CID_WHITE,  CID_BLUE,  TRUE},

     {CID_BLACK,  CID_CYAN,  FALSE},
     {CID_BLUE,   CID_CYAN,  FALSE},
     {CID_YELLOW, CID_CYAN,  TRUE},

     {CID_YELLOW, CID_BLACK,  TRUE},

     {CID_WHITE,   CID_RED,  TRUE},
     {CID_YELLOW,  CID_RED,  TRUE},

     {CID_WHITE,   CID_BLACK, FALSE},

     {-1, -1, -1}
   };

  int i=0, pair_no=1;

  while (colors[i][0]>=0)
    {
      _INIT_COLOR (colors[i][0], colors[i][1], colors[i][2]);
      i++;
    }

#endif
}

/**
 * Initializes smart handling of escaped characters (i.e. Esc-^[)
 */
static void
init_escape_keys                  (void)
{
#ifdef SCREEN_NCURSESW
  short i;
  keypad (stdscr, TRUE);

  for (i=0; i<255; ++i)
    {
      char temp[10];
      sprintf (temp, "\033%c", i);
      define_key (temp, i+MY_KEYS);
    }

  for (i=KEY_MIN; i<KEY_MAX; ++i)
    {
      char *value;
      if ((value=keybound (i, 0))!=0)
        {
          char *temp = malloc (strlen (value)+2);
          sprintf (temp, "\033%s", value);
          define_key(temp, i+MY_KEYS);

          free (temp);
          free (value);
        }
    }
#endif
}

//////
// User's backend

/**
 * Initializes screen stuff
 *
 * @param __mode - mode of screen to be initialized
 * @return a zero in successful
 */
int
screen_init                       (int __mode)
{
  mode=__mode;

#ifdef SCREEN_NCURSESW
  root_wnd=initscr ();

  cbreak ();  // take input chars one at a time, no wait for \n
  noecho ();  // don't echo input
  raw();
  nodelay (stdscr, TRUE);

  if (__mode&SM_COLOR)
    {
      // Initialize screen in color mode
      start_color ();
    }

  //
  // TODO:
  //  But does we really need this?
  //
  //  (this stuff was added to give an opportunity to
  //   handle escaped escape)
  //
  init_escape_keys ();

#endif

  if (__mode&SM_COLOR)
    {
      define_default_fonts ();
    }

  return 0;
}

/**
 * Uninitializes screen stuff
 */
void
screen_done                       (void)
{
#ifdef SCREEN_NCURSESW
  noraw();
  keypad(stdscr, FALSE);
  endwin ();
#endif
}

/**
 * Returns a root window
 *
 * @return a root window
 */
scr_window_t
screen_root_wnd                   (void)
{
  return root_wnd;
}

/**
 * Refresh screen
 *
 * @param __full_refresh - should a full screen will be refreshed?
 */
void
screen_refresh                    (BOOL __full_refresh)
{
#ifdef SCREEN_NCURSESW
  if (__full_refresh)
    touchwin (stdscr);
  refresh ();
#endif
}

/**
 * Call this method when root screen has been resized
 */
void
screen_on_resize                  (void)
{
#ifdef SCREEN_NCURSESW
  // We need reinitialize the screen
  endwin ();
  refresh ();

  // Restore visibility of cursor
  if (!curs_wis)
    scr_hide_cursor (); else
    scr_show_cursor ();

#endif
}

/**
 * Returns character from window
 *
 * @param __window - from which window expects a character
 * (this parameter maybe deprecated)
 * @return caughted character
 */
wchar_t
scr_wnd_getch                     (scr_window_t __window)
{
  //
  // TODO:
  //  Need this function to look after all caughted characters
  //  and do hotkeys stuff here
  //

  wint_t ch;

#ifdef SCREEN_NCURSESW
  struct timespec timestruc;

  timestruc.tv_sec  = 0;
  timestruc.tv_nsec = DELAY;
#endif

  for (;;)
    {
      // Read next character from window

#ifdef SCREEN_NCURSESW
      while (wget_wch (stdscr, &ch)==ERR)
        nanosleep (&timestruc, 0);
#endif

      if (!hotkey_push_character (ch))
        break;
    }

  return ch;
}

#ifdef SCREEN_NCURSESW

/**
 * Checks is specified character is a code of
 * ncurses's function key.
 *
 * This function is necessary because most of functional keys' codes
 * in ncurses have codes from extended Latin range of Unicode
 *
 * @param __ch - character which has to be checked
 * @return zero if character is not a ncurses's functional key,
 * non-zero otherwise.
 */
int
is_ncurses_funckey                (wchar_t __ch)
{
  //
  // TODO:
  //  But maybe it'll be better if we replace this function with macros like
  //  #define is_ncurses_funckey (__ch>=KEY_MIN && __ch<=KEY_MAX) ?
  //

  return __ch>=KEY_MIN && __ch<=KEY_MAX;
}
#endif

scr_window_t
scr_create_sub_window             (scr_window_t __parent,
                                   int __x, int __y,
                                   int __w, int __h)
{
  scr_window_t res;

  if (!__parent)
    return 0;

#ifdef SCREEN_NCURSESW
  while (__parent->_parent)
    {
      __x+=__parent->_begx;
      __y+=__parent->_begy;
      __parent=__parent->_parent;
    }

  res=subwin (__parent, __h, __w, __y, __x);
#endif

  scr_wnd_erase (res);

  return res;
}
