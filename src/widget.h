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

/********
 * Constants
 */

/* Widget's types */
#define WT_NONE       0x00

#define WT_CONTAINER  0x10
#define WT_WINDOW     (WT_CONTAINER+1)
#define WT_BOX        (WT_CONTAINER+2)
#define WT_BOX_ITEM   (WT_CONTAINER+3)

#define WT_SINGLE     0x30
#define WT_BUTTON     (WT_SINGLE+1)
#define WT_EDIT       (WT_SINGLE+2)
#define WT_MENU       (WT_SINGLE+3)
#define WT_CHECKBOX   (WT_SINGLE+4)
#define WT_TEXT       (WT_SINGLE+5)
#define WT_PROGRESS   (WT_SINGLE+6)

/* Window show modes */
#define WSM_DEFAULT 0x00
#define WSM_MODAL   0x01

/* Window styles */
#define WMS_CENTERED 0x01

/* Button styles */
#define WBS_NONE    0x0000
#define WBS_DEFAULT 0x0001

/* Menu styles */
#define WMS_NONE           0x0000
#define WMS_HIDE_UNFOCUSED 0x0001

/* Sub-menu's item styles */
#define SMI_NONE           0x0000
#define SMI_SEPARATOR      0x0001

/* Box styles */
#define WBS_VERTICAL       0x0000
#define WBS_HORISONTAL     0x0001

/* Modal results */
#define MR_NONE   0x0000
#define MR_OK     0x0001
#define MR_YES    0x0002
#define MR_CANCEL 0x0003
#define MR_NO     0x0004
#define MR_ABORT  0x0005
#define MR_RETRY  0x0006
#define MR_IGNORE 0x0007
#define MR_SKIP   0x0008

#define MR_CUSTOM 0x0100

/****
 * Widgets' flags
 */

/* If widget has this flag, then it can contain only one child, */
/* which will be resized to the whole paren't size */
#define WF_CONTAINER     0x0001

/* Widget doesn't have its own layout and uses parent's */
/* to draw itself */
#define WF_NOLAYOUT      0x0002

/* Redrawing of widget is locked */
#define WF_REDRAW_LOCKED 0x0004

/* Widget is always ontop */
/* This means that messages like onresize will be send */
/* to such widgets after sending those messages to widgets without */
/* this flag */
#define WF_ONTOP         0x0008

/* Widget can't be focused */
#define WF_UNFOCUSABE    0x0010

/********
 * Macroses
 */

/****
 * Typecasts
 */
#define WIDGET(_w)            ((widget_t*)_w)
#define WIDGET_CONTAINER(_w)  ((w_container_t*)_w)
#define WIDGET_WINDOW(_w)     ((w_window_t*)_w)
#define WIDGET_BUTTON(_w)     ((w_button_t*)_w)

/****
 * Common widget's macrodefs
 */
#define WIDGET_TYPE(_w)      ((WIDGET (_w))->type)
#define WIDGET_LAYOUT(_w)    ((WIDGET (_w))->layout)
#define WIDGET_POSITION(_w)  ((WIDGET (_w))->position)

#define WIDGET_SHORTCUT(_w)  ((WIDGET (_w))->shortcut_key)
#define WIDGET_USER_DATA(_w) ((WIDGET (_w))->user_data)

#define WIDGET_VISIBLE(_w)   ((WIDGET_POSITION (_w)).z>0)

#define WIDGET_METHOD(_w, _m)         (WIDGET (_w))->methods._m
#define WIDGET_CALLBACK(_w, _cb)      (WIDGET (_w))->callbacks._cb
#define WIDGET_USER_CALLBACK(_w,_cb)  (WIDGET (_w))->user_callbacks._cb

#define WIDGET_FLAGS(_w)              ((WIDGET (_w))->flags)
#define WIDGET_SET_FLAG(_w, _f)       ((WIDGET (_w))->flags|=_f)
#define WIDGET_RESET_FLAG(_w, _f)     ((WIDGET (_w))->flags&=~_f)
#define WIDGET_TEST_FLAG(_w, _f)      ((WIDGET (_w))->flags&_f)

/* Safe calling of callback */
#define WIDGET_CALL_CALLBACK(_w,_cb,_args...) \
  (WIDGET_CALLBACK (_w, _cb)?WIDGET_CALLBACK (_w, _cb) (_args):0)

/* Call user-defined callback in deep-core callbacks */
#define _WIDGET_CALL_USER_CALLBACK(_w,_cb,_args...) \
  { \
    int res; \
    if (WIDGET_USER_CALLBACK (_w, _cb) && \
       (res=WIDGET_USER_CALLBACK (_w, _cb) (_args))) \
      return res; \
  }

/****
 * Some automatization
 */

/**
 * Initialize new widget object
 * @param _widget - pointer to emery where object will be created
 * @param _datatype - datatype of widget (to determine size of object)
 * @param _parent - parent of widget
 * @param _destructor - function-destructor
 * @param _drawer - function-drawer
 * @param _x, _y, _z - coordinates of widget
 * @param _w, _h - dimensions of widget
 */
#define WIDGET_INIT(_widget, _datatype, _type, _parent, \
                    _flags, \
                    _destructor, _drawer, \
                    _x, _y, _z, _w, _h) \
  /* Allocate and free memory for new edit box */ \
  MALLOC_ZERO (_widget, sizeof (_datatype)); \
\
  (_widget)->type  = _type; \
  (_widget)->flags = _flags; \
\
  /* Set methods */ \
  (_widget)->methods.destroy = (widget_action)_destructor; \
  (_widget)->methods.draw    = (widget_action)_drawer; \
\
  /* Need this assignments here because this data */ \
  /* is needed for widget_create_layout() */ \
  (_widget)->parent=WIDGET (_parent); \
\
  (_widget)->position.x      = _x; \
  (_widget)->position.y      = _y; \
  (_widget)->position.z      = _z; \
  (_widget)->position.width  = _w; \
  (_widget)->position.height = _h; \
\
  /* Create layout for window */ \
  (_widget)->layout=widget_create_layout (WIDGET (_widget)); \
\
  WIDGET_CALLBACK (_widget, focused)  = (widget_action)widget_focused; \
  WIDGET_CALLBACK (_widget, blured)   = (widget_action)widget_blured; \
  WIDGET_CALLBACK (_widget, shortcut) = (widget_action)widget_shortcut; \
  WIDGET_CALLBACK (_widget, keydown)  = (widget_keydown_proc)widget_keydown; \
  WIDGET_CALLBACK (_widget, onresize) = (widget_action)widget_onresize;

/* Post-initializing stuff */
#define WIDGET_POST_INIT(_widget) \
  /* Register widget in container */ \
  w_container_append_child (WIDGET_CONTAINER (_widget->parent), \
    WIDGET (_widget));

/****
 * Container-based macrodefs
 */
#define WIDGET_IS_CONTAINER(_w) \
  (WIDGET_TYPE (_w)>=WT_CONTAINER && WIDGET_TYPE (_w)<WT_SINGLE)

#define WIDGET_CONTAINER_DATA(_w)   ((WIDGET_CONTAINER (_w))->container.data)
#define WIDGET_CONTAINER_LENGTH(_w) ((WIDGET_CONTAINER (_w))->container.length)
#define WIDGET_CONTAINER_FOCUSED(_w)((WIDGET_CONTAINER (_w))->focused_widget)

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
    widget_t *__iterator_; \
    for (i=0; i<n; ++i) \
      { \
        __iterator_=WIDGET_CONTAINER_DATA (_w)[i]; \
        _iterator_action (__iterator_, ##_args); \
      } \
  }

#define WIDGET_CONTAINER_DELETER(_w) \
  WIDGET_CONTAINER_ACTION_ITERONLY (_w, widget_destroy)

#define _WIDGET_CONTAINER_DRAWER_ITER(_w) \
  if (!_w->focused) \
    widget_draw (_w);

#define WIDGET_CONTAINER_DRAWER(_w) \
  WIDGET_CONTAINER_ACTION_ITERONLY (_w, _WIDGET_CONTAINER_DRAWER_ITER); \
  if (WIDGET_IS_CONTAINER (_w) && WIDGET_CONTAINER(_w)->focused_widget) \
    widget_draw (WIDGET_CONTAINER(_w)->focused_widget);

/****
 * Button's macrodefs
 */
#define WIDGET_BUTTON_MODALRESULT(_w)   ((WIDGET_BUTTON (_w))->modal_result)

/****
 * Deep-core macroses
 */
#define WIDGET_SAFE_SET_FONT(_w,_font,_val) if (_val) _w->_font=_val;

/****
 * Type definitions
 */

typedef int (*widget_action) (void *__widget);
typedef int (*widget_keydown_proc) (void *__widget, wint_t __ch);

/* Position of widget */
typedef struct
{
  int x, y;
  int width, height;

  /* If widget is visible z>0. If z=0 then widget is invisible */
  int z;
} widget_position_t;

/* Methods container for simple widgets */
typedef struct
{
  widget_action destroy;
  widget_action draw;
} widget_methods_t;

/* Callbacks for simple widgets */
typedef struct
{
  widget_keydown_proc keydown;
  widget_action shortcut;
  widget_action focused;
  widget_action blured;
  widget_action onresize;
} widget_callbacks_t;

/* Callbacks' structure for user's bindings */
typedef struct
{
  widget_keydown_proc keydown;
  widget_action clicked;
  widget_action shortcut;
  widget_action focused;
  widget_action blured;
  widget_action onresize;
} widget_user_callbacks_t;

/* Basic widget's members */
#define WIDGET_MEMBERS \
  unsigned short          type;            /* Type of widget  */ \
  unsigned long           flags;           /* Different flags of widget */ \
  widget_methods_t        methods;         /* Methods of widget */ \
  widget_callbacks_t      callbacks;       /* Callbacks of widget */ \
 \
  /* Callbacks of widget for user's usage */ \
  widget_user_callbacks_t user_callbacks;  \
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

/* Simplest widget */
typedef struct _widget_t
{
  WIDGET_MEMBERS
} widget_t;

/* Widget with container */
typedef struct
{
  WIDGET_CONTAINER_MEMBERS
} w_container_t;

/* Window */
typedef struct
{
  /* Inherit from widget container */
  WIDGET_CONTAINER_MEMBERS

  /* Panel of layout to manipulate with visibility */
  panel_t panel;

  /* Font for text on window */
  scr_font_t *font;

  struct
  {
    /* Text of caption */
    wchar_t *text;

    /* Font of caption */
    scr_font_t *font;
  } caption;

  unsigned int style;

  /* Some deep-core info */
  unsigned short show_mode;
  int modal_result;
} w_window_t;

/* Button */
typedef struct
{
  /* Inherit from widget */
  WIDGET_MEMBERS

  /* Caption in button */
  wchar_t *caption;

  /* Font for normal style */
  scr_font_t *font;

  /* Font for hotkey in normal state */
  scr_font_t *focused_font;

  /* Font for shortcut style */
  scr_font_t *hot_font;

  /* Font for hotkey in normal state */
  scr_font_t *hot_focused_font;

  unsigned int style;

  /* Modal result code for window */
  unsigned short modal_result;
} w_button_t;

/* Text */
typedef struct
{
  /* Inherit from widget */
  WIDGET_MEMBERS

  /* Text */
  wchar_t *text;

  /* Font of text */
  scr_font_t *font;
} w_text_t;

/* Progress bar */
typedef struct
{
  /* Inherit from widget */
  WIDGET_MEMBERS

  /* Maximal position */
  unsigned long max_pos;

  /* Current position */
  unsigned long cur_pos;

  scr_font_t *font;
  scr_font_t *background_font;
  scr_font_t *progress_font;
} w_progress_t;

/* Edit box */
typedef struct
{
  /* Inherit from widget */
  WIDGET_MEMBERS

  struct
  {
    /* Caption in edit box */
    wchar_t *data;

    /* Allocated buffer size */
    size_t allocated;
  } text;

  /* How much characters where scrolled */
  /* due to widget's width is limited */
  size_t scrolled;

  /* Position of caret */
  size_t caret_pos;

  /* Font for normal style */
  scr_font_t *font;
} w_edit_t;

/* Checkbox */
typedef struct
{
  WIDGET_MEMBERS

  scr_font_t *font;
  scr_font_t *font_focus;

  scr_font_t *hotkey_font;
  scr_font_t *hotkey_focus;

  wchar_t *caption;
  BOOL ischeck;
} w_checkbox_t;

/****
 * Menus
 */

struct w_menu_t_entry;
typedef int (*menu_item_callback) (void *__user_data);

typedef struct
{
  /* Caption of item */
  wchar_t *caption;

  wchar_t shortcut;
  unsigned int flags;
  void *user_data;
  menu_item_callback callback;
} w_sub_menu_item_t;

typedef struct
{
  /* Caption of sub-menu */
  wchar_t *caption;

  wchar_t shortcut;

  struct
  {
    w_sub_menu_item_t *data;
    unsigned short length;
  } items;

  /* Menu which owns this submenu */
  struct w_menu_t_entry *menu;

  /* Index of submenu in parent's container */
  short index;

  /* Position of sub-menu */
  widget_position_t position;

  /* Index of currently selected item */
  short cur_item_index;
} w_sub_menu_t;

typedef struct w_menu_t_entry
{
  /* Inherit from widget */
  WIDGET_MEMBERS

  /* Panel of layout to manipulate with visibility */
  panel_t panel;

  /* Font to draw menu line */
  scr_font_t *font;

  /* Font to draw shortcut */
  scr_font_t *hot_font;

  /* Font to draw submenu caption in case it's focused */
  scr_font_t *focused_font;

  /* Font to draw focused shortcut */
  scr_font_t *hot_focused_font;

  /* Style of menu */
  unsigned int style;

  struct
  {
    w_sub_menu_t *data;
    unsigned int length;
  } sub_menus;

  /****
   * Internal usage
   */

  /* Pointer to currently selected submenu */
  w_sub_menu_t *cur_submenu;

  /* Layout to draw a submemu's items */
  scr_window_t submenu_layout;

  /* Panel to manipulate with submenu's layout */
  panel_t submenu_panel;

  /* Shows if any sub-menu is unfolded */
  BOOL unfolded;

  /* Is menu activate? */
  BOOL active;
} w_menu_t;

/****
 * Boxes
 */

typedef struct
{
  /* Inherit from container */
  WIDGET_CONTAINER_MEMBERS

  /* Size of item */
  int size;
} w_box_item_t;

typedef struct
{
  /* Inherit from container */
  WIDGET_CONTAINER_MEMBERS

  /* Panel of layout to manipulate with visibility */
  panel_t panel;

  /* Style of box */
  unsigned int style;

  /* System info */

  /* If sizes of item were evaluated? */
  BOOL evaluted;
} w_box_t;

/********
 * Deep-core stuff
 */

void
widget_main_loop (void);

void
widget_process_char (wint_t __ch);

void
widget_process_queue (void);

int
widgets_init (void);

void
widgets_done (void);

/* Totally destroying of widget */
void
widget_destroy (widget_t *__widget);

void
widget_add_root (widget_t *__widget);

void
widget_delete_root (widget_t *__widget);

void
widget_sink_root (widget_t *__widget);

/* Draw widget on screen */
int
widget_draw (widget_t *__widget);

/* Redraw widget on screen */
int
widget_redraw (widget_t *__widget);

/* Totally redraw all widgets */
void
widget_full_redraw (void);

/* Call this method when root screen has been resized */
void
widget_on_scr_resize (void);

/* Some helpers */

/* Extracts shortcut key from text */
wchar_t
widget_shortcut_key (const wchar_t *__text);

/* Length of text with shortcuts */
int
widget_shortcut_length (const wchar_t *__text);

/* Print text with highlighted shortcut key */
void
widget_shortcut_print (scr_window_t __layout,
                       const wchar_t *__text,
                       scr_font_t __font, scr_font_t __hot_font);

void
widget_set_focus (widget_t *__widget);

widget_t *
widget_next_focused (const widget_t *__widget);

widget_t *
widget_prev_focused (const widget_t *__widget);

void
widget_resize (widget_t *__widget, int __x, int __y, int __w, int __h);

scr_window_t
widget_create_layout (widget_t *__widget);

void
widget_lock_redraw (widget_t *__widget);

void
widget_unlock_redraw (widget_t *__widget);

/* Return first focusable widget in container */
widget_t*
widget_first_focusable (const w_container_t* __parent);

/****
 * Deep-core common stuff
 */
int
widget_focused (widget_t *__widget);

int
widget_blured (widget_t *__widget);

int
widget_shortcut (widget_t *__widget);

int
widget_onresize (widget_t *__widget);

int
widget_keydown (widget_t *__widget, int __ch);

/********
 * Per-widget stuff
 */

/****
 * Container
 */

void
w_container_append_child (w_container_t *__container, widget_t *__widget);

void
w_container_insert_child (w_container_t *__container, widget_t *__widget,
                          unsigned int __pos);

void
w_container_delete (w_container_t *__widget, widget_t *__child);

void
w_container_drop (w_container_t *__widget, widget_t *__child);

widget_t*
w_container_widget_by_tab_order (const w_container_t *__container,
                                 int __tab_order);

/****
 * Window
 */

/* Deep-core stuff */
void
w_window_end_modal (w_window_t *__window, int __modal_result);

/* Creates new window */
w_window_t*
widget_create_window (const wchar_t *__caption, int __x, int __y,
                      int __w, int __h, unsigned int __style);

/* Show window */
void
w_window_show (w_window_t *__window);

/* Show modal window */
int
w_window_show_modal (w_window_t *__window);

/* Hide window */
void
w_window_hide (w_window_t *__window);

void
w_window_set_fonts (w_window_t *__window, scr_font_t *__font,
                    scr_font_t *__caption_font);

/******
 * Checkbox
 */

w_checkbox_t *
widget_create_checkbox (w_container_t *__parent, const wchar_t *__caption,
                        int __x, int __y, BOOL __check);

BOOL
w_checkbox_get (const w_checkbox_t *__checkbox);

/****
 * Button
 */

w_button_t*
widget_create_button (w_container_t *__parent, const wchar_t *__caption,
                      int __x, int __y, unsigned int __style);

void
w_button_set_fonts (w_button_t *__button,
                    scr_font_t *__font,
                    scr_font_t *__focused_font,
                    scr_font_t *__hot_font,
                    scr_font_t *__hot_focused_font);

/****
 * Menus
 */
w_menu_t*
widget_create_menu (unsigned int __style);

w_sub_menu_t*
w_menu_append_submenu (w_menu_t *__menu, const wchar_t *__caption);

void
w_menu_hide (w_menu_t *__menu);

w_sub_menu_item_t*
w_submenu_append_item (w_sub_menu_t *__sub_menu, const wchar_t *__caption,
                       menu_item_callback __callback, unsigned int __flags);

/****
 * Edit boxes
 */
w_edit_t*
widget_create_edit (w_container_t *__parent, int __x, int __y, int __width);

void
w_edit_set_text (w_edit_t* __exit, wchar_t *__text);

wchar_t*
w_edit_get_text (w_edit_t* __edit);

void
w_edit_set_fonts (w_edit_t *__edit, scr_font_t *__font);

/****
 * Boxes
 */
w_box_t*
widget_create_box (w_container_t *__parent, int __x, int __y, int __w, int __h,
                   unsigned int __style, unsigned int __count);

void
w_box_set_item_szie (w_box_t *__box, unsigned int __index, int __size);

w_box_item_t*
w_box_item (w_box_t *__box, unsigned int __index);

w_box_item_t*
w_box_insert_item (w_box_t *__box, unsigned int __index, int __size);

w_box_item_t*
w_box_append_item (w_box_t *__box, int __size);

void
w_box_delete_item (w_box_t *__box, w_box_item_t *__item);

void
w_box_delete_by_index (w_box_t *__box, unsigned int __index);

/****
 * Text
 */
w_text_t*
widget_create_text (w_container_t *__parent, const wchar_t *__text,
                    int __x, int __y);

void
w_text_set_font (w_text_t *__text, scr_font_t *__font);

void
w_text_set (w_text_t *__w_text, const wchar_t *__text);


/****
 * Progress bar
 */

w_progress_t*
widget_create_progress (w_container_t *__parent, unsigned long __max_pos,
                        int __x, int __y, int __w);

void
w_progress_set_font (w_progress_t *__progress,
                     scr_font_t *__font,
                     scr_font_t *__background_font,
                     scr_font_t *__progress_font);

void
w_progress_set_pos (w_progress_t *__progress, unsigned long __pos);

void
w_progress_set_max (w_progress_t *__progress, unsigned long __max);

#endif
