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

#ifndef _screen_h_
#define _screen_h_

#include "smartinclude.h"

BEGIN_HEADER

/********
 * Constants
 */

/* Screen modes */
#define SM_COLOR    0x0001

/* Maximal dimensions of screen */
#define MAX_SCREEN_WIDTH  2048
#define MAX_SCREEN_HEIGHT 1024

#ifdef SCREEN_NCURSESW

#include <ncursesw/curses.h>
#include <panel.h>

#define SCREEN_WIDTH      COLS
#define SCREEN_HEIGHT     LINES

/* Type definitions */
typedef WINDOW* scr_window_t;
typedef PANEL* panel_t;

extern BOOL curs_wis;

#define scr_color_pair_t  int

/* Functions' call abstraction */
#define scr_hide_cursor()                     (curs_set (0),curs_wis=FALSE)
#define scr_show_cursor()                     (curs_set (1),curs_wis=TRUE)

#define scr_create_window(__x,__y,__w,__h)    newwin (__h, __w, __y, __x)

#define scr_destroy_window(__wnd)             delwin (__wnd)

#define scr_move_caret(__x,__y)               move (__y, __x)

#define scr_ungetch(__ch)                     ungetch (__ch)

#define scr_clear(_wnd)                       clear ()
#define scr_doupdate()                        doupdate()

#define scr_wnd_keypad(__wnd, __val)          keypad (__wnd, __val)

#define scr_wnd_resize(_wnd,_w,_h)            wresize (_wnd, _h, _w)
#define scr_wnd_erase(_wnd)                   werase (_wnd)
#define scr_wnd_clear(_wnd)                   wclear (_wnd)
#define scr_wnd_refresh(_wnd)                 wrefresh (_wnd)

/*
#  define scr_wnd_getch(__wnd, __ch)            wget_wch (__wnd, &__ch)
 */
#define scr_wnd_putch(__wnd,__ch)             waddch (__wnd, __ch)
#define scr_wnd_printf(__wnd,__text,__arg...) wprintw (__wnd, __text, ##__arg)
#define scr_wnd_add_nstr(__wnd,__str,__n)     waddnwstr (__wnd, __str, __n)
#define scr_wnd_add_wchar(__wnd,__ch)         scr_wnd_add_nstr (__wnd, &__ch, 1)

#define scr_wnd_border(__wnd)                 box (__wnd, 0, 0)
#define scr_wnd_move_caret(__wnd,__x,__y)     wmove (__wnd, __y, __x)

#define scr_bkg(__font) \
  bkgd  (COLOR_PAIR ((__font).color_pair) | ((__font).bold?A_BOLD:0))
#define scr_font(__font) \
  bkgdset  (COLOR_PAIR ((__font).color_pair) | ((__font).bold?A_BOLD:0))

#define scr_wnd_bkg(__wnd,__font) \
  wbkgd  (__wnd, COLOR_PAIR ((__font).color_pair) | ((__font).bold?A_BOLD:0))
#define scr_wnd_font(__wnd,__font) \
  wbkgdset  (__wnd, COLOR_PAIR ((__font).color_pair) | ((__font).bold?A_BOLD:0))

#define scr_wnd_refresh(_wnd)           wrefresh (_wnd)
#define scr_wnd_invalidate(_wnd)        touchwin(_wnd)

#define scr_wnd_attron(_wnd,_attr)      wattron (_wnd,_attr)
#define scr_wnd_attroff(_wnd,_attr)     wattroff (_wnd,_attr)

#define scr_wnd_attr_backup(_w) \
  attr_t __old_attr_; \
  short  __old_pair_; \
  wattr_get (_w, &__old_attr_, &__old_pair_, 0);

#define scr_wnd_attr_restore(_w) \
  wattr_set (_w, __old_attr_, __old_pair_, 0);

/* Default color pairs */
#define CP_NULL            0

#define CID_LIGHT(_c) ((_c)+0x08)
#define CID(_f, _b)   (((_b)<<0x03)+(_f)+1)
#define FONT(_f, _b)  (screen_fonts[CID(_f, _b)])

#define CID_BLACK     COLOR_BLACK
#define CID_RED       COLOR_RED
#define CID_GREEN     COLOR_GREEN
#define CID_YELLOW    COLOR_YELLOW
#define CID_BLUE      COLOR_BLUE
#define CID_MAGENTA   COLOR_MAGENTA
#define CID_CYAN      COLOR_CYAN
#define CID_WHITE     COLOR_WHITE

#define CID_LBLACK     CID_LIGHT(CID_BLACK)
#define CID_LRED       CID_LIGHT(CID_RED)
#define CID_LGREEN     CID_LIGHT(CID_GREEN)
#define CID_LYELLOW    CID_LIGHT(CID_YELLOW)
#define CID_LBLUE      CID_LIGHT(CID_BLUE)
#define CID_LMAGENTA   CID_LIGHT(CID_MAGENTA)
#define CID_LCYAN      CID_LIGHT(CID_CYAN)
#define CID_LWHITE     CID_LIGHT(CID_WHITE)

#define COLOR_MIN 0x00
#define COLOR_MAX 0x07

#define LIGHT_COLOR_MIN CID_LIGHT(0x00)
#define LIGHT_COLOR_MAX CID_LIGHT(0x07)

#define COLOR_PAIR_USER   20

/* Characters */
#define CH_LTEE  ACS_LTEE
#define CH_RTEE  ACS_RTEE
#define CH_BTEE  ACS_BTEE
#define CH_TTEE  ACS_TTEE

#ifndef CTRL
#define CTRL(x)  (((x)=='m'||(x)=='M')?10:(x)&0x1f)
#endif

#ifndef ALT
#define ALT(x)  ((x) | 0x200)
#endif

/* Keys */
#define KEY_TAB       9
#define KEY_RETURN    10
#define KEY_ESC       27
#define KEY_SPACE     32
#define KEY_DELETE    KEY_DC
#define KEY_INSERT    KEY_IC

#define KEY_ESC_ESC   539 /* Escaped escape */

/********
 * Panel-stuff abstraction
 */

#define panels_doupdate() { panels_update (); scr_doupdate (); }

#define panel_new(_layout)     new_panel (_layout);
#define panel_del(_p)          { panel_hide (_p); del_panel (_p); }

#define panel_show(_p)         { show_panel (_p); panels_doupdate (); }
#define panel_hide(_p)         { hide_panel (_p); panels_doupdate (); }

#define panel_move(_p,_x,_y)   { move_panel (_p,_y,_x); panels_doupdate (); }
#define panels_update()        update_panels ()
#define panel_wnd(_p)          panel_window (_p)
#define panel_replace(_p,_w)   replace_panel(_p, _w)

#endif

/****
 * Tpyes
 */

/* Screen font */
typedef struct
{
  /* Color pair of text */
  scr_color_pair_t color_pair;

  BOOL bold;
} scr_font_t;

/********
 * Global fonts
 */

#define SCREEN_MAX_FONTS  128
extern scr_font_t screen_fonts[SCREEN_MAX_FONTS];

/********
 *
 */

/* Initialize screen stuff */
int
screen_init (int __mode);

/* Uninitialize screen stuff */
void
screen_done (void);

/* Get root window */
scr_window_t
screen_root_wnd (void);

/* Refresh screen */
void
screen_refresh (BOOL __full_refresh);

void
screen_on_resize (void);

/* Read input character */
wchar_t
scr_wnd_getch                     (BOOL __locking);

scr_window_t
scr_create_sub_window (scr_window_t __parent,
                       int __x, int __y,
                       int __w, int __h);

#ifdef SCREEN_NCURSESW

int
is_ncurses_funckey (wchar_t __ch);

#endif

END_HEADER

#endif
