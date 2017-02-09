/**
 * Includes:
 * reuseDW
 * try2saveDW
 */

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <math.h>

#include "shapefil.h"
#include "mims_spatl.h"
#include "parms3.h"

/* ============================================================= */

/* Read intermediate intersection files */
int reuseDW(
  PolyObject *pw, /* parent weight polygons */
  PolyObject *pd, /* parent data polygons */ 
  PolyObject **pwd, /* intersected polygons read from file */
  char *ename /* environment variable name (e.g. "USE_DW_FILE") */ )
{
  char *fname;
  char *cat="SAVED_ID";
  int i;
  int ii, jj;
  int n1, n2;
  int n;
  PolyShapeList *plist;
  Parent **p1;
  Parent **p2;
  PolyParent *pp;


  fname = getenv(ename);   
  if (fname == NULL) return 0;
  fname = (char *) strdup(getenv(ename));

  if (!strcmp(fname, "NONE")) return 0;

  /*putenv("SAVED_ID=PARENT1ID,PARENT2ID");*/

  /* read the shapes into pwd */
  *pwd = PolyShapeReader(ename, NULL, NULL, NULL);
  if (*pwd == NULL) return 0;
  /* associate attributes for parent polygons with polygon data */
  attachDBFAttribute(*pwd,cat,NULL);

  n1 = pw->nObjects;
  n2 = pd->nObjects;

  /* allocate space for the parent polygon shapes */  
  p1 = (Parent **)malloc(n1*sizeof(Parent *));
  p2 = (Parent **)malloc(n2*sizeof(Parent *));
  if (!p1 || !p2) {
    WARN("Allocation error in reuseDW");
    return 0;
  }

  /* recreate associations between intersected polygons and parent
   * polygons; required to connect back to attributes of parents */
  plist = pw->plist;
  for (i=0; i<n1; i++) {
    pp = plist->pp;
    p1[i] = newParent(pp, plist->ps, i);
    plist = plist->next;
  }

  plist = pd->plist;
  for (i=0; i<n2; i++) {
    pp = plist->pp;
    p2[i] = newParent(pp, plist->ps, i);
    plist = plist->next;
  }

  n = (*pwd)->nObjects;;
  plist = (*pwd)->plist;
  for (i=0; i<n; i++) {

    ii=(*pwd)->attr_val[i][0].ival;
    jj=(*pwd)->attr_val[i][1].ival;

    pp = newPolyParent(p1[ii],p2[jj]);
    if(plist->pp != NULL) {
      free(plist->pp);
    }
    plist->pp = pp;

    plist = plist->next;
  }
  (*pwd)->parent_poly1 = pw;
  (*pwd)->parent_poly2 = pd;

  return 1;
}

/* ============================================================= */

/* Save intermediate intersection files between sets of polygons 
 * (e.g. the intersection between the 
 * weight polygons and the data polygons - these are independent of grid).
 *
 * Caution: detailed checking is not performed when the intermediate
 * files are read back in.
 *
 * Returns the status of the operation (success / failure)
 */
int try2saveDW(
   PolyObject *p, /* the set of all polygons in intermediate intersection */
   char *ename    /* e.g. "SAVE_DW_FILE" */ ) 
{

  char *name;
  int i, n;
  DBFHandle        hDBF;
  PolyShapeList *plist;

  name = getenv(ename);
  if (name == NULL) return 0;
  name = (char *) strdup(getenv(ename));
  if (strcmp(name,"NONE") == 0) return 0;

  /* write out the geometry of the shapes */
  if (polyShapeWrite(p, name) != 0) {
    ERROR("try2saveDW","Could not write data-weight poly shape file",2);
  }


  /* write out the DBF portion of the shape file */

  hDBF = DBFCreate( name );
  if( hDBF == NULL ) {
    WARN2("DBFCreate failed for ", name);
    return 2;
  }

  /* each polygon came from an intersection of 2 parent polygons, so 
   * make the 2 parents be the fields in the DBF (other information 
   * can be looked up later from the parent files */
  if( DBFAddField( hDBF, "PARENT1ID", FTInteger, 20, 0 ) == -1 ) {
    WARN2("DBFAddField failed for ", name);
    return 2;
  }
  if( DBFAddField( hDBF, "PARENT2ID", FTInteger, 20, 0 ) == -1 ) {
    WARN2("DBFAddField failed for ", name);
    return 2;
  }
    
  /* write the parent info for each polygon object */
  n = p->nObjects;
  plist = p->plist;
  for (i=0; i<n; i++) {
    if (DBFWriteIntegerAttribute(hDBF, i, 0, plist->pp->p1->index) != TRUE) {
      WARN("ERROR in DBFWriteIntegerAttribute");
      return 2;
    }
    if (DBFWriteIntegerAttribute(hDBF, i, 1, plist->pp->p2->index) != TRUE) {
      WARN("ERROR in DBFWriteIntegerAttribute");
      return 2;
    }
    plist = plist->next;
  }

  DBFClose( hDBF );

  return 0;
}


