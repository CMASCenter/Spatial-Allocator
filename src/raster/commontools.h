#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <algorithm>
#include <math.h>

using namespace std;

/******************************************
* Map projections supported by WRF:       *
*    LATLONG = Latitude-Longitude         *
*    LCC  = Lambert Conformal Conic       *
*    UPS  = Universal Polar Stereographic *
*    MERC = Mercator                      *
******************************************/
#define PI 3.14159265

#define LATLONG 6
#define LCC 1
#define UPS 2 
#define MERC 3

#define MISSIING_VALUE -999.0
#define MISSIING_VALUE_IOAPI  -9.999E+36
#define MISSIING_VALUE_INT -999
#define MISSIING_VALUE_UINT 999

//for write_geogrid
#ifdef _UNDERSCORE
#define write_geogrid write_geogrid_
#endif
#ifdef _DOUBLEUNDERSCORE
#define write_geogrid write_geogrid__
#endif


/*******************************************************
*                Utility prototypes                    *
*******************************************************/

void FileExists (const char *strFilename, int code);  //code = 0 existing file, code = 1 new file code = 2 existing dir
std::string& trimL ( std::string& s );
std::string& trimR ( std::string& s );
std::string& trim ( std::string& s );
char   *getEnviVariable(const char * name);
std::string& processDirName(std::string& dirStr);
std::string& stringToUpper(std::string& str);
float getValueInProj4Str(std::string proj4Str, const char *param);
std::string getStrInProj4Str(std::string proj4Str, const char *param);
char * convert2LatLongProjection (char *pszProj4_shp);
std::vector<std::string> string2stringVector (std::string strVar, const char *sep);
bool IsLeapYear( int year);
int GetDaysInMonth ( int year,  int month);
string convert2NumsTo2Chars (int num2);
bool isStepTimePassEndTime (string stepTime, string endTime);
string getDateTimeStr (string startTime, int timeMinPassed);
int  getDateTimeRange (string startTime, string endTime, int dRange[], int hRange[], int mRange[], int timeStep, int startMins);
map<string, int>  fillTimeArrays (float *times, char *timesStr, string startTime, string endTime, int timeStep, int startMins);
string getNextDayStr ( string startDayStr );
std::string& deleteStrNewLineChar(std::string& s);
int getProjType(string proj4Str);
string getJulianDayStr ( string dateStr );
void checkYYYYMMDD ( string dateStr );
void  getDayTimeStepRange ( int *dayTimeStepRange, int dayNum, int varID, int *timeVar, int var_ndims, int *varDimSize );
float convertMCIPTimeStep2Mins ( int tStep );
double getSecondsFrom19930101( string startDateTime );
string  getDateFromJulian ( string julianDate );
vector<string> obtainSatFileNames (string dataDir, string startDateTime, string endDateTime, string satType);
string getDayTimeStrFromSatFileName ( string imageFileName, string satType );
int getTimeMinutesPased (string dateStr, string startStr);
string  convertDate2OutputDateStr (string datetimeStr);
int     getGridIDFromFileName (string fileName, string fileType);
std::map<string,int> stringVector2stringMap ( std::vector<string> strV );
char * stringVector2charString ( vector<string> soilNamesV, const int textSize );
float * allocateFloatDataArrayMemory ( int  totalSize );
string findOneDataFile ( string dataDir, string nameType, string dayMidStr, string dateStr6, string julianDateStr,
                         string julianDateStr5 );
void fillFloatArrayMissingValue ( int totalSize, float *varV );
int * allocateIntDataArrayMemory ( int  totalSize );
void fillIntArrayMissingValue ( int totalSize, int *varV );
int  getCropIDFromFileName (string fileName, string fileType);
int  getSoilIDFromFileName (string fileName, string fileType);
vector<string>  mapKey2stringVector ( map<string, int>  mapVar );
string getNextMonthStr ( string startMonStr );
int getTimeSteps ( string startDate, string endDate );
void fillCharArray ( int charLen, char *nameChar, vector<string> nameStr );
void checkSelectionYesNo ( string sel );
int write_geogrid(
      char  * fname,           /* file name to be created */
      float * rarray,          /* The array to be written */
      int * nx,                /* x-dimension of the array */
      int * ny,                /* y-dimension of the array */
      int * nz,                /* z-dimension of the array */
      int * isigned,           /* 0=unsigned data, 1=signed data */
      int * endian,            /* 0=big endian, 1=little endian */
      float * scalefactor,     /* value to divide array elements by before truncation to integers */
      int * wordsize );        /* number of bytes to use for each array element */
short * allocateShortDataArrayMemory ( int  totalSize );
bool comparePROJ4GDALPRJ (string prjPROJ4, string prjGDAL );
string  stringVector2string (vector<string> strV, const char *sep);
void fillFloatArrayMissingValueVar ( int totalSize, float *varV, float missVal );
int   dayofweek( string  dateStr );
