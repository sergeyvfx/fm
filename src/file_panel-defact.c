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

#include "file_panel.h"
#include "dir.h"
#include "util.h"
#include "messages.h"

#include <time.h>

////////
// Internal stuff

////
// Internal types,constants,etc

// Column aligments
#define CF_ALIGN_LEFT   0x0000
#define CF_ALIGN_CENTER 0x0001
#define CF_ALIGN_RIGHT  0x0010

#define CF_UNCOLORED_DELIMETER 0x0100

#define DIR_CAPTION            L"<Dir>"
#define UPDIR_CAPTION          L"<Up--Dir>"
#define COLUMN_TEXT_TRUNCATOR  L"..."

// Default Formats of date strings for recent and older dates
#define DEFAULT_DATE_FORMAT     L"%b %d %H:%M"
#define DEFAULT_OLD_DATE_FORMAT L"%b %d  %Y"

// Max. length of format string
#define MAX_DATE_FORMAT_LEN 128

// Default timedists of recent date (in days)
#define DEFAULT_RECENT_PAST     (24*30*6)
#define DEFAULT_RECENT_FUTURE   (24*30*6)

// Count of columns for brief listing mode
#define COLUMNS_PER_BRIEF 3

// Count of columns for medium listing mode
#define COLUMNS_PER_MEDIUM 2

////////
//

// Formats of date strings for recent and older dates
static wchar_t date_format[MAX_DATE_FORMAT_LEN];
static wchar_t old_date_format[MAX_DATE_FORMAT_LEN];

// Timedists of recent date (in days)
static int recent_past, recent_future;

////////
//

/**
 * Returns pointer to font which have to be used to
 * draw specified item of file panel
 *
 * @param __panel - file panel to which needed item belongs
 * @param __index - index of item for which get font
 * @return pointer to wanted font
 */
static scr_font_t*
get_file_panel_item_font          (const file_panel_t  *__panel,
                                   unsigned long        __index)
{
  if (__index>=__panel->items.length)
    return &FONT (CID_WHITE, CID_RED);

  file_panel_item_t item=__panel->items.data[__index];

  // Item is under cursor
  if (__panel->items.current==__index && __panel->focused &&
      !FILE_PANEL_TEST_FLAG (__panel, FPF_UNFOCUSABLE))
    return &FONT (CID_BLACK, CID_CYAN);

  if (S_ISDIR (item.file->stat.st_mode) &&
      wcscmp (item.file->name, L".."))
    return &FONT (CID_WHITE, CID_BLUE);

  return __panel->widget->font;
}

/**
 * Prints string in column
 *
 * @param __layout - layout where draw string
 * @param __string - string to be printed
 * @param __width - width of column
 * @param __append_delimeter - if TRUE then column delimeter will appended
 * @param __flags - different additional flags to setup look&feel.
 * This flags can include one of this constants:
 *   CF_ALIGN_LEFT, CF_ALIGN_CENTER, CF_ALIGN_RIGHT.
 */
static void
print_column_string               (scr_window_t    __layout,
                                   const wchar_t *__str,
                                   int            __width,
                                   BOOL           __append_delimeter,
                                   const scr_font_t    *__string_font,
                                   const scr_font_t    *__delimeter_font,
                                   unsigned int  __flags)
{
  int i, n, padding_left=0;
  wchar_t *str;
  size_t len;

  // Unsupported parameters
  if (__width<=0 || !__str)
    return;

  // Get string which will be fit to width of column
  str=wcsfit (__str, __width, COLUMN_TEXT_TRUNCATOR);
  len=wcslen (str);

  // Draw string
  if (__string_font)
    scr_wnd_font (__layout, *__string_font);

  // Apply aligment
  if (__flags & CF_ALIGN_CENTER)
    padding_left=(__width-len)/2; else
  if (__flags & CF_ALIGN_RIGHT)
    padding_left=__width-len;

  // Apply padding (for aligment)
  for (i=0; i<padding_left; i++)
    scr_wnd_putch (__layout, ' ');

  // Print string to column
  scr_wnd_printf (__layout, "%ls", str);

  // Append needed spaces
  for (i=0, n=__width-len-padding_left; i<n; i++)
    scr_wnd_putch (__layout, ' ');

  // Append delimeter
  if (__append_delimeter)
    {
      if (__delimeter_font)
        scr_wnd_font (__layout, *__delimeter_font);
      scr_wnd_putch (__layout, ACS_VLINE);
    }

  free (str);
}

/**
 * Draws headers for columns
 *
 * @param __panel - panel from which draw headers for columns
 * @param __columns - descriptor of columns set
 * @param __layout - layout on which header will be drawed
 * @return zero on succsess, non-zero if failed
 */
static int
draw_columns_headers              (const file_panel_t         *__panel,
                                   const file_panel_columns_t *__columns,
                                   scr_window_t                __layout)
{
  int i, n;
  file_panel_column_t *column;

  // Get layput of widget
  if (!__panel || !__columns || !__layout || !__panel->widget)
    return -1;

  // Move cater to start of row
  scr_wnd_move_caret (__layout, 1, 1);

  // Draw title for each column
  for (i=0, n=__columns->count; i<n; i++)
    {
      column=&__columns->data[i];

      print_column_string (__layout, column->title, column->width, i!=n-1,
        __panel->widget->caption_font, __panel->widget->border_font,
        CF_ALIGN_CENTER);
    }

  return 0;
}

/**
 * Draws row of full-vew table
 *
 * @param __panel - panel for which item is belong to
 * @param __intex - index of item in items list
 * @param __empty - is item empty?
 * This parameter is needed to draw empty rows of table
 * @return zero on succsess, non-zero if failed
 */
static int
draw_full_row                     (const file_panel_t *__panel,
                                   unsigned long       __index,
                                   BOOL                __empty)
{
  int                  i, n,
                       flags,
                       row_number;

  scr_font_t          *item_font;
  wchar_t              pchar[MAX_SCREEN_WIDTH];
  file_panel_column_t *column;
  scr_window_t         layout;
  file_panel_item_t   *item=NULL;

  // Invalid pointers
  if (!__panel || !__panel->widget || !__panel->widget->layout)
    return -1;

  layout=__panel->widget->layout;
  row_number=__index-__panel->widget->scroll_top;

  if (!__empty)
    {
      // Get pointer to item to be drawed
      item=&__panel->items.data[__index];
      // Set font of current item
      item_font=get_file_panel_item_font (__panel, __index);
    } else
      item_font=__panel->widget->font;

  scr_wnd_font (layout, *item_font);

  // Goto needed row
  scr_wnd_move_caret (layout, 1, row_number+2);

  for (i=0, n=__panel->columns.count; i<n; i++)
    {
      column=&__panel->columns.data[i];
      memset (pchar, 0, sizeof (pchar));
      flags=0;

      if (item)
        // Get text for column
        switch (column->type)
          {
          case COLUMN_NAME:
            if (item->file->name)
              // Fit file name to width of column
              fit_filename (item->file->name, column->width, pchar);
            break;
          case COLUMN_SIZE:
            if (S_ISDIR (item->file->stat.st_mode))
              {
                flags=CF_ALIGN_CENTER;
                if (wcscmp (item->file->name, L".."))
                  wcscpy (pchar, DIR_CAPTION); else
                  wcscpy (pchar, UPDIR_CAPTION);
              } else {
                flags=CF_ALIGN_RIGHT;

                //
                // TODO:
                //  Add adding converting to human-readable
                //  format (i.e. 1000M, 900G) here
                //

#ifdef __USE_FILE_OFFSET64
                swprintf (pchar, MAX_SCREEN_WIDTH, L"%lld",
                  item->file->lstat.st_size);
#else
                swprintf (pchar, MAX_SCREEN_WIDTH, L"%lld",
                  item->file->lstat.st_size);
#endif
              }
            break;
            case COLUMN_TIME:
              {
                struct tm tm;
                time_t *t, now;
                wchar_t *format;

                // Get current time_t and time_t of item
                t=&item->file->lstat.st_mtim.tv_sec;
                now=time (0);

                // Convert time_t to struct tm
                gmtime_r (t, &tm);

                // Get format for date string
                if (*t<now)
                  {
                    format=((now-*t)<recent_past*3600)?
                      date_format:old_date_format;
                  } else {
                    format=((*t-now)<recent_future*3600)?
                      date_format:old_date_format;
                  }

                wcsftime (pchar, MAX_SCREEN_WIDTH, format, &tm);
                break;
              }
          case COLUMN_PERM:
            // Convert umask to string
            // NOTE: First char of string is reserved for
            // directory/symlinklink prefix
            umasktowcs (item->file->lstat.st_mode, pchar+1);

            // Append directory/symlink prefix to umask string
            if (S_ISDIR (item->file->lstat.st_mode))
              pchar[0]='d'; else
            if (S_ISLNK (item->file->lstat.st_mode))
              pchar[0]='l'; else
              pchar[0]='-';
            break;
          case COLUMN_OCTPERM:
            {
              int mode=item->file->lstat.st_mode, j;
              // Comvert digital value to string
              for (j=0; j<4; j++)
                {
                  pchar[3-j]=mode%8+'0';
                  mode/=8;
                }
              break;
            }
          }

      // Print text of cell
      print_column_string (layout, pchar, column->width,
        i!=n-1, 0, 0, flags);
    }

  return 0;
}

/**
 * Draws row of brief-vew table
 *
 * @param __panel - panel for which item is belong to
 * @param __intex - index of item in items list
 * @param __empty - is item empty?
 * This parameter is needed to draw empty rows of table
 * @return zero on succsess, non-zero if failed
 * @param __column_count - total count of columns
 */
static int
draw_brief_row                    (const file_panel_t *__panel,
                                   unsigned long       __index,
                                   BOOL                __empty,
                                   unsigned short      __column_count)
{
  int                row_number;
  scr_font_t        *item_font;
  wchar_t            pchar[MAX_SCREEN_WIDTH]={0};
  scr_window_t       layout;
  file_panel_item_t *item=NULL;
  BOOL               last_column;

  unsigned short     column_number,
                     width,
                     initial_width,
                     left_offset;

  unsigned long      start,
                     per_column;

  // Invalid pointers
  if (!__panel || !__panel->widget || !__panel->widget->layout)
    return -1;

  start      = __panel->widget->scroll_top;
  per_column = __panel->widget->items_per_page/__column_count;
  layout     = __panel->widget->layout;
  row_number = __index-start;

  if (!__empty)
    {
      // Get pointer to item to be drawed
      item=&__panel->items.data[__index];
      // Set font of current item
      item_font=get_file_panel_item_font (__panel, __index);
    } else
      item_font=__panel->widget->font;

  // Get number of current column
  column_number=(__index-start)/per_column;
  if (column_number)
    row_number-=per_column*column_number;

  // Is current column is last in panel?
  last_column=column_number==__column_count-1;

  // Get width of current column
  initial_width=width=(__panel->widget->position.width-2)/__column_count;

  // For last column use the rest space of panel
  // width=<full width of panel>-<borders>-<widths of standart columns>-
  //       <cound of separators>
  if (last_column)
    width=__panel->widget->position.width-2-
      width*(__column_count-1)-(column_number);

  left_offset=column_number*(initial_width+1)+1;

  // Goto needed row and column
  scr_wnd_move_caret (layout, left_offset, row_number+2);

  // Fit file name to width of column
  if (!__empty)
    fit_filename (item->file->name, width, pchar);

  // Print name of file
  print_column_string (layout, pchar, width,
    !last_column, item_font, __panel->widget->border_font, 0);

  // If tehere is the first item in column,
  // we should print a cption of column
  if (!row_number)
    {
      scr_wnd_move_caret (layout, left_offset, row_number+1);

      print_column_string (layout, L"Name", width,
        !last_column,
        __panel->widget->caption_font, __panel->widget->border_font,
        CF_ALIGN_CENTER);

      // Draw tee enclusers
      if (column_number)
        {
          // Set border's font
          scr_wnd_font (layout, *__panel->widget->border_font);

          // Top enclosing
          scr_wnd_move_caret (layout, left_offset-1, row_number);
          scr_wnd_putch (layout, ACS_TTEE);

          // Bottom enclosing
          scr_wnd_move_caret (layout, left_offset-1, row_number+per_column+2);
          scr_wnd_putch (layout, ACS_BTEE);
        }
    }

  return 0;
}

/**
 * Encloses columns
 *
 * @param __columns - descriptor of columns set
 * @param __layout - layout on which item will be drawed
 * @param __ch - char which will be used to wnclose columns
 * @param __row - row of layout where do enclosing
 */
static void
encluse_columns                   (scr_window_t  __layout,
                                   const file_panel_columns_t *__columns,
                                   wchar_t       __ch,
                                   unsigned int  __row)
{
  int i, x=0;
  if (!__layout)
    return;

  for (i=0; i<__columns->count-1; i++)
    {
      x+=__columns->data[i].width+1;
      scr_wnd_move_caret (__layout, x, __row);
      scr_wnd_putch (__layout, __ch);
    }
}

/**
 * Draws items of panel in full-row mode
 *
 * @param __panel - panel from which draw items
 * @return zero on succsess, non-zero if failed
 */
static int
draw_full_file_panel_items        (const file_panel_t *__panel)
{
  scr_window_t         layout;
  file_panel_widget_t *widget;
  unsigned long        i, n,
                       start, per_page;

  // Get layput of widget
  if (!__panel || !(layout=WIDGET_LAYOUT ((WIDGET (__panel->widget)))))
    return -1;

  // For scrolling
  start    = __panel->widget->scroll_top;
  per_page = __panel->widget->items_per_page;

  widget=__panel->widget;

  // Draw header
  encluse_columns (layout, &__panel->columns, ACS_TTEE, 0);

  if (draw_columns_headers (__panel, &__panel->columns, layout))
    return -1;

  // Draw all items avaliable for currect scrolling offset
  for (i=0, n=MIN (per_page, __panel->items.length-start); i<n; i++)
    draw_full_row (__panel, i+start, FALSE);

  // Fill itemless area with spaces
  scr_wnd_font (layout, *__panel->widget->font);
  for (i=n; i<per_page; i++)
    draw_full_row (__panel, i+start, TRUE);

  // Draw footer
  scr_wnd_font (layout, *__panel->widget->border_font);
  encluse_columns (layout, &__panel->columns, ACS_BTEE, per_page+2);

  return 0;
}

/**
 * Draws items of panel in brief mode
 *
 * @param __panel - panel from which draw items
 * @return zero on succsess, non-zero if failed
 */
static int
draw_brief_file_panel_items       (const file_panel_t *__panel)
{
  scr_window_t         layout;
  file_panel_widget_t *widget;
  unsigned short       column_count;

  unsigned long        i,
                       start, per_page;

  // Get layput of widget
  if (!__panel || !(layout=WIDGET_LAYOUT ((WIDGET (__panel->widget)))))
    return -1;

  // For scrolling
  start    = __panel->widget->scroll_top;
  per_page = __panel->widget->items_per_page;

  // This function is used to draw both brief and medium listings,
  // so we should determine how much columns is used at this moment
  if (__panel->listing_mode==LISTING_MODE_BRIEF)
    column_count=COLUMNS_PER_BRIEF; else
    column_count=COLUMNS_PER_MEDIUM;

  // Draw all items avaliable for currect scrolling offset
  for (i=0; i<per_page; i++)
    {
      scr_wnd_font (layout, *__panel->widget->font);
      draw_brief_row (__panel, i+start, i+start>=__panel->items.length, column_count);
    }

  widget=__panel->widget;

  return 0;
}

/**
 * Updates all data needed for scrolling
 *
 * @param __panel - file panel for which update scroll data
 */
static void
update_scroll_data                (file_panel_t *__panel)
{
  if (!__panel || !__panel->widget)
    return;

  file_panel_widget_t *widget=__panel->widget;

  if (widget->position.height<3)
    {
      // Size of widget is too small, so
      // we can't properly calculate scrolling data.
      // The safiest way - nothing to do
      return;
    }

  // Scroll sizes
  widget->items_per_page=widget->position.height-3;
  if (__panel->listing_mode==LISTING_MODE_BRIEF)
    widget->items_per_page*=COLUMNS_PER_BRIEF; else
  if (__panel->listing_mode==LISTING_MODE_MEDIUM)
    widget->items_per_page*=COLUMNS_PER_MEDIUM;

  ////
  // Get scroll_top (aka index of first item on screen)

  if (__panel->items.current>=__panel->items.length)
    __panel->items.current=__panel->items.length-1;
  
  // Come out of current displaying list of items
  if (__panel->items.current>widget->scroll_top+widget->items_per_page-1)
    widget->scroll_top=__panel->items.current-widget->items_per_page+1;

  if (__panel->items.current<widget->scroll_top)
    widget->scroll_top=__panel->items.current;

  // If we tried to show list when the last item is not
  // on the last row of widget
  if (__panel->items.length-widget->scroll_top<widget->items_per_page)
    widget->scroll_top=__panel->items.length-widget->items_per_page;

  // If count of items is less than items per page,
  //  Then set scroll_top to zero
  if (__panel->items.length<widget->items_per_page)
    widget->scroll_top=0;
}

/**
 * Scrolls view to make current item be displayed on center
 *
 * @param __panel - panel for which do this operation
 */
static void
centralize_current_item           (file_panel_t *__panel)
{
  if (!__panel || !__panel->widget)
    return;

  file_panel_widget_t *widget=__panel->widget;

  if (__panel->items.current>=widget->position.height/2)
    widget->scroll_top=__panel->items.current-widget->position.height/2; else
    widget->scroll_top=0;

  update_scroll_data (__panel);
}

/**
 * Sets cursor to item with specified name
 *
 * @param __panel - panel for which do this operation
 * @param __name - name pf item on which cursor will be set
 */
static void
cursor_to_item                    (file_panel_t *__panel, wchar_t *__name)
{
  unsigned long index;
  index=file_panel_item_index_by_name (__panel, __name, 0);
  if (index<0)
    index=0;

  __panel->items.current=index;
}

/**
 * Changes CWD by concatenating specified name of subdir to current CWD
 *
 * @param __panel - panel for which CWD must be changed
 * @param __sub_dir - name of subdirectory to CHDIR to
 * @return zero on succsess, non-zero if failed
 */
static int
cwd_sink                          (file_panel_t  *__panel,
                                   const wchar_t *__sub_dir)
{
  wchar_t *new_cwd;

  if (!__panel || !__panel->widget)
    return -1;

  // Get name of new CWD
  new_cwd=wcdircatsubdir (__panel->cwd.data, __sub_dir);

  if (new_cwd)
    {
      wchar_t *fn    = 0,   // File name to select
              *s_cwd = 0;   // Previous CWD
      unsigned long s_cur, s_scrolltop;

      // Backup parameters
      s_cwd       = wcsdup (__panel->cwd.data);
      s_cur       = __panel->items.current;
      s_scrolltop = __panel->widget->scroll_top;

      if (wcscmp (__sub_dir, L".."))
        __panel->items.current=0; /* Reset current item */ else
        fn=wcfilename (__panel->cwd.data);

      // Set new CWD
      if (file_panel_set_cwd (__panel, new_cwd))
        {
          // Some errors occured
          message_box (L"Error", L"Cannot change directory",
            MB_CRITICAL|MB_OK);

          // Restore parameters
          file_panel_set_cwd (__panel, s_cwd);
          __panel->items.current      = s_cur;
          __panel->widget->scroll_top = s_scrolltop;
          file_panel_draw (__panel);

          SAFE_FREE (s_cwd);
          free (new_cwd);

          return -1;
        }

      if (fn)
        {
          FILE_PANEL_DATA_ACTION_CALL (__panel, centre_to_item, fn);
          free (fn);
        }

      SAFE_FREE (s_cwd);
      free (new_cwd);
    }
  
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
  wcscpy (date_format,     DEFAULT_DATE_FORMAT);
  wcscpy (old_date_format, DEFAULT_OLD_DATE_FORMAT);

  recent_past   = DEFAULT_RECENT_PAST;
  recent_future = DEFAULT_RECENT_FUTURE;

  //
  // TODO:
  //  Add getting data from config here
  //
  return 0;
}

////////
//

/**
 * Intialize file panels' default actions stuff
 *
 * @return zero on sucsess and non-zero if operation failed
 */
int
file_panel_defact_init            (void)
{
  // Get data from configuration
  if (read_config ())
    return -1;

  return 0;
}

/**
 * Unintializes file panels' default actions stuff
 */
void
file_panel_defact_done            (void)
{
}

/**
 * Handles an onrefresh() action of panel
 *
 * @param __panel - panel which received an onrefresh action
 * @return zero if callback hasn't handled received character
 */
int
file_panel_defact_onrefresh       (file_panel_t *__panel)
{
  if (!__panel)
    return -1;

  update_scroll_data (__panel);
  return 0;
}

/**
 * Handles an onresize() action of panel
 *
 * @param __panel - panel which received an onrefresh action
 * @return zero if callback hasn't handled received character
 */
int
file_panel_defact_onresize        (file_panel_t *__panel)
{
  if (!__panel)
    return -1;

  file_panel_update_columns_widths (__panel);
  update_scroll_data (__panel);
  return 0;
}

////
// Differenet handlers

/**
 * Action for handling keydown event
 *
 * @param __panel - panel which received a keydown event
 * @param __ch - pointer to received character (not string!)
 * @return zero on succsess, non-zero if failed
 */
int
file_panel_defact_keydown_handler (file_panel_t *__panel, wchar_t *__ch)
{
  if (!__panel || FILE_PANEL_TEST_FLAG (__panel, FPF_UNFOCUSABLE))
    return -1;

  switch (*__ch)
    {

    ////////
    // !! DEBUG !!
    case 'r':
      file_panel_rescan (__panel);
      break;
    case 'l':
      file_panel_set_listing_mode (__panel, (__panel->listing_mode+1)%3);
      break;
    //
    ////////

    case KEY_RETURN:
      // Check is tehere any selected items
      if (__panel->items.length && __panel->items.current>=0 &&
          __panel->items.current<=__panel->items.length)
        {
          file_panel_item_t *item;

          item=&__panel->items.data[__panel->items.current];

          if (S_ISDIR (item->file->stat.st_mode))
            {
              cwd_sink (__panel, item->file->name);
              file_panel_redraw (__panel);
            }
        }
      break;
    default:
      return -1;
    }

  return 0;
}

////////
//

/**
 * Gets list of panel's items 
 *
 * @param __panel - panel for which get items' list
 * @return zero on succsess, non-zero if failed
 */
int
file_panel_defact_collect_items   (file_panel_t *__panel)
{
  file_t **list;
  unsigned long i, count, ptr=0;
  BOOL cwd_root;

  if (!__panel || !__panel->cwd.data)
    return -1;

  cwd_root=!wcscmp (__panel->cwd.data, L"/");

  count=wcscandir (__panel->cwd.data, scandir_filter_skip_hidden,
    wcscandir_alphasort_sep, &list);

  __panel->items.length=0;

  // Errors while getting list of files
  if (count==-1)
    return -1;

  __panel->items.length=count-1-(cwd_root?1:0);
  MALLOC_ZERO (__panel->items.data,
    sizeof (file_panel_item_t)*__panel->items.length);

  // Copy file information from list from wcscandir
  // to panel's list of items
  for (i=0; i<count; i++)
    {
      // Skip '.' from all listings and '..' from listing of root dir
      if (  !wcscmp (list[i]->name, L".") ||
            (cwd_root && !wcscmp (list[i]->name, L".."))
         )
        {
          free (list[i]);
          continue;
        }

      __panel->items.data[ptr++].file=list[i];
    }

  // Now we can free array, but not it's items!
  SAFE_FREE (list);

  return 0;
}

/**
 * Frees list of panel's items 
 *
 * @param __panel - panel from which free items' list
 * @return zero on succsess, non-zero if failed
 */
int
file_panel_defact_free_items      (file_panel_t *__panel)
{
  unsigned long i;

  if (!__panel)
    return -1;

  // Free all items
  for (i=0; i<__panel->items.length; i++)
    {
      if (__panel->items.data[i].file)
        free (__panel->items.data[i].file);

      // Free user-defined data
      if (__panel->actions.item_user_data_deleter)
        __panel->actions.item_user_data_deleter (__panel->items.data[i].
              user_data);
    }

  SAFE_FREE (__panel->items.data);

  __panel->items.length=__panel->items.current=0;

  return 0;
}

/**
 * Default action to draw a list of panel's items
 *
 * @param __panel - panel for which draw item list
 * @return zero on succsess, non-zero if failed
 */
int
file_panel_defact_draw_item_list  (file_panel_t *__panel)
{
  // But does we really need this call here?
  //
  // NOTE:
  //  User could changed a listing mode and we haven't handle this
  //  in file_panel_set_listing_mode(). But maye be it would better
  //  if we handle this in those stuff?
  //
  update_scroll_data (__panel);

  //
  // TODO:
  //  Add choosig of items' display mode here
  //

  if (__panel->listing_mode==LISTING_MODE_FULL)
    draw_full_file_panel_items (__panel); else
  if (__panel->listing_mode==LISTING_MODE_BRIEF ||
      __panel->listing_mode==LISTING_MODE_MEDIUM)
      draw_brief_file_panel_items (__panel);

  return 0;
}

/**
 * Draws a file panel's widget
 *
 * @param __panel_widget - widget to be drawn
 * @return zero on succsess, non-zero if failed
 */
int
file_panel_defact_draw_widget     (file_panel_widget_t *__panel_widget)
{
  scr_window_t layout;
  file_panel_t *panel;
  wchar_t pchar[MAX_SCREEN_WIDTH];

  // There is no needed attributes
  if (!__panel_widget || !(layout=WIDGET_LAYOUT (__panel_widget)))
    return -1;

  if (!(panel=WIDGET_USER_DATA (WIDGET (__panel_widget))))
    return -1;

  // Set background
  scr_wnd_bkg (layout, *__panel_widget->font);

  // Draw border
  scr_wnd_font (layout, *__panel_widget->border_font);
  scr_wnd_border (layout);

  // Draw items
  if (panel->actions.draw_items)
    panel->actions.draw_items (panel);

  // Draw current working directory
  scr_wnd_font (layout, *__panel_widget->font);
  if (panel->cwd.data)
    {
      // Fit CWD name to panel's width
      // (4=margin from border and padding inside caption)
      fit_dirname (panel->cwd.data, __panel_widget->position.width-4, pchar);

      // Move caret to needed position
      scr_wnd_move_caret (layout,
        (__panel_widget->position.width-wcslen (pchar)-2)/2, 0);

      // Set font of CWD text
      scr_wnd_font (layout, panel->focused?*__panel_widget->focused_dir_font:
        *__panel_widget->font);

      // Print CWD
      scr_wnd_printf (layout, " %ls ", pchar);

      // Restore font
      scr_wnd_font (layout, *__panel_widget->font);
    }

  return 0;
}

/**
 * Walk on file panel's widget
 *
 * @param __panel - panel in which we are walking
 * @param __direction of our walking. This values are posible:
 *   WALK_NEXT
 *   WALK_PREV
 *   WALK_NEXT_PAGE
 *   WALK_PREV_PAGE
 */
void
file_panel_defact_walk            (file_panel_t *__panel, short __direction)
{
  switch (__direction)
    {
    case WALK_NEXT:
      if (__panel->items.current<__panel->items.length-1)
        __panel->items.current++; else
        return; // It is nothing to do
      break;
    case WALK_PREV:
      if (__panel->items.current>0)
        __panel->items.current--; else
        return; // It is nothing to do
      break;

    case WALK_NEXT_PAGE:
      if (__panel->items.current==__panel->items.length-1)
        return; // It is nothing to do

      __panel->items.current+=__panel->widget->items_per_page;
      __panel->widget->scroll_top+=__panel->widget->items_per_page;
      break;
    case WALK_PREV_PAGE:
      if (!__panel->items.current)
        return; // It is nothing to do

       if (__panel->items.current>__panel->widget->items_per_page)
           __panel->items.current-=__panel->widget->items_per_page; else
          __panel->items.current=0;

       if (__panel->widget->scroll_top>__panel->widget->items_per_page)
         __panel->widget->scroll_top-=__panel->widget->items_per_page; else
         __panel->widget->scroll_top=0;

      break;

    case WALK_HOME:
      __panel->items.current=0;
      break;
    case WALK_END:
      if (__panel->items.length)
        __panel->items.current=__panel->items.length-1; else
        __panel->items.current=0;
      break;

    case WALK_PREV_COLUMN:
    case WALK_NEXT_COLUMN:
      {
        file_panel_widget_t *widget = __panel->widget;

        // Variables for further quick usage
        unsigned short per_column   = __panel->widget->items_per_page;
        unsigned long  per_page     = widget->items_per_page,
                       scroll_top   = widget->scroll_top;

        // Get count of items per column
        if (__panel->listing_mode==LISTING_MODE_MEDIUM)
            per_column/=COLUMNS_PER_MEDIUM; else
        if (__panel->listing_mode==LISTING_MODE_BRIEF)
            per_column/=COLUMNS_PER_BRIEF;

        // Go to needed direction
        if (__direction==WALK_NEXT_COLUMN)
          {
            __panel->items.current+=per_column;
          } else {
            // To avoid integer overflow
            if (__panel->items.current>=per_column)
              __panel->items.current-=per_column; else
              __panel->items.current=0;
          }

        ////
        // Make specifistic checking of scrolling

        // Come out of current displaying list of items
        if (__panel->items.current>scroll_top+per_page-1)
          widget->scroll_top+=per_column;

        if (__panel->items.current<scroll_top)
          widget->scroll_top-=per_column;

        break;
      }
    }

  update_scroll_data (__panel);

  //
  // TODO:
  //  It would be better if we redraw only changed items
  //
  widget_redraw (WIDGET (__panel->widget));
}

/**
 * Sets cursor and centres view to item with specified name
 *
 * @paarm __panel - panel to operate with
 * @param __name - name of item to select
 */
void
file_panel_defact_centre_to_item   (file_panel_t *__panel, wchar_t *__name)
{
  cursor_to_item (__panel, __name);
  centralize_current_item (__panel);
  file_panel_draw (__panel);
}

/**
 * Sets cursor and scrolls view to item with specified name
 *
 * @paarm __panel - panel to operate with
 * @param __name - name of item to select
 */
void
file_panel_defact_scroll_to_item  (file_panel_t *__panel, wchar_t *__name)
{
  cursor_to_item (__panel, __name);
  update_scroll_data (__panel);
  file_panel_draw (__panel);
}
