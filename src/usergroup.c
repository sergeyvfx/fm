/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Different helpers
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "usergroup.h"

#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

/**
 * Convert structure passwd to passwd_t
 *
 * @param __dst - destination structure
 * @param __src - source structure
 */
static void
convert_passwd (passwd_t *__dst, const struct passwd *__src)
{
  MBS2WCS (__dst->name,   __src->pw_name);
  MBS2WCS (__dst->passwd, __src->pw_passwd);
  MBS2WCS (__dst->gecos,  __src->pw_gecos);
  MBS2WCS (__dst->home,   __src->pw_dir);
  MBS2WCS (__dst->shell,  __src->pw_shell);

  __dst->uid = __src->pw_uid;
  __dst->gid = __src->pw_gid;
}

/**
 * Convert structure group to group_t
 *
 * @param __dst - destination structure
 * @param __src - source structure
 */
static void
convert_group (group_t *__dst, const struct group *__src)
{
  int i = 0;

  MBS2WCS (__dst->name,   __src->gr_name);
  MBS2WCS (__dst->passwd, __src->gr_passwd);

  __dst->mem = NULL;
  while (__src->gr_mem && __src->gr_mem[i])
    {
      __dst->mem = realloc (__dst->mem, (i + 1) * sizeof (wchar_t*));
      MBS2WCS (__dst->mem[i], __src->gr_mem[i]);
      ++i;
    }

  __dst->gid = __src->gr_gid;
}

/********
 * User's backend
 */

/**
 * Get current user information
 *
 * @param __id - user identificator
 * @return passwd structure describing user or NULL if failed
 */
passwd_t*
get_user_info_by_id (int __id)
{
  struct passwd *pw;
  passwd_t *result;

  pw = getpwuid (__id);

  if (pw == NULL)
    {
      return NULL;
    }

  result = malloc (sizeof (passwd_t));

  convert_passwd (result, pw);

  return result;
}

/**
 * Get specified user information
 *
 * @param __name - name of user
 * @return passwd structure describing user or NULL if failed
 */
passwd_t*
get_user_info_by_name (const wchar_t *__name)
{
  char *mbname;
  passwd_t *result;
  struct passwd *pw;

  /* Convert wide char to multibyte */
  WCS2MBS (mbname, __name);

  pw = getpwnam (mbname);

  if (pw == NULL)
    {
      free (mbname);
      return NULL;
    }

  result = malloc (sizeof (passwd_t));

  convert_passwd (result, pw);

  free (mbname);

  return result;
}


/**
 * Get current user information
 *
 * @return passwd structure describing user or NULL if failed
 */
passwd_t*
get_user_info (void)
{
  return get_user_info_by_id (getuid ());
}

/**
 * Free memory used by user's passws information
 *
 * @param __self - descriptor to be freed
 */
void
free_user_info (passwd_t *__self)
{
  free (__self->name);
  free (__self->passwd);
  free (__self->gecos);
  free (__self->home);
  free (__self->shell);

  free (__self);
}

/**
 * Get list of users in system
 *
 * @param __list - pointer to list where users' descriptions
 * will be saved
 * @return count of users in list, or value less than zero
 * if error occured
 */
int
get_users_list (passwd_t ***__list)
{
  struct passwd *pw;
  int count = 0;

  if (!__list)
    {
      /* Invalid argument */
      return -1;
    }

  (*__list) = NULL;

  errno = 0;

  /* Go to beginning of passwd file */
  setpwent ();

  while ((pw = getpwent ()))
    {
      /* Allocate memory for new element */
      (*__list) = realloc (*__list, (count + 1) * sizeof (passwd_t*));
      (*__list)[count] = malloc (sizeof (passwd_t));

      /* Convert passwd structure to new format */
      convert_passwd ((*__list)[count], pw);

      ++count;
    }

  if (errno)
    {
      /* Error occured while reading passwd database */
      free_users_list (*__list, count);

      (*__list) = NULL;

      return -1;
    }

  return count;
}

/**
 * Free memory used by list of users
 *
 * @param __list - list of users
 * @param __count - count of items in list
 */
void
free_users_list (passwd_t **__list, int __count)
{
  int i;

  if (!__list)
    {
      return;
    }

  /* Free allocated memory */
  for (i = 0; i < __count; ++i)
    {
      free_user_info (__list[i]);
    }

  free (__list);
}

/**
 * Get specified group information
 *
 * @param __id - identificator of group
 * @return group structure describing user or NULL if failed
 */
group_t*
get_group_info_by_id (int __id)
{
  struct group *gr;
  group_t *result;

  gr = getgrgid (__id);

  if (gr == NULL)
    {
      return NULL;
    }

  result = malloc (sizeof (group_t));

  convert_group (result, gr);

  return result;
}

/**
 * Get specified group information
 *
 * @param __name - name of group
 * @return group structure describing user or NULL if failed
 */
group_t*
get_group_info_by_name (const wchar_t *__name)
{
  char *mbname;
  group_t *result;
  struct group *gr;

  /* Convert wide char to multibyte */
  WCS2MBS (mbname, __name);

  gr = getgrnam (mbname);

  if (gr == NULL)
    {
      free (mbname);
      return NULL;
    }

  result = malloc (sizeof (group_t));

  convert_group (result, gr);

  free (mbname);

  return result;
}

/**
 * Get current group information
 *
 * @return group structure describing user or NULL if failed
 */
group_t*
get_group_info (void)
{
  return get_group_info_by_id (getgid ());
}

/**
 * Free memory used by group's information
 *
 * @param __self - descriptor of group to free
 */
void
free_group_info (group_t *__self)
{
  size_t i = 0;
  SAFE_FREE (__self->name);
  SAFE_FREE (__self->passwd);

  /* Free list of members */
  while (__self->mem && __self->mem[i])
    {
      SAFE_FREE (__self->mem[i]);
    }
    SAFE_FREE (__self->mem);

  free (__self);
}

/**
 * Get list of groups in system
 *
 * @param __list - pointer to list where groups' descriptions
 * will be saved
 * @return count of groups in list, or value less than zero
 * if error occured
 */
int
get_groups_list (group_t ***__list)
{
  struct group *gr;
  int count = 0;

  errno = 0;

  (*__list) = NULL;

  /* Go to beginning of group file  */
  setgrent ();

  while ((gr = getgrent ()))
    {
      /* Allocate memory for new element */
      (*__list) = realloc (*__list, (count + 1) * sizeof (group_t*));
      (*__list)[count] = malloc (sizeof (group_t));

      memset ((*__list)[count], 0, sizeof (group_t));

      /* Convert group structure to new format */
      convert_group ((*__list)[count], gr);

      ++count;
    }

  if (errno)
    {
      /* Error occured while reading groups database */
      free_groups_list (*__list, count);

      (*__list) = NULL;

      return -1;
    }

  return count;
}

/**
 * Free memory used by list of groups
 *
 * @param __list - list of groups
 * @param __count - count of items in list
 */
void
free_groups_list (group_t **__list, int __count)
{
  int i;

  if (!__list)
    {
      return;
    }

  /* Free allocated memory */
  for (i = 0; i < __count; ++i)
    {
      free_group_info (__list[i]);
    }

  free (__list);
}
