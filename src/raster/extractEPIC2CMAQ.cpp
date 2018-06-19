/***************************************************************************
 * This program is used:
 *  1. to process EPIC modeling output files for CMAQ bi-directional 
 *     NH3 surface flux modeling
 *  2. to output three WRF format NetCDF files: *_soil.ncf, *_time.ncf and *_fert.ncf
 *  3. Time step -- can be daily or monthly
 *
 * Written by the Institute for the Environment at UNC, Chapel Hill
 * in support of US EPA ORD AMAD
 *
 * By LR, July-Aug. 2010
 *
 * Usage:  extractEPIC2CMAQ.exe
 *
 * Environment Variables needed:
 *         GRID_ROWS  -- grid domain rows
 *         GRID_COLUMNS -- grid domain columns
 *         GRID_XMIN -- grid domain xmin
 *         GRID_YMIN -- grid domain ymin
 *         GRID_XCELLSIZE -- grid domain x grid size
 *         GRID_YCELLSIZE -- grid domain y grid size
 *         GRID_PROJ -- grid domain proj4 projection definition
 *         GRID_NAME -- grid name IOAPI output
 *         DATA_DIR -- directory contains EPIC output data to be extracted
 *         START_DATE --  start date and time YYYYMMDD
 *         END_DATE -- end date and time YYYYMMDD
 *         SOIL_OUTPUT_NETCDF_FILE - EPIC soil information output
 *         DAILY_OUTPUT_NETCDF_FILE - EPIC Daily/monthly information output

***********************************************************************************/
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <set> 

#include "sa_raster.h"
#include "commontools.h"
#include "geotools.h"


//soil output functions
void extractSoilData (float *dataV[], vector<string> soilFiles, gridInfo grid);

void extractDMData (float *dataV[], vector<string> dmFiles, string startTimeStr, string endTimeStr, gridInfo grid, int dayNum);

//gloabl variables

int                      numSoils;           //number of soils in EPIC modeling

string                   cropNamesStr = string ( "Hay,Hay_ir,Alfalfa,Alfalfa_ir,Other_Grass,Other_Grass_ir,Barley,Barley_iri,BeansEdible,BeansEdible_ir,CornGrain,CornGrain_ir,CornSilage,CornSilage_ir,Cotton,Cotton_ir,Oats,Oats_ir,Peanuts,Peanuts_ir,Potatoes,Potatoes_ir,Rice,Rice_ir,Rye,Rye_ir,SorghumGrain,SorghumGrain_ir,SorghumSilage,SorghumSilage_ir,Soybeans,Soybeans_ir,Wheat_Spring,Wheat_Spring_ir,Wheat_Winter,Wheat_Winter_ir,Other_Crop,Other_Crop_ir,Canola,Canola_ir,Beans,Beans_ir" );   //crop number from 22 to 63 

const double sigmaLevels[  ] = {0.22, 0.23, 0.24,0.25,0.26,0.27,0.28,0.29,0.30,0.31,0.32,0.33,0.34,0.35,0.36,0.37,0.38,0.39,0.40,0.41,0.42,0.43,0.44,0.45,0.46,0.47,0.48,0.49,0.50,0.51,0.52,0.53,0.54,0.55,0.56,0.57,0.58,0.59,0.60,0.61,0.62,0.63,1.0};

int                      numCrops;           //number of crops in EPIC modeling
std::map<string,int>     cropNamesH;               //hash tables to store landuse class IDs and index

/*******************************
 * set soil property variables *
 *******************************/
int                      numNCSFileItems = 26;
string                   soilVarsStr = string ( "1.SoilNum, 2.Bulk Density, 3.Wilt Point, 4.Field Capacity, 5.Porosity, 6.PH, 7.Cation Ex");  
static int               soilVarsPos[] = {8,13,14,15,16,17,18,20,21,22,23,24,25};   //for two layers, needs DEC31N
int                      numSoilVars;

const Name variableNamesS[ ] = { "L1_SoilNum", "L1_Bulk_D", "L1_Wilt_P", "L1_Field_C","L1_Porosity","L1_PH","L1_Cation", "L2_Bulk_D", "L2_Wilt_P", "L2_Field_C","L2_Porosity","L2_PH","L2_Cation" };

const Name variableUnitsS[ ] = { "None", "t/m**3", "m/m", "m/m", "%","None", "cmol/kg", "t/m**3", "m/m", "m/m", "%","None", "cmol/kg" };

const Line variableDescriptionsS[ ] = { "Soil Number", "Layer1 Bulk Density", "Layer1 Wilting Point", "Layer1 Field Capacity","Layer1 Porosity","Layer1 PH","Layer1 Cation Ex", "Layer2 Bulk Density", "Layer2 Wilting Point", "Layer2 Field Capacity","Layer2 Porosity","Layer2 PH","Layer2 Cation Ex" 
     };
/***********************************
 * EPIC daily or monthly variables *
 ***********************************/
bool                     monthTimeStep;    // monthly or daily

int                      numNCMFileItems = 88;

// start from 0

static int               epicVarsPos[] = {15,16,18,19,24,26,27,28,43,44,
                                          45,46,47,48,51,52,53,54,55,56,
                                          57,58,61,62,63,64,65,66,67,70,
                                          71,72,73,74,77,78,79,85,86,87};

int                      numEpicVars = 40;

const Name variableNamesM[ ] = { "DN", "DN2", "HMN", "NFIX", "YW", "FPO", "FPL", "MNP", "L1_DEP", "L1_BD",
                                 "L1_SW", "L1_NO3", "L1_NH3", "L1_ON", "L1_C", "L1_NITR", "L2_DEP","L2_BD", "L2_SW", "L2_NO3",
                                 "L2_NH3", "L2_ON", "L2_C", "L2_NITR", "T1_DEP", "T1_BD", "T1_NO3", "T1_NH3","T1_ON", "T1_C", 
                                 "T1_NITR", "L1_ANO3", "L1_ANH3", "L1_AON", "L2_ANO3", "L2_ANH3", "L2_AON", "LAI", "CPHT", "FBARE" };  //40 vars

const Name variableUnitsM[ ] = { "kg/ha", "kg/ha", "kg/ha", "kg/ha", "ton/ha","kg/ha", "kg/ha", "kg/ha", "m", "t/m**3", 
                                 "mm", "kg/ha", "kg/ha", "kg/ha", "kg/ha", "kg/ha", "m", "t/m**3", "mm", "kg/ha",
                                 "kg/ha", "kg/ha", "kg/ha", "kg/ha", "m", "t/m**3", "kg/ha", "kg/ha","kg/ha", "kg/ha", 
                                 "kg/ha", "kg/ha", "kg/ha","kg/ha", "kg/ha", "kg/ha", "kg/ha", "m-2/m-2",  "m", "fraction" }; //40 items


const Line variableDescriptionsM[ ] = { "N-NO3 Denitrification", "N-N2O from NO3 Denitrification", "OC Change by Soil Respiration", "N Fixation", "Wind Erosion",
                                        "Organic P Fertilizer", "Labile P Fertilizer"," P Mineralized", "Layer1 Depth", "Layer1 Bulk Density", 
                                        "Layer1 Soil Water", "Layer1 N - Nitrate", "Layer1 N - Ammonia", "Layer1 N - Organic N", "Layer1 Carbon", 
                                        "Layer1 N - Nitrified NH3", "Layer2 Depth", "Layer2 Bulk Density", "Layer2 Soil Water", "Layer2 N - Nitrate", 
                                        "Layer2 N- Ammonia", "Layer2 N - Organic N", "Layer2 Carbon", "Layer2 N - Nitrified NH3", "TotalSoilnoLayer1 Depth",
                                        "TotalSoilnoLayer1 Bulk Density", "TotalSoilnoLayer1 N - Nitrate", "TotalSoilnoLayer1 N - Ammonia", "TotalSoilnoLayer1 N - Organic N", "TotalSoilnoLayer1 Carbon", 
                                        "TotalSoilnoLayer1 N - Nitrified NH3", "Layer1 N-NO3 AppRate", "Layer1 N-NH3 AppRate", "Layer1 N-ON AppRate", "Layer2 N-NO3 AppRate",
                                        "Layer2 N-NH3 AppRate", "Layer2 N-ON AppRate", "Leaf Area Index", "Crop Height", "Bare Land Fraction for Wind Erosion" };  //40 items



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
        string    outNetcdfFile_s, outNetcdfFile_d;
        string    tmp_str;
        string    satImageFile,geoImageFile;              
        string    dataDate;

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
        grid.xmin = atof( getEnviVariable("GRID_XMIN") );
        grid.ymin = atof( getEnviVariable("GRID_YMIN") );
        printf( "\txmin=%f    ymin=%f \n", grid.xmin, grid.ymin);

        //get x and y cell size
        grid.xCellSize = atof( getEnviVariable("GRID_XCELLSIZE") );
        grid.yCellSize = atof( getEnviVariable("GRID_YCELLSIZE") );
 
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

       
        /***********************************
        *       get day and time range     *
        ***********************************/
        //printf( "Getting day and time range to extract EPIC output data.\n");
        startDateTime = string( getEnviVariable("START_DATE") );
        startDateTime = trim( startDateTime );
        endDateTime = string( getEnviVariable("END_DATE") );
        endDateTime = trim( endDateTime );
        printf ( "\tStart date and time is:  %s\n",startDateTime.c_str() );
        printf ( "\tEnd date and time is:  %s\n",endDateTime.c_str() );
        if ( startDateTime.length() != 8 || endDateTime.length() != 8 )
        {
           printf ( "\tError: Start and End date/time format should be: YYYYMMDD\n" );
           exit ( 1 );
        }
        if ( atol(startDateTime.c_str()) == 0 ||  atol(endDateTime.c_str()) == 0 )
        {
           printf ( "\tError: Start and End date/time should be all numbers\n" );
           exit ( 1 );
        }

        /***********************************
        *       get output file names      *
        ***********************************/
        //printf( "Getting output NetCDF file names\n");
        string outFilePrefix = string( getEnviVariable("OUTPUT_NETCDF_FILE_PREFIX") );
        outFilePrefix = trim ( outFilePrefix );

        outNetcdfFile_s = outFilePrefix + string ("_soil.nc"); 
        printf( "\tSoil output NetCDF file is: %s\n", outNetcdfFile_s.c_str() );
        FileExists(outNetcdfFile_s.c_str(), 3 );  //the file has to be new.

        //daily EPIC output file prefix
        outNetcdfFile_d = outFilePrefix + string ("_time");
         

       /*****************************************
       * Get all data files to be processed    *
       ******************************************/
       DIR             *dpFiles;          //data directory
       struct dirent   *dirpFiles;
       struct stat     stFileInfo;

       vector<string>  soilFiles, dailyFiles, monthFiles, dmFiles;
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

             if ( tmp_str.find ( ".NCS" ) != string::npos  )
             {
                //printf( "\tSoil file obtained: %s\n", fileName.c_str() );
                soilFiles.push_back ( fileName );
             }  
             else if ( tmp_str.find ( ".NCD" ) != string::npos  )
             {
                //printf( "\tDaily file obtained: %s\n", fileName.c_str() );
                dailyFiles.push_back ( fileName );
             }
             else if ( tmp_str.find ( ".NCM" ) != string::npos  )
             {
                //printf( "\tMonthly file obtained: %s\n", fileName.c_str() );
                monthFiles.push_back ( fileName );
             }
           
             //!!!!checking
             //printf ("\t%s\n", tmp_str.c_str() );

          }  //while dirpFiles

          closedir(dpFiles);

       }  // if dpFiles
       else
       {
          printf( "Directory does not exist: %s\n",dataDir.c_str() );
       }

       //chose daily files to process if they exist. Otherwise use monthly output files
       if (dailyFiles.size() != 0 )
       {
          dmFiles = dailyFiles;
          dailyFiles.clear();
          monthTimeStep = false;
       }
       else
       {
          dmFiles = monthFiles;
          monthFiles.clear();
          monthTimeStep = true;
       } 
         
       //sort obtained files
       sort ( soilFiles.begin(), soilFiles.end() );
       sort ( dmFiles.begin(), dmFiles.end() );

       printf ("\t Soil files: %d    Monthly files: %d\n", soilFiles.size(), dmFiles.size() );
                   
       /*
       if ( soilFiles.size() != dmFiles.size() || dmFiles.size() )
       {
          printf ("Error: EPIC output files do not have matched: NCS, NCM/NCD, and TNA files.\n");
          exit ( 1 );
       }
       */

       /**************************************
       * Define variables to read EPIC files *
       ***************************************/
       vector<string> cropNamesV = string2stringVector (cropNamesStr, ",");
       numCrops = cropNamesV.size();
       printf ("Number of crops: %d\n", numCrops);

       //get soil variable string vector
       vector<string> soilVarsV = string2stringVector (soilVarsStr, ",");
       
       numSoilVars = soilVarsV.size() + 6;  //two layers soil variables
       printf ("Number of soil variables in soil output file: %d\n\n", numSoilVars );

       /*****************************
       * Read EPIC soil output file *
       ******************************/
       int        totalSize;                      //total dim size
       float      *dataVS[numSoilVars];

       totalSize = numCrops * grid.rows * grid.cols;
       printf ("\tTotalSize = %d\n", totalSize );

       for (i=0; i<numSoilVars; i++)
       {
          dataVS[i] =  allocateFloatDataArrayMemory ( totalSize );

          //set missing values
          fillFloatArrayMissingValue ( totalSize, dataVS[i] );
       }

       extractSoilData (dataVS, soilFiles, grid);

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

     
       string startDateStr_julian = getJulianDayStr ( startDateTime );

       const int VARIABLESS = numSoilVars;
       const int TIMESTEPSS = 1;
       const int LAYERS = numCrops;
       const int ROWS = grid.rows;
       const int COLUMNS = grid.cols;
       const int YYYYDDD = atoi ( startDateStr_julian.c_str() );
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

       const Line descriptionS = "EPIC soil output data";

       /************************
       * Write soil IOAPI file *
       *************************/
        int fileInfo;

        if (  proj == LCC )
        {
           fileInfo = createLambertConformalM3IOFile ( outNetcdfFile_s.c_str(),
                    VARIABLESS, TIMESTEPSS, LAYERS, ROWS, COLUMNS, YYYYDDD,
                    startTime_att, timeSteps_att,
                    xcellSize, ycellSize, westEdge, southEdge,
                    centralLongitude, centralLatitude, centralLongitude,
                    lowerLatitude, upperLatitude,
                    topPressure, sigmaLevels,
                    variableNamesS, variableUnitsS, variableDescriptionsS,
                    grid.name.c_str(), descriptionS );
        }
        else if ( proj == LATLONG )
        {
           fileInfo = createLonLatM3IOFile ( outNetcdfFile_s.c_str(),
                    VARIABLESS, TIMESTEPSS, LAYERS, ROWS, COLUMNS, YYYYDDD,
                    startTime_att, timeSteps_att,
                    xcellSize, ycellSize,
                    westEdge, southEdge,
                    topPressure, sigmaLevels,
                    variableNamesS, variableUnitsS, variableDescriptionsS,
                    grid.name.c_str(), descriptionS );
        }
        else if ( proj == MERC )
        {
           fileInfo = createEquitorialMercatorM3IOFile ( outNetcdfFile_s.c_str(),
                    VARIABLESS, TIMESTEPSS, LAYERS, ROWS, COLUMNS, YYYYDDD,
                    startTime_att, timeSteps_att,
                    xcellSize, ycellSize,
                    westEdge, southEdge,
                    centralLongitude, centralLatitude,
                    centralLongitude, upperLatitude,
                    topPressure, sigmaLevels,
                    variableNamesS, variableUnitsS, variableDescriptionsS,
                    grid.name.c_str(), descriptionS );

        }
        else if ( proj == UPS )
        {
           fileInfo = createStereographicM3IOFile ( outNetcdfFile_s.c_str(),
                    VARIABLESS, TIMESTEPSS, LAYERS, ROWS, COLUMNS, YYYYDDD,
                    startTime_att, timeSteps_att,
                    xcellSize, ycellSize,
                    westEdge, southEdge,
                    centralLongitude, centralLatitude,
                    centralLongitude, upperLatitude,
                    topPressure, sigmaLevels,
                    variableNamesS, variableUnitsS, variableDescriptionsS,
                    grid.name.c_str(), descriptionS );

        }

       const int fileS = fileInfo;

       int ok = fileS != -1;

       if ( fileS != -1 )
       {
          for (i=0; i<numSoilVars; i++)
          {
             ok = writeM3IOData( fileS, variableNamesS[i], dataVS[i] );
          }
       }
       ok = closeM3IOFile( fileS ) && ok;
 
       for (i=0; i<numSoilVars; i++)
       {
          free ( dataVS[i] );
       }

       printf( "Finished writing soil output file: %s\n\n", outNetcdfFile_s.c_str());



       /**************************************************
       * Define daily or monthly output file variables   *
       ***************************************************/
       
       printf ("Number of epic variables in time step output file: %d\n\n", numEpicVars );

       //time step
       string stepTimeStr, endTimeStr;
       long   stepTimeLong, endTimeLong;

       string stepMonthStr, endMonthStr;
       long   stepMonthLong, endMonthLong; 

       int    startMonInt, startYearInt;
       int    endMonInt, endYearInt;
  
       char     numChars[3];

       
       /*************************************
        *  extract data one month at a time *
        *************************************/

       stepTimeStr =  startDateTime;  //set beginning date step
       stepTimeLong = atol ( stepTimeStr.c_str() );
          
       stepMonthStr =  startDateTime.substr(0, 6);  //get YYYYMM

       endMonthStr = endDateTime.substr(0, 6);
       endTimeLong = atol ( endDateTime.c_str() );
       
       stepMonthLong = atol ( stepMonthStr.c_str() );
       endMonthLong = atol ( endMonthStr.c_str() );

       int    monthSteps = 0;
       while ( stepMonthLong <= endMonthLong && stepTimeLong <= endTimeLong )
       {

          if ( stepMonthLong < endMonthLong )               
          {
             //before last month
             string yearStr = stepMonthStr.substr (0, 4);
             string monthStr = stepMonthStr.substr (4, 2);

             int yearInt = atoi ( yearStr.c_str() );
             int monthInt = atoi ( monthStr.c_str() );

             int daysMonth = GetDaysInMonth ( yearInt, monthInt );

             sprintf (numChars, "%d\0", daysMonth );
 
             endTimeStr = stepMonthStr + string ( numChars );
          }
          else
          {
             //last month
             endTimeStr = endDateTime;
          }


          int timeStepsM = getTimeSteps ( stepTimeStr, endTimeStr );

          printf ("\tStart Time: %s   End Time: %s  MonthSteps: %d  DaySteps: %d\n", stepTimeStr.c_str(), endTimeStr.c_str(), monthSteps+1, timeStepsM);



          //day time step
          int timeSteps = 0;

          //obtain one month data at a time
          float      *dataVM[numEpicVars*timeStepsM];

          for (i=0; i<numEpicVars*timeStepsM; i++)
          {
             dataVM[i] =  allocateFloatDataArrayMemory ( totalSize );

             //set missing values
             fillFloatArrayMissingValue ( totalSize, dataVM[i] );
          } 

          extractDMData (dataVM, dmFiles, stepTimeStr, endTimeStr, grid, timeStepsM);

 
          //write output by day for each file
          string currentDay = stepTimeStr; 

          for (i=0; i<timeStepsM; i++)
          {

             printf ("\tWrite time step: %s\n", currentDay.c_str() );
             
             string stepTimeStr_julian = getJulianDayStr ( currentDay );

             
             /*****************************
             * Write time step IOAPI file *
             ******************************/
             const Line descriptionM = "EPIC daily output data"; 
             const int VARIABLESM = numEpicVars;
             const int TIMESTEPSM = 1;
             const int step_YYYYDDD = atoi ( stepTimeStr_julian.c_str() );
             const int timeStepsM_att = 240000;


             string  outNetcdfFilem = outNetcdfFile_d + currentDay + string ( ".nc" );

             FileExists(outNetcdfFilem.c_str(), 3 );  //the file has to be new.

             //file one


             int fileInfoM;

             if (  proj == LCC )
             {
                fileInfoM = createLambertConformalM3IOFile ( outNetcdfFilem.c_str(),
                    VARIABLESM, TIMESTEPSM, LAYERS, ROWS, COLUMNS, step_YYYYDDD,
                    startTime_att, timeStepsM_att,
                    xcellSize, ycellSize, westEdge, southEdge,
                    centralLongitude, centralLatitude, centralLongitude,
                    lowerLatitude, upperLatitude,
                    topPressure, sigmaLevels,
                    variableNamesM, variableUnitsM, variableDescriptionsM,
                    grid.name.c_str(), descriptionM );
             }
             else if ( proj == LATLONG )
             {
                fileInfoM = createLonLatM3IOFile ( outNetcdfFilem.c_str(),
                    VARIABLESM, TIMESTEPSM, LAYERS, ROWS, COLUMNS, step_YYYYDDD,
                    startTime_att, timeStepsM_att,
                    xcellSize, ycellSize,
                    westEdge, southEdge,
                    topPressure, sigmaLevels,
                    variableNamesM, variableUnitsM, variableDescriptionsM,
                    grid.name.c_str(), descriptionM );
             }
             else if ( proj == MERC )
             {
                fileInfoM = createEquitorialMercatorM3IOFile ( outNetcdfFilem.c_str(),
                    VARIABLESM, TIMESTEPSM, LAYERS, ROWS, COLUMNS, step_YYYYDDD,
                    startTime_att, timeStepsM_att,
                    xcellSize, ycellSize,
                    westEdge, southEdge,
                    centralLongitude, centralLatitude,
                    centralLongitude, upperLatitude,
                    topPressure, sigmaLevels,
                    variableNamesM, variableUnitsM, variableDescriptionsM,
                    grid.name.c_str(), descriptionM );

             }
             else if ( proj == UPS )
             {
                fileInfoM = createStereographicM3IOFile ( outNetcdfFilem.c_str(),
                    VARIABLESM, TIMESTEPSM, LAYERS, ROWS, COLUMNS, step_YYYYDDD,
                    startTime_att, timeStepsM_att,
                    xcellSize, ycellSize,
                    westEdge, southEdge,
                    centralLongitude, centralLatitude,
                    centralLongitude, upperLatitude,
                    topPressure, sigmaLevels,
                    variableNamesM, variableUnitsM, variableDescriptionsM,
                    grid.name.c_str(), descriptionM );

             }

             const int fileM = fileInfoM;
             ok = fileM != -1;

             if ( fileM == -1 )
             {
                 printf ("\tError: creating netCDf file: %s\n", outNetcdfFilem.c_str() );
                 exit ( 1 );
             }

             
             for (j=0; j<numEpicVars; j++)
             {
                ok = writeM3IODataForTimestep ( fileM, 0, variableNamesM[j], dataVM[i*numEpicVars+j] );
             }

             ok = closeM3IOFile( fileM ) && ok;

             currentDay = getNextDayStr (currentDay );


             printf( "\tFinished writing epic daily time step output file: %s\n", outNetcdfFilem.c_str() );

          }


          for (i=0; i<numEpicVars*timeStepsM; i++)
          {
             free ( dataVM[i] );
          }

           
          stepTimeStr = getNextDayStr ( endTimeStr );   //first day of next month
 

          stepMonthStr = stepTimeStr.substr(0, 6);  //get YYYYMM

          stepMonthLong = atol ( stepMonthStr.c_str() );

          stepTimeLong = atol ( stepTimeStr.c_str() );

          monthSteps ++;
          //printf ( "\tstepTimeStr: %s  monthSteps: %d\n", stepTimeStr.c_str(), monthSteps+1 ); 

      }//monthly files


}


/*********************End of Main program****************************************************************/




/**********************************
 *      Process NCS soil files    *
***********************************/
void extractSoilData (float *dataV[], vector<string> soilFiles, gridInfo grid)
{
   ifstream        imageStream;             //stream to read input EPIC site file
   string          lineStr;
   vector<string>  vecString;
   int             gridID, cropID, soilID; 
   int             row, col, cropIndex, index;
   int             i, j, k, m, n, lineNums;

   string          tmp_str;


   printf("\nReading EPIC site soil output NCS files...\n" );

   string fileType = string ( "EPICAVER" );

   for ( i=0; i<soilFiles.size(); i++ )
   {

       m = soilFiles[i].rfind("/", soilFiles[i].length() );
       string cropID_str = soilFiles[i];
       cropID_str.erase (0, m+1 );

       m = cropID_str.find ( "." );  
       cropID_str.erase (m, cropID_str.size()-m );

       int name_cropID = atoi ( cropID_str.c_str() );

      lineNums = 0;
      imageStream.open( soilFiles[i].c_str() );
      if (imageStream.good() )
      {
         getline( imageStream, lineStr, '\n');
         while ( !imageStream.eof() )
         {
            lineStr = trim (lineStr);                 //get rid of spaces at edges
            //lineStr = deleteStrNewLineChar(lineStr);  //get rid of the new line char
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

               if ( vecString.size()  != numNCSFileItems + 1 )
               {
                  printf( "\tError: NCS file = %s    items = %d   Standard Items = %d\n", soilFiles[i].c_str(), vecString.size(), numNCSFileItems );
                  exit ( 1 );
                  /*
                  while ( vecString.size() > numNCSFileItems )
                  {
                     vecString[10] = vecString[10]+vecString[11];
                     vecString.erase( vecString.begin()+11);
                  }*/
               }

               string runName = vecString[0];

               gridID = getGridIDFromFileName ( runName, fileType );
               cropID = getCropIDFromFileName ( runName, fileType );
               cropID = cropID - 21;  //BELD4 cropID starts from 1

               //printf ("\tfile: %s\n", soilFiles[i].c_str() );
 
               if ( cropID != name_cropID )
               {
                  printf( "\tError: Crop ID %d in NCS file %s does not match inside RunName crop ID %d:\n", name_cropID, soilFiles[i].c_str(), cropID); 
                  exit ( 1 );
               }

   
               //get row and col: start from 1
               col = (gridID -1) % grid.cols + 1;
               row = (gridID - col) / grid.cols + 1;

               //get row and col: start from 0
               col = col - 1;
               row = row - 1;

               //get crop index
               cropIndex = cropID - 1;  //starts from 0
            

               index = cropIndex * grid.rows * grid.cols  + row * grid.cols + col;

               //printf ("\tgridID=%d  cropID=%d  col=%d  row=%d  cropIndex=%d\n", gridID, cropID, col, row, cropIndex);

               //get soil ID
               tmp_str = vecString[8];   //ST+soilCode
               tmp_str.erase (0,2);
               soilID = atoi ( tmp_str.c_str() ) ;
               //printf ("\tSoil Num = %d\n", soilID ); 

               for ( k=0; k<numSoilVars; k++)
               {
                  if (k == 0 )
                  {
                      dataV[k][index] = soilID;
                  }
                  else
                  {
                     int pos = soilVarsPos [ k ];
                     tmp_str = vecString [pos];
                     if ( tmp_str.find ("*") != string::npos )
                     {
                        printf ("\nError: *** in EPIC soil file- %s\n", soilFiles[i].c_str() );
                        exit ( 1 );
                     }

                     double soilValue = atof ( tmp_str.c_str() ) ;

                     dataV[k][index] = soilValue;
                     //printf ("\tlayer=%d   var=%d  pos=%d  index=%d   value=%f\n", j, k, pos, index, soilValue );
                  }
               }   //k

            } //soil lines

            newloop:
            getline( imageStream, lineStr);

         }  // while loop

         printf ("\tFinished reading: %s   %d lines\n\n", soilFiles[i].c_str(), lineNums );
         imageStream.close();

      } // if good
      else
      {
         printf ("\tError: Open file - %s\n", soilFiles[i].c_str() );
         exit ( 1 );
      }

   }   //i

}  //end of extracting soil info



/********************************************************************
 *      Process annual (TNA) and timestep (NCM/NCD) output files    *
*********************************************************************/
void extractDMData (float *dataV[], vector<string> dmFiles, string startTimeStr, string endTimeStr, gridInfo grid, int dayNum)
{

   ifstream        imageStream;             //stream to read input EPIC site file
   string          lineStr;
   vector<string>  vecString;
   int             gridID, cropID, soilID; 
   int             row, col, cropIndex, index;
   int             i, j, k, m, n, lineNums;

   long            startTimeLong, endTimeLong, stepTimeLong;
   string          tmp_str;


   printf("\nReading EPIC site daily time step output NCD files...\n\n" );

   string fileType = string ( "EPICAVER" );


   //get beginning day
   string startDayStr = startTimeStr.substr (6, 2);
   int startDayInt = atoi ( startDayStr.c_str() );

   startTimeLong = atol ( startTimeStr.c_str() );
   endTimeLong = atol ( endTimeStr.c_str() );

   for ( i=0; i<dmFiles.size(); i++ )
   {
      m = dmFiles[i].rfind("/", dmFiles[i].length() );
      string cropID_str = dmFiles[i];
      cropID_str.erase (0, m+1 );

      m = cropID_str.find ( "." );  
      cropID_str.erase (m, cropID_str.size()-m );

      int name_cropID = atoi ( cropID_str.c_str() );

      lineNums = 0;
      imageStream.open( dmFiles[i].c_str() );
      if (imageStream.good() )
      {
         getline( imageStream, lineStr, '\n');
         while ( !imageStream.eof() )
         {
            lineStr = trim (lineStr);                 //get rid of spaces at edges
            //lineStr = deleteStrNewLineChar(lineStr);  //get rid of the new line char

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

   
               //output line ends with ","
               if ( vecString.size()  != numNCMFileItems )   
               {
                  printf( "\tError: File = %s  items = %d  Standard Items = %d\n", dmFiles[i].c_str(), vecString.size(), numNCMFileItems );
                  printf ( "\tlineNums = %d: %s\n", lineNums, lineStr.c_str() );
                  exit ( 1 );
               }

               string runName = vecString[0];

               gridID = getGridIDFromFileName ( runName, fileType );
               cropID = getCropIDFromFileName ( runName, fileType );

               cropID = cropID - 21;  //BELD4 cropID starts from 1

               if ( cropID != name_cropID )
               {
                  printf( "\tError: Crop ID in NCD file %s does not match inside RunName crop ID %d:\n", dmFiles[i].c_str(), cropID);
                  exit ( 1 );
               }

               //printf ("\tfile: %s\n", dmFiles[i].c_str() );

               //get row and col: start from 1
               col= (gridID -1) % grid.cols + 1;
               row = (gridID - col) / grid.cols + 1;

               //get row and col: start from 0
               col = col - 1;
               row = row - 1;

               //get crop index: starts from 0
               cropIndex = cropID - 1;

               index = cropIndex * grid.rows * grid.cols  + row * grid.cols + col;
               //printf ("\tgridID=%d  cropID=%d  col=%d  row=%d  cropIndex=%d\n", gridID, cropID, col, row, cropIndex);

               //get  time string
               int value;

               string tmp_YesrStr = vecString[8];  //get year
               string tmp_MonStr = vecString[9];   //get month

               //month
               value = atoi ( tmp_MonStr.c_str() );
               string tmp_timeSep = tmp_YesrStr +  convert2NumsTo2Chars ( value );
           
               //day
               string tmp_DayStr = vecString[10];  //get day 
               value = atoi (tmp_DayStr.c_str() );
               int stepDayInt = value;

               tmp_timeSep = tmp_timeSep +  convert2NumsTo2Chars ( value );
               stepTimeLong = atol ( tmp_timeSep.c_str() );

               //printf ("\tTime step = %s\n", tmp_timeSep.c_str() ); 
               
               if ( stepTimeLong < startTimeLong || stepTimeLong > endTimeLong )
               {
                  goto newloop; 
               }
            
               //within the month and get day index
               int dayIndex = stepDayInt - startDayInt;

               for (j=0; j<numEpicVars; j++)
               {
                   
                  int pos = epicVarsPos [j];   
                  tmp_str = vecString [pos];

                  if ( tmp_str.find ("*") != string::npos )
                  {
                     printf ("\nError: *** in EPIC daily file- %s\n", dmFiles[i].c_str() );
                     exit ( 1 );
                  }

                  double epicValue = atof ( tmp_str.c_str() ) ;

                  dataV[dayIndex*numEpicVars+j][index] = epicValue;

                  //printf ("\tvar=%d   pos=%d  value=%f\n", j, pos, epicValue );
               } //j


            } //epic lines

            newloop:
            getline( imageStream, lineStr);

         }  // while loop

        printf ("\tFinished reading: %s    %d lines\n\n", dmFiles[i].c_str(), lineNums );
        imageStream.close();

     } // if good
     else
     {
        printf ("\tError: Open file - %s\n", dmFiles[i].c_str() );
        exit ( 1 );
     }

   }   //i

}

/****************** End of the program *********************************************/


