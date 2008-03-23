/*
 *
 * ================================================================================
 *  screen.c
 * ================================================================================
 *
 *  Screen abstration layer
 *
 *  Written (by Nazgul) under General Public License.
 *
*/

#include "screen.h"
#include <locale.h>

////
//

#ifdef SCREEN_NCURSESW
static WINDOW *root_wnd = NULL;
#endif

static int mode=0;

scr_font_t sf_null;
scr_font_t sf_black_on_white, sf_blue_on_white, sf_yellow_on_white;
scr_font_t sf_white_on_red, sf_yellow_on_red;
scr_font_t sf_black_on_cyan, sf_blue_on_cyan;

#ifdef SCREEN_NCURSESW
#  define INIT_COLOR(__font,__pair_no,__fore,__back,__bold) \
  init_pair (__pair_no,  __fore,  __back); \
  __font.color_pair  = __pair_no; \
__font.bold          = __bold;
#endif

//////
//

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

  sf_null.color_pair            = CP_NULL;

#endif
 

  sf_black_on_cyan.bold   = FALSE;
}

//////
// User's backend

int            // Initialize screen stuff
screen_init                       (int __mode)
{

  mode=__mode;
  
#ifdef SCREEN_NCURSESW
  setlocale (LC_ALL, "");

  root_wnd=initscr ();

  cbreak ();
  noecho ();

  if (__mode&SM_COLOR)
    {
      // Initialize screen in color mode
      start_color ();
    }

#endif

  if (__mode&SM_COLOR)
    {
      define_default_fonts ();
    }

  return 0;
}

void           // Uninitialize screen stuff
screen_done                       (void)
{
#ifdef SCREEN_NCURSESW
  endwin ();
#endif
}

scr_window_t
screen_root_wnd                   (void)
{
  return root_wnd;
}

void
screen_refresh                    (BOOL __full_refresh)
{
#ifdef SCREEN_NCURSESW
  if (__full_refresh)
    touchwin (stdscr);
  refresh ();
#endif
}
