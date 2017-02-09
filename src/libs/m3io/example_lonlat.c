/*
example_lonlat.c - Demonstrate createLonLatM3IOFile().
cc -I. -o example_lonlat example_lonlat.c M3IO.c libnetcdf.a
./example_lonlat
ncdump example_lonlat.ncf
*/

#include <M3IO.h> /* For Name, Line, Description, createLonLatM3IOFile(). */


int main( void ) {
  const char* const fileName = "example_lonlat.ncf";
  enum {
    VARIABLES = 1, TIMESTEPS = 1, LAYERS = 1, ROWS = 5, COLUMNS = 4,
    YYYYDDD = 2002113
  };
  const double cellSize    = 1.0;   /* 1-degree square grid cells. */
  const double westEdge    = -90.0; /* Lower-left corner at New Orleans. */
  const double southEdge   = 30.0;
  const double topPressure = 10000.0;
  const double sigmaLevels[ LAYERS + 1 ] = { 1.0, 0.0 };
  const Name variableNames[ VARIABLES ] = { "HT" };
  const Name variableUnits[ VARIABLES ] = { "m" };
  const Line variableDescriptions[ VARIABLES ] = { "Terrain Height" };
  const Name gridName = { "M_02_99BRACE" };
  const Description description = "Fake data used as an example";
  const float dataHT[ TIMESTEPS * LAYERS * ROWS * COLUMNS ] = {
    10.1, 10.2, 10.3, 10.4,
    20.1, 20.2, 20.3, 20.4,
    30.1, 30.2, 30.3, 30.4,
    40.1, 40.2, 40.3, 40.4,
    50.1, 50.2, 50.3, 50.4
  };

  const int file =
    createLonLatM3IOFile(
      fileName, VARIABLES, TIMESTEPS, LAYERS, ROWS, COLUMNS, YYYYDDD,
      cellSize, westEdge, southEdge,
      topPressure, sigmaLevels,
      variableNames, variableUnits, variableDescriptions,
      gridName, description );

  int ok = file != -1;

  if ( file != -1 ) {
    ok = writeM3IOData( file, "HT", dataHT );
    ok = closeM3IOFile( file ) && ok;
  }

  return ! ok;
}



