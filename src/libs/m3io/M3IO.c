
/******************************************************************************
PURPOSE: M3IO.c - Define convenience routines for creating M3IO files.

NOTES:   For a description of M3IO Conventions see:
         http://www.baronams.com/products/ioapi/GRIDS.html

HISTORY: 2010/11/05 plessel.todd@epa.gov, Created.
******************************************************************************/

/*================================ INCLUDES =================================*/

#include <assert.h> /* For assert(). */
#include <stdio.h>  /* For stderr, fprintf(), perror(). */
#include <stdlib.h> /* For malloc(), free(). */
#include <string.h> /* For memset(). */
#include <limits.h> /* For ULONG_MAX. */
#include <float.h>  /* For FLT_MAX. */
#include <time.h>   /* For gmtime_r(). */

#include <netcdf.h> /* For nc_*. */

#include <M3IO.h> /* For public interface. */

/*================================== MACROS =================================*/

#ifndef NDEBUG
#define IS_BOOL( a ) ( (a) == 0 || (a) == 1 )
#define IN_RANGE(x,low,high) ((low)<=(x)&&(x)<=(high))
#define IMPLIES(p,c) (!(p)||(c))
#define IMPLIES_ELSE(p,c1,c2) (((p)&&(c1))||((!(p))&&(c2)))
#define AND2(a,b) ((a)&&(b))
#define AND3(a,b,c) ((a)&&(b)&&(c))
#define AND4(a,b,c,d) ((a)&&(b)&&(c)&&(d))
#define AND6(a,b,c,d,e,f) ((a)&&(b)&&(c)&&(d)&&(e)&&(f))
#define OR2(a,b) ((a)||(b))
#define IN3(x,a,b ) ((x)==(a)||(x)==(b))
#define IN4(x,a,b,c ) ((x)==(a)||(x)==(b)||(x)==(c))
#define SIGN(x) ((x) < 0.0 ? -1.0 : 1.0 )
#endif

#ifdef DEBUGGING
#define DEBUG(s) s
#else
#define DEBUG(s)
#endif

#define CLAMPED_TO_RANGE( value, low, high ) \
  ((value) < (low) ? (low) : (value) > (high) ? (high) : (value))

/* Allocate memory large enough to hold 'count' items of given 'type': */

#define NEW( type, count ) ((type*) newMemory( (count), sizeof (type) ))

/* Deallocate the memory and zero the pointer: */

#define FREE( p ) ( ( (p) ? freeMemory(p) : (void) 0 ), (p) = 0 )

/*================================== TYPES ==================================*/

/* From IOAPI Library: */

enum { TSTEP, DATE_TIME, LAY, VAR, ROW, COL, M3IO_DIMS };

enum {
  LATGRD3 = 1, LAMGRD3 = 2, MERGRD3 = 3, STEGRD3 = 4, UTMGRD3 = 5,
  POLGRD3 = 6, EQMGRD3 = 7, TRMGRD3 = 8, ALBGRD3 = 9, LEQGRD3 = 10
};

#define IS_VALID_PROJECTION(p) ((p) >= LATGRD3 && (p) <= LEQGRD3)

/*========================== FORWARD DECLARATIONS ===========================*/

static int createM3IOFile( const char* fileName,
                           int variables,
                           int timesteps,
                           int layers,
                           int rows,
                           int columns,
                           int yyyyddd,
                           double cellSize,
                           double westEdge,
                           double southEdge,
                           int projection,
                           double longitude0,
                           double latitude0,
                           double parameter1,
                           double parameter2,
                           double parameter3,
                           double topPressure,
                           const double sigmaLevels[],
                           const Name variableNames[],
                           const Name variableUnits[],
                           const Line variableDescriptions[],
                           const Name gridName,
                           const Line description );

static int writeDimensions( int file,
                            int timesteps,
                            int variables,
                            int layers,
                            int rows,
                            int columns,
                            int dimensionIds[ M3IO_DIMS ] );

static int writeVariables( int file,
                           int variables,
                           const Name variableNames[],
                           const Name variableUnits[],
                           const Line variableDescriptions[],
                           const int dimensionIds[ M3IO_DIMS ] );

static int writeAttributes( int file,
                            int timesteps,
                            int variables,
                            int layers,
                            int rows,
                            int columns,
                            int firstTimestamp,
                            int projection,
                            double cellSize,
                            double westEdge,
                            double southEdge,
                            double longitude0,
                            double latitude0,
                            double parameter1,
                            double parameter2,
                            double parameter3,
                            double topPressure,
                            const double sigmaLevels[],
                            const Name variableNames[],
                            const Name gridName,
                            const Description description );

static int writeVarListAttribute( int file,
                                  int variables,
                                  const Name variableNames[] );

static int writeTFLAG( int file, int variables, int timesteps,
                       int firstTimestamp );

/* NetCDF helper routines: */

static int createNetCDFFile( const char* fileName, int create64BitFile );

static int createDimensions( int file,
                             int count,
                             const char* const names[],
                             const int values[],
                             int ids[] );

static int createVariable( int file,
                           const char* name,
                           const char* units,
                           int type,
                           int dimensionality,
                           const int dimensionIds[] );

static int writeIntegerAttribute( int file, const char* name, int value );

static int writeRealAttribute( int file, int id, int type,
                               const char* name, double value );

static int writeTextAttribute( int file, int id, const char* name,
                               const char* value );

static int writeRealArrayAttribute( int file, int type,
                                    const char* name,
                                    const double values[], int count );

static int checkStatus( int status, const char* message );

/* Other helper routines: */

static int isValidDate( int yyyyddd );

static int isValidTime( int hhmmss );

static int isValidTimestepSize( int hhmmss );

static int isLeapYear( int yyyy );

static void incrementOneHour( int* yyyyddd, int* hhmmss );

static void incrementTime( int* yyyyddd, int* hhmmss, int step );

static long long nowUTC( void );

static void freeMemory( void* address );
  
static void* newMemory( size_t count, size_t sizeEach );

static void expandString( char* copy, const char* source, size_t length );

/*============================ PUBLIC FUNCTIONS =============================*/


/******************************************************************************
PURPOSE: createLonLatM3IOFile - Create a longitude-latitude grid file.
INPUTS:  const char* fileName  Name of NetCDF file to create.
         int variables         Number of data array variables.
         int timesteps         Number of timesteps of data.
         int layers            Number of layers of grid cells.
         int rows              Number of rows of grid cells.
         int columns           Number of columns of grid cells.
         int yyyyddd           First timestamp. E.g., 2010365.
         double cellSize       Size in degrees of grid cell width and height.
         double westEdge       Longitude of west  edge of grid
         double southEdge      Latitude  of south edge of grid
         double topPressure    Pressure (Pa) at the top of the model. 10000.
         const double sigmaLevels[ layers + 1 ] Sigma pressure levels. [1..0].
         const Name variableNames[ variables ]  Names of variables.
         const Name variableUnits[ variables ]  Units of variables.
         const Line variableUnits[ variables ]  Descriptions of variables.
         const Name gridName                    Name of the grid domain.
         const Description description          Description of the data.
RETURNS: int >= 0 if successful, else -1.
NOTES:   If unsuccessful then a failure message is printed to stderr.
         The resulting NetCDF file will also contain TFLAG variable.
******************************************************************************/

int createLonLatM3IOFile( const char* fileName,
                          int variables,
                          int timesteps,
                          int layers,
                          int rows,
                          int columns,
                          int yyyyddd,
                          double cellSize,
                          double westEdge,
                          double southEdge,
                          double topPressure,
                          const double sigmaLevels[],
                          const Name variableNames[],
                          const Name variableUnits[],
                          const Line variableDescriptions[],
                          const Name gridName,
                          const Description description ) {

  assert( cellSize < 90.0 );
  assert( IN_RANGE( westEdge, -180.0, 180.0 ) );
  assert( IN_RANGE( southEdge, -90.0,  90.0 ) );

  return createM3IOFile( fileName, variables, timesteps, layers, rows, columns,
                         yyyyddd, cellSize, westEdge, southEdge,
                         LATGRD3, 0.0, 0.0, 0.0, 0.0, 0.0,
                         topPressure, sigmaLevels,
                         variableNames, variableUnits, variableDescriptions,
                         gridName, description );
}



/******************************************************************************
PURPOSE: createLambertConformalM3IOFile - Create a Lambert Conformal grid file.
INPUTS:  const char* fileName  Name of NetCDF file to create.
         int variables         Number of data array variables.
         int timesteps         Number of timesteps of data.
         int layers            Number of layers of grid cells.
         int rows              Number of rows of grid cells.
         int columns           Number of columns of grid cells.
         int yyyyddd           First timestamp. E.g., 2010365.
         double cellSize       Size in meters of grid cell width and height.
         double westEdge       Distance in meters from longitude0 to west edge.
         double southEdge      Distance in meters from latitude0 to south edge.
         double longitude0     Longitude that projects to 0.
         double latitude0      Latitude  that projects to 0.
         double centralLongitude     Longitude of -Y-axis / center orientation.
         double lowerSecantLatitude  Lower secant latitude of projection plane.
         double upperSecantLatitude  Upper secant latitude of projection plane.
         double topPressure    Pressure (Pa) at the top of the model. 10000.
         const double sigmaLevels[ layers + 1 ] Sigma pressure levels. [1..0].
         const Name variableNames[ variables ]  Names of variables.
         const Name variableUnits[ variables ]  Units of variables.
         const Line variableUnits[ variables ]  Descriptions of variables.
         const Name gridName                    Name of the grid domain.
         const Description description          Description of the data.
RETURNS: int >= 0 if successful, else -1.
NOTES:   If unsuccessful then a failure message is printed to stderr.
         The resulting NetCDF file will also contain TFLAG variable.
******************************************************************************/

int createLambertConformalM3IOFile( const char* fileName,
                                    int variables,
                                    int timesteps,
                                    int layers,
                                    int rows,
                                    int columns,
                                    int yyyyddd,
                                    double cellSize,
                                    double westEdge,
                                    double southEdge,
                                    double longitude0,
                                    double latitude0,
                                    double centralLongitude,
                                    double lowerSecantLatitude,
                                    double upperSecantLatitude,
                                    double topPressure,
                                    const double sigmaLevels[],
                                    const Name variableNames[],
                                    const Name variableUnits[],
                                    const Line variableDescriptions[],
                                    const Name gridName,
                                    const Description description ) {

  assert( IN_RANGE( centralLongitude, -180.0, 180.0 ) );
  assert( SIGN( lowerSecantLatitude ) == SIGN( upperSecantLatitude ) );
  assert( OR2( IN_RANGE( lowerSecantLatitude, 1.0, 89.0 ),
               IN_RANGE( lowerSecantLatitude, -89.0, -1.0 ) ) );
  assert( OR2( IN_RANGE( upperSecantLatitude, lowerSecantLatitude, 89.0 ),
               IN_RANGE( upperSecantLatitude, lowerSecantLatitude, -1.0 ) ) );

  return createM3IOFile( fileName, variables, timesteps, layers, rows, columns,
                         yyyyddd, cellSize, westEdge, southEdge,
                         LAMGRD3, longitude0, latitude0,
                         lowerSecantLatitude,
                         upperSecantLatitude,
                         centralLongitude,
                         topPressure, sigmaLevels,
                         variableNames, variableUnits, variableDescriptions,
                         gridName, description );
}



/******************************************************************************
PURPOSE: createLambertEqualAreaM3IOFile - Create a Lambert Equal-Area grid file
INPUTS:  const char* fileName   Name of NetCDF file to create.
         int variables          Number of data array variables.
         int timesteps          Number of timesteps of data.
         int layers             Number of layers of grid cells.
         int rows               Number of rows of grid cells.
         int columns            Number of columns of grid cells.
         int yyyyddd            First timestamp. E.g., 2010365.
         double cellSize        Size in meters of grid cell width and height.
         double westEdge        Distance in meters from longitude0 to west edge
         double southEdge       Distance in meters from latitude0 to south edge
         double longitude0      Longitude that projects to 0.
         double latitude0       Latitude  that projects to 0.
         double centralLongitude Longitude of -Y-axis / center orientation.
         double centralLatitude  Latitude  of  X-axis / center orientation.
         double topPressure      Pressure (Pa) at the top of the model. 10000.
         const double sigmaLevels[ layers + 1 ] Sigma pressure levels. [1..0].
         const Name variableNames[ variables ]  Names of variables.
         const Name variableUnits[ variables ]  Units of variables.
         const Line variableUnits[ variables ]  Descriptions of variables.
         const Name gridName                    Name of the grid domain.
         const Description description          Description of the data.
RETURNS: int >= 0 if successful, else -1.
NOTES:   If unsuccessful then a failure message is printed to stderr.
         The resulting NetCDF file will also contain TFLAG variable.
******************************************************************************/

int createLambertEqualAreaM3IOFile( const char* fileName,
                                    int variables,
                                    int timesteps,
                                    int layers,
                                    int rows,
                                    int columns,
                                    int yyyyddd,
                                    double cellSize,
                                    double westEdge,
                                    double southEdge,
                                    double longitude0,
                                    double latitude0,
                                    double centralLongitude,
                                    double centralLatitude,
                                    double topPressure,
                                    const double sigmaLevels[],
                                    const Name variableNames[],
                                    const Name variableUnits[],
                                    const Line variableDescriptions[],
                                    const Name gridName,
                                    const Description description ) {

  assert( IN_RANGE( centralLongitude, -180.0, 180.0 ) );
  assert( IN_RANGE( centralLatitude,   -90.0,  90.0 ) );

  return createM3IOFile( fileName, variables, timesteps, layers, rows, columns,
                         yyyyddd, cellSize, westEdge, southEdge,
                         LEQGRD3, longitude0, latitude0,
                         centralLatitude,
                         0.0,
                         centralLongitude,
                         topPressure, sigmaLevels,
                         variableNames, variableUnits, variableDescriptions,
                         gridName, description );
}



/******************************************************************************
PURPOSE: createAlbersEqualAreaM3IOFile - Create a Albers Equal-Area grid file.
INPUTS:  const char* fileName   Name of NetCDF file to create.
         int variables          Number of data array variables.
         int timesteps          Number of timesteps of data.
         int layers             Number of layers of grid cells.
         int rows               Number of rows of grid cells.
         int columns            Number of columns of grid cells.
         int yyyyddd            First timestamp. E.g., 2010365.
         double cellSize        Size in meters of grid cell width and height.
         double westEdge        Distance in meters from longitude0 to west edge
         double southEdge       Distance in meters from latitude0 to south edge
         double longitude0      Longitude that projects to 0.
         double latitude0       Latitude  that projects to 0.
         double centralLongitude Longitude of -Y-axis / center orientation.
         double lowerSecantLatitude  Lower secant latitude of projection plane.
         double upperSecantLatitude  Upper secant latitude of projection plane.
         double topPressure     Pressure (Pa) at the top of the model. 10000.
         const double sigmaLevels[ layers + 1 ] Sigma pressure levels. [1..0].
         const Name variableNames[ variables ]  Names of variables.
         const Name variableUnits[ variables ]  Units of variables.
         const Line variableUnits[ variables ]  Descriptions of variables.
         const Name gridName                    Name of the grid domain.
         const Description description          Description of the data.
RETURNS: int >= 0 if successful, else -1.
NOTES:   If unsuccessful then a failure message is printed to stderr.
         The resulting NetCDF file will also contain TFLAG variable.
******************************************************************************/

int createAlbersEqualAreaM3IOFile( const char* fileName,
                                   int variables,
                                   int timesteps,
                                   int layers,
                                   int rows,
                                   int columns,
                                   int yyyyddd,
                                   double cellSize,
                                   double westEdge,
                                   double southEdge,
                                   double longitude0,
                                   double latitude0,
                                   double centralLongitude,
                                   double lowerSecantLatitude,
                                   double upperSecantLatitude,
                                   double topPressure,
                                   const double sigmaLevels[],
                                   const Name variableNames[],
                                   const Name variableUnits[],
                                   const Line variableDescriptions[],
                                   const Name gridName,
                                   const Description description ) {

  assert( IN_RANGE( centralLongitude, -180.0, 180.0 ) );
  assert( SIGN( lowerSecantLatitude ) == SIGN( upperSecantLatitude ) );
  assert( OR2( IN_RANGE( lowerSecantLatitude, 1.0, 89.0 ),
               IN_RANGE( lowerSecantLatitude, -89.0, -1.0 ) ) );
  assert( OR2( IN_RANGE( upperSecantLatitude, lowerSecantLatitude, 89.0 ),
               IN_RANGE( upperSecantLatitude, lowerSecantLatitude, -1.0 ) ) );

  return createM3IOFile( fileName, variables, timesteps, layers, rows, columns,
                         yyyyddd, cellSize, westEdge, southEdge,
                         ALBGRD3, longitude0, latitude0,
                         lowerSecantLatitude,
                         upperSecantLatitude,
                         centralLongitude,
                         topPressure, sigmaLevels,
                         variableNames, variableUnits, variableDescriptions,
                         gridName, description );
}



/******************************************************************************
PURPOSE: createPolarM3IOFile - Create a polar secant stereographic grid file.
INPUTS:  const char* fileName   Name of NetCDF file to create.
         int variables          Number of data array variables.
         int timesteps          Number of timesteps of data.
         int layers             Number of layers of grid cells.
         int rows               Number of rows of grid cells.
         int columns            Number of columns of grid cells.
         int yyyyddd            First timestamp. E.g., 2010365.
         double cellSize        Size in meters of grid cell width and height.
         double westEdge        Distance in meters from longitude0 to west edge
         double southEdge       Distance in meters from latitude0 to south edge
         double longitude0      Longitude that projects to 0.
         double latitude0       Latitude  that projects to 0.
         double centralLongitude Longitude of -Y-axis / center orientation.
         double secantLatitude  Secant latitude of projection plane.
         int isNorthern         1 = northern hemisphere, 0 = southern.
         double topPressure     Pressure (Pa) at the top of the model. 10000.
         const double sigmaLevels[ layers + 1 ] Sigma pressure levels. [1..0].
         const Name variableNames[ variables ]  Names of variables.
         const Name variableUnits[ variables ]  Units of variables.
         const Line variableUnits[ variables ]  Descriptions of variables.
         const Name gridName                    Name of the grid domain.
         const Description description          Description of the data.
RETURNS: int >= 0 if successful, else -1.
NOTES:   If unsuccessful then a failure message is printed to stderr.
         The resulting NetCDF file will also contain TFLAG variable.
******************************************************************************/

int createPolarM3IOFile( const char* fileName,
                         int variables,
                         int timesteps,
                         int layers,
                         int rows,
                         int columns,
                         int yyyyddd,
                         double cellSize,
                         double westEdge,
                         double southEdge,
                         double longitude0,
                         double latitude0,
                         double centralLongitude,
                         double secantLatitude,
                         int isNorthern,
                         double topPressure,
                         const double sigmaLevels[],
                         const Name variableNames[],
                         const Name variableUnits[],
                         const Line variableDescriptions[],
                         const Name gridName,
                         const Description description ) {

  assert( IN_RANGE( centralLongitude, -180.0, 180.0 ) );
  assert( IN_RANGE( secantLatitude,    -90.0,  90.0 ) );
  assert( IS_BOOL( isNorthern ) );

  return createM3IOFile( fileName, variables, timesteps, layers, rows, columns,
                         yyyyddd, cellSize, westEdge, southEdge,
                         POLGRD3, longitude0, latitude0,
                         isNorthern ? 1.0 : -1.0,
                         secantLatitude,
                         centralLongitude,
                         topPressure, sigmaLevels,
                         variableNames, variableUnits, variableDescriptions,
                         gridName, description );
}



/******************************************************************************
PURPOSE: createStereographicM3IOFile - Create a general stereographic grid file
INPUTS:  const char* fileName   Name of NetCDF file to create.
         int variables          Number of data array variables.
         int timesteps          Number of timesteps of data.
         int layers             Number of layers of grid cells.
         int rows               Number of rows of grid cells.
         int columns            Number of columns of grid cells.
         int yyyyddd            First timestamp. E.g., 2010365.
         double cellSize        Size in meters of grid cell width and height.
         double westEdge        Distance in meters from longitude0 to west edge
         double southEdge       Distance in meters from latitude0 to south edge
         double longitude0      Longitude that projects to 0.
         double latitude0       Latitude  that projects to 0.
         double centralLongitude Longitude of -Y-axis / center orientation.
         double secantLatitude  Secant latitude of projection plane.
         double topPressure     Pressure (Pa) at the top of the model. 10000.
         const double sigmaLevels[ layers + 1 ] Sigma pressure levels. [1..0].
         const Name variableNames[ variables ]  Names of variables.
         const Name variableUnits[ variables ]  Units of variables.
         const Line variableUnits[ variables ]  Descriptions of variables.
         const Name gridName                    Name of the grid domain.
         const Description description          Description of the data.
RETURNS: int >= 0 if successful, else -1.
NOTES:   If unsuccessful then a failure message is printed to stderr.
         The resulting NetCDF file will also contain TFLAG variable.
******************************************************************************/

int createStereographicM3IOFile( const char* fileName,
                                 int variables,
                                 int timesteps,
                                 int layers,
                                 int rows,
                                 int columns,
                                 int yyyyddd,
                                 double cellSize,
                                 double westEdge,
                                 double southEdge,
                                 double longitude0,
                                 double latitude0,
                                 double centralLongitude,
                                 double secantLatitude,
                                 double topPressure,
                                 const double sigmaLevels[],
                                 const Name variableNames[],
                                 const Name variableUnits[],
                                 const Line variableDescriptions[],
                                 const Name gridName,
                                 const Description description ) {

  assert( IN_RANGE( centralLongitude, -180.0, 180.0 ) );
  assert( IN_RANGE( secantLatitude,    -90.0,  90.0 ) );

  return createM3IOFile( fileName, variables, timesteps, layers, rows, columns,
                         yyyyddd, cellSize, westEdge, southEdge,
                         STEGRD3, longitude0, latitude0,
                         secantLatitude,
                         centralLongitude,
                         0.0,
                         topPressure, sigmaLevels,
                         variableNames, variableUnits, variableDescriptions,
                         gridName, description );
}



/******************************************************************************
PURPOSE: createEquitorialMercatorM3IOFile - Create a equitorial mercator file.
INPUTS:  const char* fileName   Name of NetCDF file to create.
         int variables          Number of data array variables.
         int timesteps          Number of timesteps of data.
         int layers             Number of layers of grid cells.
         int rows               Number of rows of grid cells.
         int columns            Number of columns of grid cells.
         int yyyyddd            First timestamp. E.g., 2010365.
         double cellSize        Size in meters of grid cell width and height.
         double westEdge        Distance in meters from longitude0 to west edge
         double southEdge       Distance in meters from latitude0 to south edge
         double longitude0      Longitude that projects to 0.
         double latitude0       Latitude  that projects to 0.
         double centralLongitude Longitude of -Y-axis / center orientation.
         double centralLatitude  Latitude  of  X-axis / center orientation.
         double topPressure     Pressure (Pa) at the top of the model. 10000.
         const double sigmaLevels[ layers + 1 ] Sigma pressure levels. [1..0].
         const Name variableNames[ variables ]  Names of variables.
         const Name variableUnits[ variables ]  Units of variables.
         const Line variableUnits[ variables ]  Descriptions of variables.
         const Name gridName                    Name of the grid domain.
         const Description description          Description of the data.
RETURNS: int >= 0 if successful, else -1.
NOTES:   If unsuccessful then a failure message is printed to stderr.
         The resulting NetCDF file will also contain TFLAG variable.
******************************************************************************/

int createEquitorialMercatorM3IOFile( const char* fileName,
                                      int variables,
                                      int timesteps,
                                      int layers,
                                      int rows,
                                      int columns,
                                      int yyyyddd,
                                      double cellSize,
                                      double westEdge,
                                      double southEdge,
                                      double longitude0,
                                      double latitude0,
                                      double centralLongitude,
                                      double centralLatitude,
                                      double topPressure,
                                      const double sigmaLevels[],
                                      const Name variableNames[],
                                      const Name variableUnits[],
                                      const Line variableDescriptions[],
                                      const Name gridName,
                                      const Description description ) {

  assert( IN_RANGE( centralLongitude, -180.0, 180.0 ) );
  assert( IN_RANGE( centralLatitude,   -90.0,  90.0 ) );

  return createM3IOFile( fileName, variables, timesteps, layers, rows, columns,
                         yyyyddd, cellSize, westEdge, southEdge,
                         EQMGRD3, longitude0, latitude0,
                         centralLatitude,
                         0.0,
                         centralLongitude,
                         topPressure, sigmaLevels,
                         variableNames, variableUnits, variableDescriptions,
                         gridName, description );
}



/******************************************************************************
PURPOSE: createTransverseMercatorM3IOFile - Create a transverse mercator file.
INPUTS:  const char* fileName   Name of NetCDF file to create.
         int variables          Number of data array variables.
         int timesteps          Number of timesteps of data.
         int layers             Number of layers of grid cells.
         int rows               Number of rows of grid cells.
         int columns            Number of columns of grid cells.
         int yyyyddd            First timestamp. E.g., 2010365.
         double cellSize        Size in meters of grid cell width and height.
         double westEdge        Distance in meters from longitude0 to west edge
         double southEdge       Distance in meters from latitude0 to south edge
         double longitude0      Longitude that projects to 0.
         double latitude0       Latitude  that projects to 0.
         double centralLongitude Longitude of -Y-axis / center orientation.
         double centralLatitude  Latitude  of  X-axis / center orientation.
         double topPressure     Pressure (Pa) at the top of the model. 10000.
         const double sigmaLevels[ layers + 1 ] Sigma pressure levels. [1..0].
         const Name variableNames[ variables ]  Names of variables.
         const Name variableUnits[ variables ]  Units of variables.
         const Line variableUnits[ variables ]  Descriptions of variables.
         const Name gridName                    Name of the grid domain.
         const Description description          Description of the data.
RETURNS: int >= 0 if successful, else -1.
NOTES:   If unsuccessful then a failure message is printed to stderr.
         The resulting NetCDF file will also contain TFLAG variable.
******************************************************************************/

int createTransverseMercatorM3IOFile( const char* fileName,
                                      int variables,
                                      int timesteps,
                                      int layers,
                                      int rows,
                                      int columns,
                                      int yyyyddd,
                                      double cellSize,
                                      double westEdge,
                                      double southEdge,
                                      double longitude0,
                                      double latitude0,
                                      double centralLongitude,
                                      double centralLatitude,
                                      double topPressure,
                                      const double sigmaLevels[],
                                      const Name variableNames[],
                                      const Name variableUnits[],
                                      const Line variableDescriptions[],
                                      const Name gridName,
                                      const Description description ) {

  assert( IN_RANGE( centralLongitude, -180.0, 180.0 ) );
  assert( IN_RANGE( centralLatitude,   -90.0,  90.0 ) );

  return createM3IOFile( fileName, variables, timesteps, layers, rows, columns,
                         yyyyddd, cellSize, westEdge, southEdge,
                         TRMGRD3, longitude0, latitude0,
                         centralLatitude,
                         0.0,
                         centralLongitude,
                         topPressure, sigmaLevels,
                         variableNames, variableUnits, variableDescriptions,
                         gridName, description );
}



/******************************************************************************
PURPOSE: createUTMM3IOFile - Create a universal transverse mercator file.
INPUTS:  const char* fileName   Name of NetCDF file to create.
         int variables          Number of data array variables.
         int timesteps          Number of timesteps of data.
         int layers             Number of layers of grid cells.
         int rows               Number of rows of grid cells.
         int columns            Number of columns of grid cells.
         int yyyyddd            First timestamp. E.g., 2010365.
         double cellSize        Size in meters of grid cell width and height.
         double westEdge        Distance in meters from longitude0 to west edge
         double southEdge       Distance in meters from latitude0 to south edge
         double longitude0      Longitude that projects to 0.
         double latitude0       Latitude  that projects to 0.
         int zone               UTM zone.
         double topPressure     Pressure (Pa) at the top of the model. 10000.
         const double sigmaLevels[ layers + 1 ] Sigma pressure levels. [1..0].
         const Name variableNames[ variables ]  Names of variables.
         const Name variableUnits[ variables ]  Units of variables.
         const Line variableUnits[ variables ]  Descriptions of variables.
         const Name gridName                    Name of the grid domain.
         const Description description          Description of the data.
RETURNS: int >= 0 if successful, else -1.
NOTES:   If unsuccessful then a failure message is printed to stderr.
         The resulting NetCDF file will also contain TFLAG variable.
******************************************************************************/

int createUTMM3IOFile( const char* fileName,
                       int variables,
                       int timesteps,
                       int layers,
                       int rows,
                       int columns,
                       int yyyyddd,
                       double cellSize,
                       double westEdge,
                       double southEdge,
                       double longitude0,
                       double latitude0,
                       int zone,
                       double topPressure,
                       const double sigmaLevels[],
                       const Name variableNames[],
                       const Name variableUnits[],
                       const Line variableDescriptions[],
                       const Name gridName,
                       const Description description ) {

  assert( IN_RANGE( zone, 1, 60.0 ) );

  return createM3IOFile( fileName, variables, timesteps, layers, rows, columns,
                         yyyyddd, cellSize, westEdge, southEdge,
                         UTMGRD3, longitude0, latitude0,
                         zone,
                         0.0,
                         0.0,
                         topPressure, sigmaLevels,
                         variableNames, variableUnits, variableDescriptions,
                         gridName, description );
}



/******************************************************************************
PURPOSE: createMercatorM3IOFile - Create a general mercator file.
INPUTS:  const char* fileName   Name of NetCDF file to create.
         int variables          Number of data array variables.
         int timesteps          Number of timesteps of data.
         int layers             Number of layers of grid cells.
         int rows               Number of rows of grid cells.
         int columns            Number of columns of grid cells.
         int yyyyddd            First timestamp. E.g., 2010365.
         double cellSize        Size in meters of grid cell width and height.
         double westEdge        Distance in meters from longitude0 to west edge
         double southEdge       Distance in meters from latitude0 to south edge
         double longitude0      Longitude that projects to 0.
         double latitude0       Latitude  that projects to 0.
         double centralLongitude Longitude of -Y-axis / center orientation.
         double centralLatitude  Latitude  of  X-axis / center orientation.
         double rotation         Tilt angle of rotation from North.
         double topPressure     Pressure (Pa) at the top of the model. 10000.
         const double sigmaLevels[ layers + 1 ] Sigma pressure levels. [1..0].
         const Name variableNames[ variables ]  Names of variables.
         const Name variableUnits[ variables ]  Units of variables.
         const Line variableUnits[ variables ]  Descriptions of variables.
         const Name gridName                    Name of the grid domain.
         const Description description          Description of the data.
RETURNS: int >= 0 if successful, else -1.
NOTES:   If unsuccessful then a failure message is printed to stderr.
         The resulting NetCDF file will also contain TFLAG variable.
******************************************************************************/

int createMercatorM3IOFile( const char* fileName,
                            int variables,
                            int timesteps,
                            int layers,
                            int rows,
                            int columns,
                            int yyyyddd,
                            double cellSize,
                            double westEdge,
                            double southEdge,
                            double longitude0,
                            double latitude0,
                            double centralLongitude,
                            double centralLatitude,
                            double rotation,
                            double topPressure,
                            const double sigmaLevels[],
                            const Name variableNames[],
                            const Name variableUnits[],
                            const Line variableDescriptions[],
                            const Name gridName,
                            const Description description ) {

  assert( IN_RANGE( centralLongitude, -180.0, 180.0 ) );
  assert( IN_RANGE( centralLatitude,   -90.0,  90.0 ) );
  assert( IN_RANGE( rotation, -90.0, 90.0 ) );

  return createM3IOFile( fileName, variables, timesteps, layers, rows, columns,
                         yyyyddd, cellSize, westEdge, southEdge,
                         MERGRD3, longitude0, latitude0,
                         centralLatitude,
                         centralLongitude,
                         rotation,
                         topPressure, sigmaLevels,
                         variableNames, variableUnits, variableDescriptions,
                         gridName, description );
}



/******************************************************************************
PURPOSE: closeM3IOFile - Close the M3IO file.
INPUTS:  int file  NetCDF file handle to close.
RETURNS: int 1 if unsuccessful, else 0 and a message is printed to stderr.
******************************************************************************/

int closeM3IOFile( int file ) {
  int result = 0;
  int status = 0;
  assert( file >= 0 );
  status = nc_close( file );
  result = checkStatus( status, "Problem closing NetCDF file" );
  assert( IS_BOOL( result ) );
  return result;
}



/******************************************************************************
PURPOSE: writeM3IOData - Write the data to the M3IO file.
INPUTS:  int file                  NetCDF file handle.
         const Name variableName   Name of variable to write.
         const float data[ timesteps * layers * rows * columns ] Data to write.
RETURNS: int 1 if unsuccessful, else 0 and a message is printed to stderr.
******************************************************************************/

int writeM3IOData( int file, const Name variableName, const float data[] ) {
  int result = 0;
  int id = -1;

  assert( file > -1 ); assert( variableName ); assert( *variableName );
  assert( data );

  if ( checkStatus( nc_inq_varid( file, variableName, &id ),
                    "Can't determine id of variable" ) ) {
    DEBUG( fprintf( stderr, "calling nc_put_var_float()...\n" ); )
    result =
      checkStatus( nc_put_var_float( file, id, data ), "Can't write variable");
    DEBUG( fprintf( stderr, "...done.\n" ); )
  }

  assert( IS_BOOL( result ) );
  return result;
}



/******************************************************************************
PURPOSE: writeM3IODataForTimestep - Write the data for a specified timestep
         to the M3IO file.
INPUTS:  int file                  NetCDF file handle.
         int timestep              0-based timestep number of data to write.
         const Name variableName   Name of variable to write.
         const float data[ layers * rows * columns ] Data to write.
RETURNS: int 1 if unsuccessful, else 0 and a message is printed to stderr.
******************************************************************************/

int writeM3IODataForTimestep( int file, int timestep, const Name variableName,
                              const float data[] ) {
  int result = 0;
  int variableId = -1;

  assert( file > -1 ); assert( timestep >= 0 );
  assert( variableName ); assert( *variableName );
  assert( data );

  if ( checkStatus( nc_inq_varid( file, variableName, &variableId ),
                    "Can't determine id of variable" ) ) {
    int dimensionIds[ NC_MAX_DIMS ];
    memset( dimensionIds, 0, sizeof dimensionIds );

    if ( checkStatus( nc_inq_vardimid( file, variableId, dimensionIds ),
                      "Can't determine variable dimension ids" ) ) {
      enum { VARIABLE_DIMENSIONS = 4 };
      size_t counts[ VARIABLE_DIMENSIONS ] = { 0, 0, 0, 0 };
      int dimension = 0;

      for ( dimension = 0; dimension < VARIABLE_DIMENSIONS; ++dimension ) {
        const int dimensionId = dimensionIds[ dimension ];

        if ( ! checkStatus(nc_inq_dimlen(file, dimensionId, counts +dimension),
                           "Can't determine variable dimensions size" ) ) {
          dimension = VARIABLE_DIMENSIONS;
        }
      }

      if ( dimension == VARIABLE_DIMENSIONS ) {
        size_t starts[ VARIABLE_DIMENSIONS ] = { 0, 0, 0, 0 };
        starts[ 0 ] = timestep;
        counts[ 0 ] = 1;
        DEBUG( fprintf( stderr, "calling nc_put_vara_float()...\n" ); )
        result =
          checkStatus( nc_put_vara_float( file, variableId, starts, counts,
                                          data ),
                       "Can't write subset of variable" );
        DEBUG( fprintf( stderr, "...done.\n" ); )
      }
    }
  }

  assert( IS_BOOL( result ) );
  return result;
}



/*============================ PRIVATE FUNCTIONS ============================*/



/******************************************************************************
PURPOSE: createM3IOFile - Create an M3IO file with the given data.
INPUTS:  const char* fileName  Name of NetCDF file to create.
         int variables         Number of data array variables.
         int timesteps         Number of timesteps of data.
         int layers            Number of layers of grid cells.
         int rows              Number of rows of grid cells.
         int columns           Number of columns of grid cells.
         int yyyyddd           First timestamp. E.g., 2010365.
         int projection        LAMGRD3 or POLGRD3.
         double cellSize       Size in meters of grid cell width and height.
         double westEdge       Distance from longitude0 to west  edge of grid.
         double southEdge      Distance from latitude0  to south edge of grid.
         int projection        Projection: LATGRD3, LAMGRD3, ... LEQGRD3.
         double longitude0     Longitude that projects to 0.
         double latitude0      Latitude  that projects to 0.
         double parameter1     Projection parameter 1.
         double parameter2     Projection parameter 2.
         double parameter3     Projection parameter 3.
         double topPressure    Pressure (Pa) at the top of the model. 10000.
         const double sigmaLevels[ layers + 1 ] Sigma pressure levels. [1..0].
         const Name variableNames[ variables ]  Names of variables.
         const Name variableUnits[ variables ]  Units of variables.
         const Line variableUnits[ variables ]  Descriptions of variables.
         const Name gridName         Name of the grid domain.
         const Line description      Description of the data.
RETURNS: int >= 0 if successful, else -1.
NOTES:   If unsuccessful then a failure message is printed to stderr.
         The resulting NetCDF file will contain TFLAG and one timestemp
         and one layer.
******************************************************************************/

static int createM3IOFile( const char* fileName,
                           int variables,
                           int timesteps,
                           int layers,
                           int rows,
                           int columns,
                           int yyyyddd,
                           double cellSize,
                           double westEdge,
                           double southEdge,
                           int projection,
                           double longitude0,
                           double latitude0,
                           double parameter1,
                           double parameter2,
                           double parameter3,
                           double topPressure,
                           const double sigmaLevels[],
                           const Name variableNames[],
                           const Name variableUnits[],
                           const Line variableDescriptions[],
                           const Name gridName,
                           const Line description ) {

  int result = -1;
  const long long fileSizeEstimate =
    (long long) variables * timesteps * layers * rows * columns * 4LL + 5000LL;
  const int create64BitFile = fileSizeEstimate > 2147483647LL;

  assert( fileName ); assert( *fileName );
  assert( timesteps > 0 );
  assert( IN_RANGE( variables, 1, MXVARS3 ) );
  assert( IN_RANGE( layers, 1, MXLAYS3 ) );
  assert( rows > 0 );
  assert( columns > 0 );
  assert( isValidDate( yyyyddd ) );
  assert( cellSize > 0.0 );
  assert( IS_VALID_PROJECTION( projection ) );
  assert( IN_RANGE( longitude0, -180.0, 180.0 ) );
  assert( IN_RANGE( latitude0, -90.0, 90.0 ) );
  assert( topPressure > 1.0 );
  assert( sigmaLevels );
  assert( IN_RANGE( sigmaLevels[ 0 ], 0.0, 1.0 ) );
  assert( IN_RANGE( sigmaLevels[ 1 ], 0.0, 1.0 ) );
  assert( IN_RANGE( sigmaLevels[ layers ], 0.0, 1.0 ) );
  assert( variableNames ); assert( *variableNames ); assert( **variableNames );
  assert( variableNames[ variables - 1 ] );
  assert( variableNames[ variables - 1 ][ 0 ] );
  assert( variableUnits ); assert( *variableUnits ); assert( **variableUnits );
  assert( variableUnits[ variables - 1 ] );
  assert( variableUnits[ variables - 1 ][ 0 ] );
  assert( variableDescriptions ); assert( *variableDescriptions );
  assert( **variableDescriptions );
  assert( variableDescriptions[ variables - 1 ] );
  assert( variableDescriptions[ variables - 1 ][ 0 ] );
  assert( gridName ); assert( *gridName );
  assert( description ); assert( *description );

  result = createNetCDFFile( fileName, create64BitFile );

  if ( result != -1 ) {
    static int dimensionIds[ M3IO_DIMS ] = { -1, -1, -1, -1, -1, -1 };
    int ok =
      writeDimensions( result, timesteps, variables, layers, rows, columns,
                       dimensionIds );

    ok = ok &&
      writeVariables( result, variables,
                      variableNames, variableUnits, variableDescriptions,
                      dimensionIds );

    ok = ok &&
      writeAttributes( result,
                       timesteps, variables, layers, rows, columns, yyyyddd,
                       projection,
                       cellSize, westEdge, southEdge,
                       longitude0, latitude0,
                       parameter1, parameter2, parameter3,
                       topPressure, sigmaLevels,
                       variableNames, gridName, description );

    if ( ok ) {
      DEBUG( int unused_ = fprintf( stderr, "calling nc_enddef()...\n" ); )
      const int status = nc_enddef( result ); /* SLOW! */
      DEBUG( fprintf( stderr, "...done.\n" ); )
      ok = checkStatus( status, "Can't end NetCDF definition" );
      ok = ok && writeTFLAG( result, variables, timesteps, yyyyddd );
    }

    if ( ! ok ) {
      closeM3IOFile( result );
      result = 0;
    }
  }

  assert( result >= -1 );
  return result;
}



/******************************************************************************
PURPOSE: writeDimensions - Write the dimensions for an M3IO file.
INPUTS:  int file           NetCDF file to write to.
         int timesteps      Number of timesteps.
         int variables      Number of variables.
         int layers         Number of layers.
         int rows           Number of rows.
         int columns        Number of columns.
OUTPUTS: int dimensionIds[ M3IO_DIMS ] Initialized NetCDF dimension handles.
                 dimensions[ TSTEP, DATE_TIME, LAY, VAR, ROW, COL ]
RETURNS: int 1 if successful, else 0.
******************************************************************************/

static int writeDimensions( int file,
                            int timesteps,
                            int variables,
                            int layers,
                            int rows,
                            int columns,
                            int dimensionIds[ M3IO_DIMS ] ) {

  const char* const names[ M3IO_DIMS ] = {
    "TSTEP", "DATE-TIME", "LAY", "VAR", "ROW", "COL"
  };
  int result = 0;
  int values[ M3IO_DIMS ] = { 0, 2, 0, 0, 0, 0 };

  assert( timesteps > 0 );
  assert( IN_RANGE( variables, 1, MXVARS3 ) );
  assert( IN_RANGE( layers, 1, MXLAYS3 ) );
  assert( rows > 0 );
  assert( columns > 0 );
  assert( dimensionIds );

  values[ TSTEP ] = timesteps > 1 ? NC_UNLIMITED : timesteps;
  values[ LAY   ] = layers;
  values[ VAR   ] = variables;
  values[ ROW   ] = rows;
  values[ COL   ] = columns;

  result = createDimensions( file, M3IO_DIMS, names, values, dimensionIds );

  if ( ! result ) {
    dimensionIds[ TSTEP     ] = -1;
    dimensionIds[ DATE_TIME ] = -1;
    dimensionIds[ LAY       ] = -1;
    dimensionIds[ VAR       ] = -1;
    dimensionIds[ ROW       ] = -1;
    dimensionIds[ COL       ] = -1;
  }

  assert( IS_BOOL( result ) );
  assert( IMPLIES_ELSE( result,
                        AND6( dimensionIds[ TSTEP     ] >= 0,
                              dimensionIds[ DATE_TIME ] >= 0,
                              dimensionIds[ LAY       ] >= 0,
                              dimensionIds[ VAR       ] >= 0,
                              dimensionIds[ ROW       ] >= 0,
                              dimensionIds[ COL       ] >= 0),
                        AND6( dimensionIds[ TSTEP     ] == -1,
                              dimensionIds[ DATE_TIME ] == -1,
                              dimensionIds[ LAY       ] == -1,
                              dimensionIds[ VAR       ] == -1,
                              dimensionIds[ ROW       ] == -1,
                              dimensionIds[ COL       ] == -1 ) ) );
  return result;
}



/******************************************************************************
PURPOSE: writeVariables - Write variable definitions and descriptions.
INPUTS:  int file                             NetCDF file to write to.
         int variables                        Number of variables.
         const Name variableNames[]           Names of variables.
         const Name variableUnits[]           Units of variables.
         const Name variableDescriptions[]    Descriptions of variables.
         const int dimensionIds[ M3IO_DIMS ]  Dimension ids.
RETURNS: int 1 if successful, else 0.
NOTES:   Alos writes definition of M3IO implicit variable TFLAG.
******************************************************************************/

static int writeVariables( int file,
                           int variables,
                           const Name variableNames[],
                           const Name variableUnits[],
                           const Line variableDescriptions[],
                           const int dimensionIds[ M3IO_DIMS ] ) {

  int result = 0;
  int variable = 0;
  int variableId = -1;
  int tempDimensionIds[ M3IO_DIMS ] = { -1, -1, -1, -1, -1, -1 };

  assert( file >= 0 );
  assert( IN_RANGE( variables, 1, MXVARS3 ) );
  assert( variableNames ); assert( *variableNames ); assert( **variableNames );
  assert( variableNames[ variables - 1 ] );
  assert( variableNames[ variables - 1 ][ 0 ] );
  assert( variableUnits ); assert( *variableUnits ); assert( **variableUnits );
  assert( variableUnits[ variables - 1 ] );
  assert( variableUnits[ variables - 1 ][ 0 ] );
  assert( variableDescriptions ); assert( *variableDescriptions );
  assert( **variableDescriptions );
  assert( variableDescriptions[ variables - 1 ] );
  assert( variableDescriptions[ variables - 1 ][ 0 ] );
  assert( dimensionIds ); assert( dimensionIds[ 0 ] >= 0 );

  /* Write implicit M3IO variable TFLAG: */

  tempDimensionIds[ 0 ] = dimensionIds[ TSTEP ];
  tempDimensionIds[ 1 ] = dimensionIds[ VAR ];
  tempDimensionIds[ 2 ] = dimensionIds[ DATE_TIME ];

  variableId =
    createVariable( file, "TFLAG", "<YYYYDDD,HHMMSS>", NC_INT, 3,
                    tempDimensionIds );

  result = variableId > -1;
  result = AND2( result, writeTextAttribute( file, variableId, "long_name",
                                             "TFLAG           " ) );
  result = AND2( result, writeTextAttribute( file, variableId, "var_desc",
                                             "Timestep-valid flags:  "
                                             "(1) YYYYDDD or (2) HHMMSS"
                                             "                         "
                                             "       " ) );

  /* Write named variables: */

  tempDimensionIds[ 1 ] = dimensionIds[ LAY ];
  tempDimensionIds[ 2 ] = dimensionIds[ ROW ];
  tempDimensionIds[ 3 ] = dimensionIds[ COL ];

  for ( variable = 0; AND2( result, variable < variables ); ++variable ) {
    Line desc = "";
    Name name = "";
    Name unit = "";
    memset( desc, 0, sizeof desc );
    memset( name, 0, sizeof name );
    memset( unit, 0, sizeof unit );
    strncpy( name, variableNames[ variable ], NAMLEN3 );
    strncpy( unit, variableUnits[ variable ], NAMLEN3 );
    strncpy( desc, variableDescriptions[ variable ], MXDLEN3 );

    variableId =
      createVariable( file, name, unit, NC_FLOAT, 4, tempDimensionIds );

    expandString( desc, desc, MXDLEN3 );
    expandString( name, name, NAMLEN3 );
    expandString( unit, unit, NAMLEN3 );

    result = variableId > -1;
    result = AND2( result,
                   writeTextAttribute( file, variableId, "long_name", name ) );
    result = AND2( result,
                   writeTextAttribute( file, variableId, "var_desc", desc ) );
  }

  assert( IS_BOOL( result ) );
  return result;
}



/******************************************************************************
PURPOSE: writeAttributes - Create an M3IO file atributes.
INPUTS:  int file              NetCDF file to write to.
         int timesteps         Number of timesteps of data.
         int variables         Number of data array variables.
         int layers            Number of layers of grid cells.
         int rows              Number of rows of grid cells.
         int columns           Number of columns of grid cells.
         int firstTimestamp    First timestamp. E.g., 2010365.
         int projection        LAMGRD3 or POLGRD3.
         double cellSize       Size in meters of grid cell width and height.
         double westEdge       Distance from longitude0 to west  edge of grid.
         double southEdge      Distance from latitude0  to south edge of grid.
         double longitude0     Longitude that projects to 0.
         double latitude0      Latitude  that projects to 0.
         double parameter1     Projection parameter 1.
         double parameter2     Projection parameter 2.
         double parameter3     Projection parameter 3.
         double topPressure       Pressure (Pa) at the top of the model. 10000.
         double sigmaLevels[ layers + 1 ] Sigma pressure levels. [1..0].
         const Name variableNames[ variables ]  Names of variables.
         const Name variableUnits[ variables ]  Units of variables.
         const Line variableUnits[ variables ]  Descriptions of variables.
         const Name gridName                    Name of the grid domain.
         const Description description          Description of the data.
RETURNS: int 1 if successful, else 0 and a message is printed to stderr.
******************************************************************************/

static int writeAttributes( int file,
                            int timesteps,
                            int variables,
                            int layers,
                            int rows,
                            int columns,
                            int firstTimestamp,
                            int projection,
                            double cellSize,
                            double westEdge,
                            double southEdge,
                            double longitude0,
                            double latitude0,
                            double parameter1,
                            double parameter2,
                            double parameter3,
                            double topPressure,
                            const double sigmaLevels[],
                            const Name variableNames[],
                            const Name gridName,
                            const Description description ) {

  int result = 0;
  const long long yyyydddhhmm = nowUTC();
  const int createdate = yyyydddhhmm / 10000; /* YYYYDDD. */
  const int createtime = yyyydddhhmm % 10000; /* HHMMSS.  */
  const int tstep = timesteps > 1 ? 10000 : 0; /* Hourly or time-invariant. */
  const char* const version = "1.0 1997349 (Dec. 15, 1997)";
  const char* const exec_id = "????????????????"
    "                                                                ";
  enum { FILE_DESCRIPTION_LENGTH = MXDESC3 * MXDLEN3 };
  char fileDescription[ FILE_DESCRIPTION_LENGTH + 1 ] = "";
  char name[ NAMLEN3 + 1 ] = "";

  assert( file >= 0 );
  assert( timesteps > 0 );
  assert( IN_RANGE( variables, 1, MXVARS3 ) );
  assert( IN_RANGE( layers, 1, MXLAYS3 ) );
  assert( rows > 0 );
  assert( columns > 0 );
  assert( isValidDate( firstTimestamp ) );
  assert( cellSize > 0.0 );
  assert( IS_VALID_PROJECTION( projection ) );
  assert( IN_RANGE( longitude0, -180.0, 180.0 ) );
  assert( IN_RANGE( latitude0, -90.0, 90.0 ) );
  assert( topPressure > 1.0 );
  assert( sigmaLevels );
  assert( IN_RANGE( sigmaLevels[ 0 ], 0.0, 1.0 ) );
  assert( IN_RANGE( sigmaLevels[ 1 ], 0.0, 1.0 ) );
  assert( IN_RANGE( sigmaLevels[ layers ], 0.0, 1.0 ) );
  assert( variableNames ); assert( *variableNames ); assert( **variableNames );
  assert( variableNames[ variables - 1 ] );
  assert( variableNames[ variables - 1 ][ 0 ] );
  assert( gridName ); assert( *gridName );
  assert( description ); assert( *description );

  memset( fileDescription, ' ', sizeof fileDescription );
  strncpy( fileDescription, description, FILE_DESCRIPTION_LENGTH );
  expandString( fileDescription, fileDescription, FILE_DESCRIPTION_LENGTH );
  assert( strlen( fileDescription ) == FILE_DESCRIPTION_LENGTH );

  memset( name, ' ', sizeof name );
  strncpy( name, name, NAMLEN3 );
  expandString( name, gridName, NAMLEN3 );
  assert( strlen( name ) == NAMLEN3 );

  result = writeTextAttribute( file, NC_GLOBAL, "IOAPI_VERSION", version );

  result = AND2( result,
                 writeTextAttribute( file, NC_GLOBAL, "EXEC_ID", exec_id ) );

  result = AND2( result, writeIntegerAttribute( file, "FTYPE", 1 ) );
  result = AND2( result, writeIntegerAttribute( file, "CDATE", createdate ) );
  result = AND2( result, writeIntegerAttribute( file, "CTIME", createtime ) );
  result = AND2( result, writeIntegerAttribute( file, "WDATE", createdate ) );
  result = AND2( result, writeIntegerAttribute( file, "WTIME", createtime ) );
  result = AND2( result, writeIntegerAttribute( file, "SDATE",firstTimestamp));
  result = AND2( result, writeIntegerAttribute( file, "STIME", 0 ) );
  result = AND2( result, writeIntegerAttribute( file, "TSTEP", tstep ) );
  result = AND2( result, writeIntegerAttribute( file, "NTHIK", 1 ) );
  result = AND2( result, writeIntegerAttribute( file, "NCOLS", columns ) );
  result = AND2( result, writeIntegerAttribute( file, "NROWS", rows ) );
  result = AND2( result, writeIntegerAttribute( file, "NLAYS", layers ) );
  result = AND2( result, writeIntegerAttribute( file, "NVARS", variables ) );
  result = AND2( result, writeIntegerAttribute( file, "GDTYP", projection ) );

  result = AND2( result, writeRealAttribute( file, NC_GLOBAL, NC_DOUBLE,
                                             "P_ALP", parameter1 ) );

  result = AND2( result, writeRealAttribute( file, NC_GLOBAL, NC_DOUBLE,
                                             "P_BET", parameter2 ) );

  result = AND2( result, writeRealAttribute( file, NC_GLOBAL, NC_DOUBLE,
                                             "P_GAM", parameter3 ) );

  result = AND2( result, writeRealAttribute( file, NC_GLOBAL, NC_DOUBLE,
                                             "XCENT", longitude0 ) );

  result = AND2( result, writeRealAttribute( file, NC_GLOBAL, NC_DOUBLE,
                                             "YCENT", latitude0 ) );

  result = AND2( result, writeRealAttribute( file, NC_GLOBAL, NC_DOUBLE,
                                             "XORIG", westEdge ) );

  result = AND2( result, writeRealAttribute( file, NC_GLOBAL, NC_DOUBLE,
                                             "YORIG", southEdge ) );

  result = AND2( result, writeRealAttribute( file, NC_GLOBAL, NC_DOUBLE,
                                             "XCELL", cellSize ) );

  result = AND2( result, writeRealAttribute( file, NC_GLOBAL, NC_DOUBLE,
                                             "YCELL", cellSize ) );

  result = AND2( result, writeIntegerAttribute( file, "VGTYP", 2 ) );

  result = AND2( result, writeRealAttribute( file, NC_GLOBAL, NC_FLOAT,
                                             "VGTOP", topPressure ) );

  result = AND2( result, writeRealArrayAttribute( file, NC_FLOAT, "VGLVLS",
                                                  sigmaLevels, layers + 1 ) );

  result = AND2( result, writeTextAttribute( file, NC_GLOBAL, "GDNAM",
                                             name ) );

  result = AND2( result, writeTextAttribute( file, NC_GLOBAL, "UPNAM",
                                             "                " ) );

  result = AND2( result,
                 writeVarListAttribute( file, variables, variableNames ) );

  result = AND2( result, writeTextAttribute( file, NC_GLOBAL, "FILEDESC",
                                             fileDescription ) );

  result = AND2( result, writeTextAttribute( file, NC_GLOBAL, "HISTORY",
                                             "                " ) );
  assert( IS_BOOL( result ) );
  return result;
}



/******************************************************************************
PURPOSE: writeVarListAttribute - Write the value of the VAR-LIST attribute.
INPUTS:  int file                    NetCDF file ID.
         int variables               Number of subset variables.
         const Name variableNames[ variables ]  Names of variables.
RETURNS: int 1 if successful, else 0 and a message is printed to stderr.
******************************************************************************/

static int writeVarListAttribute( int file,
                                  int variables,
                                  const Name variableNames[] ) {

  int result = 0;
  char attribute[ 10 * MXVARS3 * ( NC_MAX_NAME + 1 ) ] = "";
  int variable = 0;
  int length = 0;

  assert( file >= 0 );
  assert( IN_RANGE( variables, 1, MXVARS3 ) );
  assert( variableNames ); assert( *variableNames ); assert( **variableNames );
  assert( variableNames[ variables - 1 ] );
  assert( variableNames[ variables - 1 ][ 0 ] );

  memset( attribute, 0, sizeof attribute );

  do {
    const char* const variableName = variableNames[ variable ];
    assert( length < sizeof attribute / sizeof *attribute );
    expandString( attribute + length, variableName, NAMLEN3 );
    length += NAMLEN3;
    ++variable;
  } while ( variable < variables );

  result = checkStatus( nc_put_att_text( file, NC_GLOBAL, "VAR-LIST",
                                         strlen( attribute ), attribute ),
                       "Can't write text attribute VAR-LIST" );

  assert( IS_BOOL( result ) );
  return result;
}



/******************************************************************************
PURPOSE: writeTFLAG - Write the TFLAG variable.
INPUTS:  int file           NetCDF file to write to.
         int variables      Number of variables in variableNames array.
         int timesteps      Number of timesteps.
         int firstTimestamp  First timestamp. E.g., 2010365.
RETURNS: int 1 if successful, else 0 and failureMessage() is called.
******************************************************************************/

static int writeTFLAG( int file,
                       int variables,
                       int timesteps,
                       int firstTimestamp ) {

  int result = 0;
  int id = -1;
  
  assert( file >= 0 );
  assert( timesteps > 0 );
  assert( IN_RANGE( variables, 1, MXVARS3 ) );
  assert( isValidDate( firstTimestamp ) );

  if ( checkStatus( nc_inq_varid( file, "TFLAG", &id ),
                   "Can't determine id of variable TFLAG" ) ) {
    const int count = timesteps * variables * 2;
    int* data = NEW( int, count );

    if ( data ) {
      const int tstep = 10000;
      int yyyyddd = firstTimestamp;
      int hhmmss  = 0;
      int* dateTime = data;
      int timestep = 0;

      /*
       * For each timestep, advance the timestamp
       * and replicate it for each variable:
       */

      do {
        int variable = 0;
        assert( yyyyddd > 0 ); assert( hhmmss >= 0 );

        do {
          *dateTime++ = yyyyddd;
          *dateTime++ = hhmmss;
          ++variable;
        } while ( variable < variables );

        incrementTime( &yyyyddd, &hhmmss, tstep );
        ++timestep;
      } while ( timestep < timesteps );

      {
        size_t starts[ 3 ] = { 0, 0, 0 }; /*NetCDF BUG: nc_put_var_int no-op!*/
        size_t counts[ 3 ] = { 0, 0, 2 };
        counts[ 0 ] = timesteps;
        counts[ 1 ] = variables;
        result = checkStatus( nc_put_vara_int( file, id, starts, counts, data),
                              "Can't write TFLAG variable" );
      }

      FREE( data );
    }
  }

  assert( IS_BOOL( result ) );
  return result;
}



/******************************************************************************
PURPOSE: createNetCDFFile - Create a NetCDFFile for writing.
INPUTS:  const char* fileName  Name of file to create.
         int create64BitFile   Create 64-bit NetCDF file?
RETURNS: int NetCDF file ID if successful, else -1 if failed and a message is
         printed to stderr.
******************************************************************************/

static int createNetCDFFile( const char* fileName, int create64BitFile ) {

  int result = -1;
  const int mode = create64BitFile ? NC_CLOBBER | NC_64BIT_OFFSET : NC_CLOBBER;
  int ncid = -1;

  assert( fileName ); assert( *fileName );
  assert( IS_BOOL( create64BitFile ) );

  if ( AND2( checkStatus( nc_create( fileName, mode, &ncid ),
                          "Can't create NetCDF file" ), ncid >= 0 ) ) {
    result = ncid;
  }

  assert( result >= -1 );
  return result;
}



/******************************************************************************
PURPOSE: createDimensions - Create the given named dimensions of a NetCDF file.
INPUTS:  int file                          NetCDF file ID.
         int count                         Number of dimensions.
         const char* const names[ count ]  Name of each dimension.
         const int values[ count ]         Value of each dimension.
OUTPUTS: int ids[ count ] >             Ids of each dimension or -1 if failed.
RETURNS: int 1 if successful, else 0 and a message is printed to stderr.
******************************************************************************/

static int createDimensions( int file,
                             int count,
                             const char* const names[],
                             const int values[],
                             int ids[] ) {

  int result = 0;
  int index = 0;

  assert( file >= 0 ); assert( count > 0 );
  assert( names ); assert( *names ); assert( names[ count - 1 ] );
  assert( values ); assert( *values >= 0 ); assert( values[ count - 1 ] > 0 );
  assert( ids );

  do {
    int id = -1;
    
    if ( ! checkStatus( nc_def_dim( file, names[ index ], values[ index ],
                                    &id ),
                        "Can't create dimension" ) ) {
      index = count;
    } else {
      ids[ index ] = id;
    }

    ++index;
  } while ( index < count );

  result = index == count;

  if ( ! result ) {
    memset( ids, -1, count * sizeof *ids );
  }

  assert( IS_BOOL( result ) );
  assert( IMPLIES_ELSE( result,
                        AND2( ids[ 0 ] >=  0, ids[ count - 1 ] >=  0 ),
                        AND2( ids[ 0 ] == -1, ids[ count - 1 ] == -1 ) ) );

  return result;
}



/******************************************************************************
PURPOSE: createVariable - Create the given named variable of a NetCDF file.
INPUTS:  int file                 NetCDF file ID.
         const char* name         Name of variable.
         const char* units        Units of variable.
         int type                 Type of variable.
         int dimensionality       Number of dimensions.
         const int dimensionIds[ dimensionality ]  Ids of dimensions.
RETURNS: int id if successful, else -1 and a message is printed to stderr.
******************************************************************************/

static int createVariable( int file,
                           const char* name,
                           const char* units,
                           int type,
                           int dimensionality,
                           const int dimensionIds[] ) {

  int result = -1;
  int id = -1;

  assert( file >= 0 );
  assert( name ); assert( *name ); assert( units ); assert( *units );
  assert( IN4( type, NC_INT, NC_FLOAT, NC_DOUBLE ) );
  assert( dimensionality >= 1 );
  assert( dimensionIds );
  assert( OR2( dimensionIds[ 0 ] == NC_UNLIMITED, dimensionIds[ 0 ] >= 0 ) );

  DEBUG( fprintf( stderr, "createVariable( %d, '%s', '%s', %d, %d, %d )\n",
                  file, name, units, type, dimensionality,
                  dimensionIds[ 0 ] ) );

  if ( checkStatus( nc_def_var( file, name, type, dimensionality,
                                dimensionIds, &id ),
                   "Can't create variable" ) ) {
    if ( writeTextAttribute( file, id, "units", units ) ) {
      result = id;
    }
  }

  assert( result >= -1 );
  return result;
}



/******************************************************************************
PURPOSE: writeIntegerAttribute - Write the value of a global integer attribute
         to a NetCDF file.
INPUTS:  int file          NetCDF file ID.
         const char* name  Name of attribute.
         int value         Value for attribute.
RETURNS: int 1 if successful, else 0 and a message is printed to stderr.
******************************************************************************/

static int writeIntegerAttribute( int file, const char* name, int value ) {
  int result = 0;
  assert( file >= 0 ); assert( name ); assert( *name );

  result =
    checkStatus( nc_put_att_int( file, NC_GLOBAL, name, NC_INT, 1, &value ),
                 "Can't write value of attribute" );

  assert( IS_BOOL( result ) );
  return result;
}



/******************************************************************************
PURPOSE: writeRealAttribute - Write the value of a real attribute to a
         NetCDF file.
INPUTS:  int file          NetCDF file ID.
         int id            NetCDF variable ID or NC_GLOBAL.
         int type          NC_FLOAT or NC_DOUBLE.
         const char* name  Name of attribute.
         double value      Value for attribute.
RETURNS: int 1 if successful, else 0 and a message is printed to stderr.
******************************************************************************/

static int writeRealAttribute( int file, int id, int type,
                               const char* name, double value ) {

  int result = 0;
  assert( file >= 0 );
  assert( OR2( id == NC_GLOBAL, id >= 0 ) );
  assert( IN3( type, NC_FLOAT, NC_DOUBLE ) );
  assert( name ); assert( *name );

  if ( type == NC_FLOAT ) {
    const float attribute = CLAMPED_TO_RANGE( value, -FLT_MAX, FLT_MAX );
    result =
      checkStatus( nc_put_att_float( file, id, name, NC_FLOAT, 1, &attribute ),
                   "Can't write value of float attribute" );
  } else {
    result =
      checkStatus( nc_put_att_double( file, id, name, NC_DOUBLE, 1, &value ),
                   "Can't write value of double attribute" );
  }

  assert( IS_BOOL( result ) );
  return result;
}



/******************************************************************************
PURPOSE: writeTextAttribute - Write the value of a text attribute.
INPUTS:  int file          NetCDF file ID.
         int id            NetCDF variable ID or NC_GLOBAL.
         const char* name  Name of attribute.
         const char* value Value of named attribute.
RETURNS: int 1 if successful, else 0 and a message is printed to stderr.
******************************************************************************/

static int writeTextAttribute( int file, int id, const char* name,
                               const char* value ) {

  int result = 0;
  assert( file >= 0 ); assert( OR2( id == NC_GLOBAL, id >= 0 ) );
  assert( name ); assert( *name ); assert( value ); assert( *value );

  result =
    checkStatus( nc_put_att_text( file, id, name, strlen( value ), value ),
                 "Can't write text attribute" );

  assert( IS_BOOL( result ) );
  return result;
}



/******************************************************************************
PURPOSE: writeRealArrayAttribute - Write a real array attribute.
INPUTS:  int file                      NetCDF file ID to write to.
         int type                      NC_FLOAT or NC_DOUBLE.
         const char* name              Name of attribute.
         int count                     Number of elements in values array.
         const double values[ count ]  Value of attribute.
RETURNS: int 1 if successful, else 0 and a message is printed to stderr.
******************************************************************************/

static int writeRealArrayAttribute( int file, int type,
                                    const char* name,
                                    const double values[], int count ) {

  int result = 0;
  assert( file >= 0 ); assert( IN3( type, NC_FLOAT, NC_DOUBLE ) );
  assert( name ); assert( *name ); assert( values ); assert( count > 0 );

  if ( type == NC_FLOAT ) {
    float* attributes = NEW( float, count );

    if ( attributes ) {
      int index = 0;

      for ( index = 0; index < count; ++index ) {
        const double value = values[ index ];
        const float attribute = CLAMPED_TO_RANGE( value, -FLT_MAX, FLT_MAX );
        attributes[ index ] = attribute;
      }

      result =
        checkStatus( nc_put_att_float( file, NC_GLOBAL, name, NC_FLOAT, count,
                                       attributes ),
                     "Can't write value of float array attribute" );

      FREE( attributes );
    }
  } else {
    result =
      checkStatus( nc_put_att_double( file, NC_GLOBAL, name, NC_DOUBLE, count,
                                      values ),
                     "Can't write value of double array attribute" );
  }

  assert( IS_BOOL( result ) );
  return result;
}



/******************************************************************************
PURPOSE: checkStatus - Check the status returned from a NetCDF function and if
         it indicates a problem, print to stderr the message and a reason the
         problem occurred.
INPUTS:  int status           Value returned from a NetCDF function.
         const char* message  Message to print to stderr.
RETURNS: int 1 if there is no problem, else 0.
******************************************************************************/

static int checkStatus( int status, const char* message ) {
  const int result = status == NC_NOERR;
  assert( message ); assert( *message );

  if ( status != NC_NOERR ) {
    const char* const reason = nc_strerror( status );
    fprintf( stderr, "\a\n%s because %s.\n", message, reason );
  }

  assert( IS_BOOL( result ) );
  return result;
}



/******************************************************************************
PURPOSE: isLeapYear - Is the year a leap year - i.e., does it have 366 days?
INPUTS:  int year  The year.
RETURNS: int 1 if year is a leap year, else 0.
******************************************************************************/

static int isLeapYear( int year ) {
  int result = 0;
  assert( year >= 1900 ); assert( year <= 2900 );
  result = year % 4 == 0 && ! ( year % 100 == 0 && year % 400 != 0 );
  assert( result == 0 || result == 1 );
  return result;
}



/******************************************************************************
PURPOSE: nowUTC - Current timestamp in UTC.
RETURNS: long long yyyydddhhmm.
******************************************************************************/

static long long nowUTC( void ) {
  const time_t clock = time( 0 );
  struct tm timeInfo;
  long long result = 19000010000LL;

  if ( gmtime_r( &clock, &timeInfo ) ) {
    result = timeInfo.tm_year + 1900;
    result = result * 1000 + timeInfo.tm_yday + 1;
    result = result *  100 + timeInfo.tm_hour;
    result = result *  100 + timeInfo.tm_min;
  }

  assert( result >= 19000010000LL );
  return result;
}



/******************************************************************************
PURPOSE: freeMemory - Implements the FREE() macro by calling free().
         This provides a checking and debugging wrapper around free().
INPUTS:  void* address  The address to free.
******************************************************************************/

static void freeMemory( void* address ) {
  char* p = (char*) address;
  assert( address );
  DEBUG( fprintf( stderr, "freeMemory( address = %p )\n", address ); )
  *p = '\0'; /* Zero first byte to reveal dangling pointer in client. */
  free( address );
}



/******************************************************************************
PURPOSE: newMemory - Implements NEW() macro by calling the standard malloc()
         and, upon failure a message is printed to stderr.
         This provides a checking and debugging wrapper around malloc().
INPUTS:  size_t count     The number of items to allocate.
         size_t sizeEach  The number of bytes per item.
RETURNS: void*  The resulting address, or 0 if unsuccessful.
NOTES:   If malloc() returns 0 then a message is printed to stderr.
******************************************************************************/

static void* newMemory( size_t count, size_t sizeEach ) {
  void* address = 0;
  const size_t bytes = count * sizeEach;
  assert( count > 0 ); assert( sizeEach > 0 );

  DEBUG( fprintf( stderr, "newMemory( count = %lu, sizeEach = %lu )...\n",
                  count, sizeEach ); )

  /* Don't attempt to allocate if too large to represent: */

  if ( AND3( bytes > 0, bytes >= count, bytes >= sizeEach ) ) {
    address = malloc( bytes );
    DEBUG( fprintf( stderr, "...yields address = %p\n", address ); )
  }

  if ( address == 0 ) {
    fprintf( stderr,
             "\nCan't allocate %lu bytes to complete the requested action.\n",
             bytes );
    perror( 0 );
  } else {
    memset( address, 0, bytes );
  }

  return address;
}



/******************************************************************************
PURPOSE: expandString - Copy a string to padded/truncated form.
INPUTS:  const char* source  String to copy.
         sizet length        Length to truncate/pad copy.
OUTPUTS: char* copy          Padded/truncated copy.
******************************************************************************/

static void expandString( char* copy, const char* source, size_t length ) {
  size_t index = 0;
  assert( copy ); assert( source ); assert( *source ); assert( length > 0 );

  do {
    assert( index < length ); assert( source[ index ] );
    copy[ index ] = source[ index ];
    ++index;
  } while ( AND2( index < length, source[ index ] ) );

  while ( index < length ) {
    copy[ index ] = ' ';
    ++index;
  }

  assert( index == length );
  copy[ index ] = '\0';

  assert( strlen( copy ) == length );
}



/******************************************************************************
PURPOSE: isValidDate - Is the given date valid YYYYDDD format?
INPUTS:  int yyyyddd  The date to check.
RETURNS: int 1 if valid, else 0.
******************************************************************************/

static int isValidDate( int yyyyddd ) {
  const int yyyy = yyyyddd / 1000;
  const int ddd  = yyyyddd % 1000;
  const int result =
    AND3( yyyy >= 1950, IN_RANGE( ddd, 1, 366 ),
          IMPLIES( ddd == 366, isLeapYear( yyyy ) ) );
  assert( IS_BOOL( result ) );
  return result;
}



/******************************************************************************
PURPOSE: isValidTime - Is the given time valid HHMMSS format?
INPUTS:  int hhmmss  The time to check.
RETURNS: int 1 if valid, else 0.
******************************************************************************/

static int isValidTime( int hhmmss ) {
  const int hh = hhmmss / 10000;
  const int mm = ( hhmmss / 100 ) % 100;
  const int ss = hhmmss % 100;
  const int result =
    AND3( IN_RANGE( hh, 0, 23 ), IN_RANGE( mm, 0, 59 ), IN_RANGE( ss, 0, 59));
  assert( IS_BOOL( result ) );
  return result;
}



/******************************************************************************
PURPOSE: isValidTimestepSize - Is the given time valid *HHMMSS format?
INPUTS:  int hhmmss  The timestep size to check.
RETURNS: int 1 if valid, else 0.
******************************************************************************/

static int isValidTimestepSize( int hhmmss ) {
  const int hh = hhmmss / 10000;
  const int mm = ( hhmmss / 100 ) % 100;
  const int ss = hhmmss % 100;
  const int result =
    AND4( hhmmss > 0, hh >= 0, IN_RANGE( mm, 0, 59 ), IN_RANGE( ss, 0, 59 ) );
  assert( IS_BOOL( result ) );
  return result;
}



/******************************************************************************
PURPOSE: incrementTime - Increment date/time by step.
INPUTS:  int* yyyyddd  YYYYDDD.
         int* hhmmss   HHMMSS.
         int  step     ...HHMMSS.
OUTPUTS: int* yyyyddd  YYYYDDD.
         int* hhmmss   HHMMSS.
******************************************************************************/

static void incrementTime( int* yyyyddd, int* hhmmss, int step ) {

  int hours = step / 10000; /* ...hh0000 */

  assert( yyyyddd ); assert( hhmmss );
  assert( isValidDate( *yyyyddd ) ); assert( isValidTime( *hhmmss ) );
  assert( isValidTimestepSize( step ) );

  while ( hours-- ) {
    incrementOneHour( yyyyddd, hhmmss );
  }

  step %= 10000; /* 00mmss. */

  if ( step ) {
    const int stepSS = step % 100;
    int ss = *hhmmss % 100;
    int mm = ( *hhmmss / 100 ) % 100;
    int hh = 0;
    ss += stepSS;
    mm += ss / 60;
    ss %= 60;
    hh = mm / 60;
    mm %= 60;

    if ( hh > 0 ) {
      incrementOneHour( yyyyddd, hhmmss );
      hh = *hhmmss / 10000;
    }

    *hhmmss = hh * 10000 + mm * 100 + ss;
  }

  assert( isValidDate( *yyyyddd ) ); assert( isValidTime( *hhmmss ) );
}



/******************************************************************************
PURPOSE: incrementOneHour - Increment date/time by one hour.
INPUTS:  int* yyyyddd  YYYYDDD.
         int* hhmmss   HHMMSS.
OUTPUTS: int* yyyyddd  YYYYDDD.
         int* hhmmss   HHMMSS.
******************************************************************************/

static void incrementOneHour( int* yyyyddd, int* hhmmss ) {

  const int oneHour     =  10000; /* HHMMSS = 01:00:00. */
  const int maximumTime = 235959; /* HHMMSS = 23:59:59. */

  assert( yyyyddd ); assert( hhmmss );
  assert( isValidDate( *yyyyddd ) ); assert( isValidTime( *hhmmss ) );

  *hhmmss += oneHour;

  if ( *hhmmss > maximumTime ) {
    const int ss = *hhmmss % 100;
    const int mm = ( *hhmmss / 100 ) % 100;
    int ddd = 0;
    *hhmmss = mm * 100 + ss;
    *yyyyddd += 1;
    ddd = *yyyyddd % 1000;

    if ( ddd > 365 ) {
      const int yyyy = *yyyyddd / 1000;

      if ( ! isLeapYear( yyyy ) ) {
        *yyyyddd = ( yyyy + 1 ) * 1000 + 1; /* Next year, first day. */
      }
    }
  }

  assert( isValidDate( *yyyyddd ) ); assert( isValidTime( *hhmmss ) );
}



