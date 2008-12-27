/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Sortorder menu callback for default file panel
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "file_panel-defact.h"
#include "i18n.h"
#include "messages.h"
#include "file_panel.h"

#include <widget.h>

/********
 *
 */

#define WIDTH 40
#define HEIGHT 5

/********
 *
 */

/**
 * Create configuration window
 *
 * @return pointer to created window
 */
static w_window_t*
create_window (void)
{
  w_window_t *wnd = NULL;
  w_button_t *btn;

  wnd = widget_create_window (_ (L"Sort order"), 0, 0, WIDTH, HEIGHT,
                              WMS_CENTERED);

  if (!wnd)
    {
      return 0;
    }

  /*
   * TODO: Insert a smart stuff here
   */

  btn = widget_create_button (NULL, WIDGET_CONTAINER (wnd), _ (L"_Ok"),
                              WIDTH / 2 - 4, HEIGHT - 2, WBS_DEFAULT);
  btn->modal_result = MR_OK;

  return wnd;
}

/********
 * User's backend
 */

/**
 * Callback for `Sort order` menu item
 *
 * @param __user_data - associated file panel
 * @return zero if callback hasn't handled received character
 */
int
fpd_sortorder_menu_callback (void *__user_data)
{
  w_window_t *wnd;
  file_panel_t *panel;

  if (!__user_data)
    {
      MESSAGE_ERROR (L"File panel isn't associated with menu item");
      return -1;
    }

  if (!(wnd = create_window ()))
    {
      MESSAGE_ERROR (L"Error creating window");
      return -1;
    }

  panel = __user_data;

  w_window_show_modal (wnd);

  widget_destroy (WIDGET (wnd));

  return 0;
}
