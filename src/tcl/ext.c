/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * File association implementation
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include <string.h>
#include <stdlib.h>
#include <util.h>

#include "util.h"
#include "ext.h"

typedef struct extobj_t
{
  int epoch;    /* Epoch counter */
  int refcount; /* Reference counter */

  Tcl_Obj       *pattern;   /* Tcl string representation pattern */
  extension_t   extension;  /* Incapsulate extenstion command */
} extobj_t;

typedef struct extension_list_t
{
  int epoch;        /* Epoch counter */
  int refcount;     /* Reference counter */

  Tcl_Obj *extslist;
} extension_list_t;

static void
update_string_of_extobj (Tcl_Obj *);

static void
free_extobj_internal_rep (Tcl_Obj *);

static void
dup_extobj_internal_rep (Tcl_Obj *, Tcl_Obj *);

static int
set_extobj_from_any (Tcl_Interp *interp, Tcl_Obj *anyobj);

/*
 * The following structure defines the implementation of the `extension'
 * Tcl object
 */

static Tcl_ObjType tcl_extobj_type = {
    "extension",              /* Name of the type */
    free_extobj_internal_rep, /* Called to free any storage for the type's
                                 internal rep */
    dup_extobj_internal_rep,  /* Called to create a new object as a copy of
                                 an existing object */
    update_string_of_extobj,  /* Called to update the string rep from the
                                 type's internal representation */
    set_extobj_from_any       /* Called to convert the object's internal rep
                                 to this type */
};

static void
update_string_of_extobjlist_object (Tcl_Obj *);

static void
free_extlistobj_internal_rep (Tcl_Obj *);

static void
dup_extlistobj_internal_rep (Tcl_Obj *, Tcl_Obj *);

static int
set_extlistobj_from_any (Tcl_Interp *interp, Tcl_Obj *anyobj);

/*
 * The following structure defines the implementation of the `extensions list'
 * Tcl object
 */

static Tcl_ObjType tcl_extlistobj_type = {
    "extensions list",                  /* Name of the type */
    free_extlistobj_internal_rep,       /* Called to free any storage for the
                                           type's internal rep */
    dup_extlistobj_internal_rep,        /* Called to create a new object
                                           as a copy of an existing object */
    update_string_of_extobjlist_object, /* Called to update the string rep
                                           from the type's internal
                                           representation */
    set_extlistobj_from_any             /* Called to convert the object's
                                           internal rep to this type */
};

/*
 * Create new file's associations (FA) object
 *
 * @return a pointer on new FA object
 */
Tcl_Obj *
tcl_fa_new_object(void)
{
  extension_list_t  *extlist;
  Tcl_Obj           *newobject;

  newobject = Tcl_NewObj();
  Tcl_InvalidateStringRep (newobject);

  extlist = (extension_list_t *) Tcl_Alloc (sizeof (extension_list_t));
  extlist->epoch    = 0;
  extlist->refcount = 1;

  /* NOTE: can be optimized, replace NewListObj on NewObj */
  extlist->extslist = Tcl_NewListObj (0, NULL);

  newobject->internalRep.otherValuePtr = extlist;
  newobject->typePtr = &tcl_extlistobj_type;

  return newobject;
}

/**
 * Create new `extension' object for FA
 *
 * @param __pattern - a pattern describes the file extension (regexp compat.)
 * @param __viewer  - a viewer application name
 * @param __editor  - a editor application name
 * @return a pointer on new `extension' object
 */
Tcl_Obj *
tcl_fa_new_extobject (const char *__pattern,
                      const wchar_t *__viewer, const wchar_t *__editor)
{
  Tcl_Obj       *newobject;
  extobj_t      *extobj;

  newobject = Tcl_NewObj();
  Tcl_InvalidateStringRep (newobject);

  extobj = (extobj_t *) Tcl_Alloc (sizeof (extobj_t));
  extobj->epoch     = 0;
  extobj->refcount  = 1;

  extobj->pattern          = Tcl_NewStringObj (__pattern, -1);
  extobj->extension.viewer = extobj->extension.editor = NULL;

  if (__viewer != NULL)
    {
      extobj->extension.viewer  = wcsdup (__viewer);
    }

  if (__editor != NULL)
    {
      extobj->extension.editor  = wcsdup (__editor);
    }

  newobject->internalRep.otherValuePtr = extobj;
  newobject->typePtr                   = &tcl_extobj_type;

  return newobject;
}

/**
 * Add new `extension' to `extensions list' into Tcl representation
 *
 * @param __interp - a pointer on a Tcl interpreter
 * @param __extlistobj - a pointer on a `extensions list' object in Tcl
 * representation.
 * @param __extobj - a pointer on a `extension' object in Tcl representation.
 * @return TCL_OK if successful, otherwise TCL_ERROR
 */
int
tcl_fa_put_object(Tcl_Interp *__interp, Tcl_Obj *__extlistobj,
                  Tcl_Obj *__extobj)
{
  extension_list_t *extlist;
  extension_t      *ext;

  if (__extlistobj->typePtr != &tcl_extlistobj_type)
    {
      /** TODO: Added error message **/
      return TCL_ERROR;
    }

  extlist = (extension_list_t *)__extlistobj->internalRep.otherValuePtr;
  ext = __extobj->internalRep.otherValuePtr;
//  Tcl_IncrRefCount(extobj);

  if (Tcl_ListObjAppendElement (__interp,
                                extlist->extslist, __extobj) != TCL_OK)
    {
      /* TODO: Added error message */
      return TCL_ERROR;
    }

  ++extlist->epoch;

  return TCL_OK;
}

static int
set_extobj_from_any (Tcl_Interp *__interp, Tcl_Obj *__anyobj)
{
  fprintf (stderr, "call: set_extobj_from_any for `extobj' \n");
  fprintf (stderr, "      operation not supported\n");
  return TCL_ERROR;
}

static int
set_extlistobj_from_any (Tcl_Interp *__interp, Tcl_Obj *__anyobj)
{
  fprintf (stderr, "call: set_extlistobj_from_any for `extlist' \n");
  fprintf (stderr, "      operation not supported\n");
  return TCL_ERROR;
}

static void
update_string_of_extobjlist_object (Tcl_Obj *extlistobj)
{
  extension_list_t  *extlist;
  Tcl_Obj           **lobjv;
  int               i, lobjc;
  Tcl_DString       buf;

  extlist = extlistobj->internalRep.otherValuePtr;

  Tcl_DStringInit (&buf);
  Tcl_ListObjGetElements (NULL, extlist->extslist, &lobjc, &lobjv);

  Tcl_DStringStartSublist (&buf);
    for (i = 0; i < lobjc; ++i)
      {
        Tcl_DStringAppend (&buf, Tcl_GetString (lobjv[i]), -1);
      }
  Tcl_DStringEndSublist (&buf);

  extlistobj->bytes = ckstrdup (Tcl_DStringValue (&buf));
  extlistobj->length = Tcl_DStringLength (&buf) + 1;

  Tcl_DStringFree (&buf);
}

static void
update_string_of_extobj (Tcl_Obj *extobjobj)
{
  Tcl_DString   buf;
  extobj_t      *extobj = extobjobj->internalRep.otherValuePtr;

  Tcl_DStringInit (&buf);
    Tcl_DStringStartSublist (&buf);
      Tcl_DStringAppendElement (&buf, Tcl_GetString (extobj->pattern));
      if (extobj->extension.editor)
        {
          char *editor = NULL;
          wcs2mbs (&editor, extobj->extension.editor);

          Tcl_DStringStartSublist (&buf);
            Tcl_DStringAppendElement (&buf, "e");
            Tcl_DStringAppendElement (&buf, editor);
          Tcl_DStringEndSublist (&buf);

          free (editor);
        }
      if (extobj->extension.viewer)
        {
          char *viewer = NULL;
          wcs2mbs (&viewer, extobj->extension.viewer);

          Tcl_DStringStartSublist (&buf);
            Tcl_DStringAppendElement (&buf, "v");
            Tcl_DStringAppendElement (&buf, viewer);
          Tcl_DStringEndSublist (&buf);

          free (viewer);
        }
    Tcl_DStringEndSublist (&buf);

    extobjobj->bytes  = ckstrdup (Tcl_DStringValue(&buf));
    extobjobj->length = Tcl_DStringLength (&buf) + 1;
  Tcl_DStringFree (&buf);
}

static void
free_extobj_internal_rep (Tcl_Obj *extobjobj)
{
  extobj_t *extobj = extobjobj->internalRep.otherValuePtr;

  fprintf (stderr, "call: free_extobj_internal_rep\n");

  --extobj->refcount;
  if (extobj->refcount < 1)
    {
      Tcl_DecrRefCount (extobj->pattern);
      if (extobj->extension.editor)
        {
          ckfree ((void *) extobj->extension.editor);
          extobj->extension.editor = NULL;
        }
      if (extobj->extension.viewer)
        {
          ckfree ((void *) extobj->extension.viewer);
          extobj->extension.viewer = NULL;
        }

      ckfree ((char *) extobj);
    }

  extobjobj->internalRep.otherValuePtr = NULL;
}

static void
free_extlistobj_internal_rep (Tcl_Obj *extlistobj)
{
  extension_list_t *extlist = extlistobj->internalRep.otherValuePtr;
  --extlist->refcount;

  fprintf (stderr, "call: free_extlistobj_internal_rep\n");

  if (extlist->refcount < 1)
    {
      Tcl_DecrRefCount (extlist->extslist);
      ckfree ((char *) extlist);
    }
  extlistobj->internalRep.otherValuePtr = NULL;
}

static void
dup_extobj_internal_rep (Tcl_Obj *sourceobj, Tcl_Obj *copyobj)
{
  fprintf (stderr, "call: dup_extobj_internal_rep\n");
  fprintf (stderr, "      operation not supported\n");
}

static void
dup_extlistobj_internal_rep (Tcl_Obj *sourceobj, Tcl_Obj *copyobj)
{
  extension_list_t *extlist = sourceobj->internalRep.otherValuePtr;
  extension_list_t *newextlist =
    (extension_list_t *) ckalloc(sizeof(extension_list_t));

  newextlist->epoch = 0;
  newextlist->refcount = 1;
  newextlist->extslist = Tcl_NewListObj(0, NULL);
  Tcl_ListObjAppendList (NULL, newextlist->extslist, extlist->extslist);

  copyobj->internalRep.otherValuePtr = newextlist;
  copyobj->typePtr = &tcl_extlistobj_type;

  fprintf (stderr, "call dup_extlistobj_internal_rep\n");
}

/**
 * Attempt to return an file extension from the Tcl object
 *
 * @param __extobj - a pointer on `extension' Tcl object
 * @return a pointer on file extension structures if successful, otherwise NULL
 */
extension_t *
tcl_extcmd_from_object (Tcl_Obj *__extobj)
{
  extension_t *ext = malloc (sizeof (extension_t));
  /* TODO: check Tcl_Obj ptrType */
  extobj_t    *fromobject = __extobj->internalRep.otherValuePtr;

  ext->editor = ext->viewer = NULL;

  if (fromobject->extension.editor != NULL)
    {
      ext->editor = wcsdup (fromobject->extension.editor);
    }

  if (fromobject->extension.viewer != NULL)
    {
      ext->viewer = wcsdup (fromobject->extension.viewer);
    }

  return ext;
}

/**
 * Given a `extension` object from `extensions list`
 * if passed the file name corresponds to pattern
 *
 * @param __interp - a pointer on Tcl interpreter
 * @param __faobject - a pointer on `extenstions list' object
 * @param __textobj - a file name in Tcl object
 * @param __value - a result value, contains `extension' object
 * @return TCL_OK if successful, otherwise TCL_ERROR
 */
int
tcl_fa_get_object (Tcl_Interp *__interp, Tcl_Obj *__faobject,
                   Tcl_Obj *__textobj, Tcl_Obj **__value)
{
  extension_list_t *extlist;
  extobj_t         *extobj;

  Tcl_Obj          **lobjv;
  int              i, lobjc;

  if (__faobject->typePtr != &tcl_extlistobj_type)
    {
      /* TODO: Return more mnemonic error into interp */
      return TCL_ERROR;
    }

  extlist = __faobject->internalRep.otherValuePtr;
  Tcl_ListObjGetElements (__interp, extlist->extslist, &lobjc, &lobjv);
  for (i = 0; i < lobjc; ++i)
    {
      extobj = lobjv[i]->internalRep.otherValuePtr;
      if (Tcl_RegExpMatchObj (__interp, __textobj, extobj->pattern) > 0)
        {
          *__value = lobjv[i];
          return TCL_OK;
        }
    }
  return TCL_ERROR;
}
