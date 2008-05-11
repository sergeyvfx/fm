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
#  define MY_KEYS (KEY_MAX+1)
#endif

int ESCDELAY = 0;  // Disable pause when escape is pressed
static int mode=0; // Mode of screen

// Default fonts
scr_font_t sf_null;
scr_font_t sf_black_on_white, sf_blue_on_white, sf_yellow_on_white;
scr_font_t sf_white_on_red, sf_yellow_on_red;
scr_font_t sf_black_on_cyan, sf_blue_on_cyan, sf_yellow_on_cyan;
scr_font_t sf_yellow_on_black, sf_white_on_black;
scr_font_t sf_gray_on_blue, sf_lblue_on_blue, sf_cyan_on_blue,
           sf_lcyan_on_blue, sf_white_on_blue, sf_yellow_on_blue;

#ifdef SCREEN_NCURSESW
#  define INIT_COLOR(__font,__pair_no,__fore,__back,__bold) \
  init_pair (__pair_no,  __fore,  __back); \
  __font.color_pair  = __pair_no; \
__font.bold          = __bold;
#endif

//////
//

/**
 *  Initializes default fonts
 */
static void
define_default_fonts              (void)
{
  sf_null.bold = FALSE;

#ifdef SCREEN_NCURSESW
  sf_null.color_pair = 0;

  INIT_COLOR (sf_black_on_white,  CP_BLACK_ON_WHITE,   COLOR_BLACK,   COLOR_WHITE, FALSE);
  INIT_COLOR (sf_blue_on_white,   CP_BLUE_ON_WHITE,    COLOR_BLUE,    COLOR_WHITE, FALSE);
  INIT_COLOR (sf_yellow_on_white, CP_YELLOW_ON_WHITE,  COLOR_YELLOW,  COLOR_WHITE, TRUE);

  INIT_COLOR (sf_white_on_red,    CP_WHITE_ON_RED,     COLOR_WHITE,   COLOR_RED,   TRUE);
  INIT_COLOR (sf_yellow_on_red,   CP_YELLOW_ON_RED,    COLOR_YELLOW,  COLOR_RED,   TRUE);

  INIT_COLOR (sf_black_on_cyan,   CP_BLACK_ON_CYAN,    COLOR_BLACK,   COLOR_CYAN,  FALSE);
  INIT_COLOR (sf_blue_on_cyan,    CP_BLUE_ON_CYAN,     COLOR_BLUE,    COLOR_CYAN,  FALSE);
  INIT_COLOR (sf_yellow_on_cyan,  CP_YELLOW_ON_CYAN,   COLOR_YELLOW,  COLOR_CYAN,  TRUE);

  INIT_COLOR (sf_yellow_on_black, CP_YELLOW_ON_BLACK,  COLOR_YELLOW,  COLOR_BLACK, TRUE);
  INIT_COLOR (sf_white_on_black,  CP_WHITE_ON_BLACK,   COLOR_WHITE,   COLOR_BLACK, TRUE);

  INIT_COLOR (sf_gray_on_blue ,   CP_GRAY_ON_BLUE,     COLOR_WHITE,   COLOR_BLUE,  FALSE);
  INIT_COLOR (sf_lblue_on_blue ,  CP_LBLUE_ON_BLUE,    COLOR_BLUE,    COLOR_BLUE,  TRUE);
  INIT_COLOR (sf_cyan_on_blue ,   CP_CYAN_ON_BLUE,     COLOR_CYAN,    COLOR_BLUE,  FALSE);
  INIT_COLOR (sf_lcyan_on_blue ,  CP_LCYAN_ON_BLUE,    COLOR_CYAN,    COLOR_BLUE,  TRUE);
  INIT_COLOR (sf_white_on_blue ,  CP_WHITE_ON_BLUE,    COLOR_WHITE,   COLOR_BLUE,  TRUE);
  INIT_COLOR (sf_yellow_on_blue , CP_YELLOW_ON_BLUE,   COLOR_YELLOW,  COLOR_BLUE,  TRUE);

  sf_null.color_pair            = CP_NULL;

#endif
 
  sf_black_on_cyan.bold   = FALSE;
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

  if (__mode&SM_COLOR)
    {
      // Initialize screen in color mode
      start_color ();
    }

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
 * Refreshs screen
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
 * Returns character from window
 *
 * @param __window - from which window expects a character
 * @return caugthed character
 */
wchar_t
scr_wnd_getch                     (scr_window_t __window)
{
  //
  // TODO:
  //  Need this function to look after all caucghted characters
  //  and do hotkeys stuff here
  //

  wint_t ch;

  for (;;)
    {

  // Read next character from window
#ifdef SCREEN_NCURSESW
      wget_wch (__window, &ch);
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
 * This function is neccessary because most of functional keys' codes
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
  //  But maybe it'll be better if we replace this function with makros like
  //  #define is_ncurses_funckey (__ch>=KEY_MIN && __ch<=KEY_MAX) ?
  //

  return __ch>=KEY_MIN && __ch<=KEY_MAX;
}
#endif
