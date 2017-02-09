/********************************************************************************
 *  intersect.c
 *
 *
 *   Updated: 4/12/2005, BDB added support for GPC_UNION when overlaying a shapefile
 *                        
 ********************************************************************************/
/**
 * File contains:
 * polyIsect
 * point_clip
 * line_clip
 * comp_*_*_vertex
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "shapefil.h"
#include "mims_spatl.h"
#include "parms3.h"
#include "io.h"

int point_clip(PolyShape * p1,  /* the set of points */
               PolyShape * p2,  /* the input polygon */
               PolyShape * p /* the output set of points */ );
int line_clip(PolyShape * p1, PolyShape * p2, PolyShape * p);

/* ============================================================= */
/* Intersects a shape (point, line, or polygon), with a polygon and  
 * returns the result as a set of contours (which could be points) in p 
 * The integer result is 0 if no intersection, 1 if there is an 
 * intersection, or -1 if there is an error. */
int polyIsect(PolyObject * poly1,       /* a point, line, or polygon */
              PolyObject * poly2,       /* a polygon */
              PolyObject * p,   /* the resulting points, lines, or polygons */
              bool isShapeOverlay)      /* identifies this operation as being part of a shapefile
                                           overlay so that we can perform special processing 
                                           with gpc */
{

    int i, j;
    int n1, n2;
    int n;
    int at_least_one;
    int itemp = 0;
    PolyShape *polyResult, *tmpPoly;
    PolyParent *pp;
    PolyShapeList *plist, *plist1, *plist2;
    Parent **p1;
    Parent **p2;
    double dummy = 0.0;
    char mesg[100];


/*     printf("poly2 type=%d\n", poly2->nSHPType); */
    if(poly2->nSHPType != SHPT_POLYGON)
    {
        WARN("Currently only POLYGONs are supported as second poly in polyIsect\n");
        return -1;
    }

    printBoundingBox(poly1->bb);
    printBoundingBox(poly2->bb);
    if(!OVERLAP2(poly1->bb, poly2->bb))
    {
        WARN("polyIsect: There is no intersection between the sets of polygons");
        return 0;
    }
    p->nSHPType = poly1->nSHPType;

    /*p->map = copyMapProj(poly1->map);*/        /* should be identical to poly2->map */
    p->map = NULL;
    p->parent_poly1 = poly1;
    p->parent_poly2 = poly2;

    n1 = poly1->nObjects;
    n2 = poly2->nObjects;

    sprintf(mesg, "numpoly1=%d, numpoly2 = %d\n", n1, n2);
    MESG(mesg);
    p1 = (Parent **) malloc(n1 * sizeof(Parent *));
    p2 = (Parent **) malloc(n2 * sizeof(Parent *));
    if(!p1 || !p2)
    {
        WARN("Allocation error in polyIsect");
        return 0;
    }

    plist = poly1->plist;

    for(i = 0; i < n1; i++)
    {
        pp = plist->pp;
        p1[i] = newParent(pp, plist->ps, i);
        plist = plist->next;
    }

    plist = poly2->plist;
    for(i = 0; i < n2; i++)
    {
        pp = plist->pp;
        p2[i] = newParent(pp, plist->ps, i);
        plist = plist->next;
    }
    plist2 = poly2->plist;

    at_least_one = 0;

    printBoundingBox(poly1->bb);

    for(j = 0; j < n2; j++)
    {
        /* check to see if bounding boxes for poly1 and the current contour of      
         * poly2 overlap */



        if(OVERLAP2(poly1->bb, plist2->bb))
        {
            plist1 = poly1->plist;
            for(i = 0; i < n1; i++)
            {

                /* check to see if bounding boxes for the 2 current contours overlap */
                if(OVERLAP2(plist1->bb, plist2->bb))
                {
                    polyResult = getNewPolyShape(0);
                    if(polyResult == NULL)
                    {
                        WARN("Malloc error for getNewPolyShape");
                        return -1;
                    }

                    /* AME: It seems that some optimization could be done here by
                     * moving the check for the shape type outside of the loop */

                    /* intersect the shape in p1 with the polygon in p2 */
                    if(poly1->nSHPType == SHPT_POLYGON)
                    {
                        /*fprintf(stderr,"poly passed second overlap j=%d %d\n", j, i); */
                        /* added code to support overlaying of shapefiles 4/12/2005 BDB */
                        if(isShapeOverlay)
                        {
                            tmpPoly = getNewPolyShape(0);

                            /* have to look at the overlaying poly and 
                               do the union on every shape that makes up that poly
                             */
                            if(tmpPoly == NULL)
                            {
                                WARN("Malloc error for getNewPolyShape");
                                return -1;
                            }
                            /*gpc_polygon_clip(GPC_UNION, NULL, plist2->ps, tmpPoly);

                               gpc_polygon_clip(GPC_INT, plist1->ps,
                               tmpPoly->plist->ps, polyResult);
                             */
                        }
                        else
                        {
                            gpc_polygon_clip(GPC_INT, plist1->ps, plist2->ps,
                                             polyResult);
                        }
#ifdef DEBUG
                        gpc_write_polygon(stderr, 0, polyResult);
#endif
                    }
                    else if(poly1->nSHPType == SHPT_POINT)
                    {
                        /*fprintf(stderr,"point passed second overlap j=%d i=%d\n", j, i); */
                        point_clip(plist1->ps, plist2->ps, polyResult);
                    }
                    else if(poly1->nSHPType == SHPT_ARC)
                    {
                        /*fprintf(stderr,"arc passed second overlap j=%d %d\n", j, i); */
                        line_clip(plist1->ps, plist2->ps, polyResult);
                    }
                    else
                    {
                        WARN("Invalid shape found: Currently only POLYGONs, POLYARCs and POINTs are supported");
                        return -1;
                    }
                    n = polyResult->num_contours;
                    /* if n > 0, there was a non-empty intersection of p1 and p2 */

                    if(n)
                    {
                        at_least_one = 1;
                        p->nObjects++;
                        pp = newPolyParent(p1[i], p2[j]);
                        polyShapeIncl(&(p->plist), polyResult, pp);
                    }
                    else
                    {
                        freePolyShape(polyResult);
                    }
                }
                plist1 = plist1->next;
            }                   /* for (i=0; i<n1; i++) */
        }                       /* if (OVERLAP2(poly1->bb, plist2->bb)) */

        /*else
           {
           if ((itemp % 100) == 0) {      
           printBoundingBox(plist2->bb);

           }
           itemp++;  
           } */
        plist2 = plist2->next;
    }   /* end for (j=0 ... */
    p->bb = newBBox(dummy, dummy, dummy, dummy);
    recomputeBoundingBox(p);

    /* return whether there was a non-empty intersection between p1 and p2 */
    return at_least_one;
}



/* ============================================================= */
/* Intersect a set of points with a polygon.  Return the points that 
 * fall within the polygon in p.  If a point is exactly coincident 
 * with a vertex of the polygon, a warning is issued and 0 is returned.  
 * Otherwise, 1 is returned. 
  * Modified by dyang: 01/2014, : to pass isHole to gpc_add_contour. */

int point_clip(PolyShape * p1,  /* the set of points */
               PolyShape * p2,  /* the input polygon */
               PolyShape * p /* the output set of points */ )
{
    int n1 = p1->num_contours;
    int n2 = p2->num_contours;
    int i, j;
    int k;
    Shape *shp;

    for(j = 0; j < n2; j++)
    {
        for(i = 0; i < n1; i++)
        {
            shp = p1->contour + i;
            k = InPoly(shp->vertex, p2->contour + j);
            if(k == VTX)
            {
                WARN("Point coincides with Vertex. Special attention needed");
                return 0;
            }
            if(k != OUT)
            {
                gpc_add_contour(p, shp, SOLID_POLYGON);
#ifdef DEBUG
                printf("Point (%f,%f) is in part=%d\n",
                       shp->vertex->x, shp->vertex->y, j);
#endif
            }
        }
    }
    return 1;
}


/* ============================================================= */
/* Intersect a set of lines (in p1) with a polygon (in p2) and return 
 * the intersection result in p (i.e. the portions of the lines that  
 * fall within the polygon.  The integer return value is 0 if an 
 * error occurred, and 1 if the computation was successful.  
 * Modified by dyang: 01/2014, : to pass isHole to addLineSegment. */

int line_clip(PolyShape * p1, PolyShape * p2, PolyShape * p)
{
    int n1 = p1->num_contours;
    int n2 = p2->num_contours;
    int isHole ;
    int i, j;
    int k, n;
    int l;
    Shape *shp1;
    Shape *shp2;
    Vertex v;
    Vertex *v0;
    Vertex vm;
    static Isect *vis;
    static int vi_size = 0;
    int m;
    int nn, kk;
    intersection_type c;
    int ps;                     /* previous segment */
    int l1, l2;
    int comp_x_p_vertex(const void *, const void *);
    int comp_x_n_vertex(const void *, const void *);
    int comp_y_p_vertex(const void *, const void *);
    int comp_y_n_vertex(const void *, const void *);

    if(vi_size == 0)
    {
        vi_size = 1024;
        vis = (Isect *) malloc(vi_size * sizeof(Isect));
        if(vis == NULL)
        {
            WARN("Allocation error in line_clip");
            return 0;
        }
    }
    for(i = 0; i < n1; i++)
    {
        shp1 = p1->contour + i;
        isHole = p1-> hole[i];

        for(j = 0; j < n2; j++)
        {
            shp2 = p2->contour + j;
            for(k = 0; k < shp1->num_vertices - 1; k++)
            {                   /* loop over line segs */
                m = 0;
                kk = k + 1;

                l = InPoly(shp1->vertex + k, shp2);     /* left edge of segment? */
                if(l == IN)
                {
                    vis[m].v.x = (shp1->vertex + k)->x;
                    vis[m].v.y = (shp1->vertex + k)->y;
                    vis[m].type = l;
                    m++;
                }

                for(n = 0; n < shp2->num_vertices; n++)
                {               /* loop over poly segs */
                    nn = (n + 1) % shp2->num_vertices;
                    /* compute the intersection of 2 line segments */
                    c = SegSegInt(shp1->vertex + k, shp1->vertex + kk,
                                  shp2->vertex + n, shp2->vertex + nn, &v);
                    if(c != NO_INT)
                    {
                        if(m - 1 > vi_size)
                        {
                            vi_size *= 2;
                            vis =
                                (Isect *) realloc(vis, vi_size * sizeof(Isect));
                            if(vis == NULL)
                            {
                                WARN("Allocation error in line_clip");
                                return 0;
                            }
                        }
                        vis[m].v.x = v.x;
                        vis[m].v.y = v.y;
                        /* handle all the possible cases of the segments intersecting */
                        if(c == PROP_INT)
                            vis[m].type = ISC;
                        else if(c == VTX_INT)
                            vis[m].type = VTX;
                        else
                        {
                            if(c == EDGE_AB_INT)
                            {
                                vis[m].v.x = (shp1->vertex + k)->x;
                                vis[m].v.y = (shp1->vertex + k)->y;
                                m++;
                                vis[m].v.x = (shp1->vertex + kk)->x;
                                vis[m].v.y = (shp1->vertex + kk)->y;
                            }
                            else if(c == EDGE_CD_INT)
                            {
                                vis[m].v.x = (shp2->vertex + n)->x;
                                vis[m].v.y = (shp2->vertex + n)->y;
                                m++;
                                vis[m].v.x = (shp2->vertex + nn)->x;
                                vis[m].v.y = (shp2->vertex + nn)->y;
                            }
                            else if(c == EDGE_AD_INT)
                            {
                                vis[m].v.x = (shp1->vertex + k)->x;
                                vis[m].v.y = (shp1->vertex + k)->y;
                                m++;
                                vis[m].v.x = (shp2->vertex + nn)->x;
                                vis[m].v.y = (shp2->vertex + nn)->y;
                            }
                            else if(c == EDGE_BC_INT)
                            {
                                vis[m].v.x = (shp1->vertex + kk)->x;
                                vis[m].v.y = (shp1->vertex + kk)->y;
                                m++;
                                vis[m].v.x = (shp2->vertex + n)->x;
                                vis[m].v.y = (shp2->vertex + n)->y;
                            }
                            else if(c == EDGE_AC_INT)
                            {
                                vis[m].v.x = (shp1->vertex + k)->x;
                                vis[m].v.y = (shp1->vertex + k)->y;
                                m++;
                                vis[m].v.x = (shp2->vertex + n)->x;
                                vis[m].v.y = (shp2->vertex + n)->y;
                            }
                            else if(c == EDGE_BD_INT)
                            {
                                vis[m].v.x = (shp1->vertex + kk)->x;
                                vis[m].v.y = (shp1->vertex + kk)->y;
                                m++;
                                vis[m].v.x = (shp2->vertex + nn)->x;
                                vis[m].v.y = (shp2->vertex + nn)->y;
                            }
                            vis[m - 1].type = vis[m].type = EDGE;
                        }

                        m++;
                    }
                }

                l = InPoly(shp1->vertex + kk, shp2);    /* right edge of segment? */
                if(l == IN)
                {
                    vis[m].v.x = (shp1->vertex + kk)->x;
                    vis[m].v.y = (shp1->vertex + kk)->y;
                    vis[m].type = l;
                    m++;
                }

                if(m)
                {
                    /* Re-order intersection table */
                    if((shp1->vertex + k)->x < (shp1->vertex + kk)->x)
                    {
                        qsort(vis, m, sizeof(Isect), comp_x_p_vertex);
                    }
                    else if((shp1->vertex + k)->x > (shp1->vertex + kk)->x)
                    {
                        qsort(vis, m, sizeof(Isect), comp_x_n_vertex);
                    }
                    else if((shp1->vertex + k)->y < (shp1->vertex + kk)->y)
                    {
                        qsort(vis, m, sizeof(Isect), comp_y_p_vertex);
                    }
                    else
                    {
                        qsort(vis, m, sizeof(Isect), comp_y_n_vertex);
                    }


                    v0 = &(vis[0].v);
                    l1 = vis[0].type;
                    ps = 0;     /* unknown type of prev. segment */

                    for(n = 1; n < m; n++)
                    {
                        /* sprintf("test==", "vnx=%f, vny = %f\n", vis[n].v.x, vis[n].v.y);
                       sprintf("test==", "v0x=%f, v0y = %f\n", v0->x, v0->y); */
                        l2 = vis[n].type;
                        if(l1 == OUT || l2 == OUT)
                        {
                            ps = -1;
                            goto next_seg;
                        }
                        if(l1 == IN || l2 == IN)
                        {
                            ps = 1;
                            addLineSegment(&(vis[n].v), v0, p, isHole);
                            goto next_seg;
                        }

                        if(l1 == EDGE && l2 == EDGE)
                        {
                            ps = 0;
                            addLineSegment(&(vis[n].v), v0, p, isHole);
                            goto next_seg;
                        }

                        if(l1 == ISC && ps == -1)
                        {
                            ps = 1;
                            addLineSegment(&(vis[n].v), v0, p, isHole);
                            goto next_seg;
                        }

                        if(l1 == ISC && ps == 1)
                        {
                            ps = -11;
                            goto next_seg;
                        }

                        else
                        {       /* we are not sure: the following test works for all seg */
                            if((v0->x == vis[n].v.x) && (v0->y == vis[n].v.y))
                                continue;
                            /* midpoint test */
                            vm.x = 0.5 * (v0->x + vis[n].v.x);
                            vm.y = 0.5 * (v0->y + vis[n].v.y);
                            l = InPoly(&vm, shp2);
                            if(l != OUT)
                            {
                                ps = 1;
                                addLineSegment(&(vis[n].v), v0, p, isHole);
                            }
                            else
                            {
                                ps = -1;
                            }
                        }
                      next_seg:
                        v0 = &(vis[n].v);
                        l1 = l2;
                    }
                }
            }
        }
    }
    return 1;
}



/* ============================================================= */
/* Used by qsort.  return 1 if x1 > x2, -1 if x1 < x2, 0 if x1=x2 */

int comp_x_p_vertex(const void *v1, const void *v2)
{
    Isect *V1 = (Isect *) v1;
    Isect *V2 = (Isect *) v2;

    if(V1->v.x < V2->v.x)
        return -1;
    if(V1->v.x > V2->v.x)
        return 1;
    return 0;
}

/* ============================================================= */
/* Used by qsort.  return 1 if x1 < x2, -1 if x1 > x2, 0 if x1=x2 */

int comp_x_n_vertex(const void *v1, const void *v2)
{
    Isect *V1 = (Isect *) v1;
    Isect *V2 = (Isect *) v2;

    if(V1->v.x > V2->v.x)
        return -1;
    if(V1->v.x < V2->v.x)
        return 1;
    return 0;
}

/* ============================================================= */
/* Used by qsort.  return 1 if y1 > y2, -1 if y1 < y2, 0 if y1=y2 */

int comp_y_p_vertex(const void *v1, const void *v2)
{
    Isect *V1 = (Isect *) v1;
    Isect *V2 = (Isect *) v2;

    if(V1->v.y < V2->v.y)
        return -1;
    if(V1->v.y > V2->v.y)
        return 1;
    return 0;
}

/* ============================================================= */
/* Used by qsort.  return 1 if y1 < y2, -1 if y1 > y2, 0 if y1=y2 */
int comp_y_n_vertex(const void *v1, const void *v2)
{
    Isect *V1 = (Isect *) v1;
    Isect *V2 = (Isect *) v2;

    if(V1->v.y > V2->v.y)
        return -1;
    if(V1->v.y < V2->v.y)
        return 1;
    return 0;
}
