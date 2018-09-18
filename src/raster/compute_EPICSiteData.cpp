/***************************************************************************
 * This program is used: 
 *  1. to compute EPIC site table using the following input files:
 *		1). Grid description
 *		2). BELD4 NetCDF file - contains CROPF variable - 40 classes
 *		3). US county shapefile with attributes: STATE(string), COUNTY(string), FIPS(string), 
 *			COUNTRY(string), STATEABB(string), REG10(REG10)
 *		4). North American political state shapefile with attributes: COUNTRY(string), STATEABB(string)
 *		5). US 8-digit HUC boundary shapefile with attribute: HUC_8(string)
 *		6). DEM elevation data: meters and missing value=-9999
 *		7). DEM slope data: 0 to 90 degree with scalar 0.01 and missing value -9999
 *  2. to solve sptial unmatching BND issues among BND files
 *  3. to output compute site information into a comma-separated table.
 *
 * Written by the Institute for the Environment at UNC, Chapel Hill
 * in support of the EPA NOAA CMAS Modeling, 2011-12
 * 
 * By Limei Ran, Aug.- Dec 2012 - Mar 2013
 *		 Modified: April 2013
 *
 * Usage:  compute_EPICSiteData.exe
 *  
 * Environment Variables needed: 
 *         GRID_ROWS  -- grid domain rows
 *         GRID_COLUMNS -- grid domain columns
 *         GRID_XMIN -- grid domain xmin
 *         GRID_YMIN -- grid domain ymin
 *         GRID_XCELLSIZE -- grid domain x grid size 
 *         GRID_YCELLSIZE -- grid domain y grid size 
 *         GRID_PROJ -- grid domain proj4 projection definition
 *         GRID_NAME -- grid name for IOAPI output
 *
 *         DOMAIN_BELD4_NETCDF -- preprocessed NLCD file list
 *         COUNTY_SHAPEFILE -- preprocessed county FIPS code shapefile
 *         COUNTRY_SHAPEFILE -- preprocessed country with state code shapefile
 *         HUC8_SHAPEFILE -- processed 8-digit HUC shapefile
 *         ELEVATION_IMAGE --  processes elevation image
 *         SLOPE_IMAGE -- process slope image
 *         MINIMUM_CROP_ACRES -- minimum crop acres
 *
 *         OUTPUT_TEXT_FILE -- EPIC site information in CSV format
 *	   OUTPUT_TEXT_FILE2 -- EPIC site crop fraction in csv format
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


//define functions
void   spatailIssues_Solver  ( gridInfo grid,  std::vector<int> &epicSiteIDs, std::map<int, vector<string> > &gridEPICData );


//set variables for BELD4 crop table
const char  *luFVarName = "LANDUSEF";   //landuse fraction array variable in BELD4 netCDf file
char        *lcCatDimName = "land_cat"; //land cover category dimension

int         luNum;                      //number of land use classes

const char  *cropFVarName = "CROPF";   //crop fraction array variable in BELD4 netCDf file
int          cropNum   =  42;          //number of crops in CROPF array

//NLCD classes 40 classes
int          nlcd81_index = 36;    //NLCD: 81 Pasture/Hay 
int          nlcd82_index = 37;    //NLCD: 82 Cultivated Crops 
int          nlcd11_index = 20;     //NLCD: 11  Open Watr

//NLCD classes 50 classes
int          nlcd81_index50 = 18;    //NLCD: 81 Pasture/Hay
int          nlcd82_index50 = 19;    //NLCD: 82 Cultivated Crops
int          nlcd11_index50 = 0;     //NLCD: 11  Open Water


//MODIS classes 40 classes
int          modis12_index = 11;   //MODIS: 12 croplands
int          modis14_index = 13;   //MODIS: 14 cropland / natural vegetation mosaic  
int          modis17_index = 16;   //MODIS: 17 all water including NLCD water 11

//MODIS classes 50 classes
int          modis12_index50 = 42;   //MODIS: 12 croplands
int          modis14_index50 = 44;   //MODIS: 14 cropland / natural vegetation mosaic
int          modis0_index50  = 30;   //MODIS: 0 water
int          modis17_index50 = 47;   //MODIS: 17 IGBP water
int          modis255_index50 = 49;  //MODIS: 255 fill value (normally ocean water)


//ELevation and slope image missing values
string       imageMissingValue_str =  string ("-9999");    //in Elevation and Slope image from HYDRO1k, water areas are set to -9999
string       outImageValue_str = string ("-99999");        //if points are outside mage extent 

float        cropArea_threshold;                           //acres - selection minimum area for each crop 



/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main( int nArgc,  char* papszArgv[] )
{
    gridInfo              grid;                 //store a modeling domain information
    string                gdalBinDir;           //GDAL directory

    string                outTextFile, outTextFile2, outTextFile3;          //output file names
    std::ofstream         outTxtStream, outTxtStream3;         //output text file stream 

    
    std::map<int, vector<string> >   gridEPICData; 
    std::vector<int>      epicSiteIDs, allSiteIDs;

    std::vector<string>   vecItems; //contains: GRASS, CROP, TOTAL, WATER, REG10, STATE, COUNTY, 
			            //COUNTRY, STATEABB, HUC8, ELEVATION,SLOPE
    int                   gridID;
 
    int                   i,j,k;                    
    char                  temp_char[100];
    string                temp_str;
     


    //print program version
    printf ("\nUsing: %s\n", prog_version);


    /***************************
    * Map projection variables *
    ***************************/
    char     *pszProj4;
    int      proj;     //WRF projection type
    string   temp_proj;

    /***********************************
    * NetCDF variable array pointers   *
    ************************************/

  
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

    printf( "\tRows=%d    Cols=%d    Total gird cells=%d\n",grid.rows,grid.cols,grid.rows*grid.cols);
    if (grid.cols <1 || grid.rows <1)
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
    printf( "\tGrid domain projection proj4 = %s\n",pszProj4);

    //created Shapefile will use GRIDID item name
    grid.polyID = string ( "GRIDID" );   //created domain Shapefile uses GRIDID

    //get projection parameters from Proj4 string
    temp_proj = string (pszProj4);
    proj = getProjType(temp_proj);
    printf( "\tproj type = %d \n",proj);


    //get minimum crop acres limit
    cropArea_threshold = (float) atof( getEnviVariable("MINIMUM_CROP_ACRES") ); 


    /*********************
    *   Get BELD4 FILE   *
    **********************/
    string   beld4FileNC;

    //printf( "Domain BELD4 data file\n");
    beld4FileNC = string( getEnviVariable("DOMAIN_BELD4_NETCDF") );
    beld4FileNC = trim ( beld4FileNC);
    printf("\n\tDomain BELD4 netCDF file:  %s\n",beld4FileNC.c_str());
    FileExists(beld4FileNC.c_str(), 0 );  //the file has to exist.


    /***************************
    *   Get county shapefile   *
    ****************************/
    string   countySHP;
    std::vector<string>   cntySHPItems;

    //printf( "Getting U.S. county shapefile.\n");
    countySHP = string( getEnviVariable("COUNTY_SHAPEFILE") );
    countySHP = trim ( countySHP );
    printf("\n\tU.S. county shapefile:  %s\n",countySHP.c_str());
    FileExists(countySHP.c_str(), 0 );  //the file has to exist.

    //set shapefile attribute items - county shapefile has to contain the following six attribute names
    
    string regItem = string ( "REG10" );     //string item
    cntySHPItems.push_back ( regItem );

    string stfipsItem = string ( "STATE" );     //string item
    cntySHPItems.push_back ( stfipsItem );

    string cntyfipsItem = string ( "COUNTY" );  //string item
    cntySHPItems.push_back ( cntyfipsItem );

    string cntryItem = string ( "COUNTRY" );    //string item
    cntySHPItems.push_back ( cntryItem );

    string cntrystItem = string ( "STATEABB" ); //string item
    cntySHPItems.push_back ( cntrystItem );


    /****************************
    *   Get country shapefile   *
    *****************************/
    string   countrySHP;
    std::vector<string>   cntrySHPItems;

    //printf( "Getting country shapefile.\n");
    countrySHP = string( getEnviVariable("COUNTRY_SHAPEFILE") );
    countrySHP = trim ( countrySHP );
    printf("\n\tRegion country shapefile:  %s\n",countrySHP.c_str());
    FileExists(countrySHP.c_str(), 0 );  //the file has to exist.
 
    //set shapefile attribute items - has to contain COUNTRY and STATEABB attributes as those in county shapefile
    cntrySHPItems.push_back ( cntryItem );

    cntrySHPItems.push_back ( cntrystItem );
  

    /*********************************
    *   Get 8-digit HUC shapefile    *
    **********************************/
    string   huc8SHP;
    std::vector<string>   huc8SHPItems;

    //printf( "Getting 8-digit HUC shapefile.\n");
    huc8SHP = string( getEnviVariable("HUC8_SHAPEFILE") );
    huc8SHP = trim ( huc8SHP );
    printf("\n\t8-digit HUC shapefile:  %s\n",huc8SHP.c_str());
    FileExists(huc8SHP.c_str(), 0 );  //the file has to exist.

    //set shapefile attribute items - has to contain HUC_8 attribute

    string huc8sItem = string ( "HUC_8" );     //string item
    huc8SHPItems.push_back ( huc8sItem );


    /***************************************
    *   Get region elevation raster fle    *
    ****************************************/
    string   elevImage;

    //printf( "Getting region elevation image file");
    elevImage = string( getEnviVariable("ELEVATION_IMAGE") );
    elevImage = trim ( elevImage );
    printf("\n\tregion elevation image:  %s\n",elevImage.c_str());
    FileExists(elevImage.c_str(), 0 );  //the file has to exist.

 

    /***************************************
    *   Get region slope raster fle    *
    ****************************************/
    string   slopeImage;

    //printf( "Getting region slope image file");
    slopeImage = string( getEnviVariable("SLOPE_IMAGE") );
    slopeImage = trim ( slopeImage );
    printf("\n\tregion slope image:  %s\n",slopeImage.c_str());
    FileExists(slopeImage.c_str(), 0 );  //the file has to exist.


    /*****************************
    *   get output  variables    *
    ******************************/
    //printf( "Getting output text file names.\n");
    outTextFile = string( getEnviVariable("OUTPUT_TEXT_FILE") );
    printf( "\n\tOutput text file name: %s\n",outTextFile.c_str() );
    FileExists(outTextFile.c_str(), 3 );  //the file has to be new.

    outTextFile2 = string( getEnviVariable("OUTPUT_TEXT_FILE2") );
    printf( "\n\tOutput text file name: %s\n",outTextFile2.c_str() );
    FileExists(outTextFile2.c_str(), 3 );  //the file has to be new.

    outTextFile3 = string( getEnviVariable("OUTPUT_TEXT_FILE3") );
    printf( "\n\tOutput text file name: %s\n",outTextFile3.c_str() );
    FileExists(outTextFile3.c_str(), 3 );  //the file has to be new.


    /**********************************
    * Compute domain grid coordinates *
    ***********************************/
    computeGridCoordinates (&grid);

    
    /******************************/
    /*  read BELD4 file info      */
    /******************************/
    float     *luV, *cropV;

    printf ("Reading grid landuse data to select EPIC sites...\n");

    //get land cover percent array
    luV = readWRFNCVarDataFloat ( grid, luFVarName, beld4FileNC );

    //get crop percent array
    cropV = readWRFNCVarDataFloat ( grid, cropFVarName, beld4FileNC );


    //compute minimum total crop P for potential site selection

    float totalCropP_threshold;

    if ( temp_proj.find("latlong") !=string::npos )
    {
       //it is latlong projection
       totalCropP_threshold = ( cropArea_threshold * 4046.85642 / ( 111.0 * 1000.0 * 111.0 * 1000.0 ) ) / (grid.xCellSize * grid.yCellSize) * 100.0;
    }
    else
    {
       //projection in meter systems
       totalCropP_threshold = cropArea_threshold * 4046.85642 / (grid.xCellSize * grid.yCellSize) * 100.0;
    }

    printf ("\n\tMinimum crop percent used for EPIC site selection: %.6f\n", totalCropP_threshold);

    //find the size of the land use array: 50 or 40

    luNum  = getNetCDFDim (lcCatDimName, beld4FileNC);

    
    //get pasture and crop percent 
    for (i=0; i<grid.rows;i++)
    {
       for (j=0; j<grid.cols;j++)
       {
          gridID = i * grid.cols + j + 1;         
        
          float pastureP=0;
          float cropP=0;
          float waterP=0;

          if ( luNum == 40 )
          {
             //get NLCD 81 and 82 index 
             int index_81 = nlcd81_index * grid.rows * grid.cols + i *  grid.cols + j;
             int index_82 = nlcd82_index * grid.rows * grid.cols + i *  grid.cols + j;
             int index_11 = nlcd11_index * grid.rows * grid.cols + i *  grid.cols + j;


             //get MODIS IGBP 12, 14, 17 index 
             int index_12 = modis12_index * grid.rows * grid.cols + i *  grid.cols + j;
             int index_14 = modis14_index * grid.rows * grid.cols + i *  grid.cols + j;
             int index_17 = modis17_index * grid.rows * grid.cols + i *  grid.cols + j;


             //pasture percent
             //pastureP = luV[index_81];

             //crop percent
             //cropP = luV[index_82] + luV[index_12] + luV[index_14] * 0.67;  //take 2/3 of MODIS 14 class

             //water perent
             waterP = luV[index_11] + luV[index_17];  
          }
          else if ( luNum == 50 )
          {
             //get NLCD 81 and 82 index
             int index_81 = nlcd81_index50 * grid.rows * grid.cols + i *  grid.cols + j;
             int index_82 = nlcd82_index50 * grid.rows * grid.cols + i *  grid.cols + j;
             int index_11 = nlcd11_index50 * grid.rows * grid.cols + i *  grid.cols + j;


             //get MODIS IGBP 12, 14, 0, 17, 255 index
             int index_12 = modis12_index50 * grid.rows * grid.cols + i *  grid.cols + j;
             int index_14 = modis14_index50 * grid.rows * grid.cols + i *  grid.cols + j;
             int index_0 = modis0_index50 * grid.rows * grid.cols + i *  grid.cols + j;
             int index_17 = modis17_index50 * grid.rows * grid.cols + i *  grid.cols + j;
             int index_255 = modis255_index50 * grid.rows * grid.cols + i *  grid.cols + j;

             //pasture percent
             //pastureP = luV[index_81];

             //crop percent
             //cropP = luV[index_82] + luV[index_12] + luV[index_14] * 0.67;  //take 2/3 of MODIS 14 class

             //water perent
             waterP = luV[index_11] + luV[index_0] + luV[index_17] + luV[index_255];
          }
          else
          {
             printf ("\tError: number of land use classes has to be 40 or 50 in input netCDF file.\n");
             exit ( 1);
          } 


          //added doe to the cells along US and CAN border

          int cropIndex;
          for (k=0; k<6; k++ )
          {
             cropIndex = k * grid.rows * grid.cols + i * grid.cols + j;

             pastureP += cropV[cropIndex];

          }      

         for (k=6; k<cropNum; k++ )
          {  
             cropIndex = k * grid.rows * grid.cols + i * grid.cols + j;
             
             cropP += cropV[cropIndex];
          }      


       
          //potential site with crop or pasture percent and waterP < half of grid area
          //if ( ( pastureP + cropP ) >= totalCropP_threshold && waterP < 50.0 )
          //{
             sprintf( temp_char,"%.4f\0",pastureP); 
             temp_str = string ( temp_char );
             vecItems.push_back ( temp_str );

             sprintf( temp_char,"%.4f\0",cropP); 
             temp_str = string ( temp_char );      
             vecItems.push_back ( temp_str );

             sprintf( temp_char,"%.4f\0",pastureP+cropP);
             temp_str = string ( temp_char );
             vecItems.push_back ( temp_str );
          
             sprintf( temp_char,"%.4f\0",waterP);
             temp_str = string ( temp_char );
             vecItems.push_back ( temp_str );

             gridEPICData[gridID] = vecItems;        
             epicSiteIDs.push_back ( gridID );            
             allSiteIDs.push_back ( gridID );

             vecItems.clear();
          //}

       }  //j col
    }  //i row
   
        
    free ( luV );

    printf ("\tSelected %d EPIC potential sites. Final sites will be selectled based on each crop fraction > %.6f\n", epicSiteIDs.size() , totalCropP_threshold );

    if (  epicSiteIDs.size() == 0 )
    {
       printf ("No potenital EPIC sites are selected.\n");
       exit ( 1 );
    }


    /**************************************/
    /*  Create EPIC site point shapefile  */
    /**************************************/
    //printf ("Creating EPIC site point shapefile...\n"); 
    //string  epicSiteShapeFile = createEPICSiteShapeFile ( grid, epicSiteIDs );

    
    /*********************************************************/
    /*  intersect EPIC site shapefile with county shapefile  */
    /*********************************************************/
    //string  siteCNTYSHP = identity2SHPFiles ( epicSiteShapeFile, countySHP );
    //delete created Shapefiles
    //deleteShapeFile ( siteCNTYSHP );
    //GDAL OGR will have this in 1.10 version

    getPointsInPolyItems ( grid, epicSiteIDs, gridEPICData, countySHP, cntySHPItems );


    /*********************************************************/
    /*  intersect EPIC site shapefile with region shapefile  */
    /*********************************************************/
    //string  siteCNTRYSHP = identity2SHPFiles ( epicSiteShapeFile, countrySHP );
    //delete created Shapefiles
    //deleteShapeFile ( siteCNTRYSHP );
    //GDAL OGR will have this in 1.10 version

    getPointsInPolyItems ( grid, epicSiteIDs, gridEPICData, countrySHP, cntrySHPItems );
   
    /*******************************************************/
    /*  intersect EPIC site shapefile with HUC8 shapefile  */
    /*******************************************************/
    //string  siteHUC8SHP = identity2SHPFiles ( epicSiteShapeFile, huc8SHP );
    //delete created Shapefiles
    //deleteShapeFile ( siteHUC8SHP );
    //GDAL OGR will have this in 1.10 version

    getPointsInPolyItems ( grid, epicSiteIDs, gridEPICData, huc8SHP, huc8SHPItems ); 


    /***********************************/
    /*  compute EPIC site elevation    */
    /***********************************/
    getPointsInRasterValues ( grid, epicSiteIDs, gridEPICData, elevImage, outImageValue_str );

    
    /***********************************/
    /*  compute EPIC site slope        */
    /***********************************/
    getPointsInRasterValues ( grid, epicSiteIDs, gridEPICData, slopeImage, outImageValue_str );


    /*****************************************************/
    /*      Spatial boundary line offset handling        */
    /*****************************************************/
    //due to offsets among boundary lines in USA county shapefile, NA State shapefile, and 8-digit HUC shapefiles,
    //the following cases may occur:
    //	1. USA points have "0" 8-HUC number,
    //	2. EPIC sites have COUNTRY code "water/agua/d'eau" and NO STATEABB code,
    //	3. EPIC sites have correct COUNTRY codes and no STATEABB code,
    //	4. EPIC sites have no STATE, COUNTY, COUNTRY codes (as EPIC sites are computed based on NLCD data at 30m resolution) 
    // near site method will be used to handle the cases
 
    spatailIssues_Solver ( grid, epicSiteIDs, gridEPICData );


    
    /*****************************/
    /*  Write output files       */
    /*****************************/
    //set total items in vecItems
    int totalItems = 12;

   
    //
    //write EPIC crop table
    //Select site with each crop area > 40 acres
    //
    printf( "\nWrite EPIC site crop file: %s...\n",outTextFile2.c_str() );

    outTxtStream.open( outTextFile2.c_str() );
    if (! outTxtStream.good() )
    {
       printf( "\tError in opening output file: %s\n",outTextFile2.c_str() );
       exit ( 1 );
    }
  
    //write title
    string lineStr = string ("GRIDID,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,COUNTRY,HUC8\n");
    outTxtStream.write( lineStr.c_str(), lineStr.size() );

    lineStr.clear();

    for ( i=0; i<epicSiteIDs.size(); i++ )
    {
       //output gridID
       gridID = epicSiteIDs[i];

       //skip the site with attribute problems - can not solved spatial issues
       if ( gridID == 0 )
       {
          continue;
       }
       
       //get epic site info vector
       //vecItems contains 12 items: GRASS,CROPS,TOTAL,WATER,REG10,STFIPS,CNTYFIPS,COUNTRY,CNTY_PROV,HUC8,ELEVATION,SLOPE
       std::vector<string>   vecItems = gridEPICData[gridID];


       //only output USA EPIC sites for current modeling, get COUNTRY - 8 item, waterP - 4 item
       string country_str = vecItems[7];
       string waterP_str = vecItems[3];
       double  waterP = atof ( waterP_str.c_str() );


       if ( country_str.compare ( "USA" ) != 0 )
       {
          vecItems.clear();
          continue;
       }

       
       sprintf( temp_char,"%d,\0", gridID );
       temp_str = string ( temp_char );
       lineStr.append ( temp_str );

       //get row and col indices in the model domain from LL corner
       int row = (int) ( floor ( ( gridID - 1 ) / grid.cols) );   //start from 0
       int col = ( gridID - 1 ) % grid.cols;                      //start from 0


       // It may need to be changed depending on crop order

       bool selected = false;

       for (j=0; j<cropNum; j++ )
       {
          int index = j * grid.rows * grid.cols + row *  grid.cols + col;

          float percent = cropV[index];

          float crop_acre;


          if ( temp_proj.find("latlong") !=string::npos )
          {
             //it is latlong projection
             crop_acre = ( percent /100.0 * grid.xCellSize * grid.yCellSize ) * (111.0 * 1000.0) * (111.0 * 1000.0) * 0.000247105381;
          }
          else
          {
             //it is meter projection
             crop_acre   = percent /100.0 * grid.xCellSize * grid.yCellSize * 0.000247105381; 
          }

          sprintf(temp_char, "%6.4f,\0", crop_acre );   

          
          temp_str = string ( temp_char );

          lineStr.append ( temp_str ); 

          //for final site selection based on each crop area
          if ( crop_acre > cropArea_threshold && waterP < 50.0)
          {
             selected = true;
          }
       }

       //output COUNTRY -- item 8 
       lineStr.append ( country_str );
       lineStr.append( "," );

       //output HUC8
       int indexVal = totalItems - 3;
       lineStr.append ( vecItems[indexVal] );

       lineStr.append( "\n" );


       if ( ! selected )
       {
          printf ("\tEliminate site %d: no crop with area >= %.0f acres\n", gridID, cropArea_threshold );
          epicSiteIDs.at(i) = 0;   //set to skip the site
       }
       else
       {
          outTxtStream.write( lineStr.c_str(), lineStr.size() );
       }

       vecItems.clear();
       lineStr.clear();
       
    }

    free ( cropV );

    //close text file 
    outTxtStream.close (); 


    //
    //write EPIC site information table
    //
    printf( "\nWrite EPIC site info file: %s...\n",outTextFile.c_str() );

    outTxtStream.open( outTextFile.c_str() );
    if (! outTxtStream.good() )
    {
       printf( "\tError in opening output file: %s\n",outTextFile.c_str() );
       exit ( 1 );
    }
  
    //
    //write all site information table
    //
    printf( "\nWrite All site info file: %s...\n",outTextFile3.c_str() );

    outTxtStream3.open( outTextFile3.c_str() );
    if (! outTxtStream3.good() )
    {
       printf( "\tError in opening output file: %s\n",outTextFile3.c_str() );
       exit ( 1 );
    }

    //write header
    lineStr = string ("GRIDID,XLONG,YLAT,ELEVATION,SLOPE_P,HUC8,REG10,STFIPS,CNTYFIPS,GRASS,CROPS,TOTAL,COUNTRY,CNTY_PROV\n");
    outTxtStream.write( lineStr.c_str(), lineStr.size() );
    outTxtStream3.write( lineStr.c_str(), lineStr.size() );

    lineStr.clear();
    
    for ( i=0; i<allSiteIDs.size(); i++ )
    {

       //output GRIDID
       int gridIDepic = epicSiteIDs[i];
       gridID = allSiteIDs[i];

       //get epic info vector
       //vecItems contains 12 items: GRASS,CROPS,TOTAL,WATER,REG10,STFIPS,CNTYFIPS,COUNTRY,CNTY_PROV,HUC8,ELEVATION,SLOPE
       std::vector<string>   vecItems = gridEPICData[gridID];

       if ( vecItems.size() != totalItems )
       {
          printf( "\tError: number of obtained items is not %d for GRIDID: %d\n", totalItems, gridID );
          exit ( 1 );
       }

       //only output USA EPIC sites for current modeling, get COUNTRY - 8 item
       string cnty_str = vecItems[7];

    
       sprintf( temp_char,"%d,\0",gridID);
       temp_str = string ( temp_char );
       lineStr.append ( temp_str );


       //output XLONG and YLAT
       //get row and col indices in the model domain from LL corner
       int row = (int) ( floor ( ( gridID - 1 ) / grid.cols) );   //start from 0
       int col = ( gridID - 1 ) % grid.cols;                      //start from 0

       int indexVal = row * grid.cols + col;
       
       float xLong = grid.lon[ indexVal];
       float yLat =  grid.lat[indexVal]; 

       sprintf( temp_char,"%.6f,\0", xLong );
       temp_str = string ( temp_char );
       lineStr.append ( temp_str );

       sprintf( temp_char,"%.6f,\0", yLat );
       temp_str = string ( temp_char );
       lineStr.append ( temp_str );


       //output ELEVATION
       indexVal = totalItems - 2; 
       temp_str = vecItems[indexVal];

       if ( temp_str.compare( imageMissingValue_str ) == 0 )
       {
          temp_str = string ( "0" );   //missing value to be water surface, set water surface elevation = 0 meter
       }
       else if ( temp_str.compare( outImageValue_str ) == 0 )
       {
          temp_str = outImageValue_str;
       }
 
       lineStr.append ( temp_str );

       lineStr.append( ".0," );    //make elevation decimal point for siteCreate program to work


       //output SLOPE_P.  Obtained slope is in degree * 100 
       indexVal = totalItems - 1;
       temp_str = vecItems[indexVal];

       int slopeD = atoi ( temp_str.c_str() ); 
       
       //HYDRO1K slopeD = slope_degree * 100
       //percent rise is equal tan of the slope
       float slopeP;
       if ( temp_str.compare( imageMissingValue_str ) == 0 )
       {
           slopeP = 0.0;       //missing value to be water surface, set water surface slope rising percent =  0
       }
       else if ( temp_str.compare( outImageValue_str ) == 0 )
       {
           slopeP = atof ( outImageValue_str.c_str() );       
       }
       else
       {
           slopeP = 100.0 * tan ( ( slopeD/100.0 ) * PI/180.0 );  
       }

       sprintf( temp_char,"%.4f,\0", slopeP );
       temp_str = string ( temp_char );
       lineStr.append ( temp_str );


       //output HUC8
       indexVal = totalItems - 3;
       lineStr.append ( vecItems[indexVal] );
       lineStr.append( "," );

       //output REG10, STFIPS, CNTYFIPS -- 5 to 7 items
       for (j=4; j<=6; j++)
       {
          lineStr.append ( vecItems[j] );
          lineStr.append( "," );
       }

       //output GRASS, CROPS, and CROP_P -- 1 to 3 items
       for (j=0; j<=2; j++)
       {
          lineStr.append ( vecItems[j] );
          lineStr.append( "," );
       }
       
       //output COUNTRY, STATEABB -- 8 and 9 items       
       lineStr.append ( vecItems[7] );
       lineStr.append( "," );
       
       lineStr.append ( vecItems[8] );


       lineStr.append( "\n" );

       //skip the site with attribute problems - can not solved spatial issues
       if ( gridIDepic != 0 &&  cnty_str.compare ( "USA" ) == 0 )
       {
          outTxtStream.write( lineStr.c_str(), lineStr.size() );
       }


       //output all sites for SWAT connection
       outTxtStream3.write( lineStr.c_str(), lineStr.size() );

       vecItems.clear();
       lineStr.clear();

    }  //i - sites

    //close text file
    outTxtStream.close ();
    outTxtStream3.close ();


    printf ("\n\nCompleted in computing EPIC and all site information.\n");
}


/**********************************************/
/*    spatailIssues_Solver(...)               */
/**********************************************/
void   spatailIssues_Solver  ( gridInfo grid,  std::vector<int> &epicSiteIDs, std::map<int, vector<string> > &gridEPICData )
{

    int      i, j, indexVal;
    int      gridID;
    string   temp_str;


    //build neareast point trees
    int            nPts = epicSiteIDs.size();   // actual number of data points
    ANNpointArray  dataPts;                     // data points
    int            dim = 2;

    dataPts = annAllocPts(nPts, dim); // allocate data points


    for ( i=0; i<epicSiteIDs.size(); i++ )
    {
    
       //output GRIDID
       gridID = epicSiteIDs[i];
       
       //get XLONG and YLAT
       //get row and col indices in the model domain from LL corner
       int row = (int) ( floor ( ( gridID - 1 ) / grid.cols) );   //start from 0
       int col = ( gridID - 1 ) % grid.cols;                      //start from 0
       
       indexVal = row * grid.cols + col;

       float x = grid.x[col];
       float y =  grid.y[row];
       
       dataPts[i][0] = x;
       dataPts[i][1] = y;
    }


    /*************************************
    * Use ANN to get the nearest points  *
    **************************************/
    int           k = 8;       // number of nearest neighbors
    double        eps = 0.0;   // error bound
    ANNpoint      queryPt;     // query point
    ANNidxArray   nnIdx;       // near neighbor indices
    ANNdistArray  dists;       // near neighbor distances
    ANNkd_tree    *kdTree;     // search structure

    queryPt = annAllocPt(dim); // allocate query point
    nnIdx = new ANNidx[k];     // allocate near neigh indices
    dists = new ANNdist[k];    // allocate near neighbor dists


    printf ( "\tBuild geolocation point trees.\n" );
    kdTree = new ANNkd_tree( // build search structure
                           dataPts, // the data points
                           nPts, // number of points
                           dim); // dimension of space


    /***********************************/
    /*   handle no HUC8 code in USA    */
    /***********************************/
    printf ("\nFilling HUC8 numbers for sites in USA without 8-digit HUC numbers...\n");

 
    /*
     Need to commnet out for none USA CMAQ LCC projection application
     For US CA Los Angles county island points, to avoid assign it to mainland HUC8 digit
    */
    string temp_huc8_str = string ( "18070107" );


    int totalItems = 12;

    for ( i=0; i<epicSiteIDs.size(); i++ )
    {

       //output GRIDID
       gridID = epicSiteIDs[i];

       //skip the site with attribute problems - can not solved spatial issues
       if ( gridID == 0 )
       {
          continue;
       }

       //get epic site info vector
       std::vector<string>   vecItems = gridEPICData[gridID];

       //vecItems contains 12 items: GRASS,CROPS,TOTAL,WATER,REG10,STFIPS,CNTYFIPS,COUNTRY,CNTY_PROV,HUC8,ELEVATION,SLOPE
       if ( vecItems.size() != totalItems )
       {  
          printf( "\tError: number of obtained items is not %d for GRIDID: %d\n", totalItems, gridID );
          exit ( 1 );
       }

       
       //get HUC8 code in USA
       indexVal = totalItems - 3;
       string huc8_str = vecItems[indexVal]; 
       
       //get COUNTRY item 8
       indexVal = 8-1;
       string country_str =  vecItems[indexVal];

       //get COUNTY FIPS item 7
       indexVal = 7 -1;
       string county_str = vecItems[indexVal];
       

       bool  obtained=false;

       if ( country_str.compare ("USA") == 0 && huc8_str.compare ("0") == 0 )
       {
          
          //get X and Y
          //get row and col indices in the model domain from LL corner
          int row = (int) ( floor ( ( gridID - 1 ) / grid.cols) );   //start from 0
          int col = ( gridID - 1 ) % grid.cols;                      //start from 0

          int indexVal_grid = row * grid.cols + col;
       
          queryPt[0] = grid.x[col];
          queryPt[1] = grid.y[row];

          kdTree->annkSearch( // search
                              queryPt, // query point
                              k, // number of near neighbors
                              nnIdx, // nearest neighbors (returned)
                              dists, // distance (returned)
                              eps); // error bound
          // assign
          for ( j=0; j<k; j++ )
          {
             //new GRIDID 
             
             indexVal = nnIdx[j];

             int gridID_near = epicSiteIDs[indexVal];

             if ( gridID_near == 0 )
             {
                 continue;
             }
             

             //get new gridID data
             std::vector<string>   vecItems_near = gridEPICData[gridID_near];

             //get HUC8 code in USA
             indexVal = totalItems - 3;
             string huc8_str_near = vecItems_near[indexVal];             

             //get COUNTRY item 8
             indexVal = 8-1;
             string country_str_near =  vecItems_near[indexVal];

          /*   if ( country_str_near.compare ("USA") == 0 && huc8_str_near.compare ("0") != 0 ) */

             if ( grid.lon[indexVal_grid] > -118.637 && grid.lon[indexVal_grid] < -118.273 && grid.lat[indexVal_grid] > 33.26 && grid.lat[indexVal_grid] < 33.51 )
             {

                indexVal = totalItems - 3;
                vecItems.at(indexVal) = huc8_str_near;


                //handle cells on CA Los Angles county island
                /*if ( grid.lon[indexVal_grid] > -118.637 && grid.lon[indexVal_grid] < -118.273 && grid.lat[indexVal_grid] > 33.26 && grid.lat[indexVal_grid] < 33.51 ) */
                {
                   vecItems.at(indexVal) = temp_huc8_str;
                }
              
                
                //assign REG10,STFIPS,CNTYFIPS
                if ( county_str.compare ( "0" ) == 0 )
                {
                   for (indexVal=4; indexVal<=6; indexVal++)
                   {
                      vecItems.at(indexVal) = vecItems_near[indexVal];
                   }
                }

                
                gridEPICData[gridID] = vecItems;

                printf ("\tEPIC site %d obtained 8-digit HUC number from site %d\n", gridID, gridID_near );
                obtained = true;
                
                vecItems_near.clear();
                break;
             }

             vecItems_near.clear();
          }  //j: loop through obtained nearest 8 sites
         
          if ( ! obtained )
          {
             printf ("\tEPIC site %d did not obtain 8-digit HUC number\n", gridID );
             epicSiteIDs.at(i) = 0;    //set to skip the site
          }
          
          vecItems.clear();

       } //no USA and HUC8


    }  //i: loop through all sites



    /*****************************************************/
    /*   handle no REG10,STFIPS,CNTYFIPS codes in USA    */
    /*****************************************************/
    printf ("\nFilling REG10,STFIPS,CNTYFIPS for sites in USA without the codes...\n");


    for ( i=0; i<epicSiteIDs.size(); i++ )
    {

       //output GRIDID
       gridID = epicSiteIDs[i];

       //skip the site with attribute problems - can not solved spatial issues
       if ( gridID == 0 )
       {
          continue;
       }

       //get epic site info vector
       std::vector<string>   vecItems = gridEPICData[gridID];

       //
       //vecItems contains 12 items: GRASS,CROPS,TOTAL,WATER,REG10,STFIPS,CNTYFIPS,COUNTRY,CNTY_PROV,HUC8,ELEVATION,SLOPE
       //
       if ( vecItems.size() != totalItems )
       {  
          printf( "\tError: number of obtained items is not %d for GRIDID: %d\n", totalItems, gridID );
          exit ( 1 );
       }

       
       //get REG10 code in USA item - 5
       indexVal = 5 - 1;
       string reg10_str = vecItems[indexVal]; 
       
       //get STFIPS item 6
       indexVal = 6 - 1;
       string st_str =  vecItems[indexVal];

       //get CNTYFIPS item 7
       indexVal = 7 -1;
       string county_str = vecItems[indexVal];

       //get COUNTRY item 8
       indexVal = 8 - 1;
       string country_str =  vecItems[indexVal];

       bool  obtained=false;

       if ( country_str.compare ("USA") == 0 && ( reg10_str.compare( "0" ) == 0 || st_str.compare ("0") == 0 || county_str.compare ("0") == 0 ) )
       {
          
          //get X and Y
          //get row and col indices in the model domain from LL corner
          int row = (int) ( floor ( ( gridID - 1 ) / grid.cols) );   //start from 0
          int col = ( gridID - 1 ) % grid.cols;                      //start from 0

          indexVal = row * grid.cols + col;
       
          queryPt[0] = grid.x[col];
          queryPt[1] = grid.y[row];

          kdTree->annkSearch( // search
                              queryPt, // query point
                              k, // number of near neighbors
                              nnIdx, // nearest neighbors (returned)
                              dists, // distance (returned)
                              eps); // error bound
          // assign
          for ( j=0; j<k; j++ )
          {
             //new GRIDID 
             
             indexVal = nnIdx[j];

             int gridID_near = epicSiteIDs[indexVal];

             if ( gridID_near == 0 )
             {
                 continue;
             }

             
             //get new gridID data
             std::vector<string>   vecItems_near = gridEPICData[gridID_near];

             //get REG10 code in USA item - 5
             indexVal = 5 - 1;
             string reg10_str_near = vecItems_near[indexVal];

             //get STFIPS item 6
             indexVal = 6 - 1;
             string st_str_near =  vecItems_near[indexVal];

             //get CNTYFIPS item 7
             indexVal = 7 - 1;
             string county_str_near = vecItems_near[indexVal];

             //get COUNTRY item 8
             indexVal = 8 - 1;
             string country_str_near =  vecItems_near[indexVal];

             if ( country_str_near.compare ("USA") == 0 && reg10_str_near.compare ("0") != 0 && st_str_near.compare ("0") != 0 && county_str_near.compare ("0") != 0 )
             {

                //assign REG10,STFIPS,CNTYFIPS
                for (indexVal=4; indexVal<=6; indexVal++)
                {
                    vecItems.at(indexVal) = vecItems_near[indexVal];
                }

                
                gridEPICData[gridID] = vecItems;

                printf ("\tEPIC site %d obtained REG10, STFIPS, and CNTYFIPS from site %d\n", gridID, gridID_near );
                obtained = true;
                
                vecItems_near.clear();
                break;
             }

             vecItems_near.clear();
          }  //j: loop through obtained nearest 8 sites
         
          if ( ! obtained )
          {
             printf ("\tEPIC site %d did not obtain REG10, STFIPS, and CNTYFIPS\n", gridID );
             epicSiteIDs.at(i) = 0;    //set to skip the site
          }
          
          vecItems.clear();

       } //end of setting REG10, STFIPS, and CNTYFIPS 


    }  //i: loop through all sites



    /****************************************************************/
    /*   handle COUNTRY code = "water/agua/d'eau" and NO STATEABB   */
    /****************************************************************/
    printf ("\nFilling COUNTRY codes with water/agua/d'eau...\n");

    for ( i=0; i<epicSiteIDs.size(); i++ )
    {

       //output GRIDID
       gridID = epicSiteIDs[i];

       //skip the site with attribute problems - can not solved spatial issues
       if ( gridID == 0 )  
       {
          continue;
       }


       //get epic site info vector
       std::vector<string>   vecItems = gridEPICData[gridID];

       //get COUNTRY item 8
       indexVal = 8-1;
       string country_str =  vecItems[indexVal];

       bool  obtained = false;

       if ( country_str.compare ("water/agua/d'eau") == 0 )
       {
          //get XLONG and YLAT
          //get row and col indices in the model domain from LL corner
          int row = (int) ( floor ( ( gridID - 1 ) / grid.cols) );   //start from 0
          int col = ( gridID - 1 ) % grid.cols;                      //start from 0

          indexVal = row * grid.cols + col;
       
          queryPt[0] = grid.x[col];
          queryPt[1] = grid.y[row];

          kdTree->annkSearch( // search
                              queryPt, // query point
                              k, // number of near neighbors
                              nnIdx, // nearest neighbors (returned)
                              dists, // distance (returned)
                              eps); // error bound
          // assign
          for ( j=0; j<k; j++ )
          {
             //nearest GRIDID 
             indexVal = nnIdx[j];
             int gridID_near = epicSiteIDs[indexVal];

             if ( gridID_near == 0 )
             {
                 continue;
             }

             
             //get new gridID data
             std::vector<string>   vecItems_near = gridEPICData[gridID_near];
                      
             //get COUNTRY item 8
             indexVal = 8-1;
             string country_str_near =  vecItems_near[indexVal];

             if ( country_str_near.compare ("water/agua/d'eau") != 0 )
             {
                
                //assign REG10,STFIPS,CNTYFIPS
                for (indexVal=4; indexVal<=6; indexVal++)
                {
                   vecItems.at(indexVal) = vecItems_near[indexVal];
                }
               
                //COUNTRY, STATEABB
                for (indexVal=7; indexVal<=8; indexVal++)
                {
                   vecItems.at(indexVal) = vecItems_near[indexVal];
                }
                

                //handle sites on USA 06037 island county
                //18070107, 10,06,037,USA,USA-CA,


       
                gridEPICData[gridID] = vecItems;

                printf ("\tEPIC site %d obtained COUNTRY - %s and other codes from site %d\n", gridID, country_str_near.c_str(), gridID_near);
                obtained = true;
                
                vecItems_near.clear();
                break;
             }

             vecItems_near.clear();
          }  //j: loop through obtained nearest 8 sites
         
          if ( ! obtained )
          {
             printf ("\tEPIC site %d did not obtain COUNTRY code\n", gridID );
             epicSiteIDs.at(i) = 0;   //set to skip the site
             
          }
          
          vecItems.clear();

       } //no COUNTRY 


    }  //i: loop through all sites

    annDeallocPts ( dataPts );
    annDeallocPt (queryPt );
    delete [] nnIdx;   // clean things up
    delete [] dists;
    delete kdTree;
    annClose();       // done with ANN

}

     
