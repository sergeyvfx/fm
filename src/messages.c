/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Messages' displaying stuff
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "messages.h"
#include "widget.h"
#include "screen.h"
#include "i18n.h"

////////
//

#define MAX_BUTTONS 4

static widget_action old_drawer;

typedef struct {
  short    count;     // Count of buttons
  unsigned width;     // Length of buttons' block
  struct {
    int       modal_result;
    unsigned  width;
    wchar_t   *caption;
  } buttons[MAX_BUTTONS];
} btn_array_t;

// Posibile sets of buttons
static btn_array_t buttons[]={
    { 1, -1, { {MR_OK,    -1, L"_Ok"} } },
    { 2, -1, { {MR_OK,    -1, L"_Ok"},      {MR_CANCEL, -1, L"_Cancel"} } },
    { 2, -1, { {MR_YES,   -1, L"_Yes"},     {MR_NO,     -1, L"_No"} } },
    { 3, -1, { {MR_YES,   -1, L"_Yes"},     {MR_NO,     -1, L"_No"},    {MR_CANCEL, -1, L"_Cancel"} } },
    { 2, -1, { {MR_RETRY, -1, L"_Retry"},   {MR_CANCEL, -1, L"_Cancel"} } },
    { 3, -1, { {MR_RETRY, -1, L"_Retry"},   {MR_SKIP,   -1, L"_Skip"},  {MR_CANCEL, -1, L"_Cancel"} } },
  };

////////
//

/**
 * Returns position of message (x,y-cooddinates and width/height)
 *
 * @param __caption - caption of message window
 * @param __text - text to display in message
 * @param - additional configuration flags of message's window
 * @params - array of buttons
 * @return position of message's window
 */
static widget_position_t
msg_wnd_pos                       (const wchar_t     *__caption,
                                   const wchar_t     *__text,
                                   unsigned int       __flags,
                                   const btn_array_t *__buttons)
{
  int i, n, len;
  widget_position_t res={0, 0, 0, 3, 0};

  // Initially width of window is fit to caption.
  // 6 = 2 spaces + 2 of | + 2 for border's corner
  res.width=wcslen (__caption)+6;

  // Is buttons are wider than caption
  // 4 = 2 spaces for boprder + 2 spaces for padding
  res.width=MAX (res.width, __buttons->width+4);

  // Check is there is a wider line of text
  for (i=len=0, n=wcslen (__text); i<n; ++i)
    {
      if (__text[i]=='\n' || i==n-1)
        {
          res.width=MAX (res.width, len+4+(__text[i]!='\n'?1:0));
          res.height++;
          len=0;
        } else
          len++;
    }

  // Centration
  res.x=(SCREEN_WIDTH-res.width)/2;
  res.y=(SCREEN_HEIGHT-res.height)/2;

  return res;
}

/**
 * Returns array of buttons used in message window, determined by __flags
 *
 * @param __flags - flags which determenine's message's window
 * @return array of buttons for message
 */
static btn_array_t
get_buttons                       (unsigned int __flags)
{
  int i;

  // Get button set from default list
  btn_array_t res=buttons[MB_BUTTON_CODE (__flags)];

  // Localization and calculate summary width of button set
  res.width=0;
  for (i = 0; i < res.count; ++i)
  {
      res.buttons[i].caption = _(res.buttons[i].caption);

      res.buttons[i].width = widget_shortcut_length (res.buttons[i].caption)+4;
      res.width += res.buttons[i].width;
  }
  return res;
}

/**
 * Draws a message's window
 *
 * @param __window - window of message
 * @return
 */
static int
msg_window_drawer                 (w_window_t *__window)
{
  wchar_t *text=WIDGET_USER_DATA (__window);
  int i, n, prev, j, m, line, width;
  scr_window_t layout=WIDGET_LAYOUT (__window);

  // Call to default window drawer
  if (old_drawer)
    old_drawer (__window);

  // Draw text of message
  line=1;
  width=__window->position.width;

  for (i=prev=0, n=wcslen (text); i<n; ++i)
    {
      if (text[i]=='\n' || i==n-1)
        {
          scr_wnd_move_caret (layout, (width-i+prev)/2, line++);
          for (j=prev, m=i+(i==n-1&&text[i]!='\n'); j<m; j++)
            scr_wnd_add_wchar (layout, text[j]);

          i+=text[i]=='\n';
          prev=i;
        }
    }

  return 0;
}

////////
// End-user stuff

/**
 * Shows a message box
 *
 * @param __caption - caption of message
 * @param __text - text of message
 * @params __flags - other flags whish determines feel&look of message box.
 *  MB_CRITICAL - determines message is critical
  * This flags determines a set of buttons:
 *   MB_OK, MB_OKCANCEL, MB_YESNO, MB_YESNOCANCEL,
 *   MB_RETRYCANCEL, MB_RETRYSKIPCANCEL
 * This flagse determines an index of initially focused button:
 *   MB_DEFBUTTON_0, MB_DEFBUTTON_1, MB_DEFBUTTON_2
 *
 * @return modal result of message
 */
int
message_box                       (const wchar_t *__caption,
                                   const wchar_t *__text,
                                   unsigned int   __flags)
{
  w_window_t *wnd;
  w_button_t *cur_btn;
  btn_array_t btn_arr=get_buttons (__flags);
  widget_position_t pos=msg_wnd_pos (__caption, __text, __flags, &btn_arr);
  int i, res, x;
  short defbutton=MB_DEFBUTTON (__flags);
  BOOL critical=FALSE;

  wnd=widget_create_window (__caption, pos.x, pos.y, pos.width, pos.height);

  // Reset default fonts if message is critical
  if ((critical=__flags&MB_CRITICAL))
    w_window_set_fonts (wnd,
      &FONT (CID_WHITE, CID_RED), &FONT (CID_YELLOW, CID_RED));

  // Use USER_DATA to tell window its text
  if (__text)
    WIDGET_USER_DATA(wnd)=wcsdup (__text);

  // Replace default drawer
  old_drawer=WIDGET_METHOD (wnd, draw);
  WIDGET_METHOD (wnd, draw)=(widget_action)msg_window_drawer;

   // Create buttons
  x=(pos.width-btn_arr.width-btn_arr.count+1)/2;
  for (i=0; i<btn_arr.count; i++)
    {
      cur_btn=widget_create_button(WIDGET_CONTAINER (wnd),
        btn_arr.buttons[i].caption, x, pos.height-2, 0);

      // Current button is default
      if (i==defbutton)
        widget_set_focus (WIDGET (cur_btn));

      WIDGET_BUTTON_MODALRESULT (cur_btn)=btn_arr.buttons[i].modal_result;

      // If message is critical we should set another fonts to buttons
      if (critical)
        w_button_set_fonts (cur_btn,
         &FONT (CID_WHITE, CID_RED),  &FONT (CID_BLACK, CID_WHITE),
         &FONT (CID_YELLOW, CID_RED), &FONT (CID_YELLOW, CID_WHITE));

      x+=btn_arr.buttons[i].width+1;
    }

  // Show message in modal state
  res=w_window_show_modal (wnd);

  SAFE_FREE (WIDGET_USER_DATA(wnd));
  widget_destroy (WIDGET (wnd));

  return res;
}
