/****************************************************************************
 * regularGridReader.c
 *
 * Creates polygons based on a regular grid description
 *
 * Removed VariableGridReader and VariableGridReader1 on 4/8/2005
 * and put them in their own file for readability --BDB 
 *
 * 4/8/2005 BDB -- Added bounding box optimization for shape overlay purposes 
 * 5/26/2005 BDB -- Added the col and row as attr_val payload data in 
 *                   RegularGridReader so that allocation output will
 *                   include COL and ROW as the first two columns of data
 *                   in an output shapefile (used by ALLOCATE mode)
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


/* ============================================================= */
/* read a regular grid description and create corresponding polygons */
PolyObject *RegularGridReader(char *name,       /* env. var. for grid name */
                              BoundingBox * bbox,       /* bounding box of interest - ignore if NULL */
                              MapProjInfo * mpinfo,     /* the map projection for the grid */
                              bool useBBoptimization)   /* when overlaying a grid, just return the
                                                           bounding box for that grid */
{
    PolyObject *poly;
    Shape *shp;
    PolyShape *ps;
    MapProjInfo *map;
    int nObjects;
    int nv, np;
    int att, c, r, rowCount;
    extern char *prog_name;
    char mesg[256];

    char gname[30];
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
    int ncols;
    int nrows;
    int nthik;
    int max_line_seg, vCount, polyVCount;
    double xseg, yseg;

    MESG("Reading Regular Grid\n");


    if(!getEnvtValue(name, gname))
    {
        sprintf(mesg, "Please ensure that %s is set to the name of the grid", name);
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
/*
    map = getNewMap();
    if(map == NULL)
    {
        sprintf(mesg, "Unable to create new map for %s", name);
        ERROR(prog_name, mesg, 2);
    }

    map->p_alp = p_alp;
    map->p_bet = p_bet;
    map->p_gam = p_gam;
    map->xcent = xcent;
    map->ycent = ycent;
    map->xcell = xcell;
    map->ycell = ycell;
    map->xorig = xorig;
    map->yorig = yorig;
    map->ctype = ctype;
    map->ncols = ncols;
    map->nrows = nrows;
*/
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
	sprintf(mesg,"Ellipsoid var = %s",ellipsoid_var);
	MESG(mesg);
	map = getFullMapProjection(ellipsoid_var, name);
    map->gridname = (char *) strdup(gname);

    if(useBBoptimization)
    {
        nObjects = 1;           /* just the outer bounding box */
    }
    else
    {
        nObjects = nrows * ncols;
    }

    poly = getNewPoly(nObjects);
    poly->name = (char *) strdup(gname);

    if(poly == NULL)
    {
        sprintf(mesg, "Unable to allocate POLY structure for Regular Grid");
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
    }

    /* this bounding box is only correct when mpinfo is null, but 
       recomputeBoundingBox is called to correct it at the end of the function
     */
    poly->bb =
        newBBox(xorig, yorig, xorig + xcell * ncols, yorig + ycell * nrows);
    nv = 4;                     /* num vertices in shape (square grid cell) */
    np = 1;                     /* num parts of shape */

    if(useBBoptimization)
    {
        MESG("Using BB optimization\n");

        ps = getNewPolyShape(np);
        ps->num_contours = 0;   /* we need to reset,
                                   since gpc_add_contour increments num_contours */
        shp = getNewShape(nv);
        shp->vertex[0].x = xorig;       /* lower left corner */
        shp->vertex[0].y = yorig;
        shp->vertex[1].x = xorig;       /* upper left corner */
        shp->vertex[1].y = yorig + ycell * nrows;
        shp->vertex[2].x = xorig + xcell * ncols;       /* upper right corner */
        shp->vertex[2].y = yorig + ycell * nrows;
        shp->vertex[3].x = xorig + xcell * ncols;       /* lower right corner */
        shp->vertex[3].y = yorig;


        gpc_add_contour(ps, shp, NOT_A_HOLE);
        polyShapeIncl(&(poly->plist), ps, NULL);
    }
    else
    {
        MESG("Not using BB optimization\n");

        if(!poly->attr_hdr)
        {
            poly->attr_hdr = getNewAttrHeader();
        }

        addNewAttrHeader(poly->attr_hdr, "COL", FTInteger);
        addNewAttrHeader(poly->attr_hdr, "ROW", FTInteger);
        /* printf("adding new header entries poly->attr_hdr->num_attr=%d\n", 
           poly->attr_hdr->num_attr);
         */

        poly->attr_val =
            (AttributeValue **) malloc(nrows * ncols *
                                       sizeof(AttributeValue *));
        if(!poly->attr_val)
        {
            sprintf(mesg, "%s",
                    "Unable to allocate memory for regular grid row/col attributes");
            ERROR(prog_name, mesg, 1);
        }


        for(r = 0; r < nrows * ncols; r++)
        {
            if((poly->attr_val[r] =
                    (AttributeValue *) malloc(2 * sizeof(AttributeValue))) == NULL)
			{
                    sprintf(mesg, "%s", "Attribute allocation error 2");
                    ERROR(prog_name, mesg, 2);
            }
			
			for(att = 0; att < 2; att++)
            {
                poly->attr_val[r][att].str = NULL;
            }

        }


        rowCount = 0;

        for(r = 0; r < nrows; r++)
        {
            for(c = 0; c < ncols; c++)
            {
                ps = getNewPolyShape(np);
                ps->num_contours = 0;   /* we need to reset,
                                        since gpc_add_contour increments num_contours */

                if(max_line_seg <= 0) /* not using discretization interval */
                {

                    shp = getNewShape(nv);

                    if(mpinfo != NULL)
                    {
                        x = xorig + xcell * c;
                        y = yorig + ycell * r;
                        projectPoint(x, y, &projx, &projy);
                        shp->vertex[0].x = projx;
                        shp->vertex[0].y = projy;

                        x = xorig + xcell * c;
                        y = (yorig + ycell * r) + ycell;
                        projectPoint(x, y, &projx, &projy);
                        shp->vertex[1].x = projx;
                        shp->vertex[1].y = projy;
                        
                        x = (xorig + xcell * c) + xcell;
                        y = (yorig + ycell * r) + ycell;
                        projectPoint(x, y, &projx, &projy);
                        shp->vertex[2].x = projx;
                        shp->vertex[2].y = projy;
                        
                        x = (xorig + xcell * c) + xcell;
                        y = yorig + ycell * r;
                        projectPoint(x, y, &projx, &projy);
                        shp->vertex[3].x = projx;
                        shp->vertex[3].y = projy;
                   }
                    else
                    {
                        shp->vertex[0].x = xorig + xcell * c;   /* lower left corner */
                        shp->vertex[0].y = yorig + ycell * r;
                        shp->vertex[1].x = shp->vertex[0].x;    /* upper left corner */
                        shp->vertex[1].y = shp->vertex[0].y + ycell;
                        shp->vertex[2].x = shp->vertex[1].x + xcell;    /*upper right corner */
                        shp->vertex[2].y = shp->vertex[1].y;
                        shp->vertex[3].x = shp->vertex[2].x;    /* lower right corner */
                        shp->vertex[3].y = shp->vertex[0].y;
                    }
                }
                else /* using discretization interval */
                {
                    if(xcell > max_line_seg)
                    {
                        xseg = abs(ceil(xcell / max_line_seg));
                    }
                    else
                    {
                        xseg = 1.0; 
                    }

                    if(ycell > max_line_seg)
                    {

                        yseg = abs(ceil(ycell / max_line_seg));
                    }
                    else
                    {
                        yseg = 1.0;
                    }


                    /*if(xseg == 1.0 && yseg == 1.0)
                    {
                        nv = 4;
                    }*/
                    /*else
                    {*/
                    nv = (int) (2 * xseg + 2 * yseg);
                    /*}*/
                    polyVCount = 0;                        

                    shp = getNewShape(nv);

                    /* left side */
                    for(vCount = 0; vCount <= yseg; vCount++)
                    {
                        if(vCount * max_line_seg < ycell)
                        {
                             y = yorig + ycell * r + vCount * max_line_seg;
                        }
                        else
                        {
                             y = (yorig + ycell * r) + ycell;
                        }

                        x = xorig + xcell * c;   

                        projectPoint(x, y, &projx, &projy);
                        shp->vertex[polyVCount].x = projx;
                        shp->vertex[polyVCount].y = projy;
                        /*printf("x=%f ", shp->vertex[polyVCount].x);
                        printf("y=%f\n", shp->vertex[polyVCount].y);
                        */
                        polyVCount++;

                    }

                    /* top */
                    for(vCount = 1; vCount <= xseg; vCount++)
                    {

                        if(vCount * max_line_seg < xcell)
                        {
                             x = xorig + xcell * c + vCount * max_line_seg;
                        }
                        else
                        {
                             x = (xorig + xcell * c) + xcell;
                        }
                        y = yorig + ycell * r + ycell;   

                        projectPoint(x, y, &projx, &projy);
                        shp->vertex[polyVCount].x = projx;
                        shp->vertex[polyVCount].y = projy;
                        /*printf("x=%f ", shp->vertex[polyVCount].x);
                        printf("y=%f\n", shp->vertex[polyVCount].y);
                        */
                        polyVCount++;
                    }

                    /* right side */
                    for(vCount = 1; vCount <= yseg; vCount++)
                    {
                        if(vCount * max_line_seg <= ycell)
                        {
                             y = yorig + (ycell * r) + ycell - (vCount * max_line_seg);
                        }
                        else
                        {
                             y = yorig + ycell * r; 
                        }
                        x = xorig + xcell * c + xcell;   
                        projectPoint(x, y, &projx, &projy);
                        shp->vertex[polyVCount].x = projx;
                        shp->vertex[polyVCount].y = projy;
                        /*printf("x=%f ", shp->vertex[polyVCount].x);
                        printf("y=%f\n", shp->vertex[polyVCount].y);
                        */
                        polyVCount++;
                    }


                    /* bottom */
                    for(vCount = 1; vCount <= xseg - 1; vCount++)
                    {

                        if(vCount * max_line_seg < xcell)
                        {
                             x = xorig + (xcell * c) + xcell - (vCount * max_line_seg);
                        }
                        else
                        {
                             x = xorig + (xcell * c) + xcell;
                        }
                        y = yorig + ycell * r;
                        projectPoint(x, y, &projx, &projy);
                        shp->vertex[polyVCount].x = projx;
                        shp->vertex[polyVCount].y = projy;
                        /*printf("x=%f ", shp->vertex[polyVCount].x);
                        printf("y=%f\n", shp->vertex[polyVCount].y);
                        */
                        polyVCount++;
                    }

                }

                gpc_add_contour(ps, shp, NOT_A_HOLE);
                polyShapeIncl(&(poly->plist), ps, NULL);

                /*if(PolyArea(ps) < 0)
                   {
                   printf("Area of grid cell < 0\n");
                   } */

                poly->attr_val[rowCount][0].ival = c + 1;
                poly->attr_val[rowCount][1].ival = r + 1;

                rowCount++;

            }
        }

    }
    recomputeBoundingBox(poly);

    return poly;

    /*error:
       WARN(mesg);
     */
    /*return NULL; */ /* we should never get here */
}

