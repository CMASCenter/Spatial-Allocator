/**********************************************************************************
 * This program is used:
 *  1. to process EPIC modeling yearly average output files for CMAQ bi-directional 
 *     NH3 surface flux modeling
 *  2. to output one NetCDF format file
 *
 * Written by the Institute for the Environment at UNC, Chapel Hill
 * in support of US EPA ORD AMAD
 *
 * By LR, Nov. 2010
 * mod: Add total weighted by crop fraction, 03/2014
 *
 * Usage:  extractEPICYearlyAverage2CMAQ.exe
 *
 * Environment Variables needed:
 *         GRID_ROWS  -- grid domain rows
 *         GRID_COLUMNS -- grid domain columns
 *         GRID_XMIN -- grid domain xmin
 *         GRID_YMIN -- grid domain ymin
 *         GRID_XCELLSIZE -- grid domain x grid size
 *         GRID_YCELLSIZE -- grid domain y grid size
 *         GRID_PROJ -- grid domain proj4 projection definition
 *         DOMAIN_BELD4_NETCDF -- preprocessed NLCD file list
 *
 *         DATA_DIR -- directory contains EPIC output data to be extracted: FERTAPP5YEARAVE*.DAT
 *         OUTPUT_NETCDF_FILE - extracted EPIC output data file

***********************************************************************************/
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <set> 

#include "sa_raster.h"
#include "commontools.h"
#include "geotools.h"


void extractEpicData (gridInfo grid, float *dataV[], vector<string> epicFiles);

const char  *cropFVarName = "CROPF";   //crop fraction array variable in BELD4 netCDf file

/********************
 * Global variables *
 ********************/

string                   cropNamesStr = string ( "Hay,Hay_ir,Alfalfa,Alfalfa_ir,Other_Grass,Other_Grass_ir,Barley,Barley_iri,BeansEdible,BeansEdible_ir,CornGrain,CornGrain_ir,CornSilage,CornSilage_ir,Cotton,Cotton_ir,Oats,Oats_ir,Peanuts,Peanuts_ir,Potatoes,Potatoes_ir,Rice,Rice_ir,Rye,Rye_ir,SorghumGrain,SorghumGrain_ir,SorghumSilage,SorghumSilage_ir,Soybeans,Soybeans_ir,Wheat_Spring,Wheat_Spring_ir,Wheat_Winter,Wheat_Winter_ir,Other_Crop,Other_Crop_ir,Canola,Canola_ir,Beans,Beans_ir" );   //crop number from 22 to 63

int                      numCrops;            //number of crops in EPIC modeling
int                      numFileItems = 77;   //title has 131 items


string                   epicVarsStr = string ("GMN,NMN,NFIX,NITR,AVOL,  DN,YON,QNO3,SSFN,PRKN,  FNO,FNO3,FNH3,OCPD,TOC,  TNO3,DN2,YLDG,YLDF,YLN,  YLP,FTN,FTP,IRGA,WS,  NS,IPLD,IGMD,IHVD,YP,  QAP,YW,Q,SSF,PRK,  PRCP,PET,ET,QDRN,MUSL,  DRNN,DRNP,PRKP,FPO,FPL,  MNP");  //29+4+13 varaibles


static int               epicVarsPos[] = {20,21,22,23,24,  25,26,27,28,29,  30,31,32,41,43,  45,46,52,53,56,  57,58,59,60,67,  68,75,76,77,36,  37,19,14,15,16,  11,12,13,47,48,  49,50,38,33,34,  35};  //start from 1


int                      numEpicVars;


const double sigmaLevels[  ] = {1.0, 0.22, 0.23, 0.24,0.25,0.26,0.27,0.28,0.29,0.30,0.31,0.32,0.33,0.34,0.35,0.36,0.37,0.38,0.39,0.40,0.41,0.42,0.43,0.44,0.45,0.46,0.47,0.48,0.49,0.50,0.51,0.52,0.53,0.54,0.55,0.56,0.57,0.58,0.59,0.60,0.61,0.62,0.63 };
const double sigmaLevels_T[  ] = {1.0,0.63};   //for layer 1


const Name variableNames[ ] = { "GMN", "NMN", "NFIX", "NITR", "AVOL", 
                                "DN", "YON", "QNO3", "SSFN", "PRKN", 
                                "FNO", "FNO3", "FNH3", "OCPD", "TOC", 
                                "TNO3", "DN2", "YLDG", "T_YLDG", "YLDF", "T_YLDF", "YLN", 
                                "YLP", "FTN", "FTP", "IRGA", "WS", 
                                "NS", "IPLD", "IGMD", "IHVD", "YP",
                                "QAP", "YW", "Q", "SSF", "PRK",
                                "PRCP", "PET", "ET", "QDRN", "MUSL",
                                "DRNN", "DRNP", "PRKP", "FPO", "FPL",
                                "MNP" };


const Name variableUnits[ ] = { "kg/ha", "kg/ha", "kg/ha", "kg/ha", "kg/ha",
                                "kg/ha", "kg/ha", "kg/ha", "kg/ha", "kg/ha", 
                                "kg/ha", "kg/ha", "kg/ha", "kg/ha", "kg/ha", 
                                "kg/ha", "kg/ha", "ton/ha", "1000ton", "ton/ha", "1000ton", "kg/ha", 
                                "kg/ha", "kg/ha", "kg/ha", "mm", "days", 
                                "days", "Julian Date", "Julian Date", "Julian Date", "kg/ha",
                                "kg/ha", "ton/ha", "mm", "mm", "mm",
                                "mm", "mm", "mm", "mm", "ton/ha",
                                "kg/ha", "kg/ha", "kg/ha", "kg/ha", "kg/ha",
                                "kg/ha" };


const Line variableDescriptions[ ] = { "N Mineralized", "Humus Mineralization", "N Fixation", "Nitrification", "N Volitilization", 
                                       "Denitrification", "N Loss with Sediment", "N Loss in Surface Runoff", "N in Subsurface Flow", "N Loss in Percolate",
                                       "Organic N Fertilizer", "N Fertilizer Nitrate", "N Fertilizer Ammonia", "Organic Carbon in Plow Layer", "Organic Carbon in Soil Profile",
                                       "Total NO3 in Soil Profile", "Denitrification_N2", "Grain Yield", "T - Grain Yield", "Forage Yield", "T - Forage Yield", "N Used by Crop",
                                       "P Used by Crop", "N Applied", "P Applied", "Irrigation Volume Applied", "Water Stress Days", 
                                       "N Stress Days", "Planting Date", "Germination Date", "Harvest Date", "P Loss with Sediment", 
                                       "Labile P Loss in Runoff", "Wind Erosion", "Runoff", "Subsurface Flow", "Percolation",
                                       "Rainfall", "Potential ET", "Evapotranspiration", "Drain Tile Flow", "Water erosion (MUSL)",
                                       "Nitrogen in drain tile flow", "P in drain tile flow", "P in percolation", "Organic P fertilizer", "Organic P fertilizer",
                                       "P mineralized" };
                          

//output total
const Name TvariableNames[ ] = { "T_GMN", "T_NMN", "T_NFIX", "T_NITR", "T_AVOL",
                                "T_DN", "T_YON", "T_QNO3", "T_SSFN", "T_PRKN",
                                "T_FNO", "T_FNO3", "T_FNH3", "T_OCPD", "T_TOC",
                                "T_TNO3", "T_DN2", "T_YLN",
                                "T_YLP", "T_FTN", "T_FTP", "T_IRGA",
                                "T_YP", "T_QAP", "T_YW", "T_Q",
                                "T_SSF", "T_PRK",
                                "T_PRCP", "T_PET", "T_ET", "T_QDRN", "T_MUSL",
                                "T_DRNN", "T_DRNP", "T_PRKP", "T_FPO", "T_FPL",
                                "T_MNP" };

const Name TvariableUnits[ ] = { "mt", "mt", "mt", "mt", "mt",
                                 "mt", "mt", "mt", "mt", "mt",
                                 "mt", "mt", "mt", "mt", "mt",
                                 "mt", "mt", "mt",
                                 "mt", "mt", "mt", "mm",
                                 "mt", "mt", "1000ton", "mm",
                                 "mm", "mm",
                                 "mm", "mm", "mm", "mm", "1000ton",
                                 "mt", "mt", "mt", "mt", "mt",
                                 "mt" };

const Line TvariableDescriptions[ ] = { "T - N Mineralized", "T - Humus Mineralization", "T - N Fixation", "T - Nitrification", "T - N Volitilization",
                                        "T - Denitrification", "T - N Loss with Sediment", "T - N Loss in Surface Runoff", "T - N in Subsurface Flow", "T - N Loss in Percolate", 
                                        "T - Organic N Fertilizer", "T - N Fertilizer Nitrate", "T - N Fertilizer Ammonia", "T - Organic Carbon in Plow Layer", "T - Organic Carbon in Soil Profile",
                                        "T - Total NO3 in Soil Profile", "T - Denitrification_N2", "T - N Used by Crop",
                                        "T - Used by Crop", "T - N Applied", "T - P Applied", "Crop Weighted Irrigation Volume Applied",
                                        "T - P Loss with Sediment", "T - Labile P Loss in Runoff", "T - Wind Erosion", "T - Runoff",
                                        "T - Subsurface Flow", "T - Percolation",
                                        "T - Rainfall", "T - Potential ET", "T - Evapotranspiration", "T - Drain Tile Flow", "T - Water erosion (MUSL)",
                                       "T - Nitrogen in drain tile flow", "T - P in drain tile flow", "T - P in percolation", "T - Organic P fertilizer", "T - Organic P fertilizer",
                                       "T - P mineralized" };



/******************************************************
************************* MAIN  ***********************
*******************************************************/
int main(int nArgc, char *argv[])
{
        gridInfo           grid;            //data structure to store a modeling domain information
      

        //print program version
        printf ("\nUsing: %s\n", prog_version);

        printf("Process EPIC output files for CMAQ modeling...\n\n");

       
        /******************************
        * input variables             *
        *******************************/
        string    dataDir;                      //directory containing preprocessed Sat. data
        string    startDateTime, endDateTime;   //date and time range for the sat data extraction
        string    satVarName;
        string    outNetcdfFile, outNetcdfFile_T;
        string    tmp_str;
        string    satImageFile,geoImageFile;              
        string    dataDate;

        string    lineStr;
        int       i,j,k,n;

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
           printf( "\tUsage: extractEPIC2CMAQ.exe\n");
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


        //get WRF projection type from Proj4 string
        temp_proj = string (pszProj4);
        proj = getProjType(temp_proj);    //WRF has 4 projection types
        printf( "\tproj type = %d \n",proj);

        /***************************
        *       get Grid Name      *
        ***************************/
        grid.name = string( getEnviVariable("GRID_NAME") );
        printf("\tGrid name: %s\n",grid.name.c_str() );

        /*********************************
        *       get EPIC output dir      *
        **********************************/
        //printf( "Getting directory containing EPIC output files.\n");
        dataDir = string( getEnviVariable("DATA_DIR") );
        dataDir = processDirName(dataDir );  //make sure that dir ends with path separator
        printf("\tEPIC output data directory: %s\n",dataDir.c_str() );
        FileExists(dataDir.c_str(), 0 );  //the dir has to exist.


        
        /*********************
        *   Get BELD4 FILE   *
        **********************/
        string   beld4FileNC;

        //printf( "Domain BELD4 data file\n");
        beld4FileNC = string( getEnviVariable("DOMAIN_BELD4_NETCDF") );
        beld4FileNC = trim ( beld4FileNC);
        printf("\n\tDomain BELD4 netCDF file:  %s\n",beld4FileNC.c_str());
        FileExists(beld4FileNC.c_str(), 0 );  //the file has to exist.

       
        /***********************************
        *       get output file names      *
        ***********************************/
        //printf( "Getting output NetCDF file names\n");
        outNetcdfFile = string( getEnviVariable("OUTPUT_NETCDF_FILE") );
        outNetcdfFile = trim ( outNetcdfFile ); 
        printf( "\tOutput NetCDF file is: %s\n", outNetcdfFile.c_str() );
        FileExists(outNetcdfFile.c_str(), 3 );  //the file has to be new.


        int foundExt = outNetcdfFile.find_last_of(".");

        if ( foundExt != string::npos )
        {
           string namePart1 = outNetcdfFile.substr (0, foundExt);
           string namePart2 = outNetcdfFile.substr ( foundExt, outNetcdfFile.size() );
           //printf( "\tName1: %s  Name2: %s\n", namePart1.c_str(), namePart2.c_str() );
      
           //total output file name
           outNetcdfFile_T = string ( namePart1 + "_total" + namePart2 ); 
        }
        else
        {
           outNetcdfFile_T = string ( outNetcdfFile + "_total" );
        }

        outNetcdfFile_T = trim ( outNetcdfFile_T );
        printf( "\tOutput NetCDF file is: %s\n", outNetcdfFile_T.c_str() );
        FileExists(outNetcdfFile_T.c_str(), 3 );  //the file has to be new.


       /****************************************
       * Get all data files to be processed    *
       *****************************************/
       DIR             *dpFiles;          //data directory
       struct dirent   *dirpFiles;
       struct stat     stFileInfo;

       vector<string>  epicFiles;
       string          fileName; 
       int             gridID, cropID, soilID;
       
      
       printf("\nObtain files in %s to be processed...\n", dataDir.c_str() );
       
       if((dpFiles  = opendir( dataDir.c_str()) ) != NULL)
       {
          while ((dirpFiles = readdir(dpFiles)) != NULL)
          {
             tmp_str = string(dirpFiles->d_name);
             tmp_str = trim(tmp_str);

             fileName = dataDir + tmp_str;

             if ( tmp_str.find ( ".csv" ) != string::npos  )
             {
                printf( "\tFile obtained: %s\n", fileName.c_str() );
                epicFiles.push_back ( fileName );
             }  

          }  //while dirpFiles

          closedir(dpFiles);

       }  // if dpFiles
       else
       {
          printf( "Directory does not exist: %s\n",dataDir.c_str() );
       }

       printf ("\tObtained EPIC output yearly averaged files: %d\n",epicFiles.size());


       /**************************************
       * Define variables to read EPIC files *
       ***************************************/
       vector<string> cropNamesV = string2stringVector (cropNamesStr, ",");
       numCrops = cropNamesV.size();
       printf ("Number of crops: %d\n", numCrops);
          
       //get epic variable string vector
       vector<string> epicVarsV = string2stringVector (epicVarsStr, ",");

       numEpicVars = epicVarsV.size();
       printf ("Number of epic variables in output file: %d\n\n", numEpicVars);



       /***************************************
       * Read EPIC yearly average output file *
       ****************************************/
       int        totalSize;                      //total dim size
       float      *dataV[numEpicVars];

       totalSize = numCrops * grid.rows * grid.cols;
       printf ("\tTotalSize = %d\n", totalSize );

       for (i=0; i<numEpicVars; i++)
       {
          dataV[i] =  allocateFloatDataArrayMemory ( totalSize );

          //set missing values
          fillFloatArrayMissingValue ( totalSize, dataV[i] );
       }
      

       extractEpicData (grid, dataV, epicFiles);


       /******************************/
       /*  read BELD4 file info      */
       /******************************/
       float     *cropV, *dataP;
       int        totalVarT = 22+4+13;    //total chemical variables, add 4 swat vars, add 13 new vars for eco-analysis
       int        totalVarC = 2;     //total yield variables

       float     *TdataV[totalVarT];
       float     *CdataV[totalVarC];
       int        T_totalSize;

       printf ("Reading grid BELD4 crop fraction data...\n");

       //get crop percent array
       cropV = readWRFNCVarDataFloat ( grid, cropFVarName, beld4FileNC );

       
       //total value
       T_totalSize = grid.rows * grid.cols;

       for (i=0; i<totalVarT; i++)
       {
          //calloc - 0 initial
          TdataV[i] =  allocateFloatDataArrayMemory ( T_totalSize );

          //set missing values
          fillFloatArrayMissingValue ( T_totalSize, TdataV[i] );
       }


       for (i=0; i<totalVarC; i++)
       {
          //calloc - 0 initial
          CdataV[i] =  allocateFloatDataArrayMemory ( totalSize );

          //set missing values
          fillFloatArrayMissingValue ( totalSize, CdataV[i] );
       }

 
       float cellAreaHa; 

       if ( temp_proj.find("latlong") !=string::npos )
       {
          //it is latlong projection
          cellAreaHa =  (grid.xCellSize * 111.0 * 1000.0) * (grid.yCellSize * 111.0 * 1000.0) * 0.0001 ;
       }
       else
       {
          //projection in meter systems
          cellAreaHa = grid.xCellSize * grid.yCellSize * 0.0001;
       }
 

       int Tcount = 0;
       int Ccount = 0;
       for (i=0; i<totalVarT+totalVarC; i++)
       {
           if ( i < 24 ) 
           {
              dataP = dataV[i];
           }
           else
           {
              dataP = dataV[i+5];   // skip 5 day varaibles to be not in total         
           }

           for (j=0; j<grid.rows; j++)
           {
              for (k=0; k<grid.cols; k++)
              { 

                 float total_cropP = 0.0;   //for IRGA  or Q 
                 int index2D = j * grid.cols + k;

                 for (n=0; n<numCrops; n++)
                 {
                    int index3D = n * grid.rows * grid.cols  + j * grid.cols + k;
              
                    if ( dataP[index3D] >= 0.0 )
                    {

                       float cellValue;

                       if ( Tcount == 21 || ( Tcount >= 25 && Tcount <= 31) ) 
                       {
                          cellValue = dataP[index3D] * cropV[index3D];  //Tcount=21 for IRGA; Tcount=25 for Q to Tcount=31 for QDRN -- unit "mm"
                          total_cropP += cropV[index3D];
                       } 
                       else
                       {
                          cellValue = dataP[index3D] * cropV[index3D] * cellAreaHa / 100.0 / 1000.0; //kg to ton or ton to 1000ton - metric ton
                       }
                    
                       if (i == 17 || i == 18 )
                       {
                          if ( CdataV[Ccount][index3D] < 0.0 )
                          {
                             CdataV[Ccount][index3D] = 0.0;
                          }
                          CdataV[Ccount][index3D] += cellValue;
                       }
                       else
                       {
                          if ( TdataV[Tcount][index2D] < 0.0 )
                          {
                             TdataV[Tcount][index2D] = 0.0;
                          }
                          TdataV[Tcount][index2D] += cellValue;                
                       } 

                    } //have EPIC value

                 } //n crop

                 if ( (Tcount == 21 || ( Tcount >= 25 && Tcount <= 31) ) && total_cropP > 0.0 )
                 {
                    TdataV[Tcount][index2D] = TdataV[Tcount][index2D] / total_cropP; //weighted by total crop area for IRGA or Q
                 }

              } //k col 

           } //j row 


           
           if (i == 17 || i == 18 )
           {
              Ccount++;
           }
           else
           {
              Tcount++;
           }
           
       } //i
       

       free ( cropV );


       /*************************************
       * Set variables to write netCDF file *
       *************************************/
       //temp_proj = string (grid.strProj4);
       proj = getProjType(temp_proj);   //WRF projection type

       float    stdLon,stdLat;
       float    trueLat1, trueLat2;
       if (proj == LCC)
       {
          trueLat1 = getValueInProj4Str ( temp_proj, "lat_1=");
          trueLat2 = getValueInProj4Str ( temp_proj, "lat_2=");
          stdLat = getValueInProj4Str ( temp_proj, "lat_0=");
          stdLon = getValueInProj4Str ( temp_proj, "lon_0=");
       }
       else if ( proj == LATLONG)
       {
          trueLat1 = 0.0;
          trueLat2 = 0.0; 
          stdLat = 0.0;
          stdLon = 0.0;
       }
       else if ( proj == MERC )
       {
          //may have issues in IOAPI definition!!

          trueLat1 = 0.0;
          trueLat2 = getValueInProj4Str ( temp_proj, "lat_ts=");
          stdLat = 0.0;
          stdLon = getValueInProj4Str ( temp_proj, "lon_0=");
       }
       else if ( proj == UPS )
       {
          //may have issues in IOAPI definition!!

          stdLat = getValueInProj4Str ( temp_proj, "lat_ts=");
          stdLon = getValueInProj4Str ( temp_proj, "lon_0=");
          if ( stdLat > 0.0 )
          {
              trueLat1 = 1.0;
              trueLat2 = 90.0;
          }
          else
          {
              trueLat1 = -1.0;
              trueLat2 = -90.0;
          }
       }
       else
       { 
          printf ("Error: Projection type is not LCC.  Need to add new projection in the program.\n");
          exit ( 1 );
       }

       const int VARIABLES = numEpicVars+2;
       const int TIMESTEPS = 1;
       const int LAYERS = numCrops;
       const int ROWS = grid.rows;
       const int COLUMNS = grid.cols;
       const int YYYYDDD = 2002365;   //fixed
       const int startTime_att = 0;
       const int timeSteps_att = 0;
       const double xcellSize  = grid.xCellSize;
       const double ycellSize  = grid.yCellSize;
       const double westEdge  = grid.xmin;
       const double southEdge = grid.ymin;
       const double centralLongitude = stdLon;
       const double centralLatitude = stdLat;
       const double lowerLatitude = trueLat1;
       const double upperLatitude = trueLat2;
       const double topPressure = 63.0;   //faked data as layer is crop
     
       const Line description = "EPIC output yearly average data";


       //for total output file
       const int VARIABLES_T = totalVarT;
       const int LAYERS_T = 1;
       const Line Tdescription = "EPIC output yearly average data from all crops";
       
       /*******************
       * Write IOAPI file *
       ********************/

       int fileInfo, fileInfo_T;
       if (  proj == LCC )
       {
          fileInfo = createLambertConformalM3IOFile ( outNetcdfFile.c_str(),
                    VARIABLES, TIMESTEPS, LAYERS, ROWS, COLUMNS, YYYYDDD,
                    startTime_att, timeSteps_att,
                    xcellSize, ycellSize,
                    westEdge, southEdge,
                    centralLongitude, centralLatitude, centralLongitude,
                    lowerLatitude, upperLatitude,
                    topPressure, sigmaLevels,
                    variableNames, variableUnits, variableDescriptions,
                    grid.name.c_str(), description );

          fileInfo_T = createLambertConformalM3IOFile ( outNetcdfFile_T.c_str(),
                    VARIABLES_T, TIMESTEPS, LAYERS_T, ROWS, COLUMNS, YYYYDDD,
                    startTime_att, timeSteps_att,
                    xcellSize, ycellSize,
                    westEdge, southEdge,
                    centralLongitude, centralLatitude, centralLongitude,
                    lowerLatitude, upperLatitude,
                    topPressure, sigmaLevels_T,
                    TvariableNames, TvariableUnits, TvariableDescriptions,
                    grid.name.c_str(), Tdescription );

       }
       else if ( proj == LATLONG )
       {
          fileInfo = createLonLatM3IOFile ( outNetcdfFile.c_str(),
                    VARIABLES, TIMESTEPS, LAYERS, ROWS, COLUMNS, YYYYDDD,
                    startTime_att, timeSteps_att,
                    xcellSize, ycellSize,
                    westEdge, southEdge,
                    topPressure, sigmaLevels,
                    variableNames, variableUnits, variableDescriptions,
                    grid.name.c_str(), description );

          fileInfo_T = createLonLatM3IOFile ( outNetcdfFile_T.c_str(),
                    VARIABLES_T, TIMESTEPS, LAYERS_T, ROWS, COLUMNS, YYYYDDD,
                    startTime_att, timeSteps_att,
                    xcellSize, ycellSize,
                    westEdge, southEdge,
                    topPressure, sigmaLevels_T,
                    TvariableNames, TvariableUnits, TvariableDescriptions,
                    grid.name.c_str(), Tdescription );

       }
       else if ( proj == MERC )
       {
          fileInfo = createEquitorialMercatorM3IOFile ( outNetcdfFile.c_str(),
                    VARIABLES, TIMESTEPS, LAYERS, ROWS, COLUMNS, YYYYDDD,
                    startTime_att, timeSteps_att,
                    xcellSize, ycellSize,
                    westEdge, southEdge,
                    centralLongitude, centralLatitude, 
                    centralLongitude, upperLatitude,
                    topPressure, sigmaLevels,
                    variableNames, variableUnits, variableDescriptions,
                    grid.name.c_str(), description );
       
          fileInfo_T = createEquitorialMercatorM3IOFile ( outNetcdfFile_T.c_str(),
                    VARIABLES_T, TIMESTEPS, LAYERS_T, ROWS, COLUMNS, YYYYDDD,
                    startTime_att, timeSteps_att,
                    xcellSize, ycellSize,
                    westEdge, southEdge,
                    centralLongitude, centralLatitude,
                    centralLongitude, upperLatitude,
                    topPressure, sigmaLevels_T,
                    TvariableNames, TvariableUnits, TvariableDescriptions,
                    grid.name.c_str(), Tdescription );
       }
       else if ( proj == UPS )
       {
          fileInfo = createStereographicM3IOFile ( outNetcdfFile.c_str(),
                    VARIABLES, TIMESTEPS, LAYERS, ROWS, COLUMNS, YYYYDDD,
                    startTime_att, timeSteps_att,
                    xcellSize, ycellSize,
                    westEdge, southEdge,
                    centralLongitude, centralLatitude, 
                    centralLongitude, upperLatitude,
                    topPressure, sigmaLevels,
                    variableNames, variableUnits, variableDescriptions,
                    grid.name.c_str(), description );

          fileInfo_T = createStereographicM3IOFile ( outNetcdfFile_T.c_str(),
                    VARIABLES_T, TIMESTEPS, LAYERS_T, ROWS, COLUMNS, YYYYDDD,
                    startTime_att, timeSteps_att,
                    xcellSize, ycellSize,
                    westEdge, southEdge,
                    centralLongitude, centralLatitude,
                    centralLongitude, upperLatitude,
                    topPressure, sigmaLevels_T,
                    TvariableNames, TvariableUnits, TvariableDescriptions,
                    grid.name.c_str(), Tdescription );
       
       } 

       const int file = fileInfo;
       const int file_T = fileInfo_T;

       int ok = file != -1;
       int ok_T = file_T != -1;


       int varCount = 0;
       if ( file != -1 ) 
       {
  
          for (i=0; i<numEpicVars; i++)
          {
             ok = writeM3IOData( file, variableNames[varCount], dataV[i] );
             varCount ++;

             if ( i == 17)
             {
                ok = writeM3IOData( file, variableNames[varCount], CdataV[0] );
                varCount ++; 
             }
             else if ( i == 18)
             {
                ok = writeM3IOData( file, variableNames[varCount], CdataV[1] );
                varCount ++;
             }

          }    
       } 

       ok = closeM3IOFile( file ) && ok;

       printf( "varCount: %d\n\n", varCount);
       
       printf( "Finished writing epic fertilizer output file: %s\n\n", outNetcdfFile.c_str());



       if ( file_T != -1 )
       {

          for (i=0; i<totalVarT; i++)
          {
             ok = writeM3IOData( file_T, TvariableNames[i], TdataV[i] );
          }
       }

       ok_T = closeM3IOFile( file_T ) && ok_T;


       //free memory

       for (i=0; i<numEpicVars; i++)
       {
          free ( dataV[i] ); 
       }

       for (i=0; i<totalVarT; i++)
       {            
          free ( TdataV[i] ); 
       }                  
                          
                             
       for (i=0; i<totalVarC; i++)
       {                  
          free ( CdataV[i] ); 
       }                     


       printf( "Finished writing epic total output file : %s\n\n", outNetcdfFile_T.c_str());
          
}


/*********************End of Main program****************************************************************/



/****************************************************
 *      Extract EPIC yearly average output files    *
*****************************************************/
void extractEpicData (gridInfo grid, float *dataV[], vector<string> epicFiles)
{

   ifstream        imageStream;             //stream to read input EPIC site file
   string          lineStr;
   vector<string>  vecString;
   int             gridID, cropID; 
   int             row, col, cropIndex, index;
   int             i, j, k, m, n, lineNums;

   string          tmp_str;


   printf("\nReading EPIC yearly averaged data files...\n" );

   string fileType = string ( "EPICAVER" );
   for ( i=0; i<epicFiles.size(); i++ )
   {
      printf ("\tfile: %s\n", epicFiles[i].c_str() );

      m = epicFiles[i].rfind("/", epicFiles[i].length() );
      string cropID_str = epicFiles[i];
      cropID_str.erase (0, m+1 );

      m = cropID_str.find ( "." );
      cropID_str.erase (m, cropID_str.size()-m );

      int name_cropID = atoi ( cropID_str.c_str() );

      lineNums = 0;
      imageStream.open( epicFiles[i].c_str() );

      if (imageStream.good() )
      {
         getline( imageStream, lineStr, '\n');
         while ( !imageStream.eof() )
         {
            lineStr = trim (lineStr);                 //get rid of spaces at edges
            lineNums++;    //count the line number

            //get rid of empty line
            if ( lineStr.size() == 0 )
            {
               goto newloop;
            }

            if ( lineNums > 1 )   //first line is header
            {
               //put data in string vector
               vecString = string2stringVector (lineStr, ",");

               if ( vecString.size()  != numFileItems + 1)
               {
                  printf( "\tError: File = %s    items = %d   Standard Items = %d\n", epicFiles[i].c_str(), vecString.size(), numFileItems );
                  printf ( "\tlineNums = %d: %s\n", lineNums, lineStr.c_str() );

                  exit ( 1 );
               }

               gridID = getGridIDFromFileName ( vecString[0], fileType );
               cropID = getCropIDFromFileName ( vecString[0], fileType );
               cropID = cropID - 21; //BELD4 cropID starts from 1

               
               if ( cropID != name_cropID )
               {
                  printf( "\tError: Crop ID %d in %s does not match inside RunName crop ID %d:\n", name_cropID, epicFiles[i].c_str(), cropID);
                  exit ( 1 );
               }

               //get row and col: start from 1
               col= (gridID -1) % grid.cols + 1;
               row = (gridID - col) / grid.cols + 1;

               //get row and col: start from 0
               col = col - 1;
               row = row - 1;

               //get crop index
               cropIndex = cropID - 1;  //starts from 0


               //printf ("\tgridID=%d  cropID=%d  col=%d  row=%d  cropIndex=%d\n", gridID, cropID, col, row, cropIndex);

               index = cropIndex * grid.rows * grid.cols  + row * grid.cols + col;
               //printf ("\tindex=%d\n", index);

               for (j=0; j<numEpicVars; j++)
               {

                  int pos = epicVarsPos [j] - 1;  //EPIC Var Position count starts from 1
                  tmp_str = vecString [pos];

                  double epicValue = atof ( tmp_str.c_str() ) ;

                  dataV[j][index] = epicValue;

                  //printf ("\tvar=%d   pos=%d  value=%f\n", j, pos,  dataV[j][index] );

               }   //j

            } //epic data lines

            newloop:
            getline( imageStream, lineStr);

            //printf ( "\tlineNums = %d\n", lineNums );

         }  // while loop

        printf ("\tFinished reading: %d lines\n\n", lineNums );
        imageStream.close();

     } // if good
     else
     {
        printf ("\tError: Open file - %s\n", epicFiles[i].c_str() );
        exit ( 1 );
     }

   }   //i

}
