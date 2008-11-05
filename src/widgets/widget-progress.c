/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Implementation file for widget `progress`
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "widget.h"

/**
 * Destroy a progress bar widget
 *
 * @param __progress - progress bar to be destroyed
 * @return zero on success, non-zero on failure
 */
static int
progress_destructor (w_progress_t *__progress)
{
  if (!__progress)
    {
      return -1;
    }

  return 0;
}

/**
 * Draw a progress bar
 *
 * @param __progress - progress bar to be drawn
 * @return zero on success, non-zero on failure
 */
static int
progress_drawer (w_progress_t *__progress)
{
  scr_window_t layout = WIDGET_LAYOUT (__progress);
  int width, i, count, perc, start, finito;

  /* Widget is invisible or there is no layout */
  if (!WIDGET_VISIBLE (__progress) || !layout)
    {
      return -1;
    }

  width = __progress->position.width;
  if (!(__progress->style & WPBS_NOPERCENT))
    {
      width -= 5;
    }

  if (__progress->max_pos)
    {
      perc = (double) __progress->cur_pos / __progress->max_pos * 100;
    }
  else
    {
      perc = 100;
    }
  count = (double) perc / 100 * width;

  /*
   * NOTE: If WFPB_UPDATEONLY flag is set only
   *       changes will pe placed on the layout
   */
  if (WIDGET_TEST_FLAG (__progress, WFPB_UPDATEONLY))
    {
      if (count == __progress->prev_count)
        {
          return 0;
        }
      start = MAX (__progress->prev_count, 0);
      finito = __progress->prev_count;
      WIDGET_RESET_FLAG (__progress, WFPB_UPDATEONLY);
      __progress->prev_count = count;
    }
  else
    {
      start = 0;
      finito = width;
    }

  scr_wnd_attr_backup (layout);

  scr_wnd_move_caret (layout, __progress->position.x + start,
                      __progress->position.y);

  /* Draw bar */
  scr_wnd_font (layout, *__progress->progress_font);

  for (i = start; i < count; i++)
    {
      scr_wnd_putch (layout, ' ');
    }

  /* Draw the rest bar */
  i = count;
  scr_wnd_move_caret (layout, __progress->position.x + i,
                      __progress->position.y);
  scr_wnd_font (layout, *__progress->background_font);
  while (i < finito)
    {
      scr_wnd_putch (layout, ACS_CKBOARD);
      i++;
    }

  /* Draw percents */
  if (!(__progress->style & WPBS_NOPERCENT))
    {
      scr_wnd_move_caret (layout, __progress->position.x + width,
                          __progress->position.y);
      scr_wnd_printf (layout, " %3d%%", perc);
    }

  scr_wnd_attr_restore (layout);

  return 0;
}

/********
 * User's backend
 */

/**
 * Create new progress bar
 *
 * @param __parent - parent of progress bar. Should be CONTAINER
 * @param __max_pos - maximal progress position
 * @param __x, __y - coordinates of progress bar
 * @param __w - width of progress bar
 * @return a pointer to progress bar object
 */
w_progress_t*
widget_create_progress (w_container_t *__parent, unsigned long __max_pos,
                        int __x, int __y, int __w, unsigned int __style)
{
  w_progress_t *res;

  /* There is no parent, so we can't create progress bar */
  if (!__parent)
    {
      return 0;
    }

  WIDGET_INIT (res, w_progress_t, WT_PROGRESS, __parent,
               WF_NOLAYOUT | WF_UNFOCUSABE,
               FALSE,
               progress_destructor, progress_drawer, __x, __y, 1, __w, 1);

  res->max_pos = __max_pos;
  res->style = __style;

  res->font            = FONT (CID_BLUE, CID_GREY);
  res->background_font = FONT (CID_BLACK, CID_GREY);
  res->progress_font   = FONT (CID_WHITE, CID_BLACK);

  res->prev_count = -1;

  WIDGET_POST_INIT (res);

  return res;
}

/**
 * Set font used in progress bar
 *
 * @param __progress - for which progress fonts are to be set
 * @param __font - font to draw the text
 * @param __background_font - background font
 * @param __progress_font - font of progress bar
 */
void
w_progress_set_font (w_progress_t *__progress,
                     scr_font_t *__font, scr_font_t *__background_font,
                     scr_font_t *__progress_font)
{
  if (!__progress)
    {
      return;
    }

  WIDGET_SAFE_SET_FONT (__progress, font, __font);
  WIDGET_SAFE_SET_FONT (__progress, background_font, __background_font);
  WIDGET_SAFE_SET_FONT (__progress, progress_font, __progress_font);

  widget_redraw (WIDGET (__progress));
}

/**
 * Set position of progress bar
 *
 * @param __progress - for which widget position will be set
 * @param __pos - new position of progress bar
 */
void
w_progress_set_pos (w_progress_t *__progress, unsigned long __pos)
{
  if (!__progress)
    {
      return;
    }

  /* Fit current position */
  if (__progress->max_pos < __pos)
    {
      return;
    }

  __progress->cur_pos = __pos;
  WIDGET_SET_FLAG (__progress, WFPB_UPDATEONLY);
  widget_redraw (WIDGET (__progress));
}

/**
 * Set maximal position of progress bar
 *
 * @param __progress - for which widget position will be set
 * @param __max - new max position of progress bar
 */
void
w_progress_set_max (w_progress_t *__progress, unsigned long __max)
{
  if (!__progress)
    {
      return;
    }

  __progress->max_pos = __max;

  /* Fit current position */
  if (__progress->cur_pos > __max)
    {
      __progress->cur_pos = __max;
    }

  WIDGET_SET_FLAG (__progress, WFPB_UPDATEONLY);
  widget_redraw (WIDGET (__progress));
}

/**
 * Get progress position
 *
 * @param __progress - from which progress bar position will be gotten
 * @return position of progress bar
 */
unsigned long
w_progress_get_pos (w_progress_t *__progress)
{
  if (!__progress)
    {
      return 0;
    }

  return __progress->cur_pos;
}

/**
 * Get max position
 *
 * @param __progress - from which progress bar position will be gotten
 * @return ,ax position of progress bar
 */
unsigned long
w_progress_get_max (w_progress_t *__progress)
{
  if (!__progress)
    {
      return 0;
    }

  return __progress->max_pos;
}
