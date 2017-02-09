/****************************************************************************
 * polyops.c
 *
 *
 * File contains:
 * addLineSegment
 * CopyPoly
 * CopyPolySelf
 * getNewShape
 * getNewPolyShape
 * getNewPoly
 * freePolyShape
 * printPoly
 * Area
 * Length
 * PolyArea
 * PolyLength
 * newParent
 * newPolyParent
 * polyShapeIncl
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "shapefil.h"
#include "mims_spatl.h"
#include "mims_evs.h"
#include "parms3.h"
#include "io.h"


/* ============================================================= */
/* Add a line segment to a polygon */
/* modified by dyang: 01/2014, to pass isHole to gpc_add_contour */
int addLineSegment(Vertex * v1, Vertex * v2,    /* vertices for segment */
                   PolyShape * p /*polygon to add to */, int isHole )
{
    Shape *shp;

    /* create a new line */
    int nv = 2;
    shp = getNewShape(nv);
    if(!shp)
    {
        WARN("Allocation error in addLineSegment");
        return 1;
    }
    shp->vertex[0].x = v1->x;
    shp->vertex[0].y = v1->y;
    shp->vertex[1].x = v2->x;
    shp->vertex[1].y = v2->y;

    /* add the line to the polygon , 
    printf( "is a hole %d  \n", isHole); */
    gpc_add_contour(p, shp, isHole);

#ifdef DEBUG
    printf("New line seg (%f,%f => %f,%f)\n", v1->x, v1->y, v2->x, v2->y);
#endif

    return 0;
}



/* ============================================================= */
/* Make a copy of a set of polygons */
int CopyPoly(PolyObject * p /* dest. poly */ ,
             PolyObject * g /* source poly */ )
{

    if(!p || !g)
        return 0;
    p->bb = newBBox(g->bb->xmin, g->bb->ymin, g->bb->xmax, g->bb->ymax);
    p->nSHPType = g->nSHPType;
    p->nObjects = g->nObjects;
    p->map = g->map;            /* should this be a deep copy instead of a shallow one? */
    if(g->name != NULL)
    {
        p->name = (char *) strdup(g->name);
    }
    p->attr_hdr = g->attr_hdr;
    p->attr_val = g->attr_val;
    p->plist = g->plist;        /* should this be a deep copy? */
    p->parent_poly1 = g->parent_poly1;
    p->parent_poly2 = g->parent_poly2;

    return 1;
}

/* ============================================================= */
/* Make a copy of a set of polygons to create intersected weight x data polys */
int CopyPolySelf(PolyObject * pw, PolyObject * pd, PolyObject * pwd)
/* pw = weight polygons, pd = data polygons, pwd = intersected weight & data polys */
{

    int i;
    int n;
    PolyParent *pp;
    PolyShape *ps;
    PolyShapeList *plist;
    Parent **p1;

    /* copy the weight polygons to the intersection polygons */
    if(!CopyPoly(pwd, pw))
        return 0;

    /* set the weightxdata polygons to point to the weight and data polygons 
     * as parents (so it can find the attributes later) */
    pwd->plist = NULL;
    pwd->parent_poly1 = pw;
    pwd->parent_poly2 = pd;
    n = pd->nObjects;

    p1 = (Parent **) malloc(n * sizeof(Parent *));

    if(!p1)
    {
        WARN("Allocation error for p1 in CopyPolySelf");
        return 0;
    }

    /* traverse the list of data polygons */
    plist = pd->plist;

    /* update the parents */
    for(i = 0; i < n; i++)
    {
        pp = plist->pp;
        ps = plist->ps;
        p1[i] = newParent(pp, ps, i);

        pp = newPolyParent(p1[i], p1[i]);

        polyShapeIncl(&(pwd->plist), ps, pp);
        plist = plist->next;
    }
    /* update the bounding box for the intersected weightxdata polygons */
    pwd->bb = newBBox(pd->bb->xmin, pd->bb->ymin, pd->bb->xmax, pd->bb->ymax);

    return 1;
}


/* ============================================================= */
/* allocate a new shape */
Shape *getNewShape(int n)
{
    Shape *p;

    p = (Shape *) malloc(sizeof(Shape));
    if(p != NULL)
    {
        p->num_vertices = n;
        p->vertex = (Vertex *) malloc(n * sizeof(Vertex));
    }

    return p;
}


/* add more vertices to a shape */
void addNewVertices(Shape *shp, int n)
{
   shp->num_vertices += n;
   shp->vertex = (Vertex *) realloc(shp->vertex, shp->num_vertices 
                                               * sizeof(Vertex));

}

/* ============================================================= */
/* release a shape from memory */
void freeShape(Shape * s)
{
    if(s != NULL)
    {
        free(s->vertex);
        free(s);
    }
    return;
}


/* ============================================================= */
/* allocate a new polygon */
PolyShape *getNewPolyShape(int n)
{
    PolyShape *p;

    p = (PolyShape *) malloc(sizeof(PolyShape));
    if(p != NULL)
    {
        p->num_contours = n;
        if(!n)
        {
            p->hole = (int *) malloc(n * sizeof(int));
            p->contour = (Shape *) malloc(n * sizeof(Shape));
        }
        else
        {
            p->hole = NULL;
            p->contour = NULL;
        }
    }

    return p;
}


/* ============================================================= */
/* allocate a new polygon */
PolyObject *getNewPoly(int n)
{
    PolyObject *p;

    p = (PolyObject *) malloc(sizeof(PolyObject));
    if(p != NULL)
    {
        p->nObjects = n;
        p->plist = NULL;
        p->attr_val = NULL;
        p->attr_hdr = NULL;
        p->parent_poly1 = NULL;
        p->parent_poly2 = NULL;
        /* AME: initialized additional fields */
        p->nSHPType = 0;
        p->bb = NULL;
        p->map = NULL;
        p->name = NULL;
    }

    return p;
}

/* ============================================================= */
/* release a polygon from memory */
/* updated 5/10/2005 to free the header and attributes BDB */
void freePolyShape(PolyShape *p)
{
    int attribCount, numRecords;


    if(p != NULL)
    {
        gpc_free_polygon(p);
    }

    /* need to free attribs & vertices */

    return;

}

/* release a polygon object and all its attributes from memory */
/* added 5/10/2005 BDB */
void freePolyObject(PolyObject *p)
{
    int attribCount, numRecords, k, m;
    PolyShapeList *pl;


    if(p != NULL)
    {
        if(p->attr_hdr != NULL)
            attribCount = p->attr_hdr->num_attr;
        else
            attribCount = 0;

        numRecords = p->nObjects;

        if(p->bb != NULL)
            free(p->bb);

        if(p->name != NULL)
            free(p->name);

        /*
        This code does not work for some reason, perhaps because
        AttributeValue is a union?  */
        for(k = 0; k < numRecords; k++)
        {
            for(m = 0; m < attribCount; m++)
            {
                if(p->attr_val[k][m].str != NULL)
                {
                    free(p->attr_val[k][m].str);
                }
            }
        }

        if(p->attr_val != NULL)
            free(p->attr_val);


        for(m = 0; m < attribCount; m++)
        {
            /*if(p->attr_hdr->attr_desc[m] != NULL)
                free(p->attr_hdr->attr_desc[m]);*/
        }

        if(p->attr_hdr != NULL)
            free(p->attr_hdr);

        if(p->plist != NULL)
        {
            pl = p->plist;
            while(pl->next != NULL)
            {
                if(pl->ps != NULL)
                    gpc_free_polygon(pl->ps);

                if(pl->bb != NULL)
                    free(pl->bb);

                pl = pl->next;
            }


            free(p->plist);
        }

        free(p);
    }

}


/* ============================================================= */

/* print out a polygon */
int printPoly(PolyObject * poly)
{
    int i, j, k, n;
    int np, nv;
    Vertex *v;
    PolyShape *ps;
    PolyShapeList *plist;

    n = poly->nObjects;
    printf("printPoly:n=%d\n", n);
    plist = poly->plist;
    for(i = 0; i < n; i++)
    {
        ps = plist->ps;
        np = ps->num_contours;
        if(np > 1)
        {
            printf("printPoly:num_contours=%d\n", np);
        }

        for(j = 0; j < np; j++)
        {
            nv = ps->contour[j].num_vertices;
            if(nv > 1)
            {
                printf("===> Part %d(%d) nV=%d\n", j + 1, np, nv);
            }
            v = ps->contour[j].vertex;
            for(k = 0; k < nv; k++)
            {
                printf("(%12.3f,%12.3f)\n", v[k].x, v[k].y);
            }
            if(nv > 1)
            {
                printf("Area=%f, Length=%f\n", Area(ps->contour + j),
                       Length(ps->contour + j));
            }
        }

        if(nv > 1)
        {
            printf("PolyArea=%f, PolyLength=%f\n",
                   PolyArea(ps), PolyLength(ps));
        }
        plist = plist->next;
    }
    return 0;
}

/* ============================================================= */
/* return the area for a shape (polygon) */
double Area(Shape * p)
{
    int i, j, n;
    double a;


    a = 0.0;

    n = p->num_vertices;

    for(i = 0; i < n; i++)
    {
        j = i + 1;
        if(j == n)
            j = 0;
        a += (p->vertex[i].y - p->vertex[j].y) *
            (p->vertex[j].x + p->vertex[i].x);
    }
    /* a clockwise shape should have a positive area, a counter 
     * clockwise shape should have a negative area */
    return 0.5 * a;
    /*return 0.5*ABS(a); <--this is wrong */
}

/* ===================================================== */
#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif
#define PI180 (M_PI/180.0)

/* This routine computes the length of a polyline. 
 * If the used defines env. variable MIMS_FLAT_SURFACE the length is computed 
 * using arcs on the surface of the Earth (the explicit Earth radius is not 
 * needed since it cancels out later in the fraction).  NOTE: there is no 
 * algorithm available in this program for calculating POLYGON area on a 
 * curved surface. */
double Length(Shape * p)
{
    /* Note: The length of a polygon does not include it's last segment 
     *       unless the polygon has the same first and last vertex */
    int i, j, n;
    double a = 0.0;

    double xa, xb, ya, yb;
    double cy, sy;
    double x, y;
    static int firstime = 1;
    static int flat_surface;
    char tmpEnvVar[10];

    if(firstime)
    {
        firstime = 0;
        flat_surface = 1;


        if(getEnvtValue(ENVT_CURVED_LINES, tmpEnvVar))
        {
            if(!strcmp(tmpEnvVar, "YES"))
            {
                flat_surface = 0;
            }
        }
    }

    n = p->num_vertices;

    if(flat_surface)
    {
        for(i = 0; i < n - 1; i++)
        {
            j = i + 1;
            /* use the flat assumption: c*c = a*a + b*b */
            x = (p->vertex[j].x - p->vertex[i].x);
            y = (p->vertex[j].y - p->vertex[i].y);
            a += sqrt(x * x + y * y);
        }
    }
    /* NOTE: when this algorithm was used, line surrogates did not add to 1 */
    else
    {
        for(i = 0; i < n - 1; i++)
        {
            xa = PI180 * p->vertex[i].x;
            ya = PI180 * p->vertex[i].y;
            xb = PI180 * p->vertex[j].x;
            yb = PI180 * p->vertex[j].y;
            cy = cos(ya);
            sy = sin(ya);
            a += fabs(acos(cos(xb - xa) * cy * cos(yb) + sy * sin(yb)));
        }
    }
    /* length should never be negative, right? */
    return ABS(a);

}

/* ===================================================== */
/* compute the area of a polygon as the total area for all
 * contours */
double PolyArea(PolyShape * p)
{
    int i, n;
    double a = 0.0;

    n = p->num_contours;
    for(i = 0; i < n; i++)
    {
        a += Area(p->contour + i);
    }
    return a;

}

/* ===================================================== */
/* Compute the total length of all contours in a shape 
 modified by dyang: 01/2014, to deduct line length in a hole */

double PolyLength(PolyShape * p)
{
    int i, n;
    double a = 0.0;
    int  isHole;

    n = p->num_contours;
    for(i = 0; i < n; i++)
    {
        a += Length(p->contour + i);
/*      isHole = p->hole[i];
        if ( isHole == NOT_A_HOLE )
           a += Length(p->contour + i);
        else
           a -= Length(p->contour + i);
*/
    }
    return a;

}

/* ===================================================== */
/* A parent describes where a polygon came from and points to 
 * a shape that was used to create it. */
Parent *newParent(PolyParent * pp, PolyShape * ps, int index)
{
    Parent *p;
    p = (Parent *) malloc(sizeof(Parent));
    if(p)
    {
        p->pp = pp;
        p->ps = ps;
        p->index = index;
    }
    return p;

}

/* ============================================================= */
/* create a polygon parent using the 2 parent polygons passed in */
PolyParent *newPolyParent(Parent * p1, Parent * p2)
{
    PolyParent *p;
    p = (PolyParent *) malloc(sizeof(PolyParent));
    if(p)
    {
        p->p1 = p1;
        p->p2 = p2;
    }
    return p;

}



/* ============================================================= */
/* insert a shape (ps) into a linked list of shapes (list). */

void polyShapeIncl(PolyShapeList **list, PolyShape *ps, PolyParent *pp)
{
    PolyShapeList *p;
    extern char *prog_name;

    if (ps == NULL)
    {
        ERROR(prog_name, "ps is NULL in polyShapeIncl", 1);
    }
    p = (PolyShapeList *) malloc(sizeof(p[0]));
    if(p)
    {
        p->ps = ps;
        p->bb = newBoundingBox(ps);
        p->pp = pp;
        
        p->next = NULL;

        if(*list == NULL)
        {
            p->prev = p;
            *list = p;
        }
        else
        {
            p->prev = (*list)->prev;
            (*list)->prev = (*list)->prev->next = p;
        }
    }
    else
    {
        ERROR(prog_name, "Allocation error in polyShapeIncl", 1);
    }

}

void copyAttributes(PolyObject *src, PolyObject *target)
{
 
    int n, o, num_attr, numObjects;
    char mesg[256];
    extern char *prog_name;

    num_attr = src->attr_hdr->num_attr;
    numObjects = src->nObjects;
    target->attr_hdr = getNewAttrHeader();

    target->attr_hdr->attr_desc = 
           (AttributeDesc **) malloc(num_attr * sizeof(AttributeDesc *));
    target->attr_val =  
           (AttributeValue **) malloc(numObjects * sizeof(AttributeValue *));


    if(target->attr_hdr->attr_desc == NULL ||
       target->attr_val == NULL)
    {
         sprintf(mesg, "Allocation error encountered during attribute copy");
         ERROR(prog_name, mesg, 1);

    }

    target->attr_hdr->num_attr = src->attr_hdr->num_attr;

    
    for(n = 0; n < num_attr; n++)
    {
        target->attr_hdr->attr_desc[n] = 
                      (AttributeDesc *) malloc(sizeof(AttributeDesc));
        target->attr_hdr->attr_desc[n]->name = 
                    (char *) strdup(src->attr_hdr->attr_desc[n]->name);
        target->attr_hdr->attr_desc[n]->type = src->attr_hdr->attr_desc[n]->type;
    }

    for(o = 0; o < numObjects; o++)
    {
        target->attr_val[o] = 
               (AttributeValue *) malloc(num_attr * sizeof(AttributeValue));

        for(n = 0; n < num_attr; n++)
        { 
             
             if(src->attr_hdr->attr_desc[n]->type == FTDouble)
             {
                  target->attr_val[o][n].val = src->attr_val[o][n].val;
             }
             else if(src->attr_hdr->attr_desc[n]->type == FTInteger)
             {
                  target->attr_val[o][n].ival = src->attr_val[o][n].ival;
 
             }
             else if(src->attr_hdr->attr_desc[n]->type == FTString)
             {

                  target->attr_val[o][n].str = 
                           (char *) strdup(src->attr_val[o][n].str);
             }
             else
             {
                  /* type error */
             }
 
        }
      
    }

} 

  
/* this should only be used on systems that don't have hypot in their
 * math library */
double hypot(double a, double b)
{
    return sqrt(a * a + b * b);
}
