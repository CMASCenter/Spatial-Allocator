/****************************************************************************
 * mims_spatl_proj.c
 * 
 * getNewMap
 * getFullMapProjection
 * mimsFreeProjection
 * mimsProject
 * mimsSetProjection
 * projectBBox
 * projectPoint
 * storeProjection
 * copyMapProj
 * compareDatum
 * compareProjection
 *
 * An interface between the MIMS Spatial Tool and the PROJ4.3 library.
 * 
 * Developed by Atanas Trayanov of MCNC Environmental Modeling Center in
 * suport of EPA's Multimedia Integrated Modeling System.
 *
 * Updated April 20, 2005, added newer env. var. controls --BDB
 * Updated June 2005, added copyMapProj --BDB
 * Update June 2006, added compareDatum, compareProjection for checking different projections -- LR
 * Updated Nov. Dec. 2007, added datum transformation and used new pj_transform -- LR
 *                         in the new version, compareDatum, compareProjection are not used anymore 
 * **************************************************************************/
#include <string.h>
#include "mims_spatl.h"
#include "projects.h"
#include "parms3.h"
#include "io.h"

#define MAXPROJ 16
PJ *mimsSetProjection (MapProjInfo *map);

/* set these variables using storeProjection */
static PJ *inprojection  = NULL;   /* the projection of incoming data */
static PJ *outprojection = NULL;   /* the projection we want to change to */
static int projNeeded = 1; /* whether a change is needed if they have different projection and datum*/
static int count = 0;

/* **************************************************************************
 * mimsFreeProjection
 *
 * release a projection handle for use with PROJ4.3
 *
 * act as a wrapper to protect against library changes in PROJ
 * 
 * **************************************************************************/
int mimsFreeProjection ( PJ *p) {
  if ( p ) {
    pj_free ( p );
  }

  return ( 1 );
}

/* Store a pair of projections for later use many times */
int storeProjection( MapProjInfo *inproj, MapProjInfo *outproj )
{
  char mesg[256];
  extern char *prog_name;

  if (inprojection != NULL)
     mimsFreeProjection (inprojection);
  if (outprojection != NULL)
     mimsFreeProjection (outprojection);

  if (inproj == NULL)
  {
     ERROR(prog_name, "storeProjection: inproj is NULL",2);
     return -1;
  } 
  if (outproj == NULL)
  {
     ERROR(prog_name, "storeProjection: outproj is NULL",2);
     return -1;
  }  

  MESG("Input projection:\n");
  inprojection  = mimsSetProjection ( inproj );
  if (inprojection == NULL) {
    ERROR(prog_name, "Error setting PROJ.4 input projection",2);
    return -1;
  }
 
  MESG("Output projection:\n");
  outprojection = mimsSetProjection ( outproj );
  if (outprojection == NULL) {
    ERROR(prog_name,"Error setting PROJ.4 output projection",2);
    return -1;
  }

  /* projection need will be 0 if the two projections are the same*/
 /* if (compareProjection(inproj,outproj)==1 && compareDatum(inproj,outproj) == 1) 
  { 
     projNeeded = 0;  //the same projection and datum, not needed
  }
  else
  {
     projNeeded = 1;  //not the same projection, needed
  }
*/

  return 0;
}

/* Specify a projection */
PJ *mimsSetProjection (MapProjInfo *map)
{
  PJ      *p = NULL;
  extern char *prog_name;
  int i;
  int argc = 0;
  char      *args[MAXPROJ];
  char *tmp, *t;
  char mesg[256];

  
  if (map == NULL)
  {
     sprintf(mesg, "mimsSetProjection: map is NULL");
     goto error;
  } 

  /* allocate space for the maximum number of proj args */
  for (i=0; i<MAXPROJ; i++) 
  {
    args[i] = (char *)malloc(80*sizeof(char)); 
    if(!args[i]) 
    {
      sprintf(mesg,"Allocation error for projection info");
      goto error;
    }
  }

  i=1;  //first element set to +proj=

  /*Define Ellipsoid */  
    if(map->earth_ellipsoid != NULL) 
    {
      t = (char *) strdup(map->earth_ellipsoid);
      if (t==NULL) 
      {
        sprintf(mesg,"Allocation error for earth_ellipsoid");
        goto error;
      }
      tmp = strtok(t, ",");
      while (tmp != NULL) 
      {
        sprintf(mesg,"args[%d]=%s\n",i,tmp);
        MESG(mesg);
        sprintf(args[i++],"%s", tmp);
        tmp = strtok(NULL,",");
      }
      /*if (t) free(t);*/
    }
    else {
      sprintf(mesg, "Earth Ellipsoid is not defined.\n");
      goto error;
    }

    if (map->ctype == LATGRD3)
    {
      sprintf(mesg,"setting geographic lat-long parameters");
      MESG(mesg);
      sprintf(args[0],"%s","+proj=latlong");
    }
    else if (map->ctype == LAMGRD3)
    {
      sprintf(mesg,"setting Lambert Conic Conformal -- 2SP parameters");
      MESG(mesg);
      sprintf(args[0],"%s","+proj=lcc");
      sprintf(args[i++],"%s=%lf","+lat_1",map->p_alp);
      sprintf(args[i++],"%s=%lf","+lat_2",map->p_bet);
      sprintf(args[i++],"%s=%lf","+lon_0",map->xcent);
      sprintf(args[i++],"%s=%lf","+lat_0",map->ycent);
      sprintf(args[i++],"%s=%s","+units","m");
    }
    else if (map->ctype == MERGRD3) {
      //this projection has different parameters from IOAPI's Mercator projection
      sprintf(mesg,"setting Hotine Oblique Mercator parameters");
      MESG(mesg);
      sprintf(args[0],"%s","+proj=omerc");
      sprintf(args[i++],"%s=%lf","+lat_0",map->p_alp);
      sprintf(args[i++],"%s=%lf","+lonc",map->p_bet);
      sprintf(args[i++],"%s=%lf","+alpha",map->p_gam);
      sprintf(args[i++],"%s=%s","+k_0","1");          //we set scale factor = 1 different from IOAPI
      sprintf(args[i++],"%s=%lf","+x_0",map->xcent); //different from IOAPI
      sprintf(args[i++],"%s=%lf","+y_0",map->ycent); //different from IOAPI
    }
    else if (map->ctype == STEGRD3) {
      //this projection has different parameters from IOAPI's projection parameters
      sprintf(mesg,"setting stereographic parameters");
      MESG(mesg);
      sprintf(args[0],"%s","+proj=stere");
      sprintf(args[i++],"%s=%lf","+lat_0",map->p_alp);
      sprintf(args[i++],"%s=%lf","+lon_0",map->p_bet);
      sprintf(args[i++],"%s=%lf","+x_0",map->xcent);
      sprintf(args[i++],"%s=%lf","+y_0",map->ycent);
    }
    else if (map->ctype == UTMGRD3) {
      sprintf(mesg,"setting UTM parameters for the Northern Hemisphere. Unblock line with +south in mims_spatl_proj.c for the Southern hemisphere.\n");
      MESG(mesg);
      sprintf(args[0],"%s","+proj=utm");  // default for the northern hemisphere
      //sprintf(args[i++],"%s","+south");  // for the southern hemisphere
      sprintf(args[i++],"%s=%lf","+zone",map->p_alp);
      sprintf(args[i++],"%s=%lf","+x_0",map->xcent);
      sprintf(args[i++],"%s=%lf","+y_0",map->ycent); 
    }
    else if (map->ctype == POLGRD3) { 
      //this projection has different parameters from IOAPI's projection parameters
      sprintf(mesg,"setting polar stereographic parameters");
      MESG(mesg);
      sprintf(args[0],"%s","+proj=stere");
      sprintf(args[i++],"%s=%lf","+lat_ts",map->p_alp);
      if (map->p_alp > 0.0)
      { //north
         sprintf(args[i++],"%s=%s","+lat_0", "90");
      }
      if (map->p_alp < 0.0) 
      { //south
         sprintf(args[i++],"%s=%s","+lat_0", "-90");
      }
      sprintf(args[i++],"%s=%lf","+lon_0",map->p_bet);
      sprintf(args[i++],"%s=%lf","+k_0",map->p_gam);    //should be normally 1.0
      sprintf(args[i++],"%s=%lf","+x_0", map->xcent);
      sprintf(args[i++],"%s=%lf","+y_0", map->ycent);
    }
    else if (map->ctype == EQMGRD3) { 
      //this projection has different parameters from IOAPI's projection parameters
      sprintf(mesg,"setting Equatorial Mercator parameters");
      MESG(mesg);
      sprintf(args[0],"%s","+proj=merc");
      sprintf(args[i++],"%s=%lf","+lat_ts",map->p_alp);
      sprintf(args[i++],"%s=%lf","+lon_0",map->p_gam);    
      sprintf(args[i++],"%s=%lf","+x_0", map->xcent);
      sprintf(args[i++],"%s=%lf","+y_0", map->ycent);
    }
    else if (map->ctype == TRMGRD3) {
      //this projection has different parameters from IOAPI's projection parameters
      sprintf(mesg,"setting Transverse Mercator parameters");
      MESG(mesg);
      sprintf(args[0],"%s","+proj=tmerc");
      sprintf(args[i++],"%s=%lf","+lat_0",map->p_alp);
      sprintf(args[i++],"%s=%lf","+lon_0", map->p_bet);
      sprintf(args[i++],"%s=%lf","+k",map->p_gam);
      sprintf(args[i++],"%s=%lf","+x_0", map->xcent);
      sprintf(args[i++],"%s=%lf","+y_0", map->ycent);
    }
    else if (map->ctype == ALBGRD3)
    {
      sprintf(mesg,"setting Albers Equal-Area Conic parameters");
      MESG(mesg);
      sprintf(args[0],"%s","+proj=aea");
      sprintf(args[i++],"%s=%lf","+lat_1",map->p_alp);
      sprintf(args[i++],"%s=%lf","+lat_2",map->p_bet);
      sprintf(args[i++],"%s=%lf","+lon_0",map->xcent);
      sprintf(args[i++],"%s=%lf","+lat_0",map->ycent);
      sprintf(args[i++],"%s=%s","+units","m");
    }

    else if (map->ctype == LEQGRD3) {
      sprintf(mesg,"setting Lambert Azimuthal Equal Area parameters");
      MESG(mesg);
      sprintf(args[0],"%s","+proj=laea");
      sprintf(args[i++],"%s=%lf","+lat_0",map->p_alp);
      sprintf(args[i++],"%s=%lf","+lon_0",map->p_bet);
      sprintf(args[i++],"%s=%lf","+x_0",map->xcent);
      sprintf(args[i++],"%s=%lf","+y_0",map->ycent);
      sprintf(args[i++],"%s=%s","+units","m");
    }
   else if (map->ctype == SINUGRD3) {
      sprintf(mesg,"setting Sinusoidal parameters");
      MESG(mesg);
      sprintf(args[0],"%s","+proj=sinu");
      sprintf(args[i++],"%s=%lf","+lon_0", map->p_alp);
      sprintf(args[i++],"%s=%lf","+x_0", map->xcent);
      sprintf(args[i++],"%s=%lf","+y_0", map->ycent);
    }
    else if (map->ctype == CUSTOM3) {
      if(map->custom_proj_str != NULL) 
      {
        t = (char *) strdup(map->custom_proj_str);
        /*sprintf(mesg,"Custom projection string = %s\n",map->custom_proj_str);
        MESG(mesg);*/
        if (t==NULL) 
        {
          sprintf(mesg,"Allocation error for custom projection");
          goto error;
        }
        tmp = strtok(t, ",");
        while (tmp != NULL) 
        {
          if (strstr(tmp, "+proj=") == NULL)
          {
            sprintf(args[i++],"%s", tmp);
            sprintf(mesg,"param %d = %s, ",i-1,args[i-1]);
          }
          else
          {
            sprintf(args[0],"%s", tmp);
            sprintf(mesg,"param %d = %s, ",0,args[0]);
          }
          MESG(mesg);
          tmp = strtok(NULL,",");
        }
        /*if (t) free(t);*/
      }
      /* ignore */
    }
    else {
      sprintf(mesg,"Projection type (%d) not implemented yet",map->ctype);
      goto error;
    }

  argc = i;
  if (argc > MAXPROJ) {
    sprintf(mesg,"ERROR: too many projection arguments (%d)",argc);
    goto error;
  }

  strcpy(mesg,"PROJ args=");
  for (i=0; i<argc; i++) {
    strcat(mesg,args[i]);
   }
  MESG(mesg);
  MESG("\n");

 /*initialize projection*/
  if ( argc > 0 && args[0]) { 
     if (!( p = pj_init ( argc, args )))
     {
        sprintf(mesg, "Using from definition: argc=%d",argc );
        WARN(mesg);
        for( i = 0; i < argc; i++ )
        {
            sprintf(mesg, "%s ", args[i] );
            WARN(mesg);
        }
        sprintf(mesg,"projection initialization failure: %s",
              pj_strerrno(pj_errno));
        WARN(mesg);
        sprintf(mesg,"ERROR: in setting projection pj_init");
        goto error;
     }
  }

  for (i=0; i<MAXPROJ; i++) {
    if (args[i]) free(args[i]);
  }
  return ( p );

error:
  WARN(mesg);
  return NULL;
}

/***************************************************************************/

/* Return a UV point with coordinates in p converted from the inproj map
 * projection to the outproj map projection */
int projectPointInt ( PJ *inproj, PJ *outproj, projUV p, double *newx, double *newy)
{
  double z = 0.0;
  extern char *prog_name;
  
  if (pj_is_latlong(inproj))
  {
     p.u *= DEG_TO_RAD;
     p.v *= DEG_TO_RAD; 
  }

  if (p.u != HUGE_VAL) {
            if( pj_transform( inproj, outproj, 1, 0, &(p.u), &(p.v), &z ) != 0 )
            {
                p.u = HUGE_VAL;
                p.v = HUGE_VAL;
            }
  }

  if (p.u == HUGE_VAL) 
  {/* error output */
     ERROR(prog_name, "Error in projectionPointInt: p.u=HUGE_VAL",2);
     return -1;
  }

  if ( pj_is_latlong(outproj) ) 
  {
      p.u *= RAD_TO_DEG;
      p.v *= RAD_TO_DEG;
  }

  *newx = p.u;
  *newy = p.v;
  return 0;
}

/***************************************************************************
 * project all the points in poly to the new output map projection in map_out
 */
int mimsProject ( PolyObject *poly,   MapProjInfo *map_out )
{
  projUV   p;    /* struct { double u, double v } */

  /* for each vertex project it and stuff the projected point back into */
  /* same PolyObject.  Proj assumes data is in radians so convert it.      */
  /* Proj will convert Geographic -> <proj> and <proj> -> Geographic      */
  /* so <proj1> -> <proj2> requires bouncing though geographic              */

  int i, j, k, n;
  int np, nv;
  Vertex *v;
  PolyShape *ps;
  PolyShapeList *plist;
  PJ *inproj=NULL;
  PJ *outproj=NULL;
  double newx, newy;
  char mesg[256];


 /*the two projections are the same*/
/*  if (compareProjection(poly->map,map_out)==1 && compareDatum(poly->map,map_out) == 1)
  {
     //the same and no need to projection
     return 0;
  }
*/

  inproj  = mimsSetProjection ( poly->map );
  if (inproj == NULL) {
    sprintf(mesg,"%s","mimsProject: inproj is null");
    goto error;
  }


  outproj = mimsSetProjection ( map_out );
  if (outproj == NULL) {
    sprintf(mesg,"%s","mimsProject: outproj is null");
    goto error;
  }


#ifdef DEBUG
  printf("DEBUG: projection ptrs %x %x\n",inproj, outproj);
#endif

  n = poly->nObjects;
  plist = poly->plist;
  for (i=0; i < n; i++) {
    ps = plist->ps;
    np = ps->num_contours;
    for (j=0; j<np; j++) {
      nv = ps->contour[j].num_vertices;
      v =  ps->contour[j].vertex;
      for (k=0; k<nv; k++) 
      {
          p.u = v[k].x;
          p.v = v[k].y;
          projectPointInt( inproj, outproj, p, &newx, &newy);
          v[k].x = newx;
          v[k].y = newy;
      }  //end of k
    } //end of j
    plist = plist->next;
  }  //end of i

  poly->map = map_out;
  mimsFreeProjection (inproj);
  mimsFreeProjection (outproj);

  recomputeBoundingBox(poly);

  return 0;

 error:
  WARN(mesg);
  return 1;
}

/***************************************************************************/

MapProjInfo *getNewMap(void)
{
  MapProjInfo *map;

  map = (MapProjInfo *)malloc(sizeof(MapProjInfo));
  map->earth_ellipsoid = NULL;
  map->custom_proj_str = NULL;
  map->gridname = NULL;
  map->ctype = 0.0;
  map->ncols = 0.0;
  map->nrows = 0.0;
  map->p_alp = 0.0;
  map->p_bet = 0.0;
  map->p_gam = 0.0;
  map->xcent = 0.0;
  map->ycent = 0.0;
  map->xorig = 0.0;
  map->yorig = 0.0;
  map->xcell = 0.0;
  map->ycell = 0.0;
  return map;
}

/***************************************************************************/

int projectBBox ( BoundingBox *bb_in, BoundingBox *bb_out, 
    MapProjInfo *map_in, MapProjInfo *map_out )
{

  /* for each corner of bounding box bbin project it and stuff the projected 
   * point back into bbout.  Proj assumes data is in radians so convert it. 
   * Proj will convert Geographic -> <proj> and <proj> -> Geographic      
   * so <proj1> -> <proj2> requires bouncing though geographic              */

  projUV   p; /* struct { double u, double v } for proj library */
  PJ *inproj=NULL;
  PJ *outproj=NULL;
  double newx, newy;
  char mesg[256];


  /* the two projections are the same*/
/*  if (compareProjection(map_in,map_out)==1 && compareDatum(map_in,map_out) == 1)
  {
     copyBBoxToFrom(bb_out, bb_in);
     return 0;
  }
*/

  inproj  = mimsSetProjection ( map_in );
  if (inproj == NULL) {
    sprintf(mesg,"%s","Error in projectBBox: inproj is null");
    goto error;
  }

  outproj = mimsSetProjection ( map_out );
  if ( outproj == NULL) {
    sprintf(mesg,"%s","Error in projectBBox: outproj is null");
    goto error;
  }

  p.u = bb_in->xmin;
  p.v = bb_in->ymin;
  projectPointInt( inproj, outproj, p, &newx, &newy);
  bb_out->xmin = newx;
  bb_out->ymin = newy;

  p.u = bb_in->xmax;
  p.v = bb_in->ymax;
  projectPointInt( inproj, outproj, p, &newx, &newy);
  bb_out->xmax = newx;
  bb_out->ymax = newy;

  mimsFreeProjection (inproj);
  mimsFreeProjection (outproj);

  return 0;

 error:
  WARN(mesg);
  return 1;
}

/***************************************************************************/

/* Return the coordinates x & y in newx & newy using the projections set
 * in storeProjection. Return value is -1 if an error is detected. */
int projectPoint ( double x, double y, double *newx, double *newy)
{
  extern char *prog_name;
  projUV   p; /* struct { double u, double v } for proj library */
  char mesg[256];
  double z = 0.0;

/*if (projNeeded == 0)
  {
     *newx = x;
     *newy = y;
     return 0;
  }
*/

  p.u = x;
  p.v = y;

#ifdef DEBUG  
  if (count < 5)  
  {   
     printf("in point x,y = %f %f\n", x, y);  
  }  
#endif  

  if (pj_is_latlong(inprojection))
  {
     p.u *= DEG_TO_RAD;
     p.v *= DEG_TO_RAD;
  }

  if (p.u != HUGE_VAL) {

            if( pj_transform( inprojection, outprojection, 1, 0, &(p.u), &(p.v), &z ) != 0 )
            {
                p.u = HUGE_VAL;
                p.v = HUGE_VAL;
            }
  }

  if (p.u == HUGE_VAL)
  {/* error output */
     ERROR(prog_name, "Error in projectionPointInt: p.u=HUGE_VAL",2);
     return -1;
  }

  /* x-y or decimal degree ascii output */
  if ( pj_is_latlong(outprojection) )
  {
         p.u *= RAD_TO_DEG;
         p.v *= RAD_TO_DEG;
  }

  *newx = p.u;
  *newy = p.v;


#ifdef DEBUG  
  if (count < 5)  
  {    
     printf("converted x,y = %lf %lf\n", p.u, p.v);  
  }  
  count++;  
#endif  

  return 0;
}

/***************************************************************************/
MapProjInfo *getFullMapProjection(
  char *ellipsoid_envt_var_name,
  char *map_proj_envt_var_name)
{
  extern char *prog_name;
  char mesg[256];
  char map_projn[512];
  char earth_ellipsoid[50];
  MapProjInfo *map;


  /* set up the map projection for the shape file */
  map = getNewMap();
  if (map == NULL) 
  {
    sprintf(mesg,"getNewMap error for %s", map_proj_envt_var_name);
    ERROR(prog_name,mesg,2);
    return NULL;
  }


  if (ellipsoid_envt_var_name != NULL)
  {
	if(!getEnvtValue(ellipsoid_envt_var_name, earth_ellipsoid))
	{
		 sprintf(mesg, "Unable to read ellipsoid because %s is not set", 
							  ellipsoid_envt_var_name);
		 ERROR(prog_name, mesg, 2);
                 return NULL;
	}

        if ((map->earth_ellipsoid = (char *) strdup(earth_ellipsoid)) == NULL) 
	{
		sprintf(mesg,"Allocation error for ellipsoid %s",ellipsoid_envt_var_name);
		ERROR(prog_name, mesg,2);
		return NULL;
	}
  }
  else
  {
     sprintf(mesg, "ELLIPSOID variable for the Earth coordinate system is not defined.");
     ERROR(prog_name, mesg, 2);
     return NULL;
  }

  sprintf(mesg,"Ellipsoid=%s",map->earth_ellipsoid);
  MESG(mesg);

  /* need to set for all shapefile or grid*/
  if(!getEnvtValue(map_proj_envt_var_name, map_projn))
        {
                 sprintf(mesg, "Unable to read map projection because %s is not set",
                                                          map_proj_envt_var_name);
                 ERROR(prog_name, mesg, 2);
                 return NULL;
        }
 
  if (strstr(map_projn, "+proj=") == NULL) 
  {
    /* try to read as a grid, if that doesn't work, then it's an error */
    if(getMapProjFromGriddesc(map_projn, map))
    {
       sprintf(mesg,
           "Map projection environment variable %s must contain '+proj' or be a grid name",
                      map_proj_envt_var_name);
       ERROR(prog_name, mesg, 2);
       return NULL;
    }
    map->custom_proj_str = NULL; 
  }
  else
  {
    map->ctype = CUSTOM3; /* we borrowed this type to indicate that user
                             specified the projection with env. var */
    if (strcmp(map_projn,"LATLON") == 0)
    {
       sprintf(map_projn,"+proj=latlong");
    }
    map->custom_proj_str = (char *) strdup(map_projn);
    map->gridname = NULL;
  }
  return map;
}

int  getMapProjFromGriddesc(char *gname, MapProjInfo *map)
{
    char *cname[20];
    int nthik;
    char mesg[256];
    extern char *prog_name;

    if(!dscgridc(gname, cname, &map->ctype, &map->p_alp, &map->p_bet, &map->p_gam,
               &map->xcent, &map->ycent, &map->xorig, &map->yorig, &map->xcell, &map->ycell,
               &map->ncols, &map->nrows, &nthik ))
    {
          sprintf(mesg,"Error in GRIDDESC file for gridname \"%s\"", gname);
          ERROR(prog_name, mesg, 2);
          return 1;
    }
    return 0;

}

/***************************************************************************/

MapProjInfo *copyMapProj(MapProjInfo *inMap)
{
  MapProjInfo *map;

  map = (MapProjInfo *) malloc(sizeof(MapProjInfo));

  if(inMap->earth_ellipsoid != NULL)
  {
       map->earth_ellipsoid = (char *) strdup(inMap->earth_ellipsoid);
  }
  else
  {
       map->earth_ellipsoid = NULL;
  }


  if(inMap->custom_proj_str != NULL)
  {
/*        printf("custom proj string=%s\n", inMap->custom_proj_str); */
       map->custom_proj_str = (char *) strdup(inMap->custom_proj_str);
/*        printf("after strdup in copy\n"); */
         map->gridname = NULL;
  }
  else
  {
   map->custom_proj_str = NULL;
  }

  if(inMap->gridname != NULL)
  {
/*        printf("right before strdup of map->gridname inMap=%s\n", inMap->gridname); */
       map->gridname = (char *) strdup(inMap->gridname);
       map->custom_proj_str = NULL;
  }
  else
  {
      map->gridname = NULL;
  }

/*   printf("doing other int copies\n"); */
  map->ctype = inMap->ctype;
  map->ncols = inMap->ncols;
  map->nrows = inMap->nrows;
  map->p_alp = inMap->p_alp;
  map->p_bet = inMap->p_bet;
  map->p_gam = inMap->p_gam;
  map->xcent = inMap->xcent;
  map->ycent = inMap->ycent;
  map->xorig = inMap->xorig;
  map->yorig = inMap->yorig;
  map->xcell = inMap->xcell;
  map->ycell = inMap->ycell;
  

  return map;
}

/********************************************************************************/
/*function to compare ellipsoid*/
int compareDatum (MapProjInfo *inMap1, MapProjInfo *inMap2)
{
  /*return type
    0 = not the same geographic datum
    1 = the same geographic datum
  */ 
  char mesg[256];

  sprintf(mesg,"Compare two datum systems for geographic coordinate systems...");
  MESG(mesg);

  sprintf(mesg,"In Ellipsoid=%s  Out Ellipsoid=%s\n",inMap1->earth_ellipsoid,inMap2->earth_ellipsoid);
  MESG(mesg);

  if(inMap1->earth_ellipsoid != NULL && inMap2->earth_ellipsoid != NULL)
  {
    if(strcmp(inMap1->earth_ellipsoid,inMap2->earth_ellipsoid) == 0)
    {
      //all ellipsoid arguments have to be in the same order, or will result in difference 
      return 1;  /*the same*/
    }
    else 
    {
      return 0;  /*not the same*/
    }
  }
  else if (inMap1->earth_ellipsoid == NULL && inMap2->earth_ellipsoid == NULL)
  {
    printf(mesg,"The two ellipsoids to be compared with are not defined.");
    MESG(mesg);
    return 1;  /*the same NULL*/
  }
  else
  {
    return 0;  /*not the same*/
  }

}


/*only compare projection, not the datum*/
int compareProjection(MapProjInfo *inMap1, MapProjInfo *inMap2)
{
  /* return type:
  0 = not the same projection
  1 = the same projection
  */

  extern char *prog_name;
  char mesg[256];
  MapProjInfo *map1, *map2, *map;
  int j,i,k;
  char *tmp, tmp1[40], *t, *a_name, *a_value;
  int argc = 0;
  char      *args[MAXPROJ];

  
  sprintf(mesg,"Compare two projections...");
  MESG(mesg);

  /* allocate space for the maximum number of proj args */
  for (i=0; i<MAXPROJ; i++)
  {
    args[i] = (char *)malloc(80*sizeof(char));
    if(!args[i])
    {
      sprintf(mesg,"Allocation error for projection parameters");
      ERROR(prog_name, mesg, 2);
    }
  }

  i=0;

  map1 = copyMapProj(inMap1);
  map2 = copyMapProj(inMap2);

    sprintf(mesg,"1 gridname=%s   2 gridname=%s\n",map1->gridname,map2->gridname);
    MESG(mesg);
    sprintf(mesg,"1 custom_proj_str=%s   2 custom_proj_str=%s\n",map1->custom_proj_str,map2->custom_proj_str);
    MESG(mesg);
    sprintf(mesg,"1 ctype=%d   2 ctype=%d\n",map1->ctype,map2->ctype);
    MESG(mesg);
    sprintf(mesg,"1 p_alp=%lf  2 p_alp=%lf\n",map1->p_alp,map2->p_alp);
    MESG(mesg);
    sprintf(mesg,"1 p_bet=%lf  2 p_bet=%lf\n",map1->p_bet,map2->p_bet);
    MESG(mesg);
    sprintf(mesg,"1 p_gam=%lf  2 p_gam=%lf\n",map1->p_gam,map2->p_gam);
    MESG(mesg);
    sprintf(mesg,"1 xcent=%lf  2 xcent=%lf\n",map1->xcent,map2->xcent);
    MESG(mesg);
    sprintf(mesg,"1 ycent=%lf  2 ycent=%lf\n",map1->ycent,map2->ycent);
    MESG(mesg);

  /*both projections defined as custom projection -- user defined*/
  if(map1->custom_proj_str != NULL && map2->custom_proj_str != NULL)
  {
    if (strcmp(map1->custom_proj_str, map2->custom_proj_str) == 0)
    {
      // all projection arguments have to be in the same order, or will result in difference
      return 1;  /*the custom projection*/
    }
  }

  /*both projections defined in GRIDDESC.txt file*/
  if(map1->gridname != NULL && map2->gridname != NULL)
  {
    if (strcmp( map1->gridname, map2->gridname)==0)
    {
      return 1;  /*same grid name*/
    }
  }

  /*processing in and out custom map projection parameters*/
  for (j=0;j<2;j++)
  {
     if (j==0) 
     {
       map = map1;
     }
     else if (j==1) 
     {
       map = map2;
     }
   
   if(map->custom_proj_str != NULL)
    { 

     /*store parameters in args array*/
     t = (char *) strdup(map->custom_proj_str);
     sprintf(mesg,"%d custom projection string: %s\n",j+1,map->custom_proj_str);
     MESG(mesg);
     if (t==NULL)
        {
          sprintf(mesg,"1: Allocation error for custom projection");
          ERROR(prog_name, mesg, 2); 
        }
     tmp = strtok(t, ",");
     i = 0;
     while (tmp != NULL)
     { 
          sprintf(args[i++],"%s", tmp);
          /*sprintf(mesg,"param %d = %s, ",i-1,args[i-1]);
          MESG(mesg);*/
          tmp = strtok(NULL,",");
     }
     argc = i;

     for (i=0;i<argc;i++)
     {
          t = (char *) strdup(args[i]);
          if (t==NULL)
          {
            sprintf(mesg,"2: Allocation error for custom projection");
            ERROR(prog_name, mesg, 2);
          }

          /*sprintf(mesg,"Parameter: %s",t);
          MESG(mesg);*/

          a_name = strtok(t, "=");
          a_value = strtok(NULL,"=");  /*get the value for the projection argument*/

          /*sprintf(mesg,"Parameter: name=%s  value=%s\n",a_name,a_value);
          MESG(mesg);*/

          if (a_name != NULL && a_value != NULL)
          {
	     /*lat and long projection*/
	     if (strstr(map->custom_proj_str, "latlong") != NULL)
             {
	       if (strcmp(a_name, "+proj") == 0)
               {     
                 map->ctype = LATGRD3;
	       }
             } 
	    
	     /*Lambert Conic Conformal (2SP) projection*/
	     if (strstr(map->custom_proj_str, "lcc") != NULL) 
	     {
              if (strcmp(a_name, "+proj") == 0)
               {
		 map->ctype = LAMGRD3;       
	       }
	      if (strcmp(a_name, "+lat_1") == 0)
              {
                 map->p_alp = atof(a_value);
              } 
              else if (strcmp(a_name, "+lat_2") == 0)
              {
                 map->p_bet = atof(a_value);
              }
              else if (strcmp(a_name, "+lon_0") == 0)
              {
                 map->p_gam = atof(a_value);
                 map->xcent = atof(a_value);
              }
              else if (strcmp(a_name, "+lat_0") == 0)
              {
                 map->ycent = atof(a_value);
              } 
	    }

            /*Oblique Mercator projection*/
            if ( strstr(map->custom_proj_str, "omerc") != NULL )
            {
                if (strcmp(a_name, "+proj") == 0)
                {
                   map->ctype = MERGRD3;
                }
                else if (strcmp(a_name, "+lat_0") == 0)
                {
                  map->p_alp = atof(a_value);
                }
                else if (strcmp(a_name, "+lonc") == 0)
                {
                  map->p_bet = atof(a_value);
                }
                else if (strcmp(a_name, "+alpha") == 0)
                {
                  map->p_gam = atof(a_value);
                }
                else if (strcmp(a_name, "+x_0") == 0)
                {
                  map->xcent = atof(a_value);
                }
                else if (strcmp(a_name, "+y_0") == 0)
                {
                  map->ycent = atof(a_value);
                }
            }

	    /*Stereographic projection setting*/
	    if (strstr(map->custom_proj_str, "stere") != NULL && strstr(map->custom_proj_str, "lat_ts") == NULL)
            {
                if (strcmp(a_name, "+proj") == 0)
		{
		   map->ctype = STEGRD3;   
		}
		else if (strcmp(a_name, "+lat_0") == 0)
                {
                   map->p_alp = atof(a_value);
                }
               else if (strcmp(a_name, "+lon_0") == 0)
               {
                  map->p_bet = atof(a_value);               
               }
               else if (strcmp(a_name, "+x_0") == 0)
                {
                  map->xcent = atof(a_value);
                }
                else if (strcmp(a_name, "+y_0") == 0)
                {
                  map->ycent = atof(a_value);
                }
	    }

	    /*utm*/
	    if (strstr(map->custom_proj_str, "utm") != NULL)
            {
                if (strcmp(a_name, "+proj") == 0)
		{
		   map->ctype = UTMGRD3;
		}
		else if (strcmp(a_name, "+zone") == 0)
                {
                  map->p_alp = atof(a_value);
                }               
                else if (strcmp(a_name, "+x_0") == 0)
                {
                  map->xcent = atof(a_value);
                }
                else if (strcmp(a_name, "+y_0") == 0)
                {
                  map->ycent = atof(a_value);
                }
	    }

            /*polar Stereographic projection setting*/
            if (strstr(map->custom_proj_str, "stere") != NULL && strstr(map->custom_proj_str, "lat_ts") != NULL)
            {
                if (strcmp(a_name, "+proj") == 0)
                {
                   map->ctype = POLGRD3;   
                }
                else if (strcmp(a_name, "+lat_ts") == 0)
                {
                   map->p_alp = atof(a_value);
                } 
                else if (strcmp(a_name, "+lon_0") == 0)
                {
                   map->p_bet = atof(a_value);
                }
                else if (strcmp(a_name, "+k_0") == 0)
                {
                   map->p_gam = atof(a_value);
                }
                else if (strcmp(a_name, "+x_0") == 0)
                {
                  map->xcent = atof(a_value);
                }
                else if (strcmp(a_name, "+y_0") == 0)
                {
                  map->ycent = atof(a_value);
                } 
	    } 

            /*Equatorial Mercator projection*/    
            if (strstr(map->custom_proj_str, "merc") != NULL)
            {
                if (strcmp(a_name, "+proj") == 0)
                {
                   map->ctype = EQMGRD3;
                }
                else if (strcmp(a_name, "+lat_ts") == 0)
                {
                  map->p_alp = atof(a_value);
                }
                else if (strcmp(a_name, "+lon_0") == 0)
                {
                  map->p_gam = atof(a_value);
                }
                else if (strcmp(a_name, "+x_0") == 0)
                {
                  map->xcent = atof(a_value);
                }
                else if (strcmp(a_name, "+y_0") == 0)
                {
                  map->ycent = atof(a_value);
                } 
            }

            /*Transverse Mercator projection*/    
            if (strstr(map->custom_proj_str, "tmerc") != NULL)
            {
                if (strcmp(a_name, "+proj") == 0)
                {
                   map->ctype = TRMGRD3;
                }
                else if (strcmp(a_name, "+lat_0") == 0)
                {
                  map->p_alp = atof(a_value);
                }
                else if (strcmp(a_name, "+lon_0") == 0)
                {
                  map->p_bet = atof(a_value);
                }
                else if (strcmp(a_name, "+k") == 0)
                {
                  map->p_gam = atof(a_value);
                }
                else if (strcmp(a_name, "+x_0") == 0)
                {
                  map->xcent = atof(a_value);
                }
                else if (strcmp(a_name, "+y_0") == 0)
                {
                  map->ycent = atof(a_value);
                } 
            }

            /*Albers Equal-Area Conic projection*/
             if (strstr(map->custom_proj_str, "aea") != NULL)
             {
              if (strcmp(a_name, "+proj") == 0)
               {
                 map->ctype = ALBGRD3;
               }
              if (strcmp(a_name, "+lat_1") == 0)
              {
                 map->p_alp = atof(a_value);
              }
              else if (strcmp(a_name, "+lat_2") == 0)
              {
                 map->p_bet = atof(a_value);
              }
              else if (strcmp(a_name, "+lon_0") == 0)
              {
                 map->p_gam = atof(a_value);
                 map->xcent = atof(a_value);
              }
              else if (strcmp(a_name, "+lat_0") == 0)
              {
                 map->ycent = atof(a_value);
              }
            }

	    /*define Lambert Azimuthal Equal Area in ctype and in html*/
            if (strstr(map->custom_proj_str, "laea") != NULL)
            {
		if (strcmp(a_name, "+proj") == 0)
		{
		  map->ctype = LEQGRD3;  /*define a contant*/
		}
		else if (strcmp(a_name, "+lat_0") == 0)
                {
                  map->p_alp = atof(a_value);
                }
                else if (strcmp(a_name, "+lon_0") == 0)
                {
                  map->p_bet = atof(a_value);               
                }      
                else if (strcmp(a_name, "+x_0") == 0)
                {
                  map->xcent = atof(a_value);
                }
                else if (strcmp(a_name, "+y_0") == 0)
                {
                  map->ycent = atof(a_value);
                }
            }

            /*Sinusoidal projection*/
            if (strstr(map->custom_proj_str, "sinu") != NULL)
            {
                if (strcmp(a_name, "+proj") == 0)
                {
                   map->ctype = SINUGRD3;
                }  
                else if (strcmp(a_name, "+lon_0") == 0)
                {
                  map->p_alp = atof(a_value);
                } 
                else if (strcmp(a_name, "+x_0") == 0)
                {
                  map->xcent = atof(a_value);
                }   
                else if (strcmp(a_name, "+y_0") == 0)
                {
                  map->ycent = atof(a_value);
                }
            }   

	    
          } /* a_name and a_value have data*/ 
	 
      } /*end of i loop*/
      
    } /*custom map not null*/   
 
    if(map->gridname != NULL)
    {
      sprintf(mesg,"%d defined projection in GRIDESC.txt: %s\n",j+1,map->gridname);
      MESG(mesg);
    }
  } /*end of j loop*/
  
    
    /*sprintf(mesg,"1 custom_proj_str=%s   2 custom_proj_str=%s\n",map1->custom_proj_str,map2->custom_proj_str);
    MESG(mesg);
    sprintf(mesg,"1 gridname=%s   2 gridname=%s\n",map1->gridname,map2->gridname);
    MESG(mesg);
    
    sprintf(mesg,"1 ctype=%d   2 ctype=%d\n",map1->ctype,map2->ctype);
    MESG(mesg);
    sprintf(mesg,"1 p_alp=%lf  2 p_alp=%lf\n",map1->p_alp,map2->p_alp);
    MESG(mesg);
    sprintf(mesg,"1 p_bet=%lf  2 p_bet=%lf\n",map1->p_bet,map2->p_bet);
    MESG(mesg);
    sprintf(mesg,"1 p_gam=%lf  2 p_gam=%lf\n",map1->p_gam,map2->p_gam);
    MESG(mesg);
    sprintf(mesg,"1 xcent=%lf  2 xcent=%lf\n",map1->xcent,map2->xcent);
    MESG(mesg);
    sprintf(mesg,"1 ycent=%lf  2 ycent=%lf\n",map1->ycent,map2->ycent);
    sprintf(mesg,"1 xcell=%lf  2 xcell=%lf\n",map1->xcell,map2->xcell);
    MESG(mesg);
    sprintf(mesg,"1 ycell=%lf  2 ycell=%lf\n",map1->ycell,map2->ycell);
    MESG(mesg);*/

    if (map1->ctype != map2->ctype)   return 0;
    if (map1->p_alp != map2->p_alp)   return 0;
    if (map1->p_bet != map2->p_bet)   return 0;
    if (map1->p_gam != map2->p_gam)   return 0;
    if (map1->xcent != map2->xcent)   return 0;
    if (map1->ycent != map2->ycent)   return 0;

    return 1;  /*same projection*/
  }
 
