/* ============================================================= */
/* read a Fractional Vegetation data grid and create corresponding polygons */
/* projection is a Lat-Lon */
/*    unit = dregees                         */
/*    Resolution = 0.144 degrees             */
/*    longitude range = -180.0 to 179.856    */
/*    latitude range = -89.928 to 89.928     */

/* Bounding coordinates:                     */
/*    Read from environment variable DOMAIN_RANGE  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "shapefil.h"
#include "mims_spatl.h"
#include "parms3.h"


PolyObject *FractionalVegetationReader(char *name,      /* the name of the file to read */
                                       MapProjInfo * map,       /* the map projection of the file to read */
                                       BoundingBox * bbox,      /* bounding box of interest - ignore if NULL */
                                       MapProjInfo *
                                       bbox_mproj
                                       /* the map projection of the bounding box */
                                       )
{
    FILE *fp;
    int buffer[5001];           /* latitude values from -180 to 179.856 degrees */

    PolyObject *poly;
    Shape *shp;
    PolyShape *ps;
    int nObjects;
    int nv, np;
    int c, r, recno, n, ichr, i, j, ival;
    int icol, irow;
    int ncnt, nmax;
    char **list;
    char mesg[256];

 /**  projection parameters **/
    char *gname;
    char cname[NAMLEN3 + 1];
    int ctype = LATGRD3;
    double p_alp = 0.0;
    double p_bet = 0.0;
    double p_gam = 0.0;
    double xcent = 0.0;
    double ycent = 0.0;
    double xorig = 0.0;
    double yorig = 0.0;
    double xcell = 0.144;
    double ycell = 0.144;
    int ncols = 0;
    int nrows = 0;
    int nthik = 1;

 /** range of domain **/
    double lon_min = -128.0;
    double lon_max = -65.0;
    double lat_min = 23.0;
    double lat_max = 51.0;

    /* fill the map projection structure - doing it this way won't allow for a 
     * custom map projection */
    map = getNewMap();
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
    map->gridname = "Fractional Vegetation";
    map->ncols = ncols;
    map->nrows = nrows;

 /** get range values from environment variable DOMAIN_RANGE **/
    if(strlistc("DOMAIN_RANGE", "Comma separated list of range values",
                &nmax, &ncnt, &list))
    {
        if(ncnt == 4)
        {
            lon_min = (double) atof(list[0]);
            lon_max = (double) atof(list[1]);
            lat_min = (double) atof(list[2]);
            lat_max = (double) atof(list[3]);
        }
        else
        {
            printf("DOMAIN_RANGE not defined, using default\n");
        }
    }

 /** check domain limits  **/
    if(lon_min < -180.0 || lon_max > 180.0 || lat_min < -89.928
       || lat_max > 179.856)
    {
        printf("Invalid DOMAIN_RANGE values, using default\n");
        lon_min = -128.0;
        lon_max = -65.0;
        lat_min = 23.0;
        lat_max = 51.0;
    }


 /** round off minimum range values to cell **/
    icol = (lon_min + 180.0) / xcell;
    lon_min = -180.0 + icol * xcell;

    irow = (lat_min + 89.928) / ycell;
    lat_min = -89.928 + irow * ycell;

 /** compute number of rows and columns  **/
    ncols = (int) ((lon_max - lon_min) / xcell) + 1;
    nrows = (int) ((lat_max - lat_min) / ycell) + 1;

    if(getenv(name) == NULL)
    {
        sprintf(mesg, "Env. var. \"%s\" not set", name);
        goto error;
    }
    gname = (char *) strdup(getenv(name));

    nObjects = nrows * ncols;

    poly = getNewPoly(nObjects);
    if(poly == NULL)
    {
        sprintf(mesg, "Cannot allocate POLY struct");
        goto error;
    }
    poly->nSHPType = SHPT_POLYGON;
    poly->map = map;
    poly->name = (char *) strdup(gname);

    poly->bb =
        newBBox(lon_min, lat_min, lon_min + xcell * ncols,
                lat_min + ycell * nrows);
    nv = 4;
    np = 1;

    for(r = irow; r < irow + nrows; r++)
    {
        for(c = icol; c < icol + ncols; c++)
        {
            ps = getNewPolyShape(np);
            ps->num_contours = 0;       /* we need to reset,
                                           since gpc_add_contour increments num_contours */

            shp = getNewShape(nv);
            shp->vertex[0].x = -180 + c * xcell;
            shp->vertex[0].y = -89.928 + r * ycell;
            shp->vertex[1].x = shp->vertex[0].x + xcell;
            shp->vertex[1].y = shp->vertex[0].y;
            shp->vertex[2].x = shp->vertex[1].x;
            shp->vertex[2].y = shp->vertex[1].y + ycell;
            shp->vertex[3].x = shp->vertex[0].x;
            shp->vertex[3].y = shp->vertex[2].y;

            gpc_add_contour(ps, shp, SOLID_POLYGON);
            polyShapeIncl(&(poly->plist), ps, NULL);
        }
    }

  /** add attributes from file **/

    /*  open grid file */
    if((fp = fopen(gname, "r")) == NULL)
    {
        sprintf(mesg, "Cannot open grid file %s", poly->name);
        goto error;
    }

    /***  create Attribute ***/
    if(!poly->attr_hdr)
    {
        poly->attr_hdr = getNewAttrHeader();
    }
    n = addNewAttrHeader(poly->attr_hdr, "VEGFRAC", FTInteger);

    if(!poly->attr_val)
    {
        poly->attr_hdr->attr_desc[0]->category = 10;
        poly->attr_val =
            (AttributeValue **) malloc(nObjects * sizeof(AttributeValue *));

        for(recno = 0; recno < nObjects; recno++)
        {
            if((poly->attr_val[recno] =
                (AttributeValue *) malloc(n * sizeof(AttributeValue))) == NULL)
            {
                sprintf(mesg, "%s", "Attribute allocation error");
                goto error;
            }

            for(j = 0; j < n; j++)
            {
                poly->attr_val[recno][j].str = NULL;
            }
        }
    }


    /** set attribute values **/
    recno = 0;
    for(r = 0; r < 1250; r++)
    {
       /** read row of data **/
        for(i = 0; i < 5001; i++)
        {
            buffer[i] = fgetc(fp);
        }

        if(r >= irow && r < irow + nrows)
        {
            for(c = icol; c < icol + ncols; c++)
            {
                ival = 0;
                if(buffer[2 * c] > 48 && buffer[2 * c] <= 57)
                    ival = 10 * (buffer[2 * c] - 48);
                if(buffer[2 * c + 1] > 48 && buffer[2 * c + 1] <= 57)
                    ival += (buffer[2 * c + 1] - 48);
                if(ival == 0)
                    ival = -1;
                poly->attr_val[recno][0].ival = ival;
                recno++;
                if(recno > nObjects)
                    printf("number of objects exceed max %d \n", recno);
            }
        }
    }
    if(recno != nObjects)
        printf("number of objects != attributes read\n");

    /*  close grid file */
    fclose(fp);

    return poly;

  error:
    WARN(mesg);
    return NULL;                /* we should never get here */
}
