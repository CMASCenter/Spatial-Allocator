#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <math.h>

#include "shapefil.h"
#include "mims_spatl.h"
#include "parms3.h"

/* ============================================================= */
/* read a regular grid description and create corresponding polygons */
PolyObject *TestPolyReader( 
   char *name, /* file name to read */
   BoundingBox *bbox,  /* bounding box of interest - ignore if NULL */ 
   MapProjInfo *mpinfo)
{

  PolyObject *poly;
  Shape *shp;
  PolyShape *ps;
  MapProjInfo *map;
  int nObjects;
  int nv, np;
  int c, r;
  char mesg[256];
  char *earth_ellipsoid;

  char  *gname;
  char   cname[NAMLEN3+1];
  int    ctype;
  double p_alp;
  double p_bet;
  double p_gam;
  double xcent;
  double ycent;
  double xorig;
  double yorig;
  double xcell=10000;
  double ycell=10000;
  int    ncols;
  int    nrows;
  int    nthik;
  char *ioapi_sphere="+a=6370000.0,+b=6370000.0";

  map = getNewMap();
  if (map == NULL) {
    sprintf(mesg,"getNewMap error for %s", name);
    goto error;
  }

  //ellipsoid doesn't matter for a grid - the IO/API supports sphere only
  if ((map->earth_ellipsoid = (char *) strdup(ioapi_sphere)) == NULL) {
        sprintf(mesg,"Allocation error for map's ellipsoid");
        goto error;
  }

  map->p_alp = mpinfo->p_alp;
  map->p_bet = mpinfo->p_bet;
  map->p_gam = mpinfo->p_gam;
  map->xcent = mpinfo->xcent;
  map->ycent = mpinfo->ycent;
  map->xcell = mpinfo->xcell;
  map->ycell = mpinfo->ycell;
  map->xorig = mpinfo->xorig;
  map->yorig = mpinfo->yorig;
  map->ctype = mpinfo->ctype;
  map->gridname = (char *) strdup(mpinfo->gridname);
  map->ncols = mpinfo->ncols;
  map->nrows = mpinfo->nrows;

  nObjects = 1;

  poly = getNewPoly(nObjects);
  if (poly == NULL) {
    sprintf(mesg,"Cannot allocate POLY struct");
    goto error;
  }
  poly->nSHPType = SHPT_POLYGON;
  poly->map = map;

  poly->bb = newBBox(xorig, yorig, xorig+xcell, yorig+ycell);
  nv = 4; /* num vertices */
  np = 1; /* num parts of shape */
  ps = getNewPolyShape(np);
  ps->num_contours = 0; /* we need to reset,
                             since gpc_add_contour increments num_contours */

  /* clockwise shape */
  shp = getNewShape(nv);      
  shp->vertex[0].x = xorig;
  shp->vertex[0].y = yorig;
  shp->vertex[1].x = xorig;
  shp->vertex[1].y = yorig+ycell*2;
  shp->vertex[2].x = xorig+xcell*2;
  shp->vertex[2].y = yorig + ycell*2;
  shp->vertex[3].x = xorig+xcell*2;
  shp->vertex[3].y = yorig;

  gpc_add_contour(ps, shp, NOT_A_HOLE);
  polyShapeIncl(&(poly->plist), ps, NULL);
  
  /* counter clockwise hole */
  shp = getNewShape(nv);      
  shp->vertex[0].x = xorig;
  shp->vertex[0].y = yorig;
  shp->vertex[1].x = shp->vertex[0].x + xcell;
  shp->vertex[1].y = shp->vertex[0].y;
  shp->vertex[2].x = shp->vertex[1].x;
  shp->vertex[2].y = shp->vertex[1].y + ycell;
  shp->vertex[3].x = shp->vertex[0].x;
  shp->vertex[3].y = shp->vertex[2].y;

  gpc_add_contour(ps, shp, NOT_A_HOLE);
  polyShapeIncl(&(poly->plist), ps, NULL);
  
  return poly;

 error:
  WARN(mesg);
  return NULL; /* we should never get here */
}
