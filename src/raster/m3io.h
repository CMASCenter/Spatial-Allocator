#ifndef M3IO_H
#define M3IO_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
PURPOSE: M3IO.h - Declare convenience routines for creating M3IO files.

NOTES: Uses netcdf.h and libnetcdf.a.
       For a description of M3IO Conventions see:
       http://www.baronams.com/products/ioapi/GRIDS.html
 
HISTORY: 2010/11/05, plessel.todd@epa.gov, Created.

STATUS:  unreviewed, untested.
******************************************************************************/

/*================================== TYPES ==================================*/

enum { NAMLEN3 = 16, MXDLEN3 = 80, MXDESC3 = 60, MXVARS3 = 120, MXLAYS3 = 100};

typedef char Name[ NAMLEN3 + 1 ];
typedef char Line[ MXDLEN3 + 1 ];
typedef char Description[ MXDESC3 * MXDLEN3 + 1 ];

/*================================ FUNCTIONS ================================*/

extern int createLonLatM3IOFile( const char* fileName,
                                 int variables,
                                 int timesteps,
                                 int layers,
                                 int rows,
                                 int columns,
                                 int yyyyddd,
                                 int startTime_att,
                                 int timeSteps_att,
                                 double xcellSize,
                                 double ycellSize,
                                 double westEdge,
                                 double southEdge,
                                 double topPressure,
                                 const double sigmaLevels[],
                                 const Name variableNames[],
                                 const Name variableUnits[],
                                 const Line variableDescriptions[],
                                 const Name gridName,
                                 const Description description );

extern int createLambertConformalM3IOFile( const char* fileName,
                                           int variables,
                                           int timesteps,
                                           int layers,
                                           int rows,
                                           int columns,
                                           int yyyyddd,
                                           int startTime_att,
                                           int timeSteps_att,
                                           double xcellSize,
                                           double ycellSize,
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
                                           const Description description );

extern int createLambertEqualAreaM3IOFile( const char* fileName,
                                           int variables,
                                           int timesteps,
                                           int layers,
                                           int rows,
                                           int columns,
                                           int yyyyddd,
                                           int startTime_att,
                                           int timeSteps_att,
                                           double xcellSize,
                                           double ycellSize,
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
                                           const Description description );

extern int createAlbersEqualAreaM3IOFile( const char* fileName,
                                          int variables,
                                          int timesteps,
                                          int layers,
                                          int rows,
                                          int columns,
                                          int yyyyddd,
                                          int startTime_att,
                                          int timeSteps_att,
                                          double xcellSize,
                                          double ycellSize,
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
                                          const Description description );

extern int createPolarM3IOFile( const char* fileName,
                                int variables,
                                int timesteps,
                                int layers,
                                int rows,
                                int columns,
                                int yyyyddd,
                                int startTime_att,
                                int timeSteps_att,
                                double xcellSize,
                                double ycellSize,
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
                                const Description description );

extern int createStereographicM3IOFile( const char* fileName,
                                        int variables,
                                        int timesteps,
                                        int layers,
                                        int rows,
                                        int columns,
                                        int yyyyddd,
                                        int startTime_att,
                                        int timeSteps_att,
                                        double xcellSize,
                                        double ycellSize,
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
                                        const Description description );

extern int createEquitorialMercatorM3IOFile( const char* fileName,
                                             int variables,
                                             int timesteps,
                                             int layers,
                                             int rows,
                                             int columns,
                                             int yyyyddd,
                                             int startTime_att,
                                             int timeSteps_att,
                                             double xcellSize,
                                             double ycellSize,
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
                                             const Description description );

extern int createTransverseMercatorM3IOFile( const char* fileName,
                                             int variables,
                                             int timesteps,
                                             int layers,
                                             int rows,
                                             int columns,
                                             int yyyyddd,
                                             int startTime_att,
                                             int timeSteps_att,
                                             double xcellSize,
                                             double ycellSize,
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
                                             const Description description );

extern int createUTMM3IOFile( const char* fileName,
                              int variables,
                              int timesteps,
                              int layers,
                              int rows,
                              int columns,
                              int yyyyddd,
                              int startTime_att,
                              int timeSteps_att,
                              double xcellSize,
                              double ycellSize,
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
                              const Description description );

extern int createMercatorM3IOFile( const char* fileName,
                                   int variables,
                                   int timesteps,
                                   int layers,
                                   int rows,
                                   int columns,
                                   int yyyyddd,
                                   int startTime_att,
                                   int timeSteps_att,
                                   double xcellSize,
                                   double ycellSize,
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
                                   const Description description );

extern int writeM3IOData( int file,
                          const Name variableName,
                          const float data[] );

extern int writeM3IODataForTimestep( int file,
                                     int timestep,
                                     const Name variableName,
                                     const float data[] );

extern int closeM3IOFile( int file );
  
#ifdef __cplusplus
}
#endif

#endif /* M3IO_H */



