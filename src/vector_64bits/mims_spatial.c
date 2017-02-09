/* This program performs spatial allocation for which the attributes are either 
 * summed e.g. population) or averaged (e.g. population density).
 * 
 * Written by Atanas Trayanov, MCNC Environmental Modeling Center.
 * Modified by Alison Eyth, Carolina Environmental Program, UNC-Chapel Hill
 * in support of the EPA Multimedia Integrated Modeling System, 2002-2003.
 * Further modified and split into separate applications by Ben Brunk, 
 * Carolina Environmental Program, UNC-CH, 2005.
 *
 * Updated: March 2006, added support for EGrid -- LR
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

int fileCompleted;              /* a global variable that gets set elsewhere */
int maxShapes = 0;
char *prog_name;

char *prog_version = "Spatial Allocator Version 3.6, 03/10/2009\n";


/* ===================================================== */
/* ===================================================== */
/* ===================================================== */

int main(int argc, char *argv[])
{
    PolyObject *p_data;
    PolyObject *p_test;
    PolyObject *p_grid;
    PolyObject *p_overlay;
    PolyObject *tmp_overlay;
    PolyObject *p_input;
    PolyObject *p_result;
    PolyObject *p_wd = NULL;
    PolyObject *p_wdg = NULL;
    int no_weight_attr = 0;
    int no_weight_poly = 0;
    int result;
    JobType type;
    BoundingBox *outputBbox = NULL;
    MapProjInfo *dataMapProj;
    MapProjInfo *outputMapProj;
    MapProjInfo *inputMapProj;
    MapProjInfo *overlayMapProj;
    char mesg[256];
    char outfile[256], oFileTest[256];
    char filterFile[256];
    FILE *oTest;

    extern int debug_output;

    /* more new vars added March/April/May 2005 BDB */
    char overlayGrid[30];
    char overlayShape[256];
    char overlayType[256];
    char overlayMapPrjn[256];
    char overlayEllipsoid[30];
    char inputGridName[30];
    char inputPoly[256];
    char inputPolyType[100];
    char overlayOutType[30];
    char overlayOutName[30];
    char overlayOutDelim[30];
    char tempString[50];
    char debugOutput[10];
    int overlayOutHeader;
    int overlayInReverse = 0;
    int count, retVal;
    char numShapes[10];
    extern int window;

    int outputIoapi = 0;
    int inputIoapi = 0;

    /*.....Beginning of code */
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

    /* the following block determines whether to use file "chunking" or not */
    numShapes[0] = '\0';

    getEnvtValue(ENVT_MAX_INPUT_FILE_SHAPES, numShapes);

    if(numShapes[0] != '\0' && !digiCheck(numShapes))
    {
        /* may need to come back and add a isInteger type check instead of digiCheck */
        maxShapes = atoi(numShapes);
    }
    else
    {
        maxShapes = 0;          /* indicates no file "chunking" */
    }



    /* determine the type of processing to be performed */
    type = getSAJobType();


    if(type == FILTER_SHAPE || type == CONVERT_SHAPE)
    {
        if(getEnvtValue(ENVT_WEIGHT_ATTR_LIST, tempString))
        {
            if(strcmp(tempString, "NONE") == 0)
            {
                no_weight_attr = 1;
            }
        }

        getEnvtValue(ENVT_OUTPUT_FILE_NAME, outfile);

        if(outfile == NULL)
        {
            sprintf(mesg,
                    "Please set %s to the desired name of the output file\n",
                    ENVT_OUTPUT_FILE_NAME);
            ERROR(argv[0], mesg, 2);
        }
    }

    /* surrogate also uses filter file--add to surrogate program file, too */
    /* here begins the different processing modes */

    if(type == FILTER_SHAPE)    /* this also converts the map projection */
    {
        MESG("Filtering shapefile");

        /* retrieve the name of the filter file */
        getEnvtValue(ENVT_FILTER_FILE, filterFile);

        if((strcmp(filterFile, "NONE") == 0))
        {
            filterFile[0] = '\0';
        }

        if(filterFile[0] == '\0')
        {
            sprintf(mesg,
                    "Please set %s to the desired name of the filter file\n",
                    ENVT_FILTER_FILE);
            ERROR(argv[0], mesg, 2);
        }

        /*fprintf(stderr,"filter='%s', filerFile=%s\n",
           getenv(ENVT_FILTER_FILE), filterFile); */

        /*if(outfile == NULL)
           {
           sprintf(mesg,
           "Please set %s to the desired name of the output file\n", 
           ENVT_OUTPUT_FILE_NAME);
           ERROR(argv[0],mesg, 2);
           } */

        /* added test for overwrite of an existing shapefile 1/5/2005 BB */
        /* will exit with an error message if the output filename already exists */

        sprintf(oFileTest, "%s.shp", outfile);
        if((oTest = fopen(oFileTest, "r")) != NULL)
        {
            fclose(oTest);
            ERROR(outfile, "Overwrite of existing shapefile prevented", 1);

        }


#ifdef DEBUG
        sprintf(mesg, "%s filterFile = %s, outFile=%s\n",
                getenv(ENVT_INPUT_FILE_NAME), filterFile, outfile);
        MESG(mesg);
#endif
        sprintf(mesg, getenv(ENVT_INPUT_FILE_NAME));     /* use mesg as temp variable */

        filterDBF(mesg /*data_poly */ , filterFile, outfile);
    }

    else if(type == ALLOCATE)
    {
        window = 0; /* not doing any chunking for now */

 
        if(getEnvtValue(ENVT_ALLOCATE_ATTRS, tempString))
        {
            if(strcmp(tempString, "NONE") == 0)
            {
                no_weight_attr = 1;
            }
        }

        getEnvtValue(ENVT_OUTPUT_FILE_NAME, outfile);

        if(outfile == NULL)
        {
            sprintf(mesg,
                    "Please set %s to the desired name of the output file\n",
                    ENVT_OUTPUT_FILE_NAME);
            ERROR(argv[0], mesg, 2);
        }


        MESG("Using ALLOCATE mode");

        getEnvtValue(ENVT_OUTPUT_FILE_TYPE, tempString);

        if(!strcmp(tempString, "RegularGrid") || !strcmp(tempString, "Regulargrid") ||
           !strcmp(tempString, "IoapiFile")   || !strcmp(tempString, "Ioapifile"))
        {
             
            /* may want too integrate this into PolyReader fcn */
            p_data = PolyReader(ENVT_OUTPUT_GRID_NAME, ENVT_OUTPUT_FILE_TYPE, 
                        NULL, NULL, NULL);

            if(!p_data)
            {
                 ERROR(argv[0], "Error reading polydata file", 2);
            }

            outputMapProj = p_data->map;
            
            if(!strcmp(tempString, "IoapiFile") || !strcmp(tempString, "Ioapifile"))
            {
               outputIoapi = 1;
            }
                        
        }
        else if (!strcmp(tempString, "EGrid") || !strcmp(tempString, "Egrid"))
        {
            /* if the OUTPUT_POLY map projection info is set, get it, else leave null */
            if ((getenv("OUTPUT_POLY_ELLIPSOID") != NULL) && (getenv("OUTPUT_POLY_MAP_PRJN") != NULL))
            {
                 //allocate to egrid and output to ioapi file
                 dataMapProj =
                    getFullMapProjection("OUTPUT_POLY_ELLIPSOID","OUTPUT_POLY_MAP_PRJN");

                 outputMapProj =
                     getFullMapProjection(ENVT_OUTPUT_ELLIPSOID, ENVT_OUTPUT_MAP_PROJ);

                 outputIoapi = 1;
            }
            else
            {
                 dataMapProj = NULL;
                 outputMapProj = NULL;
            }
             
            /* may want too integrate this into PolyReader fcn */
            p_data = PolyReader(ENVT_OUTPUT_GRID_NAME, ENVT_OUTPUT_FILE_TYPE, 
                        dataMapProj, NULL, outputMapProj);

            if(!p_data)
            {
                 ERROR(argv[0], "Error reading EGrid polydata file", 2);
            }

            outputMapProj = p_data->map;
        }
        else if(!strcmp(tempString, "ShapeFile") || !strcmp(tempString, "Shapefile"))
        {
             dataMapProj =
                 getFullMapProjection("OUTPUT_POLY_ELLIPSOID","OUTPUT_POLY_MAP_PRJN");
            outputMapProj =
                  getFullMapProjection(ENVT_OUTPUT_ELLIPSOID, ENVT_OUTPUT_MAP_PROJ);

            p_data =
                   PolyReader(ENVT_OUTPUT_POLY_FILE, "OUTPUT_POLY_TYPE", dataMapProj, NULL, outputMapProj);

            if(!p_data)
            {
                 sprintf(mesg, "Unable to read attributes from %s", ENVT_OUTPUT_POLY_FILE);
                 ERROR(prog_name, mesg, 1);
            }
            /* associate the attributes of the data file with their polygons */
            attachAttribute(p_data, ENVT_OUTPUT_POLY_ATTRS, NULL);
        }
        else
        {
            sprintf(mesg, "%s is not a supported type for %s in ALLOCATE mode", 
                                tempString, ENVT_OUTPUT_FILE_TYPE);
            ERROR(prog_name, mesg, 1);
        }

        MESG("\nFinished reading the output file......\n");
        

        /* need a whole if-else block around input file types:
                supported types are PointFile, ShapeFile, I/O API */
        getEnvtValue(ENVT_INPUT_FILE_TYPE, tempString);

        if(!strcmp(tempString, "ShapeFile") || !strcmp(tempString, "Shapefile") ||
           !strcmp(tempString, "PointFile") || !strcmp(tempString, "Pointfile") )
        {
             inputMapProj =
                 getFullMapProjection(ENVT_INPUT_FILE_ELLIPSOID,
                                 ENVT_INPUT_FILE_MAP_PRJN);

             p_input =
                 PolyReader(ENVT_INPUT_FILE_NAME, ENVT_INPUT_FILE_TYPE,
                       inputMapProj, NULL, outputMapProj);

             if(!p_input)
             {
                 sprintf(mesg, "Unable to read attributes from %s", ENVT_INPUT_FILE_NAME);
                 ERROR(prog_name, mesg, 1);
             }
        }
        else if(!strcmp(tempString, "IoapiFile") || !strcmp(tempString, "Ioapifile"))
        {
            inputMapProj =
                 getFullMapProjection(ENVT_INPUT_FILE_ELLIPSOID,
                                 ENVT_INPUT_FILE_MAP_PRJN);
            p_input =
                PolyReader(ENVT_INPUT_FILE_NAME, ENVT_INPUT_FILE_TYPE,
                      inputMapProj, NULL, outputMapProj);
                      
            if(!p_input)
            {
                sprintf(mesg, "Unable to open input file %s", ENVT_INPUT_FILE_NAME);
                ERROR(prog_name, mesg, 1);
            }
            
            inputIoapi = 1;
        }
        else
        {
             sprintf(mesg,"%s is not a supported type for %s", 
                  tempString, ENVT_INPUT_FILE_TYPE);
             ERROR(prog_name, mesg, 1);
        }


        MESG("\nFinished reading the input shape file......\n");

        /* associate the attributes of the weight file with their polygons */

        if(!strcmp(tempString, "ShapeFile") || !strcmp(tempString, "Shapefile"))
        {
             if(!attachAttribute(p_input, ENVT_ALLOCATE_ATTRS, NULL))
             {
                 sprintf(mesg, "Unable to read %s", ENVT_ALLOCATE_ATTRS);
                 ERROR(prog_name, mesg, 1);
             }
        }
        else if(!strcmp(tempString, "IoapiFile") || !strcmp(tempString, "Ioapifile"))
        {
#ifdef USE_IOAPI
            if(!attachAttributeIoapi(p_input, ENVT_ALLOCATE_ATTRS, NULL, 
                                     ENVT_INPUT_FILE_NAME))
            {
                sprintf(mesg, "Unable to read %s", ENVT_ALLOCATE_ATTRS);
                ERROR(prog_name, mesg, 1);
            }
#endif
        }

        MESG("\nFinished reading the input attribute file......\n");


        p_wd = getNewPoly(0);
        if(!p_wd)
        {
            WARN("Allocation error in getNewPoly");
            return 1;
        }

        /* compute the intersection of the weight and data polygons */
        if(!polyIsect(p_input, p_data, p_wd, FALSE))
        {
            WARN("Possible empty intersection in data polygons");
            return 1;
        }

        MESG("\nFinished intersecting input and output files......\n");

        if(outputIoapi)
        {
#ifdef USE_IOAPI

            MESG("\nWriting ioapi output file...\n");

            allocateIoapi(p_wd, ENVT_OUTPUT_FILE_NAME, !no_weight_attr, 
                          inputIoapi);
#endif
        }
        else
        {
            allocate(p_wd, ENVT_OUTPUT_FILE_NAME, !no_weight_attr);
        }


    } /* end of ALLOCATE mode */

    else if(type == CONVERT_SHAPE)
    {
        if(getenv(ENVT_OUTPUT_FILE_NAME) == NULL)
        {
            sprintf(mesg,
                    "Please set %s to the desired name of the output file\n",
                    ENVT_OUTPUT_FILE_NAME);
            ERROR(argv[0], mesg, 2);
        }
        strcpy(outfile, getenv(ENVT_OUTPUT_FILE_NAME));
        MESG("set data map projection\n");
        dataMapProj =
            getFullMapProjection(ENVT_INPUT_FILE_ELLIPSOID, ENVT_INPUT_FILE_MAP_PRJN);
        MESG("set output map projection\n");
        outputMapProj =
            getFullMapProjection(ENVT_OUTPUT_ELLIPSOID, ENVT_OUTPUT_MAP_PROJ);
        /* this should handle the conversion from input to output map projection */
        MESG("read data\n");
        p_data =
            PolyReader(ENVT_INPUT_FILE_NAME, ENVT_INPUT_FILE_TYPE,
                       dataMapProj, NULL, outputMapProj);

        MESG("write shapefile\n");
        /* write out the geometry of the shapes */
        if(polyShapeWrite(p_data, outfile) != 0)
        {
            ERROR(argv[0], "Error writing output shape file", 2);
        }
        /* copy the .dbf file from the starting location/name to the output one */
#ifndef _WIN32
        sprintf(mesg, "cp %s.dbf %s.dbf", getenv(ENVT_INPUT_FILE_NAME), outfile);
        system(mesg);
#else
        /* TBD: the system command doesn't seem to work on windows - copy in script
         * instead */
        MESG("\nIf not running from a script, to finish the conversion, execute the command:\n");
        sprintf(mesg, "copy %s.dbf %s.dbf\n", getenv(ENVT_INPUT_FILE_NAME),
                outfile);
        MESG(mesg);
#endif
    }
    else if(type == OVERLAY)
    {
        /* added 3/30/2005 BDB */
        MESG("USING OVERLAY mode");

        /* several choices--grid, bounding box, poly_file, shape */

        /* read the thing to overlay & the data file */

        /* may want too integrate this into PolyReader fcn */
        overlayMapProj =
            getFullMapProjection(ENVT_OVERLAY_ELLIPSOID, ENVT_OVERLAY_MAP_PRJN);

        p_overlay =
            PolyReader(ENVT_OVERLAY_SHAPE, ENVT_OVERLAY_TYPE, overlayMapProj,
                       NULL, overlayMapProj);

        /* if the overlay type is a shapefile, do the union */
        getEnvtValue(ENVT_OVERLAY_TYPE, tempString);

        if(!strcmp(tempString, "Shapefile") || !strcmp(tempString, "ShapeFile"))
        {
            tmp_overlay = getNewPoly(0);
            retVal = polyUnion(p_overlay, tmp_overlay);

            /* free the original p_overlay--there doesn't seem to be a function to do that */

            p_overlay = tmp_overlay;

        }

        if(!p_overlay)
        {
            ERROR(argv[0],
                  "Unable to process overlay shape, please check your script",
                  2);
        }

        inputMapProj = getFullMapProjection(ENVT_INPUT_FILE_ELLIPSOID,
                                            ENVT_INPUT_FILE_MAP_PRJN);
        /* read in the input polygons and convert to map projection of the grid */
        window = 0;

        while(!fileCompleted)
        {

            if(maxShapes == 0)
            {
                fileCompleted = 1;
            }

            p_input = PolyReader(ENVT_INPUT_FILE_NAME, ENVT_INPUT_FILE_TYPE,
                                 inputMapProj, p_overlay->bb, p_overlay->map);


            if(!p_input)
            {
                ERROR(argv[0],
                      "Unable to process overlay input file, please check your script",
                      2);
            }


            getEnvtValue(ENVT_INPUT_FILE_TYPE, tempString);
            if((strcmp(tempString, "PointFile") != 0)
               && (strcmp(tempString, "Pointfile") != 0) && 
                (strcmp(tempString, "RegularGrid") != 0) &&
                 (strcmp(tempString, "EGrid") != 0))
            {
                attachAttribute(p_input, ENVT_OVERLAY_ATTRS, NULL);
            }


            /* at this point, input data has the attributes of interest attached to it */

            p_result = getNewPoly(0);

            /* input is parent #1, overlay is parent #2 */
            if(!polyIsect(p_input, p_overlay, p_result, FALSE))
            {
                WARN("Possibly empty intersection in polyIsect");
            }


            if(p_result->parent_poly1 != NULL)
            {
                reportOverlays(p_result);
            }

            /* free the input shape & result shape */
            /* these are not working correctly */
            /* freePolyObject(p_input); */

            /* freePolyObject(p_result); */

        }                       /* end while */

    }
    else
    {
        sprintf(mesg, "%s %s %s %s",
                "Unknown job type -", ENVT_MIMS_PROCESSING,
                "should be set to one of:\n ALLOCATE, OVERLAY, ",
                "FILTER_SHAPE or CONVERT_SHAPE");

        ERROR(argv[0], mesg, 2);

    }

    MESG2("NORMAL COMPLETION of ", argv[0]);
    MESG("");
    return 0;

}                               /* end of main */

/* ============================================================= */
/* ============================================================= */

/* Helper function to determine the type of job / processing 
 * to do based on the value of the environment variable.  
 * Options are ALLOCATE, FILTER_SHAPE, CONVERT_SHAPE,and OVERLAY */
JobType getSAJobType()
{
    char val[50];
    JobType type = SURROGATE;

    if(getEnvtValue(ENVT_MIMS_PROCESSING, val))
    {

        if(!strcmp(val, "ALLOCATE"))
        {
            type = ALLOCATE;
        }
        else if(!strcmp(val, "CONVERT_SHAPE"))
        {
            type = CONVERT_SHAPE;
        }
        else if(!strcmp(val, "FILTER_SHAPE"))
        {
            type = FILTER_SHAPE;
        }
        else if(!strcmp(val, "OVERLAY"))
        {
            type = OVERLAY;
        }

    }
    else
    {
        ERROR(prog_name, "Processing mode not specified", 2);
    }

    return type;
}
