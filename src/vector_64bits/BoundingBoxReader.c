/*********************************************************************************   
 *   BoundingBoxReader.c
 *
 *   created: 4/7/2005 by Benjamin Brunk, Carolina Environmental Program
 *    
 *   Provides a means to create a bounding box shape from coordinates provided 
 *   in a run script.
 **********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mims_spatl.h"
#include "mims_evs.h"
#include "shapefil.h"
#include "io.h"

/* ============================================================= */
/* read in bounding box coordinates and create a  polygon */
PolyObject *BoundingBoxReader(MapProjInfo *file_mapproj,/* the map projection for the grid*/
                    MapProjInfo *output_mapproj) 
{

     PolyObject *poly=NULL;
     PolyShape *ps;
     char mesg[256], coordsList[256], *tmpString;
     extern char* prog_name;
     Shape *newbbox;
     int retVal;
     double x1, x2, y1, y2, projx, projy;


     if(!getEnvtValue(ENVT_OVERLAY_SHAPE, coordsList))
     {
          sprintf(mesg, "Bounding Box coordinates not specified in %s",
                    ENVT_OVERLAY_SHAPE);
          ERROR(prog_name, mesg, 2);
     }
                                                                                              
     /* parse the list of coordinates */
     tmpString = strtok(coordsList,",");
                                                                                           
     if(tmpString == NULL)
     {
          sprintf(mesg, "Check coordinates specified in %s",
                ENVT_OVERLAY_SHAPE);
          ERROR(prog_name, mesg, 2);
     }
                                                                                              
     x1 = atof(tmpString);
                                                                                              
     tmpString = strtok(NULL, ",");
     if(tmpString == NULL)
     {
          sprintf(mesg, "Check coordinates specified in %s",
               ENVT_OVERLAY_SHAPE);
          ERROR(prog_name, mesg, 2);
     }
                                                                                              
     y1 = atof(tmpString);
                                                                                              
     tmpString = strtok(NULL, ",");
 
     if(tmpString == NULL)

     {
          sprintf(mesg, "Check coordinates specified in %s",
               ENVT_OVERLAY_SHAPE);
          ERROR(prog_name, mesg, 2);
     }
                                                                                              
     x2 = atof(tmpString);
                                                                                              
     tmpString = strtok(NULL, ",");
     if(tmpString == NULL)
     {
          sprintf(mesg, "Check coordinates specified in %s",
                      ENVT_OVERLAY_SHAPE);
          ERROR(prog_name, mesg, 2);
     }
                                                                                              
     y2 = atof(tmpString);
                                                                                              
                                                                                              
     /* verify x2 > x1 & y2 > y1 */
     if((x1 > x2) || (y1 > y2))
     {
          sprintf(mesg, "Error setting bounding box coordinates");
          ERROR(prog_name, mesg, 2);
     }
                                                                                              
     /* create a new poly with one object */
     poly = getNewPoly(1);
     if (poly == NULL)
     {
          sprintf(mesg,"Unable allocate POLY struct for bounding box");
          ERROR(prog_name, mesg, 2);
     }
     poly->nSHPType = SHPT_POLYGON;
     poly->map = file_mapproj;
     storeProjection(file_mapproj, output_mapproj);
                                                                                              
                                                                                              
     poly->bb = newBBox(x1, y1, x2, y2);
     printBoundingBox(poly->bb);
     ps = getNewPolyShape(1);
     ps->num_contours = 0;
     newbbox = getNewShape(4);

     projectPoint(x1, y1, &projx, &projy); 
     /* set the bbox coords based on the coords read in */
     newbbox->vertex[0].x = projx;
     newbbox->vertex[0].y = projy;

     projectPoint(x1, y2, &projx, &projy); 
     newbbox->vertex[1].x = projx;
     newbbox->vertex[1].y = projy;

     projectPoint(x2, y2, &projx, &projy); 
     newbbox->vertex[2].x = projx;
     newbbox->vertex[2].y = projy;

     projectPoint(x2, y1, &projx, &projy); 
     newbbox->vertex[3].x = projx;
     newbbox->vertex[3].y = projy;

     gpc_add_contour(ps, newbbox, NOT_A_HOLE);

     polyShapeIncl(&(poly->plist), ps, NULL);
                                                                                          
     recomputeBoundingBox(poly);

     return poly;
}
