/***************************************************************************
 * This program is used: 
 *  1. to compute grid cell landuse percentage information from:
 *            USGS NLCD landuse, 
 *            USGS NLCD canopy,
 *            USGS NLCD imperviousness, 
 *            MODIS IGBP landuse.
 *  2. to output landuse informaton into a text file table.
 *  3. to output landuse informaton into a WRF netcdf file.
 *
 * Written by the Institute for the Environment at UNC, Chapel Hill
 * in support of the EPA CMAS Modeling, 2007-2013.
 * 
 * By L. Ran, April-June 2008
 *            Modified 12/2011 - 05/2012 - for NLCD2006      
 *            Enhanced 11/2012 - 04/2013 - for NLCD, tiled MODIS, FIA, NASS 
 *
 * Usage:  computeGridLandUse.exe
 *  
 * Environment Variables needed: 
 *         GRID_ROWS  -- grid domain rows
 *         GRID_COLUMNS -- grid domain columns
 *         GRID_XMIN -- grid domain xmin
 *         GRID_YMIN -- grid domain ymin
 *         GRID_XCELLSIZE -- grid domain x grid size 
 *         GRID_YCELLSIZE -- grid domain y grid size 
 *         GRID_PROJ -- grid domain proj4 projection definition
 *
 *         INPUT_FILE_LIST -- Input land cover NLCD and MODIS file list
 *         INCLUDE_NLCD -- YES or NO to include USA NLCD data sets (land cover, imperiousness, canopy)
 *         INCLUDE_MODIS -- YES or NO to include NASA MODIS IGBP landuse data in computation
 *
 *         COUNTY_SHAPEFILE - county shapefile
 *         COUNTY_FIPS_ATTR - county FIPS code
 *         CAN_COUNTY_SHAPEFILE - CAN census (or crop) division shapefile
 *         CAN_COUNTY_FIPS_ATTR - CAN crop division code
 *         US_COUNTY_FIA_FILE - FIA tree fraction data at county
 *         US_COUNTY_NASS_FILE - NASS crop data at county
 *         CAN_CROP_File - CAN crop fraction table at crop division
 *         BELD4_CLASS_NAMES - Landcover and tree class name table
 *
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

#define TIMES 1

//function for computing landuse info
static void  fillLandClassHashTables();
void countGridIDs();
void computeIntersectionBox(int xCells, int yCells, double xmin, double ymin, double xmax, double ymax,
                            double xMin_grd, double yMin_grd, double xMax_grd, double yMax_grd,
                            int *col1, int *row1, int *col2, int *row2,
                            int *col1_grd, int *row1_grd, int *col2_grd, int *row2_grd);
void computeIntersectionBox_WithCnty (double xmin, double ymin, double xmax, double ymax, gridInfo cntyGrid,
                            int *col1_cnty, int *row1_cnty, int *col2_cnty, int *row2_cnty);

void   computeImpe_Cano( std::vector<string> imageFiles, string fileType );
void   computeLandUse(std::vector<string> imageFiles, string fileType);
void   computeMODIS( string modisFile ) ;

void   readFIAFile ( string fiaFile );
void   readNASSFile ( string nassFile );
void   readCANCropFile ( string canCropFile ) ;
void   readClassFile ( string classFile );


//define global variables
string                landType = "USGS NLCD Landuse Files";
string                impeType = "USGS NLCD Urban Imperviousness Files";
string                canoType = "USGS NLCD Tree Canopy Files";
string                modisType = "MODIS Landcover Tiled Files";

GDALDataset           *poGrid;                      //store rasterized 30m grid image
GDALDriver            *poDrive;
GDALRasterBand        *poGridBand;                  //band 1 for rasterized 30m grid image
void computeIntersectionBox_WithCnty (double xmin, double ymin, double xmax, double ymax, gridInfo cntyGrid,
                            int *col1_cnty, int *row1_cnty, int *col2_cnty, int *row2_cnty);

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

std::map<int,int>     nlcdIDS,modisIDS;                     //hash tables to store landuse class IDs and index  


//USGS NLCD NODATA background value: 0 for landcover and canopy   127 for Imperviousness
GByte USGS_NLCD_NODATAVALUE = 0;   //has to be > 0
GByte USGS_NLCD_IMPE_NODATAVALUE = 127;


//MODIS NODATA
GByte MODIS_NODATAVALUE = 255;   //245: unclassified water and 0,17: water  


//MODIS tile land cover class variable: may change depending on NASA MODIS LC products
string modisVarName = string ( "Land_Cover_Type_1" );   //IGBP NASA type
string modisVarName_v6 = string ( "LC_Type1" );


//match NLCD classes by array index: 0 or 127 is background
static int  nlcdClasses[] = {11,12,21,22,23,24,31,41,42,43,51,52,71,72,73,74,81,82,90,95};
    

//MODIS IGBP 255 is NODATA background
static int  modisClasses[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,0,18,19,20};

//MODIS and NLCD land cover class names


const int   NLCD_CLASSES_NUM=20, MODIS_CLASSES_NUM=20;  //number classes in NLCD and MODIS no including NODATA
int         land_cat_len = 0;    //landuse items in output

const int   timeStrLength = 19;       //time string length in WRF Netcdf output


// County, FIA, NASS, class names
int                           FIA_TREE_TOTAL;   //total tree classes
std::map<int, vector<float> > cntyTrees;

std::map<int, vector<float> > cntyCrops81, cntyCrops82, canCDCrops;
std::vector<string>           lcNames, treeNames, cropNames;
int                           nameStrLength = 0;

GDALRasterBand        *poCntyBand;                 //band 1 for rasterized 30m county image
gridInfo               cntyGrid, canCDGrid;        //grid info for county
std::map<int, vector<int> >   gridCNTYs;           //grid canopy contains intersected counties
std::map<string, double >     gridCntyCANO;        //grid and county canopy area

int                           CROP_TOTAL;          //total crop classes
int                           PASTURE_CLASSES = 6;
int                           CROP_CLASSES = 36;       //for 2006 US NASS has 36 and CAN has 36
                                                       //for 2001 US NASS has 32 and CAN has 42

std::map<string, int >        gridCntyCrop;             //grid, county, NLCD crop 81 or 82 area

std::map<int, vector<int> >   grid81CNTYs, grid82CNTYs; //grid crop contains intersected counties
std::map<int, vector<int> >   grid12CNTYs, grid14CNTYs; //grid crop contains intersected CAN counties


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
    string                dataDir;               //directory containing preprocessed Sat. data
    string                satVarName;

    string                inUSGSLand;            //inclusion indicator
    string                inNASALand;            //inclusion indicator
    string                outTextFile, outNetcdfFile;         //output file names

    std::vector<string>   landFiles, impeFiles, canoFiles, modisFiles; 
    std::ifstream         imageStream;             //input NLCD image file list  
    string                fileName, fileType;      //for reading in all NLCD files
    string                tmp_str;
    
    std::ofstream         outTxtStream;            //output text file stream 
    string                lineStr;
 
    int                   i,j,k;                    
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
    char       *luStr;      //40 land cover names
    char       *cropStr;    //42 crop names
    char       *treeStr;    //194 tree names


    float      *imperv;     //3d    time*rows*cols
    float      *canopy;     //3d    time*rows*cols
    float      *tree;       //4d    time*trees*rows*cols

    float      *luf;        //landuse fraction array 4d  time*classes*rows*cols
    float      *cropf;      //crop fractions 4d time*crops*rows*cols
    float      *lum;        //landuse mask 3d array  time*rows*cols
    float      *lud;        //landuse dominant class 3d  time*rows*cols

    int        times = TIMES;

  
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

    grid.name = string( getEnviVariable("GRID_NAME") );
    printf("\tGrid name: %s\n",grid.name.c_str() );

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

    //printf( "Getting indicator to include NLCD landuse data.\n");
    inUSGSLand =  string ( getEnviVariable("INCLUDE_NLCD") ); 
    inUSGSLand = stringToUpper( inUSGSLand );
    inUSGSLand = trim( inUSGSLand );

    //printf( "Getting indicator to include NASA MODIS IGBP landuse data.\n");
    inNASALand = string( getEnviVariable("INCLUDE_MODIS") );
    inNASALand = stringToUpper( inNASALand );
    inNASALand = trim( inNASALand );
   
    // QA selections
    printf( "Indicator to include USGS NLCD landuse data is: %s\n",inUSGSLand.c_str() ); 
    checkSelectionYesNo ( inUSGSLand );

    printf( "Indicator to include NASA MODIS IGBP landuse data is: %s\n",inNASALand.c_str() );
    checkSelectionYesNo ( inNASALand );

    if ( inNASALand.compare("NO") == 0 && inUSGSLand.compare("NO") == 0 )
    {
       printf("No land use data is selected.  Both INCLUDE_NLCD and INCLUDE_MODIS are set to NO.\n");
       exit ( 1 );
    }

    /****************************************************************
    *   Get US county shapefile, FIA data set, and NASS data sets   *
    *****************************************************************/
    string   countyShp, fipsItem;
    string   canCountyShp, canFipsItem;
    string   fiaFile, nassFile, canCropFile, classFile;

    //printf( "Getting U.S. county shapefile.\n");
    countyShp = string( getEnviVariable("COUNTY_SHAPEFILE") );
    countyShp = trim ( countyShp );
    printf("\n\tU.S. county shapefile:  %s\n",countyShp.c_str());
    FileExists(countyShp.c_str(), 0 );  //the file has to exist.

    //printf( "Getting U.S. county shapefile FIPS item name.\n");
    fipsItem = string( getEnviVariable("COUNTY_FIPS_ATTR") );
    fipsItem = trim ( fipsItem );
    printf("\tU.S. county shapefile FIPS item name:  %s\n", fipsItem.c_str());


    //printf( "Getting CAN census division shapefile.\n");
    canCountyShp = string( getEnviVariable("CAN_COUNTY_SHAPEFILE") );
    canCountyShp = trim ( canCountyShp );
    printf("\tCAN census division shapefile:  %s\n",canCountyShp.c_str());
    FileExists(canCountyShp.c_str(), 0 );  //the file has to exist.

    //printf( "Getting CAN census division shapefile Code item name.\n");
    canFipsItem = string( getEnviVariable("CAN_COUNTY_FIPS_ATTR") );
    canFipsItem = trim ( canFipsItem );
    printf("\tCAN census division shapefile code item name:  %s\n\n", canFipsItem.c_str());


    //printf( "Getting U.S. county FIA tree fraction file.\n");
    fiaFile = string( getEnviVariable("US_COUNTY_FIA_FILE") );
    fiaFile = trim ( fiaFile );
    printf("\tU.S. county FIA tree fraction file:  %s\n",fiaFile.c_str());
    FileExists(fiaFile.c_str(), 0 );  //the file has to exist.

    //printf( "Getting U.S. county NASS crop fraction file.\n");
    nassFile = string( getEnviVariable("US_COUNTY_NASS_FILE") );
    nassFile = trim ( nassFile );
    printf("\tU.S. county NASS crop fraction file:  %s\n",nassFile.c_str());
    FileExists(nassFile.c_str(), 0 );  //the file has to exist.

    //printf( "Getting CAN census division crop fraction file.\n");
    canCropFile = string( getEnviVariable("CAN_CROP_FILE") );
    canCropFile = trim ( canCropFile );
    printf("\tCAN census division crop fraction file:  %s\n", canCropFile.c_str());
    FileExists(canCropFile.c_str(), 0 );  //the file has to exist.


    //printf( "Getting land cover and tree class name file.\n");
    classFile = string( getEnviVariable("BELD4_CLASS_NAMES") );
    classFile = trim ( classFile );
    printf("\tU.S. land cover and tree class name file:  %s\n",classFile.c_str());
    FileExists(classFile.c_str(), 0 );  //the file has to exist.


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


/* -------------------------------------------- */
/*  Read in landcover and tree class name list  */
/* -------------------------------------------- */
    readClassFile ( classFile );

/* -------------------------------------------- */
/*  Read in FIA tree fraction data at county    */
/* -------------------------------------------- */

    readFIAFile ( fiaFile );

/* -------------------------------------------- */
/*  Read in NASS crop fraction data at county   */
/* -------------------------------------------- */
    readNASSFile ( nassFile );

/* ----------------------------------------------------- */
/*  Read in CAN crop fraction data at census divisions   */
/* -----------------------------------------         --- */
    readCANCropFile ( canCropFile );
   

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
    else if ( inNASALand.compare("YES") == 0 && modisFiles.size() > 0 )
    {  
       //!!! may need to change based on MODIS data sets
       //Get MODIS land cover file info

       imageFile = modisFiles.at(0);    //use the first MODIS image files
      
       if ( imageFile.find( "MCD12Q1" ) != string::npos && imageFile.find( ".006." ) != string::npos )
       {
           modisVarName = modisVarName_v6;
       }

       imageInfo = getHDF4VarInfo (imageFile, modisVarName);
    } 
    else
    {
       printf ( "\tError: No input image data files from US NLCD landcover or MODIS Landcover\n" );      
       exit ( 1 );
    }

    //get projection from the image as the standard grid for following image processing
    //convert it to const char - projection obtained from projected/rastered image is a little different
    //from projection obtained from infoGrd below

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


    /*********************************/
    /*    Compute raster resolution  */
    /*********************************/
    double  rasterResolution =  computeRasterResolution ( imageInfo, grid );


/* ------------------------------------------------------------------- */
/*     created raster 30m grid domain in NLCD or MODIS projection      */
/* ------------------------------------------------------------------- */
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
       grid.name = shapeFile;
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
    

/* ----------------------------------------------- */
/*     project and rasterize US county shapefile      */
/* ----------------------------------------------- */
    string  projectedCountyFile;
    string  cntyRasterFile;

    //project the shapefile
    printf ("\tProject and rasterize county shapefile: %s...\n", countyShp.c_str());

    projectedCountyFile =  projectShape ( gdalBinDir, countyShp, infoGrd.strProj4 );
    cntyGrid = computeNewRasterInfo ( projectedCountyFile, rasterResolution, grid);
    cntyGrid.polyID =  fipsItem;

    //rasterize the projected shapefile
    cntyRasterFile = toRasterFile (gdalBinDir, cntyGrid, imageFile, projectedCountyFile, cntyGrid);

    //delete projected Shapefiles
    deleteShapeFile ( projectedCountyFile );


/* ----------------------------------------------------------- */
/*     project and rasterize CAN census division shapefile    */
/* ----------------------------------------------------------- */
    string  projectedCANCDFile;
    string  canCDRasterFile;

    //project the shapefile
    printf ("\tProject and rasterize CAN census division shapefile: %s...\n", canCountyShp.c_str());

    projectedCANCDFile =  projectShape ( gdalBinDir, canCountyShp, infoGrd.strProj4 );
    canCDGrid = computeNewRasterInfo ( projectedCANCDFile, rasterResolution, grid);
    canCDGrid.polyID =  canFipsItem;

    //rasterize the projected shapefile
    canCDRasterFile = toRasterFile (gdalBinDir, canCDGrid, imageFile, projectedCANCDFile, canCDGrid);

    //delete projected Shapefiles
    deleteShapeFile ( projectedCANCDFile );


/* ----------------------------------------------------------------- */
/*     Clip and project MODIS land cover data into NLCD projection   */
/* ----------------------------------------------------------------- */
    gridInfo  modisInfoNew;
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

       modisFileNew = projectRasterFile ( modisFile, gridMODISInfo, modisInfoNew );
 
       //delete extracted modis image file
       deleteRasterFile ( modisFile );

       //projected MODIS file in NLCD projection
       modisFile = modisFileNew;
     
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


/* ------------------------------------------------ */
/*     Open rasterized 30m US county grid file      */
/* ------------------------------------------------ */
    GDALDataset           *poCntyGrid;         //store rasterized 30m county image

    printf( "\nOpen projected and rasterized county image file: %s\n", cntyRasterFile.c_str() );

    //open the grid domain image
    poCntyGrid = (GDALDataset *) GDALOpen( cntyRasterFile.c_str(), GA_Update );
    if( poCntyGrid == NULL )
    {
       printf( "\tError: Open raster file failed: %s.\n", cntyRasterFile.c_str() );
       exit( 1 );
    }
    poCntyBand = poCntyGrid->GetRasterBand( 1 );  // band 1


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

   if ( inUSGSLand.compare("YES") == 0 )
   {

/* ------------------------------------------------------------------- */ 
/*    compute USGS NLCD urban imperviousness percentage                */
/* ------------------------------------------------------------------- */ 
      printf( "\n\nUSGS NLCD Urban Imperviousness\n");
      gridIMPE = (double *) CPLCalloc(sizeof(double),gridPixels);
      computeImpe_Cano( impeFiles, impeType );

/* ------------------------------------------------------------------- */
/*    compute USGS  NLCD canopy percentage                             */
/* ------------------------------------------------------------------- */
      printf( "\n\nUSGS NLCD Tree Canopy\n");
      gridCANO = (double *) CPLCalloc(sizeof(double),gridPixels);
      computeImpe_Cano( canoFiles, canoType );

/* --------------------------------------------- */
/*    compute  NLCD landuse percentage           */
/* --------------------------------------------- */
      gridNLCD = (int *) CPLCalloc(sizeof(int),gridPixels * NLCD_CLASSES_NUM);
      printf( "\n\nUSGS NLCD Landuse\n");
      computeLandUse( landFiles, landType );
      poGrid->FlushCache();   //write modified domain grid image
   }


/* --------------------------------------------- */
/*   Close rasterized 30m US county grid file    */
/* --------------------------------------------- */
   GDALClose( (GDALDatasetH) poCntyGrid );


/* ----------------------------------------------------- */
/*   Open rasterized 30m CAN census division grid file   */
/* ----------------------------------------------------- */

   printf( "\nOpen projected and rasterized CAN census division image file: %s\n", canCDRasterFile.c_str() );

   //open the grid domain image
   poCntyGrid = (GDALDataset *) GDALOpen( canCDRasterFile.c_str(), GA_Update );
   if( poCntyGrid == NULL )
   {
      printf( "\tError: Open raster file failed: %s.\n", canCDRasterFile.c_str() );
      exit( 1 );
   }
   poCntyBand = poCntyGrid->GetRasterBand( 1 );  // band 1


/* -------------------------------------------------------------------- */
/*     compute NASA MODIS IGBP landuse percentage                       */
/* -------------------------------------------------------------------- */
   if ( inNASALand.compare("YES") == 0 )
   {
      gridMODIS = (int *) CPLCalloc(sizeof(int), gridPixels * MODIS_CLASSES_NUM);
      printf( "\n\nNASA MODIS IGBP Landuse\n");
      computeMODIS( modisFile );
   }


/* ------------------------------------------------------------------- */
/*   Close rasterized 30m grid domain and CAN census division files    */
/* ------------------------------------------------------------------- */
   GDALClose( (GDALDatasetH) poGrid );
   GDALClose( (GDALDatasetH) poCntyGrid );


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
  

   /*------------------------------------------------------------*/
   /*            Write text output file title                    */                  
   /*            Allocate arrays for netcdf output               */
   /*------------------------------------------------------------*/

   char  temp_char[100];
   lineStr = string ("GRIDID,ROW,COL");
   
   //get number of landuse classes -- will always output 40 classes
   land_cat_len = MODIS_CLASSES_NUM + NLCD_CLASSES_NUM;

  
   printf( "Number of total landuse classes: %d - %d from MODIS and %d from NLCD\n",land_cat_len, MODIS_CLASSES_NUM,NLCD_CLASSES_NUM);


   if ( inUSGSLand.compare("YES") == 0 )
   {
       lineStr.append( ",IMPERV" );
       if ( (imperv = (float *) calloc (times*rows*cols, sizeof(float)) ) == NULL)
       {
          printf( "Calloc imperv failed.\n");
          exit ( 1 );
       }

       lineStr.append( ",CANOPY" );
       if ( (canopy = (float *) calloc (times*rows*cols, sizeof(float)) ) == NULL)
       {
          printf( "Calloc canopy failed.\n");
          exit ( 1 );
       }
       
       //fill tree name string
       if ( (treeStr = (char *) calloc (FIA_TREE_TOTAL*nameStrLength+1, sizeof(char)) ) == NULL)
       {
          printf( "Calloc treeStr failed.\n");
          exit ( 1 );
       }

       fillCharArray ( nameStrLength, treeStr, treeNames );

      
       if ( (tree = (float *) calloc (times*FIA_TREE_TOTAL*rows*cols, sizeof(float)) ) == NULL)
       {
          printf( "Calloc tree failed.\n");
          exit ( 1 );
       }
   }


   //allocate mem to landuse class list
   if ( (lu_list = (int*) calloc (times*land_cat_len, sizeof(int)) ) == NULL)
   {
      printf( "Calloc lu_list failed.\n");
      exit ( 1 );
   }
  
   //allocate mem to landuse class name
   if ( (luStr = (char *) calloc (times*land_cat_len*nameStrLength+1, sizeof(char)) ) == NULL)
   {
      printf( "Calloc luStr failed.\n");
      exit ( 1 );
   }

   fillCharArray ( nameStrLength, luStr, lcNames );


   //allocate mem to landuse fraction array
   if ( (luf = (float*) calloc (times*land_cat_len*rows*cols, sizeof(float)) ) == NULL)
   {
      printf( "Calloc luf failed.\n");
      exit ( 1 );
   }

   //fill crop name string -- will have it for both NLCD and MODIS
   if ( ( cropStr = (char *) calloc ( CROP_TOTAL*nameStrLength+1, sizeof(char)) ) == NULL)
   {
      printf( "Calloc cropStr failed.\n");
      exit ( 1 );
   }

   fillCharArray ( nameStrLength, cropStr, cropNames );


   //allocate mem to crop fraction array
   if ( (cropf = (float*) calloc (times*CROP_TOTAL*rows*cols, sizeof(float)) ) == NULL)
   {
      printf( "Calloc cropf failed.\n");
      exit ( 1 );
   }


   //allocate mem to landuse mask array
   if ( (lum = (float*) calloc (times*rows*cols, sizeof(float)) ) == NULL)
   {
      printf( "Calloc lum failed.\n");
      exit ( 1 );
   }

   //allocate mem to landuse dominant class array
   if ( (lud = (float*) calloc (times*rows*cols, sizeof(float)) ) == NULL)
   {
      printf( "Calloc lud failed.\n");
      exit ( 1 );
   }


   /*********************************
   * prepare land cover class data  *
   **********************************/

   for (j=0; j<land_cat_len; j++)
   {
      sprintf( temp_char,",%d",j+1);
      lineStr.append( temp_char );
      lu_list[j] = j+1;
   }


   lineStr.append( "\n" );
   outTxtStream.write( lineStr.c_str(), lineStr.size() );
   

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
   
      if ( inUSGSLand.compare("YES") == 0 )
      {   
         sprintf( temp_char,",%.5lf",100.0*gridIMPE[i]/gridArea);   //area percent
         lineStr.append( temp_char );

         k = (row - 1) * cols + col - 1 ;  //index in 1d array
         imperv[k] = 100.0*gridIMPE[i]/gridArea;

         //printf ("IMPE=%lf\n",imperv[k]);


         sprintf( temp_char,",%.5lf",100.0*gridCANO[i]/gridArea);
         lineStr.append( temp_char );

         k = (row - 1) * cols + col - 1 ;  //index in 1d array
         canopy[k] = 100.0*gridCANO[i]/gridArea;

         //printf ("CANOP=%lf\n",canopy[k]);

         //compute tree fractions at grids
         double qaArea1 = 0.0;
         double qaArea2 = 0.0;
         std::vector<int>    vecFIPS;     //vector contains counties for a grid
         if ( gridCNTYs.find ( gridID ) != gridCNTYs.end() )
         {        
            //get county fips vector  
            vecFIPS = gridCNTYs[gridID];
                  
            for (int m=0; m<vecFIPS.size(); m++)
            {        
               //printf ( "\tgridID:%d   county %d FIPS: %d\n", gridID, m, vecFIPS[m]);
                     
               sprintf( temp_char,"%d.%d\0",gridID,vecFIPS[m] );
               tmp_str = string ( temp_char );

               if ( gridCntyCANO.find ( tmp_str )  != gridCntyCANO.end() )
               {
                  //get canopy area with a grid and county
                  double canoArea = gridCntyCANO[tmp_str];
                  qaArea1 += gridCntyCANO[tmp_str];

                  //printf ( "\tCounty grid canopy area:%lf\n", canoArea );

                  //get county tree fraction vector
                  std::vector<float>  vecFloat;    //vector contains tree fractions for a county

                  if ( cntyTrees.find ( vecFIPS[m] ) != cntyTrees.end() )
                  {
                     vecFloat = cntyTrees[ vecFIPS[m] ];
                     for (j=0; j<FIA_TREE_TOTAL; j++)
                     {
                        //printf ( "\tCounty FIA tree fraction: %d   %f\n", j+39, vecFloat[j] );

                        k = j * rows * cols + (row - 1) * cols + col - 1;
                        tree[k] += 100.0 * canoArea * vecFloat[j] /gridArea;
                        qaArea2 += 100.0 * canoArea * vecFloat[j] /gridArea;
                     } //for each tree
                  }  //for each county
                  else
                  {
                     //printf ("\tWarning: For gridID %d no FIA tree data for intersected county: %d\n", gridID, vecFIPS[m] );
                  }
               } //if a grid and county portion has canopy data
               else
               {
                  //printf ("\tWarning: No intersected canopy areas for gridID %d and county %d\n", gridID, vecFIPS[m] );
               }
            } //for m each county

         //printf ("gridCanopy=%lf    grid and cnounty canopy=%lf    Total county canopy=%lf\n", gridCANO[i], qaArea1, qaArea2);

         } //if grid has counties
         else
         {
             //printf ("\tWarning: No county for NLCD canopy area in gridID: %d\n", gridID );
         }

      }  //canopy computation


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


            if ( percent = 0.0 )
            {
               continue;      //skipping crop fraction computation in the grid
            }
             

            //compute crop fractions at grids
            std::vector<int>    vecFIPS;     //vector contains counties for a grid

            int qaArea1 = 0;
            double qaArea2 = 0;

            if ( j == 11 && grid12CNTYs.find ( gridID ) != grid12CNTYs.end() )
            {
               //get county fips vector
               vecFIPS = grid12CNTYs[gridID];
            }
            else
            if ( j == 13 && grid14CNTYs.find ( gridID ) != grid14CNTYs.end() )
            {
               //get county fips vector
               vecFIPS = grid14CNTYs[gridID];
            }

            for (int m=0; m<vecFIPS.size(); m++)
            {
               //printf ( "\tgridID:%d   county# %d FIPS: %d  Crop: %d\n", gridID, m, vecFIPS[m], j);

               sprintf( temp_char,"%d.%d.%d\0",gridID, vecFIPS[m], j+1 );   //12 or 14 crops
               tmp_str = string ( temp_char );
 
               if ( gridCntyCrop.find ( tmp_str ) != gridCntyCrop.end() )
               {
                  //get crop area with a grid and county
                  int cropArea = gridCntyCrop[tmp_str];
                  qaArea1 += gridCntyCrop[tmp_str];

                  //printf ( "\tCounty grid crop area:%d\n", cropArea );

                  //get county crop fraction vector
                  std::vector<float>   vecFloat;       //vector contains crop fractions for a county
                  int    cropIndex1=0, cropIndex2=0;  //initialize index numbers


                  //!!!!!!!!!!!!!!!needs changes in case no NLCD data

                  if ( canCDCrops.find ( vecFIPS[m] ) != canCDCrops.end() )
                  {
                     vecFloat = canCDCrops[ vecFIPS[m] ];
                     cropIndex1 = 0;
                     cropIndex2 = CROP_TOTAL;
                  }

                  for (int n=cropIndex1; n<cropIndex2; n++)
                  {
                      //printf ( "\tCAN Census Division crop fraction: %d   %f\n", n+1, vecFloat[n] );

                      k = n * rows * cols + (row - 1) * cols + col - 1;
                      if ( j == 13 )
                      {
                         // count 0.67 area if it is crop+vegetation mosaic class 14
                         percent = 0.67 * 100.0 *  cropArea * vecFloat[n] / gridIDS[i];
                      }
                      else
                      {
                         percent = 100.0 *  cropArea * vecFloat[n] / gridIDS[i];
                      }

                      cropf[k] += percent;
                      qaArea2 += percent;
                  } //for each crop
               }  //if grid, county,crop has data
               else
               {
                   //printf ("\tWarning: No MODIS %d crop for gridID: %d  County: %d\n", j, gridID, vecFIPS[m] );
               }

            } // /m each county

            
            /*
            if (j == 11 || j == 13)
            {
               printf ("MODIS Crop: %d  gridCrop=%d   grid and county crop=%d   Total county crop=%lf 30mgridCounts=%d \n",  j, gridNLCD[j*gridPixels+i], qaArea1, qaArea2, gridIDS[i]);
            }
            */

         }  //end of j - MODIS



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
           
                                  
            if ( percent = 0.0 )
            {         
               continue;      //skipping crop fraction computation in the grid
            }            

            //!!!!!!!!!!!!!!!needs changes in case no NLCD data == put CAN and US in one array tables


            //compute crop fractions at grids
            std::vector<int>    vecFIPS;     //vector contains counties for a grid
             
            int qaArea1 = 0;
            double qaArea2 = 0;
            
            if ( j == 16 && grid81CNTYs.find ( gridID ) != grid81CNTYs.end() )
            {  
               //get county fips vector
               vecFIPS = grid81CNTYs[gridID];
            }
            else
            if ( j == 17 && grid82CNTYs.find ( gridID ) != grid82CNTYs.end() )
            {
               //get county fips vector
               vecFIPS = grid82CNTYs[gridID];
            }

            for (int m=0; m<vecFIPS.size(); m++)
            {
               //printf ( "\tgridID:%d   county# %d FIPS: %d  Crop: %d\n", gridID, m, vecFIPS[m], j+65);

               sprintf( temp_char,"%d.%d.%d\0",gridID, vecFIPS[m], j+65 );   // j+65 = 81 or 82 crops
               tmp_str = string ( temp_char );

               if ( gridCntyCrop.find ( tmp_str )  != gridCntyCrop.end() )
               {
                  //get crop area with a grid and county
                  int cropArea = gridCntyCrop[tmp_str];
                  qaArea1 += gridCntyCrop[tmp_str];

                  //printf ( "\tCounty grid crop area:%d\n", cropArea );

                  //get county crop fraction vector
                  std::vector<float>  vecFloat;       //vector contains crop fractions for a county
                  int    cropIndex1=0, cropIndex2=0;  //initialize index numbers

                  if ( j == 16 && cntyCrops81.find ( vecFIPS[m] ) != cntyCrops81.end() )
                  {
                     vecFloat = cntyCrops81[ vecFIPS[m] ];
                     cropIndex1 = 0;
                     cropIndex2 = PASTURE_CLASSES;
                  }
                  else if (  j == 17 && cntyCrops82.find ( vecFIPS[m] ) != cntyCrops82.end() )
                  {
                     vecFloat = cntyCrops82[ vecFIPS[m] ];
                     cropIndex1 = PASTURE_CLASSES;
                     cropIndex2 = PASTURE_CLASSES + CROP_CLASSES;
                  }

                  for (int n=cropIndex1; n<cropIndex2; n++)
                  {
                     //printf ( "\tCounty NASS crop fraction: %d   %f\n", n+1, vecFloat[n-cropIndex1] );

                     k = n * rows * cols + (row - 1) * cols + col - 1;
                     percent = 100.0 *  cropArea * vecFloat[n-cropIndex1] / gridIDS[i];
                     cropf[k] += percent;
                     qaArea2 += percent;
                  } //for each crop
               }  //if grid, county,crop has data
               else
               {
                  //printf ("\tWarning: No NLCD %d crop for gridID: %d  County: %d\n", j+65, gridID, vecFIPS[m] );
               }

            } // /m each county

             
            /*
            if (j ==16 || j == 17)
            {
               printf ("NLCD Crop: %d  gridCrop=%d   grid and cnounty crop=%d   Total county crop=%lf 30mgridCounts=%d \n",  j+65, gridNLCD[j*gridPixels+i], qaArea1, qaArea2, gridIDS[i]);
            }
            */

         } //j - end of NLCD LU

      
         //!!!!!!!!!!! need to check water mask when there is no MODIS along TX coast 1 or 4km  domain

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

   //printf ( "\tcount percent > 0: %.0lf\n", testd);

   //printf ( " r1 c7 IMP=%f  CAN=%lf  LUF=%lf  LUM=%d  LUD=%d\n",imperv[6], canopy[6],luf[2456506],lum[6],lud[6]);
   //printf ( " r1 c7 IMP=%f  CAN=%lf  LUF=%lf  LUM=%d  LUD=%d\n",imperv[34999], canopy[34999],luf[179499],lum[34999],lud[34999]);
   //printf ( " r1 c7 IMP=%f  CAN=%lf  LUF=%lf  LUM=%d  LUD=%d\n",imperv[72249], canopy[72249],luf[2962249],lum[72249],lud[72249]);
  

   /**********************************************
   * free all hashtables for landuse computation *
   * delete used files   *                       *
   **********************************************/
   if ( inUSGSLand.compare("YES") == 0 )
   {
      CPLFree (gridIMPE);
      CPLFree (gridCANO);
 
      gridCNTYs.clear();
      gridCntyCANO.clear();

      CPLFree (gridNLCD);
      
      grid81CNTYs.clear();
      grid82CNTYs.clear();
      gridCntyCrop.clear();
   }

   if ( inNASALand.compare("YES") == 0 )
   {
        CPLFree (gridMODIS);
        grid12CNTYs.clear();
        grid14CNTYs.clear();
        gridCntyCrop.clear();
   }

   CPLFree (gridIDS);

   //delete 30m domain grid image after processing
   deleteRasterFile ( gridRasterFile );

   //delete 30m county image after processing
   deleteRasterFile ( cntyRasterFile );

   //delete clipped and projected MODIS file
   if ( inNASALand.compare("YES") == 0 )
   {
      deleteRasterFile ( modisFile );
   }

   //delete 30m CAN census division image after processing
   deleteRasterFile ( canCDRasterFile );

/*--------------------------------------------------------------*/
/* Prepare data to write WRF landuse data file in netCDF format */
/*--------------------------------------------------------------*/

   /**********************************
   * Compute times and timesStr data *
   **********************************/
   char       *timesStr;   //time string

   printf("\nObtain times in seconds from the start and time string in each step data...\n");

   if ( (timesStr = (char *) calloc (times*timeStrLength+1, sizeof(char)) ) == NULL)
   {
       printf( "Calloc timesStr failed.\n");
       exit ( 1 );
   }

   tmp_str = string ( "0000-00-00_00:00:00" );
   strcpy (timesStr, tmp_str.c_str() );   //asign time string 1 day only

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
   size_t time_len = times;  

   size_t tree_cat_len = FIA_TREE_TOTAL;
   size_t crop_cat_len = CROP_TOTAL;
   size_t nameStr_len = nameStrLength;


   /********************
   * Define dimensions *
   ********************/
   int dateStr_dim;
   int time_dim;
   int tree_cat_dim;
   int west_east_dim;
   int south_north_dim;
   int land_cat_dim;
   int crop_cat_dim;
   int nameStrLen_dim;

   printf( "Defining dimensions in output netcdf file...\n" );

   anyErrors( nc_def_dim(ncid, "Time", NC_UNLIMITED, &time_dim) );
   anyErrors( nc_def_dim(ncid, "DateStrLen", dateStr_len, &dateStr_dim) );
   anyErrors( nc_def_dim(ncid, "west_east", west_east_len, &west_east_dim) );
   anyErrors( nc_def_dim(ncid, "south_north", south_north_len, &south_north_dim) );

   if ( inUSGSLand.compare("YES") == 0 ||  inNASALand.compare("YES") == 0 )
   {
      anyErrors( nc_def_dim(ncid, "land_cat", land_cat_len, &land_cat_dim) );
      anyErrors( nc_def_dim(ncid, "crop_cat", crop_cat_len, &crop_cat_dim) );
      anyErrors( nc_def_dim(ncid, "tree_cat", tree_cat_len, &tree_cat_dim) );
      anyErrors( nc_def_dim(ncid, "nameStrLen", nameStrLength, &nameStrLen_dim) );
   }


   /**********************
   * Define variables IDs*
   ***********************/
   int times_id;
   int lu_list_id;
   int imperv_id;
   int canopy_id;
   int tree_id;
   int lum_id;
   int luf_id;
   int lud_id;
   int crop_id;

   int luName_id;
   int cropName_id;
   int treeName_id;

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

   /*******************************
   * Leave define mode for output *
   ********************************/
   anyErrors( nc_enddef (ncid) );

   /****************************************
   * Write the time variables to output *
   *****************************************/
   //anyErrors( nc_put_var_text(ncid, times_id, timesStr) );  //did not work
   long tx_start[2];
   long tx_count[2];

   tx_start[0]=0;
   tx_start[1]=0;
   tx_count[0]=time_len;
   tx_count[1]=dateStr_len;
   anyErrors( ncvarput(ncid, times_id, tx_start, tx_count, timesStr) );
   printf( "\tWrited Times string\n\n" );

   //Write grid coordinate variables
   writeWRFGridCoordinates_time ( ncid, time_dim, west_east_dim, south_north_dim, grid );

   free (timesStr);
   free(grid.x);
   free(grid.y);
   free(grid.lat);
   free(grid.lon);

   /***************************************
   * Set define mode for landuse output   *
   ****************************************/
   anyErrors( nc_redef (ncid) );

   dimIndex[0] = time_dim;
   dimIndex[1] = south_north_dim;
   dimIndex[2] = west_east_dim;

   if ( inUSGSLand.compare("YES") == 0 )
   {
      anyErrors( nc_def_var(ncid, "IMPERV", NC_FLOAT, 3, dimIndex, &imperv_id) );
      anyErrors( nc_def_var(ncid, "CANOPY", NC_FLOAT, 3, dimIndex, &canopy_id) );

      dimIndex[0] = time_dim;
      dimIndex[1] = tree_cat_dim;
      dimIndex[2] = nameStrLen_dim;
      anyErrors( nc_def_var(ncid, "TREE_NAME", NC_CHAR, 3, dimIndex, &treeName_id) );

      dimIndex[0] = time_dim;
      dimIndex[1] = tree_cat_dim;
      dimIndex[2] = south_north_dim;
      dimIndex[3] = west_east_dim;
      anyErrors( nc_def_var(ncid, "TREEF", NC_FLOAT, 4, dimIndex, &tree_id) );
   }

   dimIndex[0] = time_dim;
   dimIndex[1] = land_cat_dim;
   anyErrors( nc_def_var(ncid, "LU_LIST", NC_INT, 2, dimIndex, &lu_list_id) );

   dimIndex[0] = time_dim;
   dimIndex[1] = land_cat_dim; 
   dimIndex[2] = nameStrLen_dim;
   anyErrors( nc_def_var(ncid, "LU_NAME", NC_CHAR, 3, dimIndex, &luName_id) );

   dimIndex[0] = time_dim;
   dimIndex[1] = land_cat_dim;
   dimIndex[2] = south_north_dim;
   dimIndex[3] = west_east_dim;
   anyErrors( nc_def_var(ncid, "LANDUSEF", NC_FLOAT, 4, dimIndex, &luf_id) );

   dimIndex[0] = time_dim;
   dimIndex[1] = crop_cat_dim;
   dimIndex[2] = nameStrLen_dim;
   anyErrors( nc_def_var(ncid, "CROP_NAME", NC_CHAR, 3, dimIndex, &cropName_id) );

   dimIndex[0] = time_dim;
   dimIndex[1] = crop_cat_dim;
   dimIndex[2] = south_north_dim;
   dimIndex[3] = west_east_dim;
   anyErrors( nc_def_var(ncid, "CROPF", NC_FLOAT, 4, dimIndex, &crop_id) ); 

   dimIndex[0] = time_dim;
   dimIndex[1] = south_north_dim;
   dimIndex[2] = west_east_dim;
   anyErrors( nc_def_var(ncid, "LANDMASK", NC_FLOAT, 3, dimIndex, &lum_id) );
   anyErrors( nc_def_var(ncid, "LU_INDEX", NC_FLOAT, 3, dimIndex, &lud_id) );


   /********************
   * Assign attributes *
   ********************/
   printf( "\nAssigning variable attributes in output netcdf file...\n" );

   if ( inUSGSLand.compare("YES") == 0 )
   {
      // Impervious surface coverage
      anyErrors( nc_put_att_int(ncid, imperv_id, "FieldType", NC_INT, 1, fieldtype) );
      anyErrors( nc_put_att_text(ncid, imperv_id, "MemoryOrder", 3, "XY ") );
      tmp_str = string ( "Percent of impervious surface in WRF grid" );
      anyErrors( nc_put_att_text(ncid, imperv_id, "description", tmp_str.size(), tmp_str.c_str() ) );
      anyErrors( nc_put_att_text(ncid, imperv_id, "units", 7, "Percent") );
      anyErrors( nc_put_att_text(ncid, imperv_id, "stagger", 1, "M") );
      anyErrors( nc_put_att_int(ncid, imperv_id, "sr_x", NC_INT, 1, sr_xy) );
      anyErrors( nc_put_att_int(ncid, imperv_id, "sr_y", NC_INT, 1, sr_xy) );

      // Forest canopy
      anyErrors( nc_put_att_int(ncid, canopy_id, "FieldType", NC_INT, 1, fieldtype) );
      anyErrors( nc_put_att_text(ncid, canopy_id, "MemoryOrder", 3, "XY ") );
      tmp_str = string ( "Percent of forest tree canopy in WRF grid" );
      anyErrors( nc_put_att_text(ncid, canopy_id, "description", tmp_str.size(), tmp_str.c_str() ) );
      anyErrors( nc_put_att_text(ncid, canopy_id, "units", 7, "Percent") );
      anyErrors( nc_put_att_text(ncid, canopy_id, "stagger", 1, "M") );
      anyErrors( nc_put_att_int(ncid, canopy_id, "sr_x", NC_INT, 1, sr_xy) );
      anyErrors( nc_put_att_int(ncid, canopy_id, "sr_y", NC_INT, 1, sr_xy) );

      //tree name
      tmp_str = string ( "Tree class name" );
      anyErrors( nc_put_att_text(ncid, treeName_id, "description", tmp_str.size(), tmp_str.c_str() ) );
      anyErrors( nc_put_att_text(ncid, treeName_id, "units", 4 , "None") );

      //canopy tree fractions
      anyErrors( nc_put_att_int(ncid, tree_id, "FieldType", NC_INT, 1, fieldtype) );
      anyErrors( nc_put_att_text(ncid, tree_id, "MemoryOrder", 3, "XY ") );
      tmp_str = string ( "percent of FIA trees in grids" );
      anyErrors( nc_put_att_text(ncid, tree_id, "Description", tmp_str.size(), tmp_str.c_str() ) );
      anyErrors( nc_put_att_text(ncid, tree_id, "Units", 7, "Percent") );
      anyErrors( nc_put_att_text(ncid, tree_id, "stagger", 1, "M") );
      anyErrors( nc_put_att_int(ncid, tree_id, "sr_x", NC_INT, 1, sr_xy) );
      anyErrors( nc_put_att_int(ncid, tree_id, "sr_y", NC_INT, 1, sr_xy) );
   }
   
   
   // MODIS-NLCD landuse
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
   tmp_str = string ( "Land cover class name" );
   anyErrors( nc_put_att_text(ncid, luName_id, "description", tmp_str.size(), tmp_str.c_str() ) );
   anyErrors( nc_put_att_text(ncid, luName_id, "units", 4 , "None") );
      
   // landuse fraction
   anyErrors( nc_put_att_int(ncid, luf_id, "FieldType", NC_INT, 1, fieldtype) );
   anyErrors( nc_put_att_text(ncid, luf_id, "MemoryOrder", 3, "XY ") );
   tmp_str = string ( "Percent of 20 MODIS IGBP and 20 NLCD land use classes" );
   anyErrors( nc_put_att_text(ncid, luf_id, "description", tmp_str.size(), tmp_str.c_str() ) );
   anyErrors( nc_put_att_text(ncid, luf_id, "units", 7, "Percent") );
   anyErrors( nc_put_att_text(ncid, luf_id, "stagger", 1, "M") );
   anyErrors( nc_put_att_int(ncid, luf_id, "sr_x", NC_INT, 1, sr_xy) );
   anyErrors( nc_put_att_int(ncid, luf_id, "sr_y", NC_INT, 1, sr_xy) );

   //crop class names
   tmp_str = string ( "Crop class name" );
   anyErrors( nc_put_att_text(ncid, cropName_id, "description", tmp_str.size(), tmp_str.c_str() ) );
   anyErrors( nc_put_att_text(ncid, cropName_id, "units", 4 , "None") );

   //CROP fraction from NLCD/MODIS crop/pasture land
   anyErrors( nc_put_att_int(ncid, crop_id, "FieldType", NC_INT, 1, fieldtype) );
   anyErrors( nc_put_att_text(ncid, crop_id, "MemoryOrder", 3, "XY ") );
   tmp_str = string ( "percent of crops in grids" );
   anyErrors( nc_put_att_text(ncid, crop_id, "Description", tmp_str.size(), tmp_str.c_str() ) );
   anyErrors( nc_put_att_text(ncid, crop_id, "Units", 7, "Percent") );
   anyErrors( nc_put_att_text(ncid, crop_id, "stagger", 1, "M") );
   anyErrors( nc_put_att_int(ncid, crop_id, "sr_x", NC_INT, 1, sr_xy) );
   anyErrors( nc_put_att_int(ncid, crop_id, "sr_y", NC_INT, 1, sr_xy) );


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

   /********************
   * Leave define mode *
   ********************/
   anyErrors( nc_enddef (ncid) );
        
   /*********************************
   * Write NLCD variables to netCDF file *
   *********************************/
   printf( "\nWriting variable data in output netcdf file...\n" );
  
   //store lu_name
   long ty_start[3];
   long ty_count[3];

   ty_start[0]=0;
   ty_start[1]=0;
   ty_start[2]=0;

   ty_count[0]=time_len;
   ty_count[1]=land_cat_len;
   ty_count[2]=nameStr_len;

   anyErrors( ncvarput(ncid, luName_id, ty_start, ty_count, luStr) );
   printf( "\tWrited luStr\n\n" );
   free (luStr);

   if ( inUSGSLand.compare("YES") == 0 )
   {
      //      Store imperv
      anyErrors( nc_put_var_float(ncid, imperv_id, imperv) );
      printf( "\tWrited imperv\n" );
      free(imperv);

      //      Store canopy
      anyErrors( nc_put_var_float(ncid, canopy_id, canopy) );
      printf( "\tWrited canopy\n" );
      free(canopy);

      //store tree name
      ty_count[0]=time_len;
      ty_count[1]=tree_cat_len;
      ty_count[2]=nameStr_len;
      anyErrors( ncvarput(ncid, treeName_id, ty_start, ty_count, treeStr) );
      printf( "\tWrited treeStr\n\n" );
      free (treeStr);
   
      //store tree fractions
      anyErrors( nc_put_var_float(ncid, tree_id, tree) );
      printf( "\tWrited FIA tree fractions\n" );
      free (tree);
   }

   //landuse
   //      Store lu_list
   anyErrors( nc_put_var_int(ncid, lu_list_id, lu_list) );
   printf( "\tWrited lu_list\n" );
   free ( lu_list );

   //   Store crop_name
   ty_count[0]=time_len;
   ty_count[1]=crop_cat_len;
   ty_count[2]=nameStr_len;
   anyErrors( ncvarput(ncid, cropName_id, ty_start, ty_count, cropStr) );
   printf( "\tWrited cropStr\n\n" );
   free ( cropStr );

   //   Store luf
   anyErrors( nc_put_var_float(ncid, luf_id, luf) );
   printf( "\tWrited luf\n" );
   free(luf);

   //   Store cropf
   anyErrors( nc_put_var_float(ncid, crop_id, cropf) );
   printf( "\tWrited cropf\n" );
   free (cropf);

   //   Store lum
   anyErrors( nc_put_var_float(ncid, lum_id, lum) );
   printf( "\tWrited lum\n" );
   free(lum);

   //      Store lud
   anyErrors( nc_put_var_float(ncid, lud_id, lud) );
   printf( "\tWrited lud\n" );
   free(lud);


   /**************************
   * Store global attributes *
   **************************/
   string dateDate = string ( "0000-00-00_00:00:00" );
   string title = string ( "Landuse Data Generated from computeGridLandUse.exe" );
   writeWRFGlobalAtributes ( ncid, title, grid, dateDate );

   /********************

   * Close netCDF file *
   ********************/
   anyErrors( nc_close(ncid) );

   printf( "Finished writing output NetCDF file: %s\n\n", outNetcdfFile.c_str() );

   printf ("\n\nCompleted in computing and outputing all landuse data.\n");
}


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
 

/* -------------------------------------------------------- */
/*   Compute county image indexes for the overalyed domain  */
/* -------------------------------------------------------- */
         //do not need to check overaly as US NLCD will always overlay

         int row1_cnty = 0, row2_cnty = 0, col1_cnty=0, col2_cnty=0;
         int nXSize_cnty = 0 ,nYSize_cnty = 0;     //overlaypping array size for domain gird image

         //Row and column start from UL corner
         //calculate intersection UL columns
         computeIntersectionBox_WithCnty(oXmin_grd, oYmin_grd, oXmax_grd, oYmax_grd, cntyGrid, &col1_cnty,&row1_cnty,&col2_cnty,&row2_cnty);

         //compute x and y cells and min and max for county grid image for QA

         printf ("\tCounty grid image intesection: (col=%d row=%d)  (col=%d row=%d)\n",col1_cnty,row1_cnty,col2_cnty,row2_cnty);
         nXSize_cnty = col2_cnty - col1_cnty + 1;
         nYSize_cnty = row2_cnty - row1_cnty + 1;
         printf ("\t\tnXSize = %d  nYSize = %d\n",nXSize_cnty, nYSize_cnty);

         double oXmin_cnty = cntyGrid.xmin + (col1_cnty - 1) * cntyGrid.xCellSize;
         double oXmax_cnty = cntyGrid.xmin + col2_cnty * cntyGrid.xCellSize;
         double oYmax_cnty = cntyGrid.ymax - (row1_cnty - 1) * cntyGrid.yCellSize;
         double oYmin_cnty = cntyGrid.ymax - row2_cnty * cntyGrid.yCellSize;
         printf("\t\tOverlapping box: x: %lf - %lf    y: %lf - %lf\n",oXmin_cnty,oXmax_cnty,oYmin_cnty,oYmax_cnty);


/* ----------------------------------------------------------------- */
/*       read one row at time in the intersecting box from image     */
/* ----------------------------------------------------------------- */
         poBand = poRDataset->GetRasterBand( 1 );  // band 1

         //image row: too big to read once 
         GByte *poImage = (GByte *) CPLCalloc(sizeof(GByte),nXSize);


/* ------------------------------------------------------------- */
/*         get county line memory  in the intersecting box       */
/* ------------------------------------------------------------- */
         GUInt32 *poImage_cnty = NULL;

         if ( fileType.find(canoType) !=string::npos && nXSize_cnty > 0 && nXSize_cnty <= cntyGrid.cols &&
               nYSize_cnty > 0 && nYSize_cnty <= cntyGrid.rows )
         {
            poImage_cnty = (GUInt32 *) CPLCalloc(sizeof(GUInt32),nXSize_cnty);
         }


/* ----------------------------------------------------------------- */
/*      Read domain image to get satellite image and county data     */
/* ----------------------------------------------------------------- */
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
                      double gridArea =  percent * xCellSize_grd * yCellSize_grd / 100.00;  //compute area within a 30m grid
                     gridIMPE[gridID-1] += gridArea;  //compute imperviousness area 
                  }
               }  // end of k col 
            } //imperviousness 

 
            if ( fileType.find(canoType) !=string::npos )
            {    
               
               //county data
               //get  y for the cell
               double yCnty =  oYmax_grd - j * yCellSize_grd - yCellSize_grd / 2.0;  //center of the cell

               //get row in extracted county image
               int  rowCnty = (int) ( floor (( oYmax_cnty - yCnty ) / cntyGrid.yCellSize ) );   //start from 0

               int newCNTYLine = false;
               if ( poImage_cnty != NULL && rowCnty >= 0 && rowCnty < nYSize_cnty )
               {
                  if ( (poCntyBand->RasterIO(GF_Read, col1_cnty-1, row1_cnty-1+rowCnty, nXSize_cnty, 1,
                        poImage_cnty, nXSize_cnty,1, GDT_UInt32, 0, 0)) == CE_Failure)
                  {
                     printf( "\tError: reading county from image i area: %s.\n", fileName.c_str() );
                     CPLFree (poImage_cnty);
                     exit( 1 );
                  }
                  newCNTYLine = true;
               }


               for (k=0; k<nXSize; k++)
               {
                  gridID = poImage_grd[k];
                  percent = poImage[k];

                  if ( gridID > 0 && gridID <= gridPixels && percent != NoDataValue)
                  {
                     double gridArea =  percent * xCellSize_grd * yCellSize_grd / 100.00;  //compute area within a 30m grid
                     gridCANO[gridID-1] += gridArea;  //compute canopy area

                     //extracted county and overalyed domain grid
                     if ( newCNTYLine == false || gridArea == 0.0 )
                     {
                          continue;
                     }

                     //get x for the cell
                     double xCnty =  oXmin_grd + k * xCellSize_grd + xCellSize_grd / 2.0;  //center of the cell

                     //get col in extracted county image
                     int  colCnty = (int) ( floor (( xCnty - oXmin_cnty ) / cntyGrid.xCellSize ) );   //start from 0

                     if ( colCnty >= 0 && colCnty < nXSize_cnty )
                     {

                        int cntyFIPS = poImage_cnty[colCnty];
                        //printf ("\tj=%d   k=%d   cntyFIPS=%d\n", j, k, cntyFIPS);

                        if ( cntyFIPS > 0 )
                        {
                            //county FIPS and GRIDID

                            //printf ("\tObtaining canopy county information...\n" );

                            char   tmp_char[30];
                            sprintf( tmp_char,"%d.%d\0",gridID,cntyFIPS);
                            string tmp_str = string ( tmp_char );
                            if ( gridCntyCANO.find ( tmp_str )  != gridCntyCANO.end() )
                            {
                               //grid and county element is already existing
                               gridCntyCANO[tmp_str] += gridArea;
                            }
                            else
                            {
                               //grid and county element is new
                               gridCntyCANO[tmp_str] = gridArea;

                               //add county element to the grid
                               std::vector<int>  vecFIPS;
                               if ( gridCNTYs.find ( gridID ) != gridCNTYs.end() )
                               {
                                  //existing grid element and get county fips vector
                                  vecFIPS = gridCNTYs[gridID];
                               }

                               vecFIPS.push_back ( cntyFIPS );
                               gridCNTYs[gridID] = vecFIPS;

                                //printf ( "\tgridID=%d   FIPS=%d\n", gridID, cntyFIPS );

                            }  //new county to the grid
                        }  //county fips exists
                     } //grid and county intersect

                  }  //valid canopy grid

               }  // end of k col
            }  //canopy

        }  //j


        CPLFree (poImage);
        CPLFree (poImage_grd);
        if ( poImage_cnty != NULL ) CPLFree (poImage_cnty);

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
 
          
/* -------------------------------------------------------- */
/*   Compute county image indexes for the overalyed domain  */
/* -------------------------------------------------------- */
         //do not need to check overaly as US NLCD will always overlay

         int row1_cnty = 0, row2_cnty = 0, col1_cnty=0, col2_cnty=0;
         int nXSize_cnty = 0 ,nYSize_cnty = 0;     //overlaypping array size for domain gird image

         //Row and column start from UL corner
         //calculate intersection UL columns
         computeIntersectionBox_WithCnty(oXmin_grd, oYmin_grd, oXmax_grd, oYmax_grd, cntyGrid, &col1_cnty,&row1_cnty,&col2_cnty,&row2_cnty);

         //compute x and y cells and min and max for county grid image for QA

         printf ("\tCounty grid image intesection: (col=%d row=%d)  (col=%d row=%d)\n",col1_cnty,row1_cnty,col2_cnty,row2_cnty);
         nXSize_cnty = col2_cnty - col1_cnty + 1;
         nYSize_cnty = row2_cnty - row1_cnty + 1;
         printf ("\t\tnXSize = %d  nYSize = %d\n",nXSize_cnty, nYSize_cnty);

         double oXmin_cnty = cntyGrid.xmin + (col1_cnty - 1) * cntyGrid.xCellSize;
         double oXmax_cnty = cntyGrid.xmin + col2_cnty * cntyGrid.xCellSize;
         double oYmax_cnty = cntyGrid.ymax - (row1_cnty - 1) * cntyGrid.yCellSize;
         double oYmin_cnty = cntyGrid.ymax - row2_cnty * cntyGrid.yCellSize;
         printf("\t\tOverlapping box: x: %lf - %lf    y: %lf - %lf\n",oXmin_cnty,oXmax_cnty,oYmin_cnty,oYmax_cnty);


/* ----------------------------------------------------------------- */
/*       read one row at time in the intersecting box from image     */
/* ----------------------------------------------------------------- */
         poBand = poRDataset->GetRasterBand( 1 );  // band 1

         //image row: too big to read once
         GByte *poImage = (GByte *) CPLCalloc(sizeof(GByte),nXSize);


/* ------------------------------------------------------- */
/*         get county mage in the intersecting box         */
/* ------------------------------------------------------- */
         GUInt32 *poImage_cnty = NULL;

         //allocate memory for county

         if ( nXSize_cnty > 0 && nXSize_cnty <= cntyGrid.cols && nYSize_cnty > 0 && nYSize_cnty <= cntyGrid.rows )
         {
            poImage_cnty = (GUInt32 *) CPLCalloc(sizeof(GUInt32),nXSize_cnty);
         }


/* ----------------------------------------------------------------- */
/*      Read domain image to get satellite image and county data     */
/* ----------------------------------------------------------------- */

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

            //county data
            //get  y for the cell
            double yCnty =  oYmax_grd - j * yCellSize_grd - yCellSize_grd / 2.0;  //center of the cell

            //get row in extracted county image
            int  rowCnty = (int) ( floor (( oYmax_cnty - yCnty ) / cntyGrid.yCellSize ) );   //start from 0

            int newCNTYLine = false;
            if ( poImage_cnty != NULL && rowCnty >= 0 && rowCnty < nYSize_cnty )
            {
               if ( (poCntyBand->RasterIO(GF_Read, col1_cnty-1, row1_cnty-1+rowCnty, nXSize_cnty, 1,
                     poImage_cnty, nXSize_cnty,1, GDT_UInt32, 0, 0)) == CE_Failure)
               {
                  printf( "\tError: reading county from image %d area: %s.\n", i, fileName.c_str() );
                  CPLFree (poImage_cnty);
                  exit( 1 );
               }
               newCNTYLine = true;
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

                  //record 81 and 82 crops for USGS NLCD: grid and county

                  //check valid county indexes
                  if ( newCNTYLine == false )
                  {
                      continue;
                  }

                  //get x for the cell
                  double xCnty =  oXmin_grd + k * xCellSize_grd + xCellSize_grd / 2.0;  //center of the cell

                  //get col in extracted county image
                  int  colCnty = (int) ( floor (( xCnty - oXmin_cnty ) / cntyGrid.xCellSize ) );   //start from 0

                  //extracted county and domain grid overaly
                  if ( (colCnty >= 0 && colCnty < nXSize_cnty) && ( classID == 81 || classID == 82 ) )
                  {
                      int cntyFIPS = poImage_cnty[colCnty];
                      if ( cntyFIPS > 0 )
                      {
                          //county FIPS and GRIDID
                          char   tmp_char[30];

                          sprintf( tmp_char,"%d.%d.%d\0",gridID,cntyFIPS, classID);
                          string tmp_str = string ( tmp_char );

                          if ( gridCntyCrop.find ( tmp_str )  != gridCntyCrop.end() )
                          {
                             //grid, county, crop element is already existing
                             gridCntyCrop[tmp_str] += 1;   //add one cell
                          }
                          else
                          {
                             //grid, county, crop element is new
                             gridCntyCrop[tmp_str] = 1;

                             //add county element to the grid
                             std::vector<int>  vecFIPS;
                             if ( classID == 81 )
                             {
                                if ( grid81CNTYs.find ( gridID ) != grid81CNTYs.end() )
                                {
                                   //existing grid element and get county fips vector
                                   vecFIPS = grid81CNTYs[gridID];
                                }
                                vecFIPS.push_back ( cntyFIPS );
                                grid81CNTYs[gridID] = vecFIPS;
                             }
                             else
                             {
                                if ( grid82CNTYs.find ( gridID ) != grid82CNTYs.end() )
                                {
                                   //existing grid element and get county fips vector
                                   vecFIPS = grid82CNTYs[gridID];
                                }
                                vecFIPS.push_back ( cntyFIPS );
                                grid82CNTYs[gridID] = vecFIPS;
                             }

                             //printf ( "\tgridID=%d   FIPS=%d\n", gridID, cntyFIPS );

                          }  //new county for grid and crop 81 and 82
                      }  //county fips exists

                  } //grid and county intersect

               }  //valid data
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
        if ( poImage_cnty != NULL ) CPLFree (poImage_cnty);

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
   printf( "\tImage size is %dx%dx%d\n", xCells, yCells, poRDataset->GetRasterCount() );

   //get UL corner coordinates and grid size
   if( poRDataset->GetGeoTransform( adfGeoTransform ) == CE_None )
   {
      xUL = adfGeoTransform[0];
      yUL = adfGeoTransform[3];
      printf( "\tImage UL Origin = (%.6lf,%.6lf)\n", xUL,yUL );

      xCellSize =  adfGeoTransform[1] ;
      yCellSize =  -1 * adfGeoTransform[5] ;  // Make it positive.  Negative due to Origin is UL corner point
      printf( "\tImage Pixel Size = (%.3lf,%.3lf)\n", xCellSize, yCellSize );
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
        printf( "\tError: Projection is not defined in the MODIS file: %s.\n", modisFile.c_str() );
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
          

/* --------------------------------------------------------------- */
/*     Allocate memoey for one row of CAN census division image    */
/* --------------------------------------------------------------- */
      GUInt32 *poImage_cnty = (GUInt32 *) CPLCalloc(sizeof(GUInt32),canCDGrid.cols );


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

            //get row in CAN census division image
            int  rowCnty = (int) ( floor (( canCDGrid.ymax - y ) / canCDGrid.yCellSize ) );   //start from 0

            int newCNTYLine = false;
            if ( rowCnty >= 0 && rowCnty < canCDGrid.rows )
            {
               if ( (poCntyBand->RasterIO(GF_Read, 0, rowCnty, canCDGrid.cols, 1,
                     poImage_cnty, canCDGrid.cols, 1, GDT_UInt32, 0, 0)) == CE_Failure)
               {
                   printf( "\tError: reading CAN census division image for row: %i.\n", i );
                   CPLFree (poImage_cnty);
                   exit( 1 );
               }
               newCNTYLine = true;
            }

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

                        //check valid county indexes
                        if ( newCNTYLine == false )
                        {
                           continue;
                        }


                        //get col in extracted county image
                        int  colCnty = (int) ( floor (( x - canCDGrid.xmin ) / canCDGrid.xCellSize ) ); //start from 0

                        //extracted county and domain grid overaly
                        if ( (colCnty >= 0 && colCnty < canCDGrid.cols) && ( classID == 12 || classID == 14 ) )
                        {
                           int cntyFIPS = poImage_cnty[colCnty];
                           if ( cntyFIPS > 0 )
                           {
                              //county FIPS and GRIDID
                              char   tmp_char[30];

                              sprintf( tmp_char,"%d.%d.%d\0",gridID,cntyFIPS,classID);
                              string tmp_str = string ( tmp_char );
                              if ( gridCntyCrop.find ( tmp_str )  != gridCntyCrop.end() )
                              {
                                 //grid, county, crop element is already existing
                                 gridCntyCrop[tmp_str] += 1;   //add one cell
                              }
                              else
                              {
                                 //grid, county, crop element is new
                                 gridCntyCrop[tmp_str] = 1;

                                 //add county element to the grid
                                 std::vector<int>  vecFIPS;
                                 if ( classID == 12 )
                                 {
                                    if ( grid12CNTYs.find ( gridID ) != grid12CNTYs.end() )
                                    {
                                       //existing grid element and get county fips vector
                                       vecFIPS = grid12CNTYs[gridID];
                                    }
                                    vecFIPS.push_back ( cntyFIPS );
                                    grid12CNTYs[gridID] = vecFIPS;
                                 }
                                 else
                                 {
                                    if ( grid14CNTYs.find ( gridID ) != grid14CNTYs.end() )
                                    {
                                       //existing grid element and get county fips vector
                                       vecFIPS = grid14CNTYs[gridID];
                                    }
                                    vecFIPS.push_back ( cntyFIPS );
                                    grid14CNTYs[gridID] = vecFIPS;
                                 }
                                 //printf ( "\tgridID=%d   FIPS=%d\n", gridID, cntyFIPS );

                              }  //new county for grid and crop 12 and 14
                           }  //county fips exists

                        } //grid and county intersect

                     }  //MODIS image is not NODATA
                  }  // check col
               }  //check gridID
            }  // j
         }  // check row
      }  //i
  
      printf ( "\tCounts = %.0lf\n", testd);
      printf ( "\tFinished counting modeling domain grid ID numbers.\n" );

      CPLFree (poImage);
      CPLFree (poImage_grd);
      if ( poImage_cnty != NULL ) CPLFree (poImage_cnty);
   }
   else     
   {        
        printf ("\t\tMODIS image does not intersect with domain grid image.\n");
   }  

   GDALClose( (GDALDatasetH) poRDataset );

   printf ("Finished reading MODIS image.\n\n");
}  //end of the function



/************************************************************************/
/*    computeIntersectionBoxIndexes_WithCnty(...)                                */
/************************************************************************/
void computeIntersectionBox_WithCnty (double xmin, double ymin, double xmax, double ymax, gridInfo cntyGrid,
                                      int *col1_cnty, int *row1_cnty, int *col2_cnty, int *row2_cnty)
{
      
         //compute county image indexes of intersection box from the domain
      
         //calculate intersection LL columns
         if ( xmin > cntyGrid.xmin )
         {
            *col1_cnty =  (int) ( (xmin - cntyGrid.xmin) / cntyGrid.xCellSize + 1);
         }
         if ( xmin <= cntyGrid.xmin )
         {
            *col1_cnty = 1;
         }

         //calculate intersection LR columns
         if ( xmin > cntyGrid.xmin )
         {
            *col1_cnty =  (int) ( (xmin - cntyGrid.xmin) / cntyGrid.xCellSize + 1);
         }
         if ( xmin <= cntyGrid.xmin )
         {
            *col1_cnty = 1;
         }

         //calculate intersection LR columns
         if ( xmax < cntyGrid.xmax )
         {
            *col2_cnty = (int) ( (xmax - cntyGrid.xmin) / cntyGrid.xCellSize );
         }
         if ( xmax >= cntyGrid.xmax )
         {
            *col2_cnty = cntyGrid.cols;
         }

         //calculate intersection LR rows
         if ( ymin > cntyGrid.ymin )
         {
            *row2_cnty = (int) ( (cntyGrid.ymax - ymin) / cntyGrid.yCellSize );
         }
         if ( ymin <= cntyGrid.ymin )
         {
            *row2_cnty = cntyGrid.rows;
         }

         //calculate intersection UL row
         if ( ymax < cntyGrid.ymax )
         {
            *row1_cnty = (int) ( (cntyGrid.ymax - ymax) / cntyGrid.yCellSize + 1);
         }
         if ( ymax >= cntyGrid.ymax )
         {
            *row1_cnty = 1;
         }

}


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


/* ----------------------------------------------- */
/*     Read in FIA tree fraction data at county    */
/* ----------------------------------------------- */

void    readFIAFile ( string fiaFile )
{   
    std::ifstream         imageStream;
    std::vector<float>    vecFloat;
    
    string     fipsID, fiaID, fiaValue;
    int        fipsID_n, fiaID_n;
    float      fiaValue_n;
    
    int        i, j, k;
    string     lineStr;
    
    // read in FIA tree fractions at county level
    printf ("Reading FIA tree fraction file: %s...\n", fiaFile.c_str());
    
    i = 0;
    imageStream.open( fiaFile.c_str() );
    if (imageStream.good() )
    {  
       getline( imageStream, lineStr );
       while ( !imageStream.eof() )
       {  
          i++;  //count the line number
          
          //get rid of empty line and first two line headers
          if ( lineStr.size() == 0 )
          {  
             goto newloop2;
          }
          
          //printf( "\tline %d: %s\n",i, lineStr.c_str() );
          
          fipsID = lineStr.substr (0,5);
          fipsID = trim ( fipsID );
          fipsID_n = atoi ( fipsID.c_str() );
          
          fiaID = lineStr.substr (5,8);
          fiaID = trim ( fiaID );
          fiaID_n = atoi ( fiaID.c_str() );
          
          k = fiaID_n - 39;   //index in hash float vector BELD3 tree starts from 39 to 232
          
          fiaValue  = lineStr.substr (13,lineStr.size() - 13);
          fiaValue = trim ( fiaValue );
          fiaValue_n = atof ( fiaValue.c_str() );
          
          //printf( "\tFIPS=%s   fiaID=%s    Value=%s\n", fipsID.c_str(), fiaID.c_str(), fiaValue.c_str() );
          //printf( "\tFIPS=%s   fiaID=%d    Value=%f\n", fipsID.c_str(), fiaID_n, fiaValue_n );
          
          //county's data are stored in the a county hash table
          if ( cntyTrees.find ( fipsID_n )  == cntyTrees.end() )
          {
             //new county key and generate vector float with 194 items with 0.0 value
             //printf ("\t\tNew FIPS\n");
             for (j=0; j<FIA_TREE_TOTAL; j++)
             {
                vecFloat.push_back ( 0.0 );
             }
          }
          else
          {
             //printf ("\t\tOld FIPS\n");
             //exist county key and get the float vector
             vecFloat = cntyTrees[fipsID_n];
          }

           vecFloat[k] = fiaValue_n;
           cntyTrees[fipsID_n] = vecFloat;

           vecFloat.clear();


          newloop2:
          getline( imageStream, lineStr);
       }  // while loop

       imageStream.close();
    } // if good
    else
    {
       printf ("\tError: Open file - %s\n", fiaFile.c_str());
       exit ( 1 );
    }

    printf( "\tTotal lines read: %d    Counties in the file: %d\n\n",i, cntyTrees.size() );
}



/* ----------------------------------------------- */
/*     Read in NASS crop fraction data at county   */
/* ----------------------------------------------- */
void  readNASSFile ( string nassFile )
{
    std::ifstream         imageStream;
    std::vector<string>   vecString;
    std::vector<float>    vecFloat;

    string  lineStr, tmp_str;
    int     fipsID_n;
    int     cropID;
    int     i,j,k;
    // read in NASS crop fractions at county level
    printf ("Reading NASS crop fraction file: %s...\n", nassFile.c_str());

    i = 0;
    imageStream.open( nassFile.c_str() );
    if (imageStream.good() )
    {
       getline( imageStream, lineStr );
       while ( !imageStream.eof() )
       {
          lineStr = trim ( lineStr );  //get rid of spaces at edges
          i++;  //count the line number

          //get rid of empty line and first two line headers
          if ( lineStr.size() == 0 || i < 3 )
          {
             goto newloop3;
          }

          //printf( "line %d: %s\n",i, lineStr.c_str() );

          //put data in string vector
          vecString = string2stringVector (lineStr, ",");

          //get county FIPS and NLCD crop codes
          fipsID_n = atoi ( vecString[0].c_str() );
          cropID = atoi ( vecString[1].c_str() );
          //printf ("\n\t: FIPS: %d   Crop: %d\n", fipsID_n, cropID );

          vecString.erase (vecString.begin(), vecString.begin()+2 );  //erase first 2 items

          //put county and 81 and 82 in different hash tables
          if ( ( i % 2 ) == 1 )
          {
             if (vecString.size() != PASTURE_CLASSES )
             {
                 printf( "\tError: line %d in %s should have 8 items\n", i, nassFile.c_str() );
                 exit ( 1 );
             }
             for ( j=0; j<vecString.size(); j++ )
             {
                vecFloat.push_back (atof (vecString[j].c_str() ) );
                //printf ( "\t\titem: %d  %s\n", j, vecString[j].c_str() );
             }
             cntyCrops81[fipsID_n] = vecFloat;
          }
          else
          {
             //82 even lines
             for ( j=0; j<vecString.size(); j++ )
             {
                vecFloat.push_back (atof (vecString[j].c_str() ) );
                //printf ( "\t\titem: %d  %s\n", j, vecString[j].c_str() );
             }

             if (vecString.size() <= CROP_CLASSES )   //NASS crops has 36 crops, for 2001 has 32 and fill 0 
             {
                int   fill0Crops = CROP_CLASSES - vecString.size();
                for ( j=0; j<fill0Crops; j++ )
                {
                   vecFloat.push_back ( 0.0 );
                }
             }
             else
             {
                 printf( "\tError: line %d in %s should have items <= 38\n", i, nassFile.c_str() );
                 exit ( 1 );
             }
             cntyCrops82[fipsID_n] = vecFloat;
          }

          vecFloat.clear();

          newloop3:
          getline( imageStream, lineStr);
       }  // while loop

       imageStream.close();
    } // if good
    else
    {
       printf ("\tError: Open file - %s\n", nassFile.c_str());
       exit ( 1 );
    }

     printf( "\tTotal lines read: %d    Counties for 81 crops: %d    Counties for 82 crops: %d\n\n",i, cntyCrops81.size(), cntyCrops82.size() );

}



/* -------------------------------------------------------- */
/*     Read in CAN crop fraction data at census divisions   */
/* -------------------------------------------------------- */
void  readCANCropFile ( string canCropFile )
{
    std::ifstream         imageStream;
    std::vector<string>   vecString;
    std::vector<float>    vecFloat;

    string  lineStr, tmp_str;
    int     fipsID_n;
    int     i,j,k;

    // read in CAN crop fractions at census division level
    printf ("Reading CAN crop fraction file: %s...\n", canCropFile.c_str());

    i = 0;
    imageStream.open( canCropFile.c_str() );
    if (imageStream.good() )
    {
       getline( imageStream, lineStr );
       while ( !imageStream.eof() )
       {
          lineStr = trim ( lineStr );  //get rid of spaces at edges
          i++;  //count the line number

          //get rid of empty line and first line header
          if ( lineStr.size() == 0 || i < 2 )
          {
             goto newloop3;
          }

          //printf( "line %d: %s\n",i, lineStr.c_str() );

          //put data in string vector
          vecString = string2stringVector (lineStr, ",");

          //get census division six digiti code
          fipsID_n = atoi ( vecString[0].c_str() );
          //printf ("\n\t: FIPS: %d\n", fipsID_n );

          vecString.erase (vecString.begin(), vecString.begin()+2 );  //erase first 2 items

          //put census dividion crop in a hash table
          if (vecString.size() != CROP_TOTAL )
          {
              printf( "\tError: line %d in %s should have 44 items\n", i, canCropFile.c_str() );
              exit ( 1 );
          }

          for ( j=0; j<vecString.size(); j++ )
          {
             vecFloat.push_back (atof (vecString[j].c_str() ) );
             //printf ( "\t\titem: %d  %s\n", j, vecString[j].c_str() );
          }
          canCDCrops[fipsID_n] = vecFloat;

          vecFloat.clear();

          newloop3:
          getline( imageStream, lineStr);
       }  // while loop

       imageStream.close();
    } // if good
    else
    {
       printf ("\tError: Open file - %s\n", canCropFile.c_str());
       exit ( 1 );
    }

     printf( "\tTotal lines read: %d    Census divisions for CAN crops: %d\n\n",i, canCDCrops.size() );

}


/* ----------------------------------------------- */
/*     Read in landcover and tree class name list  */
/* ----------------------------------------------- */
void   readClassFile ( string classFile )
{
    std::ifstream         imageStream;
    string    lineStr;
    int       nameType;   //1 is land cover name and 2 is tree name
    int       i,j;

    // read in land cover and tree class names
    printf ("Reading land cover, crop and tree class name file: %s...\n", classFile.c_str());

    i = 0;
    imageStream.open( classFile.c_str() );
    if (imageStream.good() )
    {
       getline( imageStream, lineStr );
       while ( !imageStream.eof() )
       {
          lineStr = trim ( lineStr );  //get rid of spaces at edges
          i++;  //count the line number

          //get rid of empty line
          if ( lineStr.size() == 0 )
          {
             goto newloop4;
          }

          //get the type of files
          if ( lineStr.find( "MODIS and NLCD Land Cover Classes") !=string::npos )
          {
             nameType = 1;
             printf( "\nline %d: %s\n",i,lineStr.c_str() );
             goto newloop4;
          }

          if ( lineStr.find( "Pasture and Crop Classes") !=string::npos  )
          {
             nameType = 2;
             printf( "\nline %d: %s\n",i,lineStr.c_str() );
             goto newloop4;
          }

          if ( lineStr.find( "Tree Classes") !=string::npos  )
          {
             nameType = 3;
             printf( "\nline %d: %s\n",i,lineStr.c_str() );
             goto newloop4;
          }

          //put name in different vectors based on name type
          //printf( "line %d: %s\n",i, lineStr.c_str() );

          if ( lineStr.size() > nameStrLength )    nameStrLength = lineStr.size();

          if ( nameType == 1 )
          {
             lcNames.push_back(lineStr);
          }
          else if (  nameType == 2 )
          {
             cropNames.push_back(lineStr);
          }
          else if (  nameType == 3 )
          {
             treeNames.push_back(lineStr);
          }

          newloop4:
          getline( imageStream, lineStr);
       }  // while loop

       imageStream.close();
    } // if good
    else
    {
       printf ("\tError: Open file - %s\n", classFile.c_str());
       exit ( 1 );
    }

    FIA_TREE_TOTAL = treeNames.size();
    CROP_TOTAL = cropNames.size();

    printf( "\tTotal lines read: %d    Landcover classes: %d   Crop classes: %d    Tree classes: %d   MaxNameLength: %d\n\n",
             i, lcNames.size(), cropNames.size(), treeNames.size(), nameStrLength );



}
