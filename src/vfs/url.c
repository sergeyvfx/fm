/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * URL parsers,etc..
 *
 * Copyright 2008 Sergey I. Sharybin <nazgul@school9.perm.ru>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "url.h"
#include "errcode.h"
#include "plugin.h"

#include <util.h>

#include <wchar.h>

#define VFS_PLUGIN_DELIMETER L"::"

/**
 * Parses url string and returns responsible plugin and
 * local path inside plugin
 *
 * NOTE:
 *  This function creates buffer for returned path.
 *  It should be freed after usage.
 *
 * @param __url - url to be parsed
 * @param __plugin - responsible plugin
 * @param __path - local path inside plugin
 * @return zero on success, non-zero otherwise
 */
int
vfs_url_parse                     (const wchar_t *__url,
                                   vfs_plugin_t **__plugin,
                                   wchar_t      **__path)
{
  if (!__url || !__plugin || !__path)
    return VFS_ERROR;

  wchar_t *s=wcsstr (__url, VFS_PLUGIN_DELIMETER);

  // Plugin name hasn't been found
  if (!s)
    return VFS_ERR_INVLAID_ARGUMENT;

  size_t pos=wcslen (__url)-wcslen (s);  // Position of occurance
  wchar_t *plugin_name=wcsndup (__url, pos); // Name of plugin

  // Set output parameters
  (*__plugin)=vfs_plugin_by_name (plugin_name);
  if (!*__plugin)
    {
      free (plugin_name);
      return VFS_ERROR;
    }

  (*__path)=wcsdup (__url+pos+wcslen (VFS_PLUGIN_DELIMETER));

  // Free temporary variables
  free (plugin_name);

  return VFS_OK;
}
