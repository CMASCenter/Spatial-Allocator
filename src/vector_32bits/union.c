/********************************************************************************
 *  union.c
 *
 *
 *  Created: 4/19/2005  BDB
 *                        
 ********************************************************************************/
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <math.h>

#include "shapefil.h"
#include "mims_spatl.h"
#include "parms3.h"

/* ============================================================= */
/* Performs a union on a polygon, and  
 * returns the result as a set of contours (which could be points) in p 
 * The integer result is 0 if no intersection, 1 if there is an 
 * intersection, or -1 if there is an error. */
int polyUnion(
    PolyObject *inpoly, /* a set of polygons */
    PolyObject *p) /* the resulting points, lines, or polygons */ 
{

  int i, j;
  int n1, n2;
  int n;
  int at_least_one;
  int itemp = 0;
  PolyShape *polyResult, *tmpPoly;
  PolyParent *pp;
  PolyShapeList *plist, *plist2;
  Parent **p1;
  Parent **p2;
  double dummy = 0.0;
  char mesg[100];


  if(inpoly->nSHPType != SHPT_POLYGON ) 
  {
      WARN("Currently only POLYGONs are supported as second poly in polyIsect\n");
      return -1;
  }

  MESG("inpolybb: ");
  printBoundingBox(inpoly->bb);


  p->nSHPType = inpoly->nSHPType;
  p->map = inpoly->map; 
  p->parent_poly1 = NULL; 
  p->parent_poly2 = inpoly;
  

  n2 = inpoly->nObjects;
  
  /*sprintf(mesg,"numpoly1=%d, numinpoly = %d\n", n1, n2);
  MESG(mesg);  */
  p2 = (Parent **)malloc(n2*sizeof(Parent *)); /* may not need */

  if (!p2)  
  {
      WARN("Allocation error in polyIsect");
      return 0;
  }

  plist = inpoly->plist; /* may not need this block either */

  for (i=0; i < n2; i++) 
  {
      pp = plist->pp;
      p2[i] = newParent(pp, plist->ps, i);
      plist = plist->next;
  }
  plist2 = inpoly->plist;


  at_least_one = 0;

  polyResult = getNewPolyShape(0);

  for (j = 0; j < n2; j++) 
  {

      if (polyResult == NULL) 
      {
        WARN("Malloc error for getNewPolyShape");
        return -1;
      }
          
      gpc_polygon_clip(GPC_UNION, plist2->ps, polyResult,  polyResult);     
      /*gpc_polygon_clip(GPC_UNION, polyResult, plist2->ps, polyResult);*/
#ifdef DEBUG
      gpc_write_polygon(stderr, 0, plist2->ps);
      gpc_write_polygon(stderr, 0, polyResult);
#endif
      plist2 = plist2->next;
    } /* for (j=0; j<n2; j++) */
    
      n = polyResult->num_contours;
      /* if n > 0, there was a non-empty union of p1 and p2 */
      if(n) 
      {
         at_least_one = 1;
         p->nObjects++;
         pp = newPolyParent(p2[j], p2[j]);
         polyShapeIncl(&(p->plist), polyResult, pp);
      }
      else 
      {
         freePolyShape(polyResult);
      }
      
    p->bb = newBBox(dummy, dummy, dummy, dummy);

    recomputeBoundingBox(p);

    /* return whether there was a non-empty intersection between p1 and p2 */
    return at_least_one;
}

