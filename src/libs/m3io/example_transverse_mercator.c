/*
example_transverse_mercator.c - Demonstrate createTransverseMercatorM3IOFile().
cc -I. -o example_transverse_mercator example_transverse_mercator.c M3IO.c \
   libnetcdf.a
./example_transverse_mercator
ncdump example_transverse_mercator.ncf
*/

#include <M3IO.h> /* For createTransverseMercatorM3IOFile(). */


int main( void ) {
  const char* const fileName = "example_transverse_mercator.ncf";
  enum {
    VARIABLES = 1, TIMESTEPS = 1, LAYERS = 1, ROWS = 7, COLUMNS = 8,
    YYYYDDD = 2009121
  };
  const double cellSize         = 1000.0;
  const double westEdge         = -605000.0;
  const double southEdge        = 2773385.25;
  const double longitude0       = -84.0;
  const double latitude0        = 40.0;
  const double centralLongitude = -84.0;
  const double centralLatitude  = 0.0;
  const double topPressure = 5000.0;
  const double sigmaLevels[ LAYERS + 1 ] = { 1.0, 0.993 };
  const Name variableNames[ VARIABLES ] = { "HT" };
  const Name variableUnits[ VARIABLES ] = { "m" };
  const Line variableDescriptions[ VARIABLES ] = {
    "Terrain height above mean sea level"
  };
  const Name gridName = { "GRIDOUT_12345678" };
  const Line description =
    "Fake data as an example.";

  const float dataHt[ TIMESTEPS * LAYERS * ROWS * COLUMNS ] = {
    202.7232, 201.5671, 226.0765, 256.8724, 265.7115, 272.4124, 263.5912, 
    387.1347,
    230.4779, 189.0773, 209.0974, 257.2342, 241.4965, 226.8955, 277.8255, 
    377.1699,
    245.6115, 191.0317, 205.1927, 253.6518, 246.9528, 192.9227, 216.7385, 
    376.0814,
    240.7513, 194.132, 234.2364, 262.8504, 232.0699, 221.0696, 220.7804,
    278.112,
    309.0673, 199.3064, 235.3988, 268.6735, 204.9971, 238.3341, 270.7552, 
    176.8044,
    345.7384, 204.6591, 185.8689, 215.9475, 183.6354, 195.4428, 271.9525, 
    252.7372,
    212.9352, 185.9479, 224.5607, 279.713, 252.0324, 217.7531, 297.0352, 
    359.1869
  };

  const int file =
    createTransverseMercatorM3IOFile(
      fileName, VARIABLES, TIMESTEPS, LAYERS, ROWS, COLUMNS, YYYYDDD,
      cellSize, westEdge, southEdge,
      longitude0, latitude0,
      centralLongitude, centralLatitude,
      topPressure, sigmaLevels,
      variableNames, variableUnits, variableDescriptions,
      gridName, description );

  int ok = file != -1;

  if ( file != -1 ) {
    ok = writeM3IOData( file, "HT", dataHt );
    ok = closeM3IOFile( file ) && ok;
  }

  return ! ok;
}




