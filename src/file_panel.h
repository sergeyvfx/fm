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
#include "deque.h"

#include <wchar.h>

/********
 * Constants
 */

/* Panel can't be focused */
#define FPF_UNFOCUSABLE    0x0001

/********
 * Macroses
 */

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

/********
 * Type definitions
 */

/* Walking directions */
enum
{
  WALK_NEXT,
  WALK_PREV,
  WALK_NEXT_PAGE,
  WALK_PREV_PAGE,
  WALK_HOME,
  WALK_END,
  WALK_NEXT_COLUMN,
  WALK_PREV_COLUMN
};

/* Types of columns */
enum
{
  COLUMN_UNKNOWN = -1,
  COLUMN_NAME = 0,
  COLUMN_SIZE,
  COLUMN_MTIME,
  COLUMN_ATIME,
  COLUMN_CTIME,
  COLUMN_PERM,
  COLUMN_OCTPERM
};

enum
{
  LISTING_MODE_FULL,
  LISTING_MODE_BRIEF,
  LISTING_MODE_MEDIUM
};

/***
 * Item of file panel
 */

typedef struct
{
  /* Type of panel - name,size,mtime,etc.. */
  short type;

  /* Width of column */
  unsigned short width;

  /* Original width of column */
  unsigned short orig_width;

  wchar_t *title;
} file_panel_column_t;

typedef struct
{
  /* Columns' descriptors */
  file_panel_column_t *data;

  /* Count of columns */
  unsigned short count;
} file_panel_columns_t;

typedef struct
{
  /* Default file descriptor */
  file_t *file;

  /* User-specified data for more flexibility */
  void *user_data;
} file_panel_item_t;

typedef struct
{
  WIDGET_MEMBERS

  /* Panel of layout to manipulate with visibility */
  panel_t panel;

  /** Data for scrolling */
  /* Index of start item on page */
  unsigned long scroll_top;
  unsigned int items_per_page;

  /** Fonts */
  /* Default font */
  scr_font_t *font;

  /* Font for border */
  scr_font_t *border_font;

  /* Font for directory label on focused panel */
  scr_font_t *focused_dir_font;

  /* Font for column caption */
  scr_font_t *caption_font;
} file_panel_widget_t;

/* Simple file panel action */
typedef int (*file_panel_action) (void *__panel);

/* File panel action with data */
typedef int (*file_panel_data_action) (void *__panel, void *__data);

/***
 * File panel descriptor
 */

typedef struct
{
  /* Widget of file panel which task is */
  /* draw panel on screen and caught messages from user */
  file_panel_widget_t *widget;

  /* Name of virtual file system */
  wchar_t *vfs;

  /* Use this complex structure because I don't want to use */
  /* static array because this will limit max. length of CWD and */
  /* I dislike this limitation in Midnight Commander */
  struct
  {
    wchar_t *data;
    unsigned int allocated_length;
  } cwd; /* Current working directory */

  struct
  {
    file_panel_item_t *data;
    unsigned long length;

    /* Index of currently selected item */
    unsigned long current;
  } items; /* Items in this panel */

  /* Is panel focused */
  BOOL focused;

  /* Columns to display */
  file_panel_columns_t columns;

  /* Different flags. Low-byte is reserved for internal usage */
  unsigned int flags;

  /* Listing mode */
  unsigned int listing_mode;

  /* Data for internal panel's stuff usage */
  void* user_data;

  /* Actions on the panel */
  struct
  {
    /***
     * Construcotr/destructor
     */
    file_panel_action create;
    file_panel_action destroy;

    /***
     * Misc.
     */
    /* Fills items list */
    file_panel_action collect_items;

    /* Frees items list */
    file_panel_action free_items;

    /* Deleter of item's user-defined data */
    file_panel_action item_user_data_deleter;

    /* Draws list of items */
    file_panel_action draw_items;

    /* Set cursor to item and scroll to view */
    file_panel_data_action scroll_to_item;

    /* Set cursor to item and centres view */
    file_panel_data_action centre_to_item;

    /***
     * Handlers
     */

    /* Handles keyboard input */
    file_panel_data_action keydown_handler;

    /***
     * Events
     */

    /* Action after re-filling item list and before drawing a panel */
    file_panel_action onrefresh;

    /* Action after re-sizing and before drawing a panel */
    file_panel_action onresize;

    /***
     * iface
     */

    /* Creates items in specified submneu */
    file_panel_data_action fill_submenu;
  } actions;
} file_panel_t;

/********
 * Global variables
 */
extern file_panel_t *current_panel;

/********
 * Common stuff
 */

/* Initialize file panels */
int
file_panels_init (widget_t *__parent);

/* Uninitialize file panels */
void
file_panels_done (void);

/* Set name of VFS */
int
file_panel_set_vfs (file_panel_t *__panel, const wchar_t *__vfs);

/* Set the panel's CWD */
int
file_panel_set_cwd (file_panel_t *__panel, const wchar_t *__cwd);

/* Set focus to file panel */
void
file_panel_set_focus (file_panel_t *__panel);

/* Refresh file panel */
int
file_panel_refresh (file_panel_t *__panel);

/* Draw a panel */
int
file_panel_draw (file_panel_t *__panel);

/* Redraw panel */
int
file_panel_redraw (file_panel_t *__panel);

int
file_panel_set_columns (file_panel_t *__panel, const wchar_t *__columns);

/* Rescan file panel */
int
file_panel_rescan (file_panel_t *__panel);

/* Return item index by item name */
unsigned long
file_panel_item_index_by_name (file_panel_t *__panel, const wchar_t *__name,
                               BOOL *__found);

void
file_panel_set_listing_mode (file_panel_t *__panel, int __mode);

void
file_panel_update_columns_widths (file_panel_t *__panel);

int
file_panel_get_count (void);

deque_t*
file_panel_get_list (void);

wchar_t*
file_panel_get_full_cwd (file_panel_t *__panel);

/********
 * Default actions
 */

/* Initialize file panels' default actions stuff */
int
file_panel_defact_init (void);

/* Unintialize file panels' default actions stuff */
void
file_panel_defact_done (void);

int
file_panel_defact_create (file_panel_t *__panel);

int
file_panel_defact_destroy (file_panel_t *__panel);

int
file_panel_defact_collect_items (file_panel_t *__panel);

int
file_panel_defact_free_items (file_panel_t *__panel);

/* Draw a file panel's widget */
int
file_panel_defact_draw_widget (file_panel_widget_t *__panel_widget);

int
file_panel_defact_widget_destructor (file_panel_widget_t *__widget);

/* Default action to draw a list of panel's items */
int
file_panel_defact_draw_item_list (file_panel_t *__panel);

/* Walk on file panel's widget */
void
file_panel_defact_walk (file_panel_t *__panel, short __direction);

/* Handles an on_refresh action of panel */
int
file_panel_defact_onrefresh (file_panel_t *__panel);

int
file_panel_defact_onresize (file_panel_t *__panel);

int
file_panel_defact_keydown_handler (file_panel_t *__panel, wchar_t *__ch);

int
file_panel_defact_centre_to_item (file_panel_t *__panel, wchar_t *__name);

int
file_panel_defact_scroll_to_item (file_panel_t *__panel, wchar_t *__name);

int
file_panel_defact_fill_submenu (file_panel_t *__panel,
                                w_sub_menu_t *__submenu);

END_HEADER

#endif
