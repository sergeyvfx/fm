/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "iface.h"
#include "screen.h"
#include "widget.h"
#include "messages.h"
#include "hotkeys.h"
#include "file_panel.h"
#include "i18n.h"

#include <wchar.h>
#include <stdlib.h>
#include <dir.h>
#include <signal.h>

// Functions for test core stuff
static int
test_clicked                      (w_button_t *__btn)
{
  wchar_t buf[1024]={0};

  int res=message_box ( _(L"Critical"), _(L"Some very long text for debug\nNew line"),
    MB_YESNOCANCEL|MB_CRITICAL);

  swprintf  (buf, 1024, L"Modal result is %d\n", res);
  message_box (_(L"Information"), buf, 0);

  return TRUE;
}

static int
exit_clicked                      (w_button_t *__btn)
{
  if (message_box (L"fm", _(L"Are you sure you want to quit?"),
    MB_YESNO|MB_DEFBUTTON_1)==MR_YES)
    {
      iface_done ();
      exit (0);
    }
  return TRUE;
}

static int
menu_clicked                      (w_button_t *__btn)
{
  widget_set_focus (WIDGET (WIDGET_USER_DATA (__btn)));
  return 0;
}

static int
menu_exit_clicked                 (void)
{
  exit_clicked (0);
  return 0;
}

static int
menu_rus_clicked                  (void)
{
  message_box (L"fm", L":)", MB_OK);
  return 0;
}

w_menu_t *menu;

void
menu_hotkey_callback              (void)
{
  widget_set_focus (WIDGET (menu));
}

// Just for testing and debugging
static void
test                              (void)
{
//  wint_t ch;
  w_button_t *btn;
  w_sub_menu_t *sm;
  w_edit_t *edt;
  w_box_t *main_box=NULL;
  w_window_t *wnd;

  main_box=widget_create_box (NULL, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
    WBS_HORISONTAL, 3);

  w_box_set_item_szie (main_box, 0, 1);
  w_box_set_item_szie (main_box, 2, 1);

  hotkey_register (L"F9", menu_hotkey_callback);
  hotkey_register (L"^a M-a M-^B", menu_hotkey_callback);

  hotkey_release (L"^a M-a M-^B");

  wnd=widget_create_window (_(L"My Window 1"), 4, 2, 32, 10);

  menu=widget_create_menu (WMS_HIDE_UNFOCUSED);

  btn=widget_create_button (WIDGET_CONTAINER (wnd), _(L"_Test"), 2, 2, WBS_DEFAULT);
  WIDGET_USER_CALLBACK (btn, clicked)=(widget_action)test_clicked;

  btn=widget_create_button (WIDGET_CONTAINER (wnd), _(L"_Menu"), 13, 2, 0);
  WIDGET_USER_DATA (btn) = menu;
  WIDGET_USER_CALLBACK (btn, clicked)=(widget_action)menu_clicked;

  btn=widget_create_button (WIDGET_CONTAINER (wnd), _(L"_Exit"), 22, 2, 0);
  WIDGET_USER_CALLBACK (btn, clicked)=(widget_action)exit_clicked;

  edt=widget_create_edit (WIDGET_CONTAINER (wnd), 2, 5, 20);
  w_edit_set_text (edt, L"Test");

  edt=widget_create_edit (WIDGET_CONTAINER (wnd), 2, 7, 20);
  w_edit_set_text (edt, L"Test2");
  w_edit_set_fonts (edt, &FONT (CID_YELLOW, CID_BLACK));

  sm=w_menu_append_submenu (menu, _(L"_File"));
  w_submenu_append_item (sm, _(L"_New"), 0, 0);
  w_submenu_append_item (sm, _(L"_Open..."), 0, 0);
  w_submenu_append_item (sm, L"_Русский", menu_rus_clicked, 0);
  w_submenu_append_item (sm, 0, 0, SMI_SEPARATOR);
  w_submenu_append_item (sm, _(L"_Exit"), menu_exit_clicked, 0);

  sm=w_menu_append_submenu (menu, _(L"_Edit"));
  w_submenu_append_item (sm, 0, 0, SMI_SEPARATOR);
  w_submenu_append_item (sm, _(L"_Undo"), 0, 0);
  w_submenu_append_item (sm, _(L"_Redo"), 0, 0);
  w_submenu_append_item (sm, 0, 0, SMI_SEPARATOR);
  w_submenu_append_item (sm, _(L"_Cut"), 0, 0);
  w_submenu_append_item (sm, _(L"C_opy"), 0, 0);
  w_submenu_append_item (sm, _(L"_Paste"), 0, 0);
  w_submenu_append_item (sm, 0, 0, SMI_SEPARATOR);
  w_submenu_append_item (sm, 0, 0, SMI_SEPARATOR);

  sm=w_menu_append_submenu (menu, _(L"_Help"));
  w_submenu_append_item (sm, _(L"_About"), 0, 0);

  file_panels_init (WIDGET (w_box_item (main_box, 1)));

  w_window_show_modal (wnd);

  widget_main_loop ();

//  file_panels_done ();

//  widget_destroy (WIDGET (main_box));
//  widget_destroy (WIDGET (wnd));
//  widget_destroy (WIDGET (menu));
}

#ifdef SIGWINCH

static void
sig_winch                         (int sig ATTR_UNUSED)
{
  screen_on_resize ();
  widget_on_scr_resize ();
}
#endif

////////
// User's backend

/**
 * Initializes interface
 *
 * @return zero on success, non-zero on failure
 */
int
iface_init                        (void)
{
  int res;

  // Initialise terminal control
  if ((res=screen_init (SM_COLOR)))
    return res;

  // Initialise widgets
  if ((res=widgets_init ()))
    return res;

#ifdef SIGWINCH
  // For caughting terminal resizing
  signal (SIGWINCH, sig_winch);
#endif

  scr_hide_curser ();

  test ();

  return 0;
}

/**
 * Uninitializes interface
 */
void
iface_done                        (void)
{
  widgets_done ();

  // We don't need screen now!
  screen_done ();
}
