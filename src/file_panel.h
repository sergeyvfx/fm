/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * File panel's stuff
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _file_panel_h_
#define _file_panel_h_

#include "smartinclude.h"

BEGIN_HEADER

#include "screen.h"
#include "widget.h"
#include "file.h"

#include <wchar.h>

////////
// Constants

// Panel can't be focused
#define FPF_UNFOCUSABLE    0x0001

////////
// Macroses

#define FILE_PANEL_SET_FLAG(__panel, __flag) \
  SET_FLAG((__panel)->flags, __flag)

#define FILE_PANEL_TEST_FLAG(__panel, __flag) \
  TEST_FLAG((__panel)->flags, __flag)

#define FILE_PANEL_CLEAR_FLAG(__panel, __flag) \
  CLEAR_FLAG((__panel)->flags, __flag)

#define SET_PANEL_ACTION(__panel, __action, __val) \
  __panel->actions.__action=(file_panel_action)__val

#define SET_PANEL_DATA_ACTION(__panel, __action, __val) \
  __panel->actions.__action=(file_panel_data_action)__val

#define FILE_PANEL_ACTION_CALL(__panel, __action) \
  (__panel->actions.__action?__panel->actions.__action (__panel):-1)

#define FILE_PANEL_DATA_ACTION_CALL(__panel, __action, __data) \
  (__panel->actions.__action?__panel->actions.__action (__panel, __data):-1)

////////
// Type defenitins

// Walking directions
enum {
  WALK_NEXT,
  WALK_PREV,
  WALK_NEXT_PAGE,
  WALK_PREV_PAGE,
  WALK_HOME,
  WALK_END,
  WALK_NEXT_COLUMN,
  WALK_PREV_COLUMN
};

// Types of columns
enum {
  COLUMN_UNKNOWN = -1,
  COLUMN_NAME    =  0,
  COLUMN_SIZE    =  1,
  COLUMN_TIME    =  2,
  COLUMN_PERM    =  3,
  COLUMN_OCTPERM =  4
};

enum {
  LISTING_MODE_FULL,
  LISTING_MODE_BRIEF,
  LISTING_MODE_MEDIUM
};

////
// Item of file panel

typedef struct {
  short          type;       // Type of panel - name.size,m.time,etc
  unsigned short width;      // Width of column;
  unsigned short orig_width; // Original width of column;
  wchar_t        *title;  
} file_panel_column_t;

typedef struct {
  file_panel_column_t *data; // Columns' descriptors
  unsigned short      count; // Count of clumns
} file_panel_columns_t;

typedef struct {
  file_t  *file;      // Default file descriptor
  void    *user_data; // User-specified data for
                      // more flexibility
} file_panel_item_t;

typedef struct {
  WIDGET_MEMBERS
  panel_t panel;  // Panel of layout to manipulate with visibility

  // Data for scrolling
  unsigned long scroll_top;     // Index of start item on page
  unsigned int  items_per_page;

  // Fonts
  scr_font_t *font;             // Default font
  scr_font_t *border_font;      // Font for border
  scr_font_t *focused_dir_font; // Font for directory label on focused panel
  scr_font_t *caption_font;     // Font for column caption
} file_panel_widget_t;

// Simple file panel action
typedef int (*file_panel_action)      (void *__panel);
// File panel action with data
typedef int (*file_panel_data_action) (void *__panel, void *__data);

////
// File panel descriptor
typedef struct {
  // Widget of file panel which task is
  // draw panel on screen and caught messages from user
  file_panel_widget_t *widget;

  // Use this complex structure because I don't want to use
  // static array becase this will limit max. length of CWD and
  // I dislike this limitation in Midnight Commander
  struct {
    wchar_t            *data;
    unsigned int       allocated_length;
  } cwd;    // Current working directory

  struct {
    file_panel_item_t  *data;
    unsigned long      length;
    unsigned long      current; // Index of currently selected item
  } items;       // Items in this panel

  BOOL                   focused; // Is panel focused
  file_panel_columns_t   columns; // Columns to display

  unsigned int       flags;   // Different flags.
                              // Low-byte is reserved for internal usage

  unsigned int       listing_mode; // Listing mode

  // Actionson on the panel
  struct {
    file_panel_action      collect_items;  // Fills items list
    file_panel_action      free_items;     // Frees items list
    file_panel_action      item_user_data_deleter; // Deleter of item's
                                                   // user-defined data
    file_panel_action      draw_items;     // Draws list of items
    file_panel_action      onrefresh;      // Action after re-filling item list
                                           // and before drawing a panel
    file_panel_action      onresize;       // Action after re-filling item list
                                           // and before drawing a panel
    file_panel_data_action keydown_handler;// Handles keyboard input
    file_panel_data_action scroll_to_item; // Set cursor to item and
                                           // scroll to view
    file_panel_data_action centre_to_item; // Set cursor to item and
                                           // centres view
  } actions;
} file_panel_t;

////////
// Common stuff

int            // Initialise file panels
file_panels_init                  (widget_t *__parent);

void           // Uninitialise file panels
file_panels_done                  (void);

int            // Set the panel's CWD
file_panel_set_cwd                (file_panel_t *__panel,
                                   const wchar_t *__cwd);

void           // Set focus to file panel
file_panel_set_focus              (file_panel_t *__panel);

int            // Refresh file panel
file_panel_refresh                (file_panel_t *__panel);

int            // Draw a panel
file_panel_draw                   (file_panel_t *__panel);

int           // Redraw panel
file_panel_redraw                 (file_panel_t *__panel);

int
file_panel_set_columns            (file_panel_t  *__panel,
                                   const wchar_t *__columns);

int            // Rescan file panel
file_panel_rescan                 (file_panel_t *__panel);

unsigned long // Return item index by item name
file_panel_item_index_by_name     (file_panel_t  *__panel,
                                   const wchar_t *__name,
                                   BOOL          *__found);

void
file_panel_set_listing_mode       (file_panel_t *__panel, int __mode);

void
file_panel_update_columns_widths  (file_panel_t *__panel);

////////
// Default actions

int            // Intialize file panels' default actions stuff
file_panel_defact_init            (void);

void           // Unintialize file panels' default actions stuff
file_panel_defact_done            (void);

int
file_panel_defact_collect_items   (file_panel_t *__panel);

int
file_panel_defact_free_items      (file_panel_t *__panel);

int            // Draw a file panel's widget
file_panel_defact_draw_widget     (file_panel_widget_t *__panel_widget);

int
file_panel_defact_widget_destructor(file_panel_widget_t *__widget);

int            // Default action to draw a list of panel's items
file_panel_defact_draw_item_list  (file_panel_t *__panel);

void           // Walk on file panel's widget
file_panel_defact_walk            (file_panel_t *__panel, short __direction);

int            // Handles an on_refresh action of panel
file_panel_defact_onrefresh       (file_panel_t *__panel);

int
file_panel_defact_onresize        (file_panel_t *__panel);

int
file_panel_defact_keydown_handler (file_panel_t *__panel, wchar_t *__ch);

void
file_panel_defact_centre_to_item  (file_panel_t *__panel, wchar_t *__name);

void
file_panel_defact_scroll_to_item  (file_panel_t *__panel, wchar_t *__name);

END_HEADER

#endif
