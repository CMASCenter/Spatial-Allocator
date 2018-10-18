/***************************************************************************
 * This program is used: 
 *  1. to compute grid cell landuse percentage information from:
 *            USGS NLCD landuse, 
 *            USGS NLCD canopy,
 *            USGS NLCD imperviousness, 
 *            MODIS IGBP landuse.
 *            MODIS LAI
 *  2. to output landuse informaton into a text file table.
 *  3. to output landuse informaton into a WRF netcdf file.
 *
 * Written by EPA/ORD/CED
 * 
 * By L. Ran, 09/21/2018
 *
 * Usage:  computeGridLandUse_LAI_MODIS.exe
 *  
 * Environment Variables needed: 
 *         GRID_ROWS  -- grid domain rows
 *         GRID_COLUMNS -- grid domain columns
 *         GRID_XMIN -- grid domain xmin
 *         GRID_YMIN -- grid domain ymin
 *         GRID_XCELLSIZE -- grid domain x grid size 
 *         GRID_YCELLSIZE -- grid domain y grid size 
 *         GRID_PROJ -- grid domain proj4 projection definition
 *         INPUT_FILE_LIST -- Input land cover NLCD and MODIS file list
 *         INCLUDE_USGS_LANDUSE -- YES or NO to include USGS NLCD landuse data in computation
 *         INCLUDE_USGS_IMPERVIOUSNESS -- YES or NO to include USGS NLCD imperviousness data in computation
 *         INCLUDE_USGS_CANOPY -- YES or NO to include USGS NLCD canopy data in computation
 *         INCLUDE_MODIS -- YES or NO to include NASA MODIS IGBP landuse data in computation
 *         DATA_DIR -- MODIS LAI data directory
 *         SATELLITE_VARIABLE -- LAI variables to be re-grided
 *	   START_DATE -- start date in YYYYMMDD0000
 *	   END_DATE -- end date in YYYYMMDD0000
 *         OUTPUT_LANDUSE_TEXT_FILE -- text table output grid cell landuse information
 *         OUTPUT_LANDUSE_NETCDF_FILE -- netCDF output grid cell landuse information.  Only works for LCC now.

***********************************************************************************/
//for computing landuse info
#include <iostream>
#include <fstream>
#include <set>
#include <ogr_spatialref.h>
#include <vrtdataset.h>
#include <sstream>

#include "sa_raster.h"
#include "commontools.h"
#include "geotools.h"


//function for computing landuse info
static void  fillLandClassHashTables();
void countGridIDs();
void computeIntersectionBox(int xCells, int yCells, double xmin, double ymin, double xmax, double ymax,
                            double xMin_grd, double yMin_grd, double xMax_grd, double yMax_grd,
                            int *col1, int *row1, int *col2, int *row2,
                            int *col1_grd, int *row1_grd, int *col2_grd, int *row2_grd);
void   computeImpe_Cano( std::vector<string> imageFiles, string fileType );
void   computeLandUse(std::vector<string> imageFiles, string fileType);
void   computeMODIS( string modisFile ) ;
void   computeLAI( gridInfo infoGrd, string modisFile, gridInfo modisInfo, string laiFile, gridInfo laiInfo, std::vector<string> satVars, double *gridLAI, float *laiScaleFactors );


//define global variables
string                landType = "USGS NLCD Landuse Files";
string                impeType = "USGS NLCD Urban Imperviousness Files";
string                canoType = "USGS NLCD Tree Canopy Files";
string                modisType = "MODIS Landcover Tiled Files";

GDALDataset           *poGrid;                      //store rasterized 30m grid image
GDALDriver            *poDrive;
GDALRasterBand        *poGridBand;                  //band 1 for rasterized 30m grid image
double                xCellSize_grd, yCellSize_grd; //standard cell size from grid domain raster
int                   xCells_grd, yCells_grd;       //cells for grid domain raster 
double                xMin_grd,xMax_grd,yMin_grd,yMax_grd;  //modeling domain extent
OGRSpatialReference   oSRS_grd;
char                  *pszProj4_grd = NULL;

int                   gridRows,gridCols;                    //modeling grid rows and columns to compute grid IDs
int                   gridPixels;                           //total pixels in modeling grids
int                   *gridIDS=NULL;                        //array to store number of pixels in each modeling grids
double                *gridIMPE=NULL, *gridCANO=NULL;       //array to store grid percentage of imperviousness and canopy 
int                   *gridNLCD=NULL, *gridMODIS=NULL;      //array to store NLCD and MODIS data for modeling grid cells   

std::map<int,int>     nlcdIDS,modisIDS;             //hash tables to store landuse class IDs and index  


//USGS NLCD NODATA baclground value: 0 for LC and canopy   127 for Imperviousness
GByte USGS_NLCD_NODATAVALUE = 0;   //has to be > 0
GByte USGS_NLCD_IMPE_NODATAVALUE = 127;


//MODIS NODATA
GByte   MODIS_NODATAVALUE = 255;   //245: unclassified water and 0,17: water  
GByte   MODISLAI_NODATAVALUE = 249;     //249-255 not LAI and PAR values


//MODIS tile land cover class variable: may change depending on NASA MODIS LC products
string modisVarName = string ( "Land_Cover_Type_1" );


//match NLCD classes by array index: 0 or 127 is background
static int  nlcdClasses[] = {11,12,21,22,23,24,31,41,42,43,51,52,71,72,73,74,81,82,90,95};
    

//MODIS IGBP 255 is NODATA background
static int  modisClasses[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,0,18,19,20};

//MODIS and NLCD land cover class names
string                   lcNameStr = string ( "1: MODIS - 1 Evergreen Needleleaf forest,2: MODIS - 2 Evergreen Broadleaf forest,3: MODIS - 3 Deciduous Needleleaf forest,4: MODIS - 4 Deciduous Broadleaf forest,5: MODIS - 5 Mixed forest,6: MODIS - 6 Closed shrublands,7: MODIS - 7 Open shrublands,8: MODIS - 8 Woody savannas,9: MODIS - 9 Savannas,10: MODIS - 10 Grasslands,11: MODIS - 11 Permanent wetlands,12: MODIS - 12 Croplands,13: MODIS - 13 Urban and built-up,14: MODIS - 14 Cropland/Natural vegetation mosaic,15: MODIS - 15 Snow and ice,16: MODIS - 16 Barren or sparsely vegetated,17: MODIS - 17 Water,18: MODIS - Reserved (e.g. Unclassified),19: MODIS - Reserved (e.g. Fill Value ),20: MODIS - Reserved,21: NLCD - 11 Open Water,22: NLCD - 12 Perennial Ice/Snow,23: NLCD - 21 Developed (Open Space),24: NLCD - 22 Developed (Low Intensity),25: NLCD - 23 Developed (Medium Intensity),26: NLCD - 24 Developed High Intensity,27: NLCD - 31 Barren Land (Rock/Sand/Clay),28: NLCD - 41 Deciduous Forest,29: NLCD - 42 Evergreen Forest,30: NLCD - 43 Mixed Forest,31: NLCD - 51 Dwarf Scrub,32: NLCD - 52 Shrub/Scrub,33: NLCD - 71 Grassland/Herbaceous,34: NLCD - 72 Sedge/Herbaceous,35: NLCD - 73 Lichens,36: NLCD - 74 Moss,37: NLCD - 81 Pasture/Hay,38: NLCD - 82 Cultivated Crops,39: NLCD - 90 Woody Wetlands,40: NLCD - 95 Emergent Herbaceous Wetlands" );


const int   NLCD_CLASSES_NUM=20, MODIS_CLASSES_NUM=20;  //number classes in NLCD and MODIS no including NODATA
int         land_cat_len = 0;    //landuse items in output

const int   timeStrLength = 19;       //time string length in WRF Netcdf output
const int   nameStrLength = 49;       //name string length in netCDF output

/************************************************************************/
/*    fillLandClassHashTables ()                                        */
/************************************************************************/
static void   fillLandClassHashTables(  )
{  
    int      i;
    int      classID;

     printf( "\nFilling land classification hashtables for USGS, and MODIS landuse classes...\n" );
 

    int k = 0;

    //NLCD landuse classes: 20
    for (i=0; i<NLCD_CLASSES_NUM; i++)
    {
       k++;
       classID = nlcdClasses[i];
       nlcdIDS[classID] = i;
       printf( "\t%d.  NLCD classID=%d  nlcdIDS=%d\n",k, classID,nlcdIDS[classID]);
    }
    

    //MODIS IGBP landuse classes: 20
    for (i=0; i<MODIS_CLASSES_NUM; i++)
    {
       k++;
       classID =  modisClasses[i];
       modisIDS[classID] = i;
       printf( "\t%d.  MODIS classID=%d    modisIDS=%d\n",k, classID,modisIDS[classID]);
    }

}

/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main( int nArgc,  char* papszArgv[] )
{
    gridInfo              grid;            //store a modeling domain information
    string                gdalBinDir;           //GDAL directory

    //for computing landuse info and output into txt table
    string                dataFileList;          //file containing all processed NLCD file names
    string                modisFile;             //input MODIS data set
    string                modisFileList;         //list of MODIS tiles


    //MODIS LAI/FPAR
    string                dataDir;  
    string                satVarName;
    vector<string>        inSatVars;             //store selected Sat. variable names
    string                startDateTime, endDateTime;   //date and time range for the sat data extraction
    

    string                inUSGSLand,inUSGSImpe,inUSGSCano;   //inclusion indicator
    string                inNASALand;                         //inclusion indicator
    string                outTextFile, outNetcdfFile;         //output file names

    std::vector<string>   landFiles, impeFiles, canoFiles, modisFiles; 
    std::ifstream         imageStream;             //input NLCD image file list  
    string                fileName, fileType;      //for reading in all NLCD files

    string                tmp_str;
    
    std::ofstream         outTxtStream;            //output text file stream 
    string                lineStr;
 
    int                   i,j,k,n, nn, m;                    
    double                adfGeoTransform[6];
    double                xUL,yUL;
    double                intpart;

    double                gridArea,percent;

    //print program version
    printf ("\nUsing: %s\n", prog_version);

    //for outputing landuse info into netcdf format
    /*************************
    *    Grid variables      *
    *************************/
    int rows, cols;

    /***************************
    * Map projection variables *
    ***************************/
    char     *pszProj4;
    int      proj;     //WRF projection type
    string   temp_proj;

    /***********************************
    * NetCDF variable array pointers   *
    ************************************/
    int        *lu_list;    //landuse class list times*classes
    char       *lu_name;    //landuse class name
    float      *imperv;     //3d    time*rows*cols
    float      *canopy;     //3d    time*rows*cols
    float      *luf;        //landuse fraction array 4d  time*classes*rows*cols
    float      *lum;        //landuse mask 3d array  time*rows*cols
    float      *lud;        //landuse dominant class 3d  time*rows*cols

    //LAIFAR
    char       *day_str;    //day string for LAI
    int         numDays;    //number of days with LAI data

  
    printf("\nCompute grid landuse percentage information... \n\n"); 
/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
    GDALAllRegister();
    
/* -------------------------------------------------------------------- */
/*      Obtain environment variables                                    */
/* -------------------------------------------------------------------- */
    //no arguments to call this program
    if ( nArgc  > 1 )
    {
       printf( "\nError:  No arguments are nedded.\n");
       printf( "\tUsage:  computeGridLULC.exe\n");
       exit( 1 );
    }

    /**************************************************************
    *   get GDALBIN environment variable defined in .cshrc file   *
    **************************************************************/
    gdalBinDir =   string ( getEnviVariable("GDALBIN") );
    gdalBinDir =  processDirName( gdalBinDir );
    printf("GDAL bin directory: %s\n", gdalBinDir.c_str() );
    FileExists(gdalBinDir.c_str(), 0 );  //the dir has to exist

    /******************************************
    *   get variables for domain definitions  *
    ******************************************/
    printf( "Getting modeling grid domain variables...\n");

    //rows and columns of the domain
    grid.rows = atoi( getEnviVariable("GRID_ROWS") );
    grid.cols = atoi( getEnviVariable("GRID_COLUMNS") );

    gridRows = grid.rows;
    gridCols = grid.cols;
    rows = gridRows;
    cols = gridCols;
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

    printf( "\txcell=%.0f    ycell=%.0f \n",grid.xCellSize,grid.yCellSize);
    if (grid.xCellSize <= 0.0 || grid.yCellSize <= 0.0)
    {
       printf("\tError: domain grid cell size has to be > 0.\n");
       exit( 1 );
    }

    //max x and y
    grid.xmax = grid.xmin + grid.xCellSize * grid.cols;
    grid.ymax = grid.ymin + grid.yCellSize * grid.rows;

    //get projection info
    pszProj4 = getEnviVariable("GRID_PROJ");
    grid.strProj4 = getEnviVariable("GRID_PROJ");
    printf( "Grid domain projection proj4 = %s\n",pszProj4);

    //In case users want to use their own Shapefile and POLYGON_ID
    //those two environment variables are optional
    //Shapefile Name
    if (getenv ( "GRID_SHAPEFILE_NAME" ) != NULL)
    {
       grid.name = string ( getEnviVariable("GRID_SHAPEFILE_NAME") );
       printf("\tOutput Shapefile name: %s\n",grid.name.c_str());
    }
    else if (getenv ( "GRID_NAME" ) != NULL)
    {
       grid.name = string ( getEnviVariable("GRID_NAME") );
       printf("\tOutput grid name: %s\n",grid.name.c_str());
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
    proj = getProjType(temp_proj);
    printf( "proj type = %d \n",proj);


    /******************************************
    *   get satellite input data variables    *
    ******************************************/
    //printf( "Getting text file with processed NLCD image file list.\n");
    dataFileList = string( getEnviVariable("INPUT_FILE_LIST") );
    dataFileList = trim( dataFileList );
    printf("Processed NLCD image file list:  %s\n",dataFileList.c_str());
    FileExists(dataFileList.c_str(), 0 );  //the file has to exist.

    //printf( "Getting indicator to include USGS NLCD landuse data.\n");
    inUSGSLand =  string ( getEnviVariable("INCLUDE_USGS_LANDUSE") ); 
    inUSGSLand = stringToUpper( inUSGSLand );
    inUSGSLand = trim( inUSGSLand );

    //printf( "Getting indicator to include USGS NLCD imperviousness data.\n");
    inUSGSImpe = string( getEnviVariable("INCLUDE_USGS_IMPERVIOUSNESS") );   
    inUSGSImpe = stringToUpper ( inUSGSImpe );
    inUSGSImpe = trim( inUSGSImpe );

    //printf( "Getting indicator to include USGS NLCD canopy data.\n");
    inUSGSCano = string( getEnviVariable("INCLUDE_USGS_CANOPY") );
    inUSGSCano = stringToUpper( inUSGSCano );
    inUSGSCano = trim( inUSGSCano );

    //printf( "Getting indicator to include NASA MODIS IGBP landuse data.\n");
    inNASALand = string( getEnviVariable("INCLUDE_MODIS") );
    inNASALand = stringToUpper( inNASALand );
    inNASALand = trim( inNASALand );
   
    // QA selections
    printf( "Indicator to include USGS NLCD landuse data is: %s\n",inUSGSLand.c_str() ); 
    checkSelectionYesNo ( inUSGSLand );

    printf( "Indicator to include USGS NLCD imperviousness data is: %s\n",inUSGSImpe.c_str() );
    checkSelectionYesNo ( inUSGSImpe );

    printf( "Indicator to include USGS NLCD canopy data is: %s\n",inUSGSCano.c_str() ); 
    checkSelectionYesNo ( inUSGSCano );
   
    printf( "Indicator to include NASA MODIS IGBP landuse data is: %s\n",inNASALand.c_str() );
    checkSelectionYesNo ( inNASALand );


    
    /*******************************************************
    *       get Sat data dir and variables, date range     *
    ********************************************************/
    //printf( "Getting directory containing MODIS LAI/FPAR data.\n");
    dataDir = string( getEnviVariable("DATA_DIR") );
    dataDir = processDirName(dataDir );  //make sure that dir ends with path separator

    printf("\tMODIS LAI-FPAR data directory: %s\n",dataDir.c_str() );
    FileExists(dataDir.c_str(), 0 );  //the dir has to exist.


    //printf( "Getting MODIS LAI/FPAR file variable to be extracted \n");
    satVarName = string( getEnviVariable("SATELLITE_VARIABLE") );
    satVarName = trim ( satVarName );
    printf("\tSatellite variable to be extracted is: %s\n", satVarName.c_str() );

    inSatVars = string2stringVector (satVarName, ",");
    for ( i=0; i<inSatVars.size(); i++ )
    {
       tmp_str = inSatVars[i];
       inSatVars[i] = trim ( tmp_str );
    }

    
    //printf( "Getting day and time range to extract preprocessed satellite data.\n");
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

    /*****************************
    *   get output  variables    *
    ******************************/
    //printf( "Getting output text file name.\n");
    outTextFile = string( getEnviVariable("OUTPUT_LANDUSE_TEXT_FILE") );
    printf( "Output text file name: %s\n",outTextFile.c_str() );
    FileExists(outTextFile.c_str(), 3 );  //the file has to be new.


    //printf( "Getting output NetCDF file name.\n");
    outNetcdfFile = string( getEnviVariable("OUTPUT_LANDUSE_NETCDF_FILE") );
    printf( "Output NetCDF file name: %s\n",outNetcdfFile.c_str() );
    FileExists(outNetcdfFile.c_str(), 3 );  //the file has to be new.

   
/* ------------------------------------------------------------------------- */
/*     read in input landcover files to get image vector for each data set   */
/* ------------------------------------------------------------------------- */

    // read in image files to store them in vectors
    i = 0;
    imageStream.open( dataFileList.c_str() );
    if (imageStream.good() )
    {
       getline( imageStream, fileName);
       while ( !imageStream.eof() )
       {
          fileName = trim (fileName);  //get rid of spaces at edges
          i++;  //count the line number

          //get rid of empty line
          if ( fileName.size() == 0 )
          {
             goto newloop; 
          }

          if ( fileName.find(landType) !=string::npos || fileName.find(impeType) !=string::npos 
               || fileName.find(canoType) !=string::npos || fileName.find(modisType) !=string::npos )

          {
             //get the type of files
             fileType = fileName;
             printf( "\nline %d: %s\n",i,fileType.c_str() );
          }
          else
          {
             //put images into different vectors based on file type
             printf( "line %d: %s\n",i,fileName.c_str() );
             FileExists( fileName.c_str(), 0 );     //the file has to exist.
 
             if ( fileType.find(landType) !=string::npos )      
             {
                landFiles.push_back(fileName); 
             }
      
             if ( fileType.find(impeType) !=string::npos )
             {
                impeFiles.push_back(fileName);
             }

             if ( fileType.find(canoType) !=string::npos )
             {
                canoFiles.push_back(fileName);
             }

             if ( fileType.find(modisType) !=string::npos )
             {
                modisFiles.push_back(fileName);
             }
          }

          newloop: 
          getline( imageStream, fileName);
       }  // while loop

       imageStream.close(); 
    } // if good 
    else
    {
       printf ("\tError: Open file - %s\n", dataFileList.c_str());
       exit ( 1 );
    }


/*------------------------------------------*/
/*    obtain image file information         */
/*------------------------------------------*/
    string     createdGridImage;   //grid raster file
    string     gridRasterFile;     //projected grid raster file
    string     shapeFile, imageFile;
    gridInfo   imageInfo;

    //get image grid info
    if ( inUSGSLand.compare("YES") == 0 && landFiles.size() > 0 )
    {  
       imageFile = landFiles.at(0);    //use the first NLCD image files
       imageInfo = getImageInfo ( imageFile );
    }
    else if ( inUSGSImpe.compare("YES") == 0 && impeFiles.size() > 0 )
    { 
       imageFile = impeFiles.at(0);    //use the first NLCD image files
       imageInfo = getImageInfo ( imageFile );
    }
    else if ( inUSGSCano.compare("YES") == 0 && canoFiles.size() > 0 )
    {
       imageFile = canoFiles.at(0);    //use the first NLCD image files
       imageInfo = getImageInfo ( imageFile );
    }
    else if ( inNASALand.compare("YES") == 0 && modisFiles.size() > 0 )
    {  
       //!!! may need to change based on MODIS data sets
       //Get MODIS land cover file info

       imageFile = modisFiles.at(0);    //use the first MODIS image files
       imageInfo = getHDF4VarInfo (imageFile, modisVarName);
    } 
    else
    {
       printf ( "\tError: No input image data files from USGS NLCD landcover or MODIS Landcover\n" );      
       exit ( 1 );
    }

    //get projection from the image as the standard grid for following image processing
    //convert it to const char - projection obtained from projected/rastered image is a little different
    //from projection obtained from infoGrd in line 557

    tmp_str = string ( imageInfo.strProj4 );
    oSRS_grd.importFromProj4( tmp_str.c_str() );
    pszProj4_grd = strdup( tmp_str.c_str() );

  
/* --------------------------------------------------- */
/*     Extracted MODIS images out to be in one file    */
/* --------------------------------------------------- */
    gridInfo extModisInfo, gridMODISInfo;  //store info for extracted MODIS tiles for a domain

    if ( inNASALand.compare("YES") == 0 && modisFiles.size() > 0 )
    {
       //obtain extent of MODIS tile files.  Loop through each file
       extModisInfo = getAllMODISTileInfo ( modisFiles, modisVarName);

       printGridInfo ( extModisInfo );

       //create new MODIS TILE extent based on projected domain area

       gridMODISInfo = computeNewRasterInfo_fromImage ( extModisInfo.xCellSize, grid, extModisInfo); 


       //check MODIS data area
       if ( extModisInfo.xmin >= gridMODISInfo.xmax || extModisInfo.xmax <= gridMODISInfo.xmin ||  
            extModisInfo.ymin >= gridMODISInfo.ymax || extModisInfo.ymax <= gridMODISInfo.ymin )
       {
          printf ("Error: MODIS land cover files do not intersect with the modeling domain.\n");
          exit ( 1 );
       }

       //extract MODIS into one file
       modisFile = extractDomainMODISData ( modisFiles, modisVarName, gridMODISInfo );

       printGridInfo ( gridMODISInfo );
      
    }


    //if data processing is only for MODIS data, reassign extracted tiled MODIS file to the image info
    if ( imageFile.find( "MCD12Q1" ) != string::npos || imageFile.find( "MOD12Q1" ) != string::npos )
    {
       imageFile = modisFile;   
       imageInfo = gridMODISInfo;  
    }

    
    /*************************************/
    /*    prepare MODIS LAI/FPAR images  */
    /*************************************/

    tmp_str = string ( "MODIS" );

    ////vector for MODIS LAI/FPAR products: filename1,datetime1,filename2,datetime2...
    vector<string> laiFiles = obtainSatFileNames ( dataDir,startDateTime, endDateTime, tmp_str );

    

    /*********************************/
    /*    Compute raster resolution  */
    /*********************************/
    double  rasterResolution =  computeRasterResolution ( imageInfo, grid );


/* ----------------------------------------------------------- */
/*     created raster grid domain in MODIS projection          */
/* ----------------------------------------------------------- */
    gridInfo  infoGrd;
    if ( grid.name.find( ".shp" ) != string::npos )
    {
       //input shapefile
       shapeFile = grid.name;
    }
    else
    {
       //create shpefile from grid definition
       shapeFile = createGridShapes ( grid );
       //grid.name = shapeFile;
    }

    
    /*********************************/
    /*    Project Shapefile          */
    /*********************************/
    string projectedSHPFile =  projectShape ( gdalBinDir, shapeFile, imageInfo.strProj4 );


    /*********************************/
    /*  Compute new raster info      */
    /*********************************/
    infoGrd = computeNewRasterInfo ( projectedSHPFile, rasterResolution, grid);


    /*********************************/
    /*    Rasterize  Shapefile       */
    /*********************************/

    gridRasterFile = toRasterFile (gdalBinDir, infoGrd, imageFile, projectedSHPFile, grid);

    //delete created and projected Shapefiles
    deleteShapeFile ( shapeFile );
    deleteShapeFile ( projectedSHPFile );
    

/* ----------------------------------------------------------------- */
/*     Clip and project MODIS land cover data into NLCD projection   */
/* ----------------------------------------------------------------- */
    gridInfo  modisInfoNew, modisInfo;
    string    modisFileNew;

    if ( inNASALand.compare("YES") == 0 && modisFiles.size() > 0 )
    {
       //get projected MODIS image info
       tmp_str.clear();

       //modisInfoNew =  computeNewRasterInfo ( tmp_str, 250, infoGrd );

       printGridInfo ( gridMODISInfo );
       printGridInfo (infoGrd );
       
       modisInfoNew = computeNewRasterInfo_fromImage ( gridMODISInfo.xCellSize/2, gridMODISInfo, infoGrd);

       printGridInfo ( modisInfoNew );

       
       /*********************************/
       /*    Project MODIS image file   */
       /*********************************/
       
       //modisFileNew = projectImage ( gdalBinDir, modisFile, gridMODISInfo, modisInfoNew );


       //compare projection

       modisInfo = gridMODISInfo;

       if ( ! comparePROJ4GDALPRJ ( gridMODISInfo.strProj4, modisInfoNew.strProj4 ) )
       {
          modisFileNew = projectRasterFile ( modisFile, gridMODISInfo, modisInfoNew );
 
          //delete extracted modis image file
          deleteRasterFile ( modisFile );

          //projected MODIS file in NLCD projection
          modisFile = modisFileNew;
          modisInfo = modisInfoNew;
        }
        else
        {
           printf ("\tProjected grid domain has the same projection as MODIS land cover.\n");
        }
    }


/* ----------------------------------------- */
/*     Open rasterized grid domain file      */
/* ----------------------------------------- */

    printf( "\nProjected and rasterized grid image infomation:\n" ); 

    //open the grid domain image 
    poGrid = (GDALDataset *) GDALOpen( gridRasterFile.c_str(), GA_Update );
    if( poGrid == NULL )
    {
       printf( "   Error: Open raster file failed: %s.\n", gridRasterFile.c_str() );
       exit( 1 ); 
    }
    poGridBand = poGrid->GetRasterBand( 1 );  // band 1

    // get rows and columns of the image
    xCells_grd = infoGrd.cols;
    yCells_grd = infoGrd.rows;;
    printf( "\tGrid domain cell size is: %dx%dx%d\n", xCells_grd, yCells_grd, poGrid->GetRasterCount() );
      
    //get UL corner coordinates and grid size for the domain grid
    xUL = infoGrd.xmin;
    yUL = infoGrd.ymax;
    printf( "\tDoamin UL Origin = (%.6lf,%.6lf)\n", xUL,yUL );
         
    xCellSize_grd =  infoGrd.xCellSize;
    yCellSize_grd =  infoGrd.yCellSize;
    printf( "\tGrid domain pixel zize = (%.3lf,%.3lf)\n", xCellSize_grd, yCellSize_grd );
         
    //check if UL corner coordinates are the standard NLCD grid corner
    if ( xCellSize_grd == 30.0 && ( fabs( modf(xUL/xCellSize_grd,&intpart) )!= 0.5 || fabs (modf(yUL/yCellSize_grd,&intpart) ) != 0.5 ) )
    {  
        printf ( "\tError: UL corner not have NLCD corner coordinates: modf(fabs(xUL/30))=0.5 for: %s\n",gridRasterFile.c_str() );
        printf ( "         Regrid using gdal_translate utility.\n" );
        exit ( 1 );
    }
      
    //compute extent of band 1 domain grid image (only one band for all images)
    xMin_grd = xUL;
    xMax_grd = infoGrd.xmax;
      
    yMax_grd = yUL;
    yMin_grd = infoGrd.ymin;
    printf( "\tExtent:  minXY(%.3lf,%.3lf)   maxXY(%.3lf,%.3lf)\n", xMin_grd,yMin_grd,xMax_grd,yMax_grd );


    printf ( "\tProj4 for the domain raster image is: %s\n", pszProj4_grd);

/* -------------------------------------------------------------------- */
/*  Fill landuse class hash tables for index handling                   */
/* -------------------------------------------------------------------- */
    fillLandClassHashTables();


/* -------------------------------------------------------------------- */
/*     Allocate memory store 30m pixel number in each modeling grid           */
/* -------------------------------------------------------------------- */
    gridPixels = gridCols*gridRows;
    gridIDS = (int *) CPLCalloc(sizeof(int),gridPixels);
    countGridIDs();

/* -------------------------------------------------------------------- */ 
/*     compute USGS NLCD urban imperviousness percentage                */
/* -------------------------------------------------------------------- */ 
   if ( inUSGSImpe.compare("YES") == 0 )
   {
      printf( "\n\nUSGS NLCD Urban Imperviousness\n");
      gridIMPE = (double *) CPLCalloc(sizeof(double),gridPixels);
      computeImpe_Cano( impeFiles, impeType );
   }

/* -------------------------------------------------------------------- */
/*     compute USGS  NLCD canopy percentage                             */
/* -------------------------------------------------------------------- */
  if ( inUSGSCano.compare("YES") == 0 )
   {
      printf( "\n\nUSGS NLCD Tree Canopy\n");
      gridCANO = (double *) CPLCalloc(sizeof(double),gridPixels);
      computeImpe_Cano( canoFiles, canoType );
   }

/* ---------------------------------------------- */
/*     compute  NLCD landuse percentage           */
/* ---------------------------------------------- */
   if ( inUSGSLand.compare("YES") == 0 )
   {
      gridNLCD = (int *) CPLCalloc(sizeof(int),gridPixels * NLCD_CLASSES_NUM);
      printf( "\n\nUSGS NLCD Landuse\n");
      computeLandUse( landFiles, landType );
      poGrid->FlushCache();   //write modified domain grid image
   }

/* -------------------------------------------------------------------- */
/*     compute NASA MODIS IGBP landuse percentage                       */
/* -------------------------------------------------------------------- */
   if ( inNASALand.compare("YES") == 0 )
   {
      gridMODIS = (int *) CPLCalloc(sizeof(int), gridPixels * MODIS_CLASSES_NUM);
      printf( "\n\nNASA MODIS IGBP Landuse\n");
      computeMODIS( modisFile );
   }


/* -------------------------------------------------------------------- */
/*     Output domain grid land information to text and netcdf files     */
/* -------------------------------------------------------------------- */
   printf("Open output text file to store landuse information:  %s\n",outTextFile.c_str());
   outTxtStream.open( outTextFile.c_str() );
   if (! outTxtStream.good() )
   {
       printf( "\tError in opening output file: %s\n",outTextFile.c_str() );
       exit ( 1 );
   }
  
   //---------------- write title -------------------------
   //---------------- allocate arrays for netcdf output ---

   char  temp_char[100];
   lineStr = string ("GRIDID,ROW,COL");
   if ( inUSGSImpe.compare("YES") == 0 )
   {
       lineStr.append( ",IMPERV" );
       if ( (imperv = (float *) calloc (rows*cols, sizeof(float)) ) == NULL)
       {
          printf( "Calloc imperv failed.\n");
          exit ( 1 );
       }
   }

   if ( inUSGSCano.compare("YES") == 0 )
   {
       lineStr.append( ",CANOPY" );
       if ( (canopy = (float *) calloc (rows*cols, sizeof(float)) ) == NULL)
       {
          printf( "Calloc canopy failed.\n");
          exit ( 1 );
       }
   }

   //get number of landuse classes -- will always output 40 classes
   if ( inNASALand.compare("YES") == 0 || inUSGSLand.compare("YES") == 0 )
   {
      land_cat_len = MODIS_CLASSES_NUM + NLCD_CLASSES_NUM;  
   }

   printf( "Number of total landuse classes: %d - %d from MODIS and %d from NLCD\n",land_cat_len, MODIS_CLASSES_NUM,NLCD_CLASSES_NUM);

   if (land_cat_len > 0 )
   {
      //allocate mem to landuse class list
      if ( (lu_list = (int*) calloc (land_cat_len, sizeof(int)) ) == NULL)
      {
         printf( "Calloc lu_list failed.\n");
         exit ( 1 );
      }
  
      //allocate mem to landuse class name
      if ( (lu_name = (char *) calloc (land_cat_len*nameStrLength+1, sizeof(char)) ) == NULL)
      {
           printf( "Calloc lu_name failed.\n");
           exit ( 1 );
      }

      //allocate mem to landuse fraction array
      if ( (luf = (float*) calloc (land_cat_len*rows*cols, sizeof(float)) ) == NULL)
      {
          printf( "Calloc luf failed.\n");
          exit ( 1 );
      }

      //allocate mem to landuse mask array
      if ( (lum = (float*) calloc (rows*cols, sizeof(float)) ) == NULL)
      {
          printf( "Calloc lum failed.\n");
          exit ( 1 );
      }

      //allocate mem to landuse dominant class array
      if ( (lud = (float*) calloc (rows*cols, sizeof(float)) ) == NULL)
      {
          printf( "Calloc lud failed.\n");
          exit ( 1 );
      }
   }


   /*********************************
   * prepare land cover class data  *
   **********************************/

   vector<string> lcNameV = string2stringVector (lcNameStr, ",");
   int numClasses = lcNameV.size();

   k = 0; //landuse class index
   tmp_str.clear();

   if ( land_cat_len > 0 )
   {
      for (j=0; j< MODIS_CLASSES_NUM; j++)
      {
          sprintf( temp_char,",%d",j+1);
          lineStr.append( temp_char );
          lu_list[k] = j+1;

          //get landcover name
          int tmpSize = nameStrLength - lcNameV[j].size();  //spaces needed to fill

          for (i=1; i<=tmpSize; i++)
          {
              lcNameV[j].append (" ");   //fill to 49 chars
          } 

          tmp_str.append ( lcNameV[j] );  //MODIS names first 20
           
          k++;
      }


      for (j=0; j< NLCD_CLASSES_NUM; j++)
      {
          sprintf( temp_char,",%d",21+j);        
          lineStr.append( temp_char );
          lu_list[k] = 21+j;

          //get landcover name for NLCD last 20 names
          int tmpSize = nameStrLength - lcNameV[20+j].size();  //spaces needed to fill

          for (i=1; i<=tmpSize; i++)
          {
              lcNameV[20+j].append (" ");   //fill to nameStrLength chars 
          }

          tmp_str.append ( lcNameV[20+j] );  //NLCD names last 20
           
          k++;
      }
   }


   lineStr.append( "\n" );
   outTxtStream.write( lineStr.c_str(), lineStr.size() );
   
   strcpy (lu_name, tmp_str.c_str() );   //asign class name string


/* --------------------------------------------------------------- */
/*          output grid level landuse information                  */
/*          write data to arrays for netcdf output                 */
/*---------------------------------------------------------------- */

   int          gridID, col=0, row=0;
   double       waterP;   //total water class
   double       maxP;     //maximum percent class
   int          maxIndex;

   double testd = 0;
   for (i=0; i<gridPixels; i++)
   {
      gridID = i+1;
      col= (gridID -1) % gridCols + 1;
      row = (gridID - col) / gridCols + 1;
      gridArea = gridIDS[i] * xCellSize_grd * yCellSize_grd;     //grid area in m2
   
      //printf ("gridID=%d  row=%d  col=%d\n",gridID,row,col);

      sprintf( temp_char,"%d,%d,%d",gridID,row,col);
      lineStr = string( temp_char );
   
      if ( inUSGSImpe.compare("YES") == 0 )
      {   
         sprintf( temp_char,",%.5lf",100.0*gridIMPE[i]/gridArea);   //area percent
         lineStr.append( temp_char );

         k = (row - 1) * cols + col - 1 ;  //index in 1d array
         imperv[k] = 100.0*gridIMPE[i]/gridArea;

         //printf ("IMPE=%lf\n",imperv[k]);
      }

      if ( inUSGSCano.compare("YES") == 0 )
      {
         sprintf( temp_char,",%.5lf",100.0*gridCANO[i]/gridArea);
         lineStr.append( temp_char );

         k = (row - 1) * cols + col - 1 ;  //index in 1d array
         canopy[k] = 100.0*gridCANO[i]/gridArea;

         //printf ("CANOP=%lf\n",canopy[k]);
      }

      int kk = 0;  //number of landuse classes
      waterP = 0.0;    //initlize it
      maxP  = -100.0;  //initialize it

      if ( land_cat_len > 0 )
      {
         for (j=0; j< MODIS_CLASSES_NUM; j++)
         {
            if ( inNASALand.compare("YES") == 0 )
            {
               percent = 100.0 * gridMODIS[j*gridPixels+i]/gridIDS[i];

               sprintf( temp_char,",%.5lf",percent);   //class counts percent
               lineStr.append( temp_char );

               k = kk * rows * cols + (row - 1) * cols + col - 1;
               luf[k] = percent;

               if ( lu_list[kk] == 17 )
               {
                  //get total water percent
                  waterP += percent;
               }

               if (percent > maxP)
               {
                  //get dominant class
                  maxP = percent;
                  maxIndex = kk;  //lu_list index with max percent
               }


               if (percent > 0)  testd+=1.0;
            }
            else
            {
               percent = 0.0;
               sprintf( temp_char,",%.5lf",percent);   //class counts percent
               lineStr.append( temp_char );
            }
       
            kk++;
         }  //end of MODIS LU


         for (j=0; j< NLCD_CLASSES_NUM; j++)
         {
            if ( inUSGSLand.compare("YES") == 0 )
            {
               percent = 100.0 * gridNLCD[j*gridPixels+i]/gridIDS[i];

               sprintf( temp_char,",%.5lf",percent);   //class counts percent
               lineStr.append( temp_char );

               k = kk * rows * cols + (row - 1) * cols + col - 1;

               if ( lu_list[kk] == 21 )
               {
                  //get total water percent
                  waterP += percent;
                   
                  //put NLCD water to MODIS water items 17

                  int k_water = 16 * rows * cols + (row - 1) * cols + col - 1;

                  luf[k_water] += percent;
               }
               else
               {
                  luf[k] = percent;
               }


               if (percent > maxP)
               {
                  //get dominant class
                  maxP = percent;
                  maxIndex = 20+j;  //lu_list index with max percent
               }

            }
            else
            {
               percent = 0.0;
               sprintf( temp_char,",%.5lf",percent);   //class counts percent
               lineStr.append( temp_char );
            }
            kk++;
         } //end of NLCD LU

      
         k = (row - 1) * cols + col - 1 ;  //index in array
         //land = 1 and water = 0  mask
         if (waterP > 50.00)
         {
            lum[k] = 0.0;
         }
         else
         { 
            lum[k] = 1.0;
         }

         //printf ("i=%d  maxIndex = %d \n",i,maxIndex);
         lud[k] = maxIndex + 1;   //get class number with maximum percent

      }  //end of land_cat_len 
   

      lineStr.append( "\n" );

      outTxtStream.write( lineStr.c_str(), lineStr.size() ); 
   } //end of i
   
   //close text file
   outTxtStream.close ();

   //checking

   printf ( "\tcount percent > 0: %.0lf\n", testd);

   //printf ( " r1 c7 IMP=%f  CAN=%lf  LUF=%lf  LUM=%d  LUD=%d\n",imperv[6], canopy[6],luf[2456506],lum[6],lud[6]);
   //printf ( " r1 c7 IMP=%f  CAN=%lf  LUF=%lf  LUM=%d  LUD=%d\n",imperv[34999], canopy[34999],luf[179499],lum[34999],lud[34999]);
   //printf ( " r1 c7 IMP=%f  CAN=%lf  LUF=%lf  LUM=%d  LUD=%d\n",imperv[72249], canopy[72249],luf[2962249],lum[72249],lud[72249]);
  
   /**********************************************
   * free all hashtables for landuse computation *
   * delete used files   *                       *
   **********************************************/
   if ( inUSGSImpe.compare("YES") == 0 )
   {
     CPLFree (gridIMPE);
   }

   if ( inUSGSCano.compare("YES") == 0 )
   {   
      CPLFree (gridCANO);
   }

   if ( inUSGSLand.compare("YES") == 0 )
   {
        CPLFree (gridNLCD);
   }

   if ( inNASALand.compare("YES") == 0 )
   {
        CPLFree (gridMODIS);
   }


/*--------------------------------------------------------------*/
/* Prepare data to write WRF landuse data file in netCDF format */
/*--------------------------------------------------------------*/


   /*****************************************************
   * Compute center and domain corner lat and long data *
   *****************************************************/
   printf("\nCompute netcdf extent and dimension cooridnates...\n");
   computeGridCornerCenterLatLong ( &grid );
  
   /**********************************
   * Compute domain grid coordinates *
   ***********************************/
   computeGridCoordinates (&grid);

   /***********************************************
   * Write WRF landuse data file in netCDF format *
   ***********************************************/
   int ncid;
   anyErrors( nc_create(outNetcdfFile.c_str(), NC_CLOBBER, &ncid) );

   /********************
   * Dimension lengths *
   ********************/
   size_t west_east_len   = grid.cols;
   size_t south_north_len = grid.rows;
   size_t dateStr_len = timeStrLength;
   size_t nameStr_len = nameStrLength;


   /********************
   * Define dimensions *
   ********************/
   int dateStr_dim;
   int time_dim;
   int west_east_dim;
   int south_north_dim;
   int land_cat_dim;
   int nameStr_dim;

   printf( "Defining dimensions in output netcdf file...\n" );

   anyErrors( nc_def_dim(ncid, "Time", NC_UNLIMITED, &time_dim) );
   anyErrors( nc_def_dim(ncid, "DateStrLen", dateStr_len, &dateStr_dim) );
   anyErrors( nc_def_dim(ncid, "west_east", west_east_len, &west_east_dim) );
   anyErrors( nc_def_dim(ncid, "south_north", south_north_len, &south_north_dim) );

   if ( inUSGSLand.compare("YES") == 0 ||  inNASALand.compare("YES") == 0 )
   {
      anyErrors( nc_def_dim(ncid, "land_cat", land_cat_len, &land_cat_dim) );
      anyErrors( nc_def_dim(ncid, "NameStrLen", nameStr_len, &nameStr_dim) );
   }


   /**********************
   * Define variables IDs*
   ***********************/
   int times_id;
   int timesf_id;
   int lu_list_id;
   int lu_name_id;
   int imperv_id;
   int canopy_id;
   int lum_id;
   int luf_id;
   int lud_id;

   int satVars_id[inSatVars.size() * 2];  //define in a array

   /*****************************
   * Define variables for output*
   ******************************/
   int        dimIndex[NC_MAX_DIMS];        //dimension index array for write out array
   int        fieldtype[1];
   int        sr_xy[1];


   fieldtype[0] = 104;
   sr_xy[0] = 1;


   printf( "Defining variables in output netcdf file...\n" );
   dimIndex[0] = time_dim;
   dimIndex[1] = dateStr_dim;
   anyErrors( nc_def_var(ncid, "Times", NC_CHAR, 2, dimIndex, &times_id) );
   anyErrors( nc_put_att_text(ncid, times_id, "description", 21, "Time string (19 char)" ) );
   anyErrors( nc_put_att_text(ncid, times_id, "units", timeStrLength, "yyyy-mm-dd_hh:mm:ss") );

   //Time julian date float float variable
   printf( "\tTimes_Julian\n" );
   dimIndex[0] = time_dim;
   anyErrors( nc_def_var(ncid, "Times_julian", NC_FLOAT, 1, dimIndex, &timesf_id) );

   tmp_str = string ("Time Julian for ");
   tmp_str.append ( startDateTime );
   anyErrors( nc_put_att_text(ncid, timesf_id, "description", tmp_str.size(), tmp_str.c_str() ) );
   anyErrors( nc_put_att_text(ncid, timesf_id, "units", 7, "Day") );


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

   /***************************************
   * Set define mode for landuse output   *
   ****************************************/
   anyErrors( nc_redef (ncid) );

   dimIndex[0] = south_north_dim;
   dimIndex[1] = west_east_dim;
   if ( inUSGSImpe.compare("YES") == 0 )
   {
      anyErrors( nc_def_var(ncid, "IMPERV", NC_FLOAT, 2, dimIndex, &imperv_id) );
   }

   if ( inUSGSCano.compare("YES") == 0 )
   {
      anyErrors( nc_def_var(ncid, "CANOPY", NC_FLOAT, 2, dimIndex, &canopy_id) );
   }

   if (land_cat_len > 0 ) 
   {
      dimIndex[0] = land_cat_dim;
      anyErrors( nc_def_var(ncid, "LU_LIST", NC_INT, 1, dimIndex, &lu_list_id) );

      dimIndex[0] = land_cat_dim; 
      dimIndex[1] = nameStr_dim;
      anyErrors( nc_def_var(ncid, "LU_NAME", NC_CHAR, 2, dimIndex, &lu_name_id) );

   /*   dimIndex[0] = time_dim;
      dimIndex[1] = land_cat_dim;
      dimIndex[2] = south_north_dim;
      dimIndex[3] = west_east_dim;
      anyErrors( nc_def_var(ncid, "LANDUSEF", NC_FLOAT, 4, dimIndex, &luf_id) );
   */
      dimIndex[0] = land_cat_dim;
      dimIndex[1] = south_north_dim;
      dimIndex[2] = west_east_dim;
      anyErrors( nc_def_var(ncid, "LANDUSEF", NC_FLOAT, 3, dimIndex, &luf_id) );

      dimIndex[0] = south_north_dim;
      dimIndex[1] = west_east_dim;
      anyErrors( nc_def_var(ncid, "LANDMASK", NC_FLOAT, 2, dimIndex, &lum_id) );
      anyErrors( nc_def_var(ncid, "LU_INDEX", NC_FLOAT, 2, dimIndex, &lud_id) );
   }

   /********************
   * Assign attributes *
   ********************/
   printf( "\nAssigning variable attributes in output netcdf file...\n" );

   // Impervious surface coverage
   if ( inUSGSImpe.compare("YES") == 0 )
   {
      anyErrors( nc_put_att_int(ncid, imperv_id, "FieldType", NC_INT, 1, fieldtype) );
      anyErrors( nc_put_att_text(ncid, imperv_id, "MemoryOrder", 3, "XY ") );
      tmp_str = string ( "Percent of impervious surface in WRF grid" );
      anyErrors( nc_put_att_text(ncid, imperv_id, "description", tmp_str.size(), tmp_str.c_str() ) );
      anyErrors( nc_put_att_text(ncid, imperv_id, "units", 7, "Percent") );
      anyErrors( nc_put_att_text(ncid, imperv_id, "stagger", 1, "M") );
      anyErrors( nc_put_att_int(ncid, imperv_id, "sr_x", NC_INT, 1, sr_xy) );
      anyErrors( nc_put_att_int(ncid, imperv_id, "sr_y", NC_INT, 1, sr_xy) );
   }

   // Forest canopy
   if ( inUSGSCano.compare("YES") == 0 )
   {
      anyErrors( nc_put_att_int(ncid, canopy_id, "FieldType", NC_INT, 1, fieldtype) );
      anyErrors( nc_put_att_text(ncid, canopy_id, "MemoryOrder", 3, "XY ") );
      tmp_str = string ( "Percent of forest tree canopy in WRF grid" );
      anyErrors( nc_put_att_text(ncid, canopy_id, "description", tmp_str.size(), tmp_str.c_str() ) );
      anyErrors( nc_put_att_text(ncid, canopy_id, "units", 7, "Percent") );
      anyErrors( nc_put_att_text(ncid, canopy_id, "stagger", 1, "M") );
      anyErrors( nc_put_att_int(ncid, canopy_id, "sr_x", NC_INT, 1, sr_xy) );
      anyErrors( nc_put_att_int(ncid, canopy_id, "sr_y", NC_INT, 1, sr_xy) );
   }
   
   
   // MODIS-NLCD landuse
   if (land_cat_len > 0 ) 
   {
      // landuse classes list
      anyErrors( nc_put_att_int(ncid, lu_list_id, "FieldType", NC_INT, 1, fieldtype) );
      anyErrors( nc_put_att_text(ncid, lu_list_id, "MemoryOrder", 3, "0  ") );
      tmp_str = string ( "MODIS IGBP and NLCD classes" );
      anyErrors( nc_put_att_text(ncid, lu_list_id, "description", tmp_str.size(), tmp_str.c_str()) );
      anyErrors( nc_put_att_text(ncid, lu_list_id, "units", 8, "Category") );
      anyErrors( nc_put_att_text(ncid, lu_list_id, "stagger", 1, "") );
      anyErrors( nc_put_att_int(ncid, lu_list_id, "sr_x", NC_INT, 1, sr_xy) );
      anyErrors( nc_put_att_int(ncid, lu_list_id, "sr_y", NC_INT, 1, sr_xy) );

      //landuse class names
      anyErrors( nc_put_att_text(ncid, lu_name_id, "description", 21, "Land cover class name" ) );
      anyErrors( nc_put_att_text(ncid, lu_name_id, "units", 4 , "None") );
      
      // landuse fraction
      anyErrors( nc_put_att_int(ncid, luf_id, "FieldType", NC_INT, 1, fieldtype) );
      anyErrors( nc_put_att_text(ncid, luf_id, "MemoryOrder", 3, "XY ") );
      tmp_str = string ( "Percent of 20 MODIS IGBP and 20 NLCD land use classes" );
      anyErrors( nc_put_att_text(ncid, luf_id, "description", tmp_str.size(), tmp_str.c_str() ) );
      anyErrors( nc_put_att_text(ncid, luf_id, "units", 7, "Percent") );
      anyErrors( nc_put_att_text(ncid, luf_id, "stagger", 1, "M") );
      anyErrors( nc_put_att_int(ncid, luf_id, "sr_x", NC_INT, 1, sr_xy) );
      anyErrors( nc_put_att_int(ncid, luf_id, "sr_y", NC_INT, 1, sr_xy) );

      //land mask
      anyErrors( nc_put_att_int(ncid, lum_id, "FieldType", NC_INT, 1, fieldtype) );
      anyErrors( nc_put_att_text(ncid, lum_id, "MemoryOrder", 3, "XY ") );
      anyErrors( nc_put_att_text(ncid, lum_id, "units", 17, "None") );
      tmp_str = string ( "Landmask : 1=land, 0=water" );
      anyErrors( nc_put_att_text(ncid, lum_id, "description", tmp_str.size(), tmp_str.c_str() ) );
      anyErrors( nc_put_att_text(ncid, lum_id, "stagger", 1, "M") );
      anyErrors( nc_put_att_int(ncid, lum_id, "sr_x", NC_INT, 1, sr_xy) );
      anyErrors( nc_put_att_int(ncid, lum_id, "sr_y", NC_INT, 1, sr_xy) );

      // dominant land class
      anyErrors( nc_put_att_int(ncid, lud_id, "FieldYype", NC_INT, 1, fieldtype) );
      anyErrors( nc_put_att_text(ncid, lud_id, "MemoryOrder", 3, "XY ") );
      anyErrors( nc_put_att_text(ncid, lud_id, "units", 16, "Category") );
      tmp_str = string ( "Dominant category" );
      anyErrors( nc_put_att_text(ncid, lud_id, "description", tmp_str.size(), tmp_str.c_str() ) );
      anyErrors( nc_put_att_text(ncid, lud_id, "stagger", 1, "M") );
      anyErrors( nc_put_att_int(ncid, lud_id, "sr_x", NC_INT, 1, sr_xy) );
      anyErrors( nc_put_att_int(ncid, lud_id, "sr_y", NC_INT, 1, sr_xy) );
   }

  
   /************************************************
       * Define satellite variables in NetCDF output   *
       *************************************************/
       size_t      dims_len[10];     //set maximum 10 additional dimensions besides times, rows, cols
       int         dims_id[10];      //dim IDs for added dimensions besides first 3 
       int         numADims;         //any added dimensions besides first three
       int         satV_id;
       float       laiScaleFactors[inSatVars.size()];
 
       
       //take the first satellite image
       string laiFile = laiFiles[0];

       numADims = 0;  
       

       nn = 0;
       for ( i=0; i<inSatVars.size(); i++ )
       {

          nn = i * 2;
          printf( "\nDefine satellite variable in NetCDF output:  %s...\n",inSatVars[i].c_str() );

          gridInfo laiFileInfo = getHDF4VarInfo ( laiFile, inSatVars[i] );
          gridInfo laiFileInfo_T = getHDF4VarInfo ( laiFile, inSatVars[i] );
          
          //change variable description for cell average
          string  tmp_attr = laiFileInfo_T.attsStr[6];
          tmp_attr.append ( "-cell average" );
          laiFileInfo_T.attsStr[6] = tmp_attr;

      
          laiScaleFactors[i] = atof ( laiFileInfo.attsStr[2].c_str() );

          printf( "\t\t\tScaleFactor is: %f\n",laiScaleFactors[i] );
          
          dimIndex[0] = time_dim;
          dimIndex[1] = land_cat_dim;
          dimIndex[2] = south_north_dim;
          dimIndex[3] = west_east_dim;
          int  varDims = 4;
          

          satV_id = defineWRFNetCDFSatVar ( ncid, inSatVars[i], varDims, dimIndex, laiFileInfo );
          satVars_id[nn] = satV_id;

          tmp_str = inSatVars[i];
          tmp_str.append ( "_T" );

          dimIndex[0] = time_dim;
          dimIndex[1] = south_north_dim;
          dimIndex[2] = west_east_dim;
          varDims = 3;

          satV_id = defineWRFNetCDFSatVar ( ncid, tmp_str, varDims, dimIndex, laiFileInfo_T );
          satVars_id[nn+1] = satV_id;

       } //i


   /********************
   * Leave define mode *
   ********************/
   anyErrors( nc_enddef (ncid) );
        
   /*********************************
   * Write NLCD variables to netCDF file *
   *********************************/
   printf( "\nWriting variable data in output netcdf file...\n" );

   //      Store imperv
   if ( inUSGSImpe.compare("YES") == 0 )
   {
      anyErrors( nc_put_var_float(ncid, imperv_id, imperv) );
      printf( "\tWrited imperv\n" );
      free(imperv);
   }

   //      Store canopy
   if ( inUSGSCano.compare("YES") == 0 )
   {
      anyErrors( nc_put_var_float(ncid, canopy_id, canopy) );
      printf( "\tWrited canopy\n" );
      free(canopy);
   }

   //landuse
   if (land_cat_len > 0 )
   {
      //      Store lu_list
      anyErrors( nc_put_var_int(ncid, lu_list_id, lu_list) );
      printf( "\tWrited lu_list\n" );

      //store lu_name
      long ty_start[2];
      long ty_count[2];

      ty_start[0]=0;
      ty_start[1]=0;

      ty_count[0]=land_cat_len;
      ty_count[1]=nameStr_len;

      anyErrors( ncvarput(ncid, lu_name_id, ty_start, ty_count, lu_name) );
      printf( "\tWrited lu_name\n\n" );


      //   Store luf
      anyErrors( nc_put_var_float(ncid, luf_id, luf) );
      printf( "\tWrited luf\n" );

      //   Store lum
      anyErrors( nc_put_var_float(ncid, lum_id, lum) );
      printf( "\tWrited lum\n" );

      //      Store lud
      anyErrors( nc_put_var_float(ncid, lud_id, lud) );
      printf( "\tWrited lud\n\n" );

      free(lu_list);
      free(lu_name);
      free(luf);
      free(lum);
      free(lud);
   }


   /***********************************************
   * Loop through each day's satellite file       *
   * Merge them into one file for the domain area *
   * Read each variable from merged sat data file *
   * Write the variable in output                 *
   ************************************************/
    numDays = 0;
    string preDay = laiFiles[1];   //initialize the day
    vector<string> dayLAIFiles;


    for ( m=0; m<laiFiles.size(); m++)
    {
        string curDay = laiFiles[m+1];  //following item  is current date

       //first file: name and date
       if ( m == 0 )
       {
          printf ( "\t%d First file: m=%d  File=%s  date=%s\n", numDays+1, m, laiFiles[m].c_str(), laiFiles[m+1].c_str() );  
       }


       if ( preDay.compare ( curDay ) == 0 )
       {
          //same day files
          dayLAIFiles.push_back ( laiFiles[m] ); 


          //handle last file with the same date
          if ( m == (laiFiles.size() - 2) )
          {
             curDay = string ( "END" );    //get into the following process: != preDay
          }
       }
      
          

       while ( preDay.compare ( curDay ) != 0 )  
       {


           /**********************************
           * Write the time variables        *
           **********************************/

           //get time string
           tmp_str = convertDate2OutputDateStr( preDay );

           string tmpJulian = getJulianDayStr ( preDay );
           float tmpJulian_float = atof( tmpJulian.c_str() ); 

           writeWRF1StepTimeVariables (numDays, tmpJulian_float, tmp_str, ncid, timesf_id, times_id, dateStr_len);
          


          //Last file: name and date
          printf ( "\t%d Last: m=%d  File=%s  date=%s  numFiles=%d\n", numDays+1, m, dayLAIFiles[dayLAIFiles.size()-1].c_str(), preDay.c_str(), dayLAIFiles.size() );


          //extract LAI and FPAR for those files 
          printf ( "\tProcessing NASA LAI and FPAR: %d files for date %s\n", dayLAIFiles.size(), preDay.c_str() );


          /* ------------------------------------------------- */
          /*     Extracted LAI images out to be in one file    */
          /* ------------------------------------------------- */
          gridInfo    extLAIInfo, gridLAIInfo;  //store info for extracted LAI tiles for a domain
          GByte       *laiV;                    //extract LAI arrays for all variables
          double      *gridLAI=NULL;            //array to store LAI data for modeling grids


          //obtain extent of LAI tile files.  Loop through each file
 
          printf ("\t Variable = %s\n", inSatVars[0].c_str() );
          extLAIInfo = getAllMODISTileInfo ( dayLAIFiles, inSatVars[0] );

          printGridInfo ( extLAIInfo );

          //create new LAI TILE extent based on projected domain area

          gridLAIInfo = computeNewRasterInfo_fromImage ( extLAIInfo.xCellSize, grid, extLAIInfo);


          //check LAI data area
          if ( extLAIInfo.xmin >= gridLAIInfo.xmax || extLAIInfo.xmax <= gridLAIInfo.xmin ||
               extLAIInfo.ymin >= gridLAIInfo.ymax || extLAIInfo.ymax <= gridLAIInfo.ymin )
          {
             printf ("Error: LAI files do not intersect with the modeling domain.\n");
             exit ( 1 );
          }


          //extract LAI into one file

          string dayLAIFile = extractDomainLAIData ( dayLAIFiles, inSatVars, gridLAIInfo );

          printGridInfo ( gridLAIInfo );

           
          /* ----------------------------------- */
          /*     compute NASA LAI and FPAR       */
          /* ----------------------------------- */
          //each LAI/FAR variable has additional *_T output: for grid total 

          
          //process MODIS land cover to match LAI
          gridMODIS = (int *) CPLCalloc(sizeof(int), inSatVars.size() * MODIS_CLASSES_NUM * gridPixels );

          //process six variables
          gridLAI = (double *) CPLCalloc(sizeof(double), inSatVars.size() * MODIS_CLASSES_NUM * gridPixels );


          // initialize to MODIS no data value
          for (i=0; i<gridPixels*inSatVars.size()*MODIS_CLASSES_NUM; i++)
          {
              gridLAI[i] = -999.0;
          }

          computeLAI( infoGrd, modisFile, modisInfo, dayLAIFile, gridLAIInfo, inSatVars, gridLAI, laiScaleFactors );
 
      
          //prepare array to output for each variable
          nn = 0;

          for ( n=0; n<inSatVars.size(); n++ )
          {
             nn = n * 2;

             float *dataV = (float *) CPLCalloc(sizeof(float), 40 * gridPixels );
             float *dataV_T = (float *) CPLCalloc(sizeof(float), gridPixels );
             

             //
             //handle zero or missing issues --- need to be checked!!!
             // 

             for (i=0; i<gridPixels; i++)
             {
                gridID = i+1;
                col= (gridID -1) % gridCols;       //from 0
                row = (gridID - col) / gridCols;   //from 0

                int grdNum = 0;         // gridIDS[i];     
                double totLAI = 0; 

                for (j=0; j< MODIS_CLASSES_NUM; j++)
                {

                   k = n * MODIS_CLASSES_NUM * gridPixels + j * gridPixels + i;

                   int lcNum = gridMODIS[k]; 
                   

                   if ( gridLAI[k] != -999.0 )
                   {
                      grdNum += lcNum; 
                      totLAI +=  gridLAI[k];
                   }

                   if ( lcNum > 0 && gridLAI[k] != -999.0 )
                   {
                      dataV[j*gridPixels+i] = gridLAI[k] / lcNum; 
                      
                      //printf ("\t class=%d  totalLAI=%f    lcnum=%d\n", j+1, gridLAI[k], lcNum);

                   }
                   else
                   {
                      dataV[j*gridPixels+i] = -999.0;
                   }

                }  // j land cover 

                if ( grdNum > 0 )
                {
                   dataV_T[i] = totLAI / grdNum;
                   //printf ("\t averageLAI=%f    totalLAI=%f    grdNum=%d\n", dataV_T[i], totLAI, grdNum);
                }
                else
                {
                    dataV_T[i] = -999.0;
                }
                 

              
                if ( grdNum != gridIDS[i] )
                {
                   //printf ( "\tLAI grids = %d   Domain Grids = %d\n", grdNum, gridIDS[i]);
                }

             }  //i pixel

            
             //output to netCDF
             
             //define array to write for one time step for LC LAI
             size_t  start [4];
             size_t  count [4];

             start[0] = numDays;
             start[1] = 0;
             start[2] = 0;
             start[3] = 0;

             count[0] = 1;
             count[1] = 40;
             count[2] = south_north_len;
             count[3] = west_east_len;

             anyErrors ( nc_put_vara_float ( ncid, satVars_id[nn], start, count, dataV) );
             //printf ( "\tPut dataV\n");

             //define array to write for one time step for total LAI
             size_t  start_T[3];
             size_t  count_T[3];
          
             start_T[0] = numDays;
             start_T[1] = 0;
             start_T[2] = 0;
             
             count_T[0] = 1;
             count_T[1] = south_north_len;
             count_T[2] = west_east_len;
             
             anyErrors ( nc_put_vara_float ( ncid, satVars_id[nn+1], start_T, count_T, dataV_T) );

             //printf( "\tWrote %s for %s\n\n", inSatVars[n].c_str(), preDay.c_str() );

            
             CPLFree ( dataV );
             CPLFree ( dataV_T );


          }  //n sat variable
         

          CPLFree (gridMODIS);
          CPLFree ( gridLAI );

          deleteRasterFile ( dayLAIFile );


          numDays ++;
            
          if ( curDay.compare ("END") != 0 )
          {

             //first file: name and date
             printf ( "\t%d First: m=%d  File=%s  date=%s\n", numDays+1, m, laiFiles[m].c_str(), laiFiles[m+1].c_str() );
          }
          
          
          //clean day file vector
          dayLAIFiles.clear();


          //get the current m file
          dayLAIFiles.push_back ( laiFiles[m] );

          preDay = laiFiles[m+1];


          //handle last file is in different day
          if ( curDay.compare ("END") == 0 )
          {
            preDay = curDay;  //end of the loop  
          }
          else if ( m == (laiFiles.size() - 2) )
          {
           curDay = string ( "END" );    //loop through one more time for last file with the different date
          }

       }  //process one day files
  
       m++;
    }


    printf ("\tNumber of days processed: %d\n", numDays);

   /****************************
   * Free and delete data sets *
   *****************************/

   CPLFree (gridIDS);

   //delete 30m domain grid image after processing
   deleteRasterFile ( gridRasterFile );

   //delete clipped and projected MODIS file
   if ( inNASALand.compare("YES") == 0 )
   {
      deleteRasterFile ( modisFile );
   }



   /**************************
   * Store global attributes *
   **************************/
   string dateDate = convertDate2OutputDateStr ( startDateTime );

   string title = string ( "Landuse Data Generated from computeGridLandUse.exe" );
   writeWRFGlobalAtributes ( ncid, title, grid, dateDate );

   /********************

   * Close netCDF file *
   ********************/
   anyErrors( nc_close(ncid) );

   printf( "Finished writing output NetCDF file: %s\n\n", outNetcdfFile.c_str() );

   printf ("\n\nCompleted in computing and outputing all landuse data.\n");


}

/*****************************  END of MAIN  ****************************/



/************************************************************************/
/*    countGridIDs()                                                    */
/************************************************************************/

void   countGridIDs(  )  
{

    long                  i,nPixelCount=0;
    long long             nPixelCount_grd = 0;
    int                   cCells, rCells;       //cells
    int                   gridID; 


     printf( "\nCount grid ID numbers in each domain grid cell...\n" );

           
/* -------------------------------------------------------------------- */
/*   Read one row at a time from top to down                            */
/* -------------------------------------------------------------------- */
     GUInt32 *poImage_grd = (GUInt32 *) CPLCalloc(sizeof(GUInt32),xCells_grd);
     int iLine;
     for( iLine = 0; iLine<yCells_grd; iLine++ )
     {
        if ( (poGridBand->RasterIO(GF_Read, 0, iLine,xCells_grd, 1,
                                   poImage_grd,xCells_grd,1,GDT_UInt32,0,0)) == CE_Failure)
        {
            printf( "\tError: Reading row = %d from domain grid image.\n",iLine+1);
            CPLFree (poImage_grd);
            exit( 1 );
        }

        for (i=0; i<xCells_grd; i++)
        {
           gridID = poImage_grd[i] ;
           if (gridID > 0 && gridID <= gridPixels)
           {
               gridIDS[gridID - 1] += 1;
           }
        }
     } 
     printf ( "Finished counting modeling domain grid ID numbers.\n\n" );

}  //end of countGridIDs


/************************************************************************/
/*    computeImpe_Cano(std::vector<string> imageFiles, string fileType) */
/************************************************************************/

void   computeImpe_Cano( std::vector<string> imageFiles, string fileType )  
{

    int                   i,j,k;
    int                   pixelIndex;
    string                fileName;            
    GDALDataset           *poRDataset;
    GDALRasterBand        *poBand;
    double                adfGeoTransform[6];
    double                xUL,yUL;
    int                   xCells, yCells;              //cells
    double                xCellSize, yCellSize;        //cell size
    double                xmin,xmax,ymin,ymax;         //current image extent
    double                intpart; 

    const char            *pszWKT = NULL;
    char                  *pszWKT_nc = NULL;
    OGRSpatialReference   oSRS;
    char                  *pszProj4 = NULL, *tmpProj4=NULL;
   
    GByte                 NoDataValue = USGS_NLCD_NODATAVALUE;     //nodata value for i and j images
    GByte                 percent;
    int                   gridID; 


   printf( "\nCompute percentage of imperviousness or canopy in domain grid cells...\n" );

/* -------------------------------------------------------------------- */
/*      Process one image at a time for the image vector                */
/* -------------------------------------------------------------------- */
   //loop through all images in the vector
   for ( i = 0; i<imageFiles.size(); i++)
   {
      fileName = imageFiles.at(i);
      printf( "  %s\n",fileName.c_str() );
     
/* -------------------------------------------------------------------- */
/*      Open image i                                                    */
/* -------------------------------------------------------------------- */ 
      //open the current i image 
      poRDataset = (GDALDataset *) GDALOpen( fileName.c_str(), GA_ReadOnly );
      if( poRDataset == NULL )
      {
         printf( "  Error: Open raster file failed: %s.\n", fileName.c_str() );
         exit( 1 );
      }
/* -------------------------------------------------------------------- */
/*   get rows, columns, cell size, and image extent of image i in vector*/
/* -------------------------------------------------------------------- */
      // get rows and columns of the image
      xCells = poRDataset->GetRasterXSize();
      yCells = poRDataset->GetRasterYSize();
      printf( "  Image size is %dx%dx%d\n", xCells, yCells, poRDataset->GetRasterCount() );

      //get UL corner coordinates and grid size
      if( poRDataset->GetGeoTransform( adfGeoTransform ) == CE_None )
      {
         xUL = adfGeoTransform[0];
         yUL = adfGeoTransform[3];
         printf( "  Image UL Origin = (%.6lf,%.6lf)\n", xUL,yUL );

         xCellSize =  adfGeoTransform[1] ;
         yCellSize =  -1 * adfGeoTransform[5] ;  // Make it positive.  Negative due to Origin is UL corner point
         printf( "  Image Pixel Size = (%.3lf,%.3lf)\n", xCellSize, yCellSize );

         //check if UL corner coordinates are the standard NLCD grid corner
         if ( xCellSize_grd == 30.0 && ( fabs( modf(xUL/xCellSize,&intpart) )!= 0.5 || fabs (modf(yUL/yCellSize,&intpart) ) != 0.5 ) )
         {
            printf ( "\tError: UL corner not have NLCD corner coordinates: modf(fabs(xUL/30))=0.5 for: %s\n",fileName.c_str() );
            printf ( "         Regrid using gdal_translate utility.\n" );
            exit ( 1 );
         }
      }

      //check consistent cell size
      if ( xCellSize_grd != xCellSize || yCellSize_grd != yCellSize )
      {
           printf( "  Error: This image's cell size is different from the domain grid image.\n" );
           printf( "         Resmaple it using gdal_translate utility.\n" );
           exit ( 1 );
      }
 
      //compute extent of band 1 image (only one band for all images)
      xmin = xUL;
      xmax = xUL + xCellSize * xCells;

      ymax = yUL;
      ymin = yUL - yCellSize * yCells;
      printf( "  Image extent:  minXY(%.3f,%.3f)   maxXY(%.3f,%.3f)\n", xmin,ymin,xmax,ymax ); 

/* -------------------------------------------------------------------- */
/*   Get image projection to make sure that all images have the same    */
/* -------------------------------------------------------------------- */
     //get projection from the image
     if( (pszWKT = poRDataset->GetProjectionRef())  == NULL )
     {
        printf( "  Error: Projection is not defined in the NLCD file: %s.\n", fileName.c_str() );
        printf( "         Define it using gdal_translate utility.\n");
        exit( 1 );
     }

     //convert it to no const char
     pszWKT_nc =strdup ( pszWKT ); 
     oSRS.importFromWkt( &pszWKT_nc );
     oSRS.exportToProj4( &pszProj4 );    
     oSRS_grd.exportToProj4 (&tmpProj4);
      
     //make sure that the image has the same projection as the first image's
     if ( strcmp (pszProj4, tmpProj4) != 0 )
     //if (! oSRS.IsSame (&oSRS_grd) )  -- did not work in GDAL 1.9.1
     {
         printf( "\tError: This image's projection is different from the domain grid image's.\n" );
         printf( "\tProjection for this image = %s\n", pszProj4);
         printf( "\tProject it using gdalwarp utility to: %s\n", pszProj4_grd );
         exit ( 1 );
     }
 


/* -------------------------------------------------------------------- */
/*   Image data type has to be GByte                                    */
/* -------------------------------------------------------------------- */
     //get the band 1 image from the current image and make sure that it is GByte image
     poBand = poRDataset->GetRasterBand( 1 );  // band 1
     GDALDataType poBandType = poBand->GetRasterDataType();
     printf ("  Image Type = %d\n",poBandType);
     if ( poBandType !=GDT_Byte )
     {
         printf( "Error: Image data type is not GDT_Byte: %s.\n", fileName.c_str() );    
         exit ( 1 );
     }  

/* -------------------------------------------------------------------- */
/*   Compute overlapping box indexes in both image i and domain image   */
/* -------------------------------------------------------------------- */
     //check whether those two images are overlaying and calculate row and col in overlay areas from both images
     int row1 = 0, row2 = 0, col1=0, col2=0;
     int nXSize = 0 ,nYSize = 0;     //overlaypping array size for image i 

     int row1_grd = 0, row2_grd = 0, col1_grd=0, col2_grd=0;
     int nXSize_grd = 0 ,nYSize_grd = 0;     //overlaypping array size for domain gird image

     //if the two images intersecting
     if (! (xmin >= xMax_grd || xmax <= xMin_grd ||  ymin >= yMax_grd || ymax <= yMin_grd) )
     { 
         //Row and column start from LU corner
         //calculate intersection UL columns
         computeIntersectionBox(xCells,yCells,xmin,ymin,xmax,ymax,xMin_grd,yMin_grd,xMax_grd,yMax_grd,&col1,&row1,&col2,&row2,&col1_grd,&row1_grd,&col2_grd,&row2_grd);
         
         //compute x and y cells, min and max for QA checking in current image i
         printf ("\ti=%d image intesection: (col=%d row=%d)  (col=%d row=%d)\n",i,col1,row1,col2,row2);
         nXSize = col2 - col1 + 1;
         nYSize = row2 - row1 + 1; 
         printf ("\t\tnXSizei = %d  nYSize = %d\n",nXSize, nYSize);

         double oXmin = xmin + (col1 - 1) * xCellSize_grd;
         double oXmax = xmin + col2 * xCellSize_grd;
         double oYmax = ymax - (row1 - 1) * yCellSize_grd;
         double oYmin = ymax - row2 * yCellSize_grd;
         printf("\t\tOverlapping box: x: %lf - %lf    y: %lf - %lf\n",oXmin,oXmax,oYmin,oYmax); 

         //compute x and y cells and min and max for domain grid image
         printf ("\tdomain grid image intesection: (col=%d row=%d)  (col=%d row=%d)\n",col1_grd,row1_grd,col2_grd,row2_grd);
         nXSize_grd = col2_grd - col1_grd + 1;
         nYSize_grd = row2_grd - row1_grd + 1;
         printf ("\t\tnXSize = %d  nYSize = %d\n",nXSize_grd, nYSize_grd);
         //split grid domain intersection box into 4 blocks for reading because of 4 bytes data type
         int colm_grd = col1_grd + nXSize_grd / 2;
         int rowm_grd = row1_grd + nYSize_grd / 2;
         printf( "\t\tMiddle point in grid domain intersection box: col=%d   row=%d\n",colm_grd,rowm_grd);

         double oXmin_grd = xMin_grd + (col1_grd - 1) * xCellSize_grd;
         double oXmax_grd = xMin_grd + col2_grd * xCellSize_grd;
         double oYmax_grd = yMax_grd - (row1_grd - 1) * yCellSize_grd;
         double oYmin_grd = yMax_grd - row2_grd * yCellSize_grd;
         printf("\t\tOverlapping box: x: %lf - %lf    y: %lf - %lf\n",oXmin_grd,oXmax_grd,oYmin_grd,oYmax_grd);

         //make sure that overlapping box are the same in both image i and j
         if (nXSize != nXSize_grd || nYSize != nYSize_grd)
         {
            printf( "  Error: overlapping images x and y sizes are different for images i and domain grid image.\n" );
            exit ( 1 );
         }
         if (oXmin != oXmin_grd || oXmax != oXmax_grd || oYmax != oYmax_grd || oYmin != oYmin_grd)
         {
            printf( "  Error: overlapping box x and y are different for images i and domain grid image.\n" );
            exit ( 1 );
         }
 
/* ----------------------------------------------------------------- */
/*       read one row at time in the intersecting box from image     */
/* ----------------------------------------------------------------- */
         poBand = poRDataset->GetRasterBand( 1 );  // band 1

         //image row: too big to read once 
         GByte *poImage = (GByte *) CPLCalloc(sizeof(GByte),nXSize);

         //domain grid image row: too big to read once 
         GUInt32 *poImage_grd = (GUInt32 *) CPLCalloc(sizeof(GUInt32),nXSize_grd);


         for (j=0; j<nYSize; j++)
         {
            //get a row from grid
            if ( (poGridBand->RasterIO(GF_Read, col1_grd-1, row1_grd-1+j, nXSize_grd, 1,
                                       poImage_grd, nXSize_grd, 1, GDT_UInt32, 0, 0)) == CE_Failure)
            {
               printf( "\tError: reading line: row=%d  col=%d from domain grid image.\n",row1_grd+j,col1_grd);
               CPLFree (poImage_grd);
               exit( 1 );
            }
          
            //get a row from image
            if ( (poBand->RasterIO(GF_Read, col1-1, row1-1+j, nXSize, 1,
                                   poImage, nXSize, 1, GDT_Byte, 0, 0)) == CE_Failure)
            {
               printf( "\tError: reading line: row=%d  col=%d from image: %s.\n",  row1+j, col1, fileName.c_str() );
               CPLFree (poImage);
               exit( 1 );
            }
             
            if ( fileType.find(impeType) !=string::npos )
            {
               for (k=0; k<nXSize; k++)
               {
                  gridID = poImage_grd[k];
                  percent = poImage[k];
 
                  if ( gridID > 0 && gridID <= gridPixels && (percent != NoDataValue &&  
                                                              percent != USGS_NLCD_IMPE_NODATAVALUE ) )
                  {
                     gridIMPE[gridID-1] += percent * xCellSize_grd * yCellSize_grd / 100.00;  //compute imperviousness area 
                  }
               }  // end of k col 
            } //imperviousness 

 
            if ( fileType.find(canoType) !=string::npos )
            {    
               for (k=0; k<nXSize; k++)
               {
                  gridID = poImage_grd[k];
                  percent = poImage[k];

                  if ( gridID > 0 && gridID <= gridPixels && percent != NoDataValue)
                  {
                     gridCANO[gridID-1] += percent * xCellSize_grd * yCellSize_grd / 100.00;  //compute canopy area
                  }
               }  // end of k col
            }  //canopy
        }  //j


        CPLFree (poImage);
        CPLFree (poImage_grd);

     }  //end of overlapping processing
     else
     {
         printf ("\ti=%d image does not intersect with domain grid image.\n",i);
     }

     GDALClose( (GDALDatasetH) poRDataset );

   }  //end of i

   printf ("Finished preprocessing images: %s\n",fileType.c_str());
}  //end of the function


/************************************************************************/
/*    computeLandUse(std::vector<string> imageFiles, string fileType) */
/************************************************************************/

void   computeLandUse( std::vector<string> imageFiles, string fileType )  
{

    int                   i,j,k,n,m;
    int                   pixelIndex;
    string                fileName;             //file to be processed
    GDALDataset           *poRDataset;
    GDALRasterBand        *poBand;
    double                adfGeoTransform[6];
    double                xUL,yUL;
    int                   xCells, yCells;       //cells
    double                xCellSize, yCellSize; //cell size
    double                xmin,xmax,ymin,ymax;         //current image extent
    double                intpart; 

    const char            *pszWKT = NULL;
    char                  *pszWKT_nc = NULL;
    OGRSpatialReference   oSRS;
    char                  *pszProj4 = NULL, *tmpProj4=NULL;
   
    int                   gridID, classID, idIndex; 
    int                   outIndex;

    GByte                 NoDataValue = USGS_NLCD_NODATAVALUE;

   printf( "\nCompute percentage of NLCD landuse classes in domain grid cells...\n" );


/* -------------------------------------------------------------------- */
/*      Process one image at a time for the image vector                */
/* -------------------------------------------------------------------- */
   //loop through all images in the vector
   for ( i = 0; i<imageFiles.size(); i++)
   {
      fileName = imageFiles.at(i);
      printf( "  %s\n",fileName.c_str() );
     
/* -------------------------------------------------------------------- */
/*      Open image i                                                    */
/* -------------------------------------------------------------------- */ 
      //open the current i image 
      poRDataset = (GDALDataset *) GDALOpen( fileName.c_str(), GA_ReadOnly );
      if( poRDataset == NULL )
      {
         printf( "  Error: Open raster file failed: %s.\n", fileName.c_str() );
         exit( 1 );
      }
/* -------------------------------------------------------------------- */
/*   get rows, columns, cell size, and image extent of image i in vector*/
/* -------------------------------------------------------------------- */
      // get rows and columns of the image
      xCells = poRDataset->GetRasterXSize();
      yCells = poRDataset->GetRasterYSize();
      printf( "  Image size is %dx%dx%d\n", xCells, yCells, poRDataset->GetRasterCount() );

      //get UL corner coordinates and grid size
      if( poRDataset->GetGeoTransform( adfGeoTransform ) == CE_None )
      {
         xUL = adfGeoTransform[0];
         yUL = adfGeoTransform[3];
         printf( "  Image UL Origin = (%.6lf,%.6lf)\n", xUL,yUL );

         xCellSize =  adfGeoTransform[1] ;
         yCellSize =  -1 * adfGeoTransform[5] ;  // Make it positive.  Negative due to Origin is UL corner point
         printf( "  Image Pixel Size = (%.3lf,%.3lf)\n", xCellSize, yCellSize );

         //check if UL corner coordinates are the standard NLCD grid corner
         if ( xCellSize_grd == 30.0 &&  ( fabs( modf(xUL/xCellSize,&intpart) )!= 0.5 || fabs (modf(yUL/yCellSize,&intpart) ) != 0.5 ) )
         {
            printf ( "\tError: UL corner not have NLCD corner coordinates: modf(fabs(xUL/30))=0.5 for: %s\n",fileName.c_str() );
            printf ( "         Regrid using gdal_translate utility.\n" );
            exit ( 1 );
         }
      }

      //check consistent cell size
      if ( xCellSize_grd != xCellSize || yCellSize_grd != yCellSize )
      {
           printf( "  Error: This image's cell size is different from the domain grid image.\n" );
           printf( "         Resmaple it using gdal_translate utility.\n" );
           exit ( 1 );
      }
 
      //compute extent of band 1 image (only one band for all images)
      xmin = xUL;
      xmax = xUL + xCellSize * xCells;

      ymax = yUL;
      ymin = yUL - yCellSize * yCells;
      printf( "  Image extent:  minXY(%.3f,%.3f)   maxXY(%.3f,%.3f)\n", xmin,ymin,xmax,ymax ); 

/* -------------------------------------------------------------------- */
/*   Get image projection to make sure that all images have the same    */
/* -------------------------------------------------------------------- */
     //get projection from the image
     if( (pszWKT = poRDataset->GetProjectionRef())  == NULL )
     {
        printf( "  Error: Projection is not defined in the NLCD file: %s.\n", fileName.c_str() );
        printf( "         Define it using gdal_translate utility.\n");
        exit( 1 );
     }

     //convert it to no const char
     pszWKT_nc =strdup ( pszWKT ); 
     oSRS.importFromWkt( &pszWKT_nc );
     oSRS.exportToProj4( &pszProj4 );    
     oSRS_grd.exportToProj4 (&tmpProj4);

     //make sure that the image has the same projection as the first image's
     if ( strcmp (pszProj4, tmpProj4) != 0 )
     //if (! oSRS.IsSame (&oSRS_grd) )  -- did not work in GDAL 1.9.1
     {
         printf( "\tError: This image's projection is different from the domain grid image's.\n" );
         printf( "\tProjection for this image = %s\n", pszProj4);
         printf( "\tProject it using gdalwarp utility to: %s\n", pszProj4_grd );
         exit ( 1 );
     }


/* -------------------------------------------------------------------- */
/*   Image data type has to be GByte                                    */
/* -------------------------------------------------------------------- */
     //get the band 1 image from the current image and make sure that it is GByte image
     poBand = poRDataset->GetRasterBand( 1 );  // band 1
     GDALDataType poBandType = poBand->GetRasterDataType();
     printf ("  Image Type = %d\n",poBandType);
     if ( poBandType !=GDT_Byte )
     {
         printf( "Error: Image data type is not GDT_Byte: %s.\n", fileName.c_str() );    
         exit ( 1 );
     }  

/* -------------------------------------------------------------------- */
/*   Compute overlapping box indexes in both image i and domain image   */
/* -------------------------------------------------------------------- */
     //check whether those two images are overlaying and calculate row and col in overlay areas from both images
     int row1 = 0, row2 = 0, col1=0, col2=0;
     int nXSize = 0 ,nYSize = 0;     //overlaypping array size for image i 

     int row1_grd = 0, row2_grd = 0, col1_grd=0, col2_grd=0;
     int nXSize_grd = 0 ,nYSize_grd = 0;     //overlaypping array size for domain gird image

     //if the two images intersecting
     if (! (xmin >= xMax_grd || xmax <= xMin_grd ||  ymin >= yMax_grd || ymax <= yMin_grd) )
     { 
         //Row and column start from LU corner
         //calculate intersection UL columns
         computeIntersectionBox(xCells,yCells,xmin,ymin,xmax,ymax,xMin_grd,yMin_grd,xMax_grd,yMax_grd,&col1,&row1,&col2,&row2,&col1_grd,&row1_grd,&col2_grd,&row2_grd);
         
         //compute x and y cells, min and max for QA checking in current image i
         printf ("\ti=%d image intesection: (col=%d row=%d)  (col=%d row=%d)\n",i,col1,row1,col2,row2);
         nXSize = col2 - col1 + 1;
         nYSize = row2 - row1 + 1; 
         printf ("\t\tnXSizei = %d  nYSize = %d\n",nXSize, nYSize);

         double oXmin = xmin + (col1 - 1) * xCellSize_grd;
         double oXmax = xmin + col2 * xCellSize_grd;
         double oYmax = ymax - (row1 - 1) * yCellSize_grd;
         double oYmin = ymax - row2 * yCellSize_grd;
         printf("\t\tOverlapping box: x: %lf - %lf    y: %lf - %lf\n",oXmin,oXmax,oYmin,oYmax); 

         //compute x and y cells and min and max for domain grid image
         printf ("\tdomain grid image intesection: (col=%d row=%d)  (col=%d row=%d)\n",col1_grd,row1_grd,col2_grd,row2_grd);
         nXSize_grd = col2_grd - col1_grd + 1;
         nYSize_grd = row2_grd - row1_grd + 1;
         printf ("\t\tnXSize = %d  nYSize = %d\n",nXSize_grd, nYSize_grd);
         //split grid domain intersection box into 4 blocks for reading because of 4 bytes data type
         int colm_grd = col1_grd + nXSize_grd / 2;
         int rowm_grd = row1_grd + nYSize_grd / 2;
         printf( "\t\tMiddle point in grid domain intersection box: col=%d   row=%d\n",colm_grd,rowm_grd);

         double oXmin_grd = xMin_grd + (col1_grd - 1) * xCellSize_grd;
         double oXmax_grd = xMin_grd + col2_grd * xCellSize_grd;
         double oYmax_grd = yMax_grd - (row1_grd - 1) * yCellSize_grd;
         double oYmin_grd = yMax_grd - row2_grd * yCellSize_grd;
         printf("\t\tOverlapping box: x: %lf - %lf    y: %lf - %lf\n",oXmin_grd,oXmax_grd,oYmin_grd,oYmax_grd);

         //make sure that overlapping box are the same in both image i and j
         if (nXSize != nXSize_grd || nYSize != nYSize_grd)
         {
            printf( "  Error: overlapping images x and y sizes are different for images i and domain grid image.\n" );
            exit ( 1 );
         }
         if (oXmin != oXmin_grd || oXmax != oXmax_grd || oYmax != oYmax_grd || oYmin != oYmin_grd)
         {
            printf( "  Error: overlapping box x and y are different for images i and domain grid image.\n" );
            exit ( 1 );
         }
 
          

/* ----------------------------------------------------------------- */
/*       read one row at time in the intersecting box from image     */
/* ----------------------------------------------------------------- */
         poBand = poRDataset->GetRasterBand( 1 );  // band 1

         //image row: too big to read once
         GByte *poImage = (GByte *) CPLCalloc(sizeof(GByte),nXSize);

         //domain grid image is too big to read once 
         GUInt32 *poImage_grd = (GUInt32 *) CPLCalloc(sizeof(GUInt32),nXSize_grd);
             
         //count USGS NLCD landuse classes
              
         for (j=0; j<nYSize; j++)
         {
            if ( (poGridBand->RasterIO(GF_Read, col1_grd-1, row1_grd-1+j, nXSize_grd, 1,
                                       poImage_grd, nXSize_grd, 1, GDT_UInt32, 0, 0)) == CE_Failure)
            { 
               printf( "\tError: reading line: row=%d  col=%d from domain grid image.\n",row1_grd+j,col1_grd);
               CPLFree (poImage_grd);
               exit( 1 );
            }    
         
            //get a row from image
            if ( (poBand->RasterIO(GF_Read, col1-1, row1-1+j, nXSize, 1,
                                   poImage, nXSize, 1, GDT_Byte, 0, 0)) == CE_Failure)
            {
               printf( "\tError: reading line: row=%d  col=%d from image: %s.\n",  row1+j, col1, fileName.c_str() );
               CPLFree (poImage);
               exit( 1 );
            }
 
            int  writeIndicator = 0;  
            for (k=0; k<nXSize; k++)
            {
              gridID = poImage_grd[k];
              classID = poImage[k];

              if ( gridID > 0 && gridID <= gridPixels && classID != NoDataValue)
              {
                 idIndex = nlcdIDS[classID];    //index in USGS NLCD ID pointer
                 outIndex = idIndex * gridPixels + gridID - 1;  //index in NLCD landuse pointer              
                 gridNLCD[outIndex] += 1;   // count USGS NLCD landuse class in 2d array

                 //set gridID to 0 in poImage_grd after reading USGS NLCD data.  So, cells will not be counted again
                 poImage_grd[k] = 0;
                 writeIndicator = 1;
              }
           }  // end of k col

           //write changed doamin grid image line back to the image
           if ( writeIndicator == 1)
           {
              if ( (poGridBand->RasterIO(GF_Write, col1_grd-1, row1_grd-1+j, nXSize_grd, 1,
                                      poImage_grd, nXSize_grd, 1, GDT_UInt32, 0, 0)) == CE_Failure)
              {
                  printf( "\tError: writing line: row=%d  col=%d to domain grid image.\n",row1_grd+j,col1_grd);
                  CPLFree (poImage_grd);
                  exit( 1 );
              }
           }

        }  //end of j row

        
        CPLFree (poImage);
        CPLFree (poImage_grd);

     }  //end of overlapping processing
     else
     {
         printf ("\ti=%d image does not intersect with domain grid image.\n",i);
     }

     GDALClose( (GDALDatasetH) poRDataset );

   }  //end of i

   printf ("Finished preprocessing images: %s\n",fileType.c_str());
}  //end of the function


/************************************************************************/
/*    computeMODIS( string modisFile )                                  */
/************************************************************************/

void   computeMODIS( string modisFile )  
{

    int                   i,j;
    int                   pixelIndex;
    GDALDataset           *poRDataset;
    GDALRasterBand        *poBand;
    double                adfGeoTransform[6];
    double                xUL,yUL;
    int                   xCells, yCells;       //cells
    double                xCellSize, yCellSize; //cell size
    double                xmin,xmax,ymin,ymax;         //current image extent
    double                intpart; 

    const char            *pszWKT = NULL;
    char                  *pszWKT_nc = NULL;
    OGRSpatialReference   oSRS;
    char                  *pszProj4 = NULL, *tmpProj4=NULL;
   
    GByte                 NoDataValue = MODIS_NODATAVALUE;          // MODIS image nodata value = 255
    int                   gridID, classID, idIndex; 
    int                   outIndex;


   printf( "\nCompute percentage of MODIS landuse classes in domain grid cells...\n" );


/* -------------------------------------------------------------------- */
/*      Open MODIS image                                                */
/* -------------------------------------------------------------------- */
   //open the current i image 
   poRDataset = (GDALDataset *) GDALOpen( modisFile.c_str(), GA_ReadOnly );
   if( poRDataset == NULL )
   {
      printf( "\tError: Open raster file failed: %s.\n", modisFile.c_str() );
      exit( 1 );
   }
/* -------------------------------------------------------------------- */
/*   get rows, columns, cell size, and image extent of MODIS data       */
/* -------------------------------------------------------------------- */
   // get rows and columns of the image
   xCells = poRDataset->GetRasterXSize();
   yCells = poRDataset->GetRasterYSize();
   printf( "  Image size is %dx%dx%d\n", xCells, yCells, poRDataset->GetRasterCount() );

   //get UL corner coordinates and grid size
   if( poRDataset->GetGeoTransform( adfGeoTransform ) == CE_None )
   {
      xUL = adfGeoTransform[0];
      yUL = adfGeoTransform[3];
      printf( "  Image UL Origin = (%.6lf,%.6lf)\n", xUL,yUL );

      xCellSize =  adfGeoTransform[1] ;
      yCellSize =  -1 * adfGeoTransform[5] ;  // Make it positive.  Negative due to Origin is UL corner point
      printf( "  Image Pixel Size = (%.3lf,%.3lf)\n", xCellSize, yCellSize );
   }

 
   //compute extent of MODIS image
   xmin = xUL;
   xmax = xUL + xCellSize * xCells;

   ymax = yUL;
   ymin = yUL - yCellSize * yCells;
   printf( "\tImage extent:  minXY(%.3f,%.3f)   maxXY(%.3f,%.3f)\n", xmin,ymin,xmax,ymax ); 

/* -------------------------------------------------------------------- */
/*   Get image projection to make sure that all images have the same    */
/* -------------------------------------------------------------------- */
   //get projection from the image
   if( (pszWKT = poRDataset->GetProjectionRef())  == NULL )
   {
        printf( "\tError: Projection is not defined in the NLCD file: %s.\n", modisFile.c_str() );
        printf( "\tDefine it using gdal_translate utility.\n");
        exit( 1 );
   }

   //convert it to no const char
   pszWKT_nc =strdup ( pszWKT ); 
   oSRS.importFromWkt( &pszWKT_nc );
   oSRS.exportToProj4( &pszProj4 );    
   oSRS_grd.exportToProj4 (&tmpProj4);

   //make sure that the image has the same projection as the first image's

   //projected rasterized data set has slight different projection specification in GDAL 1.9.1
   /*
   if ( strcmp (pszProj4, tmpProj4) != 0 )
   if (! oSRS.IsSame (&oSRS_grd) )  -- did not work in GDAL 1.9.1
   {
       printf( "\tError: This image's projection is different from the domain grid image's.\n" );
       printf( "\tProjection for this image = %s\n", pszProj4);
       printf( "\tProject it using gdalwarp utility to: %s\n", pszProj4_grd );
       exit ( 1 );
   }
   */


   /********************************************************
    *   If MODIS image and grid domain image intersecting  *
    ********************************************************/
   if (! (xmin >= xMax_grd || xmax <= xMin_grd || ymin >= yMax_grd || ymax <= yMin_grd) )
   { 

/* -------------------------------------------------------------------- */
/*    Image data type has to be GByte                                    */
/* -------------------------------------------------------------------- */
      //get the band 1 image from the image and make sure that it is GByte image
      poBand = poRDataset->GetRasterBand( 1 );  // band 1
      GDALDataType poBandType = poBand->GetRasterDataType();
      printf ("\tImage Type = %d\n",poBandType);
      if ( poBandType !=GDT_Byte )
      {
         printf( "\tError: Image data type is not GDT_Byte: %s.\n", modisFile.c_str() );    
         exit ( 1 );
      }  

/* -------------------------------------------------------------------- */
/*         get image into array                                         */
/* -------------------------------------------------------------------- */
      GByte *poImage = (GByte *) CPLCalloc(sizeof(GByte),xCells*yCells);
      if ( (poBand->RasterIO(GF_Read, 0,0,xCells,yCells,
           poImage,xCells,yCells,GDT_Byte,0,0)) == CE_Failure) 
      {
         printf( "\tError: reading band 1 data from image: %s.\n", modisFile.c_str() );
         CPLFree (poImage);
         exit( 1 );
      }
          
/* -------------------------------------------------------------------- */
/*     read one row at time from domain image due to the size           */
/* -------------------------------------------------------------------- */
      //domain grid image is too big to read once 
      GUInt32 *poImage_grd = (GUInt32 *) CPLCalloc(sizeof(GUInt32),xCells_grd);
             
      double testd = 0.0;
      double x,y;
      int  col,row;
      for( i = 0; i<yCells_grd; i++ )
      {
         if ( (poGridBand->RasterIO(GF_Read, 0, i,xCells_grd, 1,
                                   poImage_grd,xCells_grd,1,GDT_UInt32,0,0)) == CE_Failure)
         {
            printf( "\tError: Reading row = %d from domain grid image.\n",i+1);
            CPLFree (poImage_grd);
            exit( 1 );
         }

          //get cell center point y and compute row in MODIS image
         y = yMax_grd - i * yCellSize_grd - yCellSize_grd / 2.0;
         row = (int) (floor ((ymax - y) / yCellSize));   //start from 0
         if ( row >= 0 && row < yCells )
         {
            for (j=0; j<xCells_grd; j++)
            {
               gridID = poImage_grd[j] ;
               if (gridID > 0 && gridID <= gridPixels)
               {
                  //get cell center point x and compute col in MODIS image
                  x = xMin_grd + j * xCellSize_grd + xCellSize_grd / 2.0;
                  col = (int) (floor ((x - xmin) / xCellSize));  //start from 0
                  if ( col >=0 && col < xCells )
                  {
                     pixelIndex = xCells * row + col;
                     classID = poImage[pixelIndex];       //get MODIS class ID

                     //water: 0, 17 and 254 are set to 0 in extractDomainMODISData function 
                     if ( classID != NoDataValue)
                     {
                        idIndex = modisIDS[classID];                   //index in MODIS class ID pointer
                        outIndex = idIndex * gridPixels + gridID - 1;  //index in output MODIS landuse pointer
                        gridMODIS[outIndex] += 1;                      // count MODIS landuse class in 2d array
                        testd += 1.0;
                        //printf ("\tgot MODIS class: %d\n", classID);
                     }
                  }  // check col
               }  //check gridID
            }  // j
         }  // check row
      }  //i
  
      printf ( "\tCounts = %.0lf\n", testd);
      printf ( "\tFinished counting modeling domain grid ID numbers.\n" );

      CPLFree (poImage);
      CPLFree (poImage_grd);
   }
   else     
   {        
        printf ("\t\tMODIS image does not intersect with domain grid image.\n");
   }  

   GDALClose( (GDALDatasetH) poRDataset );

   printf ("Finished reading MODIS image.\n\n");
}  //end of the function

/***********************/
/*    computeLAI       */
/***********************/

void   computeLAI( gridInfo infoGrd, string modisFile, gridInfo modisInfo, string laiFile, gridInfo laiInfo, std::vector<string> satVars, double *gridLAI, float *laiScaleFactors )  
{

    int                   i,j,k,m,n;
    int                   pixelIndex, pixelIndex_lai;

    GDALDataset           *poRDataset, *poRDataset_lai;
    GDALRasterBand        *poBand, *poBand_lai[satVars.size()];
    double                adfGeoTransform[6];
    double                xUL,yUL;
    int                   xCells, yCells ;                 //cells
    double                xCellSize, yCellSize;            //cell size
    double                xmin,xmax,ymin,ymax;             //current image extent


    double                intpart; 

    const char            *pszWKT = NULL;
    char                  *pszWKT_nc = NULL;
    OGRSpatialReference   oSRS;
    char                  *pszProj4 = NULL, *tmpProj4=NULL;
   
    GByte                 NoDataValue = MODIS_NODATAVALUE;          // MODIS image nodata value = 255

    GByte                 NoDataValue_lai = MODISLAI_NODATAVALUE;          // MODIS image nodata value >= 249

    int                   gridID, classID, laiInt;  
    int                   idIndex, outIndex, outIndex_lai;
    


   printf( "\nCompute average LAI or PAR for MODIS landuse classes in domain grid cells...\n" );

   printf( "\nRasterized domain grid information:\n" );
   printGridInfo (infoGrd );

   printf( "\nExtracted MODIS Land cover information:\n" );
   printGridInfo ( modisInfo );

   printf( "\nExtracted MODIS LAI information:\n" );
   printGridInfo ( laiInfo );

   
   xCells = modisInfo.cols;
   yCells = modisInfo.rows;
   xCellSize = modisInfo.xCellSize;
   yCellSize = modisInfo.yCellSize;

   xmin = modisInfo.xmin;
   xmax = modisInfo.xmax;
   ymin = modisInfo.ymin;
   ymax = modisInfo.ymax;


   /********************************************************
    *   If MODIS image and grid domain image intersecting  *
    ********************************************************/

   if ( xmin >= xMax_grd || xmax <= xMin_grd || ymin >= yMax_grd || ymax <= yMin_grd )
   {

      printf ("\tMODIS land cover image does not intersect with domain grid image.\n");
      return;
   }


   if ( laiInfo.xmin >= xMax_grd || laiInfo.xmax <= xMin_grd || laiInfo.ymin >= yMax_grd || laiInfo.ymax <= yMin_grd )
   {     
         
      printf ("\tMODIS image does not intersect with domain grid image.\n");
      return;
   }


/* -------------------------------------------------------------------- */
/*      Open MODIS image                                                */
/* -------------------------------------------------------------------- */
   //open the current MODIS land cover image 
   poRDataset = (GDALDataset *) GDALOpen( modisFile.c_str(), GA_ReadOnly );
   if( poRDataset == NULL )
   {
      printf( "\tError: Open raster file failed: %s.\n", modisFile.c_str() );
      exit( 1 );
   }


   //open the current MODIS LAI image
   poRDataset_lai = (GDALDataset *) GDALOpen( laiFile.c_str(), GA_ReadOnly );
   if( poRDataset_lai == NULL )
   {
      printf( "\tError: Open raster file failed: %s.\n", laiFile.c_str() );
      exit( 1 );
   }


/* -------------------------------------------------------------------- */
/*    Image data type has to be GByte                                    */
/* -------------------------------------------------------------------- */
   //get the band 1 image from the image and make sure that it is GByte image
   poBand = poRDataset->GetRasterBand( 1 );  // band 1
   GDALDataType poBandType = poBand->GetRasterDataType();
   printf ("\tImage Type = %d\n",poBandType);
   if ( poBandType !=GDT_Byte )
   {
       printf( "\tError: Image data type is not GDT_Byte: %s.\n", modisFile.c_str() );    
       exit ( 1 );
   }  

   for (i=0; i<satVars.size(); i++)
   {
      poBand_lai[i] = poRDataset_lai->GetRasterBand( i+1 );  // band i+1  

      poBandType = poBand_lai[i]->GetRasterDataType();
      //printf ("\tImage Type = %d\n",poBandType);

      if ( poBandType !=GDT_Byte )
      {
         printf( "\tError: Image data type is not GDT_Byte for %d band in %s.\n", i+1, laiFile.c_str() );  
         exit ( 1 );
      }   
   }  //i


/* -------------------------------------------------------------------- */

/*         get image into array                                         */
/* -------------------------------------------------------------------- */
      GByte *poImage = (GByte *) CPLCalloc(sizeof(GByte),xCells*yCells);
      if ( (poBand->RasterIO(GF_Read, 0,0,xCells,yCells,
           poImage,xCells,yCells,GDT_Byte,0,0)) == CE_Failure) 
      {
         printf( "\tError: reading band 1 data from image: %s.\n", modisFile.c_str() );
         CPLFree (poImage);
         exit( 1 );
      }
  
      
      GByte *poImage_lai[satVars.size()];
 
      for ( i=0; i<satVars.size(); i++ )
      {
         poImage_lai[i] = (GByte *) CPLCalloc(sizeof(GByte),laiInfo.cols * laiInfo.rows);

         if ( (poBand_lai[i]->RasterIO(GF_Read, 0,0, laiInfo.cols, laiInfo.rows,
                              poImage_lai[i], laiInfo.cols, laiInfo.rows, GDT_Byte,0,0)) == CE_Failure)
         {
            printf( "\tError: reading band %d data from image %s.\n", i+1, laiFile.c_str() );

            for (j=0; j<=i; j++)
            {
               CPLFree (poImage_lai[j]);
            }
            CPLFree (poImage);

            exit( 1 );
         }
      }
      
          
     
/* -------------------------------------------------------------------- */
/*     read one row at time from domain image due to the size           */
/* -------------------------------------------------------------------- */
      //domain grid image is too big to read once 
      GUInt32 *poImage_grd = (GUInt32 *) CPLCalloc(sizeof(GUInt32),xCells_grd);
             
      double testd = 0.0;
      double x, y;
      int  col, row, col_lai, row_lai;

      for( i = 0; i<yCells_grd; i++ )
      {
         if ( (poGridBand->RasterIO(GF_Read, 0, i,xCells_grd, 1,
                                   poImage_grd,xCells_grd,1,GDT_UInt32,0,0)) == CE_Failure)
         {
            printf( "\tError: Reading row = %d from domain grid image.\n",i+1);
            CPLFree (poImage_grd);

            for (n=0; n<satVars.size(); n++)
            {
               CPLFree (poImage_lai[n]);
            }
            CPLFree (poImage);

            exit( 1 );
         }

          //get cell center point y and compute row in MODIS image
         y = yMax_grd - i * yCellSize_grd - yCellSize_grd / 2.0;

         row = (int) (floor ((ymax - y) / yCellSize));   //start from 0 in landcover

         row_lai = (int) (floor ((laiInfo.ymax - y) / laiInfo.yCellSize));   //start from 0 in LAI

         if ( row >= 0 && row < yCells && row_lai >=0 && row_lai < laiInfo.rows )
         {
            for (j=0; j<xCells_grd; j++)
            {
               gridID = poImage_grd[j] ;

               if (gridID > 0 && gridID <= gridPixels)
               {
                  //get cell center point x and compute col in MODIS image
                  x = xMin_grd + j * xCellSize_grd + xCellSize_grd / 2.0;

                  col = (int) (floor ((x - xmin) / xCellSize));  //start from 0
                  col_lai = (int) (floor ((x - laiInfo.xmin) / laiInfo.xCellSize));  //start from 0

                  if ( col >=0 && col < xCells &&  col_lai >=0 && col_lai < laiInfo.cols )
                  {
                     pixelIndex = xCells * row + col;

                     pixelIndex_lai = laiInfo.cols * row_lai + col_lai;

                     classID = poImage[pixelIndex];       //get MODIS class ID

                     //water: 0, 17 and 254 are set to 0 in extractDomainMODISData function 
                     
                     for ( n=0; n<satVars.size(); n++ )
                     {
                        
                        laiInt =   poImage_lai[n][pixelIndex_lai];
                          
                        if ( classID != NoDataValue && laiInt >= 0 )
                        {

                            /*if (laiInt > 100.0 )
                            {
                               printf ("\tclassID=%d  big LAI=%d\n", classID, laiInt);
                            }*/
                           

                            idIndex = modisIDS[classID];                   //index in MODIS class ID pointer
                            outIndex_lai = n * MODIS_CLASSES_NUM * gridPixels + idIndex * gridPixels + gridID - 1;  //index in output MODIS LAI pointer

                           
                            // compute grid cell average LAI and FPAR - with 0 and no vegetation areas

                            gridMODIS[outIndex_lai] += 1;   //count grid cells

                            if ( laiInt < NoDataValue_lai )
                            {
                               // only add valid value
                               if ( gridLAI[outIndex_lai] == -999.0)
                               {
                                   gridLAI[outIndex_lai] = 0.0;
                               }
                               gridLAI[outIndex_lai] += laiInt * laiScaleFactors[n];  //store LAI in 4D array
                            }
                        
                            //printf ("\tgot MODIS class: %d   %s: %d  %f\n", classID, satVars[n].c_str(), laiInt, laiInt * laiScaleFactors[n] );
                        }

                     } //n

                  }  // check col

               }  //check gridID

            }  // j

         }  // check row
      }  //i
  
   printf ( "\tFinished counting modeling domain grid ID numbers.\n" );

   CPLFree (poImage);
   CPLFree (poImage_grd);

   for ( i=0; i<satVars.size(); i++ )
   {
      CPLFree (poImage_lai[i]);
   }

   GDALClose( (GDALDatasetH) poRDataset );
   GDALClose( (GDALDatasetH) poRDataset_lai );

   printf ("Finished reading MODIS land cover and LAI images.\n\n");

}  //end of the function



/************************************************************************/
/*    computeIntersectionBoxIndexes(...)                                */
/************************************************************************/
void computeIntersectionBox(int xCells, int yCells, double xmin, double ymin, double xmax, double ymax,
                            double xMin_grd, double yMin_grd, double xMax_grd, double yMax_grd,
                            int *col1, int *row1, int *col2, int *row2,
                            int *col1_grd, int *row1_grd, int *col2_grd, int *row2_grd)
{

         //compute indexes of intersection box in domain grid image and image i

         if ( xmin>xMin_grd )
         {
            *col1 = 1;
            *col1_grd =  (int) ( (xmin - xMin_grd) / xCellSize_grd + 1);
         }
         if ( xmin<xMin_grd )
         {
            *col1 = (int) ( (xMin_grd - xmin) / xCellSize_grd + 1);
            *col1_grd = 1;
         }
         if ( xmin==xMin_grd )
         {
            *col1 = 1;
            *col1_grd = 1;  
         }

         //calculate intersection LR columns
         if ( xmax<xMax_grd )
         {
            *col2 = xCells;
            *col2_grd = (int) ( (xmax - xMin_grd) / xCellSize_grd );
         }
         if ( xmax>xMax_grd )
         {
            *col2 = (int) ( (xMax_grd - xmin) / xCellSize_grd );
            *col2_grd = xCells_grd;
         }
         if ( xmax==xMax_grd )   
         {
            *col2 = xCells; 
            *col2_grd = xCells_grd; 
         }
     
         //calculate intersection LR rows 
         if ( ymin>yMin_grd )
         {
            *row2 = yCells;
            *row2_grd = (int) ( (yMax_grd - ymin) / yCellSize_grd ); 
         }
         if ( ymin<yMin_grd )
         {
            *row2 = (int) ( (ymax - yMin_grd) / yCellSize_grd );
            *row2_grd = yCells_grd;
         }
         if ( ymin==yMin_grd )
         {
            *row2 = yCells;
            *row2 = yCells_grd;
         }

         //calculate intersection UL row
         if ( ymax<yMax_grd )
         {
            *row1 = 1;
            *row1_grd = (int) ( (yMax_grd - ymax) / yCellSize_grd + 1); 
         }
         if ( ymax>yMax_grd )
         {
            *row1 = (int) ( (ymax - yMax_grd) / yCellSize_grd + 1);
            *row1_grd = 1;
         }
         if ( ymax==yMax_grd )
         {
            *row1 = 1;
            *row1_grd = 1; 
         }
        
}
