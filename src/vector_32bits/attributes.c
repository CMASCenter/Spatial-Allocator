/*****************************************************************************
 * attributes.c
 *
 * Includes:
 * attachAttribute
 * getNewAttrHeader
 * addNewAttrHeader
 *
 * Updated 5/23/2005 -- added support for Regular Grids with no attribs
 *                      in attachAttribute  BDB
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <math.h>

#include "shapefil.h"
#include "mims_spatl.h"
#include "parms3.h"
#include "io.h"

/* ============================================================= */

int attachAttribute(PolyObject *poly, char *attr_env_name, char *ctgr_name)
{
  char type[100], mesg[256];
  extern char *prog_name;
  char **attribList;
  int i, ncnt, nmax;

  if(attr_env_name == NULL) /* no attribs */
  {
       return attachDBFAttribute(poly, NULL, ctgr_name);
  }
  else if(getEnvtValue(attr_env_name, type) )
  {
     if (!strcmp(type,"VEGFRAC"))
     {
        return 1;    /*attributes have been read*/
     }
     else
     {
        return attachDBFAttribute(poly, attr_env_name, ctgr_name);
     }
  }

  sprintf(mesg, "Unable to continue because %s has not been set", attr_env_name);
  ERROR(prog_name, mesg, 2);

  return 0;
}

/* ============================================================= */

#ifdef USE_IOAPI

int attachAttributeIoapi(PolyObject *poly, char *attr_env_name, 
                         char *ctgr_name, char *file_name)
{
    char type[100], mesg[256];
    extern char *prog_name;

    if(attr_env_name == NULL) /* no attribs */
    {
        return attachIoapiAttribute(poly, NULL, ctgr_name, file_name);
    }
    else if(getEnvtValue(attr_env_name, type) )
    {
        if (!strcmp(type,"VEGFRAC"))
        {
            return 1;    /* attributes have been read */
        }
        else
        {
            return attachIoapiAttribute(poly, attr_env_name, ctgr_name, file_name);
        }
    }
    
    sprintf(mesg, "Unable to continue because %s has not been set", attr_env_name);
    ERROR(prog_name, mesg, 2);
    
    return 0;
}

#endif

/* ============================================================= */
/* Create an empty attribute header / name */

AttributeHeader *getNewAttrHeader(void)
{
  AttributeHeader *p;
  
  p = (AttributeHeader *)malloc(sizeof(AttributeHeader));
  if (p) {
    p->num_attr = 0;
    p->attr_desc = NULL;
  }

  return p;
}

/* ============================================================= */
/* Add a new attribute to p, the list of attributes for a shape file. 
 * return the index of the attribute in the list */

int addNewAttrHeader(  
   AttributeHeader *p, /* list of attributes for a shape file */   
   char *name, /* name of new attribute to add */  
   int type  /* type of the attribute */ )
{

  int i, n;
  int found;
  AttributeDesc **attr_desc;

  if ( !p ) {
    WARN("Attribute header not valid");
    return -1;
  }

  /* search so we do not insert attributes already there */
  found = 0;
  n = p->num_attr;
  for (i=0; i < n; i++) {
    /* search loop */
    if (!strcmp(p->attr_desc[i]->name, name)) {
      found = 1;
      break;
    }
  }

  if (found) return i;
  i = n;
  n++; /* we are adding 1 more attribute */
  attr_desc = (AttributeDesc **)realloc(p->attr_desc,
                                        n*sizeof(AttributeDesc *));
  if (attr_desc) {
    p->attr_desc = attr_desc;
    attr_desc[i] = (AttributeDesc *)malloc(sizeof(AttributeDesc));
    if (attr_desc[i]) {
      p->num_attr = n;
      attr_desc[i]->name = (char *) strdup(name);
      attr_desc[i]->type = type;
    }
  }

  return i;
}


