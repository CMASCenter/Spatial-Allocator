/*******************************************************
*    Geoanalysis Tools from GDAL, PROJ4 and NetCDF     *
*******************************************************/
#include <map>
#include  <string>
#include "cpl_conv.h"
#include "cpl_string.h"
#include "cpl_multiproc.h"

#include "ogrsf_frmts.h"
#include "ogr_p.h"
#include "ogr_core.h"
//#include "ogr_api.h"



#include "gdal_priv.h"
//#include "gdal.h"
//#include "gdal_alg.h"

#include  "proj_api.h"
#include  "netcdf.h"

//#include "hdf.h"
#include  "mfhdf.h"
//#include  "H5Cpp.h"
#include  "hdf5.h"

#include <geos_c.h>
//#include <geos.h>

#include <algorithm>

//nearest point search interface
#include <ANN/ANN.h>

//IOAPI public interface header
#include "m3io.h"


using namespace std;

typedef struct _gridInfo {
  string       name;
  string       polyID;
  char         *strProj4;
  double       xCellSize;
  double       yCellSize;
  int          rows; 
  int          cols;
  vector<int>  dims;
  double       xmin;
  double       ymin;
  double       xmax;
  double       ymax;
  float        cornerLats[4];
  float        cornerLons[4];
  float        cenLat;
  float        cenLon;
  float        *x;
  float        *y;
  float        *lat;
  float        *lon;
  vector<string> attsStr; //0=title,1=units,2=scale factor,3=offset,4=missing value,5=data type
} gridInfo;

typedef struct _ncVarData {
  int          *intData;
  float        *floatData;
} ncVarData;


/**********************************
*        Functions                *
***********************************/

string   createGridShapes ( gridInfo grid );
gridInfo getImageInfo (string imageFile);
string   projectShape (string gdalBinDir, string shapeFile, char *toProj4 );
double   computeRasterResolution ( gridInfo imageInfo, gridInfo grid );
gridInfo computeNewRasterInfo (string shapeFile, double rasterResolution, gridInfo grid );
string   toRasterFile (string gdalBinDir, gridInfo newRasterInfo, string srcRasterFile, string shapeFile, gridInfo grid );
void     deleteShapeFile ( string shapeFile );
void     deleteRasterFile ( string rasterFile );
projUV   computeLatLong(projPJ proj4DF, double x, double y);
void     computeGridCornerCenterLatLong ( gridInfo *grid );
void     anyErrors(int e);
void     computeGridCoordinates ( gridInfo *grid );
void     writeWRFGlobalAtributes ( int ncid, string title, gridInfo grid, string startDateTime );
void     writeWRFGridCoordinates ( int ncid, int west_east_dim, int south_north_dim, gridInfo grid );
projUV   projectPoint( projPJ proj4From, projPJ proj4To, double x, double y );
void     getDayTimeStepRange ( int *dayTimeStepRange, int mcipDay, int varID, int *timeVar, int var_ndims, size_t *varDimSize );
void     getGridInfofromNC ( int ncid , gridInfo *grid, const char *ncType );
bool     sameGrids ( gridInfo grid1, gridInfo grid2 );
void     printGridInfo (gridInfo grid);
void     anyHDF4Errors( int  e );
string   createGridImage( gridInfo grid );
string   resampleRaster (string imageFile, gridInfo grid);
void     readHDF4SatVarData (string imageFile, string varName, double *poImage);
gridInfo getHDF4VarInfo ( string imageFile, string varName );
string   getHDF4VarAttrInfo (int32 sds_id, const char *attName );
gridInfo getHDF5VarInfo ( string imageFile, string varName );
string   getHDF5VarAttrInfo ( hid_t dataset_id, const char *attName );
void     readHDF5SatVarData (string imageFile, string varName, double *poImage);
string   getHDF5VarGroups (hid_t file_id, string varName);
void     anyHDF5Errors( int  e );
void     getStartArraytoWriteNetCDF (size_t *start, gridInfo imageInfo, int timeSteps);
void     getCountArraytoWriteNetCDF ( size_t *count, gridInfo imageInfo, size_t south_north_len, size_t west_east_len );
void     writeWRF1StepTimeVariables (int timeSteps, float minutesPassed, string time_str, int ncid, int time_id, 
                                     int timeStr_id, size_t dateStr_len);
void     setWRFNetCDFSatVarDimIndex ( int ncid, size_t *dims_len, int *dims_id, int *numADims, int *dimIndex, gridInfo imageInfo );
int      defineWRFNetCDFSatVar ( int ncid, string satVarName, int numDims, int *dimIndex, gridInfo imageInfo );
void     defineWRFNC4Dimensions ( int ncid, size_t dateStr_len, size_t west_east_len, size_t south_north_len,
                               int *time_dim, int *dateStr_dim, int *west_east_dim, int *south_north_dim );
void     defineWRFNCTimeVars ( int ncid, int time_dim, int dateStr_dim, string startDateTime, int *time_id, int *timeStr_id);
bool     computeDomainGridImageIndex ( int *grdIndex, double *longP, double *latP,
                                       gridInfo imageInfoLat, gridInfo newRasterInfo, double searchRadius );
void     computeGridSatValues ( GUInt32 *poImage_grd, int *grdIndex, float *satV, double *poImage,
                                gridInfo imageInfo, gridInfo newRasterInfo, gridInfo grid );
void     writeWRFCharVariable ( int ncid, int dimNum, size_t dimLen, size_t textlen, int var_id, char *charStr_epic, string arrayName);
int      defineNCFloatVariable (int ncid, const char *varName, int numDims, int *dimIndex, const char *varDesc, 
                                const char *varUnit, float scaleFactor, float offset );
void      readIOAPIVar ( ncVarData *mcipData,  int ncid, int var_id, const char *varName, nc_type var_type, int var_ndims, int *var_dimids, size_t *dimSizes, size_t *varDimSize );
int defineNCCharVariable (int ncid, const char *varName, int numDims, int *dimIndex, const char *varDesc, const char *varUnit );
int defineNCIntVariable (int ncid, const char *varName, int numDims, int *dimIndex, const char *varDesc, const char *varUnit, int scaleFactor, int offset );
gridInfo computeNewRasterInfo_fromImage ( double rasterResolution, gridInfo grid, gridInfo imageInfo ); 
string projectImage (string gdalBinDir, string imageFile, gridInfo inGrid, gridInfo outGrid );
gridInfo copyImageInfo ( gridInfo grid ); 
string  getRandomFileName ( string ext );
gridInfo  getAllMODISTileInfo (  std::vector<string> modisFiles, string varName );
string  extractDomainMODISData ( std::vector<string> modisFiles, string varName, gridInfo grid);
string  projectRasterFile ( string imageFile, gridInfo imageInfo, gridInfo grid );
void  getImageGByteArray ( string imageFile, gridInfo imageInfo, GByte *poImage);
bool compareGridProjection  (gridInfo grid1, gridInfo grid2);
void writeWRFGridCoordinates_time ( int ncid, int time_dim, int west_east_dim, int south_north_dim, gridInfo grid );
float * readWRFNCVarDataFloat ( gridInfo grid, const char * cropFName, string fileName );
string  createEPICSiteShapeFile (gridInfo grid, std::vector<int> epicSiteIDs );
string  identity2SHPFiles (string shpFile1, string shpFile2 );
void getPointsInPolyItems (gridInfo grid,  std::vector<int> epicSiteIDs, std::map<int, vector<string> > &gridEPICData, string polySHP, std::vector<string> cntrySHPItems );
void getPointsInRasterValues (gridInfo grid,  std::vector<int> epicSiteIDs, std::map<int, vector<string> > &gridEPICData, string imageFile, string outImageValue_str );
int getNetCDFDim ( char * dimName, string fileName );
string   extractDomainLAIData ( std::vector<string> modisFiles, std::vector<string> satVars, gridInfo grid);
string   extractDomainALBData ( std::vector<string> modisFiles, std::vector<string> satVars, gridInfo grid);
