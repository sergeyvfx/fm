/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#define NO_XOPEN_SOURCE /* For use SIGWINCH */

#include "iface.h"
#include "screen.h"
#include "hotkeys.h"
#include "file_panel.h"
#include "messages.h"
#include "i18n.h"
#include "shared.h"

#include <signal.h>

#include <vfs/vfs.h>
#include <widget.h>

/********
 * Some helpful macroses
 */

#define _INIT_ITERATOR(_proc, _args...) \
  if ((res=_proc (_args))) \
    return res;

/********
 * Variables
 */

static w_box_t *main_box = NULL;

/********
 *
 */

/**
 * Create all widgets needed by iface
 *
 * @return zero on success, non-zero on failure
 */
static int
create_widgets (void)
{
  /* Create box for basic layout */
  main_box = widget_create_box (NULL, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                                WBS_HORISONTAL, 3);

  if (!main_box)
    {
      return -1;
    }

  /*
   * NOTE: First item of box is reserved for menu
   *       when it hasn't got style WMS_HIDE_UNFOCUSED
   */
  w_box_set_item_szie (main_box, 0, 0);

  w_box_set_item_szie (main_box, 2, 1);

  return 0;
}

#ifdef SIGWINCH

/**
 * Handler for signal SIGWINCH which is called
 * when a terminal is resized
 *
 * @param sig(unused) - code of returned signal
 */
static void
sig_winch (int sig ATTR_UNUSED)
{
  if (!iface_screen_disabled ())
    {
      /* We should call this function only if screen is not paused */
      /* Otherwise it will be unpaused */
      screen_on_resize ();
      widget_on_scr_resize ();
    }
}
#endif

/********
 * Handlers
 */

/**
 * Handler for global hotkey for action Exit
 *
 * @return zero on success, non-zero otherwise
 */
static int
exit_hotkey (void *__reg_data ATTR_UNUSED)
{
  iface_act_exit ();
  return 0;
}

/**
 * Load VFS plugin
 *
 * @param __fn - name of file to load as VFS plugin
 * @return zero on success, non-zero otherwise
 */
static int
load_vfs_plugin (wchar_t *__fn)
{
  int res = 0;

  if ((res = vfs_plugin_load (__fn)))
    {
      wchar_t error[1024];
      swprintf (error, 1024,
                _(L"%ls\n\nContinue executong without this plugin?"),
                vfs_get_error (res));

      if (message_box (_(L"VFS error"), error,
                       MB_YESNO | MB_CRITICAL | MB_DEFBUTTON_1) == MR_YES)
        {
          res = 0;
        }

      return res;
    }

  return 0;
}

/**
 * Initialize VFS stuff
 *
 * @param zero on success, non-zero otherwise
 */
static int
init_vfs (void)
{
  int res;
  wchar_t **list;
  long i, count;

  _INIT_ITERATOR (vfs_init);

  count = get_shared_files (L"plugins/vfs", L"VFS_PLUGINS", &list);

  /* Load VFS plugins */
  for (i = 0; i < count; ++i)
    {
      _INIT_ITERATOR (load_vfs_plugin, list[i]);
      free (list[i]);
    }

  SAFE_FREE (list);

  /* We can not work without support of local file systems in VFS */
  /* So, we should display an error and exit */
  if (!vfs_plugin_loaded (VFS_LOCALFS_PLUGIN))
    {
      MESSAGE_ERROR (_(L"Can not work without localfs VFS plugin!\n"
                       L"The program will now exit. Sorry."));
      return -1;
    }

  return 0;
}

/********
 * User's backend
 */

/**
 * Initialize interface
 *
 * @return zero on success, non-zero on failure
 */
int
iface_init (void)
{
  int res;

  /* Initialize terminal control */
  _INIT_ITERATOR (screen_init, SM_COLOR);

  /* Initialize widgets */
  _INIT_ITERATOR (widgets_init);

#ifdef SIGWINCH
  /* For catching terminal resizing */
  signal (SIGWINCH, sig_winch);
#endif

  scr_hide_cursor ();

  /* Initialize VFS */
  _INIT_ITERATOR (init_vfs);

  /* Create all widgets */
  _INIT_ITERATOR (create_widgets);

  /* Initialize hotkeys' stuff */
  _INIT_ITERATOR (hotkeys_init);

  /*
   * NOTE: We should initialize hotkeys here, because this hotkeys
   *       should be implemented in base context.
   *       But after file panels are initialized, new hotkey context
   *       may be present.
   */
  hotkey_register (L"F10", exit_hotkey);

  file_panels_init (WIDGET (w_box_item (main_box, 1)));

  /* Create menu */
  _INIT_ITERATOR (iface_create_menu);

  /*
   * FIXME: There is some troubles with panels' disappearing after
   *        menu has been created.
   */
  widget_redraw (WIDGET (w_box_item (main_box, 1)));

  return 0;
}

/**
 * Uninitialize interface
 */
void
iface_done (void)
{
  file_panels_done ();
  hotkeys_done ();
  widgets_done ();
  vfs_done ();

  /* We don't need screen now! */
  screen_done ();
}

/**
 * Main loop of interface
 *
 * @return zero on success, non-zero on failure
 */
int
iface_mainloop (void)
{
  widget_main_loop ();

  return 0;
}
