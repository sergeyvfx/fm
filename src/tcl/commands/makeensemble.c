/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Create commands `ensemble' stuff implementation.
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include <tcl.h>
#include "makeensemble.h"

/**
 * Create an ensemble from a table of implementation commands
 *
 * @param __interp - a pointer on interpreter
 * @param __cmdname - name of the family commands
 * @param __subcommand - a list of subcommands information
 * return handle for the ensemble or NULL if creation of it fails
 */
Tcl_Command
tcl_make_ensemble (Tcl_Interp *__interp,
                   const char *__cmdname,
                   const ensemblecmd_t *__subcommand)
{
  Tcl_Command   ensemble;
  Tcl_Namespace *ns;
  Tcl_DString   buf;

  Tcl_DStringInit(&buf);
  Tcl_DStringAppend(&buf, "::", -1);
  Tcl_DStringAppend(&buf, __cmdname, -1);

  ns = Tcl_FindNamespace(__interp, Tcl_DStringValue(&buf), NULL, 0);
  if ( ns == NULL)
    {
      ns = Tcl_CreateNamespace(__interp, Tcl_DStringValue(&buf), NULL, NULL);
    }

  ensemble = Tcl_CreateEnsemble(__interp, Tcl_DStringValue(&buf),
                                ns, TCL_ENSEMBLE_PREFIX);
  Tcl_DStringAppend(&buf, "::", -1);
  if (ensemble != NULL)
    {
      Tcl_Obj *mapdict, *to, *from;
      int i, code;

      mapdict = Tcl_NewObj();
      for (i = 0; __subcommand[i].name != NULL; ++i)
        {
          from = Tcl_NewStringObj (__subcommand[i].name, -1);
          to = Tcl_NewStringObj (Tcl_DStringValue (&buf),
                                 Tcl_DStringLength (&buf));
          Tcl_AppendToObj (to, __subcommand[i].name, -1);
          code = Tcl_DictObjPut (__interp, mapdict, from, to);
          Tcl_CreateObjCommand (__interp, Tcl_GetString(to),
                                __subcommand[i].proc, NULL, NULL);
        }
      code = Tcl_SetEnsembleMappingDict(__interp, ensemble, mapdict);
    }

  Tcl_DStringFree(&buf);
  return ensemble;
}
