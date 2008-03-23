/*
 *
 * ================================================================================
 *  messages.h
 * ================================================================================
 *
 *  Written (by Nazgul) under General Public License.
 *
*/

#include "messages.h"
#include "widget.h"
#include "screen.h"

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

static widget_position_t
msg_wnd_pos                       (wchar_t *__caption, wchar_t *__text, unsigned int __flags, btn_array_t *__buttons)
{
  int i, n, len;
  widget_position_t res={0, 0, 0, 3, 0};

  res.width=wcslen (__caption)+6;

  res.width=MAX (res.width, __buttons->width+4);

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

  res.x=(SCREEN_WIDTH-res.width)/2;
  res.y=(SCREEN_HEIGHT-res.height)/2;

  return res;
}

static btn_array_t
get_buttons                       (unsigned int __flags)
{
  int i;
  btn_array_t res=buttons[MB_BUTTON_CODE (__flags)];
  res.width=0;
  for (i=0; i<res.count; ++i)
      res.width+=(res.buttons[i].width=widget_shortcut_length (res.buttons[i].caption)+4);
  return res;
}

static int
msg_window_drawer                 (w_window_t *__window)
{
  wchar_t *text=WIDGET_USER_DATA (__window);
  int i, n, prev, j, m, line, width;
  scr_window_t layout=WIDGET_LAYOUT (__window);
  
  if (old_drawer)
    old_drawer (__window);

  line=1;
  width=__window->position.width;

  for (i=prev=0, n=wcslen (text); i<n; ++i)
    {
      if (text[i]=='\n' || i==n-1)
        {
          scr_wnd_move_caret (layout, (width-i+prev)/2, line++);
          for (j=prev, m=i+(i==n-1&&text[i]!='\n'); j<m; j++)
            scr_wnd_putch (layout, text[j]);

          i+=text[i]=='\n';
          prev=i;
        }
    }

  return 0;
}

////////
//

int
message_box                       (wchar_t *__caption, wchar_t *__text, unsigned int __flags)
{
  w_window_t *wnd;
  w_button_t *cur_btn;
  btn_array_t btn_arr=get_buttons (__flags);
  widget_position_t pos=msg_wnd_pos (__caption, __text, __flags, &btn_arr);
  int i, res, x;
  short defbutton=MB_DEFBUTTON (__flags);
  BOOL critical=FALSE;

  wnd=widget_create_window (__caption, pos.x, pos.y, pos.width, pos.height);

  if ((critical=__flags&MB_CRITICAL))
    w_window_set_fonts (wnd, &sf_white_on_red, &sf_yellow_on_red);
  
  WIDGET_USER_DATA(wnd)=__text;

  // Replace default drawer
  old_drawer=WIDGET_METHOD (wnd, draw);
  WIDGET_METHOD (wnd, draw)=(widget_action)msg_window_drawer;

 
  // Create buttons
  x=(pos.width-btn_arr.width-btn_arr.count+1)/2;
  for (i=0; i<btn_arr.count; i++)
    {
      cur_btn=widget_create_button(WIDGET_CONTAINER (wnd), btn_arr.buttons[i].caption,
        x, pos.height-2, 0);

      if (i==defbutton)
        widget_set_focus (WIDGET (cur_btn));

      WIDGET_BUTTON_MODALRESULT (cur_btn)=btn_arr.buttons[i].modal_result;

      if (critical)
        w_button_set_fonts (cur_btn, &sf_white_on_red, &sf_black_on_white, &sf_yellow_on_red, &sf_yellow_on_white);

      x+=btn_arr.buttons[i].width+1;
    }

  // Show message in modal state
  res=w_window_show_modal (wnd);
  
  widget_destroy (WIDGET (wnd));

  return res;
}
