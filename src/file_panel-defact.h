/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Default actions' handlers for file panel
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _file_panel_defact_
#define _file_panel_defact_

#include "smartinclude.h"
#include "dir.h"

BEGIN_HEADER

#include "file_panel.h"
#include "hotkeys.h"

/*
 * NOTE: To reduce lengths of symbols from this module,
 *       prefix `file_panel_defact_` reduced to `fpd_`.
 */

typedef struct
{
  struct
  {
    vfs_cmp_proc comparator;
    vfs_filter_proc filter;
  } dir;

  /* Saved widget's callbacks */
  struct
  {
    widget_action focused;
    widget_action blured;
  } s_widget_callbacks;

  hotkey_context_t *hotkey_context;

  struct
  {
    unsigned long count;
    wchar_t **names;
    wchar_t *current_name;

    unsigned long caret_pos;
    unsigned long scroll_top;
  } selection_context;
} fpd_data_t;

/* Initialize file panels' default actions stuff */
int
fpd_init (void);

/* Unintialize file panels' default actions stuff */
void
fpd_done (void);

int
fpd_create (file_panel_t *__panel);

int
fpd_destroy (file_panel_t *__panel);

int
fpd_collect_items (file_panel_t *__panel);

int
fpd_free_items (file_panel_t *__panel);

/* Draw a file panel's widget */
int
fpd_draw_widget (file_panel_widget_t *__panel_widget);

int
fpd_widget_destructor (file_panel_widget_t *__widget);

/* Default action to draw a list of panel's items */
int
fpd_draw_item_list (file_panel_t *__panel);

/* Walk on file panel's widget */
void
fpd_walk (file_panel_t *__panel, short __direction);

/* Handles an on_refresh action of panel */
int
fpd_onrefresh (file_panel_t *__panel);

int
fpd_onresize (file_panel_t *__panel);

int
fpd_keydown_handler (file_panel_t *__panel, wchar_t *__ch);

int
fpd_centre_to_item (file_panel_t *__panel, wchar_t *__name);

int
fpd_scroll_to_item (file_panel_t *__panel, wchar_t *__name);

int
fpd_fill_submenu (file_panel_t *__panel, w_sub_menu_t *__submenu);

int
fpd_save_selection (file_panel_t *__panel);

int
fpd_restore_selection (file_panel_t *__panel);

int
fpd_free_saved_selection (file_panel_t *__panel);

/********
 *
 */

int
fpd_sortorder_menu_callback (void *__user_data);

END_HEADER

#endif
