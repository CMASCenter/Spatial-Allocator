/*
example_large.c - Demo large file usage of createLambertConformalM3IOFile().
cc -g -I. -o example_large example_large.c M3IO.c libnetcdf.a
./example_large
ncdump example_large.ncf
*/

#include <stdlib.h> /* For malloc(), free(). */
#include <string.h> /* For memset(). */

#include <M3IO.h> /* For public interface. */

int main( void ) {
  const char* const fileName = "example_large.ncf";
  enum {
    VARIABLES = 50, TIMESTEPS = 10, LAYERS = 50, ROWS = 300, COLUMNS = 500,
    YYYYDDD = 2002113
  };
  const double cellSize            = 2000.0;
  const double westEdge            = 1578000.0;
  const double southEdge           = -1270000.0;
  const double longitude0          = -100.0;
  const double latitude0           = 40.0;
  const double centralLongitude    = -100.0;
  const double lowerSecantLatitude = 30.0;
  const double upperSecantLatitude = 60.0;
  const double topPressure = 10000.0;
  const double sigmaLevels[ LAYERS + 1 ] = { 1.0, 0.995, 0.99, 0.985 };
  const Name variableNames[ VARIABLES ] = {
    "V1", "V2", "V3", "V4", "V5", "V6", "V7", "V8", "V9", "V10",
    "V11", "V12", "V13", "V14", "V15", "V16", "V17", "V18", "V19", "V20",
    "V21", "V22", "V23", "V24", "V25", "V26", "V27", "V28", "V29", "V30",
    "V31", "V32", "V33", "V34", "V35", "V36", "V37", "V38", "V39", "V40",
    "V41", "V42", "V43", "V44", "V45", "V46", "V47", "V48", "V49", "V50"
  };
  const Name variableUnits[ VARIABLES ] = {
    "%", "%", "%", "%", "%", "%", "%", "%", "%", "%",
    "%", "%", "%", "%", "%", "%", "%", "%", "%", "%",
    "%", "%", "%", "%", "%", "%", "%", "%", "%", "%",
    "%", "%", "%", "%", "%", "%", "%", "%", "%", "%",
    "%", "%", "%", "%", "%", "%", "%", "%", "%", "%"
  };
  const Line variableDescriptions[ VARIABLES ] = {
    "V1", "V2", "V3", "V4", "V5", "V6", "V7", "V8", "V9", "V10",
    "V11", "V12", "V13", "V14", "V15", "V16", "V17", "V18", "V19", "V20",
    "V21", "V22", "V23", "V24", "V25", "V26", "V27", "V28", "V29", "V30",
    "V31", "V32", "V33", "V34", "V35", "V36", "V37", "V38", "V39", "V40",
    "V41", "V42", "V43", "V44", "V45", "V46", "V47", "V48", "V49", "V50"
  };
  const Name gridName = { "M_02_99BRACE" };
  const Line description = "Test large file.";
  const size_t bytes = TIMESTEPS * LAYERS * ROWS * COLUMNS * sizeof (float);
  float* data = malloc( bytes );
  int ok = 0;

  if ( data ) {
    const void* unused = memset( data, 0, bytes );
    const int file =
      createLambertConformalM3IOFile(
        fileName,
        VARIABLES, TIMESTEPS, LAYERS, ROWS, COLUMNS, YYYYDDD,
        cellSize, westEdge, southEdge,
        longitude0, latitude0,
        centralLongitude,
        lowerSecantLatitude, upperSecantLatitude,
        topPressure, sigmaLevels,
        variableNames, variableUnits, variableDescriptions,
        gridName, description );

    if ( file != -1 ) {
      int variable = 0;

      for ( variable = 0; variable < VARIABLES; ++variable ) {
        ok = writeM3IOData( file, variableNames[ variable ], data );

        if ( ! ok ) {
          variable = VARIABLES;
        }
      }

      ok = closeM3IOFile( file ) && ok;
    }

    free( data ), data = 0;
  }

  return ! ok;
}

