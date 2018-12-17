/****************************************************************************
 * EGridReader.c
 *
 * Creates polygons based on a arcGIS polygon shape text file and
 * obtain projection information from GRIDDESC.txt file.
 *
 * 2/06/2006 -- The shape text file has a gridname defined in 
 *              the GRIDDESC.txt file.  L. Ran
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#if defined(__MACH__)
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include "shapefil.h"
#include "mims_spatl.h"
#include "mims_evs.h"
#include "parms3.h"
#include "io.h"

typedef struct _point {
  double x;
  double y;
  struct _point *next;
} point;


/* ============================================================= */
/* read a regular grid description and create corresponding polygons */
PolyObject *EGridReader(char *name,       /* env. var. for grid name */
                              char *name_txt,  /*env. var for output poly file in arcGIS text format*/
                              BoundingBox * bbox,       /* bounding box of interest - ignore if NULL */
                              MapProjInfo * mpinfo,     /* the map projection for the grid */
                              bool useBBoptimization)   /* when overlaying a grid, just return the
                                                           bounding box for that grid */
{
    PolyObject *poly;
    Shape *shp;
    PolyShape *ps;
    PolyShapeList *plist;
    MapProjInfo *map;
    int nObjects,isHole;
    int nv, np;
    int i,n,k;
    Vertex *v;
    int shapeCount,pointCount,lines;
    extern char *prog_name;
    char mesg[256];

    char gname[30];
    char sname[256];
    FILE *arcTxtFile;
    char   *readResult;
    char   tempString[256];
    char   *strPtr;
    char maxSeg[50];
    char cname[NAMLEN3 + 1];
    char *ellipsoid_var;
    int ctype;
    double p_alp;
    double p_bet;
    double p_gam;
    double xcent;
    double ycent;
    double xorig;
    double yorig;
    double xcell;
    double ycell;
    double x, y, projx, projy;
    double cx, cy;  /*polygon center point*/
    int ncols;
    int nrows;
    int nthik;
    int max_line_seg, vCount, polyVCount;
    double xseg, yseg;
   
    int polyid;    /*to store polyID*/

    /*define polygon point list variable*/
    point *polygon;   /*set root for polygon point list*/
    point *temp_point, *pre_point; /*temp point for polygon point list*/
    
    /*variables for bounding box*/
    double minBound[4], maxBound[4];
    
    /* the bounding box of a read polygon in projected coordinates*/
    double shape_minx, shape_miny, shape_maxx, shape_maxy;

    /* the bounding box for the whole file in projected coordinates*/
    double xmin=1E20, ymin=1E20, xmax=-1E20, ymax=-1E20;

        
    MESG("Reading an ArcGIS polygon text file for an EGrid\n");

    if(!getEnvtValue(name, gname))
    {
        sprintf(mesg, "Please ensure that %s is set to the name of the grid", name);
        ERROR(prog_name, mesg, 2);
    }
    
    if(!getEnvtValue(name_txt, sname))
    {
        sprintf(mesg, "Please ensure that %s is set to the name of the arcGIS shape text file.",name_txt);
        ERROR(prog_name, mesg, 2);
    }
    
    max_line_seg = 0;           /* indicates not being used */

    if(!getEnvtValue(ENVT_MAX_LINE_SEG, maxSeg))
    {
        MESG("MAX_LINE_SEG not set, discretization intervals disabled");
    }

    if(maxSeg != NULL)
    {
        max_line_seg = atoi(maxSeg);
    }

    /*get projection info and egrid row and column*/
    if(!dscgridc(gname, cname, &ctype, &p_alp, &p_bet, &p_gam,
                 &xcent, &ycent, &xorig, &yorig, &xcell, &ycell,
                 &ncols, &nrows, &nthik))
    {
        sprintf(mesg, "Error in GRIDDESC file for gridname \"%s\"", gname);
        ERROR(prog_name, mesg, 2);
    }

    if (strncmp(name,"OUTPUT",6) == 0)
    {
		ellipsoid_var = ENVT_OUTPUT_ELLIPSOID;
    }
    else if (strncmp(name,"INPUT",5) == 0)
    {
		ellipsoid_var = ENVT_INPUT_FILE_ELLIPSOID;
    }
    else if (strncmp(name,"OVERLAY",7) == 0)
    {
		ellipsoid_var = ENVT_OVERLAY_ELLIPSOID;
    }
    else
    {
		sprintf(mesg,"Couldn't determine ellipsoid variable for %s",name);
		WARN(mesg);
		ellipsoid_var = NULL;
    }
    sprintf(mesg,"Ellipsoid var = %s, Grid_name=%s",ellipsoid_var,name);
    MESG(mesg);

    if (getenv("OUTPUT_POLY_MAP_PRJN") != NULL && getenv("OUTPUT_POLY_ELLIPSOID") != NULL)
    {
       map = getFullMapProjection("OUTPUT_POLY_ELLIPSOID", "OUTPUT_POLY_MAP_PRJN");
       map->gridname = (char *) strdup(gname);
       //printf("Map type = %d  PROJ=%s\n",map->ctype,getenv("OUTPUT_POLY_MAP_PRJN"));
    }
    else
    {
       map = getFullMapProjection(ellipsoid_var, name);
       map->gridname = (char *) strdup(gname);
    }

    /*read the arcGIS polygon text file for number of objects (shapes)*/
    if ((arcTxtFile = fopen(sname,"r")) == NULL) 
    {
       sprintf(mesg,"Cannot open arcGIS polygon text file %s\n",sname);
       ERROR(prog_name, mesg, 2);
    }
     
    nObjects = 0;
    lines = 0;
    while ((readResult = fgets(tempString,256,arcTxtFile)) != NULL)
    {
	lines++;    
	if (( strPtr = (char *) strstr(tempString,"end")) != NULL || ( strPtr = (char *) strstr(tempString,"END")) != NULL)
        {
           nObjects++;  /*end of the polygon*/
        }
    }
    
    /*go to the beginning of the file*/
    rewind(arcTxtFile);
    
    nObjects = nObjects - 1;  /*get rid of the last end*/
    sprintf(mesg, "The arcGIS polygon text file has: %d polygons   %d lines\n",nObjects,lines);
    MESG(mesg);
       
    poly = getNewPoly(nObjects);
    poly->name = (char *) strdup(gname);

    if(poly == NULL)
    {
        sprintf(mesg, "Unable to allocate POLY structure for EGrid");
        ERROR(prog_name, mesg, 1);
    }

    poly->nSHPType = SHPT_POLYGON;

    if(mpinfo == NULL)
    {
        poly->map = map;
    }
    else
    {
        storeProjection(map, mpinfo);
        poly->map = mpinfo;
        if (mpinfo->gridname == NULL)
        {
            mpinfo->gridname = strdup(gname);
        }
    }
 
   /*initialize bounding box
     recomputeBoundingBox is called to correct it at the end of the function
     */
   poly->bb = newBBox(xmin,ymin,xmax,ymax);


    /*set attribute and allocate memory to attribute items*/
    if(!poly->attr_hdr)
     {
       poly->attr_hdr = getNewAttrHeader();
     }

     addNewAttrHeader(poly->attr_hdr, "ID", FTInteger);
    
     poly->attr_val = (AttributeValue **) malloc(nObjects * sizeof(AttributeValue *));
     if(!poly->attr_val)
     {
            sprintf(mesg, "%s",
                    "Unable to allocate memory for polygon ID attribute");
            ERROR(prog_name, mesg, 1);
     }

  
    /* Make polygon at the front of the point list first. */
    polygon = (point *)malloc(sizeof(point));
    /* Check for malloc failure. */
    if (polygon == NULL)  /* malloc failed */
    {
       sprintf(mesg,"Error: malloc() failed in allocating memory to polygon point list.\n");
       ERROR(prog_name, mesg, 2);
    }
    /*initialize the polygon point list*/
    polygon->x = 0.0;
    polygon->y = 0.0;
    polygon->next = NULL;    
       
    np = 1;                     /* num parts of shape */    
    shapeCount = 0;
    lines = 0;
    while((readResult = fgets(tempString,256,arcTxtFile)) != NULL)
    {
	lines++;    	

	/*check the end of the file*/    
        if (( strPtr = (char *) strstr(tempString,"end")) != NULL || ( strPtr = (char *) strstr(tempString,"END")) != NULL)
        {
          break;  /*end of the polygon text file reading loop*/
        }
       
	sscanf(tempString,"%d,lf%,lf%",&polyid,&cx,&cy);	
	
  	/*store polyid into poly object*/
	if((poly->attr_val[shapeCount] =
                    (AttributeValue *) malloc(1 * sizeof(AttributeValue))) == NULL)
          {
                    sprintf(mesg, "%s", "Attribute allocation error");
                    ERROR(prog_name, mesg, 1);
          }
        /*sprintf(mesg,"polyID=%d   i=%d",polyid,shapeCount);
        MESG(mesg);*/
        poly->attr_val[shapeCount][0].ival = polyid;
	
	/*read in points of a polygon into the polygon list*/
        nv = 0;
	pre_point = polygon;
        while((readResult = fgets(tempString,256,arcTxtFile)) != NULL)
        {
	  lines++;	
          /*sprintf(mesg, "polyline: %s\n",readResult);
          MESG(mesg);*/

          if (( strPtr = (char *) strstr(tempString,"end")) != NULL || ( strPtr = (char *) strstr(tempString,"END")) != NULL)
          {
               break;  /*end of the polygon point list*/
          }
           
          sscanf(tempString,"%lf,%lf",&x,&y);

          /*store point into a singly-linked polygon point list*/
          temp_point = (point *)malloc(sizeof(point));
          if (temp_point == NULL)  /* malloc failed */
          {
             sprintf(mesg,"Error: malloc() failed in allocating memory to temp_point.\n");
             ERROR(prog_name, mesg, 2);
          }   

          /* Initialize the new point. */
          temp_point->x = x;
          temp_point->y = y;
          temp_point->next = NULL;
          /* Link previous point to the new point. */
          pre_point->next = temp_point;
          /* Set the 'pre' pointer to point to the new point. */
          pre_point = temp_point;                
          nv++;
        } /*end of while loop to read the points of a polygon*/

        /*put the polygon in a shape object*/
        ps = getNewPolyShape(np);
        ps->num_contours = 0;   /* we need to reset,
                                        since gpc_add_contour increments num_contours */
        pointCount=0;
        if(max_line_seg <= 0) /* not using discretization interval */
        {
          shp = getNewShape(nv);
          pre_point = polygon->next;
          while (pre_point != NULL)
          { 
            temp_point=pre_point; 
            x = temp_point->x;
            y = temp_point->y;
            pre_point = temp_point->next;
	    free (temp_point);   /*free used list point*/
            if(mpinfo != NULL)
            {
              projectPoint(x, y, &projx, &projy);
              x = projx;
              y = projy;
            }
            shp->vertex[pointCount].x = x;
            shp->vertex[pointCount].y = y;
            pointCount++; 
          } 

        gpc_add_contour(ps, shp, NOT_A_HOLE);
        polyShapeIncl(&(poly->plist), ps, NULL);        
        }  /*end of not using discretization interval */
	shapeCount ++;
    }/*end of while to read the txt file*/
    fclose(arcTxtFile);

    /*sprintf(mesg, "The arcGIS polygon text file has: %d polygons   %d lines\n",shapeCount,lines);
    MESG(mesg);*/
    
    /*free memory allocated for lists*/
    free(polygon);
    
    recomputeBoundingBox(poly);
    printBoundingBox(poly->bb);

    /*nObjects = poly->nObjects;
    plist = poly->plist;
    sprintf(mesg,"nObjects=%d",nObjects);
    MESG(mesg);
    for(i = 0; i < nObjects; i++)
    {
      polyid = poly->attr_val[i][0].ival;
      ps = plist->ps;
      np = ps->num_contours;
        if (polyid <= 10 || polyid >=205700)
        {
        sprintf(mesg,"i=%d   ID=%d   np=%d",i,polyid,np);
        MESG(mesg);
        for(n = 0; n < np; n++)
         {
            nv = ps->contour[n].num_vertices;
            v = ps->contour[n].vertex;
            isHole = ps->hole[n];
            sprintf(mesg,"contour=%d     isHole=%d",n,isHole);
            MESG(mesg);
            for(k = 0; k < nv; k++)
            {
               sprintf(mesg,"   vertex=%d     x=%lf    y=%lf",k,v[k].x,v[k].y);
               MESG(mesg);
            }

         }
       }
      plist = plist->next;
    }*/


    recomputeBoundingBox(poly);
    printBoundingBox(poly->bb);

    return poly;
}

