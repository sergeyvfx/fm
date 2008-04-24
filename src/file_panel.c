/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Common part of file panel's stuff
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "file_panel.h"

////////
// Variables

static file_panel_t *left_panel   = NULL;
static file_panel_t *last_focused = NULL;

////////
// Some helpful macroses

// Macros for panels initialisation which creates new panel
// and returns from file_panels_init with error
#define SPAWN_NEW_PANEL(__panel) \
  if (!(__panel=file_panel_create ())) \
    { \
      return -1; \
    }

// Default set of columns for panel
//
// name - name of file
// size - size of file
// time - last midification time
// perm - permissions of file in format rwxrwxrwx
// octperm - permissions in octal format
//
#define DEFAULT_FULL_ROW_MASK L"name size time"

////////
//

static struct {
  wchar_t *token;
  int     type;
} column_parser_info[]={
  {L"name",    COLUMN_NAME},
  {L"size",    COLUMN_SIZE},
  {L"time",    COLUMN_TIME},
  {L"perm",    COLUMN_PERM},
  {L"octperm", COLUMN_OCTPERM},
  {0, COLUMN_UNKNOWN}
};

// Non-localized titles of columns
static wchar_t *orig_column_titles[]={
    L"Name",
    L"Size",
    L"Time",
    L"Perm",
    L"Perm"
  };

// Non-recalcuated widths of columns
static unsigned short orig_columns_widths[]={
  0,
  11,
  12,
  10,
  4
};

////////
// Forward defenitions

static int     // Keydown callback for file panel widget
file_panel_keydown                (file_panel_widget_t *__panel_widget,
                                   wint_t __ch);

static int
file_panel_defact_widget_destructor(file_panel_widget_t *__widget);

////////
//

/**
 * Zerolizes file panel's columns
 *
 * @param __columns - columns to bew zerolized
 */
void
zerolize_columns                  (file_panel_columns_t *__columns)
{
  if (!__columns)
    return;

  // Free data
  SAFE_FREE (__columns->data);

  __columns->count=0;
}

/**
 *  Parser iteration for parse_columns_mask()
 *
 * @param __data - string to parse
 * @param __token - pointer to string where save token
 * @param __max_len - max length of token
 * @return pinter to unparsed part of string
 */
static wchar_t*
columns_mask_parser_iterator      (wchar_t  *__data,
                                   wchar_t  *__token,
                                   size_t   __max_len)
{
  size_t len=0;

  if (!__data || !__token || !*__data)
    return 0;

  // Skip spaces
  while (*__data>0 && *__data<=' ')
    __data++;

  // Collect characters to string while there is no space character
  while (*__data>' ')
    {
      // Token is too long
      if (len>=__max_len-1)
        break;

      __token[len++]=*__data;
      __data++;
    }
  __token[len]=0;

  return __data;
}

/**
 * Parses a mask of columns and creates a columns_t structure
 *
 * @param __panel - panel for which mask is parsing
 * @param __mask - mask of columns
 * will be stored
 * @return zero on succsess, non-zero if failed
 */
static int
parse_columns_mask                (file_panel_t *__panel,
                                  const wchar_t *__mask)
{
  wchar_t *shift, token[1024];
  int column_type, i;
  int unused_width, delta;
  int unset_count=0, reset_count=0;
  
  file_panel_columns_t *columns;

  // Invalid pointers
  if (!__panel || !__panel->widget || !__mask)
    return -1;

  columns=&__panel->columns;

  shift=(wchar_t*)__mask;

  unused_width=__panel->widget->position.width-2;

  zerolize_columns (columns);

  // Get next token
  while ((shift=columns_mask_parser_iterator (shift, token, 1024)))
    {
      column_type=COLUMN_UNKNOWN;

      i=0;

      // Get type of column
      while (column_parser_info[i].type!=COLUMN_UNKNOWN)
        {
          if (!wcscmp (token, column_parser_info[i].token))
            {
              column_type=column_parser_info[i].type;
              break;
            }
          i++;
        }
      
      // If column type is cnown
      if (column_type!=COLUMN_UNKNOWN)
        {
          wchar_t *title;
          // Allocate memory for new column
          columns->data=realloc (columns->data,
              sizeof (file_panel_column_t)*(columns->count+1));

          // Get original title f column
          title=orig_column_titles[column_type];

          //
          // TODO:
          //  Add localization stuff here
          //

          // Fill fields of new column
          columns->data[columns->count].type=column_type;
          columns->data[columns->count].width=orig_columns_widths[column_type];
          columns->data[columns->count].title=title;

          if (!orig_columns_widths[column_type])
            unset_count++;

          unused_width-=orig_columns_widths[column_type];

          columns->count++;
        } else {
          // Otherwise free allocated memory and return error
          columns->count=0;
          SAFE_FREE (columns->data);
          return -1;
        }
    }

  // Subrtact column separators
  unused_width-=(columns->count-1);
  delta=(unused_width-unset_count)/unset_count;

  // Set width of previously unset columns
  for (i=0; i<columns->count; i++)
    {
      if (!columns->data[i].width)
        {
          columns->data[i].width=
            (reset_count==unset_count-1)?unused_width:delta;
          reset_count++;
          unused_width-=delta;
        }
    }

  return 0;
}

/**
 * Refill items of panel
 *
 * @param __panel - panel for which items are refilling
 * @return zero on succsess, non-zero if failed
 */
static int
refill_items                      (file_panel_t *__panel)
{
  // Free existing list of items
  if (__panel->actions.free_items)
    {
      //
      // TODO:
      //  But maybe we should abort refreshing if
      //  there is an error while freeing list of items?
      //
      __panel->actions.free_items (__panel);
    }

  // Update list of items
  if (__panel->actions.collect_items)
    {
      if (__panel->actions.collect_items (__panel))
          return -1; // Error getting list of items
    } else
      return -1;
  return 0;
}

/**
 * Sets default action to panel
 *
 * @param __panel - panel for which set default actions
 */
static void
set_default_actions               (file_panel_t *__panel)
{
  SET_PANEL_ACTION (__panel, collect_items,  file_panel_defact_collect_items);
  SET_PANEL_ACTION (__panel, free_items,     file_panel_defact_free_items);
  SET_PANEL_ACTION (__panel, item_user_data_deleter, 0);
  SET_PANEL_ACTION (__panel, draw_items,     file_panel_defact_draw_item_list);
  SET_PANEL_ACTION (__panel, on_refresh,     file_panel_defact_on_refresh);
  SET_PANEL_DATA_ACTION (__panel, keydown_handler,
    file_panel_defact_keydown_handler);
}

/**
 * Spawns new file panel
 *
 * @return new spawned panel or NULL if creation failed
 */
static file_panel_t*
file_panel_create                 (void)
{
  file_panel_t *res;

  // Allocate memory for panel and its widget
  MALLOC_ZERO (res, sizeof (file_panel_t));
  MALLOC_ZERO (res->widget, sizeof (file_panel_widget_t));

  ////
  // Widget initialisation

  // Create layout and panel
  WIDGET_LAYOUT (WIDGET (res->widget))=scr_create_window (0, 0,
    SCREEN_WIDTH/2, /*SCREEN_HEIGHT*/15);

  res->widget->panel=panel_new (WIDGET_LAYOUT (WIDGET (res->widget)));

  // Methods of widget
  res->widget->methods.destroy =
    (widget_action)file_panel_defact_widget_destructor;
  res->widget->methods.draw    = (widget_action)file_panel_defact_draw_widget;

  // Position of panel
  res->widget->position.x=0;
  res->widget->position.y=0;
  res->widget->position.width=SCREEN_WIDTH/2;
  res->widget->position.height=/*SCREEN_HEIGHT*/15;

  // Set up panels fonts
  res->widget->font             = &sf_lcyan_on_blue;
  res->widget->border_font      = &sf_lcyan_on_blue;
  res->widget->focused_dir_font = &sf_black_on_cyan;
  res->widget->caption_font     = &sf_yellow_on_blue;

  // Save pointer to file panel descriptor in
  // userdata in widget
  WIDGET_USER_DATA (WIDGET (res->widget))=res;

  // Fill up the actions
  set_default_actions (res);

  // Fill up callbacks
  WIDGET_USER_CALLBACK (WIDGET (res->widget), keydown)=
    (widget_keydown)file_panel_keydown;

  return res;
}

/**
 * Destroys a file panel's widget
 *
 * @param __widget - widget to destroy
 */
static int
file_panel_defact_widget_destructor(file_panel_widget_t *__widget)
{
  if (!__widget)
    return -1;

  // Delete panel associated with layout
  if (__widget->panel)
    panel_del (__widget->panel);

  // Destroy screen layout
  if (WIDGET_LAYOUT (__widget))
      scr_destroy_window (WIDGET_LAYOUT (__widget));

  free (__widget);

  return 0;
}

/**
 * Destroys a file panel
 *
 * @param __panel - panel to destroy
 * @return zero on succsess, non-zero if failed
 */
static int
file_panel_destructor             (file_panel_t *__panel)
{
  if (!__panel)
    return -1;

  // Destroy widget
  widget_destroy (WIDGET (__panel->widget));

  // Free CWD string
  if (__panel->cwd.data)
    free (__panel->cwd.data);

  // Free existing list of items
  if (__panel->actions.free_items)
    __panel->actions.free_items (__panel);

  zerolize_columns (&__panel->columns);

  free (__panel);

  return 0;
}

/**
 * Gets data from config
 *
 * @return zero on succsess, non-zero if failed
 */
static int
read_config                       (void)
{
  // Set default configuration

  //
  // TODO:
  //  Add getting data from config here
  //
  return 0;
}

/**
 * Common part of handling keydown event
 *
 * @param __panel - panel which received a keydown event
 * @param __ch - pointer to received character (not string!)
 * @return zero on succsess, non-zero if failed
 */
static int
file_panel_comm_keydown_handler   (file_panel_t *__panel, const wchar_t *__ch)
{
  if (!__panel || FILE_PANEL_TEST_FLAG (__panel, FPF_UNFOCUSABLE))
    return -1;

  switch (*__ch)
    {
    // Navigation
    case KEY_DOWN:
      file_panel_defact_walk (__panel, WALK_NEXT);
      break;
    case KEY_UP:
      file_panel_defact_walk (__panel, WALK_PREV);
      break;
    case KEY_NPAGE:
      file_panel_defact_walk (__panel, WALK_NEXT_PAGE);
      break;
    case KEY_PPAGE:
      file_panel_defact_walk (__panel, WALK_PREV_PAGE);
      break;
    case KEY_HOME:
      file_panel_defact_walk (__panel, WALK_HOME);
      break;
    case KEY_END:
      file_panel_defact_walk (__panel, WALK_END);
      break;
    case KEY_LEFT:
      file_panel_defact_walk (__panel, WALK_PREV_COLUMN);
      break;
    case KEY_RIGHT:
      file_panel_defact_walk (__panel, WALK_NEXT_COLUMN);
      break;
    default:
      return -1;
    }
  
  return 0;
}

////
// Callbacks

/**
 * Keydown callback for file panel widget
 *
 * @param __panel_widget - widget which received this callback
 * @param __ch - received character
 * @return zero if callback hasn't handled received character
 */
static int
file_panel_keydown                (file_panel_widget_t *__panel_widget,
                                   wint_t __ch)
{
  if (!__panel_widget || !WIDGET_USER_DATA (__panel_widget))
    return -1;

  file_panel_t *panel=WIDGET_USER_DATA (__panel_widget);

  if (panel->actions.keydown_handler)
    {
      int res;
      if (!(res=panel->actions.keydown_handler (panel, &__ch)))
        return res;
    }

  // Lets think that if user-defined handler hadn't hadled
  // action we should execute default keydown handler
  return file_panel_comm_keydown_handler (panel, (wchar_t*)&__ch);
}

////////
// User's backend

/**
 * Initialises file panels
 *
 * @return zero on sucsess and non-zero if operation failed
 */
int
file_panels_init                  (void)
{
  wchar_t ch;

  
  if (
      // Get data from configuration
      read_config () ||           

      // Intialize file panels' default actions stuff
      file_panel_defact_init ())  
    return -1;

  // Create panels
  SPAWN_NEW_PANEL (left_panel);

  file_panel_set_listing_mode (left_panel, LISTING_MODE_MEDIUM);
  file_panel_set_cwd (left_panel, L"/");
  file_panel_set_columns (left_panel, DEFAULT_FULL_ROW_MASK);
  
  // Just for testing
  file_panel_set_focus (left_panel);
  file_panel_refresh (left_panel);
  file_panel_draw (left_panel);

  scr_wnd_keypad (WIDGET_LAYOUT (WIDGET (left_panel->widget)), TRUE);

  while ((ch=scr_wnd_getch (WIDGET_LAYOUT (WIDGET (left_panel->widget))))!='q')
    {
      WIDGET_USER_CALLBACK (WIDGET (left_panel->widget), keydown)(left_panel->widget, ch);
    }

  return 0;
}

/**
 * Uninitialises file panels
 */
void
file_panels_done                  (void)
{
  // Destroy all panels
  file_panel_destructor (left_panel);

  file_panel_defact_done ();
}

/**
 * Sets the panel's CWD
 *
 * @param __panel - panel where set the CWD
 * @param __cwn - CWD to set
 */
int
file_panel_set_cwd                (file_panel_t *__panel,
                                   const wchar_t *__cwd)
{
  int len;
  if (!__panel || !__cwd)
    return -1;

  // If new CWD longer than allocated buffer
  // re-alloc this buffer
  if ((len=wcslen (__cwd))>=__panel->cwd.allocated_length)
    {
      __panel->cwd.data=realloc (__panel->cwd.data, (len+1)*sizeof (wchar_t));
      memset (__panel->cwd.data, 0, (len+1)*sizeof (wchar_t));
      __panel->cwd.allocated_length=len+1;
    }

  // Copy new data
  wcscpy (__panel->cwd.data, __cwd);

  return file_panel_refresh (__panel);
}

/**
 * Sets focus to file panel
 *
 * @param __panel - panel to be focused
 */
void
file_panel_set_focus              (file_panel_t *__panel)
{
  if (!__panel)
    return;

  // Check if panel can't be focused
  if (FILE_PANEL_TEST_FLAG (__panel, FPF_UNFOCUSABLE))
    return;

  // Clear focused flag of previous focused file panel
  // and redraw this panel
  if (last_focused && last_focused!=__panel)
    {
      last_focused->focused=last_focused->widget->focused=FALSE;

      if (__panel->actions.on_refresh)
        __panel->actions.on_refresh (__panel);

      file_panel_draw (last_focused);
    }

  // Focus new panel and redraw it
  __panel->focused=TRUE;

  if (__panel->actions.on_refresh)
    __panel->actions.on_refresh (__panel);

  widget_set_focus (WIDGET (__panel->widget));

  // Store last focused panel
  last_focused=__panel;
}

/**
 * Draws a panel
 *
 * @param __panel - panel to draw
 * @return zero on succsess, non-zero if failed
 */
int
file_panel_draw                   (file_panel_t *__panel)
{
  if (!__panel || !__panel->widget)
    return -1;

  return widget_draw (WIDGET (__panel->widget));
}

/**
 * Refreshs file panel
 *
 * @param __panel - panel to refresh
 * @return zero on succsess, non-zero if failed
 */
int
file_panel_refresh                (file_panel_t *__panel)
{
  int res;
  if (!__panel)
    return -1;

  if ((res=refill_items (__panel)))
    return res;

  // Call to on_refresh action
  if (__panel->actions.on_refresh)
    __panel->actions.on_refresh (__panel);

  file_panel_draw (__panel);

  return 0;
}

/**
 * Rescans file panel
 *
 * @param __panel - panel to be rescaned
 * @return zero on succsess, non-zero if failed
 */
int
file_panel_rescan                 (file_panel_t *__panel)
{
  int res, found;
  unsigned long prev_current;
  wchar_t name[MAX_FILENAME_LEN]={0};

  // Store name of selected file
  prev_current=__panel->items.current;
  if (__panel->items.data)
    wcscpy (name, __panel->items.data[prev_current].file->name);

  if ((res=refill_items (__panel)))
    return res;

  __panel->items.current=file_panel_item_index_by_name (__panel, name, &found);
  if (!found)
    __panel->items.current=prev_current;

  // Call to on_refresh action
  if (__panel->actions.on_refresh)
    __panel->actions.on_refresh (__panel);

  file_panel_draw (__panel);

  return 0;
}

/**
 * Sets set of columns to file panel displayed in full-row mode
 *
 * @param __panel - panel for which set set of columns
 * @param __columns - string which describes set of columns
 */
int
file_panel_set_columns            (file_panel_t  *__panel,
                                   const wchar_t *__columns)
{
  int res;
  res=parse_columns_mask (__panel, __columns);
  if (!res)
    file_panel_draw (__panel);
  return res;
}

/**
 * Returns item index by item name
 *
 * @param __panel - panel where search item
 * @param __name - name of item to search
 * @param __found - if pointer is not null then this parameter
 * will be set to TRUE if item has been found and to FALSE otherwise
 * @return index of found item or -1 if item hadn't been found
 */
unsigned long
file_panel_item_index_by_name     (file_panel_t  *__panel,
                                   const wchar_t *__name,
                                   BOOL          *__found)
{
  unsigned long i, n;

  if (__found)
    (*__found)=FALSE;

  n=__panel->items.length;

  for (i=0; i<n; i++)
    if (!wcscmp (__panel->items.data[i].file->name, __name))
      {
        if (__found)
          (*__found)=TRUE;
        return i;
      }

  return 0;
}

/**
 * Sets listing mode to panel
 *
 * @param __panel - file panel for which set mode
 * @param __mode - listing mode to set
 */
void
file_panel_set_listing_mode       (file_panel_t *__panel, int __mode)
{
  if (!__panel)
    return;

  __panel->listing_mode=__mode;
  file_panel_rescan (__panel);
}
