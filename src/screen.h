/*
 *
 * ================================================================================
 *  screen.h
 * ================================================================================
 *
 *  Screen abstration layer
 *
 *  Written (by Nazgul) under General Public License.
 *
*/

#ifndef _screen_h_
#define _screen_h_

#include "smartinclude.h"

////
// Constants

// Screen modes
#define SM_COLOR    0x0001

#ifdef SCREEN_NCURSESW

#  include <ncursesw/curses.h>

#  define SCREEN_WIDTH      COLS
#  define SCREEN_HEIGHT     LINES

// Type definitions
#  define scr_window_t      WINDOW*
#  define scr_color_pair_t  int

// Functions' call abstraction
#  define scr_hide_curser()                     curs_set (0)
#  define scr_show_curser()                     curs_set (1)

#  define scr_create_window(__x,__y,__w,__h)    newwin (__h, __w, __y, __x)
#  define scr_destroy_window(__wnd)             delwin (__wnd)

#  define scr_move_caret(__x,__y)               move (__y, __x)

#  define scr_ungetch(__ch)                     ungetch (__ch)

#  define scr_clear(_wnd)                       clear ()

#  define scr_wnd_keypad(__wnd, __val)          keypad (__wnd, __val)

#  define scr_wnd_clear(_wnd)                   wclear (_wnd)
#  define scr_wnd_refresh(_wnd)                 wrefresh (_wnd)

#  define scr_wnd_getch(__wnd)                  wgetch (__wnd)
#  define scr_wnd_putch(__wnd,__ch)             waddch (__wnd, __ch)
#  define scr_wnd_printf(__wnd,__text,__arg...) wprintw (__wnd, __text, ##__arg)

#  define scr_wnd_border(__wnd)                 box (__wnd, 0, 0)
#  define scr_wnd_move_caret(__wnd,__x,__y)     wmove (__wnd, __y, __x)

#  define scr_bkg(__font) \
  bkgd  (COLOR_PAIR ((__font).color_pair) | ((__font).bold?A_BOLD:0));
#  define scr_font(__font) \
  bkgdset  (COLOR_PAIR ((__font).color_pair) | ((__font).bold?A_BOLD:0));

#  define scr_wnd_bkg(__wnd,__font) \
  wbkgd  (__wnd, COLOR_PAIR ((__font).color_pair) | ((__font).bold?A_BOLD:0));
#  define scr_wnd_font(__wnd,__font) \
  wbkgdset  (__wnd, COLOR_PAIR ((__font).color_pair) | ((__font).bold?A_BOLD:0));

#  define scr_wnd_attron(_wnd,_attr)      wattron (_wnd,_attr)
#  define scr_wnd_attroff(_wnd,_attr)     wattroff (_wnd,_attr)

#  define scr_wnd_attr_backup(_w) \
  attr_t __old_attr_; \
  short  __old_pair_; \
  wattr_get (_w, &__old_attr_, &__old_pair_, 0);

#  define scr_wnd_attr_restore(_w) \
  wattr_set (_w, __old_attr_, __old_pair_, 0);

// Default color pairs
#  define CP_NULL            0

#  define CP_BLACK_ON_WHITE    1
#  define CP_BLUE_ON_WHITE     2
#  define CP_YELLOW_ON_WHITE   3
#  define CP_WHITE_ON_RED      4
#  define CP_YELLOW_ON_RED     5
#  define CP_BLACK_ON_CYAN     6
#  define CP_BLUE_ON_CYAN      7

#  define COLOR_PAIR_USER   20

// Characters
#  define CH_LTEE  ACS_LTEE
#  define CH_RTEE  ACS_RTEE
#  define CH_BTEE  ACS_BTEE
#  define CH_TTEE  ACS_TTEE

//Keys
#  define KEY_TAB       9
#  define KEY_RETURN    10
#  define KEY_ESC       27

#endif

////
// Type defenitions

typedef struct {
  scr_color_pair_t color_pair;
  BOOL             bold;
} scr_font_t;

////
// Global fonts

extern scr_font_t sf_null;
extern scr_font_t sf_black_on_white, sf_blue_on_white, sf_yellow_on_white;
extern scr_font_t sf_white_on_red,   sf_yellow_on_red;
extern scr_font_t sf_black_on_cyan,  sf_blue_on_cyan;

////////
//

int            // Initialize screen stuff
screen_init                       (int __mode);

void           // Uninitialize screen stuff
screen_done                       (void);

scr_window_t
screen_root_wnd                   (void);

void
screen_refresh                    (BOOL __full_refresh);

#endif
