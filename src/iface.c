/*
 *
 * =============================================================================
 *  iface.h
 * =============================================================================
 *
 *  Written (by Nazgul) under General Public License.
 *
*/

#include "iface.h"
#include "screen.h"
#include "widget.h"
#include "messages.h"
#include "hotkeys.h"

#include "i18n.h"

#include <wchar.h>
#include <stdlib.h>
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
  w_window_t *wnd;
  w_button_t *btn;
  w_sub_menu_t *sm;

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
  
  w_window_show_modal (wnd);

  widget_destroy (WIDGET (wnd));
  widget_destroy (WIDGET (menu));
}

#ifdef SIGWINCH
static void
sig_winch                         (int sig ATTR_UNUSED)
{
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
  // We don't need screen now!
  screen_done ();
}
