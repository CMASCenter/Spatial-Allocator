/*
example_mercator.c - Demonstrate usage of createMercatorM3IOFile().
cc -I. -o example_mercator example_mercator.c M3IO.c libnetcdf.a
./example_mercator
ncdump example_mercator.ncf
*/

#include <M3IO.h> /* For createMercatorM3IOFile(). */


int main( void ) {
  const char* const fileName = "example_mercator.ncf";
  enum {
    VARIABLES = 3, TIMESTEPS = 1, LAYERS = 1, ROWS = 7, COLUMNS = 8,
    YYYYDDD = 2006075
  };
  const double cellSize         = 108000.0;
  const double westEdge         = 702000.0;
  const double southEdge        = -4914000.0;
  const double longitude0       = -90.0;
  const double latitude0        = 40.0;
  const double centralLongitude = -90.0;
  const double centralLatitude  = 0.0;
  const double rotation         = 0.0;
  const double topPressure = 10000.0;
  const double sigmaLevels[ LAYERS + 1 ] = { 1.0, 0.995 };
  const Name variableNames[ VARIABLES ] = { "LON", "LAT", "HT" };
  const Name variableUnits[ VARIABLES ] = { "deg", "deg", "m" };
  const Line variableDescriptions[ VARIABLES ] = {
    "Longitude (west negative)",
    "Latitude (south negative)",
    "Terrain height above mean sea level"
  };
  const Name gridName = { "GRIDOUT_HEMI_108" };
  const Line description =
    "Example subsetted output from MCIP meteorology model.";

  const float dataLon[ TIMESTEPS * LAYERS * ROWS * COLUMNS ] = {
    -89.15818, -87.9194, -86.69006, -85.47118, -84.26373, -83.06857, -81.88658,
    -80.7185,
    -88.96051, -87.69514, -86.43987, -85.19573, -83.96375, -82.74487, 
    -81.53998, -80.34987,
    -88.75388, -87.46081, -86.1785, -84.9081, -83.65067, -82.40718, -81.17859, 
    -79.96571,
    -88.53767, -87.2157, -85.90524, -84.6075, -83.3236, -82.0546, -80.80145, 
    -79.56505,
    -88.31121, -86.95906, -85.61923, -84.29303, -82.98164, -81.68615, 
    -80.40757, -79.14684,
    -88.07375, -86.69006, -85.31961, -83.96375, -82.62375, -81.30075, 
    -79.99583, -78.70995,
    -87.82449, -86.40782, -85.00538, -83.61861, -82.24883, -80.89726, 
    -79.56505, -78.25317
  };

  const float dataLat[ TIMESTEPS * LAYERS * ROWS * COLUMNS ] = {
    41.32537, 41.17015, 40.99514, 40.80065, 40.58706, 40.35472, 40.10408, 
    39.83553,
    42.26213, 42.10232, 41.92216, 41.722, 41.50221, 41.2632, 41.00541,
    40.7293,
    43.20518, 43.04058, 42.85506, 42.64898, 42.42275, 42.1768, 41.91159, 
    41.62761,
    44.15443, 43.98483, 43.79371, 43.58147, 43.34853, 43.09536, 42.82243, 
    42.53028,
    45.10975, 44.93493, 44.73798, 44.51931, 44.27938, 44.01867, 43.73772, 
    43.43707,
    46.07103, 45.89076, 45.68772, 45.46233, 45.21511, 44.94656, 44.65724, 
    44.34774,
    47.03814, 46.85217, 46.64275, 46.41035, 46.15551, 45.87878, 45.58075, 
    45.26205
  };

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
    createMercatorM3IOFile(
      fileName, VARIABLES, TIMESTEPS, LAYERS, ROWS, COLUMNS, YYYYDDD,
      cellSize, westEdge, southEdge,
      longitude0, latitude0,
      centralLongitude, centralLatitude, rotation,
      topPressure, sigmaLevels,
      variableNames, variableUnits, variableDescriptions,
      gridName, description );

  int ok = file != -1;

  if ( file != -1 ) {
    ok = writeM3IOData( file, "LON", dataLon );
    ok = ok && writeM3IOData( file, "LAT", dataLat );
    ok = ok && writeM3IOData( file, "HT", dataHt );
    ok = closeM3IOFile( file ) && ok;
  }

  return ! ok;
}




