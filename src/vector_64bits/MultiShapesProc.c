/********************************************************************************
 * PolyMShapesInOne.c
 *
 * File contains:
 * PolyMShapesInOne
 *
 * Created: Nov 2005 process PolyObject to have all shapes with the same ID into one record, LR
 *
 * Note from old files:  Many (most) of the functions in this module 
 *        return "1" indicating success whereas elsewhere in the codeset 
 *        a "0" indicates success. There is no consistency amongst return codes 
 *        at all, so pay attention to how they are used by the calling function.
 *
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
                                                                                    
#include "shapefil.h"
#include "mims_spatl.h"
#include "mims_evs.h"           
#include "projects.h"
#include "parms3.h"
#include "io.h"                 

                                                                              
/*
 * read through polygons and attributes from PolyObject.
 * put all shapes with the same ID into one shape record.*/
 
PolyObject *PolyMShapesInOne(PolyObject * poly)
{
    extern char *prog_name;
                                                                            
    PolyShapeList *plist;
    SHPObject *psShape;
    Shape *shp;
    PolyShape *ps;    
    PolyShape **psList; 
    
    char PolyID[80];
    char **idList;
    int outIDType;

    
    
    
    BoundingBox *polyBB;       /* bounding box for a shape */
    int i, j, k, n, z, extraVerts, numNewPoints;
    int nvtx, nv, np;           /* nv = num vertices, np = num parts of
                                 * shape */
    int hi, lo;
    int loopControl;
    int skipped_verts = 0;
    int skipped_polys = 0;
    int isHole;                 /* 0 for nonhole, 1 for hole */
    int includedShape;          /* 0 if didn't include, 1 if did
                                 * include */
    double minBound[4], maxBound[4];
    double nextPointx, nextPointy, length, numSegs, deltax, deltay;
    double x, y, x2, y2;                /* the x & y of a vertex */
    double projx, projy, lastProjx, lastProjy; /* the x & y of a vertex projected to
                                 * output coords */
    
                                                                                    
    /* the bounding box of a read polygon in projected coordinates */
    double shape_minx, shape_miny, shape_maxx, shape_maxy;
                                                                                    
    /*the bounding box for the whole file in projected coordinates*/
    double full_minx, full_miny, full_maxx, full_maxy;
                                                                                    
    /*the bounding box of all shapes that intersect the limiting_bbox, in projected coordinates*/
    double intersect_minx, intersect_miny, intersect_maxx, intersect_maxy;
                                                                                    
    double areatot = 0;
    double shparea;
                                                                                    
    char pname[256];
    char mesg[256], maxSeg[50];
    char *earth_ellipsoid;
    int retVal;

    int max_line_seg = 0;


    if(poly == NULL){
       MESG("PolyObject is empty in PolyMShapesInOne program\n");  
       return poly;
    }

    n = poly->nObjects;

    plist = poly->plist;

    for(i = 0; i < n; i++)
    { 
        ps = getNewPolyShape(np);
        ps->num_contours = 0;   /* we need to reset, since gpc_add_contour
                                 * increments num_contours */
        if(nShapeType == SHPT_POINT)
        {
            nv = nvtx;
            shp = getNewShape(nv);
            for(j = 0; j < nv; j++)
            {
                /*
                 * we'd like to throw out points that don't overlap our
                 * output region, but then we need to know how to remove
                 * the corresponding attributes
                 */
                                                                                    
                /* write these coords to a file x,y, projx, projy */
                /* need to add file management outside of loop to
                   support this feature
                   will be controlled by an EV
                 */
                                                                                    
                /* raw/input proj */
                x = psShape->padfX[j];
                y = psShape->padfY[j];
                projectPoint(x, y, &projx, &projy);
                /* output proj */
                shp->vertex[j].x = projx;
                shp->vertex[j].y = projy;
                /*
                 * right now, retain all points in the file even if
                 * they're not in the requested bbox
                 */
                                                                                    
                /*
                 * update the file-wide bounding box - don't compute a
                 * shape bb for point data
                 */
                intersect_minx = full_minx = MIN(full_minx, projx);
                intersect_maxx = full_maxx = MAX(full_maxx, projx);
                intersect_miny = full_miny = MIN(full_miny, projy);
                intersect_maxy = full_maxy = MAX(full_maxy, projy);
            }
                                                                                    
            gpc_add_contour(ps, shp, NOT_A_HOLE);
                                                                                    
        }                       /* end if the shape is a point */
        else
        {                       /* the shape is a line or polygon */
            /*
             * for each part of the shape
             */
	       ps = getNewPolyShape(np);
        ps->num_contours = 0;   /* we need to reset, since gpc_add_contour
                                 * increments num_contours */
            for(k = 0; k < np; k++)
            {
                lo = psShape->panPartStart[k];
                if(k + 1 != np)
                {
                    hi = psShape->panPartStart[k + 1];
                }
                else
                {
                    hi = nvtx;
                }
                nv = hi - lo;
                                                                                    
                shp = getNewShape(nv);
 
                
                if(max_line_seg <= 0)
                {
                        /** AME: Adjusted this to project 
                            to output projection here */
                    for(j = 0; j < nv; j++)
                    {
                        x = psShape->padfX[j + lo];
                        y = psShape->padfY[j + lo];
                        projectPoint(x, y, &projx, &projy);
                        shp->vertex[j].x = projx;
                        shp->vertex[j].y = projy;
                        /*
                         * update the shape-wide bounding box
                         */
                        shape_minx = MIN(shape_minx, projx);
                        shape_maxx = MAX(shape_maxx, projx);
                        shape_miny = MIN(shape_miny, projy);
                        shape_maxy = MAX(shape_maxy, projy);
                    }   /* j: for each vertex in the shape part */

                    /*
                     * set bounding box for current shape through current part
                     */
                    fillBBox(shapeBB, shape_minx, shape_miny, shape_maxx,
                         shape_maxy);
                }
                else
                {
                      /* Here, max_line_seg is set, so we must calculate
                         line lengths and bisect any that are longer
                         than the maximum allowable segment length
                       */
                        extraVerts = 0;

                        for(j = 0; j < nv; j++)
                        {
                            x = psShape->padfX[j + lo];
                            y = psShape->padfY[j + lo];
                            projectPoint(x, y, &projx, &projy);

                            if(j > 0)
                            {
                                lastProjx = shp->vertex[extraVerts - 1].x;
                                lastProjy = shp->vertex[extraVerts - 1].y;

                                length = sqrt( pow((projx - lastProjx), 2) + 
                                              pow((projy - lastProjy), 2) );
                                
                                
                                if(length >= max_line_seg)
                                {
                                     numSegs = ceil(length / max_line_seg);
                                     numNewPoints = (int) numSegs - 1; 

                                     deltax = (projx - lastProjx) / numSegs;
                                     deltay = (projy - lastProjy) / numSegs;

                                     addNewVertices(shp, numNewPoints);
                                     nextPointx = lastProjx;
                                     nextPointy = lastProjy;
                                     for(z = 0; z < numNewPoints; z++)
                                     {
                                         nextPointx += deltax;
                                         nextPointy += deltay;
                                         shp->vertex[extraVerts].x = nextPointx;
                                         shp->vertex[extraVerts].y = nextPointy;
                                         extraVerts++;
                                     }
                                }
                                 
                                      

                            }
                            shp->vertex[extraVerts].x = projx;
                            shp->vertex[extraVerts].y = projy;
                            extraVerts++;
                            /*
                             * update the shape-wide bounding box
                             */
                            shape_minx = MIN(shape_minx, projx);
                            shape_maxx = MAX(shape_maxx, projx);
                            shape_miny = MIN(shape_miny, projy);
                            shape_maxy = MAX(shape_maxy, projy);
                        }   /* j: for each vertex in the shape part */


                        /*
                         * set bounding box for current shape through current part
                         */
                        fillBBox(shapeBB, shape_minx, shape_miny, shape_maxx,
                             shape_maxy);

                }
                                                                                    
                                                                                    
                /*
                 * if there is no overlap between this part of the shape
                 * and the bounding box, replace it with a shape w/
                 * 1 vertex
                 */
                isHole = NOT_A_HOLE;
                includedShape = 1;
                if(limiting_bbox != NULL)
                {
                                                                                    
                    if(!OVERLAP2(shapeBB, limiting_bbox))
                    {           /* if there is no overlap */
                        skipped_verts += nv;
                        skipped_polys++;
                        freeShape(shp);
                        /*
                         * create a dummy shape with only 1 vertex to put
                         * in structure
                         */
                        shp = getNewShape(1);
                        shp->vertex[0].x = projx;
                        shp->vertex[0].y = projy;
                        includedShape = 0;
                    }
                }
                if(includedShape && (nShapeType == SHPT_POLYGON))
                {
                    /*
                     * if this is a polygon we care about, determine if
                     * it's a hole
                     */
                                                                                    
                    shparea = Area(shp);
                    areatot += shparea;
                    if(shparea < 0)
                    {
                        isHole = IS_A_HOLE;
                        /*printf("area is a hole\n");*/
                    }
                }
                /*
                 * if we don't add the object at all, then we need to understand
                 * what happens when the attributes are associated with
                 * the file - perhaps we add a dummy empty shape to the structure
                 */
                                                                                    
                gpc_add_contour(ps, shp, isHole);
                                                                                    
            }                   /* end k: for each part of shape */
                                                                                    
            /* debug code BDB */
            /*if(PolyArea(ps) < 0)
              printf("Area of shape is negative\n");
             */
                                                                                    
            /*
             * In order to correctly evaluate whether this object overlaps
             * the limiting_bbox of interest, we need to be sure that both
             * bboxes have the same map projection */
            /* at this point, the bounding box is for the whole shape, not part
             */
            fillBBox(shapeBB, shape_minx, shape_miny, shape_maxx, shape_maxy);
                                                                                    
            /*
             * if the shape is within the specified bbox, update the
             * file-wide bounding box
             */
            if(limiting_bbox != NULL)
            {
                if(OVERLAP2(shapeBB, limiting_bbox))
                {
                                                                                    
                    intersect_minx = MIN(shape_minx, intersect_minx);
                    intersect_maxx = MAX(shape_maxx, intersect_maxx);
                    intersect_miny = MIN(shape_miny, intersect_miny);
                    intersect_maxy = MAX(shape_maxy, intersect_maxy);
                }
            }
            /*
             * update the full bounding box regardless of overlap
             */
            full_minx = MIN(shape_minx, full_minx);
            full_maxx = MAX(shape_maxx, full_maxx);
            full_miny = MIN(shape_miny, full_miny);
            full_maxy = MAX(shape_maxy, full_maxy);
                                                                                    
        }                       /* end if shape is a point, else it's a polygon */
                                                                                    
                                                                                    
        polyShapeIncl(&(poly->plist), ps, NULL);
                                                                                    
        SHPDestroyObject(psShape);
                                                                                    
        i++;
    } /* end while i: scanning each shape in file */
                                                                                    
    if(!strcmp(polyName, name))
    {
        window++;
        lastObjectRead = i;
                                                                                    
        if(lastObjectRead >= nObjects - 1)
        {
            fileCompleted = 1; /* the whole file is done */
        }
    }
                                                                                    
    recomputeBoundingBox(poly);
    /*
     * set the poly bounding box to the bbox for the intersecting shapes
     */
    if(limiting_bbox != NULL)
    {
        poly->bb =
            newBBox(intersect_minx, intersect_miny, intersect_maxx,
                    intersect_maxy);
    }
    projBB = newBBox(full_minx, full_miny, full_maxx, full_maxy);
                                                                                    
    sprintf(mesg, "Total area = %0.4e\n", areatot);
    MESG(mesg);
    sprintf(mesg, "Skipped %d polygons and %d vertices\n",
            skipped_polys, skipped_verts);
    MESG(mesg);
                                                                                    
    /*
     * check to see if bounding box of whole file overlaps the input bounding box
     */
    MESG("File bounding box: ");
    printBoundingBox(projBB);
    if(limiting_bbox != NULL)
    {
        /*
         * MESG("Limiting bounding box: ");
         * printBoundingBox(limiting_bbox);
         */
        if(!OVERLAP2(poly->bb, limiting_bbox))
        {
            /*
             * there is no overlap, so don't process the file
             */
            MESG2
                ("There is no overlap between the output grid/polygon and shapefile ",
                 pname);
            /*
             * TBD: may need to free all data in poly
             */
            return NULL;
        }
        /*
         * else { MESG2("Bounding box for shapes intersecting limiting
         * bbox ",pname); printBoundingBox(poly->bb); }
         */
    }
    MESG("");

    SHPClose(hSHP);
    return poly;
}

