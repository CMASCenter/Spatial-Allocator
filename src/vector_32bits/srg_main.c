/* This program produces spatial surrogates for emission processing (e.g. with 
 * SMOKE) 
 *
 * Written by Atanas Trayanov, MCNC Environmental Modeling Center.
 * Modified by Alison Eyth, Carolina Environmental Program, UNC-Chapel Hill
 * in support of the EPA Multimedia Integrated Modeling System, 2002-2003.
 * Further modified and split into separate applications by Ben Brunk, 
 * Carolina Environmental Program, UNC-CH, 2005.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "shapefil.h"
#include "mims_spatl.h"
#include "mims_evs.h"
#include "parms3.h"
#include "io.h"


char *prog_name;

char *prog_version = "Surrogate Creator Version 4.3, 01/20/2017\n";
int maxShapes = 0;
int fileCompleted;


/* ===================================================== */
/* ===================================================== */
/* ===================================================== */

int main(int argc, char *argv[])
{
    PolyObject *p_data;
    PolyObject *p_weight;
    PolyObject *p_grid;
    PolyObject *p_wd = NULL;
    PolyObject *p_wdg = NULL;
    int no_weight_attr = 0;
    int no_weight_poly = 0;
    int result;
    BoundingBox *outputBbox = NULL;
    MapProjInfo *outMapProj;  
    MapProjInfo *dataMapProj;    
    MapProjInfo *weightMapProj;
    char mesg[256];
    char outfile[256];
    char filterFile[256];
    char weightFile[256];
    char debugOutput[10];
    char tempString[100];

    extern int debug_output;


    prog_name = argv[0];
    if(getEnvtValue(ENVT_DEBUG_OUTPUT, debugOutput))
    {
        if(!strcmp(debugOutput, "Y"))
        {
            debug_output = 1;
        }
        else if(!strcmp(debugOutput, "N"))
        {
            debug_output = 0;
        }
        else
        {
            sprintf(mesg, "%s can only be set to 'Y' or 'N'",
                    ENVT_DEBUG_OUTPUT);
            ERROR(prog_name, mesg, 2);
        }
    }
    else
    {
        sprintf(mesg, "Please specify a %s of 'Y' or 'N' in your script",
                ENVT_DEBUG_OUTPUT);
        ERROR(prog_name, mesg, 2);
    }

    MESG(prog_version);
    
    if(getEnvtValue(ENVT_OUTPUT_FILE_TYPE, tempString))
    {
        if(strcmp(tempString, "RegularGrid")==0 || strcmp(tempString, "EGrid")==0)
        {
            MESG("Setting output grid\n");
            p_grid = PolyReader(ENVT_OUTPUT_GRID_NAME, ENVT_OUTPUT_FILE_TYPE, NULL, NULL, NULL);
        }
	
	if(strcmp(tempString, "Polygon")==0)
	{
           MESG("Setting output polygon\n");
	   outMapProj = getFullMapProjection(ENVT_OUTPUT_ELLIPSOID, ENVT_OUTPUT_MAP_PROJ);
           p_grid = PolyReader(ENVT_OUTPUT_POLY_FILE, ENVT_OUTPUT_FILE_TYPE,outMapProj, NULL, NULL);
	   /* associate the attributes with the output polygons */
           if (!attachAttribute(p_grid, ENVT_OUTPUT_POLY_ATTR, NULL))
	   {
	     ERROR(prog_name, "Attaching output polygon attribute error", 2);
	   }
           MESG("\nFinished attaching ATTR to output Shapefile ");
	}
    }
    
    if(!p_grid)
    {
        ERROR(prog_name, "Error reading output grid/polygon data", 2);
    }
    outputBbox = p_grid->bb;

#ifdef DEBUG
    printPoly(p_grid);
#endif
    /* the following lines were causing the duplicate header printing */
	/* for some reason, argc is 3 on Windows when -header used */
    if(argc >= 2)
    {
        /* print the header if the user wants to */
        if(!strncmp(argv[1], "-header", 7))
        {
            print_hdr(stdout, p_grid);
            exit(0);
        }
	else if ((argc > 2) && (!strncmp(argv[2], "-header", 7)))
	{
  	    fprintf(stderr,"argv[2]=%s\n",argv[2]);
            print_hdr(stdout, p_grid);
            exit(0);
	}
    }

    /* this is to test some speical cases */
#ifdef DEBUG
    p_test = TestPolyReader(NULL, outputBbox, p_grid->map);
    printPoly(p_test);
#endif

    getEnvtValue(ENVT_OUTPUT_FILE_NAME, outfile);

    if(outfile == NULL)
    {
        sprintf(mesg, "Please set %s to the desired name of the output file\n",
                ENVT_OUTPUT_FILE_NAME);
        ERROR(prog_name, mesg, 2);
    }

    if(getEnvtValue(ENVT_WEIGHT_ATTR_LIST, tempString))
    {
        if(!strcmp(tempString, "NONE"))
        {
            no_weight_attr = 1;
        }
    }


    /* retrieve the name of the filter file */
    getEnvtValue(ENVT_FILTER_FILE, filterFile);

    if(strlen(filterFile)==0)
    {
        strcpy(filterFile,"NONE");
    }
    MESG2("Filter File=",filterFile);

    MESG("Reading data polygons\n");
    /* read in the data polygons and convert to grid's mapprojection */
    dataMapProj = getFullMapProjection(ENVT_DATA_ELLIPSOID, ENVT_DATA_MAP_PROJ);
    p_data = PolyReader(ENVT_DATA_FILE_NAME, ENVT_DATA_FILE_NAME_TYPE,
                        dataMapProj, outputBbox, p_grid->map);
/*     printf("2. p_grid->nSHPType=%d\n", p_grid->nSHPType); */
    if(!p_data)
    {
        ERROR(prog_name,
              "Error reading poly-data file, or no intersection with output grid exists",
              2);
    }

    /* associate the data attributes with the data polygons */
    if (!attachAttribute(p_data, ENVT_DATA_ID_ATTR, NULL))
    {
	ERROR(prog_name, "Attaching base data polygon attribute error", 2);
    }
    

    /*process multiple shapes with the same ID into one record*/
    if (PolyMShapeInOne(p_data) != 0)
    {
	ERROR(prog_name, "Processing multiple shapes with the same ID into one record failed", 2);
    }
    
#ifdef DEBUG
    printPoly(p_data);
#endif
    /* read in the weight polygons */
    MESG("Reading weight points/lines/polygons\n");
    weightMapProj =
        getFullMapProjection(ENVT_WEIGHT_FILE_ELLIPSOID, 
                             ENVT_WEIGHT_FILE_MAP_PROJ);

    /* determine whether to filter the weight polygons first */
    if(strcmp(filterFile, "NONE")!=0)
    {
        getEnvtValue(ENVT_WEIGHT_FILE_NAME, weightFile);

        sprintf(mesg, "filterFile = '%s'\n", filterFile);
        MESG(mesg);

        if(!getEnvtValue(ENVT_FILTERED_WEIGHT_SHAPES, outfile))
        {
            sprintf(mesg, "To filter the weight file, you must also set %s",
                    ENVT_FILTERED_WEIGHT_SHAPES);
            ERROR(prog_name, mesg, 2);
        }

        /* filter the weight polygons and write to a temp file */
        filterDBF(weightFile, filterFile, outfile);

        p_weight =
            PolyReader(ENVT_FILTERED_WEIGHT_SHAPES, ENVT_WEIGHT_FILE_TYPE,
                       weightMapProj, p_data->bb, p_grid->map);
    }
    else /* no filter is being used */
    {
        /* for the weight polygons, we need to keep all objects that lie in all counties
         * that intersect the grid. This means that we need to give the reader the 
         * bbox that includes the entirety of all the counties, not just the grid bbox */
        p_weight = PolyReader(ENVT_WEIGHT_FILE_NAME, ENVT_WEIGHT_FILE_TYPE,
                              weightMapProj, p_data->bb, p_grid->map);
    }
/*     printf("3. p_grid->nSHPType=%d\n", p_grid->nSHPType); */
    if(!p_weight)
    {
        ERROR(prog_name,
              "Error reading weight file, or no intersection with output grid exists",
              2);
    }
    /* if no weight polygons are available, area will be used */
    if(p_weight->nObjects == 0)
    {
        MESG("No weight polygons available - use AREA\n");
        no_weight_poly = 1;
        CopyPoly(p_weight, p_data);
        p_weight->attr_hdr = NULL;
        p_weight->attr_val = NULL;
    }
    /* associate the weight attributes with the weight polygons */
/*     printf("3.25 p_grid->nSHPType=%d\n", p_grid->nSHPType); */
    if(!attachAttribute(p_weight, ENVT_WEIGHT_ATTR_LIST, ENVT_SURROGATE_ID))
    {
        ERROR(prog_name, "Attaching weight polygon attribute error", 2);
    }
    
/*     printf("3.3 p_grid->nSHPType=%d\n", p_grid->nSHPType); */
#ifdef DEBUG
    printPoly(p_weight);
#endif

    p_wd = getNewPoly(0);
    p_wdg = getNewPoly(0);
/*     printf("3.4 p_grid->nSHPType=%d\n", p_grid->nSHPType); */

    if(!p_wdg || !p_wd)
    {
        WARN("Allocation error in getNewPoly");
        return 1;
    }
    if(no_weight_poly)
    {
        if(!CopyPolySelf(p_weight, p_data, p_wd))
        {
            WARN("Error in CopyPolySelf called from main program");
            return 1;
        }
    }
    else
    {
/*         printf("3.5. p_grid->nSHPType=%d\n", p_grid->nSHPType); */
        /* decide whether to make DW intersection or to re-use existing one */
        if(!reuseDW(p_weight, p_data, &p_wd, ENVT_USE_DW_FILE))
        {
            /* intersect the weight and data polygons */
            MESG("Intersecting weight objects with data polygons\n");
/*             printf("4. p_grid->nSHPType=%d\n", p_grid->nSHPType); */
            result = polyIsect(p_weight, p_data, p_wd, FALSE);
            if(result < 0)
            {
                WARN("Error intersecting weight and data polygons");
                return 1;
            }
            else if(result == 0)
            {
                WARN("Weight and data polygons do not intersect");
                return 1;
            }
            /* save the intersected polygons if requested */
            try2saveDW(p_wd, ENVT_SAVE_DW_FILE);
        }
    }
#ifdef DEBUG
    printPoly(p_wd);
#endif
    /* compute the intersection of the weight and data polygons with 
     * the grid cells */
    MESG("Intersecting weight-data objects with grid polygons\n");
/*     printf("5. p_grid->nSHPType=%d\n", p_grid->nSHPType); */
    result = polyIsect(p_wd, p_grid, p_wdg, FALSE);
    if(result < 0)
    {
        WARN("Error intersecting weight+data and grid polygons");
        return 1;
    }
    else if(result == 0)
    {
        WARN("Weight+data polygons do not overlap the output grid");
        return 1;
    }
#ifdef DEBUG
    printPoly(p_wdg);
#endif
    /* write the surrogate file */
    MESG("\nComputing surrogates\n");

    /* see if user wants to output surrogate numerators in shape file */
    if(!getEnvtValue(ENVT_OUTPUT_FILE_NAME, outfile))
    {
        if(!strcmp(outfile, "NONE"))
        {
            strcpy(outfile, "");
        }
    }

    if(!reportSurrogate(p_wdg, ENVT_SURROGATE_FILE, !no_weight_attr, outfile))
    {
        ERROR(prog_name, "Error generating surrogates", 2);
    }

    MESG2("NORMAL COMPLETION of ", prog_name);
    MESG("");
    return 0;
}                               /* end of main */

