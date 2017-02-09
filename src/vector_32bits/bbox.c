/**
 * File contains:
 * newBBox
 * newBoundingBox
 * computeBoundingBox
 * recomputeBoundingBox
 * printBoundingBox
 * copyBBoxToFrom
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "shapefil.h"
#include "mims_spatl.h"

static char mesg[256];

/* ============================================================= */
/* allocate a new bounding box that uses the specified coordinates */
BoundingBox *newBBox(double xmin, double ymin, double xmax, double ymax)
{
    BoundingBox *bb;

    bb = (BoundingBox *) malloc(sizeof(BoundingBox));
    if(bb)
    {
        bb->xmin = xmin;
        bb->ymin = ymin;
        bb->xmax = xmax;
        bb->ymax = ymax;
    }
    return bb;
}

/* ============================================================= */
/* allocate a new bounding box that uses the specified coordinates */
int fillBBox(BoundingBox * bb, double xmin, double ymin, double xmax,
             double ymax)
{
    if(bb)
    {
        bb->xmin = xmin;
        bb->ymin = ymin;
        bb->xmax = xmax;
        bb->ymax = ymax;
        return 0;
    }
    return 1;
}


/* ============================================================= */
/* compute the bounding box for the shape ps and return it
 * in the double * arguments for xmin,ymin,xmax,ymax  */
int computeBoundingBox(PolyShape * ps,
                       double *xmin, double *ymin, double *xmax, double *ymax)
{
    Vertex *v;
    int np, nv;
    int j, k;

    if(ps == NULL)
        return 0;
    np = ps->num_contours;
    /* initialize min & max */
    /*if (np > 0)
       {
       *xmin = 1E20;
       *xmax = -1E20;
       *ymin = 1E20;
       *ymax = -1E20;
       } */
    for(j = 0; j < np; j++)
    {
        nv = ps->contour[j].num_vertices;
        v = ps->contour[j].vertex;
        for(k = 0; k < nv; k++)
        {
            if((j == 0) && (k == 0))
            {
                *xmin = v[k].x;
                *xmax = v[k].x;
                *ymin = v[k].y;
                *ymax = v[k].y;
            }

            *xmin = MIN(*xmin, v[k].x);
            *xmax = MAX(*xmax, v[k].x);
            *ymin = MIN(*ymin, v[k].y);
            *ymax = MAX(*ymax, v[k].y);
        }
    }
    return 1;
}


/* ============================================================= */
/* Create a new bounding box for a shape */
BoundingBox *newBoundingBox(PolyShape * ps)
{
    double xmin, ymin, xmax, ymax;
    BoundingBox *bb;

    computeBoundingBox(ps, &xmin, &ymin, &xmax, &ymax);
    bb = newBBox(xmin, ymin, xmax, ymax);
    return bb;

}


/* ============================================================= */
/* recompute a bounding box for a polygon */
int recomputeBoundingBox(PolyObject * p)
{
    PolyShapeList *plist;
    BoundingBox *bb;
    double xmin, ymin, xmax, ymax;
    double x1, y1, x2, y2;
    int i, n;

    if(p == NULL)
        return 0;

    n = p->nObjects;

    plist = p->plist;

    for(i = 0; i < n; i++)
    {
        /* compute the bounding box for this sub-object */
        if(plist == NULL)
        {
            fprintf(stdout,"recomputeBoundingBox: i=%d\n",i);
            ERROR("allocator","plist NULL in recomputeBoundingBox",2);
        }
        
        if(plist->bb == NULL)
        {
            fprintf(stderr,"plist->bb was null\n");
            plist->bb = newBBox(0, 0, 0, 0);
        }
        bb = plist->bb;
        
        computeBoundingBox(plist->ps, &xmin, &ymin, &xmax, &ymax);

        bb->xmin = xmin;
        bb->ymin = ymin;
        bb->xmax = xmax;
        bb->ymax = ymax;

        /* compute the overall bounding box for the whole polygon */
        if(i == 0)
        {
            x1 = xmin;
            x2 = xmax;
            y1 = ymin;
            y2 = ymax;
        }
        else
        {
            x1 = MIN(x1, xmin);
            x2 = MAX(x2, xmax);
            y1 = MIN(y1, ymin);
            y2 = MAX(y2, ymax);
        }
        plist = plist->next;
    }

    /* set the bounding box params for the whole polygon */
    bb = p->bb;
    bb->xmin = x1;
    bb->ymin = y1;
    bb->xmax = x2;
    bb->ymax = y2;
    return 1;
}

/* print a bounding box */
int printBoundingBox(BoundingBox * bbox)
{

    if(bbox == NULL)
    {
        MESG("NULL bounding box\n");
        return 1;
    }
    else
    {
        sprintf(mesg, "xmin = %.3f xmax = %.3f ymin = %.3f ymax = %.3f\n",
                bbox->xmin, bbox->xmax, bbox->ymin, bbox->ymax);
        MESG(mesg);
    }
    return 0;
}

/* get the location of the center of the shape as computed as
 * the center of the bounding box; holes are treated the
 * same was as regular shapes */
void computeCenterOfBox(PolyShape * ps, double *xcent, double *ycent)
{
    double xmin, xmax;
    double ymin, ymax;

    computeBoundingBox(ps, &xmin, &ymin, &xmax, &ymax);
    *xcent = (xmin + xmax) / 2;
    *ycent = (ymin + ymax) / 2;
}

/* Copy the contents of bb_in to bb_out */
int copyBBoxToFrom(BoundingBox * bb_out, BoundingBox * bb_in)
{
    if((bb_out == NULL) || (bb_in == NULL))
        return 1;
    bb_out->xmin = bb_in->xmin;
    bb_out->ymin = bb_in->ymin;
    bb_out->xmax = bb_in->xmax;
    bb_out->ymax = bb_in->ymax;
    return 0;
}
