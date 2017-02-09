/***************************************************************************
 * This program is used:
 *  1. to compute grid cell GOES satellite variables from:
 *     (1). GOES Imager Cloud Albedo (CALB, 4km resolution)        %
 *          goes_rtv_CALB_YYYYMMDDhhZ.grb or goes_rtv_CALB_YYYYMMDDhhmmZ.grb
 *
 *     (2). GOES Imager Insolation (INSL, 4km resolution)          Watts per square meters
 *          goes_rtv_INSL_YYYYMMDDhhZ.grb or goes_rtv_INSL_YYYYMMDDhhmmZ.grb
 *
 *     (3). GOES Imager Surface Albedo (SALB, 4km resolution)      %
 *          goes_rtv_SALB_YYYYMMDDhhZ.grb or goes_rtv_SALB_YYYYMMDDhhmmZ.grb
 *
 *     (4). GOES Imager Cloud Top Pressure (CTP, 4km resolution)   Pascals
 *          goes_rtv_CTP_YYYYMMDDhhZ.grb or goes_rtv_CTP_YYYYMMDDhhmmZ.grb
 *
 *     (5)  GOES Imager Infrared Temperature at Cloud/Surafce Level (IR, 4km resolution)    Celsius
 *          goes_rtv_IR_YYYYMMDDhhZ.grb or goes_rtv_IR_YYYYMMDDhhmmZ.grb
 *
 *     (6). GOES Sounder Cloud Top Pressure (CTP, 10km resolution)  Pascals
 *          goes_snd_rtv_CTP_YYYYMMDDhhZ.grb or goes_snd_rtv_CTP_YYYYMMDDhhmmZ.grb
 *
 *     (7). GOES Sounder Skin Temperature (TSKN, 10km resolution)   Celsius
 *          goes_snd_rtv_TSKN_YYYYMMDDhhZ.grb or goes_snd_rtv_TSKN_YYYYMMDDhhmmZ.grb
 *
 *     (8). GOES Sounder Total Precipitable Water Vapor at Ground/Water Surface (TPW, 10km resolution)  mm 
 *          goes_snd_rtv_TPW_YYYYMMDDhhZ.grb or goes_snd_rtv_TPW_YYYYMMDDhhmmZ.grb
 *
 *  2. to output GOES satellite variables into a WRF netcdf file.
 *  3. Time step for GOES data processing is 1 hour.  Since GOES data are obtained around 45 minutes after hours,
 *     time step will always start from 45 minutes from the hour.
 *
 * Written by the Institute for the Environment at UNC, Chapel Hill
 * in support of the EPA NOAA CMAS Modeling, 2008-2009.
 *
 * By L. Ran, Jan-Jun. 2009
 *   Modified  05/2011
 *             09/2012
 *             07/2014: add reading text and latlong grid files in addition to Grib format
 *
 * Usage:  computeGridGOES.exe
 *
 * Environment Variables needed:
 *         GRID_ROWS  -- grid domain rows
 *         GRID_COLUMNS -- grid domain columns
 *         GRID_XMIN -- grid domain xmin
 *         GRID_YMIN -- grid domain ymin
 *         GRID_XCELLSIZE -- grid domain x grid size
 *         GRID_YCELLSIZE -- grid domain y grid size
 *         GRID_PROJ -- grid domain proj4 projection definition
 *         DATA_DIR -- directory of downloded preprocessed GOES data: each day is in a directory
 *         START_DATE_TIME  -- Start day and time: YYYYMMDDHHMM
 *         END_DATE_TIME -- End day and time: YYYYMMDDHHMM
 *         INCLUDE_GOES_IMG_CALB -- YES or NO to include GOES Imager cloud albedo in computation
 *         INCLUDE_GOES_IMG_CTP -- YES or NO to include GOES Imager cloud top pressure in computation
 *         INCLUDE_GOES_IMG_INSL -- YES or NO to include GOES Imager insolation in computation
 *         INCLUDE_GOES_IMG_SALB -- YES or NO to include GOES Imager surface albedo in computation
 *         INCLUDE_GOES_IMG_IR -- YES or NO to include GOES Imager infrared temperature in computation
 *         INCLUDE_GOES_SND_CTP -- YES or NO to include GOES Sounder cloud top pressure in computation
 *         INCLUDE_GOES_SND_TSKN -- YES or NO to include GOES Sounder skin temperature in computation
 *         INCLUDE_GOES_SND_TPW -- YES or NO to include GOES Sounder total precipitable water vapor in computation
 *         OUTPUT_NETCDF_FILE -- netCDF output grid cell GOES satellite information.  Only works for LCC now.

***********************************************************************************/
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <set> 

#include "sa_raster.h"
#include "commontools.h"
#include "geotools.h"

#define GOES_MISSIING_VALUE -999.0

void setSelGOESVariables ( int index );
void readGridLatLongFile (string inputGridFile, double *xLong, double *yLat);
void setGOESGridInfo ( gridInfo *imageInfo, gridInfo *imageInfo_SND );
string getdayStrFromFileName ( string imageFileName, string inGOESVarFileNameFmt );
string matchGOESTimeStep ( string dayTimeStr, int startMins );
void fillGOESMissingValues ( string dayTimeStr, float *goesV );
void readGOESImageTXT ( double *poImage, string imageFileName );
void readGOESImageFile ( string dayTimeStr, string imageFileName, float *goesV, string inGOESVarFileNameFmt, string inGOESVarFileNameFmt_txt );
void computeGOESVariable (int timeStep, int startMins, float *goesV, string inGOESVarFileNameFmt, string inGOESVarFileNameFmt_txt,
                          string gridRasterFile, string dataDir );

//gloabl variables to set GOES variables
const int             timeStep = 60;         //time step to process GOES data is 60 minutes
const int             startMins = 45;        //GOES images are most obtained around 45 mins after hours 
const int             numGOESVars = 8;       //number of GOES variable selections       
const int             timeStrLength = 19;    //time string length in WRF Netcdf output 
const char            goesEnvVarNames [][25] = {
                              "INCLUDE_GOES_IMG_CALB",
                              "INCLUDE_GOES_IMG_CTP",
                              "INCLUDE_GOES_IMG_INSL",
                              "INCLUDE_GOES_IMG_SALB",
                              "INCLUDE_GOES_IMG_IR",
                              "INCLUDE_GOES_SND_CTP",
                              "INCLUDE_GOES_SND_TSKN",
                              "INCLUDE_GOES_SND_TPW"
                             };

const char            goesVarFileNameFmts [][25] = {
                              "goes_rtv_CALB_",
                              "goes_rtv_CTP_",
                              "goes_rtv_INSL_",
                              "goes_rtv_SALB_",
                              "goes_rtv_IR_",
                              "goes_snd_rtv_CTP_",
                              "goes_snd_rtv_TSKN_",
                              "goes_snd_rtv_TPW_"
                             };

const char            goesVarFileNameFmts_txt [][25] = {
                              "goes_img_rtv_CALB_",
                              "goes_img_rtv_CTP_",
                              "goes_img_rtv_INSL_",
                              "goes_img_rtv_SALB_",
                              "goes_img_rtv_IR_",
                              "goes_snd_rtv_CTP_",
                              "goes_snd_rtv_TSKN_",
                              "goes_snd_rtv_TPW_"
                             };

const char            goesVarUnits [][15] = {
                              "Fraction",
                              "Pascals",
                              "Watts/m^2",
                              "Fraction",
                              "Celsius",
                              "Pascals",
                              "Celsius",
                              "mm"
                             };

const char            goesVarDesc [][65] = {
                              "Imager cloud albedo",
                              "Imager cloud top pressure",
                              "Imager insolation",
                              "Imager surface albedo",
                              "Imager infrared temperature at cloud/surface level",
                              "Sounder cloud top pressure",
                              "Sounder skin temperature",
                              "Sounder total precipitable water vapor at ground/water surface"
                             };

vector<string>        inGOESVars,inGOESVarFileNameFmts,inGOESVarUnits,inGOESVarDesc;   //store selected GOES variable names and file format
vector<string>        inGOESVarFileNameFmts_txt;                                       //for new txr format GOES files

map<string,int>       timeIndexHash;       //store time index for each time string      
int                   gridRows,gridCols;   //modeling grid rows and columns to compute grid IDs
GDALDataset           *poGrid;             //store rasterized grid domain image
GDALRasterBand        *poGridBand;         //band 1 for rasterized grid domain image
GUInt32               *poImage_grd=NULL;         //rasterized grid domain image

double                xCellSize_grd, yCellSize_grd; //standard cell size from grid domain raster
int                   xCells_grd, yCells_grd;       //cells for grid domain raster
double                halfGridCellArea;                     //domain grid area
double                xMin_grd,xMax_grd,yMin_grd,yMax_grd;  //modeling domain extent

OGRSpatialReference   oSRS_grd;
char                  *pszProj4_grd = NULL;

//set Imager and Sounder image grids
int                   rows_IMG = 800;
int                   cols_IMG = 1500;
int                   gridSize_IMG    = 4000;    //meters
int                   rows_SND = 320;
int                   cols_SND = 600;
int                   gridSize_SND    = 10000;    //meters 

int                   *grdIndex_IMG=NULL;        //store grid index in Imager grids for multiple variable processing
int                   *grdIndex_SND=NULL;        //store grid index in Sounder grids for multiple variable processing

double                 searchRadius_IMG = 3000.0;   //set maximum search radius for the nearest Imager point
double                 searchRadius_SND = 6000.0;   //set maximum search radius for the nearest SOunder point


/***************************
*********** MAIN  **********
***************************/
int main(int nArgc, char *argv[])
{
        string      shapeFile;       //temp shapeFile to be created
        gridInfo    grid;            //data structure to store a modeling domain information
        string      gdalBinDir;      //GDAL directory

        //print program version
        printf ("\nUsing: %s\n", prog_version);

        printf("Compute grid GOES variable information...\n\n");

       
        /******************************
        * input variables             *
        *******************************/
        string    tmp_str;    
        string    gridRasterFile;
        string    dataDir,dayDir;               //directory containing preprocessed GOES data
        string    startDateTime, endDateTime;   //date and time range for the GOES data extraction
        string    inputGridFile_IMG;           //GOES Imager grid point long and lat
        string    inputGridFile_SND;           //GOES Sounder grid point long and lat
        string    outNetcdfFile;              //output file name

	/***************************
	* Map projection variables *
	***************************/
        char    *pszProj4;
        string  projInfo;
        int     proj;     //WRF projection type
        string  temp_proj;

        /***********************************
        * NetCDF variable array pointers   *
        ************************************/
        int        timeSteps;   //total time steps for the time range
        int        goesDates[2],goesHours[2],goesMins[2];   //input date and time ranges
        float      *times;      //time second array since the starting time
        char       *timesStr;   //time string
        float      *goesV;
        
        int i,j,k;


        //print program version
        printf ("\nUsing: %s\n", prog_version);

	/*********************************
	* Obtain environmental variables *
	*********************************/
        printf ("Obtaining environment variables...\n");

        //no arguments to call this program
        if ( nArgc  > 1 )
        {   
           printf( "\nError:  No arguments are nedded.\n");
           printf( "\tUsage: computeGridGOES.exe\n");
           exit( 1 );
        }

        /******************************************
        *   get variables for domain definitions  *
        ******************************************/
        //rows and columns of the domain
        grid.rows = atoi( getEnviVariable("GRID_ROWS") );
        grid.cols = atoi( getEnviVariable("GRID_COLUMNS") );
        gridRows = grid.rows; 
        gridCols = grid.cols; 
        printf( "\tRows=%d    Cols=%d    Total gird cells=%d\n",gridRows,gridCols,gridRows*gridCols);
        if (gridCols <1 || gridRows <1)
        {
            printf("\tError: grid rows and columns have to be great than 1.\n");
            exit( 1 );
        }

        //get xmin and ymin for the domain
        grid.xmin = (float) atof( getEnviVariable("GRID_XMIN") );
        grid.ymin = (float) atof( getEnviVariable("GRID_YMIN") );
        printf( "\txmin=%f    ymin=%f \n", grid.xmin, grid.ymin);

        //get x and y cell size
        grid.xCellSize = (float) atof( getEnviVariable("GRID_XCELLSIZE") );
        grid.yCellSize = (float) atof( getEnviVariable("GRID_YCELLSIZE") );


        //max x and y
        grid.xmax = grid.xmin + grid.xCellSize * grid.cols;
        grid.ymax = grid.ymin + grid.yCellSize * grid.rows;


        halfGridCellArea = grid.xCellSize * grid.yCellSize / 2.0 ;   //domain cell area
        printf( "\txcell=%.0f    ycell=%.0f \n",grid.xCellSize,grid.yCellSize);

        if (grid.xCellSize <= 0.0 || grid.yCellSize <= 0.0)
        {
            printf("\tError: domain grid cell size has to be > 0.\n");
            exit( 1 );
        }

        //get projection info
        pszProj4 = getEnviVariable("GRID_PROJ");
        grid.strProj4 = getEnviVariable("GRID_PROJ");
        printf( "\tproj4=%s\n",pszProj4);

        //In case users want to use their own Shapefile and POLYGON_ID
        //those two environment variables are optional
        //Shapefile Name
        if (getenv ( "GRID_SHAPEFILE_NAME" ) != NULL)
        {
           grid.name = string ( getEnviVariable("GRID_SHAPEFILE_NAME") );
           printf("\tOutput Shapefile name: %s\n",grid.name.c_str());
        }

        //get polygon ID item in Shapefile
        if (getenv ( "POLYGON_ID" ) != NULL)
        {
           grid.polyID = string ( getEnviVariable( "POLYGON_ID" ) );
           printf( "\tShapefile polygon ID item: %s\n",grid.polyID.c_str() );
        }
        else
        {
           //created Shapefile will use GRIDID item
           grid.polyID = string ( "GRIDID" );   //created domain Shapefile uses GRIDID
        }

        //get projection parameters from Proj4 string
        temp_proj = string (pszProj4);
        proj = getProjType(temp_proj);
        printf( "\tproj type = %d \n",proj);


        /*******************************************
        *       get GDAL bin dir                   *
        *******************************************/
        gdalBinDir =   string ( getEnviVariable("GDALBIN") );
        gdalBinDir =  processDirName( gdalBinDir );
        FileExists(gdalBinDir.c_str(), 0 );  //the dir has to exist

        /*******************************************
        *       get GOES data dir                  *
        *******************************************/
        //printf( "Getting directory containing preprocessed GOES data.\n");
        dataDir = string( getEnviVariable("DATA_DIR") );
        dataDir = processDirName(dataDir );  //make sure that dir ends with path separator
        printf("\tPreprocessed GOES file directory: %s\n",dataDir.c_str() );
        FileExists(dataDir.c_str(), 0 );  //the dir has to exist.

        //printf( "Getting day and time range to extract preprocessed GOES data.\n");
        startDateTime = string( getEnviVariable("START_DATE_TIME") );
        startDateTime = trim( startDateTime );
        endDateTime = string( getEnviVariable("END_DATE_TIME") );
        endDateTime = trim( endDateTime );
        printf ( "\tStart date and time is:  %s\n",startDateTime.c_str() );
        printf ( "\tEnd date and time is:  %s\n",endDateTime.c_str() );
        if ( startDateTime.length() != 12 || endDateTime.length() != 12 )  
        {
           printf ( "\tError: Start and End date/time format should be: YYYYMMDDHHMM\n" );
           exit ( 1 );
        } 
        if ( atol(startDateTime.c_str()) == 0 ||  atol(endDateTime.c_str()) == 0 )
        {
           printf ( "\tError: Start and End date/time should be all numbers\n" );
           exit ( 1 );
        }

        //printf( "Getting indicator to include GOES variable computation.\n");
        for (i=0; i<numGOESVars; i++)
        {
           setSelGOESVariables ( i );//fill vectors: inGOESVars,inGOESVarFileNameFmt,inGOESVarUnits,inGOESVarDesc
        }

      
        /***********************************************************/
        /*  get Imager and Sounder point lat and long point files  */
        /***********************************************************/
        inputGridFile_IMG = string ( getEnviVariable("GOES_IMAGER_POINT_LATLONG") );
        inputGridFile_IMG = trim( inputGridFile_IMG );
        printf( "\n\tGOES Imager point long and lat file is:  %s\n",inputGridFile_IMG.c_str() );
        FileExists(inputGridFile_IMG.c_str(), 0 );  //the file has to exist.

        inputGridFile_SND = string ( getEnviVariable("GOES_SOUNDER_POINT_LATLONG") );
        inputGridFile_SND = trim( inputGridFile_SND );
        printf( "\tGOES Sounder point long and lat file is:  %s\n",inputGridFile_SND.c_str() );
        FileExists(inputGridFile_SND.c_str(), 0 );  //the file has to exist.

        
        //printf( "Getting output NetCDF file name.\n");
        outNetcdfFile = string( getEnviVariable("OUTPUT_NETCDF_FILE") );
        printf( "\tOutput NetCDF file is: %s\n", outNetcdfFile.c_str() );
        FileExists(outNetcdfFile.c_str(), 3 );  //the file has to be new.
        

        /*********************************/
        /*    Create grid raster file    */
        /*********************************/
        string createdGridImage  = createGridImage ( grid );


        /****************************************************************/
        /*      read into latlong arrays Imager and Sounder images      */
        /****************************************************************/
 
        double *latP_IMG = (double *) CPLCalloc(sizeof(double),rows_IMG*cols_IMG);
        double *longP_IMG = (double *) CPLCalloc(sizeof(double),rows_IMG*cols_IMG);

        double *latP_SND = (double *) CPLCalloc(sizeof(double),rows_SND*cols_SND);
        double *longP_SND = (double *) CPLCalloc(sizeof(double),rows_SND*cols_SND);

        readGridLatLongFile (inputGridFile_IMG, longP_IMG, latP_IMG);

        readGridLatLongFile (inputGridFile_SND, longP_SND, latP_SND);

        
        /*********************************************************/
        /*  Set grid info for Imager 4km resolution in latlong   */
        /*  Set grid info for SOunder 10km resolution in latlong */
        /*********************************************************/
        gridInfo imageInfo, imageInfo_SND;
    
        setGOESGridInfo ( &imageInfo, &imageInfo_SND );
        
        //printGridInfo (imageInfo );
        //printGridInfo (imageInfo_SND );

       
        /*********************************/
        /*    Compute raster resolution  */
        /*********************************/
        double  rasterResolution =  computeRasterResolution ( imageInfo, grid );

        /*********************************/
        /*  Compute new raster info      */
        /*********************************/
        string no_shape;

        gridInfo newRasterInfo = computeNewRasterInfo ( no_shape, rasterResolution, grid);

        printGridInfo (newRasterInfo);

        /*********************************/
        /*    resample grid raster       */
        /*********************************/
        gridRasterFile = resampleRaster (createdGridImage, newRasterInfo);

        //delete created raster file
        deleteRasterFile ( createdGridImage );


        /**********************************
        * Compute times and timesStr data *
        **********************************/
        printf("\nObtain times in seconds from the start and time string in each step data...\n");

        //get date, hour, minute ranges in 2d arrays
        timeSteps = getDateTimeRange(startDateTime, endDateTime, goesDates, goesHours, goesMins, timeStep, startMins); 

        // allocate memory for times in seconds array
        if ( (times = (float*) calloc (timeSteps, sizeof(float)) ) == NULL)
        {
           printf( "Calloc times failed.\n");
           exit ( 1 );
        } 

        if ( (timesStr = (char *) calloc (timeSteps*timeStrLength+1, sizeof(char)) ) == NULL)
        {
            printf( "Calloc timesStr failed.\n");
            exit ( 1 );
        }

        //fill the two time arrays for WRF Netcdf output
        timeIndexHash = fillTimeArrays (times, timesStr, startDateTime, endDateTime, timeStep, startMins);  

        /*****************************************************
        * Compute center and domain corner lat and long data *
        ******************************************************/ 
        printf("\nCompute netcdf extent and dimension cooridnates...\n");
        computeGridCornerCenterLatLong ( &grid );  

	/**********************************
	* Compute domain grid coordinates *
	************************************/
        computeGridCoordinates (&grid);


	/***********************************************
	* Write WRF dimension data in netCDF format    *
	***********************************************/

	/*****************************
	* Open input and output files*
	******************************/
        int ncid;
        
	anyErrors( nc_create(outNetcdfFile.c_str(), NC_CLOBBER, &ncid) );   //open output file
 
        /********************
        * Dimension lengths *
        ********************/
        size_t west_east_len   = grid.cols;
        size_t south_north_len = grid.rows;
        size_t dateStr_len = timeStrLength;
        size_t time_len = timeSteps;   

        /********************
        * Define dimensions *
        ********************/
        int dateStr_dim;
        int time_dim;
        int west_east_dim;
        int south_north_dim;

        /***********************
        * Define variables IDs *
        ************************/
        int time_id,timeStr_id;
        int goesV_id;
        
        /*****************************
        * Define variables for output*
        ******************************/
        int        dimIndex[NC_MAX_DIMS];                 //dimension index array for write out array
        int        totalSize;                             //total dim size
        int        fieldtype[1];
        int        firstTime;                             //indicator to notice that first dimension is Time
        int        timeLayOffset;                         //total dimesions above x and y dimensions

        fieldtype[0] = 104;

        /******************************
        * Define dimensions for output*
        *******************************/

        printf( "Define dimensions in output netcdf file...\n" );

        printf( "\tTime\n" );
        anyErrors( nc_def_dim(ncid, "Time", NC_UNLIMITED, &time_dim) );
        printf( "\tDateStrLen\n" );
        anyErrors( nc_def_dim(ncid, "DateStrLen", dateStr_len, &dateStr_dim) );
        printf( "\twest_east\n" );
        anyErrors( nc_def_dim(ncid, "west_east", west_east_len, &west_east_dim) );
        printf( "\tsouth_north\n\n" );
        anyErrors( nc_def_dim(ncid, "south_north", south_north_len, &south_north_dim) );

        printf( "Define time variables in output netcdf file...\n" );

        dimIndex[0] = time_dim;
        anyErrors( nc_def_var(ncid, "Times_sec", NC_FLOAT, 1, dimIndex, &time_id) );
        tmp_str = string ("Seconds since ");
        tmp_str.append (startDateTime);
        anyErrors( nc_put_att_text(ncid, time_id, "description", tmp_str.size(), tmp_str.c_str() ) );
        anyErrors( nc_put_att_text(ncid, time_id, "units", 7, "Seconds") );
       
        dimIndex[0] = time_dim;
        dimIndex[1] = dateStr_dim;
        anyErrors( nc_def_var(ncid, "Times", NC_CHAR, 2, dimIndex, &timeStr_id) );
        anyErrors( nc_put_att_text(ncid, timeStr_id, "description", 21, "Time string (19 char)" ) );
        anyErrors( nc_put_att_text(ncid, timeStr_id, "units", 19, "yyyy-mm-dd_hh:mm:ss") );

        
        /*******************************
        * Leave define mode for output *
        ********************************/
        anyErrors( nc_enddef (ncid) );


        /****************************************
        * Write the time variables to output *
        *****************************************/

        //output Times_sec variable -- due to error output in using the following different function
        long tx_start[2];
        long tx_count[2];
        tx_start[0]=0;
        tx_count[0]=time_len;
        anyErrors( ncvarput(ncid, time_id, tx_start, tx_count, times) );
        //anyErrors( nc_put_var_float(ncid, time_id, times) );  //error output for this 1D
        printf( "\tWrited Times_sec\n" );
        
        //anyErrors( nc_put_var_text(ncid, timeStr_id, timesStr) );
        tx_start[0]=0;
        tx_start[1]=0;
        tx_count[0]=time_len;
        tx_count[1]=dateStr_len;
        anyErrors( ncvarput(ncid, timeStr_id, tx_start, tx_count, timesStr) );
        printf( "\tWrited Times string\n\n" );

        //Write grid coordinate variables
        writeWRFGridCoordinates ( ncid, west_east_dim, south_north_dim, grid );
        
        free (times);
        free (timesStr);
        free (grid.x);
        free (grid.y);
        free (grid.lat);
        free (grid.lon);

        /******************************/
        /*      Register format(s)    */
        /******************************/
        GDALAllRegister();


        /***********************************************/
        /*     Open rasterized grid domain image info  */
        /***********************************************/
        const char   *pszWKT = NULL;
        char         *pszWKT_nc = NULL;
        double       xUL;
        double       yUL;

        //open the grid domain image
        //printf ( "Obtain information from rasterized domain image file: %s\n",gridRasterFile.c_str() );
        poGrid = (GDALDataset *) GDALOpen( gridRasterFile.c_str(), GA_ReadOnly );
        if( poGrid == NULL )
        {
          printf( "\tError: Open rasterized domain file failed: %s.\n", gridRasterFile.c_str() );
          exit( 1 );
        }
        poGridBand = poGrid->GetRasterBand( 1 );  // band 1

        // get rows and columns of the image
        xCells_grd = newRasterInfo.cols;
        yCells_grd = newRasterInfo.rows;
        //printf( "\tGrid domain cells are: %dx%d\n", xCells_grd, yCells_grd );

        //get UL corner coordinates and grid size for the domain grid
        xUL = newRasterInfo.xmin;
        yUL = newRasterInfo.ymax;
        //printf( "\tDoamin UL Origin = (%.6lf,%.6lf)\n", xUL,yUL );

        xCellSize_grd =  newRasterInfo.xCellSize;
        yCellSize_grd =  newRasterInfo.yCellSize;  
        //printf( "\tGrid domain pixel size = (%.3lf,%.3lf)\n", xCellSize_grd, yCellSize_grd );

        //compute extent of band 1 domain grid image (only one band for all images)
        xMin_grd = xUL;
        xMax_grd = newRasterInfo.xmax;

        yMax_grd = yUL;
        yMin_grd = newRasterInfo.ymin;;
        //printf( "\tDomain grid raster extent:  minXY(%.3lf,%.3lf)   maxXY(%.3lf,%.3lf)\n", xMin_grd,yMin_grd,xMax_grd,yMax_grd );

        //get projection from the domain rater image
        if( (pszWKT = poGrid->GetProjectionRef())  == NULL )
        {
           printf( "\tError: Projection is not defined in the domain grid raster file: %s.\n", gridRasterFile.c_str() );
           printf( "\tCan define it using gdal_translate utility.\n");
           exit( 1 );
        }
        pszWKT_nc =strdup ( pszWKT );   //convert it to no const char
        oSRS_grd.importFromWkt( &pszWKT_nc );      //needed it for comparison
        oSRS_grd.exportToProj4( &pszProj4_grd );
        //printf ( "\tProj4 for the domain raster image is: %s\n\n", pszProj4_grd);


        totalSize = xCells_grd * yCells_grd;
        poImage_grd = (GUInt32 *) CPLCalloc(sizeof(GUInt32), totalSize);

        if ( ( poGridBand->RasterIO( GF_Read, 0, 0, xCells_grd, yCells_grd,
              poImage_grd, xCells_grd, yCells_grd, GDT_UInt32, 0, 0 ) ) == CE_Failure )
        {
            printf( "\t\tError: Reading rasterized domain image failed: %s\n",gridRasterFile.c_str() );
            CPLFree (poImage_grd);
            exit( 1 );
        }


        /******************************************************
        *  Compute rasterized grid Indexes in satellite       *
        *  geolocation array                                  *
        ******************************************************/
        totalSize = xCells_grd * yCells_grd;

        //allocate memory to store geolocation index for the rasterized domain grids to Imager
        if ( (grdIndex_IMG = (int*) calloc (totalSize, sizeof(int)) ) == NULL)
        {
            printf( "Calloc grdIndex_IMG variable failed.\n");
            exit ( 1 );
        }

        bool gotIndex = computeDomainGridImageIndex( grdIndex_IMG, longP_IMG, latP_IMG, imageInfo, newRasterInfo, searchRadius_IMG );

        CPLFree (latP_IMG);
        CPLFree (longP_IMG);

        if (! gotIndex )
        {
           printf( "Domain grids and GOES Imager grids do not intersect.\n");
           exit ( 1 );
        }

         
        //allocate memory to store geolocation index for the rasterized domain grids to Sounder
        if ( (grdIndex_SND = (int*) calloc (totalSize, sizeof(int)) ) == NULL)
        {
           printf( "Calloc grdIndex_SND variable failed.\n");
           exit ( 1 );
        }

        gotIndex = computeDomainGridImageIndex( grdIndex_SND, longP_SND, latP_SND, imageInfo_SND, newRasterInfo, searchRadius_SND );

        CPLFree (latP_SND);
        CPLFree (longP_SND);

        if (! gotIndex )
        {
           printf( "Domain grids and GOES Sounder grids do not intersect.\n");
           exit ( 1 );
        }


        /*****************************************
        * Read each variable from GOES directory *
        * Define new variable and attributes     *
        * Write the variable in output           *
        *****************************************/
        //total dim size
        totalSize = time_len * south_north_len * west_east_len;
        for ( i=0; i<inGOESVars.size(); i++ )
        {
           printf( "Compute GOES variable: %s   File format: %s*.* or %s*.*\n",inGOESVars[i].c_str(),inGOESVarFileNameFmts[i].c_str(), inGOESVarFileNameFmts_txt[i].c_str() );

           //allocate memory for the GOES variable 
           if ( (goesV = (float*) calloc (totalSize, sizeof(float)) ) == NULL)
           {
              printf( "Calloc goesV failed.\n");
              exit ( 1 );
           }

           computeGOESVariable (timeStep,startMins,goesV,inGOESVarFileNameFmts[i],inGOESVarFileNameFmts_txt[i], gridRasterFile,dataDir );

           //put the output in define mode
           anyErrors( nc_redef (ncid) );

           //define the varaible
           float   missValue[1];
           missValue[0] = MISSIING_VALUE;

           dimIndex[0] = time_dim;   
           dimIndex[1] = south_north_dim;
           dimIndex[2] = west_east_dim;
           anyErrors( nc_def_var(ncid, inGOESVars[i].c_str(), NC_FLOAT, 3, dimIndex, &goesV_id) );           
           anyErrors( nc_put_att_int(ncid, goesV_id, "FieldType", NC_INT, 1, fieldtype) ); 
           anyErrors( nc_put_att_text(ncid, goesV_id, "MemoryOrder", 3, "XY ") );
           anyErrors( nc_put_att_text(ncid, goesV_id, "description", strlen(inGOESVarDesc[i].c_str()), inGOESVarDesc[i].c_str()) );
           anyErrors( nc_put_att_text(ncid, goesV_id, "units", strlen(inGOESVarUnits[i].c_str()), inGOESVarUnits[i].c_str()) );
           anyErrors( nc_put_att_float(ncid, goesV_id, "missing_value", NC_FLOAT, 1, missValue) ); 
           anyErrors( nc_put_att_text(ncid, goesV_id, "stagger", 1, "M") );

           // Leave define mode for output 
           anyErrors( nc_enddef (ncid) );

           //write variable
           anyErrors( nc_put_var_float(ncid, goesV_id, goesV) );
           free ( goesV );
 
        }

        GDALClose( (GDALDatasetH) poGrid );
        CPLFree (poImage_grd);
        CPLFree ( grdIndex_IMG );
        CPLFree ( grdIndex_SND );
        
        /*******************************************************/
        /*  Close and delete rasterized grid domain image file */
        /*******************************************************/
        deleteRasterFile ( gridRasterFile ); 


	/**************************
	* Store global attributes *
	**************************/
        string title = string ( "WRF GRID GOES DATA FROM computeGridGOES.exe" );
        writeWRFGlobalAtributes ( ncid, title, grid, startDateTime );


	/********************
	* Close netCDF file *
	********************/
        anyErrors( nc_close(ncid) );

        
        printf( "Finished writing output WRF NetCDF file: %s\n\n", outNetcdfFile.c_str());
}


/*********************End of Main program****************************************************************/




/***************************************************
* set selected GOES variable and file name format  *
***************************************************/
void setSelGOESVariables ( int index )
{
    string   selection;  //YES or NO selection
    int      i;
    char   *enviVar;   //environmental variable to get


    enviVar = getenv ( goesEnvVarNames[index] );

    if ( enviVar == NULL )    
    {
       return;       // this environment variablle is not defined
    }

    selection =  string ( enviVar );
    selection = stringToUpper( selection );
    selection = trim( selection );
   
    printf("\t%s is set to: %s\n",goesEnvVarNames[index], selection.c_str() );
    if ( selection.compare("YES") != 0 && selection.compare("NO") != 0)
    {
       printf( "\tError: %s should be set to YES or NO.\n", goesEnvVarNames[index] );
       exit ( 1 );
    }
 
    if ( selection.compare("YES") == 0 )
    {
       string tmp_str = string ( goesEnvVarNames[index] );
       
       //extract GOES variable name out from the environment variable name such as: IMG_CALB or SND_TSKN
       //start from 13 position (INCLUDE_GOES_) to the end of the environment variable name
       tmp_str.erase (0, 13);
       inGOESVars.push_back ( tmp_str );   //store this GOES variable name
       printf( "\t\tGOES variable name is set to: %s\n",tmp_str.c_str() );

       tmp_str = string ( goesVarFileNameFmts[index] );
       inGOESVarFileNameFmts.push_back ( tmp_str );  //store this GOES variable name format
       printf( "\t\tGOES variable file format is: %s for grb format files\n",tmp_str.c_str() );

       tmp_str = string ( goesVarFileNameFmts_txt[index] );
       inGOESVarFileNameFmts_txt.push_back ( tmp_str );  //store this GOES variable name format
       printf( "\t\tGOES variable file format is: %s for txt format files\n",tmp_str.c_str() );


       tmp_str = string ( goesVarUnits[index] );
       inGOESVarUnits.push_back ( tmp_str );   //store this GOES variable unit
       printf( "\t\tGOES variable unit is set to: %s\n",tmp_str.c_str() );

       tmp_str = string ( goesVarDesc[index] );
       inGOESVarDesc.push_back ( tmp_str );   //store this GOES variable description
       printf( "\t\tGOES variable description is set to: %s\n",tmp_str.c_str() );
       
    }

}


/*****************************************************************
*  Read in Imager and Sounder grid point longitude and latitude  *
******************************************************************/
void readGridLatLongFile (string inputGridFile, double *xLong, double *yLat)
{

     printf("\nReading GOES grid location from: %s...\n", inputGridFile.c_str() );


     /********************
     * reading variables *
     ********************/
     ifstream     imageStream;             //stream to read input EPIC site file
     double       x,y;
     string       lineStr,dataStr;         //for reading in site line
     vector<string>     vecString;
     int          i;                       //file index
     int          cols, rows;


     if ( inputGridFile.find ( "Imager" )  != string::npos )
     {
        cols = cols_IMG;
        rows = rows_IMG;
     }
     else if ( inputGridFile.find ( "Sounder" )  != string::npos )
     {
        cols = cols_SND;
        rows = rows_SND;
     }
     else
     {
         printf ("\tFile name should constain Imager or Sounder letters: %s\n",inputGridFile.c_str() );
         exit ( 1 );
     }

     i = 0;

     imageStream.open( inputGridFile.c_str() );
     if (imageStream.good() )
     {
        getline( imageStream, lineStr, '\n');
        while ( !imageStream.eof() )
        {
           lineStr = trim (lineStr);                 //get rid of spaces at edges
           i++;  //count the line number

           //get rid of empty line
           if ( lineStr.size() == 0 )
           {
              goto newloop;
           }

           //put data in string vector
           vecString = string2stringVector (lineStr, ",");
           if ( vecString.size()  < 3 )
           {
              printf( "\tError: File should contain at least 3 items: gridID,longitude,latitude.  %d - %s\n", i,lineStr.c_str() );
              exit ( 1 );
           }

           if ( i == 1 )
           {
              printf( "\tFile header: %s\n", lineStr.c_str() );
           }
           else
           {
              //get longitude
              dataStr = vecString[1];
              x = atof ( dataStr.c_str() );

              //get latitude
              dataStr = vecString[2];
              y = atof ( dataStr.c_str() );

              int gridID = i - 1;     //gridID from LL to UR
              int row = (int) ( floor ( ( gridID - 1 ) / cols) );   //start from 0
              int col = ( gridID - 1 ) % cols;                      //start from 0

              if ( row < 0 || row >= rows || col < 0 || col >= cols)
              {
                 printf( "\tError: computed row % and col % outside defined image dimensions: %s\n", row, col, inputGridFile.c_str() );
                 exit ( 1 );
              }

              int row_new = rows - row - 1;                         //0 from UL corner to LR
              int index_new = row_new * cols + col;

              xLong[index_new] = x;
              yLat[index_new] = y;
  
              //if ( i == 2 || i == (rows*cols+1) ) printf( "\ngridID old: %d  gridID new: %d  Row old=%d   Row new=%d longX = %lf latY = %lf\n",i-1, index_new+1,row,row_new, x, y);

           } //site lines

           newloop:
           getline( imageStream, lineStr);
        }  // while loop

        printf ("\tFinished reading: %d lines\n\n", i);
        imageStream.close();

     } // if good
     else
     {
        printf ("\tError: Open file - %s\n", inputGridFile.c_str() );
        exit ( 1 );
     }


}


/*********************************************************/
/*  Set grid info for Imager 4km resolution in latlong   */
/*  Set grid info for SOunder 10km resolution in latlong */
/*********************************************************/
void setGOESGridInfo ( gridInfo *imageInfo, gridInfo *imageInfo_SND )
{

        string   tmp_str;


        imageInfo->strProj4 = strdup ( "+proj=latlong +datum=NAD83" );
        imageInfo->xCellSize = (gridSize_IMG / 1000.0 ) / 111.1;    //in degree
        imageInfo->yCellSize = (gridSize_IMG / 1000.0 ) /111.1;

        imageInfo->rows = rows_IMG;
        imageInfo->cols = cols_IMG;

        imageInfo_SND->strProj4 = strdup ( "+proj=latlong +datum=NAD83" );
        imageInfo_SND->xCellSize = (gridSize_SND / 1000.0 ) / 111.1;    //in degree
        imageInfo_SND->yCellSize = (gridSize_SND / 1000.0 ) /111.1;

        imageInfo_SND->rows = rows_SND;
        imageInfo_SND->cols = cols_SND;


        //title
        tmp_str = string ("GOES image grid info" );
        imageInfo->attsStr.push_back ( tmp_str );

        tmp_str = string ("GOES Sounder grid info" );
        imageInfo_SND->attsStr.push_back ( tmp_str );

        //units
        tmp_str = string ("Degree" );
        imageInfo->attsStr.push_back ( tmp_str );
        imageInfo_SND->attsStr.push_back ( tmp_str );

        //scale factor
        tmp_str = string ("1.0" );
        imageInfo->attsStr.push_back ( tmp_str );
        imageInfo_SND->attsStr.push_back ( tmp_str );

        //offset
        tmp_str = string ("0.0" );
        imageInfo->attsStr.push_back ( tmp_str );
        imageInfo_SND->attsStr.push_back ( tmp_str );


        //missing value
        tmp_str = string ("-999.0" );
        imageInfo->attsStr.push_back ( tmp_str );
        imageInfo_SND->attsStr.push_back ( tmp_str );

}


/******************************************************
*  Get day and time string from GOES image file name  *
******************************************************/
string getdayStrFromFileName ( string imageFileName, string inGOESVarFileNameFmt )
{
     int       i,j;
     string    dayTimeStr;
     long      value; 

     
      i = inGOESVarFileNameFmt.size (); 

      if ( imageFileName.find ( "Z.grb" ) != string::npos  )
      {
          j = imageFileName.find ( "Z.grb" );
      }
      else if ( imageFileName.find ( "Z.txt" ) != string::npos  )
      {
          j = imageFileName.find ( "Z.txt" );
      }
      else
      {
         printf ("\tFile name should be in *Z.grb or *Z.txt: %s\n",imageFileName.c_str() );
         exit ( 1 );
      } 


      //printf ("\t\t num of day time string = %d\n",j-i);
      if ( (j-i) != 10 && (j-i) != 12 )
      {
         printf ("\t Time format should be YYYYMMDDhh or YYYYMMDDhhmm: %s\n",imageFileName.c_str() ); 
         exit ( 1 );
      }

      dayTimeStr = imageFileName.substr (i,j-i);

      if ( atol (dayTimeStr.c_str() ) == 0 )
      {
         printf ("\tTime format should be YYYYMMDDhhZ.* or YYYYMMDDhhmmZ.*: %s\n",imageFileName.c_str() );
         exit ( 1 );
      }
    
      return   dayTimeStr;

}


/****************************************************************
*  Match GOES time step with startMins to the time from a file *
*  GOES data are around 45 minutes after the hour               *
*  Always assume that time of image is 45 mins at the hour      *
****************************************************************/
string matchGOESTimeStep ( string dayTimeStr, int startMins )
{

   string   tmp_str;
   string   time_str,yearStr,monthStr,dayStr,hourStr,minStr;
   int      value;
   int      minus1 = -1;
   int      maxDays;

   //obtain data from the date/time string
   yearStr = dayTimeStr.substr(0, 4);   //get year
   monthStr = dayTimeStr.substr(4, 2);  //get month
   dayStr = dayTimeStr.substr(6, 2);    //get day
   hourStr = dayTimeStr.substr(8, 2);   //get hour

   if ( dayTimeStr.size() == 12 )
   {
      minStr =  dayTimeStr.substr(10, 2);  //get minute
   }
   else
   {
      minStr = string ( "45" );
   }

   //process minutes first
   value = atoi( minStr.c_str() );   //min < 15, then the image will below to last hour' startMins
   minStr = convert2NumsTo2Chars (startMins);
   if ( value < 15  )
   {
      value = atoi( hourStr.c_str() ) + minus1;   //use previous hour
      if ( value >= 0 )
      {
         hourStr = convert2NumsTo2Chars ( value );
      }
      else
      {
         hourStr = string ( "23" );
         value = atoi( dayStr.c_str() ) + minus1;   //update day
         if ( value >= 1 )
         {
            dayStr = convert2NumsTo2Chars ( value );
         }
         else
         {
            value = atoi( monthStr.c_str() )+ minus1;   //update month
            if (value >= 1 )
            {
               maxDays = GetDaysInMonth ( atoi(yearStr.c_str()), value );
               monthStr = convert2NumsTo2Chars ( value );
            }
            else
            {
               value = atoi( yearStr.c_str() ) + minus1;   //update year
               maxDays = GetDaysInMonth ( value, 12 );
               monthStr = string ( "12" );
               yearStr = convert2NumsTo2Chars ( value );
            }
            dayStr = convert2NumsTo2Chars ( maxDays );

         }  //day, month and year

      }  //hour

   }  //minute

   time_str = yearStr + monthStr + dayStr + hourStr + minStr;

   return time_str;

}

/************************************************************
*  Set one time step data to MISSING_VALUE  for a variable  *
*************************************************************/
void fillGOESMissingValues ( string dayTimeStr, float *goesV )
{
     int  timeIndex, dataIndex;
     int  i,j;

     timeIndex = timeIndexHash[dayTimeStr];
     printf ("\tMissing %s   time step: %d\n",dayTimeStr.c_str(), timeIndex);   
     for ( i=0; i<gridRows; i++ )
     {
        for (j=0; j<gridCols; j++ )
        {
           dataIndex = timeIndex * gridRows * gridCols + i * gridCols + j;
           goesV[dataIndex] = MISSIING_VALUE;
        }  //j
     } //i
}



/************************************
*  Read image file in TXT format    *
*************************************/
void readGOESImageTXT ( double *poImage, string imageFileName )
{

     printf("\nReading GOES text image file: %s...\n", imageFileName.c_str() );


     /********************
     * reading variables *
     ********************/
     ifstream     imageStream;             //stream to read input EPIC site file
     double       value;
     string       lineStr,dataStr;         //for reading in site line
     vector<string>     vecString;
     int          i;                       //file index
     int          cols, rows;
    
    
     if ( imageFileName.find ( "_snd_" )  == string::npos )
     { 
        cols = cols_IMG;
        rows = rows_IMG;
     }
     else
     {
        cols = cols_SND;
        rows = rows_SND;
     } 

     i = 0;

     imageStream.open( imageFileName.c_str() );
     if (imageStream.good() )
     {
        getline( imageStream, lineStr, '\n');
        while ( !imageStream.eof() )
        {
           lineStr = trim (lineStr);                 //get rid of spaces at edges
           i++;  //count the line number

           //get rid of empty line
           if ( lineStr.size() == 0 )
           {
              goto newloop;
           }

           //put data in string vector
           vecString = string2stringVector (lineStr, ",");
           if ( vecString.size()  < 1 )
           {
              printf( "\tError: File should contain at 1 item: value.  %d - %s\n", i,lineStr.c_str() );
              exit ( 1 );
           }

           if ( i <= 31 )
           {
              printf( "\tFile header: %s\n", lineStr.c_str() );
           }
           else
           {
              //get value
              dataStr = vecString[0];
              value = atof ( dataStr.c_str() );

              int gridID = i - 31;     //gridID from LL to UR

              int row = (int) ( floor ( ( gridID - 1 ) / cols) );   //start from 0
              int col = ( gridID - 1 ) % cols;                      //start from 0

              if ( row < 0 || row >= rows || col < 0 || col >= cols)
              {
                 printf( "\tError: computed row % and col % outside defined image dimensions: %s\n", row, col, imageFileName.c_str() );
                 exit ( 1 );
              }
              
              int row_new = rows - row - 1;                         //0 from UL corner to LR
              int index_new = row_new * cols + col;

              poImage[index_new] = value;                          //image starts from UL corner
  
              //if ( i == 32 || i == (rows*cols+31) ) printf( "\ngridID old: %d  gridID new:  %d  value = %lf\n",i-32, index_new+1, value);

           } //grid lines from LL to UR corner

           newloop:
           getline( imageStream, lineStr);
        }  // while loop

        printf ("\tFinished reading: %d lines\n\n", i);
        imageStream.close();

     } // if good
     else
     {
        printf ("\tError: Open file - %s\n", imageFileName.c_str() );
        exit ( 1 );
     }

}


/*********************************************************
*  Compute one time step data from a GOES variable file  *
**********************************************************/
void readGOESImageFile ( string dayTimeStr, string imageFileName, float *goesV, string inGOESVarFileNameFmt, string inGOESVarFileNameFmt_txt )
{
     int                   gridPixels;          //total pixels in modeling grids
     int                   *gridIDs=NULL;       //array to store number of pixels in each modeling grids
     double                *gridValues = NULL;  //array to store total values in each modeling grids
     int                   timeIndex, dataIndex, pixelIndex;
     int                   i,j;
     GDALDataset           *poRDataset;
     GDALRasterBand        *poBand;
     double                *poImage;

     double                adfGeoTransform[6];
     double                xUL,yUL;
     int                   xCells, yCells;       //cells
     double                xCellSize, yCellSize; //cell size
     double                xmin,xmax,ymin,ymax;  //current image extent
     const char            *pszWKT = NULL;
     char                  *pszWKT_nc = NULL;
     OGRSpatialReference   oSRS;
     char                  *pszProj4 = NULL;

     int                   gridID, idIndex;
     double                value;

     int                   *grdIndex = NULL; 


     timeIndex = timeIndexHash[dayTimeStr];
     printf ("\tReading %s   time step: %d\n",dayTimeStr.c_str(), timeIndex );
     printf ("\t\tFile: %s\n",imageFileName.c_str() );


     
     /************************************
     *         get image into array      *
     *************************************/
     if ( imageFileName.find ( "_snd_" ) != string::npos  )
     {
         xCells = cols_SND;
         yCells = rows_SND;

         grdIndex = grdIndex_SND;    //for Sounder
     }
     else 
     {
         xCells = cols_IMG;
         yCells = rows_IMG;

         grdIndex = grdIndex_IMG;    //for Imager
     }
      
     poImage = (double *) CPLCalloc(sizeof(double),xCells*yCells);


     if ( imageFileName.find ( "Z.grb" ) != string::npos  )
     {

         /***********************
         *   Open GOES image    *
         ***********************/
         poRDataset = (GDALDataset *) GDALOpen( imageFileName.c_str(), GA_ReadOnly );
         if( poRDataset == NULL )
         {
            printf( "\tError: Open GOES raster file failed: %s.\n", imageFileName.c_str() );
            exit( 1 );
         }

         // check rows and columns of the image
         printf( "\tGOES image dimensions:  %d X %d, fixed GOES image dimensions: %d X %d.\n", poRDataset->GetRasterXSize(), poRDataset->GetRasterYSize(), xCells, yCells );

         if ( xCells != poRDataset->GetRasterXSize() || yCells != poRDataset->GetRasterYSize() )
         {
             printf( "\tError: Open GOES image dimensions do not match fixed GOES image dimensions.\n" );
             exit( 1 );
         }


         /*********************************************
          *   Image data type has to be GDT_Float64   *
          *********************************************/
         //get the band 1 image from the image and make sure that it is GByte image
         poBand = poRDataset->GetRasterBand( 1 );  // band 1

         GDALDataType poBandType = poBand->GetRasterDataType();
         printf ("\t\tImage GDALDataType is: %d\n",poBandType);

         if ( poBandType != GDT_Float64 )
         {
            printf( "\tError: Image data type is not GDT_Float64.\n" );
            printf( "\tConvert it using gdal_translate utility to GDT_Float64.\n" );
            exit ( 1 );
         } 

         

         //image from UL corner
         if ( (poBand->RasterIO(GF_Read, 0,0,xCells,yCells,
           poImage,xCells,yCells,GDT_Float64,0,0)) == CE_Failure)
         {
             printf( "\tError in reading band 1 data from image: %s.\n", imageFileName.c_str() );
             CPLFree (poImage);
            exit( 1 );
         }


         //GDAL obtained image from UL to LR
         GDALClose( (GDALDatasetH) poRDataset );

     }   //grb file
     else if ( imageFileName.find ( "Z.txt" ) != string::npos  )
     {
         readGOESImageTXT ( poImage, imageFileName );   //image from LL to UR
     }


     /**********************************************************************************
     *     Allocate memory store pixel number and total value in each modeling grid    *
     ***********************************************************************************/
     gridPixels = gridCols*gridRows;
     gridIDs = (int *) CPLCalloc(sizeof(int),gridPixels);
     gridValues = (double *) CPLCalloc(sizeof(double),gridPixels);


     /**************************************************************
     *  read gridded domain image and index to get image values    *
     ***************************************************************/
     double x,y;
     int  col,row;   //in GOES image

     //loop through rasterized domain image one row at a time from top
     for( i = 0; i<yCells_grd; i++ )
     {
        for (j=0; j<xCells_grd; j++)
        {
           pixelIndex = i * xCells_grd + j;   //from UL
           gridID = poImage_grd[pixelIndex];   //domain grid ID from 1

           if (gridID > 0 && gridID <= gridPixels)
           {
              //get row and col indices in the model domain from LL corner
              int dmnRow = (int) ( floor ( ( gridID - 1 ) / gridCols) );   //start from 0
              int dmnCol = ( gridID - 1 ) % gridCols;                      //start from 0

              //printf ("\t\tgridID=%d    dmnRow=%d   dmnCol=%d\n",gridID,dmnRow,dmnCol);

              int geoIndex = grdIndex[pixelIndex];  //get the index to get the value image rowsXcols starting from 0
 
              if ( geoIndex != -999 )
              {
                 //compute geo-site row, col 
                 int satRow = (int) ( floor (geoIndex / xCells) );   //start from 0
                 int satCol = geoIndex % xCells;                     //start from 0

                 //printf ("\tgeoIndex=%d    row=%d   col=%d\n",geoIndex,satRow,satCol);

                 value = poImage[geoIndex];

                 if ( value != GOES_MISSIING_VALUE && value != 9999.0 )
                 {
                     gridIDs[gridID - 1] += 1;             //count Satellite cells
                     gridValues[gridID - 1] += value;      //sum the Satellite value for a domain grid
                 }
              }

           }  //check gridID
        }  //j
     }  //i

       
     //loop through modeling domain cells to fill netcdf array
     for (i=0; i<gridRows; i++)
     {
        for (j=0; j<gridCols; j++)
        {
           gridID = i * gridCols + j;    //grid cell ID - 1
           dataIndex = timeIndex * gridRows * gridCols + i * gridCols + j;   //netcdf variable array index

           //make sure that at least half of domain grid has values
           if ( gridIDs[gridID] * xCellSize_grd * yCellSize_grd >  halfGridCellArea )
           {
               value = gridValues[gridID] / gridIDs[gridID];    //take average for all GOES variables
               goesV[dataIndex] = value;
           }
           else
           {
               goesV[dataIndex] = MISSIING_VALUE;
           }
        }  //j
     } //i


     CPLFree (poImage);
     CPLFree (gridIDs);
     CPLFree (gridValues);

     printf ( "\t\tFinished processing the GOES image file.\n\n" );

}


/******************************************************
*  Compute one GOES variable data for the time range  *
******************************************************/
void computeGOESVariable ( int timeStep, int startMins, float *goesV, string inGOESVarFileNameFmt, string inGOESVarFileNameFmt_txt,
                           string gridRasterFile, string dataDir )
{
    string         startDateTime, endDateTime;    
    DIR            *dpFiles;             //image data directory
    struct dirent  *dirpFiles;
    string         dayStartStr,dayEndStr,dayMidStr;      //YYYYMMDD string
    long           dayStart, dayEnd, dayMid;             //day string number
    string         timeStartStr, timeEndStr;             //time strings
    int            timeStart, timeEnd, timeMid;          //times
    string         dayDirName;           //gp_YYYYMMDD string to store each day direcotry name
    string         tmp_str,imageStr;
    string         imageFileName;        //image file name with dayDirName + imageStr
    string         dayTimeStr;           //day and time string from the file name
    string         dayTimeStr_match;     //day and time string matches to the time step
    int            time1, time2;         //day time start and end 

    map<string,int>::iterator it;          // for timeIndexHash
    map<string,int>::reverse_iterator rit; //for timeIndexHash
    map <string, string>  fileNameList;    //map store day/time string and corresponding GOES image
    map <string, string>::iterator it_fileNameList;     // for fileNameList

    int             i;


    /****************************************************************
     *   Get first time step and last time step string information  *
     ****************************************************************/

    //get first time step string  
    it = timeIndexHash.begin ();
    startDateTime = it->first;

    //get last time step string
    rit = timeIndexHash.rbegin ();
    endDateTime = rit->first;        

    printf ("\t startDateTime = %s   endDateTime = %s\n",startDateTime.c_str(), endDateTime.c_str() );

    dayStartStr = startDateTime.substr(0, 8);
    dayStart = atol ( dayStartStr.c_str() );
  
    //get starting time
    timeStartStr = startDateTime.substr(8, 2);
    timeStart = atoi ( timeStartStr.c_str() );

    //get last day
    dayEndStr = endDateTime.substr(0, 8); 
    dayEnd = atol ( dayEndStr.c_str() );

    //get ending time
    timeEndStr = endDateTime.substr(8, 2);
    timeEnd = atoi ( timeEndStr.c_str() ); 

    /***************************************
     *   Process each day directoty files  *
     ***************************************/
    //initialize
    dayMidStr = string ( dayStartStr );
    dayMid = dayStart;
    while ( dayMid <= dayEnd )
    { 
       dayDirName = dataDir + string ( "gp_" );
       dayDirName.append ( dayMidStr ); 
       dayDirName = processDirName ( dayDirName ); 

       //open image day directory to all image files
       if((dpFiles  = opendir( dayDirName.c_str()) ) != NULL)
       {
          printf( "Process images in directory: %s\n",dayDirName.c_str() );
          //get the files from one day directory
          while ((dirpFiles = readdir(dpFiles)) != NULL)
          {
             tmp_str = string(dirpFiles->d_name);
             tmp_str = trim(tmp_str);
             if ( (tmp_str.find( inGOESVarFileNameFmt.c_str() )!=string::npos || tmp_str.find( inGOESVarFileNameFmt_txt.c_str() )!=string::npos ) && ( tmp_str.find( ".grb" )!=string::npos || tmp_str.find( ".txt" )!=string::npos ) )
             {
                imageStr = string (tmp_str);  //found the image 
                imageFileName = dayDirName + imageStr;
                printf( "\tFile: %s\n", imageFileName.c_str() );  

                if ( tmp_str.find( ".grb" )!=string::npos )
                {
                   dayTimeStr = getdayStrFromFileName ( imageStr, inGOESVarFileNameFmt );
                }
                else
                {
                   dayTimeStr = getdayStrFromFileName ( imageStr, inGOESVarFileNameFmt_txt );
                }

                dayTimeStr_match = matchGOESTimeStep ( dayTimeStr, startMins ); 
                //printf( "\tDay and time in file: %s   Time step set to: %s\n", dayTimeStr.c_str(),dayTimeStr_match.c_str() );

                fileNameList[dayTimeStr_match] = imageFileName;  //store time step for each file name in a hash table

             }  //if fileFmt*.grb or .txt exists
          }  //while dirpFiles

          closedir(dpFiles);
       }  // if dpFiles
       else 
       {
          printf( "Directory does not exist: %s\n",dayDirName.c_str() );
       }
       
       //if hour for the first year
       if ( dayMidStr.compare ( dayStartStr ) == 0 )
       {
          time1 = timeStart;
       }
       else
       {
          time1 = 0;
       }
            
       // if hour for the last year
       if ( dayMidStr.compare ( dayEndStr ) == 0 )
       {
          time2 = timeEnd;
       }
       else
       {
          time2 = 23;
       }

       for ( i=time1; i<=time2; i++ )
       {
          string hourStr = convert2NumsTo2Chars (i);
          dayTimeStr = dayMidStr + hourStr + "45";    
          it_fileNameList =  fileNameList.find( dayTimeStr );

          if ( it_fileNameList != fileNameList.end() ) 
          {
             readGOESImageFile ( dayTimeStr, it_fileNameList->second.c_str(), goesV, inGOESVarFileNameFmt, inGOESVarFileNameFmt_txt );
          }
          else
          {
             fillGOESMissingValues ( dayTimeStr, goesV );
          }
       } //i
            
       dayMidStr = getNextDayStr ( dayMidStr);
       dayMid = atol ( dayMidStr.c_str() );
       fileNameList.clear();
    }

}
