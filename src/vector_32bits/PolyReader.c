/**********************************************************************************
 *  PolyReader.c
 *
 * read a file and return the corresponding polygons 
 * char * ename is the file name 
 * char * ecat is the environment variable name from which to get the file type 
 * MapProjInfo *file_mapproj is map projection for the file 
 * BoundingBox bbox is the bounding box of interest - if NULL, this will be ignored 
 * MapProjInfo *bbox_proj is the map projection of the bounding box / grid 
 *
 *  Updated:  April 2005, added suport for BoundingBox, PolygonFile, PointFile and 
 *            IoapiFile  --BDB
 *            March 2006, added support for EGrid  -- LR
 *            Dec. 2007, updated for PROj.4 datum transformation -- LR
 *
 ***********************************************************************************/
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <math.h>
#include "mims_spatl.h"
#include "mims_evs.h"
#include "shapefil.h"
#include "io.h"


/* new types added 4/2005 BB */
/* new types handled are BoundingBox, PolygonFile, PointFile, IoapiFile */
 

PolyObject *PolyReader(char *ename, char *ecat, MapProjInfo *file_mapproj, 
                           BoundingBox *bbox, MapProjInfo *bbox_proj)
{

  char type[50],otype[50],itype[50];
  PolyObject *poly=NULL;
  char mesg[256], gridname[50];
  extern char* prog_name;
  bool useBBoptim = FALSE;


  if(!getEnvtValue(ecat, type))
  {
     sprintf(mesg,"%s not set\n", ecat);
     ERROR(prog_name, mesg, 2);
  }

  if(type[0] != '\0')
  {
    if (!strcmp(type,"RegularGrid")) 
    {
        if(!strcmp(ecat, ENVT_OVERLAY_TYPE))
        {
            useBBoptim = TRUE;

            /* set to optimized bbox instead of grid */
            /* Basically, we want to use the bbox optimization if the input
             * file type is a polygon, but not if it's points.  This hack will
             * not fix the case when the input file is a point Shapefile.
             * That is still TBD. */
            if(getEnvtValue(ENVT_INPUT_FILE_TYPE, itype))
            {
               if (strcmp(itype,"PointFile")==0)
               {
                 useBBoptim = FALSE;
               }
            }
        }
        else
        {
            useBBoptim = FALSE;
        }
        poly = RegularGridReader(ename, bbox, bbox_proj, useBBoptim);
    }
    else if (!strcmp(type,"EGrid"))
    {
        poly = EGridReader(ename, ENVT_OUTPUT_POLY_FILE, bbox, bbox_proj, useBBoptim);
    }
    else if (!strcmp(type,"VariableGrid")) 
    {
        poly = VariableGridReader(ename, bbox , bbox_proj);
    }
    else if(!strcmp(type,"ShapeFile") || strcmp(type, "Polygon")==0 || !strcmp(type, "Shapefile")) 
    {
        /* this reads the shape file and converts to bbox_proj map projection */
        poly = PolyShapeReader(ename, file_mapproj, bbox, bbox_proj);

#ifdef NO_PROJECT_ON_READ

        if(mimsProject(poly, bbox_proj) != 0) 
        {
           sprintf(mesg,"Error projecting file from envt. var. %s", ename); 
           ERROR(prog_name,mesg,2);
        }
#endif
    }
    else if(!strcmp(type,"FractionalVegetationFile")) 
    {
      poly = FractionalVegetationReader(ename, file_mapproj, bbox, bbox_proj);
      
      /* convert the polygons to the grid's map projection */
      if (mimsProject(poly, bbox_proj) != 0) 
      {
            sprintf(mesg,"Error projecting file from envt. var. %s", ename);
            ERROR(prog_name,mesg,2);
      }

    }
    else if(!strcmp(type,"BoundingBox")) 
    {
       poly = BoundingBoxReader(file_mapproj, bbox_proj);
    }
    else if(!strcmp(type,"PolygonFile"))
    {
       poly = PolygonFileReader(ename, file_mapproj, bbox_proj);
    }
    else if(!strcmp(type,"PointFile"))
    {
       poly = PointFileReader(ename, file_mapproj, bbox_proj);
    }
    else if(!strcmp(type,"IoapiFile"))
    {
       if(!getEnvtValue(ENVT_OVERLAY_TYPE, otype))
       {
         if(!getEnvtValue(ENVT_OUTPUT_FILE_TYPE, otype))
         {
            sprintf(mesg,"%s and %s not set\n", ENVT_OVERLAY_TYPE,ENVT_OUTPUT_FILE_TYPE);
            ERROR(prog_name, mesg, 2);   
          }
       }

#ifdef USE_IOAPI
        if(!strcmp(ecat, ENVT_OVERLAY_TYPE))
        {
            /* set to optimized bbox instead of grid */
            useBBoptim = TRUE;
        }
        else
        {
            useBBoptim = FALSE;
        }

        if(!strcmp(ecat, ENVT_OUTPUT_FILE_TYPE) && (!strcmp(otype,"RegularGrid") || !strcmp(otype,"IoapiFile")))
        {
           /*for output grid ioapi file*/
            poly = RegularGridReader(ename, bbox, bbox_proj, useBBoptim);
        }
        else if (!strcmp(ecat, ENVT_OUTPUT_FILE_TYPE) && !strcmp(otype,"EGrid"))
        {
            poly = EGridReader(ename, ENVT_OUTPUT_POLY_FILE, bbox, bbox_proj, useBBoptim);
        }
        else
        {
            /*for input ioapi file*/
            //poly = IoapiInputReader(ename, bbox, bbox_proj, useBBoptim);
            poly = IoapiInputReader(ename, file_mapproj, bbox_proj, useBBoptim);
        }
#else
        sprintf(mesg, "I/O API files not supported\n");
        ERROR(prog_name, mesg, 2);
#endif
    }
    else 
    {
      sprintf(mesg, "Unknown poly reader type %s", type);
      ERROR("PolyReader", mesg, 2);
    }
  }
  else 
  {
    sprintf(mesg, "Poly reader file type not set: %s", ecat);
    ERROR("PolyReader", mesg, 2);
  }

  return poly;
}


/*void getMapProjFromGriddesc(char *gname, MapProjInfo *map)
{
    char *cname = NULL;
    int nthik;
    extern char *prog_name;
    char mesg[256];
                                                                                                    
    if(!dscgridc(gname, cname, &map->ctype, &map->p_alp, &map->p_bet, &map->p_gam,
               &map->xcent, &map->ycent, &map->xorig, &map->yorig, &map->xcell, &map->ycell,
               &map->ncols, &map->nrows, &nthik ))
    {
          sprintf(mesg,"Error in GRIDDESC file for gridname \"%s\"", gname);
          ERROR(prog_name, mesg, 2);
    }
}*/
