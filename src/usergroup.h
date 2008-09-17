/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * User/group specified stuff
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#ifndef _USERGROUP_H_
#define _USERGROUP_H_

#include "smartinclude.h"

#include <wchar.h>

/********
 * Type definitions
 */

typedef struct
{
  wchar_t *name; /* user name */
  wchar_t *passwd; /* user password */
  uid_t uid; /* user ID */
  gid_t gid; /* group ID */
  wchar_t *gecos; /* real name */
  wchar_t *home; /* home directory */
  wchar_t *shell; /* shell program */
} passwd_t;

typedef struct
{
  wchar_t *name;   /* group name */
  wchar_t *passwd; /* group password */
  gid_t   gid;     /* group ID */
  wchar_t  **mem;  /* group members */
} group_t;

/********
 *
 */

/* Get specified user information */
passwd_t*
get_user_info_by_id (int __id);

/* Get specified user information */
passwd_t*
get_user_info_by_name (const wchar_t *__name);

/* Get current user information */
passwd_t*
get_user_info (void);

/* Free memory used by user's passws information */
void
free_user_info (passwd_t *__self);

/* Get list of users in system */
int
get_users_list (passwd_t ***__list);

/* Free memory used by list of users */
void
free_users_list (passwd_t **__list, int __count);

/* Get specified group information */
group_t*
get_group_info_by_id (int __id);

/* Get specified group information */
group_t*
get_group_info_by_name (const wchar_t *__name);

/* Get current group information */
group_t*
get_group_info (void);

/* Free memory used by group's information */
void
free_group_info (group_t *__self);

/* Get list of groups in system */
int
get_groups_list (group_t ***__list);

/* Free memory used by list of groups */
void
free_groups_list (group_t **__list, int __count);

#endif
