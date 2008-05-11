/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _widget_h_
#define _widget_h_

#include "smartinclude.h"

#include <wctype.h>
#include <wchar.h>

#include "screen.h"

////////
// Constants

// Widget's types
#define WT_NONE       0x00

#define WT_CONTAINER  0x10
#define WT_WINDOW     (WT_CONTAINER+1)
#define WT_MENU       (WT_CONTAINER+2)

#define WT_SINGLE     0x30
#define WT_BUTTON     (WT_SINGLE+1)
#define WT_EDIT       (WT_SINGLE+2)

// Window show modes
#define WSM_DEFAULT 0x00
#define WSM_MODAL   0x01

// Button styles
#define WBS_NONE    0x0000
#define WBS_DEFAULT 0x0001

// Menu styles
#define WMS_NONE           0x0000
#define WMS_HIDE_UNFOCUSED 0x0001

// Sub-menu's item styles
#define SMI_NONE           0x0000
#define SMI_SEPARATOR      0x0001

// Modal results
#define MR_NONE   0x0000
#define MR_OK     0x0001
#define MR_YES    0x0002
#define MR_CANCEL 0x0003
#define MR_NO     0x0004
#define MR_ABORT  0x0005
#define MR_RETRY  0x0006
#define MR_IGNORE 0x0007
#define MR_SKIP   0x0008

////////
// Macroses

////
// Typecasts
#define WIDGET(_w)            ((widget_t*)_w)
#define WIDGET_CONTAINER(_w)  ((w_container_t*)_w)
#define WIDGET_WINDOW(_w)     ((w_window_t*)_w)
#define WIDGET_BUTTON(_w)     ((w_button_t*)_w)

////
// Common widget's macrodefs
#define WIDGET_TYPE(_w)      ((WIDGET (_w))->type)
#define WIDGET_LAYOUT(_w)    ((WIDGET (_w))->layout)
#define WIDGET_POSITION(_w)  ((WIDGET (_w))->position)

#define WIDGET_SHORTCUT(_w)  ((WIDGET (_w))->shortcut_key)
#define WIDGET_USER_DATA(_w) ((WIDGET (_w))->user_data)

#define WIDGET_VISIBLE(_w)   ((WIDGET_POSITION (_w)).z>0)

#define WIDGET_METHOD(_w, _m)         (WIDGET (_w))->methods._m
#define WIDGET_CALLBACK(_w, _cb)      (WIDGET (_w))->callbacks._cb
#define WIDGET_USER_CALLBACK(_w,_cb)  (WIDGET (_w))->user_callbacks._cb

// Safe calling to callback
#define WIDGET_CALL_CALLBACK(_w,_cb,_args...) \
  (WIDGET_CALLBACK (_w, _cb)?WIDGET_CALLBACK (_w, _cb) (_args):0)

// Call user-defined callback in deep-core callbacks
#define _WIDGET_CALL_USER_CALLBACK(_w,_cb,_args...) \
  { \
    int res; \
    if (WIDGET_USER_CALLBACK (_w, _cb) && (res=WIDGET_USER_CALLBACK (_w, _cb) (_args))) \
      return res; \
  }

////
// Container-based macrodefs
#define WIDGET_IS_CONTAINER(_w)      (WIDGET_TYPE (_w)>=WT_CONTAINER && WIDGET_TYPE (_w)<WT_SINGLE)
#define WIDGET_CONTAINER_DATA(_w)    ((WIDGET_CONTAINER (_w))->container.data)
#define WIDGET_CONTAINER_LENGTH(_w)  ((WIDGET_CONTAINER (_w))->container.length)
#define WIDGET_CONTAINER_FOCUSED(_w) ((WIDGET_CONTAINER (_w))->focused_widget)

#define WIDGET_CONTAINER_ACTION_FULL(_w, _iterator_action, _post_action) \
  if (WIDGET_IS_CONTAINER (_w)) { \
    unsigned int i, n=WIDGET_CONTAINER_LENGTH (_w); \
    for (i=0; i<n; ++i) \
      _iterator_action (WIDGET_CONTAINER_DATA (_w)[i]); \
    _post_action (WIDGET_CONTAINER_DATA (_w)); \
  }

#define WIDGET_CONTAINER_ACTION_ITERONLY(_w, _iterator_action,_args...) \
  if (WIDGET_IS_CONTAINER (_w)) { \
    unsigned int i, n=WIDGET_CONTAINER_LENGTH (_w); \
    for (i=0; i<n; ++i) \
      _iterator_action (WIDGET_CONTAINER_DATA (_w)[i], ##_args); \
  }

#define WIDGET_CONTAINER_DELETER(_w) \
  WIDGET_CONTAINER_ACTION_FULL (_w, widget_destroy, SAFE_FREE)

#define WIDGET_CONTAINER_DRAWER(_w) \
  WIDGET_CONTAINER_ACTION_ITERONLY (_w, widget_draw)

////
// Button's macrodefs
#define WIDGET_BUTTON_MODALRESULT(_w)        ((WIDGET_BUTTON (_w))->modal_result)

////
// Deep-core macroses
#define WIDGET_SAFE_SET_FONT(_w,_font,_val) if (_val) _w->_font=_val;

////////
// Type defenitions

typedef int (*widget_action)    (void *__widget);
typedef int (*widget_keydown)   (void *__widget, wint_t __ch);

// Position of widget
typedef struct {
  int x, y;
  int width, height;
  int z; // If widget is visible z>0. If z=0 then widget is invisible
} widget_position_t;

// Methods container for simple widgets
typedef struct {
  widget_action destroy;
  widget_action draw;
} widget_methods_t;

// Callbacks for simple widgets
typedef struct {
  widget_keydown  keydown;
  widget_action   shortcut;
  widget_action   focused;
  widget_action   blured;
} widget_callbacks_t;

// Callbacks' structure for user's bindings
typedef struct {
  widget_keydown  keydown;
  widget_action   clicked;
  widget_action   shortcut;
  widget_action   focused;
  widget_action   blured;
} widget_user_callbacks_t;

// Basic widget's members
#define WIDGET_MEMBERS \
  unsigned short          type;            /* Type of widget  */ \
  widget_methods_t        methods;         /* Methods of widget */ \
  widget_callbacks_t      callbacks;       /* Callbacks of widget */ \
  widget_user_callbacks_t user_callbacks;  /* Callbacks of widget for user's usage */ \
  scr_window_t       layout;          /* Layout where the widgets draws */ \
  widget_position_t  position;        /* Position of widget */ \
  BOOL               focused;         /* Is widget focused  */ \
  struct _widget_t   *parent;         /* Parent widget */ \
  int                tab_order;       /* Tab order */ \
  wchar_t            shortcut_key;    /* Shortcut key of widget */ \
  void               *user_data;      /* To keep user-specified data */

#define WIDGET_CONTAINER_MEMBERS \
  WIDGET_MEMBERS \
  struct { \
    widget_t      **data; \
    unsigned int  length;       /* Current length */ \
    unsigned int  alloc_length; /* Allocated length */ \
  } container; \
  widget_t        *focused_widget; /* Ptr to focused widget */

// Simpliest widget
typedef struct _widget_t {
  WIDGET_MEMBERS
} widget_t;

// Widget with container
typedef struct {
  WIDGET_CONTAINER_MEMBERS
} w_container_t;

// Window
typedef struct {
  // Inherit from widget container
  WIDGET_CONTAINER_MEMBERS

  panel_t            panel;  // Panel of layout to manipulate with visibility

  scr_font_t        *font;   // Font for text on window

  struct {
    wchar_t         *text;  // Text of caption
    scr_font_t      *font;  // Font of caption
  } caption;

  // Some deep-core info
  unsigned short show_mode;
  int            modal_result;
} w_window_t;

// Button
typedef struct {
  // Inherit from widget
  WIDGET_MEMBERS

  wchar_t            *caption;          // Caption in button

  scr_font_t         *font;             // Font for normal style
  scr_font_t         *focused_font;     // Font for hotkey in normal state

  scr_font_t         *hot_font;         // Font for shortcut style
  scr_font_t         *hot_focused_font; // Font for hotkey in normal state

  unsigned int      style;
  unsigned short    modal_result;       // Modal result code for window
} w_button_t;

// Edit box
typedef struct {
  // Inherit from widget
  WIDGET_MEMBERS

  struct {
    wchar_t         *data;              // Caption in edit box
    size_t          allocated;          // Allocated bufefr size
  } text;

  size_t            scrolled;           // How much characters where scrolled
                                        // due to widget's width is limited
  size_t            caret_pos;          // Position of caret

  scr_font_t         *font;             // Font for normal style
} w_edit_t;

////
// Menus

struct w_menu_t_entry;
typedef int (*menu_item_callback) (void);

typedef struct {
  wchar_t                *caption;  // Caption of sub-menu
  wchar_t                shortcut;

  struct {
    struct {
      wchar_t            *caption; // Caption of item
      wchar_t            shortcut;
      unsigned int       flags;
      menu_item_callback callback;
    } *data;
    unsigned short length;
  } items;

  struct w_menu_t_entry  *menu;     // Menu which owns this submenu
  short                  index;     // Index of submenu in parent's container
  widget_position_t      position;  // Position of sub-menu
  short                  cur_item_index; // Index of currently selected item
} w_sub_menu_t;

typedef struct w_menu_t_entry {
  // Inherit from widget
  WIDGET_MEMBERS

  panel_t            panel; // Panel of layout to manipulate with visibility

  scr_font_t         *font;               // Font to draw menu line
  scr_font_t         *hot_font;           // Font to draw shortcut
  scr_font_t         *focused_font;       // Font to draw submenu caption in case
                                          // it's focused
  scr_font_t         *hot_focused_font;   // Font to draw focused shortcut

  unsigned int       style;               // Style of menu

  struct {
    w_sub_menu_t *data;
    unsigned int length;
  } sub_menus;

  ////
  // Internal usage
  w_sub_menu_t       *cur_submenu; // Pointer to currently selected submenu

  scr_window_t       submenu_layout; // Layout to draw a submemu's items
  panel_t            submenu_panel;  // Panel to manipulate with submenu's layout
  BOOL               unfolded;       // Shows if any sub-menu is unfolded
  BOOL               active;         // Is menu activate?
} w_menu_t;

////////////////
//

//////
// Deep-core stuff

// Code to operate with non-modal windows
/*void           // Set widget where to send messages from user
widget_set_current_widget         (widget_t *__widget);

void           // Main loop of widget stuff to manipulate with messages from user
widget_main_loop                  (void);*/

//////
// Common stuff

void           // Totally destroing of widget
widget_destroy                    (widget_t *__widget);

int            // Draw widget on screen
widget_draw                       (widget_t *__widget);

void           // Redraw widget on screen
widget_redraw                     (widget_t *__widget);

void           // Totally redraw all widgets
widget_full_redraw                (void);

void           // Call this method when root screen has been resized
widget_on_scr_resize              (void);

// Some helpers

wchar_t       // Extracts shortcut key from etxt
widget_shortcut_key               (const wchar_t *__text);

int           // Length of text with shortcuts
widget_shortcut_length            (const wchar_t *__text);

void           // Print text with highlighted shortcut key
widget_shortcut_print             (scr_window_t __layout,
                                   const wchar_t *__text,
                                   scr_font_t __font, scr_font_t __hot_font);

void
widget_set_focus                  (widget_t *__widget);

widget_t *
widget_next_focused               (const widget_t *__widget);

widget_t *
widget_prev_focused               (const widget_t *__widget);

////
// Deep-core common stuff
int
widget_focused                    (widget_t *__widget);

int
widget_blured                     (widget_t *__widget);

/////
// Per-widget stuff

////
// Container

void
w_container_append_child          (w_container_t *__container,
                                   widget_t *__widget);

widget_t*
w_container_widget_by_tab_order   (const w_container_t *__container,
                                   int __tab_order);

////
// Window

// Deep-core stuff
void
w_window_end_modal                (w_window_t *__window, int __modal_result);

//
w_window_t*    // Creates new window
widget_create_window              (const wchar_t *__caption,
                                   int __x, int __y,
                                   int __w, int __h);

void           //  Show window
w_window_show                     (w_window_t *__window);

int            //  Show modal window
w_window_show_modal               (w_window_t *__window);

void           // Hide window
w_window_hide                     (w_window_t *__window);

void
w_window_set_fonts                (w_window_t *__window, scr_font_t *__font,
                                   scr_font_t *__caption_font);

////
// Button

w_button_t*
widget_create_button              (w_container_t *__parent,
                                   const wchar_t *__caption,
                                   int __x, int __y,
                                   unsigned int __style);

void
w_button_set_fonts                (w_button_t *__button,
                                   scr_font_t *__font,
                                   scr_font_t *__focused_font,
                                   scr_font_t *__hot_font,
                                   scr_font_t *__hot_focused_font);

////
// Menus
w_menu_t*
widget_create_menu                (unsigned int __style);

w_sub_menu_t*
w_menu_append_submenu             (w_menu_t *__menu, const wchar_t *__caption);

void
w_submenu_append_item             (w_sub_menu_t       *__sub_menu,
                                   const wchar_t      *__caption,
                                   menu_item_callback  __callback,
                                   unsigned int        __flags);

////
// Edit boxes
w_edit_t*
widget_create_edit                (w_container_t *__parent,
                                   int __x, int __y, int __width);

void
w_edit_set_text                   (w_edit_t* __exit, wchar_t *__text);

wchar_t*
w_edit_get_text                   (w_edit_t* __edit);

void
w_edit_set_fonts                  (w_edit_t *__edit, scr_font_t *__font);

#endif
