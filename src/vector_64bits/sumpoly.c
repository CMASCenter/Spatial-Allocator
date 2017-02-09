/*****************************************************************************
* Updated 1/20/2014 -- Fixed bug for polygon surrogates when polygon 
*                      has holes inside for point weigh shapefile
******************************************************************************/

/*****************************************************************************
 * sumpoly.c
 *
 * File contains:
 * sum1Poly
 * sum2Poly
 * avg1Poly
 * typeAreaPercent
 * getPolyIntValue
 * setPolyIntValue
 * printOnePolyIntInfo
 * fillPolyIntInfo
 * freePolyIntInfo
 ******************************************************************************/

/*#define DEBUGCOUNTY  <-- define this to see debugging output for a county */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "shapefil.h"
#include "mims_spatl.h"
#include "parms3.h"

//#define DEBUGCOUNTY 
/*#define OLD_SUM */
static int inSum2Poly = 0;
static char mesg[256];


static char* debugcty = "51161";    /* set this to a county of interest */

/* ============================================================= */
/* Computes the denominator for the surrogate calculation.  Sum an  
 * attribute for a set of points/lines/polygons.  Also used for  
 * aggregating attributes such as population or area. 
 * poly = polyshapes to sum over
 * psum = pointer to sums (one per poly)
 * pn1 = number of polyshapes in poly
 * attr_id = attribute ID to use
 * use_weight_attr_value = if 1, then there is a weight attribute,
 *    otherwise, use the computed area / length / count
 *
 * Modified by dyang: 01/2014, : to deduct point value in polygon hole */

int sum1Poly(PolyObject * poly, double **psum, int *pn1, int attr_id,
             int use_weight_attr_value)
{
    int i, n;
    int n1;
    PolyShape *ps;
    PolyShapeList *plist;
    PolyParent *pp;
    PolyObject *w_poly;
    PolyObject *d_poly;
    double val;
    int ival;
    int id;
    double *sum;
    double frac;
    int weight_val_type;
    int data_val_type;
    int weight_shp_type;
    char* countyid;

    w_poly = poly->parent_poly1;        /* e.g. ports, roads, census tracts */
    d_poly = poly->parent_poly2;        /* e.g. counties */

    n1 = d_poly->nObjects;      /* e.g. # counties */

    *pn1 = n1;
    if(n1 < 0)
        return -1;
#ifdef DEBUGCOUNTY
    printf("sum1poly malloc n1 = %d, tot = %d\n", n1, n1 * sizeof(double));
#endif
    *psum = sum = (double *) malloc(n1 * sizeof(double));
    if(!sum)
    {
        WARN("Allocation error in Sum1Poly");
        return 1;
    }

    /* initialize sum of attribute value in "counties" to 0 */
    for(i = 0; i < n1; i++)
    {
        sum[i] = 0.0;
    }
#ifdef DEBUGCOUNTY
    data_val_type = d_poly->attr_hdr->attr_desc[0]->type;
    if(data_val_type == FTInteger)
        MESG("data value is INTEGER");
    else if(data_val_type == FTDouble)
        MESG("Data value is DOUBLE");
    else
        MESG("data value is STRING");
#endif 

    /* weight value type is int, double or string */

    weight_val_type = w_poly->attr_hdr->attr_desc[attr_id]->type;
    
    if(weight_val_type == FTInteger)
        MESG("weight value is INTEGER");
    else if(weight_val_type == FTDouble)
        MESG("weight value is DOUBLE\n");
    else
        MESG("weight value is STRING\n");

    /* weight poly type is point, arc or poly */
    weight_shp_type = w_poly->nSHPType;
    if(weight_shp_type == SHPT_POINT)
        MESG("weight object type is POINT");
    else if(weight_shp_type == SHPT_ARC)
        MESG("weight object type is ARC");
    else
        MESG("weight object type is POLY");
    //sprintf(mesg, "use weight attribute value = %d", use_weight_attr_value);
    //MESG(mesg);

    /* loop over polygons in the weight-data intersection */
    n = poly->nObjects;
    //sprintf(mesg, "num data polys = %d num weight-data ojects = %d\n", n1, n);
    //MESG(mesg);

    plist = poly->plist;

    for(i = 0; i < n; i++)
    {
        ps = plist->ps;         /* the intersected weight-data polygon */
        id = -1;
        pp = plist->pp;         /* the parent poly (= the county polygon) */

        if(pp)
        {
            while(pp->p1->pp)
            {
                pp = pp->p1->pp;
            }
            /* p1 is the parent weight poly, p2 is the parent data poly */
            /* AME: items are summed on ID the index of the data poly */
            id = pp->p2->index; /* this id corresponds to the county */
        }
        /* determine the fraction of the weight that should be added 
           to the county's total for that attribute */
        if(id >= 0)
        {
#ifdef DEBUGCOUNTY
            countyid = (d_poly->attr_val[id][0]).str;
            printf("sum1Poly: countyid=%s\n", countyid);
            printf("processing county %s, id=%d\n",countyid, id); 
            printf("num_contours=%d\n", ps->num_contours);
            /* bug??? for some reason the countyid is not in the right numeric range */
#endif
            /* get the value of the weight */
            if(use_weight_attr_value)
            {
                frac = 0.0;
                if(weight_val_type == FTInteger)
                {
                    val =
                        (double) (w_poly->attr_val[pp->p1->index][attr_id].
                                  ival);
                }
                else if(weight_val_type == FTDouble)
                {
                    val = w_poly->attr_val[pp->p1->index][attr_id].val;
                }
                /* else it's a string - let val be 1 */
                else
                {
                    val = 1.0;
                }
                if(weight_shp_type == SHPT_POINT)
                {
                    /* frac = the weight value * number in w-d poly */
                    if( ps->num_contours > 1 )
                        frac = 0.0;
                    else
                        frac = val;
                }
                else if((weight_shp_type == SHPT_ARC) && (val != 0.0))
                {
                    /* TBD: we're computing the length and area over and over again... */
                    /* the weight value * length in w-d poly / total weight length */
                    frac = val * PolyLength(ps) / PolyLength(pp->p1->ps);
#ifdef DEBUGCOUNTY
                    printf("aa %s  %s  %.4lf", countyid, debugcty, frac );
                    if((frac > 0.0) && (countyid == debugcty))
                    {
                        fprintf(stderr,
                                "sum1poly1: %s lenght = %lf, parent length = %lf, fract=%.4lf\n",
                                debugcty, PolyLength(ps), PolyLength(pp->p1->ps),
                                frac);
                    }
#endif

                }
                else /* it's a poly */ if (val != 0.0)
                {
                    /* frac = the weight value * area of w-d int / total weight area */
                    frac = val * (PolyArea(ps) / PolyArea(pp->p1->ps));
#ifdef DEBUGCOUNTY
                    if((frac > 0.0) && (countyid == debugcty))
                    {
                        fprintf(stderr,
                                "sum1poly1: %s shape area = %lf, parent area = %lf, fract=%.4lf\n",
                                debugcty, PolyArea(ps), PolyArea(pp->p1->ps),
                                frac);
                    }
#endif
                }
            }
            /* else use area, length, or count */
            else
            {
                if(weight_shp_type == SHPT_POINT)
                {
                    /* frac = the weight value * number in w-d poly */
                    frac = 1.0;
                }
                else if(weight_shp_type == SHPT_ARC)
                {
                    /* TBD: we're computing the length and area over and over again... */
                    /* frac = the weight value * length in w-d poly */
                    frac = PolyLength(ps);
                }
                else            /* polys */
                {
                    /* frac = area of w-d int */
                    frac = PolyArea(ps);
#ifdef DEBUGCOUNTY
                    if((frac > 0) && (countyid == debugcty))
                    {
                        fprintf(stderr,
                                "sum1poly2: %s shape area = %lf, frac=%lf\n",
                                debugcty, PolyArea(ps), frac);
                    }
#endif
                }
            }
            /* add the fraction for the current object to the total for the county */
            /* accumulate the sum based on the data poly ID */
            sum[id] += frac;
#ifdef DEBUG
            fprintf(stderr, "i = %d, id = %d, frac = %f\n", i, id, frac);
#endif
        }
        plist = plist->next;
    }

#ifdef DEBUG
    for(i = 0; i < n1; i++)
    {
        printf(" sum1poly(%d)=%f\n", i, sum[i]);
    }
#endif
    return 0;
}


/* return the number of intersections for data element i.
 * if i is out of range, return -1 */
int getNumIntersections(PolyIntStruct * polyInts, int i, int n1)
{
    if((i < 0) || (i >= n1))
        return -1;

    return polyInts[i].numIntersections;
}


/* get the value in the polyInts array for row i, "column j", where n1
 * is the total number of items in polyInts.  If a bad i or j is entered,
 * return MISSING */
double getPolyIntValue(PolyIntStruct * polyInts, int i, int j, int n1)
{
    int *indices;
    int numIntersects;
    int k;

    if((i < 0) || (i >= n1))
    {
        return MISSING;
    }
    numIntersects = polyInts[i].numIntersections;
    if(numIntersects == 0)
    {
        return 0.0;
    }
    indices = polyInts[i].intIndices;
    for(k = 0; k < numIntersects; k++)
    {
        if(indices[k] == j)
        {
            return polyInts[i].intValues[k];
        }
    }
    /* will only get here if the requested j index is not found */
    return 0.0;
}

/* set the value in the polyInts array for row i, "column j", to newVal,
 * where n1 is the total number of items in polyInts.  If a bad i or j is
 * entered, return -1 */
int setPolyIntValue(PolyIntStruct * polyInts, int i, int j, double newVal,
                    int n1)
{
    int *indices;
    int numIntersects;
    int k;

    if((i < 0) || (i >= n1))
        return -1;
    numIntersects = polyInts[i].numIntersections;
    indices = polyInts[i].intIndices;
    for(k = 0; k < numIntersects; k++)
    {
        if(indices[k] == j)
        {
            polyInts[i].intValues[k] = newVal;
            return 0;
        }
    }
    /* will only get here if the requested j index is not found */
    return -1;
}


/* allocate the guts of a single polyIntStruct give the number of intersections,
  and a pointer to an array of indices for those intersections with the 
  intersecting indices populating the first elements of the array */
int allocateOnePolyIntInfo(PolyIntStruct * polyIntInfoPtr, int numIntersects,
                           int *indices, double initialValue)
{
    int k;
    polyIntInfoPtr->numIntersections = numIntersects;
    polyIntInfoPtr->intIndices = (int *) malloc(numIntersects * sizeof(int));
    polyIntInfoPtr->intValues =
        (double *) malloc(numIntersects * sizeof(double));

    for(k = 0; k < numIntersects; k++)
    {
        polyIntInfoPtr->intIndices[k] = indices[k];
        polyIntInfoPtr->intValues[k] = initialValue;
    }
    return 0;
}

int printOnePolyIntInfo(PolyIntStruct * polyIntInfoPtr)
{
    int k;
    int ni = polyIntInfoPtr->numIntersections;

    if(ni == 0)
    {
        MESG("No intersections\n");
        return 0;
    }

    MESG(" indices = ");
    for(k = 0; k < ni; k++)
    {
        sprintf(mesg, "%d ", polyIntInfoPtr->intIndices[k]);
        MESG(mesg);
    }
    fprintf(stdout, "\n values = ");
    for(k = 0; k < ni; k++)
    {
        sprintf(mesg, "%f ", polyIntInfoPtr->intValues[k]);
        MESG(mesg);
    }
    MESG("\n");
    return 0;
}

/* populate polyInt Infowith values for data polygons and grid polygons */
int fillPolyIntInfo(PolyIntStruct * polyIntInfo, PolyObject * d_poly,
                    PolyObject * g_poly, int n1, int n2)
{
    PolyShapeList *d_list, *g_list;
    BoundingBox *d_bb, *g_bb;
    int *intersectionIndexList; /* the max # of grid cells that can be 
                                 * intersected with is n2 */
    int numIntersectsFound;
    int i, j;

    intersectionIndexList = (int *) malloc(n2 * sizeof(int));

    /* fill polyIntInfo with info about the intersections of the data polys with
     * the grid polys */
    d_list = d_poly->plist;
    for(i = 0; i < n1; i++)
    {
        d_bb = d_list->bb;
        g_list = g_poly->plist;
        numIntersectsFound = 0;
        for(j = 0; j < n2; j++)
        {
            g_bb = g_list->bb;
            /* the data poly bounding box overlaps the grid cell bounding box */
            if(OVERLAP2(d_bb, g_bb))
            {
                intersectionIndexList[numIntersectsFound] = j;
                numIntersectsFound++;
            }
            g_list = g_list->next;
        }
        allocateOnePolyIntInfo(&(polyIntInfo[i]), numIntersectsFound,
                               intersectionIndexList, 0.0);
        /*printOnePolyIntInfo(&(polyIntInfo[i])); */
        d_list = d_list->next;
    }
    free(intersectionIndexList);
    return 0;
}

/* free all items in polyIntInfo */
int freePolyIntInfo(PolyIntStruct * polyIntInfo, int n1)
{
    int i;
    if(polyIntInfo == NULL)
        return 1;
    for(i = 0; i < n1; i++)
    {
        free(polyIntInfo[i].intIndices);
        free(polyIntInfo[i].intValues);
        free(&polyIntInfo[i]);
    }
    return 0;
}

/* initialize an instance of polyInt Info with some dummy data for testing */
int initPolyIntInfo(PolyIntStruct * polyIntInfo, int n1, int n2)
{
    int numIntersects;
    int count;
    int i, j;

    if(polyIntInfo == NULL)
        return 1;

    numIntersects = 4;
    count = 0;
    for(i = 0; i < n1; i++)
    {
        polyIntInfo[i].numIntersections = numIntersects;
        polyIntInfo[i].intIndices = (int *) malloc(numIntersects * sizeof(int));
        polyIntInfo[i].intValues =
            (double *) malloc(numIntersects * sizeof(double));
        for(j = 0; j < numIntersects; j++)
        {
            /* or call:
               setPolyIntValue(polyIntInfo, i, j, count*2);
             */
            polyIntInfo[i].intIndices[j] = count;
            polyIntInfo[i].intValues[j] = (double) (count * 2);
            count++;
        }
    }
    return 0;
}


/* run a few tests of the basic functions */
int testPolyIntInfo(PolyIntStruct * polyIntInfo)
{
    int n1 = 100;

    polyIntInfo = (PolyIntStruct *) malloc(n1 * sizeof(PolyIntStruct));
    initPolyIntInfo(polyIntInfo, n1, 40);
    fprintf(stderr, "ints 10 = %d, ints 90 = %d\n",
            getNumIntersections(polyIntInfo, 10, n1),
            getNumIntersections(polyIntInfo, 90, n1));
    /* should return 4 and 4 */
    fprintf(stderr, "val(0, 3) = %f  \n",
            getPolyIntValue(polyIntInfo, 0, 3, n1));
    /* should return 6 */
    fprintf(stderr, "val(1, 6) = %f\n", getPolyIntValue(polyIntInfo, 1, 6, n1));
    /* should return 12 */

    /* calling getPolyIntValue with any bad j's should return MISSING */
    /* calling setPolyIntValue with any bad j's should return -1 */

    setPolyIntValue(polyIntInfo, 1, 6, 525.0, n1);
    fprintf(stderr, "val(1, 6) = %f\n", getPolyIntValue(polyIntInfo, 1, 6, n1));
    /* should return 525 */
    return 0;
}

/* ============================================================= */

/* Computes the numerator for the surrogate computation.  Sums an 
 * attribute based on data polygons with specified weights 
 * Modified by dyang: 01/2014, : to deduct point value in polygon hole */

int sum2Poly(PolyObject * dwg_poly, double ***psum, int *pnum_data_polys,
             int *pnum_grid_polys, int attr_id, PolyIntStruct ** polyIntInfoPtr,
             int use_weight_attr_value)
{
    int i, j, num_dwg_polys;
    int num_data_polys, num_grid_polys;
    PolyShape *ps;
    PolyShapeList *plist;
    PolyParent *pp;
    PolyObject *d_poly;
    PolyObject *w_poly;
    PolyObject *wd_poly;
    PolyObject *g_poly;
    double val;
    int ival;
    int data_poly_idx;
    int grid_cell_idx;
    double **sum;
    double oldVal;
    double frac;
    PolyIntStruct *polyIntInfo; /* will be malloc'd for # of d_polys */
    int weight_val_type;
    int weight_shp_type;
    char* countyid;

    inSum2Poly = 1;
    wd_poly = dwg_poly->parent_poly1;
    w_poly = wd_poly->parent_poly1;
    d_poly = wd_poly->parent_poly2;
    num_data_polys = d_poly->nObjects;

    g_poly = dwg_poly->parent_poly2;
    num_grid_polys = g_poly->nObjects;

    *pnum_data_polys = num_data_polys;
    *pnum_grid_polys = num_grid_polys;
    if(num_data_polys < 0 || num_grid_polys < 0)
        return -1;

    /* initialize the polyIntInfo structure with info about the intersection
     * of the grid and data polygons */
    polyIntInfo =
        (PolyIntStruct *) malloc(num_data_polys * sizeof(PolyIntStruct));
    fillPolyIntInfo(polyIntInfo, d_poly, g_poly, num_data_polys,
                    num_grid_polys);
    /* set the value of polyIntInfoPtr to polyIntInfo to pass back to caller */
    *polyIntInfoPtr = polyIntInfo;

#ifdef OLD_SUM
    printf("num_data_polys malloc in sum2poly = %d %d\n", num_data_polys,
           num_data_polys * sizeof(double *));
    *psum = sum = (double **) malloc(num_data_polys * sizeof(double *));
    if(!sum)
    {
        WARN("Allocation error in Sum2Poly");
        return 1;
    }

    /* instead of allocating the huge 2d array, could keep linear array that
       has values for each data-grid intersection, but may also need transpose -
       i.e. grid->data intersection */
    printf("num_grid_polys malloc in sum2poly = %d %d\n", num_grid_polys,
           num_data_polys * num_grid_polys * sizeof(double *));
    /* this malloc for each num_data_polys is the big one - almost 300MB worth */
    /* num_data_polys = # of data polygons (=54775 in the test case) */
    for(i = 0; i < num_data_polys; i++)
    {
        /* allocate an array where num_grid_polys=# of grid cells, for each data poly (num_data_polys) */
        /* num_grid_polys = 1240 in the test case */
        /* allows for case where any data poly can intersect any grid cell */
        sum[i] = (double *) malloc(num_grid_polys * sizeof(double));
        if(!sum[i])
        {
            WARN("Allocation error in Sum2Poly");
            return 2;
        }
        for(j = 0; j < num_grid_polys; j++)
        {
            sum[i][j] = 0.0;
        }
    }
#endif
    /* weight value type is int, double or string */
    weight_val_type = w_poly->attr_hdr->attr_desc[attr_id]->type;
    /* weight poly type is point, arc or poly */
    weight_shp_type = w_poly->nSHPType;
    if(weight_val_type == FTInteger)
        fprintf(stdout, "weight val is INTEGER\n");
    else if(weight_val_type == FTDouble)
        fprintf(stdout, "weight val is DOUBLE\n");
    else
        fprintf(stdout, "weight val is STRING\n");

    if(weight_shp_type == SHPT_POINT)
        fprintf(stdout, "weight poly type is POINT\n");
    else if(weight_shp_type == SHPT_ARC)
        fprintf(stdout, "weight poly type is ARC\n");
    else
        fprintf(stdout, "weight poly type is POLY\n");
    //sprintf(mesg, "use weight attribute value = %d\n", use_weight_attr_value);
    //MESG(mesg);

    num_dwg_polys = dwg_poly->nObjects;
    //sprintf(mesg, "num data-weight-grid polys = %d\n", dwg_poly->nObjects);
    //MESG(mesg);
    /* TBD: cache the area / length of polys to prevent recomputing */
    plist = dwg_poly->plist;
    for(i = 0; i < num_dwg_polys; i++)
    {
        ps = plist->ps;

        /* find grid cell index */
        grid_cell_idx = -1;
        pp = plist->pp;
        if(pp)
        {
            while(pp->p2->pp)
            {
                pp = pp->p2->pp;
            }
            grid_cell_idx = pp->p2->index;
        }

        /* find data poly index */
        data_poly_idx = -1;
        pp = plist->pp;
        if(pp)
        {
            while(pp->p1->pp)
            {
                pp = pp->p1->pp;
            }
            data_poly_idx = pp->p2->index;
        }

        if(data_poly_idx >= 0 && grid_cell_idx >= 0)
        {
            if(use_weight_attr_value)
            {
                if(weight_val_type == FTInteger)
                {
                    val =
                        (double) (w_poly->attr_val[pp->p1->index][attr_id].
                                  ival);
                }
                else if(weight_val_type == FTDouble)
                {
                    val = w_poly->attr_val[pp->p1->index][attr_id].val;
                }
                else            /*it's a string */
                {
                    val = 1.0;
                }
                if(weight_shp_type == SHPT_POINT)
                {
                    if( ps->num_contours > 1 )
                        frac = 0.0;
                    else
                        frac = val;
                }
                else if(weight_shp_type == SHPT_ARC)
                {
                    /* the weight value * length in w-d-g poly / total weight length */
                    frac = val * PolyLength(ps) / PolyLength(pp->p1->ps);
                }
                else
                {
                    /* the weight value * area of w-d-g int / total weight area */
                    frac = val * (PolyArea(ps) / PolyArea(pp->p1->ps));
#ifdef DEBUGCOUNTY
                    countyid = (d_poly->attr_val[data_poly_idx][0]).str;
                    //fprintf(stderr, "sum2poly ctyid = %s\n", countyid);

                    if(countyid == debugcty)
                    {
                        fprintf(stderr,
                                "sum2poly1: %d shape area = %lf, parentarea = %lf, gc=%d, frac=%.4lf\n",
                                debugcty, PolyArea(ps), PolyArea(pp->p1->ps),
                                grid_cell_idx, frac);
                    }
#endif
                }
            }
            else
            {
                if(weight_shp_type == SHPT_POINT)
                {

                    if( ps->num_contours > 1 )
                        frac = 0.0;
                    else
                        frac = 1.0;
                }
                else if(weight_shp_type == SHPT_ARC)
                {
                    /* frac = length in w-d-g poly */
                    frac = PolyLength(ps); /* why not an actual fraction? */
                }
                else
                {               /* polygons */
                    /* frac = area of w-d-g poly */
                    frac = PolyArea(ps); /* why not an actual fraction? */
#ifdef DEBUGCOUNTY
                    countyid = (d_poly->attr_val[data_poly_idx][0]).str;
                    fprintf(stderr, "sum2poly ctyid = %s\n", countyid);
                    /* AME: why isn't the county id in the right range?!? */
                    if(countyid == debugcty)
                    {
                        fprintf(stderr,
                         "sum2poly2: %d shape area = %lf, data poly = %d, gridcell=%d, frac=%.4lf\n",
                                debugcty, PolyArea(ps), data_poly_idx,
                                grid_cell_idx, frac);
                    }
#endif
                }
            }

            /* replace sum with 1D array + helpers */
#ifdef OLD_SUM
            sum[data_poly_idx][grid_cell_idx] += frac;
#endif
            oldVal = getPolyIntValue(polyIntInfo, data_poly_idx, grid_cell_idx,
                                     num_data_polys);
            if(oldVal == MISSING)
            {
                WARN("couldn't find index in polyIntInfo");
            }
            else
            {
                setPolyIntValue(polyIntInfo, data_poly_idx, grid_cell_idx,
                                oldVal + frac, num_data_polys);
            }

        }
        plist = plist->next;
    }

#ifdef DEBUG
    for(i = 0; i < num_data_polys; i++)
    {
        for(j = 0; j < num_grid_polys; j++)
        {
            printf(" A(%d,%d)=%f\n", i, j, sum[i][j]);
        }
    }
#endif

    inSum2Poly = 0;
    //fprintf(stdout, "returning 0\n");
    return 0;
}

/* ============================================================= */
/* Compute an average of an attribute, as would be used for aggregating 
 * densities.  Attributes are normalized by area and resulting values  
 * averaged. 
 * Modified by dyang: 01/2014, : to deduct point value in polygon hole */

int avg1Poly(PolyObject * poly, double **pavg, int *pnum_data_polys,
             int attr_id, int use_weight_attr_value)
{
    int i, n;
    int num_data_polys;
    PolyShape *ps;
    PolyShapeList *plist;
    PolyParent *pp;
    PolyObject *w_poly;
    PolyObject *d_poly;
    double val;
    int ival;
    int id;
    int weight_val_type, weight_shp_type;
    double *avg;
    double frac;


    w_poly = poly->parent_poly1;
    d_poly = poly->parent_poly2;

    num_data_polys = d_poly->nObjects;

    *pnum_data_polys = num_data_polys;
    if(num_data_polys < 0)
        return -1;
    *pavg = avg = (double *) malloc(num_data_polys * sizeof(double));
    if(!avg)
    {
        WARN("Allocation error in Avg1Poly");
        return 1;
    }

    for(i = 0; i < num_data_polys; i++)
    {
        avg[i] = 0.0;
    }

    n = poly->nObjects;  /* number of intersected objects */
    plist = poly->plist;

    weight_val_type = w_poly->attr_hdr->attr_desc[attr_id]->type;

    /* loop over each intersected object */
    for(i = 0; i < n; i++)
    {
        ps = plist->ps;

        id = -1;
        pp = plist->pp;

        if(pp)
        {
            while(pp->p1->pp)
            {
                pp = pp->p1->pp;
            }
            id = pp->p2->index;
        }

        if(id >= 0)
        {
            if(use_weight_attr_value)
            {
                frac = 0.0;

                if(weight_val_type == FTInteger)
                {
                    val =
                        (double) (w_poly->attr_val[pp->p1->index][attr_id].
                                  ival);
                }
                else if(weight_val_type == FTDouble)
                {
                    val = w_poly->attr_val[pp->p1->index][attr_id].val;
                }
                /* else it's a string - let val be 1 */
                else
                {
                    val = 1.0;
                }
                if(w_poly->nSHPType == SHPT_POINT)
                {
                    if( ps->num_contours > 1 )
                        frac = 0.0;
                    else
                        frac = val;  /* frac is the weight value */
                }
                /* AME -  don't bother to calc length or area if val is 0 */
                else if((w_poly->nSHPType == SHPT_ARC) && (val != 0))
                {
                    frac = val * PolyLength(ps) / PolyLength(pp->p1->ps); 
                }
                else /* is's a poly */ if (val != 0)
                {
                    frac = val * PolyArea(ps);
                }
            } 
            else /* don't use_weight_attr_value */
            {
                if(weight_shp_type == SHPT_POINT)
                {
                    /* frac = the weight value * number in w-d poly */
                    frac = 1.0;
                }
                else if(weight_shp_type == SHPT_ARC)
                {
                    frac = PolyLength(ps);
                }
                else            /* polys */
                {
                    frac = PolyArea(ps);
                }
            }
            avg[id] += frac;
        }
        plist = plist->next;
    }
    plist = d_poly->plist;
    for(i = 0; i < num_data_polys; i++)
    {
        ps = plist->ps;
        if(d_poly->nSHPType == SHPT_POINT)
        {
            frac = 1.0;
        }
        else if(d_poly->nSHPType == SHPT_ARC)
        {
            frac = PolyLength(ps);
        }
        else
        {
            frac = PolyArea(ps);
        }
        if(frac != 0.0)
        {
            avg[i] /= frac;
            /*printf("frac=%f\n", frac);*/
        }
        else
        {
            WARN("Division by zero attempt in Avg1Poly. Normalization ignored.");
        }
        plist = plist->next;
    }

    return 0;
}


/* Calculates the discrete overlap, which allows you to specify non-numeric
   attributes with the input poly (aka weight poly) and have a way to 
   decide which attribute will be output if there is more than one candidate.

   For example, a county shapefile has discrete attributes FIPS_CODE and
   COUNTY and when the county shapes are intersected with a grid, we calculate
   the area for each county overlapped by the grid and use that to decide
   which attributes are representative.

   This code also works with lines (tests line length) and points (tests
   total number of points)


*/
int discreteOverlap(PolyObject *poly, int **pmax, int *pn1, int attr_id)
{
    int i, n;
    int n1;
    PolyShape *ps;
    PolyShapeList *plist;
    PolyParent *pp;
    PolyObject *w_poly;
    PolyObject *d_poly;
    double val;
    int ival;
    int id;
    int *idsOfMaxOverlaps;
    double frac, *area;
    int weight_val_type;
    int data_val_type;
    int weight_shp_type;
    extern char *prog_name;

    w_poly = poly->parent_poly1;        /* e.g. ports, roads, census tracts */
    d_poly = poly->parent_poly2;        /* e.g. counties */

    n1 = d_poly->nObjects;      /* e.g. # counties */

    *pn1 = n1;
    
    if(n1 < 0)
        return -1;

    *pmax = idsOfMaxOverlaps = (int *) malloc(n1 * sizeof(int));

 
    area = (double *) malloc(n1 * sizeof(double));

     
    if(!idsOfMaxOverlaps || !area)
    {
        ERROR(prog_name, "Memory allocation error in discreteOverlay", 1);
    }

    /* initialize sum of attribute value for data polys to 0 */
    for(i = 0; i < n1; i++)
    {
        idsOfMaxOverlaps[i] = -1;
        area[i] = 0.0;
    }

    /* weight value type is int, double or string */

    weight_val_type = w_poly->attr_hdr->attr_desc[attr_id]->type;
    
    if(weight_val_type == FTInteger)
        MESG("weight value is INTEGER");
    else if(weight_val_type == FTDouble)
        MESG("weight value is DOUBLE\n");
    else
        MESG("weight value is STRING\n");

    /* weight poly type is point, arc or poly */
    weight_shp_type = w_poly->nSHPType;
    if(weight_shp_type == SHPT_POINT)
        MESG("weight object type is POINT");
    else if(weight_shp_type == SHPT_ARC)
        MESG("weight object type is ARC");
    else
        MESG("weight object type is POLY");
   /* sprintf(mesg, "use weight attribute value = %d", use_weight_attr_value);
    MESG(mesg);
    */

    /* loop over polygons in the weight-data intersection */
    n = poly->nObjects;

    
    //sprintf(mesg, "num data polys = %d num weight-data ojects = %d\n", n1, n);
    //MESG(mesg);

    plist = poly->plist;

    for(i = 0; i < n; i++)
    {
        ps = plist->ps;         /* the intersected weight-data polygon */
        id = -1;
        pp = plist->pp;         /* the parent poly (= the county polygon) */

        if(pp)
        {
            while(pp->p1->pp)
            {
                pp = pp->p1->pp;
            }
            /* p1 is the parent weight poly, p2 is the parent data poly */
            /* AME: items are summed on ID the index of the data poly */
            id = pp->p2->index; /* this id corresponds to the county */
        }
        /* determine the fraction of the weight that should be added 
           to the county's total for that attribute */
        if(id >= 0)
        {
            if(weight_shp_type == SHPT_POINT)
            {
                /* frac = the weight value * number in w-d poly */
                frac = 1.0; /* not sure what happens with point shapes... */
            }
            else if(weight_shp_type == SHPT_ARC)
            {
                /* TBD: we're computing the length and area over and over again... */
                /* frac = the weight value * length in w-d poly */
                frac = PolyLength(ps);
            }
            else            /* polys */
            {
                /* frac = the weight value * area of w-d int */
                frac = PolyArea(ps);
            }
           

            if(frac > area[id])
            {
                 area[id] = frac;
                 idsOfMaxOverlaps[id] = pp->p1->index;
            }
            

#ifdef DEBUG
            printf("i = %d, id = %d, frac = %f area[%d]=%f\n", i, id, frac, id, area[id]);
#endif
        }
        plist = plist->next;
    }

#ifdef DEBUG
    
    for(i = 0; i < n1; i++)
    {
         printf(" largestOverlap(%d)=%d\n", i, idsOfMaxOverlaps[i]);
    }
#endif
    return 0;
}



int discreteCentroid(PolyObject *poly, int **cent, int *pn1, int attr_id)
{
    int i, n;
    int n1;
    PolyShape *ps;
    PolyShapeList *plist;
    PolyParent *pp;
    PolyObject *w_poly;
    PolyObject *d_poly;
    PolyObject *c_poly;
    PolyObject *res_poly;
    double val;
    int *centroid;
    int ival;
    int id;
    int *idsOfMaxOverlaps;
    int data_val_type;
    extern char *prog_name;

    w_poly = poly->parent_poly1;        
    d_poly = poly->parent_poly2;        

    n1 = d_poly->nObjects;      

    *pn1 = n1;
    
    if(n1 < 0)
        return -1;

    *cent = centroid = (int *) malloc(n1 * sizeof(int));
     
    if(!cent)
    {
        ERROR(prog_name, "Memory allocation error in discreteCentroid", 1);
    }

    for(i = 0; i < n1; i++)
    {
         centroid[i] = -1;
    }


    /* iterate through d_poly, calculate centroids, results in a new poly
       that is the centroids */
    /* keep d_poly attribs */
    c_poly = getCentroidPoly(d_poly);

    copyAttributes(d_poly, c_poly);

    /* check to make sure the weights are polygons  before intersection call */
    if(w_poly->nSHPType != SHPT_POLYGON)   
    {
         sprintf(mesg,"Centroid calculation can only be performed on a polygon shapefile");
         ERROR(prog_name, mesg, 1);
    }
    res_poly = getNewPoly(0);
    polyIsect(c_poly, w_poly, res_poly, FALSE);

    /* weight value type is int, double or string */

    /* call polyIsect with new poly against w_poly (w must be type POLY)
       use newly intersected polygon and use that to fill up the centroid array */
    /* loop over polygons in the weight-data intersection */
    n = res_poly->nObjects;
    sprintf(mesg, "num data polys = %d num weight-data ojects = %d\n", n1, n);
    MESG(mesg);

    /* this is the newly intersected poly */
    plist = res_poly->plist;

    for(i = 0; i < n; i++)
    {
        ps = plist->ps;         /* the intersected weight-data polygon */
        id = -1;
        pp = plist->pp;         /* the parent poly */

        if(pp)
        {
            while(pp->p1->pp)    /* p1 is centroid poly */
            {
                pp = pp->p1->pp;
            }

            id = pp->p2->index;  /* p2 is weight poly */
        }

        if(id >= 0)
        {
            centroid[pp->p1->index] = id; 
#ifdef DEBUG
            printf("i = %d, id = %d\n", i, id);
#endif
        }

        plist = plist->next;
    }

#ifdef DEBUG
    
    for(i = 0; i < n1; i++)
    {
         
    }
#endif
    return 0;
}



/*This subroutine will return area percent for each type in the attribute*/
int typeAreaPercent(PolyObject * poly, double ***psum, double *gridA, int attr_id, int numTypes, char ***list)
{
    int i, j,n;
    int n1, id_w, id_d;
    PolyShape *ps;
    PolyShapeList *plist;
    PolyParent *pp;
    PolyObject *w_poly;
    PolyObject *d_poly;
    double douVal;
    int intVal;
    char strVal[50];  
    double **sum;  /*sum for all grid and all types [type][grid]*/
    double gridArea,typeArea;
    int weight_val_type;
    int data_val_type;
    int weight_shp_type;
    char ** list_array;  /*types of a variable list*/
    extern char *prog_name;


    w_poly = poly->parent_poly1;        /* e.g. polygon shapes such as surf zone with land and ocean */
    d_poly = poly->parent_poly2;        /* e.g. polygon shapes such as grids -- output shapes*/
   
    n1 = d_poly->nObjects;      /* e.g. # of grids */
  
    list_array = *list;     /*get attribute type list array*/

    if(n1 < 0)
        return -1;
    
    /*allocate memory for each type*/
    *psum = sum = (double **) malloc(numTypes * sizeof(double *));
    if(!sum)
    {
        WARN("Allocation error 1 in typeAreaPercent");
        return 1;
    }
    
    /*allocate memory for all grids in each type*/
    for(i = 0; i < numTypes; i++)
    {        
        sum[i] = (double *) malloc(n1 * sizeof(double));
        if(!sum[i])
        {
            WARN("Allocation error 2 in typeAreaPercent");
            return 2;
        }
        for(j = 0; j < n1; j++)
        {
            sum[i][j] = 0.0;
        }
    }
    
    /* weight value type is int, double or string */
    weight_val_type = w_poly->attr_hdr->attr_desc[attr_id]->type;
    if(weight_val_type == FTInteger)
        MESG("weight value is INTEGER");
    else if(weight_val_type == FTDouble)
        MESG("weight value is DOUBLE\n");
    else
        MESG("weight value is STRING\n");

    /* weight poly type is point, arc or poly */
    weight_shp_type = w_poly->nSHPType;
    if(weight_shp_type != SHPT_POLYGON)
    {
	sprintf(mesg,"Weight object type is not POLYGON");
        ERROR(prog_name, mesg, 1);         
    }
       
    /* loop over polygons in the weight-data intersection */
    n = poly->nObjects;
    sprintf(mesg, "num data polys = %d num weight-data ojects = %d\n", n1, n);
    MESG(mesg);

   /*go through each poly in the intersected polygons*/
    plist = poly->plist;
    for(i = 0; i < n; i++)
    {
        ps = plist->ps;         /* the intersected weight-data polygon */
	typeArea = PolyArea(ps);
        id_w = -1;        
	id_d = -1;
        pp = plist->pp;         /* the parent poly */

        if(pp)
        {
            while(pp->p1->pp)
            {
                pp = pp->p1->pp;
            }
            /* p1 is the parent weight poly, p2 is the parent data poly */
            /* AME: items are summed on ID the index of the data poly */
            id_w = pp->p1->index; /* index id to weight polys	    */
            id_d = pp->p2->index; /* index id to data polygon such county or grid*/
	    
        }
        /* determine the area of the attribute type*/ 
        if(id_d >= 0 && id_w >=0)
        {
            *gridA = gridArea = PolyArea(pp->p2->ps);
            /*printf("typeAreaPercent: gridArea=%lf.5\n", gridArea);*/
           
            /* get the attribute of the weight */
	    if (weight_val_type == FTInteger)
            {
              intVal = w_poly->attr_val[id_w][attr_id].ival; 
              sprintf(strVal, "%d", intVal);
            }
            else if (weight_val_type == FTDouble)
            {
              douVal = w_poly->attr_val[id_w][attr_id].val; 
              sprintf(strVal, "%lf", douVal);
            }
            else if (weight_val_type == FTString)
            {
              sprintf(strVal,"%s",w_poly->attr_val[id_w][attr_id].str); 
            } 
		    
            /*get index to the type in type list*/
	    for(j=0; j<numTypes;j++)
            {
              if(strcmp(strVal,list_array[j])==0)
              {
                break;
              }
            }	    
	    sum[j][id_d] += typeArea/gridArea;
        }
        plist = plist->next;
    }


    /*print out the area for each type*/
    /*for (i=0;i<numTypes;i++)
    {	   
      for(j = 0; j < n1; j++)
      {
        printf(" typeAreaPercent type=%d  poly=%d  typePercent=%f\n", i,j, sum[i][j]);
      }
    }*/
      
return 0;
}
