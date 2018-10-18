/***************************************************************************
 * This program is used:
 *  1. to compute grid cell OMI L2 variables from downloaded OMI L2
 *     product files from NASA Mirador site.
 *  2. to output satellite variables into a WRF format netcdf file.
 *  3. Time step -- File time
 *
 * Written by the Institute for the Environment at UNC, Chapel Hill
 * in support of the UAE and NASA ROSES projects, 2009.
 *
 * By Limei Ran, Dec 2009 - Mar. 2010
 *
 * Usage:  computeGridOMIL2.exe
 *
 * Environment Variables needed:
 *         GRID_ROWS  -- grid domain rows
 *         GRID_COLUMNS -- grid domain columns
 *         GRID_XMIN -- grid domain xmin
 *         GRID_YMIN -- grid domain ymin
 *         GRID_XCELLSIZE -- grid domain x grid size
 *         GRID_YCELLSIZE -- grid domain y grid size
 *         GRID_PROJ -- grid domain proj4 projection definition
 *         DATA_DIR -- directory contains MODIS06 L2 cloud and MODIS03 L1 geo-location files
 *         SATELLITE_VARIABLE -- satellite HDF4 variables (separated by comma) to be extracted
 *         START_DATE --  start date and time YYYYMMDDHHMM
 *         END_DATE -- end date and time YYYYMMDDHHMM
 *         OUTPUT_NETCDF_FILE -- output WRF NetCDF file containing satellite value

***********************************************************************************/
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <set> 

//#include <algorithm>
//#include <ANN/ANN.h>

#include "sa_raster.h"
#include "commontools.h"
#include "geotools.h"

float SAT_MISSIING_VALUE; 

bool computeGridSatIndexes ( gridInfo newRasterInfo, string startDateTime, string imageFileName );
void  computeSatVariable ( GUInt32 *poImage_grd, string imageFileName, string varName, float *satV,
                           gridInfo imageInfo, gridInfo newRasterInfo, gridInfo grid);

//gloabl variables to set satellite data variables

double                searchRadius = 65000.0;   //set maximum search radius for the nearest sat point

const int             timeStrLength = 19;    //time string length in WRF Netcdf output 

int                   *grdIndex=NULL;        //store grid index in image grids for multiple variable processing


/******************************************************
************************* MAIN  ***********************
*******************************************************/
int main(int nArgc, char *argv[])
{
        gridInfo           grid;            //data structure to store a modeling domain information
        vector<string>     inSatVars;       //store selected Sat. variable names
        GDALDataset        *poGrid;         //store rasterized grid domain image
        GDALRasterBand     *poGridBand;     //band 1 for rasterized grid domain image
      

        //print program version
        printf ("\nUsing: %s\n", prog_version);

        printf("Compute grid Satellite OMI L2 variable information...\n\n");

       
        /******************************
        * input variables             *
        *******************************/
        string    dataDir;                      //directory containing preprocessed Sat. data
        string    startDateTime, endDateTime;   //date and time range for the OMI data extraction
        string    satVarName;
        string    outNetcdfFile;
        string    tmp_str;
        string    satImageFile;              
        string    dataDate;

        string    outTxtFile;                  //output file name
        string    lineStr;

        int       i,j,k;

	/***************************
	* Map projection variables *
	***************************/
        char    *pszProj4;
        int     proj;     //WRF projection type
        string  temp_proj;


	/*********************************
	* Obtain environmental variables *
	*********************************/
        printf ("Obtaining environment variables...\n");

        //no arguments to call this program
        if ( nArgc  > 1 )
        {   
           printf( "\nError:  No arguments are nedded.\n");
           printf( "\tUsage: computeGridOMIL2.exe\n");
           exit( 1 );
        }

        /******************************************
        *   get variables for domain definitions  *
        ******************************************/
        //rows and columns of the domain
        grid.rows = atoi( getEnviVariable("GRID_ROWS") );
        grid.cols = atoi( getEnviVariable("GRID_COLUMNS") );
        int gridRows = grid.rows; 
        int gridCols = grid.cols; 
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
           //created Shapefile will use GRIDID item name
           grid.polyID = string ( "GRIDID" );   //created domain Shapefile uses GRIDID
        }

        //get projection parameters from Proj4 string
        temp_proj = string (pszProj4);
        proj = getProjType(temp_proj);    //WRF has 4 projection types
        printf( "\tproj type = %d \n",proj);

        /*******************************************
        *       get Sat data dir and variables     *
        *******************************************/
        //printf( "Getting directory containing preprocessed Satellite L2 data.\n");
        dataDir = string( getEnviVariable("DATA_DIR") );
        dataDir = processDirName(dataDir );  //make sure that dir ends with path separator
        printf("\tSatellite data directory: %s\n",dataDir.c_str() );
        FileExists(dataDir.c_str(), 0 );  //the dir has to exist.

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

        //printf( "Getting day and time range to extract preprocessed GOES data.\n");
        startDateTime = string( getEnviVariable("START_DATE") );
        startDateTime = trim( startDateTime );
        endDateTime = string( getEnviVariable("END_DATE") );
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
       //OMI L2 aerosol product is in latlong and around 13x24 km at Nadir: 13/111.1 degree resolution
       gridInfo imageInfo;
       imageInfo.strProj4 = strdup ( "+proj=latlong +datum=WGS84" );
       imageInfo.xCellSize = 13/111.1;
       imageInfo.yCellSize = 13/111.1;

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


       /*****************************************
       * Get all image files to be processed    *
       ******************************************/
       printf("\nObtain files to be processed and total time steps...\n");

       tmp_str = string ( "OMI" );

       //vector for OMI L2 files: filename1,datetime1,filename2,datetime2...
       //vector for MODIS L2 and L1 files: L2_filename1,L1_filename1,datetime1,L2_filename2,L1_filename2,datetime2...

       vector<string> satFileList = obtainSatFileNames ( dataDir,startDateTime, endDateTime, tmp_str );
  
       /*****************************************************
       * Compute center and domain corner lat and long data *
       ******************************************************/ 
       printf("\nCompute netcdf extent and dimension cooridnates...\n");
       computeGridCornerCenterLatLong ( &grid );  

       /**********************************
       * Compute domain grid coordinates *
       ***********************************/
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
       int satVars_id[inSatVars.size()];  //define in a array
        
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
       defineWRFNC4Dimensions ( ncid, dateStr_len, west_east_len, south_north_len,
                               &time_dim, &dateStr_dim, &west_east_dim, &south_north_dim );

       /*******************************
       * Define output time variables *
       *******************************/
       defineWRFNCTimeVars ( ncid, time_dim, dateStr_dim, startDateTime, &time_id, &timeStr_id);

       /*******************************
       * Leave define mode for output *
       ********************************/
       anyErrors( nc_enddef (ncid) );

       //Write grid coordinate variables
       writeWRFGridCoordinates ( ncid, west_east_dim, south_north_dim, grid );

       free(grid.x);
       free(grid.y);
       free(grid.lat);
       free(grid.lon);

       /******************************
       * Set define mode for output  *
       *******************************/
       anyErrors( nc_redef (ncid) );

       /************************************************
       * Define satellite variables in NetCDF output   *
       *************************************************/
       size_t      dims_len[10];     //set maximum 10 additional dimensions besides times, rows, cols 
       int         dims_id[10];      //dim IDs for added dimensions besides first 3
       int         numADims;         //any added dimensions besides first three
       int         satV_id;

       //take the first satellite image
       satImageFile = satFileList[0];
       numADims = 0;  

       for ( i=0; i<inSatVars.size(); i++ )
       {
          printf( "\nDefine satellite variable in NetCDF output:  %s...\n",inSatVars[i].c_str() );
          imageInfo = getHDF5VarInfo ( satImageFile, inSatVars[i] );

          dimIndex[0] = time_dim;
          dimIndex[1] = south_north_dim;
          dimIndex[2] = west_east_dim;
          int  varDims = 3;


          //set defined dimensions
          if ( imageInfo.dims.size() > 2 )
          {
             setWRFNetCDFSatVarDimIndex ( ncid, dims_len, dims_id, &numADims, dimIndex, imageInfo );

             /*printf ("\nTest print numADims=%d\n",numADims);
             for (j=0; j<numADims;j++)
             {
                printf ("\tdims_len=%d     dims_id=%d\n",dims_len[j],dims_id[j]);
             }

             varDims = imageInfo.dims.size() + 1;
             */
          }

          //define Sat variable in the WRF NetCDF output file
          //printf ("\tvarDims=%d   3+numADims=%d\n",varDims, 3+numADims);

          satV_id = defineWRFNetCDFSatVar ( ncid, inSatVars[i], 3+numADims, dimIndex, imageInfo );
          satVars_id[i] = satV_id;    
       } //i
  

       /*******************************
       * Leave define mode for output *
       ********************************/
       anyErrors( nc_enddef (ncid) );

       /******************************/
       /*      Register format(s)    */
       /******************************/
       GDALAllRegister();


       /********************************************************/
       /*     Open and read rasterized grid domain image       */
       /********************************************************/
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

       int xCells_grd = newRasterInfo.cols;
       int yCells_grd = newRasterInfo.rows;

       totalSize = xCells_grd * yCells_grd;
       GUInt32 *poImage_grd = (GUInt32 *) CPLCalloc(sizeof(GUInt32), totalSize);

       if ( ( poGridBand->RasterIO( GF_Read, 0, 0, xCells_grd, yCells_grd,
              poImage_grd, xCells_grd, yCells_grd, GDT_UInt32, 0, 0 ) ) == CE_Failure )
       {
          printf( "\t\tError: Reading rasterized domain image failed: %s\n",gridRasterFile.c_str() );
          CPLFree (poImage_grd);
          exit( 1 );
       }


       /***********************************
       * NetCDF variable array pointers   *
       ************************************/
       int          timeSteps;   //total time steps for the time range
       float        *satV;

       /*****************************************
       * Loop through each satellite file and   *
       * Read each variable from sat data file  *
       * Define new variable and attributes     *
       * Write the variable in output           *
       *****************************************/
 
       //total dim size without time
       timeSteps = 0;
       for (j=0; j<satFileList.size(); j++)
       {
          //get image name
          satImageFile = satFileList[j];

          //process time information
          j = j+ 1;  //get date and time for the file
          dataDate = satFileList[j]; 

          int minutesPassed = getTimeMinutesPased ( dataDate, startDateTime );

          printf( "\nProcess satellite file: %s\n", satFileList[j].c_str() );
          printf( "\tMinutes passed from starting date %s: %d\n", startDateTime.c_str(),minutesPassed );

          /************************************************
          *   compute satellite image geolocation index   *
          *    in rasterized domain grid - grdIndex array *
          *************************************************/
          if ( computeGridSatIndexes ( newRasterInfo, dataDate, satImageFile ) )
          {
             //the grid domain and satellite image interset

             //get time string
             tmp_str = convertDate2OutputDateStr( dataDate );

             //write the time variables for this step
             float tmpTime = minutesPassed*60;
             writeWRF1StepTimeVariables (timeSteps, tmpTime, tmp_str, ncid, time_id, timeStr_id, dateStr_len);

             //process each variable
             for ( i=0; i<inSatVars.size(); i++ )
             {
                totalSize = south_north_len * west_east_len;
                printf( "\tCompute satellite variable: %s\n",inSatVars[i].c_str() );

                imageInfo = getHDF5VarInfo ( satImageFile, inSatVars[i] );

                //add variable dimensions other then x and y. 0 and 1 are dimensions for rows and cols                   
                //regrid all variables
                for (k=2; k<imageInfo.dims.size(); k++)
                {
                   totalSize = totalSize * imageInfo.dims[k]; 
                   printf ( "\tDimension %d: %d\n",k,imageInfo.dims[k] );
                }
                printf ( "\ttotalSize for output = %d\n",totalSize );

                //allocate memory for output sat variable 
                if ( (satV = (float*) calloc (totalSize, sizeof(float)) ) == NULL)
                {
                   printf( "Calloc satV failed.\n");
                   exit ( 1 );
                }

                SAT_MISSIING_VALUE = atof ( imageInfo.attsStr[4].c_str() );  //0 to 5 are var attributes
          
                //printf ( "\t SAT_MISSIING_VALUE=%lf\n", SAT_MISSIING_VALUE);

                computeSatVariable ( poImage_grd, satImageFile, inSatVars[i], satV, imageInfo, newRasterInfo, grid );  

                //write satellite variables
                //anyErrors( nc_put_var_float(ncid, satV_id, satV) );
                  
                //define array to write for one time step
                size_t  start [imageInfo.dims.size() + 1];
                size_t  count [imageInfo.dims.size() + 1];

                getStartArraytoWriteNetCDF ( start, imageInfo, timeSteps );
                getCountArraytoWriteNetCDF ( count, imageInfo, south_north_len, west_east_len );

                /*
                for ( k=0; k<imageInfo.dims.size()+1; k++)
                {
                   printf ("\tArray to write:\n");
                   printf ( "\t\tstart = %d   count = %d\n", start[k], count[k] );
                }  
                */
                 
                anyErrors ( nc_put_vara_float ( ncid, satVars_id[i], start, count, satV) );
                printf( "\tWrited satV\n\n" );

                free ( satV );

             }  //i satellite variables

             timeSteps ++;  //one file is one time step

          }  //process the file
          else 
          {
             printf ( "\tSkip this file: satellite image and model domain do not intersect.\n" );
          }
          
          free  (grdIndex);
       }  //satellite files  j

       CPLFree (poImage_grd);
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
*  Compute rasterized grid Indexes in satellite       *
*  geolocation array                                  *
******************************************************/
bool computeGridSatIndexes ( gridInfo newRasterInfo, string startDateTime, string imageFileName )
{
     string          tmp_str;


     printf ( "\tCompute rasterized grid indexes in: %s...\n", imageFileName.c_str() );

     /**************************************************
     * get OMI L2  latitude and longitude              *
     ***************************************************/
     tmp_str =  string ("Latitude");  //HDF5 file variable - Latitude
     gridInfo imageInfoLat = getHDF5VarInfo ( imageFileName, tmp_str);

     int rows = imageInfoLat.rows;
     int cols = imageInfoLat.cols;
     printf( "\tSatellite image geolocation size is %dx%d\n", rows, cols );

     /******************************************************
     *    get satellite geolocation array and time array  *
     *****************************************************/
     double *latP = (double *) CPLCalloc(sizeof(double),rows*cols);
     double *longP = (double *) CPLCalloc(sizeof(double),rows*cols);

     tmp_str = string ( "Latitude" );
     readHDF5SatVarData (imageFileName, tmp_str, latP);

     tmp_str = string ( "Longitude" );
     readHDF5SatVarData (imageFileName, tmp_str, longP);

     /*****************************************
     * Allocate memory for image index array  *
     *****************************************/
     int xCells_grd = newRasterInfo.cols;
     int yCells_grd = newRasterInfo.rows;
     int totalSize = xCells_grd * yCells_grd;

     //allocate memory to store geolocation index for the rasterized domain grids
     if ( (grdIndex = (int*) calloc (totalSize, sizeof(int)) ) == NULL)
     {
        printf( "Calloc grdIndex variable failed.\n");
        exit ( 1 );
     }

     bool gotIndex = computeDomainGridImageIndex( grdIndex, longP, latP, imageInfoLat, newRasterInfo, searchRadius ); 

     CPLFree (latP);
     CPLFree (longP);

     return gotIndex;
}



/*********************************************************
*  Compute satellite variable for modeling grids         *
**********************************************************/
void  computeSatVariable ( GUInt32 *poImage_grd, string imageFileName, string varName, float *satV, 
                           gridInfo imageInfo, gridInfo newRasterInfo, gridInfo grid)
{

     printf ("\n\tReading satellite image variable: %s : %s\n",imageFileName.c_str(),varName.c_str() );


     /******************************
      *   Get satllite image  Info *
      *****************************/
     // get rows and columns of the image
     int xCells = imageInfo.cols;
     int yCells = imageInfo.rows;
     //printf( "\tSatellite image size is %dx%d\n", yCells, xCells );

     /*************************************
     *    get satellite image into array  *
     *************************************/
     //total dimension size
     int  totalSize = 1;
     
     if ( imageInfo.dims.size() < 2 || imageInfo.dims.size() > 4 )
     {
        printf ("\tError: the program only process satellite images with dimension size 2 to 4.\n");
        exit ( 1 );
     }

     for ( int i=0; i<imageInfo.dims.size(); i++ )
     {
        totalSize *= imageInfo.dims[i];
     }
     //printf ( "\tTotal satellite image dimension size = %d\n", totalSize );

     double *poImage = (double *) CPLCalloc(sizeof(double), totalSize);

     readHDF5SatVarData (imageFileName, varName, poImage);


     /******************************************************
     *     Compute model grid satellite image values      *
     ******************************************************/

     computeGridSatValues ( poImage_grd, grdIndex, satV, poImage, imageInfo, newRasterInfo, grid );

     CPLFree (poImage);

     //printf ( "\tFinished processing satellite image file.\n\n" );

}
