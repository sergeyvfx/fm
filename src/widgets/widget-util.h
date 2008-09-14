/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Different utilities for widgets
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _widget_util_h_
#define _widget_util_h_

#ifndef _widget_h_
#  error Do not include this file directly. Include widget.h instead.
#endif

/* Draw border insode widget */
void
widget_draw_border (widget_t *__widget);

#endif
