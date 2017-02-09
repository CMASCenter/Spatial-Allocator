/***************************************************************************
 * This program is used:
 *  1. to compute grid cell MODIS L2 cloud variable from downloaded MODIS L2
 *     cloud product files from NASA LAADS site.
 *  2. to output satellite variables into a WRF format netcdf file.
 *  3. Time step -
 *
 * Written by the Institute for the Environment at UNC, Chapel Hill
 * in support of the UAE Modeling, 2009.
 *
 * By Limei Ran, Oct. 2009
 *
 * Usage:  computeGridMODISL2Clouds.exe
 *
 * Environment Variables needed:
 *         GRID_ROWS  -- grid domain rows
 *         GRID_COLUMNS -- grid domain columns
 *         GRID_XMIN -- grid domain xmin
 *         GRID_YMIN -- grid domain ymin
 *         GRID_XCELLSIZE -- grid domain x grid size
 *         GRID_YCELLSIZE -- grid domain y grid size
 *         GRID_PROJ -- grid domain proj4 projection definition
 *         SATELLITE_DATA_FILE -- downloaded satellite data in hdf4 format
 *         SATELLITE_VARIABLE -- satellite HDF4 variable to be extracted
 *         DATA_DATE -- satellite data day
 *         OUTPUT_NETCDF_FILE -- output WRF NetCDF file containing satellite value

***********************************************************************************/
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <set> 
#include <algorithm>
#include <ANN/ANN.h>

#include "sa_raster.h"
#include "commontools.h"
#include "geotools.h"

float SAT_MISSIING_VALUE; 

bool computeGridSatIndexes (string startDateTime, int *grdIndex, string imageFileName, string varName, gridInfo imageInfo);
void setSelOMIVariables ( int index );
string getdayStrFromFileName ( string imageFileName );
void fillSatMissingValues ( int timeIndex, float *satV );
void computeSatVariable ( string dataDate, string imageFileName, string varName, float32 *satV, gridInfo imageInfo );

//gloabl variables to set satellite data variables
const int             timeStep = 5;         // time step to process satellite data: 24X60 minutes
const int             startMins = 00;        //OMI images are daily average 
const int             timeStrLength = 19;    //time string length in WRF Netcdf output 

vector<string>        inSatVars,inSatVarUnits,inSatVarDesc;   //store selected Sat. variable names
vector<int>           inSatVarIndex;       //store variable band index
map<string,int>       timeIndexHash;       //store time index for each time string      
int                   gridRows,gridCols;   //modeling grid rows and columns to compute grid IDs
GDALDataset           *poGrid;             //store rasterized grid domain image
GDALRasterBand        *poGridBand;         //band 1 for rasterized grid domain image
double                xCellSize_grd, yCellSize_grd; //standard cell size from grid domain raster
int                   xCells_grd, yCells_grd;       //cells for grid domain raster
double                halfGridCellArea;                     //domain grid area
double                xMin_grd,xMax_grd,yMin_grd,yMax_grd;  //modeling domain extent
OGRSpatialReference   oSRS_grd;
char                  *pszProj4_grd = NULL;

double                *timeScans, *timeScans03;
/***************************
*********** MAIN  **********
***************************/
int main(int nArgc, char *argv[])
{
        string      shapeFile;       //temp shapeFile to be created
        gridInfo    grid;            //data structure to store a modeling domain information

        //print program version
        printf ("\nUsing: %s\n", prog_version);

        printf("Compute grid Satellite MODIS06 L2 variable information...\n\n");

       
        /******************************
        * input variables             *
        *******************************/
        string    tmp_str;
        string    dataDir, dayDir, satImageFile;               //directory containing preprocessed Sat. data
        string    startDateTime, endDateTime;   //date and time range for the OMI data extraction
        
        string    satVarName;
        string    dataDate;
        string    outNetcdfFile, outTxtFile;                  //output file name
        string               lineStr;


	/***************************
	* Map projection variables *
	***************************/
        char    *pszProj4;
        int     proj;     //WRF projection type
        string  temp_proj;

        /***********************************
        * NetCDF variable array pointers   *
        ************************************/
        int        timeSteps;   //total time steps for the time range
        int        satDates[2],satHours[2],satMins[2];   //input date and time ranges
        float      *times;      //time second array since the starting time
        char       *timesStr;   //time string
        float      *satV;
        
        int        i,j,k;

	/*********************************
	* Obtain environmental variables *
	*********************************/
        printf ("Obtaining environment variables...\n");

        //no arguments to call this program
        if ( nArgc  > 1 )
        {   
           printf( "\nError:  No arguments are nedded.\n");
           printf( "\tUsage: computeGridOMI.exe\n");
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
        *       get Sat data file                  *
        *******************************************/
        //printf( "Getting satellite data file to be processed \n");
        satImageFile = string( getEnviVariable("SATELLIE_DATA_FILE") );
        satImageFile = trim ( satImageFile );
        printf("\tInput Satellite file is: %s\n", satImageFile.c_str() );
        FileExists(satImageFile.c_str(), 0 );  //the file has to exist.

        //printf( "Getting satellite data file variable to be extracted \n");
        satVarName = string( getEnviVariable("SATELLITE_VARIABLE") );
        satVarName = trim ( satVarName );
        printf("\tSatellite variable to be extracted is: %s\n", satVarName.c_str() );
        inSatVars = string2stringVector (satVarName, ","); 
        for ( i=0; i<inSatVars.size(); i++ )
        {
           tmp_str = inSatVars[i];
           inSatVars[i] = trim ( tmp_str );
        }

        //printf( "Getting date for the satellite data.\n");
        dataDate = getdayStrFromFileName ( satImageFile );
        printf ( "\tSatellite data date is:  %s\n",dataDate.c_str() );

        //printf( "Getting output NetCDF file name.\n");

        outNetcdfFile = string( getEnviVariable("OUTPUT_NETCDF_FILE") );
        outNetcdfFile = trim ( outNetcdfFile ); 
        printf( "\tOutput NetCDF file is: %s\n", outNetcdfFile.c_str() );
        FileExists(outNetcdfFile.c_str(), 3 );  //the file has to be new.

       /*********************************/
       /*    Create grid raster file    */
       /*********************************/
       string createdGridImage  = createGridImage ( grid );

       /*********************************/
       /*  Set grid info from Satellite */
       /*********************************/
       //MODIS L2 cloud product is in latlong and around 1 at Nadi: 1/111.1 degree resolution
       gridInfo imageInfo;
       imageInfo.strProj4 = strdup ( "+proj=latlong +datum=WGS84" );
       imageInfo.xCellSize = 1/111.1;
       imageInfo.yCellSize = 1/111.1;

       /*********************************/
       /*    Compute raster resolution  */
       /*********************************/
       double  rasterResolution =  computeRasterResolution ( imageInfo, grid );

       /*********************************/
       /*  Compute new raster info      */
       /*********************************/
        string no_shape;
        gridInfo newRasterInfo = computeNewRasterInfo ( no_shape, rasterResolution, grid);

       /*********************************/
       /*    resample grid raster       */
       /*********************************/
       string gridRasterFile = resampleRaster (createdGridImage, newRasterInfo);

      //delete created raster file
       deleteRasterFile ( createdGridImage );

       /**********************************
       * Compute times and timesStr data *
       **********************************/
        printf("\nObtain times in seconds from the start and time string in each step data...\n");

        //get date, hour, minute ranges in 2d arrays
        //timeSteps = getDateTimeRange(startDateTime, endDateTime, imageDates, satHours, satMins, timeStep, startMins);
        timeSteps = 1;

        // allocate memory for times in seconds array
        if ( (times = (float*) calloc (timeSteps, sizeof(float)) ) == NULL)
        {
           printf( "Calloc times failed.\n");
           exit ( 1 );
        }

        if ( (timesStr = (char *) calloc (timeSteps*timeStrLength, sizeof(char)) ) == NULL)
        {
            printf( "Calloc timesStr failed.\n");
            exit ( 1 );
        }

        //fill the two time arrays for WRF Netcdf output
        //timeIndexHash = fillTimeArrays (times, timesStr, startDateTime, endDateTime, timeStep, startMins);

        times[0] = timeStep;   //1 day minutes
        string temp_str = dataDate; 
        temp_str.append ( ":00" );
        strcpy (timesStr, temp_str.c_str() );   //asign time string 1 day only
       
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
        int satV_id;
        
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
        tmp_str.append ( startDateTime );
        anyErrors( nc_put_att_text(ncid, time_id, "description", tmp_str.size(), tmp_str.c_str() ) );
        anyErrors( nc_put_att_text(ncid, time_id, "units", 7, "Seconds") );

        dimIndex[0] = time_dim;
        dimIndex[1] = dateStr_dim;
        anyErrors( nc_def_var(ncid, "Times", NC_CHAR, 2, dimIndex, &timeStr_id) );
        anyErrors( nc_put_att_text(ncid, timeStr_id, "description", 21, "Time string (19 char)" ) );
        anyErrors( nc_put_att_text(ncid, timeStr_id, "units", 15, "yyyymmddhhmm:ss") );

        
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
        free(grid.x);
        free(grid.y);
        free(grid.lat);
        free(grid.lon);

        /******************************/
        /*      Register format(s)    */
        /******************************/
        GDALAllRegister();


        /***********************************************/
        /*     Open rasterized grid domain image       */
        /***********************************************/
        const char   *pszWKT = NULL;
        char         *pszWKT_nc = NULL;
        double       xUL;
        double       yUL;

        //open the grid domain image
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
        //printf( "\tGrid domain re-sampled pixel size = (%.3lf,%.3lf)\n", xCellSize_grd, yCellSize_grd );

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


        /*****************************************
        * Read each variable from sat data file  *
        * Define new variable and attributes     *
        * Write the variable in output           *
        *****************************************/
        //total dim size

        totalSize = time_len * south_north_len * west_east_len;

        for ( i=0; i<inSatVars.size(); i++ )
        {
           printf( "Compute satellite variable: %s\n",inSatVars[i].c_str() );

           //allocate memory for sat variable 
           if ( (satV = (float*) calloc (totalSize, sizeof(float)) ) == NULL)
           {
              printf( "Calloc satV failed.\n");
              exit ( 1 );
           }

           tmp_str = satImageFile + string ("|") + inSatVars[i];  //HDF4 file variable has different dimension
           imageInfo = getHDF4VarInfo ( tmp_str );

           SAT_MISSIING_VALUE = atof ( imageInfo.attsStr[4].c_str() );  //0 to 4 are var attributes
          
           //printf ( "\t SAT_MISSIING_VALUE=%lf\n", SAT_MISSIING_VALUE);
   
           computeSatVariable ( dataDate, satImageFile, inSatVars[i], satV, imageInfo);  

           //put the output in define mode
           anyErrors( nc_redef (ncid) );

           //define value array
           float   attValue[1];

           dimIndex[0] = time_dim;   
           dimIndex[1] = south_north_dim;
           dimIndex[2] = west_east_dim;
           anyErrors( nc_def_var(ncid, inSatVars[i].c_str(), NC_FLOAT, 3, dimIndex, &satV_id) );           

           anyErrors( nc_put_att_int(ncid, satV_id, "FieldType", NC_INT, 1, fieldtype) ); 
           anyErrors( nc_put_att_text(ncid, satV_id, "MemoryOrder", 3, "XY ") );
           anyErrors( nc_put_att_text(ncid, satV_id, "description", imageInfo.attsStr[0].size(), imageInfo.attsStr[0].c_str() ));
           anyErrors( nc_put_att_text(ncid, satV_id, "units", imageInfo.attsStr[1].size(), imageInfo.attsStr[1].c_str() ) );

           attValue[0] = atof ( imageInfo.attsStr[2].c_str() );

           anyErrors( nc_put_att_float(ncid, satV_id, "scale_factor", NC_FLOAT, 1, attValue) ); 

           attValue[0] = atof ( imageInfo.attsStr[3].c_str() );
           anyErrors( nc_put_att_float(ncid, satV_id, "add_offset", NC_FLOAT, 1, attValue) );  

           attValue[0] = MISSIING_VALUE;
           anyErrors( nc_put_att_float(ncid, satV_id, "missing_value", NC_FLOAT, 1, attValue) );
           
           anyErrors( nc_put_att_text(ncid, satV_id, "stagger", 1, "M") );

           // Leave define mode for output 
           anyErrors( nc_enddef (ncid) );

           //write variable
           anyErrors( nc_put_var_float(ncid, satV_id, satV) );

           
           free ( satV );
 
        }

        GDALClose( (GDALDatasetH) poGrid );
        
        /*******************************************************/
        /*  Close and delete rasterized grid domain image file */
        /*******************************************************/
        deleteRasterFile ( gridRasterFile ); 


	/**************************
	* Store global attributes *
	**************************/
        string title = string ( "GRIDDED OMI DATA FROM computeGridOMI.exe" );
        writeWRFGlobalAtributes ( ncid, title, grid, dataDate );


	/********************
	* Close netCDF file *
	********************/
        anyErrors( nc_close(ncid) );

        
        printf( "Finished writing output file: %s\n\n", outNetcdfFile.c_str());
}


/*********************End of Main program****************************************************************/



/******************************************************
*  Get day and time string from OMI image file name  *
******************************************************/
string getdayStrFromFileName ( string imageFileName )
{

     int       i,j;
     string    dayTimeStr;
     long      value;

     string    dayStr, timeStr;

    
     i = imageFileName.rfind("/", imageFileName.length() );
     if (i != string::npos)
     {
        imageFileName.erase(0,i+1);  //get rid of dir part
     }

     //printf ("\tSatellite file name: %s\n", imageFileName.c_str() );

     //MODIS L2 cloud products
     if ( imageFileName.find( "06_L2.A" ) != string::npos )
     {
        dayStr = imageFileName.substr(10, 7);
        timeStr = imageFileName.substr(18, 4);
     }
     else //MODIS 03 L1 geolocation prodcuts
     {
        dayStr = imageFileName.substr(7, 7);
        timeStr = imageFileName.substr(15, 4);
     }

     string newDayStr = getDateFromJulian ( dayStr );

     dayTimeStr = newDayStr + timeStr;


     //printf ("\tSatellite acquisition date is: %s\n", dayTimeStr.c_str() );

      if ( atol (dayTimeStr.c_str() ) == 0 )
      {
         printf ("\tMODIS L2 cloud product name should be:\n" );
         printf ("\t\t MOD06_L2.AYYYYDDD.HHMM.VVV.YYYYDDDHHMMSS.hdf or MYD06_L2.AYYYYDDD.HHMM.VVV.YYYYDDDHHMMSS.hdf\n" );
         printf ("\tMODIS 03 L1 geolocation product name should be:\n" );
         printf ("\t\t MOD03.AYYYYDDD.HHMM.VVV.YYYYDDDHHMMSS.hdf or MYD03.AYYYYDDD.HHMM.VVV.YYYYDDDHHMMSS.hdf\n" );
         exit ( 1 );
      }

      return   dayTimeStr;

}


/******************************************************
*  Compute rasterized grid Indexes in 5X5km or 1X1km  *
*  MODIS L2 Cloud product arrays                      *
******************************************************/
bool computeGridSatIndexes (string startDateTime, int *grdIndex, string imageFileName, string varName, gridInfo imageInfo)
{
     string          imageDate;
     string          dayStartStr,dayEndStr,dayMidStr;      //YYYYMMDD string
     string          dayStartStr_julian,dayEndStr_julian;  //YYYYDDD string - Julian string
     string          timeStartStr, timeEndStr;             //time strings
     long            dayTimeStart, dayTimeEnd, dayTimeMid; //day and time numbers
     string          tmp_str;

     int             i,j;


     printf ( "\tCompute rasterized grid indexes in: %s | %s...\n", imageFileName.c_str(), varName.c_str() );

     /**************************************************
     * get MODIS 06 L2 or 03 L1 latitude and longitude *
     ***************************************************/
     tmp_str = imageFileName + string ("|Latitude");  //HDF4 file variable - Latitude
     gridInfo imageInfoLat = getHDF4VarInfo ( tmp_str );

     /****************************************************
     * Define projection from satellite to model domain  *
     *****************************************************/
     projUV    xyP;
     projPJ    proj4To, proj4From;
     string    proj4Str;

     //satellite projection
     proj4Str = string(imageInfoLat.strProj4);
     //printf ( "\tProj4From = %s\n", proj4Str.c_str() );
     proj4From = pj_init_plus( proj4Str.c_str() );
     if ( proj4From == NULL )
     {
        printf( "\tInitializing Satellite Proj4 projection failed: %s.\n",  proj4Str.c_str() );
        exit( 1 );
     }

     //grid domain projection
    proj4Str = string( pszProj4_grd );
     //printf ( "\tProj4To = %s\n", proj4Str.c_str() );
     proj4To = pj_init_plus ( proj4Str.c_str() );
     if (proj4To == NULL)
     {
        printf( "\tInitializing grid domain Proj4 projection failed: %s\n", pszProj4_grd );
        exit ( 1 );
     }


     //get variable attributes
     float scaleF = atof ( imageInfoLat.attsStr[2].c_str() );
     float offset = atof ( imageInfoLat.attsStr[3].c_str() );
     SAT_MISSIING_VALUE = atof ( imageInfoLat.attsStr[4].c_str() );
     printf ( "\tLatitude 5kmX5km: SAT_MISSIING_VALUE = %lf   scale_factor = %f   add_offset = %f \n", SAT_MISSIING_VALUE,scaleF,offset);

     int rows = imageInfoLat.rows;
     int cols = imageInfoLat.cols;

    /*****************************************************
     *    get satellite 5kmX5km location and time array  *
     *****************************************************/
     double *latP = (double *) CPLCalloc(sizeof(double),rows*cols);
     double *longP = (double *) CPLCalloc(sizeof(double),rows*cols);

     tmp_str = string ( "Latitude" );
     readHDF4SatVarData (imageFileName, tmp_str, latP);

     tmp_str = string ( "Longitude" );
     readHDF4SatVarData (imageFileName, tmp_str, longP);


     //get time array
     tmp_str = imageFileName + string ("|Scan_Start_Time");
     gridInfo imageInfoTime = getHDF4VarInfo ( tmp_str );

     double TIME_MISSIING_VALUE = atof ( imageInfoTime.attsStr[4].c_str() );
     printf ("\tTIME_MISSIING_VALUE = %.13lf\n", TIME_MISSIING_VALUE);

/*     timeScans = (double *) CPLCalloc(sizeof(double),imageInfoTime.rows*imageInfoTime.cols);
     tmp_str = string ( "Scan_Start_Time" );
     readHDF4SatVarData (imageFileName, tmp_str, timeScans);
*/

     //get seconds since 1993-1-1 00:00:00.0 0
     double secondsStart = getSecondsFrom19930101( startDateTime );

     printf ( "\tSeconds since 19930101: %lf\n",secondsStart);

     /*****************************************************
     *    get satellite 1kmX1km location and time array  *
     *****************************************************/
     gridInfo  imageInfoLat03;
     double    *latP03, *longP03;
     int       rows03, cols03;
     bool      readMODIS03 = false;
     double    SAT_MISSIING_VALUE03;

     //1kmX1km array in 03 L1 products
     if ( imageInfo.rows != rows || imageInfo.cols != cols )
     {
        /*
        tmp_str = imageFileName03 + string ("|Latitude");  //HDF4 MODIS 03 L1 file variable - Latitude
        imageInfoLat03 = getHDF4VarInfo ( tmp_str );

        SAT_MISSIING_VALUE03 = atof ( imageInfoLat03.attsStr[4].c_str() );
        printf ( "\tLatitude 1kmX1km: SAT_MISSIING_VALUE03 = %lf\n", SAT_MISSIING_VALUE03);

        rows03 = imageInfoLat03.rows;
        cols03 = imageInfoLat03.cols;
        if ( imageInfo.rows == rows03 && imageInfo.cols == cols03 )
        { //1kmX1km array

           readMODIS03 = true;
           latP03 = (double *) CPLCalloc(sizeof(double),rows03*cols03);
           longP03 = (double *) CPLCalloc(sizeof(double),rows03*cols03);

           tmp_str = string ( "Latitude" );
           readHDF4SatVarData (imageFileName03, tmp_str, latP03);

           tmp_str = string ( "Longitude" );
           readHDF4SatVarData (imageFileName03, tmp_str, longP03);

           //allocate 03 time array
           timeScans03 = (double *) CPLCalloc(sizeof(double),rows03*cols03);

        }
        else
        { */
            printf ( "\tError: it is not 5km array variable: %s\n", varName.c_str() );
            exit ( 1 );
       // }
     }


     /**************************************
     * Project 5kmX5km lat and long arrays *
     ***************************************/
     int            nPts = rows*cols;   // actual number of data points
     ANNpointArray  dataPts;            // data points
     int            dim = 2;
     int            index;
     double         satXmin = 99999999.0;
     double         satXmax = -99999999.0;
     double         satYmin = 99999999.0;
     double         satYmax = -99999999.0;

     dataPts = annAllocPts(nPts, dim); // allocate data points

     //project lat and long into grid domain projection
     for (i=0; i<rows; i++)
     {
        for (j=0; j<cols; j++)
        {
           index = i*cols + j;
           if ( latP[index] != SAT_MISSIING_VALUE && longP[index] != SAT_MISSIING_VALUE )
           {
              float latY = latP[index] * scaleF + offset;
              float longX = longP[index] * scaleF + offset;
              xyP = projectPoint ( proj4From, proj4To, longX,latY);

              dataPts[index][0] = xyP.u;
              dataPts[index][1] = xyP.v;
              satXmin = min ( satXmin, xyP.u );
              satXmax = max ( satXmax, xyP.u );
              satYmin = min ( satYmin, xyP.v );
              satYmax = max ( satYmax, xyP.v );

              //printf ("%d,%lf,%lf\n",index+1,xyP.u,xyP.v );
           }
        }
     }

     CPLFree (latP);
     CPLFree (longP);

     
     printf ("\tSatellite data extent: x(%lf, %lf)  y(%lf, %lf)\n",satXmin,satXmax,satYmin,satYmax );

     //modeling grid and satellite image does not intersect
     if ( satXmin >= xMax_grd || satXmax <= xMin_grd ||  satYmin >= yMax_grd || satYmax <= yMin_grd )
     {
/*        CPLFree (timeScans);

        if ( readMODIS03 )
        {
           CPLFree (latP03);
           CPLFree (longP03);
           CPLFree (timeScans03);
        }
*/
        annDeallocPts ( dataPts );
        annClose();       // done with ANN

        return false;
     }

     /**************************************
     * Project 1kmX1km lat and long arrays *
     ***************************************/
 /*    ANNpointArray  dataPts03;            // data points
     int            nPts03;

     //store 1kmX1km points
     if ( readMODIS03 )
     {
        nPts03 = rows03 * cols03;
        dataPts03 = annAllocPts(nPts03, dim); // allocate data points

        //project lat and long into grid domain projection
        for (i=0; i<rows03; i++)
        {
           for (j=0; j<cols03; j++)
           {
              index = i*cols03 + j;
              if ( latP03[index] != SAT_MISSIING_VALUE03 && longP03[index] != SAT_MISSIING_VALUE03 )
              {
                 float latY = latP03[index];
                 float longX = longP03[index];
                 xyP = projectPoint ( proj4From, proj4To, longX, latY);

                 dataPts03[index][0] = xyP.u;
                 dataPts03[index][1] = xyP.v;

                 //printf ("%d,%lf,%lf\n",index+1,xyP.u,xyP.v);
              }
           } //j
        } //i

        CPLFree (latP03);
        CPLFree (longP03);
     }  //read MODIS 03 true
*/

     /************************************
     * Use ANN to get the nearest point  *
     *************************************/
     int           k = 1;       // number of nearest neighbors
     double        eps = 0.0;   // error bound
     ANNpoint      queryPt;     // query point
     ANNidxArray   nnIdx;       // near neighbor indices
     ANNdistArray  dists;       // near neighbor distances
     ANNkd_tree    *kdTree, *kdTree03;  // search structure

     queryPt = annAllocPt(dim); // allocate query point
     nnIdx = new ANNidx[k];     // allocate near neigh indices
     dists = new ANNdist[k];    // allocate near neighbor dists

     printf ( "\tBuild 5kmX5km point trees.\n" );
     kdTree = new ANNkd_tree( // build search structure
                             dataPts, // the data points
                             nPts, // number of points
                             dim); // dimension of space

 /*    if ( readMODIS03 )
     {
        printf ( "\tBuild 1kmX1km point trees.\n" );
        kdTree03 = new ANNkd_tree( dataPts03, nPts03, dim);
     }
*/
     int totalNums = 0;

     for (i=0; i<yCells_grd; i++)
     {
        double y = yMax_grd - i * yCellSize_grd - yCellSize_grd / 2.0;
        for (j=0; j<xCells_grd; j++)
        {
           index = i*xCells_grd + j;
           double x = xMin_grd + j * xCellSize_grd + xCellSize_grd / 2.0;
           queryPt[0] = x;
           queryPt[1] = y;

           //search 5kmX5km tree
           //if ( ! readMODIS03 )
           //{
              kdTree->annkSearch( // search
                                 queryPt, // query point
                                 k, // number of near neighbors
                                 nnIdx, // nearest neighbors (returned)
                                 dists, // distance (returned)
                                 eps); // error bound
              // print summary
              dists[0] = sqrt(dists[0]); // unsquare distance
              if ( dists[0] < 2800.0)   //get value within 3km
              {
                 //printf ( "\t5kmX5km tree: i=%d      nnIndex=%d        nnDist=%lf\n",index,nnIdx[0], dists[0] );
                 grdIndex[index] = nnIdx[0];
                 totalNums ++;
              }
              else
              {
                 grdIndex[index] = -999;
              }

           //}  //get 5kmX5km index
 /*          else
           {  //search 1kmX1km tree
              kdTree03->annkSearch( // search
                                 queryPt, // query point
                                 k, // number of near neighbors
                                 nnIdx, // nearest neighbors (returned)
                                 dists, // distance (returned)
                                 eps); // error bound
              // print summary
              dists[0] = sqrt(dists[0]); // unsquare distance
              //printf ( "\t1kmX1km tree: i=%d      nnIndex=%d        nnDist=%lf\n",index,nnIdx[0], dists[0] );
              if ( dists[0] < 1000.0)   //get value within 600m
              {
                 //printf ( "\t1kmX1km tree: i=%d      nnIndex=%d        nnDist=%lf\n",index,nnIdx[0], dists[0] );
                 grdIndex[index] = nnIdx[0];
                 totalNums ++;
              }
              else
              {
                 grdIndex[index] = -999;
              }

           }  //end of 1kmX1km tree search
*/
        }  //j
     }  //i

    printf ( "\tNumber of rasterized grid cells with index numbers: %d\n",totalNums );

    //if 1kmX1km, get time index in 5kmX5km array from MODIS 06 L2 file

 /*   if ( readMODIS03 )
    {
       totalNums = 0;
       for (i=0; i<nPts03; i++)
       {
          queryPt[0] = dataPts03[i][0];
          queryPt[1] = dataPts03[i][1];

          printf ( "\tx=%lf  y=%lf\n",queryPt[0] , queryPt[1]);
          //search 5kmX5km tree
          kdTree->annkSearch( // search
                           queryPt, // query point
                           k, // number of near neighbors
                           nnIdx, // nearest neighbors (returned)
                           dists, // distance (returned)
                           eps); // error bound
          // print summary
          dists[0] = sqrt(dists[0]); // unsquare distance
          printf ( "\t5kmX5km tree for time index: i=%d      nnIndex=%d        nnDist=%lf\n",index,nnIdx[0], dists[0] );
          if ( dists[0] < 3000.0)   //get value within 3km
          {
              //printf ( "\t5kmX5km tree for time index: i=%d      nnIndex=%d        nnDist=%lf\n",index,nnIdx[0], dists[0] );
              timeScans03[index] = nnIdx[0];
              totalNums ++;
           }
           else
           {
              timeScans03[index] = -999;
           }
       } //i

       printf ( "\tNumber of 1kmX1km grid cells got time index numbers: %d\n",totalNums );

       annDeallocPts ( dataPts03 );
       delete kdTree03;

    }  //if it is 1kmX1km array
*/

    annDeallocPts ( dataPts );
    annDeallocPt (queryPt );
    delete [] nnIdx;   // clean things up
    delete [] dists;
    delete kdTree;
    annClose();       // done with ANN

    return true;

}


/************************************************************
*  Set one time step data to MISSING_VALUE  for a variable  *
*************************************************************/
void fillSatMissingValues ( int timeIndex, float *satV )
{
     int  dataIndex;
     int  i,j;

     //timeIndex = timeIndexHash[dayTimeStr];
     //printf ("\tMissing %s   time step: %d\n",dayTimeStr.c_str(), timeIndex);
     for ( i=0; i<gridRows; i++ )
     {
        for (j=0; j<gridCols; j++ )
        {
           dataIndex = timeIndex * gridRows * gridCols + i * gridCols + j;
           satV[dataIndex] = MISSIING_VALUE;
        }  //j
     } //i
}


/*********************************************************
*  Compute satellite variable for modeling grids         *
**********************************************************/
void  computeSatVariable ( string dataDate, string imageFileName, string varName, float *satV, gridInfo imageInfo)
{
     vector<string>        varAtts;
     int                   gridPixels;          //total pixels in modeling grids
     int                   *gridIDs=NULL;       //array to store number of pixels in each modeling grids
     double                *gridValues = NULL;  //array to store total values in each modeling grids
     int                   dataIndex, pixelIndex;
     int                   i,j;

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


     printf ("Reading satellite image variable: %s : %s\n",imageFileName.c_str(),varName.c_str() );


     /******************************
      *   Get satllite image  Info *
      *****************************/
     /********************************************************************
      *   get rows, columns, cell size, and image extent of sat data   *
     ********************************************************************/
     // get rows and columns of the image
     xCells = imageInfo.cols;
     yCells = imageInfo.rows;
     printf( "\tSatellite image size is %dx%d\n", yCells, xCells );

     /*****************************************
     * Convert rasterized domain x-y to       *
     * row and col in 5X5km and 1X1km latlong *
     * arrays of MODIS L2 Cloud Procuts       *
     *****************************************/
     int *grdIndex;

     //allocate memory to store geolocation index for the rasterized domain grids
     int totalSize = xCells_grd * yCells_grd;
     if ( (grdIndex = (int*) calloc (totalSize, sizeof(int)) ) == NULL)
     {
        printf( "Calloc grdIndex variable failed.\n");
        exit ( 1 );
     }

     
     if (! computeGridSatIndexes (dataDate, grdIndex, imageFileName, varName, imageInfo) )
     {
        printf ( "Program exit: satellite image and model domain do not intersect.\n" );
        exit ( 1 );

     }


     /*************************************
     *    get satellite image into array  *
     *************************************/

     double *poImage = (double *) CPLCalloc(sizeof(double),xCells*yCells);

     readHDF4SatVarData (imageFileName, varName, poImage);

     //get variable attribute
     SAT_MISSIING_VALUE = atof ( imageInfo.attsStr[4].c_str() );
     printf ( "\tSAT_MISSIING_VALUE = %lf\n", SAT_MISSIING_VALUE);


     /***********************************************************************************
      *     Allocate memory store pixel number and total value in each modeling grid    *
      ***********************************************************************************/
     gridPixels = gridCols*gridRows;
     gridIDs = (int *) CPLCalloc(sizeof(int),gridPixels);
     gridValues = (double *) CPLCalloc(sizeof(double),gridPixels);

     /***************************************************************
      *  read one row at time from domain image due to the size     *
      ***************************************************************/
     //domain grid image may be too big to read once
     GUInt32 *poImage_grd = (GUInt32 *) CPLCalloc(sizeof(GUInt32),xCells_grd);

     double x,y;
     int  col,row;   //in satellite image

     int    timeIndex = 0;
     int    imageIndex;

     //loop through rasterized domain image one row at a time from top
    
     for( i = 0; i<yCells_grd; i++ )
     {
        if ( (poGridBand->RasterIO(GF_Read, 0, i,xCells_grd, 1,
              poImage_grd,xCells_grd,1,GDT_UInt32,0,0)) == CE_Failure)
        {
           printf( "\t\tError: Reading row = %d from domain grid image.\n",i+1);
           CPLFree (poImage_grd);
           exit( 1 );
        }

        for (j=0; j<xCells_grd; j++)
        {
           gridID = poImage_grd[j] ;
           if (gridID > 0 && gridID <= gridPixels)
           {
              pixelIndex =  i*xCells_grd + j;
              imageIndex = grdIndex[pixelIndex];  //get the index to get the value
              if ( imageIndex != -999 )
              {
                  value = poImage[imageIndex]; 
                  if ( value != SAT_MISSIING_VALUE )
                  {
                    gridIDs[gridID - 1] += 1;             //count Satellite cells                          
                    gridValues[gridID - 1] += value;      //sum the Satellite value for a domain grid
                  }
              }  //check missing
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

           //double area_cover_ratio = gridIDs[gridID] * xCellSize_grd * yCellSize_grd / (halfGridCellArea*2);
           //printf ("\t row=%d  col=%d   counts=%d  total=%lf  cover_ratio=%lf \n", i, j, gridIDs[gridID], satV[dataIndex], area_cover_ratio);

           if ( gridIDs[gridID] * xCellSize_grd * yCellSize_grd >=  halfGridCellArea )
           {
               value = gridValues[gridID] / gridIDs[gridID];    //take average for all satellite variables
               satV[dataIndex] = value;
           }
           else
           {
               satV[dataIndex] = MISSIING_VALUE;
           }
        }  //j
     } //i

     CPLFree (poImage);
     CPLFree (poImage_grd);
     CPLFree (gridIDs);
     CPLFree (gridValues);
     free  (grdIndex);

     printf ( "\tFinished processing satellite image file.\n\n" );

}
