/********************************************************************************
 * PolyShapeWrite.c
 *
 * File contains:
 * PolyShapeWriter
 *
 * Updated: Jan 2005 for weight function capabilities BB
 * Updated: April 2005 for "ALL" keyword support  BB
 * Updated: May 2005 to support file chunking   BB
 * Updated: June 2005 split shape_ifc.c into three files BB
 *
 * Comment from shape_ifc.c:
 * Note:  Many (most) of the functions in this module return "1" indicating
 *        success whereas elsewhere in the codeset a "0" indicates success.
 *        There is no consistency amongst return codes at all, so pay attention to
 *        how they are used by the calling function.
 *
 ********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
                                                                                        
#include "shapefil.h"
#include "mims_spatl.h"
#include "mims_evs.h"           /* added 4/4/2005 BB */
#include "projects.h"
#include "parms3.h"
#include "io.h"                 /* added 4/4/2005 BB */

/*
 * Write the geometry for a set of polygons to a shape file
 * This needs to map between our internal structure for the
 * shape polygons and that expected by the shape file writer.
 */
int polyShapeWrite(PolyObject * poly /* set of polygons */ ,
                   char *name /* name of file to write to */ )
{
    SHPHandle hSHP;
    SHPObject *psObject;
    Vertex *v;
    PolyShape *ps;
    PolyShapeList *plist;
    static int v_size;
    static int p_size;
    int i, j, k, n;
    int np, nv;
    int nShapeType;
    int m;
    int nVertices;
    int *partsStart;
    double *vx, *vy;

    if(v_size == 0)
    {
        v_size = 1024;
                                                                                        
        /*
         * allocate space for each of the vertices
         */
        vx = (double *) malloc(2 * v_size * sizeof(double));
        if(vx == NULL)
        {
            WARN("Memory allocation error in polyShapeWrite");
            return 1;
        }
        vy = vx + v_size;
    }
    if(p_size == 0)
    {
        p_size = 1024;
        partsStart = (int *) malloc(p_size * sizeof(int));
        if(partsStart == NULL)
        {
            WARN("Memory allocation error in polyShapeWrite");
            return 1;
        }
    }
    nShapeType = poly->nSHPType;
    hSHP = SHPCreate(name, nShapeType);
                                                                                        
    if(hSHP == NULL)
    {
        WARN2("Unable to create ", name);
        return 1;
    }
    /*
     * make sure space is available for all vertices - if it gets full,
     * reallocate with double the size
     */
    n = poly->nObjects;
    plist = poly->plist;
    /*
     * loop over all polygons
     */
    for(i = 0; i < n; i++)
    {
        ps = plist->ps;
        /* debug code BDB */
        /*if(PolyArea(ps) < 0)
               printf("In polyShapeWrite area is negative for shape %d\n", i);
         */
        np = ps->num_contours;
        nVertices = 0;
        while(np > p_size)
        {
            p_size *= 2;
            partsStart = (int *) realloc(partsStart, 2 * p_size * sizeof(int));
            if(partsStart == NULL)
            {
                WARN("Memory allocation error in polyShapeWrite");
                return 1;
            }
        }
        partsStart[0] = 0;
        /*
         * population of parts field for ShapeFiles
         */
        for(j = 0; j < np; j++)
        {
            nv = ps->contour[j].num_vertices;
            nVertices += nv;
            partsStart[j + 1] = nVertices;
        }
        while(nVertices > v_size)
        {
            v_size *= 2;
                                                                                        
            vx = (double *) realloc(vx, 2 * v_size * sizeof(double));
            if(vx == NULL)
            {
                WARN("Memory allocation error in polyShapeWrite");
                return 1;
            }
            vy = vx + v_size;
        }
        /*
         * population of vertices field for ShapeFiles
         */
        m = 0;
        for(j = 0; j < np; j++)
        {
            nv = ps->contour[j].num_vertices;
            v = ps->contour[j].vertex;
            for(k = 0; k < nv; k++)
            {
                vx[m] = v[k].x;
                vy[m] = v[k].y;
                m++;
            }
        }
                                                                                        
        /*
         * write a polygon to the .shp file
         */
        psObject = SHPCreateObject(nShapeType, -1, np, partsStart, NULL,
                                   nVertices, vx, vy, NULL, NULL);
        SHPWriteObject(hSHP, -1, psObject);
        SHPDestroyObject(psObject);
        plist = plist->next;    /* go to next polygon */
    }                           /* end loop over polygons */
    SHPClose(hSHP);
    return 0;
}                               /* end of polyShapeWrite function */

