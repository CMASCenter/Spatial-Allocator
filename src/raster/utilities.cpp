/********************************************************************
 * This program includes some common utilities used by other programs: 
 *  1. FileExists - Check file and directory existence. 
 *  2. trim, trimL and trimR - Trim a string
 *  3. getEnviVariable - Get environmental variable
 *  4. processDirName - Process dir to make that it ends with path separator
 *  5. stringToUpper - Convert a string to upper case
 *  6. getValueInProj4Str - Get a value parameter from PROJ4 string
 *  7. getStrInProj4Str - Get a string parameter from PROJ4 string
 *  8. convert2LatLongProjection - Convert PROJ4 projection to PROJ4 latlong projection
 *  9. string2stringVector - convert string to string vector
 * 10. IsLeapYear - chech year is leap year or not
 * 11. GetDaysInMonth - get the number of days in a month
 * 12. convert2NumsTo2Chars - get 2 chars string for a 2-digits number
 * 13. isStepTimePassEndTime - check whether a time step passes the end time
 * 14. getDateTimeStr - get the date and time string YYYYMMDDHHMM given a start date/time string and seconds passed
 * 15. getDateTimeRange - fill date, hour, minute range 2d arrays for YYYYMMDDHHMM start and end time
 * 16. fillTimeArrays - fill time arrays needed to output WRF Netcdf files and return total number of time steps
 * 17. getNextDayStr - get next day string
 * 18. matchGOESTimeStep - get closest GOES time step string from time in a file name 
 * 19. deleteStrNewLineChar - delete the new line character from a string obtained by getline 
 * 20. getProjType - get WRF projection type.  4 types in total
 * 21. getJulianDayStr - get YYYYDDD string from YYYYMMDD string
 * 22. checkYYYYMMDD - check day YYYYMMDD is correct or nor
 * 23. convertMCIPTimeStep2Mins - convert MCIP time step in HHMMSS int to minutes
 * 24. getSecondsFrom19930101 - get seconds passed since 19930101 to YYYYMMDDHHMM
 * 25. getDateFromJulian - get YYYYMMDD from Julian date
 * 26. obtainSatFileNames - get all MODIS 04/06 L2 and 03 L1 files, MISR AEROSOL L2 files, or OMI L2 files in the directory
 * 27. getDayTimeStrFromSatFileName - get day and time string (YYYYDDDHHMM)
 * 28. getTimeMinutesPased - compute minutes passed for a time from the start date/time
 * 29. convertDate2OutputDateStr - convert date YYYYMMDDHHMM to "0000-00-00_00:00:00" format
 * 30. projectLatLongtoANNpointArray - project lat and long array to domain projection and store points in ANNpointArray
 * 31. getGridIDFromFileName - get modeling grid ID from file name
 * 32. stringVector2stringMap - convert string vector to string hash table 
 * 33. allocateFloatDataArrayMemory - calloc float data array memory
 * 34. findOneDataFile - find a file from a data directory with match name type and date *date
 * 35. fillFloatArrayMissingValue - fill a float array with MISSIING_VALUE
 * 36. allocateIntDataArrayMemory - allocate int data array memory
 * 37. fillIntArrayMissingValue - fill int array missing value
 * 38. getCropIDFromFileName - get crop ID from file name
 * 39. getSoilIDFromFileName - get soil ID from file name
 * 40. mapKey2stringVector - convert map key to string vector
 * 41. getNextMonthStr - get next month YYYYMM string
 * 42. getTimeSteps - get time steps for YYYY, YYYYMM, YYYYMMDD
 * 43. fillCharArray - fill a fixed char array using string vector
 * 44. getGridIDFromRunName  - get grid ID and crop ID from EPIC run name
 * 45. checkSelectionYesNo - selection has to be YES or NO (not case sensitive)
 * 46. write_geogrid - write geogrid data set and obtained from WRF geogrid package
 * 47. allocateShortDataArrayMemory - allocate short memory
 * 48. comparePROJ4GDALPRJ - compare proj4 projection string with GDAL projection string
 * 49. stringVector2string - convert string vector to string separated by ","
 * 50. fillFloatArrayMissingValueVar - fill missing value variable 
 * 51. dayofweek - find the day of week (1-monday ... 7-sunday)
 *
 * Written by the Institute for the Environment at UNC, Chapel Hill
 * in support of the EPA NOAA CMAS Modeling, 2007-2008.
 * 
 * By Limei Ran, Feb-March 2008
 *
 * Usage: to be included in other programs
***********************************************************************/

#include <sys/stat.h>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "commontools.h"
#include <cstring>


float PRJ_MISSIING_VALUE=-9999.0;

/************************************************************************/
/*        void FileExists(const char *strFilename, int eCode)           */
/************************************************************************/

void FileExists(const char *strFilename, int eCode) {
  struct stat stFileInfo;
  int intStat;

  /*  eCode
      0 = file or directory has to exist
      1 = file or directory has to be new 
      2 = directory has to exist, if not create it 
      3 = file exists and remove it
  */
      
  // Attempt to get the file attributes
  intStat = stat(strFilename,&stFileInfo);
  //printf("File = %s   intStat = %d   eCode = %d\n",strFilename,intStat,eCode);
  //file had to be new
  if(intStat == 0 && eCode == 1) 
  {
    // get the file attributes, so the file obviously exists.
    //eCode = 1: the file has to be new 
    printf("\tError: File exists: %s\n",strFilename);
    exit(1);
  } 

  //file has to exist
  if (intStat != 0 && eCode == 0)  
  {
    // We were not able to get the file attributes.
    // This may mean that we don't have permission to
    // access the folder which contains this file. If you
    // need to do that level of checking, lookup the
    // return values of stat which will give you
    // more details on why stat failed.

    //eCode = 0 : the file has to exist
   
     printf("\tError: File doe not exist: %s\n",strFilename);
     exit(1);
  }

  //create new dir
  if (intStat != 0 && eCode == 2) 
  {
    // We were not able to get the file attributes.
    // This may mean that we don't have permission to
    // access the folder which contains this file. If you
    // need to do that level of checking, lookup the
    // return values of stat which will give you
    // more details on why stat failed.

    //eCode = 2 : the directory has to exist
    if(mkdir(strFilename,0777)==-1)//creating a directory
    {
        printf ("\tError: could not create directory: %s\n",strFilename);
        exit(1);
    } 
  }

  //file exists and remove it
  if(intStat == 0 && eCode == 3)
  {
     // get the file attributes, so the file obviously exists.
     //eCode = 3: delete the file
     if( remove( strFilename ) != 0 )
     {
        printf( "\tError deleting file: %s\n", strFilename );
        exit (1); 
     }
     else
     {
        //printf("\tDeleted existing file: %s\n",strFilename);
     }
    
  }
  
}


/************************************************************************/
/*                               trimL(string& s)                       */
/************************************************************************/

//! \return modified string ``s'' with spaces trimmed from left
std::string& trimL(std::string& s) 
{
  int pos(0);
  for ( ; s[pos]==' ' || s[pos]=='\t'; ++pos );
  s.erase(0, pos);
  return s;
}


/************************************************************************/
/*                               trimR(string& s)                       */
/************************************************************************/
//! \return modified string ``s'' with spaces trimmed from right
std::string& trimR(std::string& s) 
{
  int pos(s.size());
  for ( ; pos && s[pos-1]==' ' || s[pos]=='\t'; --pos );
  s.erase(pos, s.size()-pos);
  return s;
}


/************************************************************************/
/*                               trim(string& s)                       */
/************************************************************************/
// return a string trim by both edge spaces
std::string& trim(std::string& s) 
{
   return trimL(trimR ( s ) );
}


/************************************************************************/
/*                        char   *  getEnviVar(const char * name)      */
/************************************************************************/
char   *getEnviVariable(const char * name)
{
       char   *enviVar;   //environmental variable to get


       enviVar = getenv (name);
       if ( enviVar == NULL )
       {
          printf("  Error: environmental variable -- %s is not set.\n",name);
          exit (1);
       }
       return enviVar;
}   


/************************************************************************/
/*           string& processDirName(string& dirStr)                     */
/************************************************************************/
// make sure that directory ends with path separator
std::string& processDirName(std::string& dirStr)
{
   int      i;

   dirStr = trim( dirStr );
   i = dirStr.length() - 1;            //last char for the dir
    if ( ! (dirStr.at(i) == '\\' || dirStr.at(i) == '/')  )
    {
       if ( dirStr.find_last_of("\\")!=string::npos )
       {
          dirStr.append("\\");
       }
       else if ( dirStr.find_last_of("/")!=string::npos )
       {
         dirStr.append("/");
       }
       else
       {
         printf( "Error: directory has to end with path separator: %s\n",dirStr.c_str() );
         exit (1);
       }
    }

   return ( dirStr );
}


/************************************************************************/
/*           string& stringToUpper(string& str)                         */
/************************************************************************/
std::string&  stringToUpper(std::string& myString) 
{
  const int length = myString.length();
  for(int i=0; i<length; i++)
  {
    myString[i] = std::toupper(myString[i]);
  }
  return myString;
}



/***********************************************
* Get the value from PROJ.4 projection string  *
***********************************************/
float getValueInProj4Str(std::string proj4Str, const char *param)
{
     float    value;
     int      pos;
     

     if( (pos = proj4Str.find(param)) != string::npos )
     {
          proj4Str.erase(0,pos);         //get rid of everything before this parm

          pos = proj4Str.find("="); 
          proj4Str.erase(0,pos+1);       //get rid of parm name and =

          pos = proj4Str.find(" ");
          value = (float) atof( proj4Str.substr(0,pos).c_str() );
          //printf ("%s= %f\n", param,value );
     }
     else
     {
         value = PRJ_MISSIING_VALUE; 
         printf ("\tNo %s in GRID_PROJ - %s and set it to: %f\n", param, proj4Str.c_str(), value );
     } 

    return value;
}


/***********************************************
* Get the string from PROJ.4 projection string *
***********************************************/
std::string getStrInProj4Str(std::string proj4Str, const char *param)
{
     string    value;
     int       pos;
   

     if( (pos = proj4Str.find(param)) != string::npos )
     {
          proj4Str.erase(0,pos);         //get rid of everything before this parm

          pos = proj4Str.find("="); 
          proj4Str.erase(0,pos+1);       //get rid of parm name and =

          pos = proj4Str.find(" ");
          value =  proj4Str.substr(0,pos);
          //printf ("%s%s\n", param,value.c_str() );
     }
     else
     {
         printf ("\tNo %s in %s\n", param, proj4Str.c_str() );
     }

    return value;
}



/************************************************************************/
/*           char * convert2LatLongProjection (char *pszProj4_shp)      */
/************************************************************************/
char * convert2LatLongProjection (char *pszProj4_shp)
{
   
   string  projStr, projStr_geo;
   string parmStr;
   float  parmVal; 
   char   extStr[150];
   char   *proj4Chars;

   projStr = string (pszProj4_shp);

   parmStr = getStrInProj4Str(projStr, "+proj=");
   if ( parmStr.compare ("latlong") == 0 )
   {
      //the projection is already in lat and long
      projStr_geo = projStr;
   }
   else
   {
      //set a projection to latlong
      projStr_geo = string ( "+proj=latlong ");

      //get parm from input projection
      parmVal = getValueInProj4Str(projStr, "+a=");
      if ( parmVal != PRJ_MISSIING_VALUE )
      {
         sprintf(extStr, " +a=%.5f\0",parmVal);
         projStr_geo.append (extStr);
      }

      parmVal = getValueInProj4Str(projStr, "+b=");
      if ( parmVal != PRJ_MISSIING_VALUE )
      {
         sprintf(extStr, " +b=%.5f\0",parmVal);
         projStr_geo.append (extStr);
      }

      parmStr = getStrInProj4Str(projStr, "+datum=");
      if (! parmStr.empty() )
      {
         sprintf( extStr, " +datum=%s\0",parmStr.c_str() );
         projStr_geo.append (extStr);
      }

      parmStr = getStrInProj4Str(projStr, "+ellps=");
      if (! parmStr.empty() )
      {
         sprintf( extStr, " +ellps=%s\0",parmStr.c_str() );
         projStr_geo.append (extStr);
      }
   }

   proj4Chars = strdup ( projStr_geo.c_str() ); 

   return (proj4Chars);

}


/************************************************************************/
/*      Convert string with ',' to string vector                        */
/************************************************************************/
vector<string>  string2stringVector (string strVar, const char *sep)
{
   string temp_str = strVar;
   int pos;
   vector<string> strVector;

   temp_str = trim ( temp_str );

   while ( (pos = temp_str.find(sep)) != string::npos )
   {
       string strName = temp_str.substr(0,pos);
       strName = trim( strName );
       strVector.push_back ( strName );

       //printf ("\titem = %s\n", strName.c_str() );
       temp_str.erase(0,pos+1);

       //for space separated string
       temp_str = trim( temp_str );
   }
  
   //printf ("\titem = %s\n", temp_str.c_str() );
   strVector.push_back ( temp_str );

   return strVector;
}

/*****************************************/
/*      Check year is leap year or not   */
/*****************************************/
bool IsLeapYear( int year) {

   return ((year%400==0) || (year%4==0 && year%100!=0));

}

 
/*****************************************/
/*      return days in a month           */
/*****************************************/
int GetDaysInMonth ( int year,  int month) {

   switch (month) {
      case 2: return IsLeapYear(year) ? 29 : 28;
      case 4:
      case 6:
      case 9:
      case 11: return 30;
      default: return 31;
  }   

}


/***************************************************/
/*      convert 2 digits number to 2 digits chars   */
/***************************************************/
string convert2NumsTo2Chars (int num2)
{
    string   numStr;
    char     numChars[3];

    sprintf (numChars, "%d\0", num2);  
    if ( num2 < 10 )
    {
      numStr = string ( "0" );
      numStr.append( numChars );
    }
    else
    {
       numStr = string ( numChars );
    }
    
    return ( numStr );

}

/*****************************************/
/*      Check whether a step time passes */
/*      the end time                     */
/*****************************************/
bool isStepTimePassEndTime (string stepTime, string endTime)
{
    int    yearStep, yearEnd;
    int    monthStep, monthEnd; 
    int    dayStep, dayEnd;
    int    hourStep, hourEnd;
    int    minStep, minEnd;
    string   tmp_str;

    //get date and time for a step time
    tmp_str = stepTime.substr(0, 4);  //get year
    yearStep = atoi ( tmp_str.c_str() );
    tmp_str = stepTime.substr(4, 2);  //get month
    monthStep = atoi ( tmp_str.c_str() );
    tmp_str = stepTime.substr(6, 2);         //get day
    dayStep = atoi ( tmp_str.c_str() );
    tmp_str = stepTime.substr(8, 2);  //get hour
    hourStep = atoi ( tmp_str.c_str() );
    tmp_str =  stepTime.substr(10, 2);  //get minute
    minStep = atoi ( tmp_str.c_str() );
    //printf ("\t\t step time=%s  %d  %d  %d  %d  %d\n",stepTime.c_str(),yearStep,monthStep,dayStep,hourStep,minStep);

    //get date and time for the end time
    tmp_str = endTime.substr(0, 4);  //get year
    yearEnd = atoi ( tmp_str.c_str() );
    tmp_str = endTime.substr(4, 2);  //get month
    monthEnd = atoi ( tmp_str.c_str() );
    tmp_str = endTime.substr(6, 2);  //get day
    dayEnd = atoi ( tmp_str.c_str() );
    tmp_str = endTime.substr(8, 2);  //get hour
    hourEnd = atoi ( tmp_str.c_str() );
    tmp_str =  endTime.substr(10, 2);  //get minute
    minEnd = atoi ( tmp_str.c_str() );
    //printf ("\t\t end time=%s  %d  %d  %d  %d  %d\n",endTime.c_str(),yearEnd,monthEnd,dayEnd,hourEnd,minEnd);

    if ( yearStep > yearEnd )
    {
       return true;
    } 
    else if ( yearStep < yearEnd )
    {
       return false;
    }

    if ( monthStep > monthEnd )
    {
       return true;
    }
    else if ( monthStep < monthEnd )
    {
       return false;
    }

    if ( dayStep > dayEnd )
    {
       return true;
    }
    else if ( dayStep < dayEnd )
    {
       return false;
    }

    if ( hourStep > hourEnd )
    {
       return true;
    }
    else if ( hourStep < hourEnd )
    {
       return false;
    } 

    if ( minStep > minEnd )
    {
       return true;
    }
    else 
    {
       return false;
    }

}

/********************************************************************************/
/*      get new date and time string YYYYMMDDHHMM for start date/time string    */ 
/*      + minutes passed                                                        */
/********************************************************************************/
string getDateTimeStr (string startTime, int timeMinPassed)
{

   string   tmp_str;
   string   time_str,yearStr,monthStr,dayStr,hourStr,minStr;
   int      value; 
   int      add1 = 1;
   int      maxDays;

   //starting minutes and minutes to start the first step are the same
   if ( timeMinPassed == 0)
   {
      time_str = string ( startTime );
      return time_str;
   }


   //printf ("\t\tStart day/time = %s     timepassed = %d\n",startTime.c_str(),timeMinPassed);
   //obtain data from the date/time string

   yearStr = startTime.substr(0, 4);  //get year

   monthStr = startTime.substr(4, 2);  //get month

   dayStr = startTime.substr(6, 2);  //get day

   hourStr = startTime.substr(8, 2);  //get hour

   minStr =  startTime.substr(10, 2);  //get minute                
  

   //process minutes first 
   value = atoi( minStr.c_str() ) +  timeMinPassed;  //update minutes with minutes adding

   if ( value < 60 )
   {
      minStr = convert2NumsTo2Chars (value);
   }
   else 
   {
      minStr = convert2NumsTo2Chars (value - 60);
      value = atoi( hourStr.c_str() ) + add1;   //update hour
      if ( value < 24 )
      {
         hourStr = convert2NumsTo2Chars ( value ); 
      }
      else  
      { 
         hourStr = convert2NumsTo2Chars (value - 24);
         value = atoi( dayStr.c_str() ) + add1;   //update day 
         maxDays = GetDaysInMonth ( atoi(yearStr.c_str()), atoi(monthStr.c_str()) );
         if ( value <= maxDays )
         {
            dayStr = convert2NumsTo2Chars ( value );
         }
         else
         {
            dayStr = convert2NumsTo2Chars ( value - maxDays ); 
            value = atoi( monthStr.c_str() ) + add1;   //update month
            if (value <= 12 )
            {
              monthStr = convert2NumsTo2Chars ( value ); 
            } 
            else
            {
              monthStr = convert2NumsTo2Chars ( value - 12 );
              value = atoi( yearStr.c_str() ) + add1;   //update year
              yearStr = convert2NumsTo2Chars ( value );
            }  
         }  //month and year
      }  //day
   } // minute and houe
   
   //printf ("\t\tyearStr=%s    monthStr =%s   dayStr=%s   hourStr=%s  minStr=%s\n",yearStr.c_str(),monthStr.c_str(),dayStr.c_str(),hourStr.c_str(),minStr.c_str() );
   time_str = yearStr + monthStr + dayStr + hourStr + minStr;

 
   return time_str;
   
}


/************************************************************************************/
/*      Fill date, hour, minute range 2d arrays for YYYYMMDDHHMM start and end time */
/*      Return the number of time steps in the input date and time range            */
/************************************************************************************/
int  getDateTimeRange (string startTime, string endTime, int dRange[], int hRange[], int mRange[], int timeStep, int startMins)
{
   int      numTimeSteps = 0;
   string   tmp_str;
   int      minutesPass;
   string   datetimeStr;            //date and time string at the step
 
   //get date range
   tmp_str = startTime.substr(0, 8);
   dRange[0] = atoi ( tmp_str.c_str() );
   tmp_str = endTime.substr(0, 8);
   dRange[1] = atoi ( tmp_str.c_str() );
   
   //get hour range
   tmp_str = startTime.substr(8, 2);
   hRange[0] = atoi ( tmp_str.c_str() );
   tmp_str = endTime.substr(8, 2);
   hRange[1] = atoi ( tmp_str.c_str() );

   //get minutes range
   tmp_str = startTime.substr(10, 2);
   mRange[0] = atoi ( tmp_str.c_str() );
   tmp_str = endTime.substr(10, 2);
   mRange[1] = atoi ( tmp_str.c_str() );

   //set the first time step starting from startMins, compute minutes passed and new date/time string
   minutesPass = 0;
   if ( mRange[0] < startMins )
   {   
       minutesPass = startMins - mRange[0];  
   } 
   else if ( mRange[0] > startMins )
   {
      minutesPass = 60 - mRange[0] + startMins;
   }
   datetimeStr = getDateTimeStr (startTime, minutesPass);

   printf( "\tThe time step is %d minutes\n",timeStep);
   printf( "\tAssume that Satellite data are always at %d minutes after each hour\n",startMins);
   //printf( "\tThe start time step is: %s\n",datetimeStr.c_str() );
   
   while (! isStepTimePassEndTime (datetimeStr, endTime) )
   {
      numTimeSteps++;
      //printf ("\tnumTimeSteps = %d     datetimeStr = %s\n",numTimeSteps,datetimeStr.c_str() );
      datetimeStr = getDateTimeStr (datetimeStr, timeStep);
   }   


   if ( numTimeSteps == 0 )
   {
      printf ( "\t Error: Number of the time steps between input starting and ending times is: 0\n");
      exit ( 1 );
   }
   
   printf ("\tThe number of time steps for the time range is:%d\n",numTimeSteps);

   return numTimeSteps;

}


/************************************************************************/
/*      Fill time arrays for WRF Netcdf file output                     */
/************************************************************************/
map<string, int>  fillTimeArrays (float *times, char *timesStr, string startTime, string endTime, int timeStep, int startMins)
{
   int      numTimeSteps = 0;   //every timeStep minutes
   int      mRange0;            //starting minute input;
   string   tmp_str;
   string   time_str;
   long     minutesPass;
   string   datetimeStr;        //date and time string at the step
   map<string,int> timeIndexHash;
  
   //get starting minutes
   tmp_str = startTime.substr(10, 2);
   mRange0 = atoi ( tmp_str.c_str() );

   //set the first time step starting from startMins, compute minutes passed and new date/time string
   minutesPass = 0;
   if ( mRange0 < startMins )
   {   
       minutesPass = startMins - mRange0;
   }
   else if ( mRange0 > startMins )
   {
      minutesPass = 60 - mRange0 + startMins;
   }
   datetimeStr = getDateTimeStr (startTime, minutesPass);

   while (! isStepTimePassEndTime (datetimeStr, endTime) )
   {
      times[numTimeSteps] = minutesPass * 60.0;
   
      //printf ( "\t Fill time times=%f    step=%d     day time string=%s\n",times[numTimeSteps],numTimeSteps,datetimeStr.c_str() );
      tmp_str = datetimeStr.substr(0, 4);  //get year
      time_str = tmp_str; 
      time_str.append ( "-" );
       
      tmp_str = datetimeStr.substr(4, 2);  //get month
      time_str.append ( tmp_str );
      time_str.append ( "-" );

      tmp_str = datetimeStr.substr(6, 2);  //get day
      time_str.append ( tmp_str );
      time_str.append ( "_" );

      tmp_str = datetimeStr.substr(8, 2);  //get hour
      time_str.append ( tmp_str );
      time_str.append ( ":" );

      tmp_str = datetimeStr.substr(10, 2);  //get minute
      time_str.append ( tmp_str );
      time_str.append ( ":00" );

      strcat ( timesStr, time_str.c_str() );   //append the time string to time string array


      //file timeIndexHash table
      timeIndexHash[datetimeStr] = numTimeSteps;

      numTimeSteps++;
           
      minutesPass += timeStep;                 //accumulate minutes passed from the input starting date and time
      datetimeStr = getDateTimeStr (datetimeStr, timeStep);
   }   
  

   return timeIndexHash;
}

/***************************************/
/*      get next day string YYYYMMDD   */ 
/***************************************/
string getNextDayStr ( string startDayStr )
{

   string   tmp_str;
   string   time_str,yearStr,monthStr,dayStr;
   int      value; 
   int      add1 = 1;
   int      maxDays;

   //obtain data from the day string
   yearStr = startDayStr.substr(0, 4);  //get year
   monthStr = startDayStr.substr(4, 2);  //get month
   dayStr = startDayStr.substr(6, 2);  //get day
  
   //process day first 
   value = atoi( dayStr.c_str() ) + add1;   //update day 
   maxDays = GetDaysInMonth ( atoi(yearStr.c_str()), atoi(monthStr.c_str()) );
   if ( value <= maxDays )
   {
      dayStr = convert2NumsTo2Chars ( value );
   }
   else
   {
      dayStr = convert2NumsTo2Chars ( value - maxDays ); 
      value = atoi( monthStr.c_str() ) + add1;   //update month
      if (value <= 12 )
      {
         monthStr = convert2NumsTo2Chars ( value ); 
      } 
      else
      {
         monthStr = convert2NumsTo2Chars ( value - 12 );
         value = atoi( yearStr.c_str() ) + add1;   //update year
         yearStr = convert2NumsTo2Chars ( value );
       }  //month and year 
    }  // day
   
   //printf ("\t\tyearStr=%s    monthStr =%s   dayStr=%s \n",yearStr.c_str(),monthStr.c_str(),dayStr.c_str() );
   time_str = yearStr + monthStr + dayStr;
   return time_str;
   
}


/************************************************************************/
/*                               deleteStrNewLineChar (string& s)          */
/************************************************************************/
// return a string without new line character from getline string
std::string& deleteStrNewLineChar(std::string& s)
{
   int strLen;

   strLen = s.size();
   s.erase ( strLen-1, 1);

   return ( s ) ;
}

/***************************************************
* Get the WRF map projection type from PROJ4 string*
***************************************************/
int getProjType(string proj4Str)
{
     int type;

     if(  proj4Str.find("+proj=latlong") != string::npos )
     {
        //latitude and longitude projection
        type = LATLONG;
     }
     else if(  proj4Str.find("+proj=lcc") != string::npos )
     {
        //Lambert Conformal Conic
        type = LCC;
     }
     else if(  proj4Str.find("+proj=stere") != string::npos )
     {
        //Universal Polar Stereographic
        type = UPS;
     }
     else if(  proj4Str.find("+proj=merc") != string::npos )
     {
        //Mercator
        type = MERC;
     }
     else
     {
        printf("WRF only had four projection types: LATLONG, LCC, UPS, and MERC.\n");
        printf("Check GRID_PROJ definition in the script file.\n");
        exit ( 1 );
     }

     return type;
}


/*******************************************/
/*      get string YYYYDDD from YYYYMMDD   */
/*******************************************/
string getJulianDayStr ( string dateStr )
{
   string   julianStr;
   string   yearStr, monthStr, dayStr;
   int      totalDays;
   int      totalMonths;   //total months in this day string
   int      i;
    

   //obtain data from the day string
   yearStr = dateStr.substr(0, 4);  //get year
   monthStr = dateStr.substr(4, 2);  //get month
   dayStr = dateStr.substr(6, 2);  //get day

   totalDays = atoi( dayStr.c_str() );   //first get current month days

   totalMonths = atoi( monthStr.c_str() );
   //get days from all previous months
   for (i=1; i<totalMonths; i++)
   {
       totalDays += GetDaysInMonth ( atoi(yearStr.c_str()), i );
   }

   stringstream out;
   out << totalDays;

   string tmp_str = out.str();

   julianStr = yearStr;
   if ( tmp_str.size()  == 1 )
   {
      julianStr.append ( "00" );
   }
   else if ( tmp_str.size()  == 2 )
   {
      julianStr.append ( "0" );
   }

   julianStr.append ( tmp_str );

   return ( julianStr );

}


/***********************************/
/*      check day YYYYMMDD         */
/***********************************/
void checkYYYYMMDD (string dateStr)
{
   string   yearStr,monthStr,dayStr;
   int      yearNum, monthNum,dayNum;
   int      maxDays;
   int      i;


   //obtain data from the day string
   yearStr = dateStr.substr(0, 4);  //get year
   monthStr = dateStr.substr(4, 2);  //get month
   dayStr = dateStr.substr(6, 2);  //get day
   
   yearNum = atoi( yearStr.c_str() );
   monthNum = atoi( monthStr.c_str() ); 
   dayNum = atoi( dayStr.c_str() ); 

   if ( monthNum < 1 || monthNum > 12 )
   {
       printf ("\tNot correct month in YYYYMMDD day: %s\n", dayStr.c_str() );
       exit ( 1 );
   }

   maxDays = GetDaysInMonth ( yearNum, monthNum );
   if ( dayNum < 1 || dayNum > maxDays )
   {
       printf ("\tNot correct day in YYYYMMDD day: %s\n", dayStr.c_str() );
       exit ( 1 );
   }

}


float convertMCIPTimeStep2Mins ( int tStep )
{
   float    timeStepMins;     
   string   hh, mm, ss;
   int      hhNum, mmNum, ssNum;

   //convert time step to string
   stringstream out;

   out << tStep;
   string tStepStr = out.str();
     
   if ( tStepStr.size() >= 4 )
   {
      hh = tStepStr.substr( 0, tStepStr.size() - 4 );
      mm = tStepStr.substr( tStepStr.size() - 4, 2 );
      ss = tStepStr.substr( tStepStr.size() - 2, 2 );
   }
   else if ( tStepStr.size() >= 2 )
   {
      mm = tStepStr.substr( 0, tStepStr.size() - 2 );
      ss = tStepStr.substr( tStepStr.size() - 2, 2 );
   }
   else
   {
      ss = tStepStr.substr( 0, tStepStr.size() );
   }

   hhNum = 0; 
   mmNum = 0;
   ssNum = 0;
   if (hh.size() != 0 )
   {
      hhNum = atoi ( hh.c_str() );
   }
   if (mm.size() != 0 )
   {
      mmNum = atoi ( mm.c_str() );
   }
   if (ss.size() != 0 )
   {
      ssNum = atoi ( ss.c_str() );
   }

   //printf ("\thh=%s  mm=%s  ss=%s\n", hh.c_str(), mm.c_str(), ss.c_str() );

   timeStepMins = hhNum * 60.0 + mmNum + ssNum/60.0;

   return ( timeStepMins );

}


/***********************************/
/*      getSecondsFrom19930101     */
/***********************************/
double getSecondsFrom19930101( string startDateTime )
{

     string dayStartStr = startDateTime.substr(0, 8);
     string dayStartStr_julian =  getJulianDayStr ( dayStartStr );

     string yearStr = dayStartStr_julian.substr (0, 4); 
     string dayStr = dayStartStr_julian.substr (4, 3);
     string hhStr = startDateTime.substr(8, 2);
     string mmStr = startDateTime.substr(10, 2);;

     int yearNum = atoi ( yearStr.c_str() );
     int dayNum = atoi ( dayStr.c_str() );
     int hhNum = atoi ( hhStr.c_str() );
     int mmNum = atoi ( mmStr.c_str() );

     printf ( "\tdayStartStr_julian=%s  year=%d  days=%d   hours=%d   minutes=%d\n",dayStartStr_julian.c_str(), yearNum, dayNum, hhNum, mmNum );

     //currect year days before DDHHMM 
     int dayTotals = dayNum - 1;
     for (int i=1993; i<yearNum; i++)
     {
        for (int j=1; j<=12; j++)
        {
            dayTotals += GetDaysInMonth ( i, j);       
        }
     }

     double secondsTotal = dayTotals * 24.0 * 60.0 * 60.0 + hhNum * 60.0 * 60.0 + mmNum * 60.0;

     return secondsTotal;

}


/********************************************/
/*      Get YYYYMMDD from Juliane date      */
/********************************************/
string  getDateFromJulian ( string julianDate )
{
   string   dateStr;
   string   yearStr, monthStr, dayStr;
   int      totalDays;
   int      totalMonths;   //total months in this day string
   int      i;
   char     tmp_char[3];


   //obtain data from the day string
   yearStr = julianDate.substr(0, 4);  //get year
   dayStr = julianDate.substr(4, 3);  //get DDD

   totalDays = atoi( dayStr.c_str() );  

   totalMonths = 1;
   for (i=1; i<12; i++)
   {
       int monDays = GetDaysInMonth ( atoi(yearStr.c_str()), i );
       if ( totalDays > monDays)
       {
          totalMonths++; 
          totalDays = totalDays - monDays;
       }
       else
       {
          break;
       }
   }


   dateStr = yearStr;

   sprintf(tmp_char, "%d\0", totalMonths );
   monthStr = string ( tmp_char );

   sprintf(tmp_char, "%d\0", totalDays );
   dayStr = string ( tmp_char );

   if (monthStr.size()  == 1 )
   {
      dateStr.append ( "0" );
   }
   dateStr.append ( monthStr );
  

   if (dayStr.size()  == 1 )
   {
      dateStr.append ( "0" );
   }
   dateStr.append ( dayStr );


   return ( dateStr );

}


/*********************************************************
 *  Get files and dates to be processed:
 *  MODIS cloud 06 and location 03 file names
 *  MODIS aerosol 04 file names
 *  MODIS land products
 *  OMI AOD or NO2 L2 products
**********************************************************/
vector<string> obtainSatFileNames (string dataDir, string startDateTime, string endDateTime, string satType)
{
     DIR             *dpFiles;             //image data directory
     struct dirent   *dirpFiles;
     struct stat     stFileInfo;
     string          imageFileName, imageDate;
     vector<string>  fileNames_06,fileNames_03;
     string          dayStartStr,dayEndStr,dayMidStr;      //YYYYMMDD string
     string          dayStartStr_julian,dayEndStr_julian;  //YYYYDDD string - Julian string
     string          timeStartStr, timeEndStr;             //time strings
     long            dayTimeStart, dayTimeEnd, dayTimeMid; //day and time numbers
     string          tmp_str;

     int             i,j;


     printf ( "\nObtained satellite files to be processed...\n" );

     /**************************************
     * Get satellite files to be processed *
     ***************************************/
     //MODIS and MISR L2 cloud or aerosol products have YYYYDDD date, and OMI files have YYYYMMDD
     if ( satType.compare ("MODIS") == 0 )
     {
         
        dayStartStr = startDateTime.substr(0, 8);
        dayStartStr_julian =  getJulianDayStr ( dayStartStr );
        timeStartStr = startDateTime.substr(8, 4);
        dayStartStr = dayStartStr_julian + timeStartStr;

        //get last day
        dayEndStr = endDateTime.substr(0, 8);
        dayEndStr_julian = getJulianDayStr ( dayEndStr );
        timeEndStr = endDateTime.substr(8, 4);
        dayEndStr = dayEndStr_julian + timeEndStr;
     }
     else if ( satType.compare ("OMI") == 0 )
     {
        dayStartStr = startDateTime;
        dayEndStr = endDateTime;
     }
     else 
     {
        printf ( "\tSatellite type is not implemented.\n" );
        exit ( 1 );
     }

     dayTimeStart = atol ( dayStartStr.c_str() );
     dayTimeEnd = atol ( dayEndStr.c_str() );


     //MODIS satellite files to be processed: *06_L2*.* for 5kmX5km latlong and *03.A*.* for 1kmX1km geolocations
     //(e.g. MOD06_L2.A2006182.0035.005.2006249045012.hdf and MOD03.A2006182.0035.005.2008121043340.hdf or MYD*.*)
     //MODIS Landcover: 500m  MCD12Q1.A2006001.h09v04.005.2011230183453.hdf 
     //MODIS Landcover: 1km  MOD12Q1
     //MODIS LAIFPAR: MOD15A2GFS 1km: MOD15A2GFS.A2006001.h07v03.005.2009023132100.hdf
     //MODIS LAIFPAR: MOD15A2 1km: MOD15A2.A2006057.h10v06.005.2008077144707.hdf
     //MODIS LAIFPAR: MCD15A2 1km: MCD15A2.A2011089.h09v03.005.2011103004948.hdf
     //MODIS LAIFPAR: MCD15A2H 500m 8-day: MCD15A2H.A2016241.h15v03.006.2016250072611.hdf
     //MODIS albedo: MCD43A3 500km: MCD43A3.A2006177.h14v03.005.2008134144426.hdf
     //MODIS albedo: MCD43A1 500km: MCD43A1.A2006121.h08v04.005.2008117094839.hdf
     //MODIS albedo: MCD43A2 500km: MCD43A2.A2006121.h08v04.005.2008117094839.hdf

     //MISR files to be processed (e.g. MISR_AS_AEROSOL_F12_0022.A2006242.141906.D2989.C011.f841c4272.conv.hdf)

     //OMI satellite files to be processed (e.g. OMI-Aura_L2-OMAERUV_2006m0422t1726-o09414_v003-2009m0304t182047.he5 or
     //OMI-Aura_L2-OMNO2_2007m0630t2354-o15738_v003-2007m1217t191741.he5)
     
     if((dpFiles  = opendir( dataDir.c_str()) ) != NULL)
     {
        printf( "\tProcess images in directory: %s\n",dataDir.c_str() );
        while ((dirpFiles = readdir(dpFiles)) != NULL)
        {
           tmp_str = string(dirpFiles->d_name);
           tmp_str = trim(tmp_str);

           if ( ( ( tmp_str.find( "D06_L2.A" ) != string::npos || tmp_str.find( "D04_L2.A" ) != string::npos ||
                    tmp_str.find( "MISR_AS_AEROSOL_F12_0022" ) != string::npos ||
                    tmp_str.find( "MCD12Q1.A" ) != string::npos || tmp_str.find( "MOD12Q1.A" ) != string::npos || 
                    tmp_str.find( "MOD15A2GFS.A" ) != string::npos  || tmp_str.find( "MOD15A2.A" ) != string::npos ||
                    tmp_str.find( "MCD15A2.A" ) != string::npos || tmp_str.find( "MCD15A2H.A" ) != string::npos ||
                    tmp_str.find( "MCD43A3.A" ) != string::npos || tmp_str.find( "MCD43A1.A" ) != string::npos ||
                    tmp_str.find( "MCD43A2.A" ) != string::npos )  
                    && tmp_str.find( ".hdf" ) != string::npos ) ||
                ( tmp_str.find( "OMI-Aura_L2-OM" ) != string::npos && tmp_str.find( ".he5" ) != string::npos && 
                  tmp_str.find( ".he5.xml" ) == string::npos) )
           {
              imageDate = getDayTimeStrFromSatFileName ( tmp_str, satType );
              dayTimeMid = atol ( imageDate.c_str() );
              //printf ("\tdayTimeMid = %ld dayTimeStart=%ld   dayTimeEnd=%ld \n", dayTimeMid, dayTimeStart,dayTimeEnd );

              if ( dayTimeMid >= dayTimeStart && dayTimeMid < dayTimeEnd )
              {
                 imageFileName = dataDir + tmp_str;
                 //printf( "\tSatellite L2 file obtained: %s\n", imageFileName.c_str() );
                 fileNames_06.push_back ( imageFileName );

              }  //within the time
           } //MODIS, MISR, or OMI L2 products

           //get matching MODIS 03.A geolocation 1kmX1km file name
           if ( tmp_str.find( "D03.A" ) != string::npos && tmp_str.find( ".hdf" ) != string::npos )
           {
              imageDate = getDayTimeStrFromSatFileName ( tmp_str, satType );
              dayTimeMid = atol ( imageDate.c_str() );
              if ( dayTimeMid >= dayTimeStart && dayTimeMid < dayTimeEnd )
              {
                 imageFileName = dataDir + tmp_str;
                 //printf( "\tSatellite MODIS 03 L1 file obtained: %s\n", imageFileName.c_str() );
                 fileNames_03.push_back ( imageFileName );

              }  //within the time
           } //MODIS 03 L1 products for 1kmX1km geolocation

        }  //while dirpFiles

        closedir(dpFiles);
     }  // if dpFiles
     else
     {
        printf( "Directory does not exist: %s\n",dataDir.c_str() );
     }

     //sort satellite product files
     sort ( fileNames_06.begin(), fileNames_06.end() );

     //sort geolocation files for MODIS L1 
     if ( fileNames_03.size() != 0 )
     {
        sort ( fileNames_03.begin(), fileNames_03.end() );
     }

     //fill file names
     vector<string> fileNames;

     for ( i=0; i<fileNames_06.size(); i++)
     {
        //printf ("\tObtained Satellite L2 file Name: %s\n", fileNames_06[i].c_str() );

        string imageDate_06 = getDayTimeStrFromSatFileName ( fileNames_06[i], satType );
        //printf ("\t\tDateTime: %s\n", imageDate_06.c_str() );


        //store MODIS 06 file in one vector
        fileNames.push_back ( fileNames_06[i] );

        if (satType.compare ( "MODIS" ) == 0 )
        {
           if ( fileNames_03.size() != 0 )
           {
              //printf ("\tObtained MODIS 03 L1 Name: %s\n", fileNames_03[i].c_str() );
              string imageDate_03 = getDayTimeStrFromSatFileName ( fileNames_03[i], satType );
              if (imageDate_06.compare ( imageDate_03 ) != 0 )
              {
                 printf ( "\tError: MODIS 06 L2 file does not have matched MODIS 03 L2 file for date: %s\n", imageDate_06.c_str() );
                 printf ( "\tMatched MODIS 03 L1 file are needed for obtaining 1kmX1km geolocation data.\n");
                 exit ( 1 );
              }
 
              //store MODIS 03 file in one vector
              fileNames.push_back ( fileNames_03[i] );
           }
          
           //MODIS data file had julian date
           //convert Julian date to calendar date
           string dateStr = imageDate_06.substr (0,7);

           string timeStr = imageDate_06.substr (7,4);
           imageDate_06 =  getDateFromJulian (dateStr) + timeStr; 
        }

        fileNames.push_back ( imageDate_06 );
     }

     printf ( "\tSatellite files to be processed:\n" );
     for ( i=0; i<fileNames.size(); i++ )
     {
         printf ( "\t%s\n", fileNames[i].c_str() );
     }

     if ( fileNames.size() == 0 )
     {
        printf ( "\tError: Satellite file directory does not contain needed satellite files\n" );
        exit ( 1 );
     }

     return fileNames;
}


/**********************************************************
*  Get day and time string from satellite image file name *
***********************************************************/
string getDayTimeStrFromSatFileName ( string imageFileName, string satType )
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

     //satellite L2 cloud products
     if ( imageFileName.find( "D06_L2.A" ) != string::npos || 
          imageFileName.find( "D04_L2.A" ) != string::npos )  //MODIS MOD06/MOD04 or MYD06/MYD04 L2 cloud/aerosol products
     {
        dayStr = imageFileName.substr(10, 7);
        timeStr = imageFileName.substr(18, 4);
     }
     else if ( imageFileName.find( "MCD12Q1.A" ) != string::npos ||
          imageFileName.find( "MOD12Q1.A" ) != string::npos || imageFileName.find( "MOD15A2.A" ) != string::npos || 
          imageFileName.find( "MCD15A2.A" ) != string::npos ||
          imageFileName.find( "MCD43A3.A" ) != string::npos || imageFileName.find( "MCD43A1.A" ) != string::npos ||
          imageFileName.find( "MCD43A2.A" ) != string::npos )  //MODIS land cover products 500m and 1km
     {
        dayStr = imageFileName.substr(9, 7);
        timeStr = string ( "0000" ); //yearly products
     }
     else if ( imageFileName.find( "MCD15A2H.A" ) != string::npos )
     {
        dayStr = imageFileName.substr(10, 7);
        timeStr = string ( "0000" ); //yearly products
     }
     else if ( imageFileName.find( "MOD15A2GFS.A" ) != string::npos )  //filled MODIS LAI/FPAR 1km
     {
        dayStr = imageFileName.substr(12, 7);
        timeStr = string ( "0000" ); //yearly products
     }
     else if ( imageFileName.find( "MISR_AS_AEROSOL_F12_0022" ) != string::npos )  //MISR aerosol L2 products
     {
        dayStr = imageFileName.substr(26, 7);
        timeStr = imageFileName.substr(34, 4);
     }
     else if ( imageFileName.find( "D03.A" ) != string::npos ) //MODIS MOD03 or MYD03 L1 geolocation prodcuts
     {
        dayStr = imageFileName.substr(7, 7);
        timeStr = imageFileName.substr(15, 4);
     }
     else if ( imageFileName.find( "OMI-Aura_L2-OMAERUV" ) != string::npos ) //OMI L2 aerosol prodcuts
     {
        dayStr = imageFileName.substr(20, 9);
        dayStr.erase (4,1); //get rid of m in time: 2006m0422
        timeStr = imageFileName.substr(30, 4);
     }
     else if ( imageFileName.find( "OMI-Aura_L2-OMNO2" ) != string::npos ) //OMI L2 NO2 producuts
     {
        dayStr = imageFileName.substr(18, 9);
        dayStr.erase (4,1); //get rid of m in time: 2006m0422
        timeStr = imageFileName.substr(28, 4);
     }
     else
     {
         printf ("\tError: No matched Satellite file format.\n" );
         exit ( 1 );
     }


     dayTimeStr = dayStr + timeStr;
     //printf ("\tSatellite acquisition date is: %s\n", dayTimeStr.c_str() );

     if ( atol (dayTimeStr.c_str() ) == 0 )
     {
        printf ( "\tSatellite file name should be:\n" );
        printf ( "\t\tMOD06_L2.AYYYYDDD.HHMM.VVV.YYYYDDDHHMMSS.hdf or MYD06_L2.AYYYYDDD.HHMM.VVV.YYYYDDDHHMMSS.hdf\n" );
        printf ( "\t\tMOD03.AYYYYDDD.HHMM.VVV.YYYYDDDHHMMSS.hdf or MYD03.AYYYYDDD.HHMM.VVV.YYYYDDDHHMMSS.hdf\n" );
        printf ( "\t\tOMI-Aura_L2-OMAERUV_YYYYmMMDDtHHMM-o<Orbit>_v<Version>-<ProductionDateTime>.he5\n" );
        printf ( "\t\tOMI-Aura_L2-OMNO2_YYYYmMMDDtHHMM-o<Orbit>_v<Version>-<ProductionDateTime>.he5\n" );
        printf ( "\t\tMISR_AS_AEROSOL_F12_0022.A2006242.141906.D2989.C011.f841c4272.conv.hdf\n" );
        exit ( 1 );
     }

     return   dayTimeStr;

}


/*************************************************************
*  Compute minutes passed for a time from a start date/time  *
**************************************************************/
int getTimeMinutesPased (string dateStr, string startStr)
{

   int       startYearV, startMonV, startDayV, startHourV, startMinV;
   int       yearV, monV, dayV, hourV, minV;
   int       timeMinPassed = 0;

   char      tmp_char[5]; 


   //dateStr and startStr are the same
   if ( dateStr.compare ( startStr ) == 0 )
   {
      return timeMinPassed;
   }

   //printf ("\t\tStart Date string: %s    Date string: %s\n",startStr.c_str(),dateStr.c_str() );

   //obtain data from start date/time string
   startYearV = atoi ( startStr.substr(0, 4).c_str() );  //get year
   startMonV = atoi ( startStr.substr(4, 2).c_str() );  //get month
   startDayV = atoi ( startStr.substr(6, 2).c_str() );  //get day
   startHourV = atoi ( startStr.substr(8, 2).c_str() );  //get hour
   startMinV = atoi ( startStr.substr(10, 2).c_str() );  //get minute                
  

   //obtain data from the date/time string
   yearV = atoi ( dateStr.substr(0, 4).c_str() );  //get year
   monV = atoi ( dateStr.substr(4, 2).c_str() );  //get month
   dayV = atoi ( dateStr.substr(6, 2).c_str() );  //get day
   hourV = atoi ( dateStr.substr(8, 2).c_str() );  //get hour
   minV =  atoi ( dateStr.substr(10, 2).c_str() );  //get minute 

   //printf ("\tstart: %d %d %d %d %d  date: %d %d %d %d %d\n", startYearV,startMonV,startDayV,startHourV,startMinV,yearV,monV,dayV,hourV,minV);

   //process min  
   if ( minV >= startMinV )
   {
      timeMinPassed = minV - startMinV;
   }
   else
   {
      timeMinPassed = 60+ minV - startMinV;
      hourV = hourV - 1;
      if ( hourV < 0 )
      {
         hourV = 23;
         dayV = dayV - 1;
         if ( dayV < 1 )
         {
            monV = monV - 1;
            dayV =  GetDaysInMonth ( yearV, monV );
            if ( monV < 1 )
            {
               monV = 12;
               yearV = yearV - 1;
            } 
         }
      }
   }

   if ( hourV >= startHourV )
   {
      timeMinPassed += ( hourV - startHourV ) * 60; 
   }
   else
   {
      timeMinPassed += ( 24 + hourV - startHourV ) * 60;            
      dayV = dayV - 1;
      if ( dayV < 1 )
      {
         monV = monV - 1;
         dayV =  GetDaysInMonth ( yearV, monV );
         if ( monV < 1 )
         {
            monV = 12;
            yearV = yearV - 1;
         }
      }
   }

   sprintf (tmp_char, "%d\0", yearV); 
 
   string newDayStr = tmp_char + convert2NumsTo2Chars (monV) + convert2NumsTo2Chars (dayV);
   string startDayStr = startStr.substr ( 0, 8 );

   //printf ("\t\t newDayStr: %s  newStartDayStr: %s\n", newDayStr.c_str(), startDayStr.c_str() );

   long newDayV = atol ( newDayStr.c_str() ); 
   long startDateV = atol ( startDayStr.c_str() );

   string nexDayStr = startDayStr;

   while ( startDateV < newDayV )
   {
       timeMinPassed += 60 * 24;
       nexDayStr = getNextDayStr ( nexDayStr );    
       startDateV = atol ( nexDayStr.c_str() ); 
       //printf ( "\t\t nextday %s    startDateV: %ld \n", nexDayStr.c_str(), startDateV );
   }

   //printf ("\tMinutes passed: %d\n", timeMinPassed); 
   
   return timeMinPassed;
}



/************************************************************************************/
/*      convert date YYYYMMDD or YYYYMMDDHHMM to "0000-00-00_00:00:00" format       */
/************************************************************************************/
string  convertDate2OutputDateStr (string datetimeStr)
{

   string    time_str;
   string    tmp_str;

  
   tmp_str = datetimeStr.substr(0, 4);  //get year
   time_str = tmp_str; 
   time_str.append ( "-" );
       
   tmp_str = datetimeStr.substr(4, 2);  //get month
   time_str.append ( tmp_str );
   time_str.append ( "-" );

   tmp_str = datetimeStr.substr(6, 2);  //get day
   time_str.append ( tmp_str );
   time_str.append ( "_" );

   if ( datetimeStr.size() > 8 )
   {
      tmp_str = datetimeStr.substr(8, 2);  //get hour
      time_str.append ( tmp_str );
   }
   else
   {
      time_str.append ( "00" );
   }
   time_str.append ( ":" );


   if ( datetimeStr.size() > 10 )
   {
      tmp_str = datetimeStr.substr(10, 2);  //get minute
      time_str.append ( tmp_str );
   }
   else
   {
      time_str.append ( "00" );
   }

   //seconds
   time_str.append ( ":00" );

   return time_str;
} 


/**************************************/
/*  32. get grid ID from filename     */
/**************************************/
int  getGridIDFromFileName (string fileName, string fileType)
{
   int pos, gridID;
 
   //get rid of directory
   if ( (pos = fileName.find_last_of("/") ) != string::npos  )
   {
      fileName.erase(0,pos+1);       //get rid of dir
   }

   //get rid of extension
   if ( (pos = fileName.find_last_of(".") ) != string::npos  )
   {
      fileName.erase(pos, fileName.size() - pos);       //get rid of .*
   }

   //get the gridID from EPIC output file names 
   if ( fileType.compare ( "EPIC" ) == 0 )
   {
      //get gridID from fileName: gridID + BELD4CROP (2digits) + .NCS
      
      pos = fileName.size() - 2; 
      fileName.erase( pos, 2 );       //get rid of BELD crop IDs 2 digit at the end

      gridID = atoi ( fileName.c_str() ); 
   }

   //get the gridID from EPIC output average site ID
   if ( fileType.compare ( "EPICAVER" ) == 0 )
   {
      //get gridID from fileName: gridID + 0 + BELD4CROP (2digits)

      pos = fileName.size() - 3;
      fileName.erase( pos, 3 );       //get rid of BELD crop IDs 3 digit at the end

      gridID = atoi ( fileName.c_str() );
   }


   //get the gridID from EPIC output average site ID
   if ( fileType.compare ( "BIO_EPICAVER" ) == 0 )
   {
      //get gridID from fileName: gridID + BELD4CROP (2digits) + BIOFUL BELD4CROP (2digits) 

      pos = fileName.size() - 4;
      fileName.erase( pos, 4 );       //get rid of BELD crop IDs 4 digit at the end

      gridID = atoi ( fileName.c_str() );
   }

   return gridID;
}


/*************************************************************/
/*  33. convert string vector to string hash table (map)     */
/*************************************************************/
std::map<string,int> stringVector2stringMap ( std::vector<string> strV )
{
   std::map<string,int>  strH;
   int                   i;

   for (i=0; i<strV.size(); i++)
   {
      strH[strV[i]] = i;     
   } 

   return strH;
}


/******************************************************/
/*  33. convert string vector to char string array    */
/******************************************************/
char * stringVector2charString ( vector<string> namesV, const int textSize )
{
   char    *charStr;
   string  name;
   int     dimSize;
   int     i,j;

   dimSize = namesV.size();  

   //allocate memory for the char array
   if ( (charStr = (char *) calloc (dimSize*textSize+1, sizeof(char)) ) == NULL)
   {
      printf( "Calloc charStr failed.\n");
      exit ( 1 );
   }   
   
   for (i=0; i<dimSize; i++)
   {
      name = namesV[i];
      int nameSize = name.size();
      if ( nameSize > textSize )
      {
         //erase the extra chars
         name.erase ( textSize, nameSize-textSize); 
      }
      else //fill blank to make its size = textSize
      {
         for (j=0; j<textSize-nameSize; j++)
         {
           name.append (" ");
         }
      }
         
      //printf ("\t Long name = %d    ==%s==\n", name.size(), name.c_str() );
      strcat ( charStr, name.c_str() ); 
   } //i

   return charStr;

}


/******************************************/
/*  34. calloc float data array memory    */
/******************************************/
float * allocateFloatDataArrayMemory ( int  totalSize )
{
   float  *dataV;

   if ( (dataV = (float*) calloc (totalSize, sizeof(float)) ) == NULL)
   {
      printf( "Calloc float dataV failed.\n");
      exit ( 1 );
   }

   return dataV;

}


/*******************************************************/
/*  35. find a data file with name and date pattern    */
/*******************************************************/
string findOneDataFile ( string dataDir, string nameType, string dayMidStr, string dateStr6, string julianDateStr, 
                         string julianDateStr5 )
{
     DIR           *dp;        //data directory
     struct dirent *dpFiles;   //files

     int           i,j;
     string        dataFile;
     string        fileDateFormat;  
     string        tmp_str;

     //open data directory to get an data file with a certain date
     if((dp  = opendir(dataDir.c_str())) == NULL)
     {
        printf( "Error in opening directory: %s\n",dataDir.c_str() );
        exit ( 1 );
     }

     int firstDataFile = 0;
     j = 0;
     while ((dpFiles = readdir(dp)) != NULL)
     {
        tmp_str = string(dpFiles->d_name);
        tmp_str = trim(tmp_str);

        if ( tmp_str.find( nameType ) !=string::npos )
        {
           //get format
           if ( firstDataFile == 0 )
           {
               for (i=tmp_str.size()-1; i>=0; i--)
               {
                  if ( isdigit(tmp_str.at(i)) == 0 )  break;   //is not number
                  j++;
                  //printf ("\n\n i=%d  j=%d   %c\n",i,j,tmp_str.at(i));
               }

               if (j == 8 )  fileDateFormat = dayMidStr;
               if (j == 6 )  fileDateFormat = dateStr6;
               if (j == 7 )  fileDateFormat = julianDateStr;
               if (j == 5 )  fileDateFormat = julianDateStr5;
               firstDataFile = 1;
               //printf ("\n\n j=%d   Data file date format = %s\n",j,fileDateFormat.c_str() );
           }

           if (  tmp_str.find( fileDateFormat.c_str() ) !=string::npos )
           {
              //find match data file file
              dataFile = string( dataDir );
              dataFile.append( tmp_str );
              break;  //get out of loop
           }
        }    //end of find
     } 

     closedir(dp);
     if ( dataFile.empty() )
     {
        printf ( "\tError: Day %s does not have Data file - %s with YYYYMMDD, YYMMDD, YYYYDDD, or YYDDD\n", dayMidStr.c_str(), nameType.c_str() );
        exit ( 1 );
     }

     FileExists(dataFile.c_str(), 0 );  //the file has to exist.

     printf ( "\tFound daily file: %s\n", dataFile.c_str() );

     return dataFile;
}


/************************************************/
/*  36. Fill float array with MISSIING_VALUE    */
/************************************************/
void fillFloatArrayMissingValue ( int totalSize, float *varV )
{
     int  i;

     for ( i=0; i<totalSize; i++ )
     {
        //varV[i] = MISSIING_VALUE;
        varV[i] = MISSIING_VALUE_IOAPI;

     } //i
}



/******************************************/
/*  37. calloc int data array memory    */
/******************************************/
int * allocateIntDataArrayMemory ( int  totalSize )
{
   int  *dataV;

   if ( (dataV = (int*) calloc (totalSize, sizeof(int)) ) == NULL)
   {
      printf( "Calloc int dataV failed.\n");
      exit ( 1 );
   }

   return dataV;

}


/**************************************************/
/*  38. Fill int array with MISSIING_VALUE_INT    */
/**************************************************/
void fillIntArrayMissingValue ( int totalSize, int *varV )
{
     int  i;

     for ( i=0; i<totalSize; i++ )
     {
        varV[i] = MISSIING_VALUE_INT;
     } //i
}


/***************************************/
/*  39. find crop ID from file name    */
/***************************************/
int  getCropIDFromFileName (string fileName, string fileType)
{
   int pos, cropID;

   //get rid of directory
   if ( (pos = fileName.find_last_of("/") ) != string::npos  )
   {
      fileName.erase(0,pos+1);       //get rid of dir
   }

   //get rid of extension
   if ( (pos = fileName.find_last_of(".") ) != string::npos  )
   {
      fileName.erase(pos, fileName.size() - pos);       //get rid of .*
   }

   //get the cropID from EPIC output file names
   if ( fileType.compare ( "EPIC" ) == 0 || fileType.compare ( "EPICAVER" ) == 0 )
   {
      //get cropID from fileName: gridID + BELD4CROP (2digits)
      pos = fileName.size() - 2;
      fileName.erase( 0, pos );       //get rid of gridID

      cropID = atoi ( fileName.c_str() ); 
      //printf ( "\tCropID = %d\n", cropID);
   }

   //get the cropID from EPIC output file names
   if ( fileType.compare ( "BIO_EPICAVER" ) == 0 )
   {
      //get cropID from fileName: gridID + BELD4CROP (2digits) + allocation crop (2digits)
      pos = fileName.size() - 4;
     
      string temp_str = fileName.substr(pos, 2);   //get gridID

      cropID = atoi ( temp_str.c_str() );
      //printf ( "\tCropID = %d\n", cropID);
   }

   return cropID;
}


/***************************************/
/*  40. find soil ID from file name    */
/***************************************/
int  getSoilIDFromFileName (string fileName, string fileType)
{
   int pos, soilID;

   //get rid of directory
   if ( (pos = fileName.find_last_of("/") ) != string::npos  )
   {
      fileName.erase(0,pos+1);       //get rid of dir
   }

   //get rid of extension
   if ( (pos = fileName.find_last_of(".") ) != string::npos  )
   {
      fileName.erase(pos, fileName.size() - pos);       //get rid of dir
   }

   //get the soilID from EPIC output file names
   if ( fileType.compare ( "EPIC" ) == 0 )
   {
      //get soilID from fileName: STATE(2 letters)+SoilNUM
      fileName.erase( 0, 2 );       //get rid of State code

      soilID = atoi ( fileName.c_str() ); 
   }

   return soilID;
}


/***************************************************/
/*      41. Convert map key to string vector       */
/***************************************************/
vector<string>  mapKey2stringVector ( map<string, int>  mapVar )
{
   map<string,int>::iterator it;
   vector<string> strVector;

   for ( it=mapVar.begin() ; it != mapVar.end(); it++ )
   {
      strVector.push_back ( it->first );
   }

   return strVector;
}



/******************************************/
/*    42.  get next month string YYYYMM   */ 
/******************************************/
string getNextMonthStr ( string startMonStr )
{
   string   tmp_str;
   string   time_str,yearStr,monthStr;
   int      value; 
   int      add1 = 1;

   //obtain data from month string
   yearStr = startMonStr.substr(0, 4);  //get year
   monthStr = startMonStr.substr(4, 2);  //get month
  
   //process month first 
   value = atoi( monthStr.c_str() ) + add1;   //update day 
   if (value <= 12 )
   {
      monthStr = convert2NumsTo2Chars ( value ); 
   } 
   else
   {
      monthStr = convert2NumsTo2Chars ( value - 12 );
      value = atoi( yearStr.c_str() ) + add1;   //update year
      yearStr = convert2NumsTo2Chars ( value );
   }  //month and year 
   
   //printf ("\t\tyearStr=%s    monthStr =%s \n",yearStr.c_str(),monthStr.c_str() );
   time_str = yearStr + monthStr;

   return time_str;
   
}


/**************************************************************/
/*    43.  get time steps for year, month or day time steps   */
/**************************************************************/
int getTimeSteps ( string startDate, string endDate )
{
   int    YMD = 0; 
   int    timeSteps = 0;
 
   //obtain data from start date/time string
   if ( startDate.size() == 4 )
   {
      YMD = 1;
   }
   
   if ( startDate.size() == 6 )
   {
      YMD = 2;
   }

   if ( startDate.size() == 8 )
   {
      YMD = 3;
   }

   if ( YMD == 0 )
   {
      printf ("\tError: Date in getTimeSteps function is not YYYY, YYYYMM, YYYYMMDD.\n");
      exit ( 1 ); 
   }     

   //year time step
   if ( YMD == 1 )
   {
      timeSteps = atoi ( endDate.substr(0,4).c_str() ) - atoi ( startDate.substr(0,4).c_str() ) + 1; 
   }  
   else
   {
      long   stepTimeLong = atol ( startDate.c_str() );
      long   endTimeLong = atol ( endDate.c_str() );
      string stepTimeStr = startDate;

      while ( stepTimeLong <= endTimeLong )
      {
         if ( YMD == 2 )    stepTimeStr = getNextMonthStr ( stepTimeStr );
         if ( YMD == 3 )    stepTimeStr = getNextDayStr ( stepTimeStr ); 
         
         stepTimeLong = atol ( stepTimeStr.c_str() );
         timeSteps++;
      }
   }

   printf ("\ttimeSteps = %d\n", timeSteps );

  return timeSteps;
}


/*******************************************************/
/*    44.  fill fixed char array using string vector   */
/*******************************************************/
void fillCharArray ( int charLen, char *nameChar, vector<string> nameStr )
{

   int                   i, j;
   string                tmp_str;
   std::vector<string>   vecString;
   char                  temp_char[5];

   
   //printf ("\nCheck name string length: %d\n", charLen );

   for ( i=0; i<nameStr.size(); i++ )
   {
      tmp_str = nameStr[i];
 
      tmp_str = trim ( tmp_str );

      //process MODIS index if there is only MODIS land cover data
      if ( tmp_str.find( "MODIS" ) != string::npos && nameStr.size() == 20 )
      {
         vecString = string2stringVector (tmp_str, ".");
         string  temp = vecString[1];
         temp = trim ( temp ); 

         sprintf( temp_char,"%d. \0",i);
          
         tmp_str = string ( temp_char ) + temp;   //new index

      }

      int  tmpLen = tmp_str.size();

      if ( tmpLen > charLen )
      {
         tmp_str.erase (charLen, tmpLen - charLen );        
      }
      else if ( tmpLen < charLen )
      {
         for ( j=tmpLen; j<charLen; j++ )
         {
            tmp_str.append ( " " );   //fill space
         }
      }
   
      //printf ("%s\n", tmp_str.c_str() );

      strcat ( nameChar, tmp_str.c_str() );
   }  //i
   //printf ("%s\n", nameChar );

}


/****************************************/
/*    45.  Ckeck selection YES or NO    */
/****************************************/
void checkSelectionYesNo ( string sel )
{

   if ( sel.compare("YES") != 0 && sel.compare("NO") != 0)
   {
       printf( "  Error: Indicator - %s is not YES or NO.\n", sel.c_str()  );
       exit ( 1 );
   }

}


/****************************/
/*    46.  write_geogrid    */
/****************************/
/* File: write_geogrid.c

   Sample subroutine to write an array into the geogrid binary format.

   Side effects: Upon completion, a file named 00001-<NX>.00001-<NY> is
   created, where <NX> is the argument nx and <NY> is the argument ny,
   both in i5.5 format.

   Notes: Depending on the compiler and compiler flags, the name of
   the write_geogrid() routine may need to be adjusted with respect
   to the number of trailing underscores when calling from Fortran.

   Michael G. Duda, NCAR/MMM
*/
int write_geogrid(
      char  * fname,            /* file name to be created */
      float * rarray,          /* The array to be written */
      int * nx,                /* x-dimension of the array */
      int * ny,                /* y-dimension of the array */
      int * nz,                /* z-dimension of the array */
      int * isigned,           /* 0=unsigned data, 1=signed data */
      int * endian,            /* 0=big endian, 1=little endian */
      float * scalefactor,     /* value to divide array elements by before truncation to integers */
      int * wordsize )         /* number of bytes to use for each array element */
{
   int i, narray;
   int A2, B2;
   int A3, B3, C3;
   int A4, B4, C4, D4;
   unsigned int * iarray;
   unsigned char * barray;
//   char fname[24];
   FILE * bfile;

   narray = (*nx) * (*ny) * (*nz);

   iarray = (unsigned int *)malloc(sizeof(int) * narray);
   barray = (unsigned char *)malloc(sizeof(unsigned char) * narray * (*wordsize));

   /* Scale real-valued array by scalefactor and convert to integers */
   for (i=0; i<narray; i++)
      iarray[i] = (unsigned int)(rarray[i] / (*scalefactor));

   /*
      Set up byte offsets for each wordsize depending on byte order.
      A, B, C, D give the offsets of the LSB through MSB (i.e., for
      word ABCD, A=MSB, D=LSB) in the array from the beginning of a word
   */
   if (*endian == 0) {
      A2 = 0; B2 = 1;
      A3 = 0; B3 = 1; C3 = 2;
      A4 = 0; B4 = 1; C4 = 2; D4 = 3;
   }
   else {
      B2 = 0; A2 = 1;
      C3 = 0; B3 = 1; A3 = 2;
      D4 = 0; C4 = 1; B4 = 2; A4 = 3;
   }

   /* Place words into storage byte order */
   switch(*wordsize) {
      case 1:
         for(i=0; i<narray; i++) {
            if (iarray[i] < 0 && *isigned) iarray[i] += (1 << 8);
            barray[(*wordsize)*i] = (unsigned char)(iarray[i] & 0xff);
         }
         break;

      case 2:
         for(i=0; i<narray; i++) {
            if (iarray[i] < 0 && *isigned) iarray[i] += (1 << 16);
            barray[(*wordsize)*i+A2] = (unsigned char)((iarray[i] >> 8) & 0xff);
            barray[(*wordsize)*i+B2] = (unsigned char)( iarray[i]       & 0xff);
         }
         break;

      case 3:
         for(i=0; i<narray; i++) {
            if (iarray[i] < 0 && *isigned) iarray[i] += (1 << 24);
            barray[(*wordsize)*i+A3] = (unsigned char)((iarray[i] >> 16) & 0xff);
            barray[(*wordsize)*i+B3] = (unsigned char)((iarray[i] >> 8)  & 0xff);
            barray[(*wordsize)*i+C3] = (unsigned char)( iarray[i]        & 0xff);
         }
         break;

      case 4:
         for(i=0; i<narray; i++) {
            if (iarray[i] < 0 && *isigned) iarray[i] += (1 << 32);
            barray[(*wordsize)*i+A4] = (unsigned char)((iarray[i] >> 24) & 0xff);
            barray[(*wordsize)*i+B4] = (unsigned char)((iarray[i] >> 16) & 0xff);
            barray[(*wordsize)*i+C4] = (unsigned char)((iarray[i] >> 8)  & 0xff);
            barray[(*wordsize)*i+D4] = (unsigned char)( iarray[i]        & 0xff);
         }
         break;
   }

//   sprintf(fname,"%5.5i-%5.5i.%5.5i-%5.5i",1,*nx,1,*ny);

   /* Write array to file */
   bfile = fopen(fname,"wb");
   fwrite(barray,sizeof(unsigned char),narray*(*wordsize),bfile);
   fclose(bfile);

   free(iarray);
   free(barray);

   return 0;
}


/******************************************/
/*  47. calloc short data array memory    */
/******************************************/
short * allocateShortDataArrayMemory ( int  totalSize )
{
   short  *dataV;

   if ( (dataV = (short*) calloc (totalSize, sizeof(short)) ) == NULL)
   {
      printf( "Calloc int dataV failed.\n");
      exit ( 1 );
   }

   return dataV;

}

/*****************************************************/
/*  48. compare PROJ4 and GDAL projection strings    */
/*****************************************************/
bool comparePROJ4GDALPRJ (string prjPROJ4, string prjGDAL )
{
   string proj4Str, gdalStr;
   float  proj4Val, gdalVal;

   
   proj4Str = getStrInProj4Str(prjPROJ4, "+proj=");
   gdalStr = getStrInProj4Str(prjGDAL, "+proj=");
   if (proj4Str.compare ( gdalStr ) != 0 )
   {
      return false;
   }

   proj4Val = getValueInProj4Str(prjPROJ4, "+a=");
   gdalVal = getValueInProj4Str(prjGDAL, "+a=");
   if ( proj4Val != gdalVal )
   {
      return false;
   }
   
   proj4Val = getValueInProj4Str(prjPROJ4, "+b=");
   gdalVal = getValueInProj4Str(prjGDAL, "+b=");
   if ( proj4Val != gdalVal )
   {
      return false;
   }

   proj4Val = getValueInProj4Str(prjPROJ4, "+lat_1=");
   gdalVal = getValueInProj4Str(prjGDAL, "+lat_1=");
   if ( proj4Val != gdalVal )
   {
      return false;
   }

   proj4Val = getValueInProj4Str(prjPROJ4, "+lat_2=");
   gdalVal = getValueInProj4Str(prjGDAL, "+lat_2=");
   if ( proj4Val != gdalVal )
   {
      return false;
   }

   proj4Val = getValueInProj4Str(prjPROJ4, "+lat_0=");
   gdalVal = getValueInProj4Str(prjGDAL, "+lat_0=");
   if ( proj4Val != gdalVal )
   {
      return false;
   }
 
   proj4Val = getValueInProj4Str(prjPROJ4, "+lon_0=");
   gdalVal = getValueInProj4Str(prjGDAL, "+lon_0=");
   if ( proj4Val != gdalVal )
   {
      return false;
   }

   return true;
}


/*******************************************************************/
/*  49. Convert string vector to string with ','                   */
/*******************************************************************/
string  stringVector2string (vector<string> strV, const char *sep)
{
   string strVar;

   string tmp_str = string ( sep );

   strVar = strV[0];
   for ( int i=1; i<strV.size(); i++ )
   {
       strVar.append ( tmp_str+strV[i] );
   }

   return strVar;
}


/*******************************************************/
/*  50. Fill float array with MISSIING_VALUE missValue */
/*******************************************************/
void fillFloatArrayMissingValueVar ( int totalSize, float *varV, float missVal )
{
     int  i;

     for ( i=0; i<totalSize; i++ )
     {
        varV[i] = missVal;

     } //i
}


/*******************************/
/*  51. find day of week       */
/*******************************/
int   dayofweek( string  dateStr )
{
    int d, m, y;

    string yearStr = dateStr.substr(0, 4);
    y = atoi ( yearStr.c_str() );

    string monStr = dateStr.substr(4, 2);
    m = atoi ( monStr.c_str() );

    string dayStr = dateStr.substr(6, 2);
    d = atoi ( dayStr.c_str() );

    static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    y -= m < 3;

    int day = ( y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;

    if ( day == 0 )  day = 7;

    return day;
}
