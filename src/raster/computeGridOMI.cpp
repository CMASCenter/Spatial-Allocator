/***************************************************************************
 * This program is used:
 *  1. to compute grid cell OMI satellite variable from downloaded OMI data
 *     from Gionanni web site:
 *  2. to output OMI satellite variables into a WRF format netcdf file and
 *     a text file.
 *  3. Time step - 1 day
 *
 * Written by the Institute for the Environment at UNC, Chapel Hill
 * in support of the UAE Modeling, 2009.
 *
 * By Limei Ran, Sept. 2009
 *
 * Usage:  computeGridOMI.exe
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
 *         OUTPUT_TEXT_FILE -- output text file containing GRIDID and satellite value
 *         OUTPUT_NETCDF_FILE -- output WRF NetCDF file containing satellite value

***********************************************************************************/
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <set> 

#include "sa_raster.h"
#include "commontools.h"
#include "geotools.h"

float SAT_MISSIING_VALUE; 

void setSelOMIVariables ( int index );
void computeSatVariable ( string imageFileName, string varName, float32 *satV, gridInfo imageInfo );

//gloabl variables to set satellite data variables
const int             timeStep = 86400;       // time step to process satellite data: 24X60X60 seconds
const int             timeStrLength = 19;        //time string length in WRF Netcdf output 

vector<string>        inSatVars;           //store selected Sat. variable names
int                   gridRows,gridCols;   //modeling grid rows and columns to compute grid IDs
GDALDataset           *poGrid;             //store rasterized grid domain image
GDALRasterBand        *poGridBand;         //band 1 for rasterized grid domain image
double                xCellSize_grd, yCellSize_grd; //standard cell size from grid domain raster
int                   xCells_grd, yCells_grd;       //cells for grid domain raster
double                halfGridCellArea;                     //domain grid area
double                xMin_grd,xMax_grd,yMin_grd,yMax_grd;  //modeling domain extent
OGRSpatialReference   oSRS_grd;
char                  *pszProj4_grd = NULL;

/***************************
*********** MAIN  **********
***************************/
int main(int nArgc, char *argv[])
{
        string      shapeFile;       //temp shapeFile to be created
        gridInfo    grid;            //data structure to store a modeling domain information

        //print program version
        printf ("\nUsing: %s\n", prog_version);

        printf("Compute grid Satellite OMI variable information...\n\n");

       
        /******************************
        * input variables             *
        *******************************/
        string    tmp_str;
        string    dataDir, dayDir, satImageFile;               //directory containing preprocessed Sat. data
        string    startDateTime, endDateTime;   //date and time range for the OMI data extraction
        
        string    satVarName;
        string    dataDate;
        string    outNetcdfFile, outTxtFile;                  //output file name
        std::ofstream        outTxtStream;            //output text file stream
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
        dataDate = string( getEnviVariable("DATA_DATE") );
        dataDate = trim( dataDate );
        printf ( "\tSatellite data date is:  %s\n",dataDate.c_str() );
        if ( dataDate.length() != 8 )  
        {
           printf ( "\tError: Data date format should be: YYYYMMDD\n" );
           exit ( 1 );
        } 
        if ( atol(dataDate.c_str()) == 0 )
        {
           printf ( "\tError: Data date should be all numbers\n" );
           exit ( 1 );
        }

        //printf( "Getting output NetCDF and text file name.\n");
        outTxtFile = string( getEnviVariable("OUTPUT_TEXT_FILE") );
        outTxtFile = trim ( outTxtFile );
        printf( "\tOutput text file is: %s\n", outTxtFile.c_str() );
        FileExists(outTxtFile.c_str(), 3 );  //the file has to be new.

        outNetcdfFile = string( getEnviVariable("OUTPUT_NETCDF_FILE") );
        outNetcdfFile = trim ( outNetcdfFile ); 
        printf( "\tOutput NetCDF file is: %s\n", outNetcdfFile.c_str() );
        FileExists(outNetcdfFile.c_str(), 3 );  //the file has to be new.

       /*********************************/
       /*    Create grid raster file    */
       /*********************************/
       string createdGridImage  = createGridImage ( grid );

       /*********************************/
       /*  Get grid info from Satellite */
       /*********************************/
       tmp_str = satImageFile + string ("|") + satVarName;  //HDF4 file variable has different dimension

       gridInfo imageInfo;
       if ( satImageFile.find ( ".hdf5" ) != string::npos )
       {
          //OMI NO2 L3 data which only has a data set call NO2TropCS30 and no attribute
          imageInfo = getHDF5VarInfo ( satImageFile, satVarName );
          
       }
       else 
       {
          imageInfo = getImageInfo ( tmp_str );  //hdf (4) file and other GDAL image file
       }

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

        times[0] = timeStep;   //1 day minutes
        string temp_str = dataDate; 
        temp_str.append ( ":0000" );
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
        tmp_str.append (dataDate);
        anyErrors( nc_put_att_text(ncid, time_id, "description", tmp_str.size(), tmp_str.c_str() ) );
        anyErrors( nc_put_att_text(ncid, time_id, "units", 7, "Seconds") );
       
        dimIndex[0] = time_dim;
        dimIndex[1] = dateStr_dim;
        anyErrors( nc_def_var(ncid, "Times", NC_CHAR, 2, dimIndex, &timeStr_id) );
        anyErrors( nc_put_att_text(ncid, timeStr_id, "description", 21, "Time string (19 char)" ) );
        anyErrors( nc_put_att_text(ncid, timeStr_id, "units", 13, "yyyymmdd:hhmm") );

        
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

           if ( satImageFile.find ( ".hdf5" ) != string::npos )
           {
              //OMI NO2 L3 data which only has a data set call NO2TropCS30 and no attribute
              imageInfo = getHDF5VarInfo ( satImageFile, satVarName );
           }
           else
           {
             imageInfo = getImageInfo ( tmp_str );  //hdf (4) file and other GDAL image file
           }

           if ( imageInfo.attsStr[4].compare ("NULL" ) != 0 )
           {
              SAT_MISSIING_VALUE = atof ( imageInfo.attsStr[4].c_str() );  //0 to 4 are var attributes
           } 
           else
           {
              SAT_MISSIING_VALUE = 0.0;    //OMI NO2 L3 hdf5 file
           }

           //printf ( "\t SAT_MISSIING_VALUE=%lf\n", SAT_MISSIING_VALUE);
   
           computeSatVariable ( satImageFile, inSatVars[i], satV, imageInfo);  

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
        
           if ( imageInfo.attsStr[1].compare ( "NULL" ) == 0 )
           {
             imageInfo.attsStr[1] = string ( "molec/cm^2" ); 
           }
           anyErrors( nc_put_att_text(ncid, satV_id, "units", imageInfo.attsStr[1].size(), imageInfo.attsStr[1].c_str() ) );

           if ( imageInfo.attsStr[2].compare ( "NULL" ) != 0 )
           {
              attValue[0] = atof ( imageInfo.attsStr[2].c_str() );
           }
           else
           {
              attValue[0] = 1.0;
           }
           anyErrors( nc_put_att_float(ncid, satV_id, "scale_factor", NC_FLOAT, 1, attValue) ); 


           if ( imageInfo.attsStr[3].compare ( "NULL" ) != 0 )
           {
              attValue[0] = atof ( imageInfo.attsStr[3].c_str() );
           }
           else
           {
              attValue[0] = 0.0;
           }
           anyErrors( nc_put_att_float(ncid, satV_id, "add_offset", NC_FLOAT, 1, attValue) );  

           attValue[0] = MISSIING_VALUE;
           anyErrors( nc_put_att_float(ncid, satV_id, "missing_value", NC_FLOAT, 1, attValue) );
           
           anyErrors( nc_put_att_text(ncid, satV_id, "stagger", 1, "M") );

           // Leave define mode for output 
           anyErrors( nc_enddef (ncid) );

           //write variable
           anyErrors( nc_put_var_float(ncid, satV_id, satV) );

           
           printf("\tOpen output text file to store OMI information:  %s\n",outTxtFile.c_str());
           outTxtStream.open( outTxtFile.c_str() );
           if (! outTxtStream.good() )
           {
               printf( "\tError in opening output file: %s\n",outTxtFile.c_str() );
               exit ( 1 );
           }
           
           char  temp_char[100];
           tmp_str = string ( "GRIDID,ROW,COL,OMI\n" );
           outTxtStream.write( tmp_str.c_str(), tmp_str.size() );
           for (i=0; i<grid.rows; i++)
           {
              for (j=0; j<grid.cols; j++)
              {
                  int gridID = i * grid.cols + j + 1;   
                  int pixelIndex = gridID - 1 ;   //netcdf variable array index
                  sprintf (temp_char, "%d,%d,%d,%.10f\n",gridID,i+1,j+1,satV[pixelIndex]); 
                  tmp_str = string ( temp_char );
                  outTxtStream.write( tmp_str.c_str(), tmp_str.size() );
              }  //j
           } //i

           //close text file
           outTxtStream.close ();

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


/*********************************************************
*  Compute satellite variable for modeling grids         *
**********************************************************/
void  computeSatVariable ( string imageFileName, string varName, float *satV, gridInfo imageInfo)
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
     printf( "\tSatellite image size is %dx%d\n", xCells, yCells );

     xUL = imageInfo.xmin; 
     yUL = imageInfo.ymax;
     printf( "\tSatellite image UL Origin = (%.6lf,%.6lf)\n", xUL,yUL );

     xCellSize = imageInfo.xCellSize;
     yCellSize = imageInfo.yCellSize;
     printf( "\tSatellite image Pixel Size = (%.3lf,%.3lf)\n", xCellSize, yCellSize );
      
     //compute extent of OMI image
     xmin = xUL;
     xmax = imageInfo.xmax;

     ymax = yUL;
     ymin = imageInfo.ymin;
     printf( "\tSatellite image extent:  minXY(%.3f,%.3f)   maxXY(%.3f,%.3f)\n", xmin,ymin,xmax,ymax );


     /***********************************************************************
      *   Get image projection to make sure that all images have the same   *
      ***********************************************************************/
     oSRS.importFromProj4( imageInfo.strProj4 );
     oSRS.exportToProj4( &pszProj4 );
     printf( "\tGrid projection: %s\n", pszProj4_grd);
     printf( "\tSatellite projection: %s\n", pszProj4 );

     /****************************************
     * Define grid to satellite projection   * 
     ****************************************/
     projPJ     proj4To, proj4From;
     projUV     xyP;

     //Grid domain projection
     string tmp_str = string ( pszProj4_grd );
     proj4From = pj_init_plus( tmp_str.c_str() );
     if ( proj4From == NULL )
     {
        printf( "\tInitializing grid domain PROJ4 projection failed.\n" );
        exit( 1 );
     }

     //Satellite projection
     tmp_str = string ( pszProj4 );
     proj4To = pj_init_plus ( tmp_str.c_str() );
     if (proj4To == NULL)
     {
        printf( "\tInitializing satellite PORJ4 projection failed.\n" );
        exit ( 1 );
     }


     /*************************************
     *    get satellite image into array  *
     *************************************/
     double *poImage = (double *) CPLCalloc(sizeof(double),xCells*yCells);

     if ( imageFileName.find ( ".hdf5" ) != string::npos )
     {
       //OMI NO2 L3 data which only has a data set call NO2TropCS30 and no attribute
       readHDF5SatVarData (imageFileName, varName, poImage);
     }
     else
     {
        readHDF4SatVarData (imageFileName, varName, poImage);   //OMI L2G and L3 data
     }

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

        //get cell center point y and compute row in satellite image
        y = yMax_grd - i * yCellSize_grd - yCellSize_grd / 2.0;
        for (j=0; j<xCells_grd; j++)
        {
           gridID = poImage_grd[j] ;
           if (gridID > 0 && gridID <= gridPixels)
           {
              //get cell center point x and compute col in satellite image
              x = xMin_grd + j * xCellSize_grd + xCellSize_grd / 2.0;
                 
              xyP = projectPoint ( proj4From, proj4To, x, y);
              double longX = xyP.u;
              double latY  = xyP.v;

              col = (int) (floor ((longX - xmin) / xCellSize));  //start from 0 LL to UR like NetCDF
              row = (int) (floor ((latY - ymin) / yCellSize));   //start from 0 LL to UR like NetCDF
              //printf ("\t  col=%d   row=%d    long=%lf  lat=%lf\n", col,row, longX, latY);
                  
              if ( (col >=0 && col < xCells) && (row >= 0 && row < yCells) )
              {
                 pixelIndex = xCells * row + col;
                 value = poImage[pixelIndex];     //get satellite image value

                 if ( value != SAT_MISSIING_VALUE )
                 {
                    gridIDs[gridID - 1] += 1;             //count Satellite cells                          
                    gridValues[gridID - 1] += value;      //sum the Satellite value for a domain grid
                 }
              }  //check row, col
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

     printf ( "\tFinished processing satellite image file.\n\n" );

}
