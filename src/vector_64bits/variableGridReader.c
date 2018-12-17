/* ============================================================= */
/* read a varable grid description and create corresponding polygons 
 * The name of the grid should be the name of a grid in the GRIDDESC
 * file that corresponds to the outermost edge of the modeled domain /
 * coarsest resolution.
 *
 * THIS READER READS AN ASCII DUMP OF THE NETCDF GRID_DOT or GRID_CRO FILE
 * It looks for the variables LAT and LON
 * 
 * Moved into a separate variableGridReader.c file 4/8/2005 BDB
 */
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

static char strTokString[512];
static char *strTokPtr;
static char *nextStrTokPtr;
static char token[512];

char * ioapi_sphere = "+a=6370000.0,+b=6370000.0";

/** custom implementation of strtok since default didn't seem to be working */
int nextVal(char * string, const char *delim, double *valuePtr)
{
   char *delimPtr;
   
   if (string != NULL)
   {    
      /* copy the string for reference */ 
      strcpy(strTokString,string);
      strTokPtr = strTokString;
   }
   else  /* this is not the first call, so advance the pointer */
   {
      if ((nextStrTokPtr == NULL) || (strlen(nextStrTokPtr) == 0))
      {
         valuePtr = NULL;
         return 1;
      }
      strTokPtr = nextStrTokPtr;
   }
   delimPtr = (char *) strstr(strTokPtr,delim);
   if (delimPtr == NULL)
   { 
      /* handle backup delimiter of semicolon */
      delimPtr = (char *) strstr(strTokPtr,";");
      if (delimPtr == NULL)
      {
         valuePtr = NULL;
         return 1;
      }   
   }
   /*else*/
   {
      /* return the string with a terminator where the next delimiter is */
      if (strlen(delimPtr) > 1)
      {
         nextStrTokPtr = &(delimPtr[1]);
         *delimPtr = '\0';
      }
      else
      {
         nextStrTokPtr = NULL;
      }   
   }   
   sscanf(strTokPtr," %lf",valuePtr);
   return 0;   
}

PolyObject *VariableGridReader( 
   char *name, /* file name to read */
   BoundingBox *bbox,  /* bounding box of interest - ignore if NULL */
   MapProjInfo *mpinfo /* the map projection for the grid */ )
{

  PolyObject *poly;
  Shape *shp;
  PolyShape *ps;
  MapProjInfo *map;
  FILE *gridDotFile;
  int nObjects;
  int nv, np;
  int c, r;
  char mesg[256];
  /*char *earth_ellipsoid;*/

  char YVARSTR[10];
  char XVARSTR[10];

  char  *gname;
  /*char   xStr[100];
  char   yStr[100];*/
  char   junkStr1[50];
  char   junkStr2[50];
  char   cname[NAMLEN3+1];
  char   gridDotFileName[256];
  char   tempString[256];
  char   *strPtr;
  int    ctype;
  double p_alp;
  double p_bet;
  double p_gam;
  double xcent;
  double ycent;
  double xorig;
  double yorig;
  double xcell;
  double ycell;
  /*double lastXVal;
  double lastYVal;*/
  double latVal;
  double lonVal;
  double *latVals; /* sorted array of all lat values */
  double *lonVals; /* sorted array of all lon values */
  double *xVals; /* sorted array of all X values */
  double *yVals; /* sorted array of all Y values */
  double cellXCent;
  double cellYCent;
  double totalX; /* total width of grid */
  double totalY; /* total height of grid */
  double maxX; /* rightmost X coordinate of grid */
  double maxY; /* topmost Y coordinate of grid */
  double projx;
  double projy;
  double cminX;  /* computed min X from all transformed points */
  double cminY;
  double cmaxX;
  double cmaxY;
  int    ncols;
  int    nrows;
  int    nthik;
  int    numVarDotCols;
  int    numVarDotRows;
  char   *readResult;
  /*int    scanResult;*/
  int    lineNum;
  /*int    foundAllXs;*/
  int    readingLAT;
  int    readingLON;
  int    currLat;
  int    currLon;
  int    gotVal;
  int    totDotPoints;
  int    ll;
  int    ul;
  int    readingXY = 0;  /* 0 if reading lat/lon, 1 for x/y */
  int    foundX = 0;
  int    foundY = 0;
  FILE   *outGridFile;
  MapProjInfo *latlonMapProj;


  MESG("Reading Variable Grid output from NCDUMP\n");
  map = getNewMap();
  if (map == NULL) {
    sprintf(mesg,"getNewMap error for %s", name);
    goto error;
  }

  //ellipsoid doesn't matter for a grid - the IO/API supports sphere = 6370000.0m only
  if ((map->earth_ellipsoid = (char *) strdup(ioapi_sphere)) == NULL) {
        sprintf(mesg,"Allocation error for map ellipsoid.");
        goto error;
  }
  
  if (getenv(name) == NULL ) {
    sprintf(mesg,"Environment variable \"%s\" not set", name);
    goto error;
  }
  /* read the name of the grid that has the coarsest resolution / bounding
   * box that matches the edge of the domain, and populate the map projection */
  gname = (char *) strdup(getenv(name));
  fflush(stdout);
    
  if(!dscgridc(gname, cname, &ctype, &p_alp, &p_bet, &p_gam,
               &xcent, &ycent, &xorig, &yorig, &xcell, &ycell,
               &ncols, &nrows, &nthik )) {
    sprintf(mesg,"GRIDDESC error for gridname \"%s\"", gname);
    goto error;
  }

  /* this info is for the "coarse grid" - grid boundary info is right in 
   * the GRIDDESC, but not the cell size or ncols / rows */      
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
  map->gridname=(char *) strdup(gname);
  map->ncols = ncols;
  map->nrows = nrows;
  
  /* compute bounding box of grid */
  totalX = ncols * xcell;
  totalY = nrows * ycell;
  maxX = xorig + totalX;
  maxY = yorig + totalY;

  /** Here, we need to start accounting for the variable grid */
  if (getenv(ENVT_GRID_DOT_FILE) !=NULL)
  {
    strcpy(gridDotFileName,(char *) strdup(getenv(ENVT_GRID_DOT_FILE)));
  }
  else  
  {
     sprintf(mesg,
       "The grid dot file name was not specified using the variable %s",
       ENVT_GRID_DOT_FILE);
       goto error;
  }    
  sprintf(mesg,"Opening grid dot file %s\n",gridDotFileName);
  MESG(mesg);
  if ((gridDotFile = fopen(gridDotFileName,"r")) == NULL) {
     sprintf(mesg,"Cannot open grid dot file %s\n",gridDotFileName);
     goto error;
  }
  
  outGridFile = fopen("variableGrid.txt","w");
  
  lineNum = 1;
  numVarDotCols = 0;
  numVarDotRows = 0;  
  /* first read NCOLS, NROWS */
  do
  {
     readResult = fgets(tempString,256,gridDotFile);
     if (readResult == NULL)
     {
       sprintf(mesg, "Error finding NCOLS or NROWS in ASCII grid dot file %s (line %d)",
            gridDotFileName, lineNum);
       goto error;
     }
     else 
     {
        if (( strPtr = (char *) strstr(tempString,"NCOLS = ")) != NULL)
        {
           sscanf(tempString,"%s %s %d", junkStr1, junkStr2, &numVarDotCols);
           /* need to skip to next line before reading anything */
        }           
        else if ((strPtr = (char *) strstr(tempString,"NROWS = "))!= NULL)
        {
           sscanf(tempString,"%s %s %d", junkStr1, junkStr2, &numVarDotRows);
        }
        lineNum++;           
     }
     
  } while ((numVarDotCols == 0) || (numVarDotRows == 0));
  sprintf(mesg,"Variable grid: number of dot cols = %d, number of dot rows = %d\n",
     numVarDotCols, numVarDotRows);
  MESG(mesg);
  
  totDotPoints = numVarDotCols*numVarDotRows;
  latVals = malloc(totDotPoints*sizeof(double));
  lonVals = malloc(totDotPoints*sizeof(double));
  /*if ((latVals <= 0) || (lonVals <= 0)) fixed for AIX 1/11/2005 BB */
  if ((latVals == NULL) || (lonVals == NULL))
  {
     sprintf(mesg,"Could not allocate space for variable grid dot lat and lon points");
     goto error;
  }
  readingLAT = 0;
  readingLON = 0;
  currLat = 0;
  currLon = 0;
  MESG("Reading LAT and LON\n");
  /* next read LON and LAT - need to skip to start of each first */  
  
  strcpy(YVARSTR,"LAT =");
  strcpy(XVARSTR,"LON =");
  if (getenv("READ_XYDOT") != NULL)
  {
    if (strcmp(getenv("READ_XYDOT"),"1") == 0)
    {
       strcpy(YVARSTR,"YDOT =");
       strcpy(XVARSTR,"XDOT =");
       readingXY = 1;
    }
  }      

  do
  {          
     readResult = fgets(tempString,256,gridDotFile);
     if (readResult == NULL)
     {
       if (!foundX)
       {
          sprintf(mesg, "Error finding %s in ASCII grid dot file %s (line %d)",
            XVARSTR, gridDotFileName, lineNum);
       }
       else if (!foundY)
       {
          sprintf(mesg, "Error finding %s in ASCII grid dot file %s (line %d)",
            YVARSTR, gridDotFileName, lineNum);
       }
       else
       {
          sprintf(mesg, "Error reading line %d from ASCII grid dot file %s",
            lineNum, gridDotFileName);
       }     
       goto error;
     }
     else 
     {
        if (readingLAT)
        {
           /* read the first number of the string */
           gotVal = nextVal(tempString, ",", &latVal);
           if (gotVal == 0)
           {
              latVals[currLat] = latVal;
              fprintf(outGridFile,"%lf,",latVal);
              currLat++;
           }
           else
           {
              sprintf(mesg, "Error reading y/latitude value # %d, max=%d\nline=%s",
                 currLat,totDotPoints, tempString);
              WARN(mesg);
              goto error;
           }
           /* passing NULL gets the subsequent tokens from the same string */
           while (( (gotVal = nextVal(NULL, ",", &latVal)) == 0 ) &&
              (currLat < totDotPoints))
           {
              /*if (currLat < 100)
                 fprintf(stderr,"lat %d = %lf\n",currLat,latVal); */              
              latVals[currLat] = latVal;
              fprintf(outGridFile,"%lf,",latVal);
              currLat++;
           }    
           if (currLat == totDotPoints)
           {
              fprintf(stderr,"read %d y/lat vals\n", currLat);
              readingLAT = 0;
           }   
           fprintf(outGridFile,"\n");
        }
        else if (readingLON)
        {
           /* read the first number of the string */
           gotVal = nextVal(tempString, ",", &lonVal);
           if (gotVal == 0)
           {
              lonVals[currLon] = lonVal;
              fprintf(outGridFile,"%lf,",lonVal);
              currLon++;
           }
           else
           {
              sprintf(mesg, "Error reading x/longitude value # %d, max = %d\nline=%s", 
                 currLon,totDotPoints, tempString);
              goto error;
           }
           /* passing NULL gets the subsequent tokens from the same string */
           while (( (gotVal = nextVal(NULL, ",", &lonVal)) == 0 ) &&
              (currLon < totDotPoints))
           {
              /* if (currLon < 100)
                 fprintf(stderr,"lon %d = %lf\n",currLon,lonVal);   */            
              lonVals[currLon] = lonVal;
              fprintf(outGridFile,"%lf,",lonVal);
              currLon++;
           }    
           if (currLon == totDotPoints)
           {
              fprintf(stderr,"read %d x/lon vals\n", currLon);
              readingLON = 0;
           }     
           fprintf(outGridFile,"\n");
        }
        else if ((strPtr = (char *) strstr(tempString,YVARSTR)) != NULL)
        {
           sprintf(mesg,"found %s\n",YVARSTR);        
           MESG(mesg);
           readingLAT = 1;
           foundY = 1;
           fprintf(outGridFile,"start %s\n",YVARSTR);
           readingLON = 0;
           /* need to skip to next line before reading anything */
        }           
        else if ((strPtr = (char *) strstr(tempString,XVARSTR)) != NULL)
        {
           sprintf(mesg,"found %s\n",XVARSTR);        
           MESG(mesg);
           fprintf(outGridFile,"start %s\n",XVARSTR);
           foundX = 1;
           readingLON = 1;
           readingLAT = 0;
        }
        lineNum++;           
     }
     
  } while ((readResult != NULL) && 
           ((currLon < totDotPoints) || ( currLat < totDotPoints)));
  fclose(gridDotFile);
  map->ncols = numVarDotCols-1;  /* subtract 1 because we want # cells, not lines */
  map->nrows = numVarDotRows-1;
   
  MESG("Done reading Variable Grid\n");
  
  /* now that we've read in the grid points, we need to create the grid */

  /* first, convert to x and y from lat and lon */
  xVals = (double *)malloc(totDotPoints*sizeof(double));
  yVals = (double *)malloc(totDotPoints*sizeof(double));
  

  latlonMapProj = getFullMapProjection(ioapi_sphere, "+proj=latlong");
  storeProjection(latlonMapProj, map);
  
  cminX = 1E20;
  cminY = 1E20;
  cmaxX = -1E20;
  cmaxY = -1E20;
  fprintf(outGridFile,"X and Y\n");
  for (r = 0; r < totDotPoints; r++)
  {
    if (!readingXY)
    {
       /* convert lat lon to projected coordinates */
       projectPoint ( lonVals[r], latVals[r], &projx, &projy);    
       fprintf(outGridFile,"%lf %lf %lf %lf\n",lonVals[r], projx, projy, latVals[r]);
    }
    else  /* we just want to adjust the origin, we don't need to project */
    {
       projx = lonVals[r]+xorig;
       projy = latVals[r]+yorig;
       fprintf(outGridFile,"%lf %lf %lf %lf\n",lonVals[r], projx, projy, latVals[r]);
    }   
    xVals[r] = projx;
    yVals[r] = projy;  
    cminX = MIN(cminX, projx);
    cminY = MIN(cminY, projy);
    cmaxX = MAX(cmaxX, projx);
    cmaxY = MAX(cmaxY, projy);
  }
  fprintf(stderr,"check of BBox (minx, miny, maxx, maxy) = %lf, %lf, %lf, %lf\n",
     cminX, cminY, cmaxX, cmaxY);
  fprintf(outGridFile,"BBOX (minx, maxx, miny, maxy) = %lf, %lf, %lf, %lf\n",
     cminX, cmaxX, cminY, cmaxY);
  
  nObjects = (numVarDotCols - 1) * (numVarDotRows - 1);
  sprintf(mesg,"num grid cells = %d\n",nObjects);
  MESG(mesg);
  poly = getNewPoly(nObjects);
  if (poly == NULL) {
    sprintf(mesg,"Cannot allocate POLY structure for variable grid");
    goto error;
  }
  poly->nSHPType = SHPT_POLYGON;
  poly->map = map;

  /* overall bbox is the same as for coarse grid */
  poly->bb = newBBox(xorig, yorig, maxX, maxY);
  printBoundingBox(poly->bb);
  nv = 4;  /* num vertices in shape (square grid cell) */
  np = 1;  /* num parts of shape */

  MESG("computing cell coordinates \n");
  fprintf(outGridFile,"GRID CELLS\n");
  
  /* r and c are for grid cells (one less than dot rows/cols), 
   * plus indexing on low size, so go to num dot rows and cols - 2 */
  for (r = 0; r < numVarDotRows-1; r++) {
    for (c = 0; c < numVarDotCols-1; c++) {
      ll = r*numVarDotCols+c;
      ul = (r+1)*numVarDotCols+c;
      ps = getNewPolyShape(np);
      ps->num_contours = 0; 
      shp = getNewShape(nv);     
      shp->vertex[0].x = xVals[ll];   /* lower left cell corner */
      shp->vertex[0].y = yVals[ll];
      shp->vertex[1].x = xVals[ul];  /* upper left corner */
      shp->vertex[1].y = yVals[ul];
      shp->vertex[2].x = xVals[ul+1];  /* upper right corner */
      shp->vertex[2].y = yVals[ul+1];
      shp->vertex[3].x = xVals[ll+1]; /* lower right corner */
      shp->vertex[3].y = yVals[ll+1];
      fprintf(outGridFile,"%d %d %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n",
         r+1, c+1, shp->vertex[0].x, shp->vertex[0].y,
         shp->vertex[1].x, shp->vertex[1].y, shp->vertex[2].x, shp->vertex[2].y,
         shp->vertex[3].x, shp->vertex[3].y);

      gpc_add_contour(ps, shp, NOT_A_HOLE);
      computeCenterOfBox(ps, &cellXCent, &cellYCent);
      polyShapeIncl(&(poly->plist), ps, NULL);
    }
  }  
  fclose(outGridFile);
  if (xVals) free(xVals);
  if (yVals) free(yVals);
  if (latVals) free(latVals);
  if (lonVals) free(lonVals);
  return poly;

 error:
  WARN(mesg);
  if (xVals) free(xVals);
  if (yVals) free(yVals);
  if (latVals) free(latVals);
  if (lonVals) free(lonVals);
  return NULL; /* we only get here if there was an error */
}

/* ============================================================= */
/* read a varable grid description and create corresponding polygons 
 * The name of the grid should be the name of a grid in the GRIDDESC
 * file that corresponds to the outermost edge of the modeled domain /
 * coarsest resolution.
 *
 * THIS READER READS DOT CELL POINTS FROM FILES WITH THE FORMAT:
   0.000000E+00  0.000000E+00
   0.360000E+02  0.000000E+00
   0.720000E+02  0.000000E+00
   0.108000E+03  0.000000E+00
   0.144000E+03  0.000000E+00
   0.180000E+03  0.000000E+00
   0.180000E+04  0.000000E+00
   ...
   0.000000E+00  0.360000E+02
   0.360000E+02  0.360000E+02
   0.720000E+02  0.360000E+02
   0.108000E+03  0.360000E+02
   0.144000E+03  0.360000E+02
 */
PolyObject *VariableGridReader1( 
   char *name, /* file name to read */
   BoundingBox *bbox,  /* bounding box of interest - ignore if NULL */ 
   MapProjInfo *mpinfo)
{

  PolyObject *poly;
  Shape *shp;
  PolyShape *ps;
  MapProjInfo *map;
  FILE *gridDotFile;
  int nObjects;
  int nv, np;
  int c, r;
  char mesg[256];
  /*char *earth_ellipsoid;*/

  char  *gname;
  char   xStr[100];
  char   yStr[100];
  char   cname[NAMLEN3+1];
  char   gridDotFileName[256];
  char   tempString[256];
  int    ctype;
  double p_alp;
  double p_bet;
  double p_gam;
  double xcent;
  double ycent;
  double xorig;
  double yorig;
  double xcell;
  double ycell;
  double lastXVal;
  double lastYVal;
  double xVal;
  double yVal;
  double *xVals; /* sorted array of all X values */
  double *yVals; /* sorted array of all Y values */
  double cellXCent;
  double cellYCent;
  double totalX; /* total width of grid */
  double totalY; /* total height of grid */
  double maxX; /* rightmost X coordinate of grid */
  double maxY; /* topmost Y coordinate of grid */
  int    ncols;
  int    nrows;
  int    nthik;
  int    numVarCols;
  int    numVarRows;
  char   *readResult;
  int    scanResult;
  int    lineNum;
  int    foundAllXs;
  FILE   *outGridFile;


  MESG("Reading Variable Grid\n");
  map = getNewMap();
  if (map == NULL) {
    sprintf(mesg,"getNewMap error for %s", name);
    goto error;
  }

  //ellipsoid doesn't matter for a grid - the IO/API supports sphere = 6370000.0m only
  if ((map->earth_ellipsoid = (char *) strdup(ioapi_sphere)) == NULL) {
        sprintf(mesg,"Allocation error for map ellipsoid.");
        goto error;
  }

  if (getenv(name) == NULL ) {
    sprintf(mesg,"Environment variable \"%s\" not set", name);
    goto error;
  }
  /* read the name of the grid that has the coarsest resoluation / bounding
   * box that matches the edge of the domain, and populate the map projection */
  gname = (char *) strdup(getenv(name));
  fflush(stdout);
    
  if(!dscgridc(gname, cname, &ctype, &p_alp, &p_bet, &p_gam,
               &xcent, &ycent, &xorig, &yorig, &xcell, &ycell,
               &ncols, &nrows, &nthik )) {
    sprintf(mesg,"GRIDDESC error for gridname \"%s\"", gname);
    goto error;
  }

  /* this info is for the "coarse grid" - grid boundary info is right, not 
   * cell size or ncols / rows */      
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
  map->gridname=(char *) strdup(gname);
  map->ncols = ncols;
  map->nrows = nrows;
  
  /* compute bounding box of grid */
  totalX = ncols * xcell;
  totalY = nrows * ycell;
  maxX = xorig + totalX;
  maxY = yorig + totalY;

  /** Here, we need to start accounting for the variable grid */
  if (getenv(ENVT_GRID_DOT_FILE) !=NULL)
  {
    strcpy(gridDotFileName,(char *) strdup(getenv(ENVT_GRID_DOT_FILE)));
  }
  else  
  {
     sprintf(mesg,
       "The grid dot file name was not specified using the variable %s",
       ENVT_GRID_DOT_FILE);
       goto error;
  }    
  sprintf(mesg,"Opening grid dot file %s\n",gridDotFileName);
  MESG(mesg);
  if ((gridDotFile = fopen(gridDotFileName,"r")) == NULL) {
     WARN2("Cannot open grid dot info file ", gridDotFileName);
     goto error;
  }
  /* assume resolution won't be greater than 5x coarse grid */
  xVals = (double *)malloc(5*ncols*sizeof(double));
  yVals = (double *)malloc(5*nrows*sizeof(double));
  /* if ((xVals <= 0) || (yVals <= 0)) fixed for AIX 1/11/2005 BB */
  if ((xVals == NULL) || (yVals == NULL))
  {
     sprintf(mesg,"Could not allocate space for variable grid dot points");
     goto error;
  }
  xVal = -1.0;
  yVal = -1.0;
  lastXVal = -1.0;
  lastYVal = -1.0;
  lineNum = 1;
  numVarCols = 0;
  numVarRows = 0;
  foundAllXs = 0;
  do
  {
     readResult = fgets(tempString,256,gridDotFile);
     if (readResult == NULL)
     {
        if (lineNum == 1)
        {
          sprintf(mesg, "Failed to read first line from gridDotFile %s",
            gridDotFileName);
          WARN(mesg);
          goto error;
        }
     }
     else if (*readResult == '#')  /* comment line */
     {
        lineNum++;   
     }
     else /* try to read values */
     {
        lastXVal = xVal;
        lastYVal = yVal;                         
        scanResult = sscanf(tempString,"%s %s", xStr, yStr);
        xVal = strtod(xStr, NULL);
        yVal = strtod(yStr, NULL);
        if (scanResult != 2)
        {
          sprintf(mesg,"Could not read x and y values on line %d",lineNum);
          WARN(mesg);
        }
        if (!foundAllXs && (xVal != lastXVal))
        {
           if (xVal == xVals[0])
           {
              foundAllXs = 1;   /** don't need to keep looking for Xs */
           }
           else
           {
              xVals[numVarCols] = xVal;
              numVarCols++;
           }   
        }
        if (yVal != lastYVal)
        {
           yVals[numVarRows] = yVal;
           numVarRows++;
        }
        lineNum++;
     }   
     
  } while (readResult != NULL);
  fclose(gridDotFile);
   
  
  sprintf(mesg,"Variable grid: number of dot X = %d, number of dot Y = %d\n",
     numVarCols, numVarRows);
  MESG(mesg);
  
  /* update map projection for variable grid */
  /*map->ncols = numVarCols;
  map->nrows = numVarRows;
  map->xcell = 0;
  map->ycell = 0;*/
  
  nObjects = (numVarCols - 1)* (numVarRows - 1);
  sprintf(mesg,"numCells = %d\n",nObjects);
  MESG(mesg);
  poly = getNewPoly(nObjects);
  if (poly == NULL) {
    sprintf(mesg,"Cannot allocate POLY structure for variable grid");
    fprintf(stderr,"couldn't alloc poly");
    goto error;
  }
  MESG("set shape type\n");
  poly->nSHPType = SHPT_POLYGON;
  MESG("set map\n");
  poly->map = map;

  MESG("set bbox");

  /* overall bbox is the same as for coarse grid */
  poly->bb = newBBox(xorig, yorig, maxX, maxY);
  printBoundingBox(poly->bb);
  nv = 4;  /* num vertices in shape (square grid cell) */
  np = 1;  /* num parts of shape */
  MESG("computing cell coordinates \n");
  outGridFile = fopen("variableGrid.txt","w");
  
  /* when the reader is updated, instead of computing cells, we'll build
   * individual cells */
  for (r = 0; r < numVarRows-1; r++) {
    for (c = 0; c < numVarCols-1; c++) {
      ps = getNewPolyShape(np);
      ps->num_contours = 0; 
      shp = getNewShape(nv);     
      /* multiply by 1000 to convert from km to meters, add origin to 
       * get into correct coordinate space */ 
      shp->vertex[0].x = (xVals[c]*1000)+xorig;   /* lower left cell corner */
      shp->vertex[0].y = (yVals[r]*1000)+yorig;
      shp->vertex[1].x = (xVals[c]*1000)+xorig;  /* upper left corner */
      shp->vertex[1].y = (yVals[r+1]*1000)+yorig;
      shp->vertex[2].x = (xVals[c+1]*1000)+xorig;  /* upper right corner */
      shp->vertex[2].y = (yVals[r+1]*1000)+yorig;
      shp->vertex[3].x = (xVals[c+1]*1000)+xorig; /* lower right corner */
      shp->vertex[3].y = (yVals[r]*1000)+yorig;
      fprintf(outGridFile,"%d %d %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n",
         r, c, shp->vertex[0].x, shp->vertex[0].y, shp->vertex[1].x, 
         shp->vertex[1].y, shp->vertex[2].x, shp->vertex[2].y,
         shp->vertex[3].x, shp->vertex[3].y);

      gpc_add_contour(ps, shp, NOT_A_HOLE);
      computeCenterOfBox(ps, &cellXCent, &cellYCent);
      /*if (r < 2)
      {
        fprintf(stderr,"%d, %d, y cell cent = %.2f, x cell cent = %.2f\n",
          r+1, c+1, cellYCent, cellXCent);
      }*/
      polyShapeIncl(&(poly->plist), ps, NULL);
    }
  }  
  fclose(outGridFile);
  return poly;

 error:
  WARN(mesg);
  return NULL; /* we should never get here */
}

/* not used in actual execution - only for testing */
void testHoleGridCell()
{      
        PolyObject *poly;
        Shape *shp;
        PolyShape *ps;
        int nv, np;
        int c, r;
        double xorig;
        double yorig;
        double xcell;
        double ycell;

         WARN("Special grid cell being generated");
         np = 2;
         ps = getNewPolyShape(np);
         ps->num_contours = 0; /* we need to reset,
                                  since gpc_add_contour increments num_contours */
         /* add regular grid cell shape */                                  
         shp = getNewShape(nv);      
         shp->vertex[0].x = xorig + xcell*c;
         shp->vertex[0].y = yorig + ycell*r;
         shp->vertex[1].x = shp->vertex[0].x;
         shp->vertex[1].y = shp->vertex[0].y + ycell;
         shp->vertex[2].x = shp->vertex[1].x + xcell;
         shp->vertex[2].y = shp->vertex[1].y;
         shp->vertex[3].x = shp->vertex[2].x;
         shp->vertex[3].y = shp->vertex[0].y;

         gpc_add_contour(ps, shp, NOT_A_HOLE);
         polyShapeIncl(&(poly->plist), ps, NULL);

         /* add a second smaller grid cell on top */         
         shp = getNewShape(nv);      
         shp->vertex[0].x = xorig + xcell*c;
         shp->vertex[0].y = yorig + ycell*r;
         shp->vertex[1].x = shp->vertex[0].x;
         shp->vertex[1].y = shp->vertex[0].y + ycell/2;
         shp->vertex[2].x = shp->vertex[1].x + xcell/2;
         shp->vertex[2].y = shp->vertex[1].y;
         shp->vertex[3].x = shp->vertex[2].x;
         shp->vertex[3].y = shp->vertex[0].y;

         gpc_add_contour(ps, shp, NOT_A_HOLE);
         polyShapeIncl(&(poly->plist), ps, NULL);

         shp = getNewShape(nv);      
         shp->vertex[0].x = xorig + xcell*c + xcell;
         shp->vertex[0].y = yorig + ycell*r + ycell;
         shp->vertex[1].x = shp->vertex[0].x-xcell/2;
         shp->vertex[1].y = shp->vertex[0].y;
         shp->vertex[2].x = shp->vertex[1].x;
         shp->vertex[2].y = shp->vertex[1].y-ycell/2;
         shp->vertex[3].x = shp->vertex[0].x;
         shp->vertex[3].y = shp->vertex[2].y;

         gpc_add_contour(ps, shp, IS_A_HOLE);
         polyShapeIncl(&(poly->plist), ps, NULL);

}      


