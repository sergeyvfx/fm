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
#include "file_panel-defact.h"
#include "hook.h"

#include <vfs/vfs.h>

/********
 * Variables
 */

static deque_t *panels = NULL;
static file_panel_t *last_focused = NULL;
static w_box_t *panels_grid;
static int panels_count = 0;

/* Count of panels which will be created */
/* during initialization of panels stuff */
static int default_panels_count = 2;

file_panel_t *current_panel = NULL;

/********
 * Some helpful macroses
 */

/* Macros for panels initialization which creates new panel */
/* and returns from file_panels_init with error */
#define SPAWN_NEW_PANEL() \
  if (!file_panel_create ()) \
    { \
      return -1; \
    }

/* Default set of columns for panel
 *
 * name - name of file
 * size - size of file
 * mtime - last modification time
 * atime - access time
 * ctime - change time
 * perm - permissions of file in format rwxrwxrwx
 * octperm - permissions in octal format
 */
#define DEFAULT_FULL_ROW_MASK L"name size mtime"

/* Name of default virtual file system */
#define DEFAULT_VFS_NAME      VFS_LOCALFS_PLUGIN

/* Directory name of default CWD */
#define DEFAULT_CWD L"/"

/********
 *
 */

static struct
{
  wchar_t *token;
  int type;
} column_parser_info[] = {
  {L"name",    COLUMN_NAME},
  {L"size",    COLUMN_SIZE},
  {L"mtime",   COLUMN_MTIME},
  {L"atime",   COLUMN_ATIME},
  {L"ctime",   COLUMN_CTIME},
  {L"perm",    COLUMN_PERM},
  {L"octperm", COLUMN_OCTPERM},
  {0,          COLUMN_UNKNOWN}
};

/* Non-localized titles of columns */
static wchar_t *orig_column_titles[] = {
  L"Name",
  L"Size",
  L"MTime",
  L"ATime",
  L"CTime",
  L"Perm",
  L"Perm"
};

/* Non-recalculated widths of columns */
static unsigned short orig_columns_widths[] = {
  0,
  9,
  12,
  12,
  12,
  10,
  4
};

/****
 * Forward definitions
 */

/* Keydown callback for file panel widget */
static int
file_panel_keydown (file_panel_widget_t *__panel_widget, wint_t __ch);

static int
file_panel_onresize (file_panel_widget_t *__panel_widget);

static int
file_panel_focused (file_panel_widget_t *__panel_widget);

/********
 *
 */

/**
 * Zerolize file panel's columns
 *
 * @param __columns - columns to bew zerolized
 */
void
zerolize_columns (file_panel_columns_t *__columns)
{
  if (!__columns)
    {
      return;
    }

  /* Free data */
  SAFE_FREE (__columns->data);

  __columns->count = 0;
}

/**
 * Validate widths of columns
 *
 * @param __panel - for wich panel columns will be validated
 */
static void
validate_colums_widths (file_panel_t *__panel)
{
  int i, ptr;
  int total_width = 0;
  int max;
  int fit_width;

  for (i = 0; i < __panel->columns.count; i++)
    {
      if (__panel->columns.data[i].width < 2)
        {
          __panel->columns.data[i].width = 2;
        }
      total_width += __panel->columns.data[i].width;
    }

  fit_width = __panel->widget->position.width - 2 - __panel->columns.count + 1;
  while (total_width > fit_width)
    {
      max = -1;
      ptr = -1;
      for (i = 0; i < __panel->columns.count; i++)
        if (max < 0 || __panel->columns.data[i].width > max)
          {
            ptr = i;
            max = __panel->columns.data[i].width;
          }
      if (ptr < 0 || __panel->columns.data[ptr].width <= 2)
        {
          break;
        }

      __panel->columns.data[ptr].width--;
      total_width--;
    }
}

/**
 * Parser iteration for parse_columns_mask()
 *
 * @param __data - string to parse
 * @param __token - pointer to string where save token
 * @param __max_len - max length of token
 * @return pinter to unparsed part of string
 */
static wchar_t*
columns_mask_parser_iterator (wchar_t *__data, wchar_t *__token,
                              size_t __max_len)
{
  size_t len = 0;

  if (!__data || !__token || !*__data)
    {
      return 0;
    }

  /* Skip spaces */
  while (*__data > 0 && *__data <= ' ')
    {
      __data++;
    }

  /* Collect characters to string while there is no space character */
  while (*__data > ' ')
    {
      /* Token is too long */
      if (len >= __max_len - 1)
        {
          break;
        }

      __token[len++] = *__data;
      __data++;
    }
  __token[len] = 0;

  return __data;
}

/**
 * Parse a mask of columns and creates a columns_t structure
 *
 * @param __panel - panel for which mask is parsing
 * @param __mask - mask of columns
 * will be stored
 * @return zero on success, non-zero if failed
 */
static int
parse_columns_mask (file_panel_t *__panel, const wchar_t *__mask)
{
  wchar_t *shift, token[1024];
  int column_type, i;
  int unused_width, delta;
  int unset_count = 0, reset_count = 0;

  file_panel_columns_t *columns;

  /* Invalid pointers */
  if (!__panel || !__panel->widget || !__mask)
    {
      return -1;
    }

  columns = &__panel->columns;

  shift = (wchar_t*) __mask;

  unused_width = __panel->widget->position.width - 2;

  zerolize_columns (columns);

  /* Get next token */
  while ((shift = columns_mask_parser_iterator (shift, token, 1024)))
    {
      column_type = COLUMN_UNKNOWN;

      i = 0;

      /* Get type of column */
      while (column_parser_info[i].type != COLUMN_UNKNOWN)
        {
          if (!wcscmp (token, column_parser_info[i].token))
            {
              column_type = column_parser_info[i].type;
              break;
            }
          i++;
        }

      /* If column type is known */
      if (column_type != COLUMN_UNKNOWN)
        {
          wchar_t *title;

          /* Allocate memory for new column */
          columns->data = realloc (columns->data,
                                   sizeof (file_panel_column_t)*
                                          (columns->count + 1));

          /* Get original title f column */
          title = orig_column_titles[column_type];

          /*
           * TODO: Add localization stuff here
           */

          /* Fill fields of new column */
          columns->data[columns->count].type = column_type;

          columns->data[columns->count].width =
                  columns->data[columns->count].orig_width =
                  orig_columns_widths[column_type];

          columns->data[columns->count].title = title;

          if (!orig_columns_widths[column_type])
            {
              unset_count++;
            }

          unused_width -= orig_columns_widths[column_type];

          columns->count++;
        }
      else
        {
          /* Otherwise free allocated memory and return error */
          columns->count = 0;
          SAFE_FREE (columns->data);
          return -1;
        }
    }

  /* Subtract column separators */
  unused_width -= (columns->count - 1);

  if (unused_width < 0)
    {
      unused_width = 0;
    }

  if (unset_count)
    {
      delta = (unused_width - unset_count) / unset_count;
    }

  /* Set width of previously unset columns */
  for (i = 0; i < columns->count; i++)
    {
      if (!columns->data[i].width)
        {
          columns->data[i].width =
                  (reset_count == unset_count - 1) ? unused_width : delta;
          reset_count++;
          unused_width -= delta;
        }
    }

  validate_colums_widths (__panel);

  return 0;
}

/**
 * Refill items of panel
 *
 * @param __panel - panel for which items are refilling
 * @return zero on success, non-zero if failed
 */
static int
refill_items (file_panel_t *__panel)
{
  /* Free existing list of items */
  if (__panel->actions.free_items)
    {
      /*
       * TODO: But maybe we should abort refreshing if
       *       there is an error while freeing list of items?
       */
      __panel->actions.free_items (__panel);
    }

  /* Update list of items */
  if (__panel->actions.collect_items)
    {
      if (__panel->actions.collect_items (__panel))
        {
          /* Error getting list of items */
          return -1;
        }
    }
  else
    {
      return -1;
    }

  return 0;
}

/**
 * Set default action to panel
 *
 * @param __panel - panel for which set default actions
 */
static void
set_default_actions (file_panel_t *__panel)
{
  SET_PANEL_ACTION (__panel, create, fpd_create);
  SET_PANEL_ACTION (__panel, destroy, fpd_destroy);

  SET_PANEL_ACTION (__panel, collect_items, fpd_collect_items);
  SET_PANEL_ACTION (__panel, free_items, fpd_free_items);
  SET_PANEL_ACTION (__panel, item_user_data_deleter, 0);
  SET_PANEL_ACTION (__panel, draw_items, fpd_draw_item_list);
  SET_PANEL_ACTION (__panel, onrefresh, fpd_onrefresh);
  SET_PANEL_ACTION (__panel, onresize, fpd_onresize);

  SET_PANEL_DATA_ACTION (__panel, keydown_handler, fpd_keydown_handler);

  /* Used in such stuff, as cwd_sink() when directory is changed to */
  /* parent and cursor should be placed onto item, from which we come. */

  /* In such situation it would be better if this item */
  /* will be at the center of visible part of list. */
  SET_PANEL_DATA_ACTION (__panel, centre_to_item, fpd_centre_to_item);

  /* Will be helpful for actions like incremental search, */
  /* because in difference with "centre_to_item" it will */
  /* make less scrolling. */
  SET_PANEL_DATA_ACTION (__panel, scroll_to_item, fpd_scroll_to_item);

  SET_PANEL_DATA_ACTION (__panel, fill_submenu, fpd_fill_submenu);

  SET_PANEL_ACTION (__panel, save_selection, fpd_save_selection);
  SET_PANEL_ACTION (__panel, restore_selection, fpd_restore_selection);
  SET_PANEL_ACTION (__panel, free_saved_selection, fpd_free_saved_selection);
}

/**
 * Set default parameters of new-created file panel
 *
 * @param __panel - panel which has to be initialized
 */
static void
set_default_params (file_panel_t *__panel)
{
  file_panel_set_listing_mode (__panel, LISTING_MODE_MEDIUM);
  file_panel_set_vfs (__panel, DEFAULT_VFS_NAME);
  file_panel_set_cwd (__panel, DEFAULT_CWD);
  file_panel_set_columns (__panel, DEFAULT_FULL_ROW_MASK);
}

/**
 * Spawn new file panel
 *
 * @return new spawned panel or NULL if creation failed
 */
static file_panel_t*
file_panel_create (void)
{
  file_panel_t *res;
  BOOL prev;
  widget_t *parent;

  /* Allocate memory for panel and its widget */
  MALLOC_ZERO (res, sizeof (file_panel_t));

  parent = WIDGET (w_box_append_item (panels_grid, -1));

  /* Fill up the actions */
  set_default_actions (res);
  FILE_PANEL_ACTION_CALL (res, create);

  /* Set file panel number */
  res->number = panels_count;

  /****
   * General widget initialization
   */
  WIDGET_INIT (res->widget, file_panel_widget_t, WT_SINGLE, parent, 0,
               fpd_widget_destructor,
               fpd_draw_widget,
               0, 0, 1, 0, 0);

  /* Set up panels fonts */
  res->widget->font             = &FONT (CID_CYAN, CID_BLUE);
  res->widget->border_font      = &FONT (CID_CYAN, CID_BLUE);
  res->widget->focused_dir_font = &FONT (CID_BLACK, CID_CYAN);
  res->widget->caption_font     = &FONT (CID_YELLOW, CID_BLUE);

  /* Save pointer to file panel descriptor in userdata in widget */
  WIDGET_USER_DATA (WIDGET (res->widget)) = res;

  /* Fill up callbacks */
  WIDGET_CALLBACK (WIDGET (res->widget), keydown) =
          (widget_keydown_proc) file_panel_keydown;

  WIDGET_CALLBACK (WIDGET (res->widget), onresize) =
          (widget_action) file_panel_onresize;

  WIDGET_CALLBACK (WIDGET (res->widget), focused) =
          (widget_action) file_panel_focused;

  WIDGET_POST_INIT (res->widget);

  set_default_params (res);

  prev = deque_head (panels) != NULL;

  ++panels_count;

  if (panels_count == 3)
    {
      /* Need this because while there were less than three panels */
      /* they didn't draw theri numbers. */
      widget_redraw (WIDGET (panels_grid));
    }

  deque_push_back (panels, res);

  if (!prev)
    {
      file_panel_set_focus (res);
    }
  else
    {
      file_panel_redraw (res);
    }

  return res;
}

/**
 * Destroy a file panel
 *
 * @param __panel - panel to destroy
 * @return zero on success, non-zero if failed
 */
static void
file_panel_destructor (void *__panel)
{
  if (!__panel)
    {
      return;
    }

  file_panel_t *panel = __panel;

  FILE_PANEL_ACTION_CALL (panel, destroy);

  --panels_count;

  /* Delete widget from widget tree (destructor will be applied) */
  w_container_delete (WIDGET_CONTAINER (panel->widget->parent),
                      WIDGET (panel->widget));

  /* Free CWD string */
  if (panel->cwd.data)
    {
      free (panel->cwd.data);
    }

  /* Free existing list of items */
  if (panel->actions.free_items)
    {
      panel->actions.free_items (panel);
    }

  zerolize_columns (&panel->columns);

  free (panel);

  return;
}

/**
 * Get data from config
 *
 * @return zero on success, non-zero if failed
 */
static int
read_config (void)
{
  /* Set default configuration */

  /*
   * TODO: Add getting data from config here
   */

  return 0;
}

/**
 * Common part of handling keydown event
 *
 * @param __panel - panel which received a keydown event
 * @param __ch - pointer to received character (not string!)
 * @return zero if callback hasn't handled received character
 *   non-zero otherwise
 */
static int
file_panel_comm_keydown_handler (file_panel_t *__panel, const wchar_t *__ch)
{
  if (!__panel || FILE_PANEL_TEST_FLAG (__panel, FPF_UNFOCUSABLE))
    {
      return 0;
    }

  switch (*__ch)
    {
      /* Navigation */
    case KEY_DOWN:
      fpd_walk (__panel, WALK_NEXT);
      break;
    case KEY_UP:
      fpd_walk (__panel, WALK_PREV);
      break;
    case KEY_NPAGE:
      fpd_walk (__panel, WALK_NEXT_PAGE);
      break;
    case KEY_PPAGE:
      fpd_walk (__panel, WALK_PREV_PAGE);
      break;
    case KEY_HOME:
      fpd_walk (__panel, WALK_HOME);
      break;
    case KEY_END:
      fpd_walk (__panel, WALK_END);
      break;
    case KEY_LEFT:
      fpd_walk (__panel, WALK_PREV_COLUMN);
      break;
    case KEY_RIGHT:
      fpd_walk (__panel, WALK_NEXT_COLUMN);
      break;
    default:
      return 0;
    }

  return 1;
}

/********
 * Callbacks
 */

/**
 * Keydown callback for file panel widget
 *
 * @param __panel_widget - widget which received this callback
 * @param __ch - received character
 * @return zero if callback hasn't handled received character
 */
static int
file_panel_keydown (file_panel_widget_t *__panel_widget, wint_t __ch)
{
  if (!__panel_widget || !WIDGET_USER_DATA (__panel_widget))
    {
      return -1;
    }

  file_panel_t *panel = WIDGET_USER_DATA (__panel_widget);

  if (panel->actions.keydown_handler)
    {
      int res;
      if (!(res = panel->actions.keydown_handler (panel, &__ch)))
        {
          return res;
        }
    }

  /* Lets think that if user-defined handler hadn't handled */
  /* action we should execute default keydown handler */
  return file_panel_comm_keydown_handler (panel, (wchar_t*) & __ch);
}

/**
 * Onresize callback for file panel widget
 *
 * @param __panel_widget - widget which received this callback
 * @return zero if callback hasn't handled received character
 */
static int
file_panel_onresize (file_panel_widget_t *__panel_widget)
{
  if (!__panel_widget)
    {
      return -1;
    }

  /* Call default widget's handler */
  widget_onresize (WIDGET (__panel_widget));

  if (WIDGET_USER_DATA (__panel_widget))
    {
      file_panel_t *panel = WIDGET_USER_DATA (__panel_widget);
      if (panel->actions.onresize)
        {
          panel->actions.onresize (panel);
        }
    }

  return 0;
}

/**
 * Focused handler for file panel widget
 *
 * @param __panel_widget - widget which received this callback
 * @return zero if callback hasn't handled received character
 */
static int
file_panel_focused (file_panel_widget_t *__panel_widget)
{
  if (WIDGET_TEST_FLAG (__panel_widget, 0x1000))
    {
      return 0;
    }

  WIDGET_SET_FLAG (__panel_widget, 0x1000);
  file_panel_set_focus (WIDGET_USER_DATA (__panel_widget));
  WIDGET_RESET_FLAG (__panel_widget, 0x1000);

  return 1;
}

/********
 * User's backend
 */

/**
 * Initialize file panels
 *
 * @param __parent - parent widget in which file panels' grid
 * will be integrated
 * @return zero on success and non-zero if operation failed
 */
int
file_panels_init (widget_t *__parent)
{
  int i;

  if (
      /* Get data from configuration */
      read_config () ||

      /* Initialize file panels' default actions stuff */
      fpd_init ()
      )
    {
      return -1;
    }

  /* Create grid to manage panels' position */
  panels_grid = widget_create_box (WIDGET_CONTAINER (__parent),
                                   0, 0, 0, 0, WBS_VERTICAL, 0);

  panels = deque_create ();

  /* Create panels */
  for (i = 0; i < default_panels_count; i++)
    {
      SPAWN_NEW_PANEL ();
    }

  return 0;
}

/**
 * Uninitialize file panels
 */
void
file_panels_done (void)
{
  /* Destroy all panels */
  deque_destroy (panels, file_panel_destructor);

  /* Delete grid from widget tree */
  if (panels_grid)
    {
      w_container_delete (WIDGET_CONTAINER (panels_grid->parent),
                          WIDGET (panels_grid));
    }

  fpd_done ();
}

/**
 * Set name of virtual file system
 *
 * @param __panel - panel where set the VFS
 * @param __vfs - name of virtual file system
 * @return zero on success, non-zero otherwise
 */
int
file_panel_set_vfs (file_panel_t *__panel, const wchar_t *__vfs)
{
  if (!__panel || !__vfs)
    {
      return -1;
    }

  if (__panel->vfs)
    {
      free (__panel->vfs);
    }

  __panel->vfs = wcsdup (__vfs);

  return 0;
}

/**
 * Set the panel's CWD
 *
 * @param __panel - panel where set the CWD
 * @param __cwn - CWD to set
 * @return zro on success, non-zero otherwise
 */
int
file_panel_set_cwd (file_panel_t *__panel, const wchar_t *__cwd)
{
  int len;
  if (!__panel || !__cwd)
    {
      return -1;
    }

  /* If new CWD longer than allocated buffer realloc this buffer*/
  if ((len = wcslen (__cwd)) >= __panel->cwd.allocated_length)
    {
      __panel->cwd.data = realloc (__panel->cwd.data,
                                   (len + 1) * sizeof (wchar_t));
      memset (__panel->cwd.data, 0, (len + 1) * sizeof (wchar_t));
      __panel->cwd.allocated_length = len + 1;
    }

  /* Copy new data */
  wcscpy (__panel->cwd.data, __cwd);

  if (__panel == current_panel)
    {
      hook_call (L"cwd.changed", (void*) __cwd);
    }

  return file_panel_refresh (__panel);
}

/**
 * Set focus to file panel
 *
 * @param __panel - panel to be focused
 */
void
file_panel_set_focus (file_panel_t *__panel)
{
  if (!__panel)
    {
      return;
    }

  /* Check if panel can't be focused */
  if (FILE_PANEL_TEST_FLAG (__panel, FPF_UNFOCUSABLE))
    {
      return;
    }

  /* Clear focused flag of previous focused file panel */
  /* and redraw this panel */
  if (last_focused && last_focused != __panel)
    {
      last_focused->focused = last_focused->widget->focused = FALSE;

      if (__panel->actions.onrefresh)
        __panel->actions.onrefresh (__panel);

      file_panel_redraw (last_focused);
    }

  /* Focus new panel and redraw it */
  __panel->focused = TRUE;

  if (__panel->actions.onrefresh)
    {
      __panel->actions.onrefresh (__panel);
    }

  widget_set_focus (WIDGET (__panel->widget));

  current_panel = __panel;

  /* Store last focused panel */
  last_focused = __panel;
}

/**
 * Draw a panel
 *
 * @param __panel - panel to draw
 * @return zero on success, non-zero if failed
 */
int
file_panel_draw (file_panel_t *__panel)
{
  if (!__panel || !__panel->widget)
    {
      return -1;
    }

  return widget_draw (WIDGET (__panel->widget));
}

/**
 * Redraw a panel
 *
 * @param __panel - panel to draw
 * @return zero on success, non-zero if failed
 */
int
file_panel_redraw (file_panel_t *__panel)
{
  if (!__panel || !__panel->widget)
    {
      return -1;
    }

  return widget_redraw (WIDGET (__panel->widget));
}

/**
 * Refresh file panel (update list and redraw)
 *
 * @param __panel - panel to refresh
 * @return zero on success, non-zero if failed
 */
int
file_panel_refresh (file_panel_t *__panel)
{
  int res;
  if (!__panel)
    {
      return -1;
    }

  __panel->items.selected_count = 0;

  if ((res = refill_items (__panel)))
    {
      return res;
    }

  /* Call onrefresh action */
  FILE_PANEL_ACTION_CALL (__panel, onrefresh);

  file_panel_draw (__panel);

  return 0;
}

/**
 * Rescan file panel
 *
 * @param __panel - panel to be rescanned
 * @return zero on success, non-zero if failed
 */
int
file_panel_rescan (file_panel_t *__panel)
{
  int res;

  FILE_PANEL_ACTION_CALL (__panel, save_selection);

  if ((res = refill_items (__panel)))
    {
      return res;
    }

  FILE_PANEL_ACTION_CALL (__panel, restore_selection);

  /* Call onrefresh action */
  FILE_PANEL_ACTION_CALL (__panel, onrefresh);

  FILE_PANEL_ACTION_CALL (__panel, free_saved_selection);

  file_panel_redraw (__panel);

  return 0;
}

/**
 * Set set of columns to file panel displayed in full-row mode
 *
 * @param __panel - panel for which set set of columns
 * @param __columns - string which describes set of columns
 */
int
file_panel_set_columns (file_panel_t *__panel, const wchar_t *__columns)
{
  int res;
  res = parse_columns_mask (__panel, __columns);
  if (!res)
    {
      file_panel_draw (__panel);
    }
  return res;
}

/**
 * Return item index by item name
 *
 * @param __panel - panel where search item
 * @param __name - name of item to search
 * @param __found - if pointer is not null then this parameter
 * will be set to TRUE if item has been found and to FALSE otherwise
 * @return index of found item or -1 if item hadn't been found
 */
unsigned long
file_panel_item_index_by_name (file_panel_t *__panel, const wchar_t *__name,
                               BOOL *__found)
{
  unsigned long i, n;

  if (__found)
    {
      (*__found) = FALSE;
    }

  if (!__name)
    {
      return 0;
    }

  n = __panel->items.length;

  for (i = 0; i < n; i++)
    if (!wcscmp (__panel->items.data[i].file->name, __name))
      {
        if (__found)
          {
            (*__found) = TRUE;
          }
        return i;
      }

  return 0;
}

/**
 * Set listing mode to panel
 *
 * @param __panel - file panel for which set mode
 * @param __mode - listing mode to set
 */
void
file_panel_set_listing_mode (file_panel_t *__panel, int __mode)
{
  if (!__panel)
    {
      return;
    }

  __panel->listing_mode = __mode;

  file_panel_rescan (__panel);
  file_panel_update_columns_widths (__panel);
  file_panel_redraw (__panel);
}

/**
 * Update widths of columns
 *
 * @param __panel - panel for which widths are updating
 */
void
file_panel_update_columns_widths (file_panel_t *__panel)
{
  int i, unused_width, delta;
  int unset_count = 0, reset_count = 0;

  file_panel_columns_t *columns;

  /* Invalid pointers */
  if (!__panel || !__panel->widget)
    {
      return;
    }

  columns = &__panel->columns;

  unused_width = __panel->widget->position.width - 2;

  /* reset widths of columns to original */
  for (i = 0; i < columns->count; i++)
    {
      unused_width -= columns->data[i].orig_width;
      columns->data[i].width = columns->data[i].orig_width;
      if (!columns->data[i].orig_width)
        {
          unset_count++;
        }
    }

  /* Subtract column separators */
  unused_width -= (columns->count - 1);

  if (unused_width < 0)
    {
      unused_width = 0;
    }

  if (unset_count)
    {
      delta = (unused_width - unset_count) / unset_count;
    }

  /* Set width of previously unset columns */
  for (i = 0; i < columns->count; i++)
    {
      if (!columns->data[i].width)
        {
          columns->data[i].width =
                  (reset_count == unset_count - 1) ? unused_width : delta;
          reset_count++;
          unused_width -= delta;
        }
    }

  validate_colums_widths (__panel);
}

/**
 * Return count of file panels
 *
 * @return count of panels
 */
int
file_panel_get_count (void)
{
  return panels_count;
}

/**
 * Return list of file panels
 *
 * @return list of file panels
 */
deque_t*
file_panel_get_list (void)
{
  return panels;
}

/**
 * Return full URL to panel's CWD
 * Returned buffer must be freed
 *
 * @return url to CWD
 */
wchar_t*
file_panel_get_full_cwd (file_panel_t *__panel)
{
  wchar_t *res;
  size_t len;

  if (!__panel)
    {
      return NULL;
    }

  if (wcscmp (__panel->vfs, VFS_LOCALFS_PLUGIN) == 0)
    {
      return wcsdup (__panel->cwd.data);
    }

  /* Length of URL */
  len = wcslen (__panel->vfs) + wcslen (__panel->cwd.data) +
          wcslen (VFS_PLUGIN_DELIMETER) + 1;

  /* Allocate memory */
  res = malloc (len * sizeof (wchar_t));

  swprintf (res, len, L"%ls%ls%ls", __panel->vfs, VFS_PLUGIN_DELIMETER,
            __panel->cwd.data);

  return res;
}

/**
 * Get list of selected items.
 * If there is no selected items item under cursor will be added to the list
 *
 * NOTE: Items are NOT duplicated in this function
 *
 * @param __panal - from which panel selected items will be gotten
 * @param __items - pointer to an array where selected items where stored.
 * Should be freed after usage
 * @return count of selected items
 */
unsigned long
file_panel_get_selected_items (file_panel_t *__panel,
                               file_panel_item_t *** __items)
{
  if (!__panel || !__panel->items.length || !__items)
    {
      return 0;
    }

  if (__panel->items.selected_count)
    {
      unsigned long i, j;
      file_panel_item_t *cur;

      /* Allocate memory */
      (*__items) = malloc (__panel->items.selected_count *
                           sizeof (file_panel_item_t*));

      /* Collect list of selected items */
      for (i = 0, j = 0; i < __panel->items.length; ++i)
        {
          cur = &__panel->items.data[i];
          if (cur->selected)
            {
              /* Add current item to list */
              (*__items)[j++] = cur;
            }
        }

      return __panel->items.selected_count;
    }
  else
    {
      /* Add the only item under cursor */

      /* Allocate memory */
      (*__items) = malloc (sizeof (file_panel_item_t*));

      /* Save pointer of item under cursor */
      (*__items)[0] = &__panel->items.data[__panel->items.current];

      return 1;
    }
}
