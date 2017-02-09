/*****************************************************************************
 * centroid.c
 * 
 * Created by:  Ben Brunk, CEP June 2005
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "shapefil.h"
#include "mims_spatl.h"
#include "mims_evs.h"           
#include "io.h"                 

PolyObject *getCentroidPoly(PolyObject *p)
{
     PolyObject *c_poly;
     PolyShape *ps, *cps;
     Shape *shp, *cshp;
     PolyShapeList *plist;
     gpc_vertex vx;
     int numObjects, n;
     char mesg[256];
     extern char *prog_name;
     /* take data poly centroids, put them in a poly of their own */

     numObjects = p->nObjects;

     c_poly = getNewPoly(numObjects); 
     c_poly->name = (char *) strdup("Centroid poly");
     c_poly->nSHPType = SHPT_POINT;
/*      printf("getCentroidPoly: copying map proj\n"); */
     c_poly->map = copyMapProj(p->map);
     c_poly->parent_poly1 = p->parent_poly1;
     c_poly->parent_poly2 = p->parent_poly2;

     c_poly->bb = newBBox(0, 0, 0, 0);


    if(c_poly == NULL)
    {
        sprintf(mesg, "Unable to allocate POLY structure for centroid polygon");
        ERROR(prog_name, mesg, 1);     
    }

    plist = p->plist;

    for(n = 0; n < numObjects; n++)
    {
        cps = getNewPolyShape(1); /* just one contour per shape */
        cps->num_contours = 0;

        ps = plist->ps;

        vx = getCentroidVertex(ps);
        
        cshp = getNewShape(1); /* just one shape per contour */
        cshp->vertex[0].x = vx.x;
        cshp->vertex[0].y = vx.y;
        
        gpc_add_contour(cps, cshp, NOT_A_HOLE);
        polyShapeIncl(&(c_poly->plist), cps, NULL);

        plist = plist->next;       
    }

    /* still need to get the attributes from d_poly and attach to c_poly, can use
       same loop above */
   recomputeBoundingBox(c_poly);
   
   return c_poly;
}


/* Calculates the centroid (average of all points) */
gpc_vertex getCentroidVertex(PolyShape *ps)
{
     gpc_vertex vx;
     int c, v;
     double totx = 0.0, toty = 0.0, avgx = 0.0, avgy = 0.0;
     

     /* average the vertices of ps */
     for(c = 0; c < ps->num_contours; c++)
     {
          for(v = 0; v < ps->contour[c].num_vertices; v++)
          {
               avgx += ps->contour[c].vertex[v].x;
               avgy += ps->contour[c].vertex[v].y;
               totx++; 
               toty++;
          }
     }
    
     vx.x = avgx / totx;
     vx.y = avgy / toty;
     

     /*printf("centroid x=%f centroid y=%f\n", vx.x, vx.y);*/

     return vx;
}

