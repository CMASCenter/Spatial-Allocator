/****************************************************************************
 *  PolygonFileReader.c
 *
 *  Created: 4/18/2005 BDB
 *
 *  Purpose:  Allows a polygon to be described in an ASCII file format
 *            and used for spatial allocation, especially the overlay function
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
                                                                                            
#include "shapefil.h"
#include "mims_spatl.h"
#include "mims_evs.h"
#include "parms3.h"
#include "io.h"


PolyObject *PolygonFileReader(char *ename, MapProjInfo *file_mapproj, 
                                 MapProjInfo *output_mapproj)
{
    FILE *ifp;
    PolyObject *poly;
    PolyShapeList *plist;
    PolyShape *ps;
    Shape *shp;
    extern char *prog_name;
    char inputFileName[256], mesg[256], lineBuffer[200], de[30];
    char fileName[256];
    char *header, *tmpString, *ptr;
    int count, nVertices, nObjects, nShapeType;
    double minX, minY, maxX, maxY;
    double xVal, yVal, projx, projy;
    char delimiter[2];

    if(!getEnvtValue(ENVT_OVERLAY_DELIM, de))
    {
        sprintf(mesg, "%s must be set in order to use a PointFile",
                         ENVT_OVERLAY_DELIM);
        ERROR(prog_name, mesg, 2);
     }
     if(!strcmp(de, "COMMA"))
     {
          strcpy(delimiter,",");
     }
     else if(!strcmp(de, "SEMICOLON"))
     {
          strcpy(delimiter, ";");
     }
     else if(!strcmp(de, "PIPE"))
     {
          strcpy(delimiter, "|");
     }
     /* add other delimiters here */
     else
     {
          /* no delimiter specified so use a space */
          strcpy(delimiter, " ");
     }
     if(!getEnvtValue(ename, fileName))
     {
        sprintf(mesg,"Unable to read fileName from %s\n", ename);
        ERROR(prog_name, mesg, 2);
     }
     if((ifp = fopen(fileName, "r")) == NULL)
     {
        sprintf(mesg,"Unable to read %s\n", fileName);
        ERROR(prog_name, mesg, 2);
     }
     nVertices = 0;
    
     /* read in each line of the PolygonFile and fill up the poly structure */
     while(fgets(lineBuffer, sizeof(lineBuffer), ifp) != NULL)
     {
         nVertices++;
     }
     rewind(ifp);

     poly = getNewPoly(1);

     if (poly == NULL)
     {
        sprintf (mesg, "Cannot allocate POLY struct");
        ERROR (prog_name, mesg, 2);
     }

     poly->nSHPType = SHPT_POLYGON;
     poly->map = file_mapproj;
     poly->bb = newBBox(0, 0, 0, 0);

     storeProjection(file_mapproj, output_mapproj);
     
     /*printBoundingBox(poly->bb);*/

     ps = getNewPolyShape(1);
     ps->num_contours = 0;

     shp = getNewShape(nVertices);

     for(count = 0; count < nVertices; count++)
     {
          fgets(lineBuffer, sizeof(lineBuffer), ifp);
          tmpString = strtok(lineBuffer, delimiter);

          if(tmpString == NULL)
          {
              sprintf(mesg, "Polygon File %s not formatted correctly", fileName);
              ERROR(prog_name, mesg, 2);
          }

          xVal = atof(tmpString);

          tmpString = strtok(NULL, delimiter);

          if(tmpString == NULL)
          {
              sprintf(mesg, "Polygon File %s not formatted correctly", fileName);
              ERROR(prog_name, mesg, 2);
          }

          yVal = atof(tmpString);
          projectPoint(xVal, yVal, &projx, &projy);

          shp->vertex[count].x = projx;
          shp->vertex[count].y = projy;
     }

     /*minX = 1E20;
     minY = 1E20;
     maxX = -1E20;
     maxY = -1E20;
     */

     /* try calling recomputeBoundingBox */
     /*for(count = 0; count < nVertices; count++)
     {
         if(minX > shp->vertex[count].x)
         {
             minX = shp->vertex[count].x;
         }

         if(minY > shp->vertex[count].y)
         {
             minY = shp->vertex[count].y;
         }

         if(maxX < shp->vertex[count].x)
         {
             maxX = shp->vertex[count].x;
         }

         if(maxY < shp->vertex[count].y)
         {
             maxY = shp->vertex[count].y;
         }

     }*/
     /*poly->bb = newBBox(minX, minY, maxX, maxY);*/

     gpc_add_contour(ps, shp, NOT_A_HOLE);
     polyShapeIncl(&(poly->plist), ps, NULL);
     recomputeBoundingBox(poly);

     return poly;
}

#ifdef TEST_POLYR
char *prog_name = "Polygon File Tester";
int main(int argc, char *argv[])
{
     char *name = "test";
     int counter;
     PolyObject *p;

     p = PolygonFileReader(getenv(ENVT_INPUT_FILE_NAME), NULL);

     return 0;
}
#endif

