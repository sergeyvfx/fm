/*
 *
 * ================================================================================
 *  iface.h
 * ================================================================================
 *
 *  Written (by Nazgul) under General Public License.
 *
*/

#include "iface.h"
#include "screen.h"
#include "widget.h"
#include "messages.h"

#include <wchar.h>
#include <stdlib.h>
#include <signal.h>

static int
test_clicked                      (w_button_t *__btn)
{
  wchar_t buf[1024]={0};

  int res=message_box (L"Critical", L"Some very long text for debug\nNew line", MB_YESNOCANCEL|MB_CRITICAL);

  swprintf  (buf, 1024, L"Modal result is %d\n", res);
  message_box (L"Information", buf, 0);

  return TRUE;
}

static int
exit_clicked                      (w_button_t *__btn)
{
  if (message_box (L"fm", L"Are you sure you want to quit?", MB_YESNO|MB_DEFBUTTON_1)==MR_YES)
    {
      iface_done ();
      printf ("Good bye!\n");
      exit (0);
    }
  return TRUE;
}

// Just for testing and debugging
static void
test                              (void)
{
  w_window_t *wnd;
  w_button_t *btn;

  wnd=widget_create_window (L"My Window 1", 4, 1, 31, 10);

  btn=widget_create_button (WIDGET_CONTAINER (wnd), L"_Test", 2, 2, WBS_DEFAULT);
  WIDGET_USER_CALLBACK (btn, clicked)=(widget_action)test_clicked;

  btn=widget_create_button (WIDGET_CONTAINER (wnd), L"_Exit", 13, 2, 0);
  WIDGET_USER_CALLBACK (btn, clicked)=(widget_action)exit_clicked;

  w_window_show_modal (wnd);

  widget_destroy (WIDGET (wnd));
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

int            // Initialize interface
iface_init                        (void)
{
  int res;

  if ((res=screen_init (SM_COLOR)))
    return res;

#ifdef SIGWINCH
  signal (SIGWINCH, sig_winch);
#endif

  scr_hide_curser ();
  
  test ();

  return 0;
}

void           // Uninitialize interface
iface_done                        (void)
{
  screen_done ();
}
