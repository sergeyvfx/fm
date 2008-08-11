/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _widget_box_h_
#define _widget_box_h_

#ifndef _widget_h_
#  error Do not include this file directly. Include widget.h instead.
#endif

/********
 * Constants
 */

/* Box styles */
#define WBS_VERTICAL       0x0000
#define WBS_HORISONTAL     0x0001

/********
 * Type defenitions
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
 *
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

#endif
