/********************************************************************
 * This program includes some common geospatial analysis functions
 * used by other programs:
 *  1. createGridShapes - create a modeling domain grid Shapefile.
 *  2. getImageInfo - get general image information
 *  3. projectShape - project a Shapefile.
 *  4. computeRasterResolution - compute raster resolution based on domain grid size and image resolution
 *  5. computeNewRasterInfo - compute raster information for rasterized domain grids.
 *  6. toRasterFile - rasterize Shapefile, such as grid domain Shapefile
 *  7. deleteShapeFile - delete a Shapefile
 *  8. deleteRasterFile - delete a created raster BIL file using .aux.xml file to check existence
 *  9. computeLatLong - get lat and long coordinates of a x and y point
 *  10. computeGridCornerCenterLatLong - compute grid center and corner lat and long for WRF global attributes
 *  11. anyErrors - handle netcdf error
 *  12. computeGridCoordinates - compute x, y, lat and lon for domain grids
 *  13. writeWRFGridCoordinates - write x, y, lat and lon grid cooordinates
 *  14. writeWRFGlobalAtributes - write WRF NetCDF global attributes
 *  15. projectPoint - project a point from a projection to another projection
 *  16. getDayTimeStepRange - get a day time step range from MCIP NetCDF time variable
 *  17. getGridInfofromNC - put a NetCDF file grid information in a grid variable
 *  18. sameGrids - check to see whether two grids are the same
 *  19. printGridInfo - printf gridInfo variable contents
 *  20. anyHDF4Errors - handle HDF4 errors
 *  21. createGridImage - create a domain grid image in EHdr format (.bil)
 *  22. resampleRaster - re-sample a grid image
 *  23. readHDF4SatVarData - read HDF4 variable array data
 *  24. getHDF4VarInfo - get HDF4 variable info
 *  25. getHDF4VarAttrInfo - get HDF attribute value
 *  26. readHDF4SatVarDataDouble - read HDF4 double array
 *  27. readHDF4SatVarDataByte - read HDF4 byte array
 *  28. readHDF4SatVarDataShort - read HDF4 short array
 *  29. getHDF5VarInfo - get HDF5 variable info
 *  30. getHDF5VarAttrInfo - get HDF5 attribute value
 *  31. readHDF5SatVarData - read HDF5 variable array data
 *  32. getHDF5VarGroups - get data set group path for a HDF5 file
 *  33. anyHDF5Errors - handle HDF5 errors
 *  34. getStartArraytoWriteNetCDF - get start array to write one time step NetCDF array
 *  35. getCountArraytoWriteNetCDF - get count array to write one time step NetCDF array 
 *  36. writeWRF1StepTimeVariables - write one time step time WRF time variables
 *  37. setWRFNetCDFSatVarDimIndex - set NetCDF Sat Variable Dimention Information
 *  38. defineWRFNetCDFSatVar - define WRF NetCDF attribute variables
 *  39. defineWRFNC4Dimensions - define WRF NetCDF 4 dimensions
 *  40. defineWRFNCTimeVars - define WRF NetCDF time variables
 *  41. computeDomainGridImageIndex - compute domain grid indexes in sat image using ANN 
 *  42. computeGridSatValues - compute domain grid satellite image value
 *  43. writeWRFCharVariable - write a WRF char variable
 *  44. defineNCFloatVariable - define a float NC variable
 *  45. readIOAPIVar - read a IOAPI file variable
 *  46. defineNCCharVariable - define a char NC variable
 *  47. defineNCIntVariable - define c int NC variable
 *  48. computeNewRasterInfo_fromImage - computed grid info for an image to be projected
 *  49. projeictImage - project an image
 *  50. copyImageInfo - copy a image information structure
 *  51. getRandomFileName - get random file name which is new
 *  52. getAllMODISTileInfo - get MODIS land cover tile info with extent
 *  53. extractDomainMODISData - extract MODIS data out for a domain area
 *  54. projectRasterFile - project a raster file into a new projection
 *  55. getImageGByteArray - get GDAL image band 1 array
 *  56. compareGridProjection - compare gridInfo projection same or not
 *  57. writeWRFGridCoordinates_time - write WRF netCDF grid coordinates with Time variable
 *  58. readWRFNCVarDataFloat - read WRF NC float array
 *  59. createEPICSiteShapeFile - create EPIC site shapefile
 *  60. identity2SHPFiles - intersect two shapefiles
 *  61. getPointsInPolyItems - get point values from shapefile
 *  62. getPointsInRasterValues - get point value in raster file
 *  63. getNetCDFDim - get netcdf dimension by name
 *  64. extractDomainLAIData - extract LAI image into one
 *  65. extractDomainALBData - extract Albedo image into one
 *
 * Written by the Institute for the Environment at UNC, Chapel Hill
 * in support of the EPA CMAS Modeling and NASA Grants, 2009.
 *
 * By L. Ran, May 2009 - Current
 *
 * Usage: to be included in other programs
***********************************************************************/

#include <dirent.h>
#include <ctime> 
#include <cstdlib>

#include  "geotools.h"
#include  "commontools.h"

const char   *pszDriverName = "ESRI Shapefile";
struct stat   stFileInfo;    //for checking file or directory
int           MODIS_LC_FillValue = 0;     //missing data areas are Ocean in MODIS land cover data and check them always 
int           MODIS_LAI_FillValue = 255;  //255 is fillvalue for both LAI and FPAR
int           MODIS_ALB_FillValue = 32767;   //MODIS albedo fillvalue
int           MODIS_ALB_PARAMS = 3;   //3 albedo MCD43A1 variable parameters


/****************************************/
/*   1.      createGridShapes           */
/****************************************/
string  createGridShapes ( gridInfo grid )
{
    string shapeName;

    GDALDriver  *poDriver = NULL;
    GDALDataset *poDS = NULL;
    OGRSpatialReference oSRS;
    OGRLayer *poLayer = NULL;
    char szName[33];
    OGRFeature *poFeature = NULL;
    OGRLinearRing  oRing;
    OGRPolygon oPoly;

    double xmin,xmax;
    double ymin,ymax;
    int i,j;
    int gid = 0;
    char *pszProj4;


    printf("\nGenerating grid shapefile for a given domain...\n");

    //get Shapefile Name
    if ( grid.name.empty() )
    {
       //create temp Shapefile name 

       //get random file name in the current directoty
       string ext = string ( "_gis.shp" );
       shapeName = getRandomFileName ( ext );
    }
    else
    {
       shapeName = grid.name;
       if ( shapeName.find ( ".shp" ) == string::npos )
       {
          shapeName.append ( ".shp" );
       }
    }


/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
   GDALAllRegister();
   

   //check the output shapefile.  If it exists delete it
   deleteShapeFile ( shapeName );

/* -------------------------------------------------------------------- */
/*      Open output shapefile.                                          */
/* -------------------------------------------------------------------- */
    poDriver = GetGDALDriverManager()->GetDriverByName( pszDriverName );
    if( poDriver == NULL )
    {
        printf( "\t%s driver not available.\n", pszDriverName );
        exit( 1 );
    }


    poDS = poDriver->Create( shapeName.c_str(), 0, 0, 0, GDT_Unknown, NULL );
    if( poDS == NULL )
    {
        printf( "\tCreating Shapefile file failed.\n" );
        exit( 1 );
    }
    printf( "\tCreated shapes.\n" );

/* -------------------------------------------------------------------- */
/*      Create output Polygon feature.                                  */
/* -------------------------------------------------------------------- */
    string proj4Str = string(grid.strProj4);
    printf ( "\tGrid projection: %s\n", proj4Str.c_str() );

    if ( oSRS.importFromProj4( proj4Str.c_str() ) != OGRERR_NONE )
    {
        printf ("\tError: Importing a projection into a shapefile failed - %s\n", proj4Str.c_str() );
        exit ( 1 );

    }


    poLayer = poDS->CreateLayer( shapeName.c_str(), &oSRS, wkbPolygon, NULL );

    if( poLayer == NULL )
    {
        printf( "\tCreating layer failed.\n" );
        exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Create Polygon Attribute Name.                                  */
/* -------------------------------------------------------------------- */

    OGRFieldDefn oField( grid.polyID.c_str(), OFTInteger);
    printf ("\tCreated item.\n");

    if( poLayer->CreateField( &oField ) != OGRERR_NONE )
    {
        printf( "\tCreating Name field failed.\n" );
        exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Assign each polygon attribute and geometry.                     */
/* -------------------------------------------------------------------- */

    for ( i = 0; i < grid.rows; i++ )
    {
      ymin = grid.ymin + i * grid.yCellSize;
      ymax = ymin + grid.yCellSize;

      for( j = 0; j < grid.cols; j++ )
      {
         //printf ("\trow=%d  col=%d\n",i,j);

         gid ++;

         poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
         poFeature->SetField( grid.polyID.c_str(), gid );

         xmin = grid.xmin + j * grid.xCellSize;
         xmax = xmin + grid.xCellSize;

         oRing.addPoint( xmin, ymin );
         oRing.addPoint( xmin, ymax );
         oRing.addPoint( xmax, ymax );
         oRing.addPoint( xmax, ymin );
         oRing.addPoint( xmin, ymin );
         //printf("ID = %d   x: %lf   %lf, y: %lf   %lf\n",gid,xmin,xmax,ymin,ymax);

         oPoly.addRing( &oRing );

         if ( poFeature->SetGeometry( &oPoly ) != OGRERR_NONE )
         {
            printf( "\tSetting a polygon feature failed.\n" );
            exit( 1 );
         }

         if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE )
         {
            printf( "\tFailed to create feature in shapefile.\n" );
            exit( 1 );
         }

         oRing.empty();
         oPoly.empty();
         OGRFeature::DestroyFeature( poFeature );
      }
    }

    GDALClose( poDS );
    printf ("\tCompleted in creating the grid shapefile.\n\n");
    
    return shapeName;

}


/****************************************/
/*   2.    getImageInfo                 */
/****************************************/
gridInfo getImageInfo (string imageFile)
{

    gridInfo        imageInfo;

    GDALDataset     *poRDataset;
    GDALDriver      *poDrive;
    GDALRasterBand  *poBand;
    double          adfGeoTransform[6];
    const char      *pszWKT = NULL;
    char            *pszWKT_nc = NULL;
    OGRSpatialReference oSRS;

    const char      *poDriverDesc;
    int             i;

    //HDF4 
    bool            isHDF4;
    string          varName;


    printf("\n\tObtaining image information from %s...\n",imageFile.c_str() );

    //process imageFile to get name and HDF variable 
    vector<string> strValues = string2stringVector (imageFile, "|"); 

    if (strValues.size() == 1)
    {
      // GDAL image, not HDF4 image       
      isHDF4 = false;
    }
    else if ( strValues.size() == 2 )
    {  
        isHDF4 = true;
        imageFile = strValues [0];  //first is the file name
        varName = strValues [1];    //second is the variable name for HDF4 file
    }
    else
    {
       printf( "\tImage file name is not correct:\n", imageFile.c_str() );
       exit( 1 ); 
    }


   if( ! isHDF4 )
   {

/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
       GDALAllRegister();

       poRDataset = (GDALDataset *) GDALOpen( imageFile.c_str(), GA_ReadOnly );
       if( poRDataset == NULL )
       {
          printf( "\tOpen GDAL image file failed: %s.\n", imageFile.c_str() );
          exit( 1 );
       }

       printf( "\tDriver: %s/%s\n",
            poRDataset->GetDriver()->GetDescription(),
            poRDataset->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );
     
       poDriverDesc = poRDataset->GetDriver()->GetDescription();

       imageInfo.rows = poRDataset->GetRasterYSize();
       imageInfo.cols = poRDataset->GetRasterXSize();
       printf( "\tSize is %dx%dx%d\n",
            poRDataset->GetRasterXSize(), poRDataset->GetRasterYSize(),
            poRDataset->GetRasterCount() );

       if( poRDataset->GetGeoTransform( adfGeoTransform ) != CE_None )
       {
           printf( "\tGetGeoTransform parameters failed for image: %s.\n", imageFile.c_str() );
           exit( 1 );
       }

       imageInfo.xmin = adfGeoTransform[0];
       imageInfo.ymax = adfGeoTransform[3];
       printf( "\tOrigin = (%.6f,%.6f)\n", adfGeoTransform[0], adfGeoTransform[3] );

       imageInfo.xCellSize =  adfGeoTransform[1];
       imageInfo.yCellSize =  -1 * adfGeoTransform[5] ;  // Make it positive.  Negative due to Origin is UL corner point
       printf( "\tPixel Size = (%.3f,%.3f)\n", imageInfo.xCellSize, imageInfo.yCellSize );

       imageInfo.xmax = imageInfo.xmin + imageInfo.xCellSize * imageInfo.cols;
       imageInfo.ymin = imageInfo.ymax - imageInfo.yCellSize * imageInfo.rows;


       printf( "\tSatellite image extent:  minXY(%.3f,%.3f)   maxXY(%.3f,%.3f)\n", imageInfo.xmin,imageInfo.ymin,imageInfo.xmax,imageInfo.ymax );

       if( (pszWKT = poRDataset->GetProjectionRef())  == NULL )
       {
          printf( "\tProjection is not defined in file: %s.\n", imageFile.c_str() );
          exit( 1 );
       }
      
    
       pszWKT_nc =strdup ( pszWKT );

       oSRS.importFromWkt( &pszWKT_nc );
       oSRS.exportToProj4( &imageInfo.strProj4 );
       printf ( "\tProj4 projection string from image is: %s\n", imageInfo.strProj4);

       
       if (strcmp (poDriverDesc, "HDF5") == 0 )
       {

          //================testing metadata
          int    iMDD;
          char   **papszMetadata;
          char   **papszExtraMDDomains = NULL;

          papszMetadata = GDALGetMetadata( (GDALDatasetH) poRDataset,"");
          if( CSLCount(papszMetadata) > 0 )
          {
             printf( "\tMetadata:\n" );
             for( i = 0; papszMetadata[i] != NULL; i++ )
             {
                printf( "\t%s\n", papszMetadata[i] );
             }
          }
      
          for( iMDD = 0; iMDD < CSLCount(papszExtraMDDomains); iMDD++ )
          {
             papszMetadata = GDALGetMetadata( (GDALDatasetH) poRDataset, papszExtraMDDomains[iMDD] );
             if( CSLCount(papszMetadata) > 0 )
             {
                printf( "\tMetadata (%s):\n", papszExtraMDDomains[iMDD]);
                for( i = 0; papszMetadata[i] != NULL; i++ )
                {
                    printf( "\t%s\n", papszMetadata[i] );
                }
             }
          }

          papszMetadata = GDALGetMetadata( (GDALDatasetH) poRDataset, "SUBDATASETS" );
          if( CSLCount(papszMetadata) > 0 )
          {
             printf( "\tSubdatasets:\n" );
             for( i = 0; papszMetadata[i] != NULL; i++ )
             {
                printf( "\t%s\n", papszMetadata[i] );
             }
          }
       }  

       GDALClose( (GDALDatasetH) poRDataset );

    }  //for GDAL image
    else   //GIOVANNI HDF4 L3 and LG2 image files
    {
       int32      sd_id, sds_id, istat, n_datasets, n_file_attrs, index;
       char       sds_name[64]; 
       int32      rank, dim_sizes[MAX_VAR_DIMS], num_type, attributes;
       char       *varName_nc;

       sd_id = SDstart(imageFile.c_str(), DFACC_READ);
       if (sd_id == FAIL)
       {
          printf ("\tOpen HDF4 file failed: %s.\n", imageFile.c_str() );
          exit ( 1 );
       }
     
       /* Determine the contents of the file. */
       if ( SDfileinfo(sd_id, &n_datasets, &n_file_attrs) == FAIL) 
       {
           printf ("\tGetting HDF4 file info failed: %s\n", imageFile.c_str() );
           exit ( 1 );
       }

       //define variables related to attributes
       int32   attr_index, data_type, count;
       char    attr_name[64], buffer[164];
       float   bufferFloat[0];
       int     bufferInt[0];
       bool    isLatLong=true;
       string  valueStr, tmp_str;
         
       //get dimension size of HDF4 variable
       varName_nc = strdup ( varName.c_str() ); 
       anyHDF4Errors ( index = SDnametoindex ( sd_id, varName_nc ) );
       anyHDF4Errors ( sds_id = SDselect (sd_id, index) );
       anyHDF4Errors( SDgetinfo( sds_id, sds_name, &rank, dim_sizes, &num_type, &attributes ) );

       printf ( "\tDim1=%d   Dim2=%d  Data Type: %d\n", dim_sizes[rank-2],dim_sizes[rank-1], num_type); 
       printf("\tname = %s  rank = %d  number of attributes = %d\n", sds_name, rank, attributes );

       //get Title
       imageInfo.attsStr.push_back ( varName );

       //Units
       valueStr = getHDF4VarAttrInfo (sds_id, "units");
       imageInfo.attsStr.push_back ( valueStr );
       printf ("\t Units = %s\n", valueStr.c_str() );
    
       //scale_factor
       valueStr = getHDF4VarAttrInfo (sds_id, "scale_factor");
       if (imageFile.find ("OMAEROe") != string::npos )
       {
         valueStr = string ("1.0");
       }
       imageInfo.attsStr.push_back ( valueStr );
       printf ( "\t scale_factor = %s\n", valueStr.c_str() );

       //add_offset
       valueStr = getHDF4VarAttrInfo (sds_id, "add_offset");
       imageInfo.attsStr.push_back ( valueStr );
       printf ( "\t add_offset = %s\n", valueStr.c_str() ); 
    
       //missing value
       valueStr = getHDF4VarAttrInfo (sds_id, "_FillValue");
       //valueStr = getHDF4VarAttrInfo (sds_id, "MissingValue");
       imageInfo.attsStr.push_back ( valueStr );
       printf ( "\t MissingValue = %s\n", valueStr.c_str() );

       //data type
       sprintf (buffer, "%d\0",num_type);
       imageInfo.attsStr.push_back ( string ( buffer ) );
       printf ("\t Variable type = %s\n", buffer);

       anyHDF4Errors ( SDendaccess(sds_id) );


       //get YDim variable attributes
       if ( imageFile.find( "MODIS" ) == string::npos  )
       {
          varName_nc = strdup ( "YDim" );
       }
       else
       {
          varName_nc = strdup ( "YDim:mod08" );
       }
       anyHDF4Errors ( index = SDnametoindex ( sd_id, varName_nc ) );
       anyHDF4Errors ( sds_id = SDselect (sd_id, index) );
       anyHDF4Errors( SDgetinfo(sds_id, sds_name, &rank, dim_sizes,  &num_type, &attributes) );

       printf("\tname = %s  rank = %d  number of attributes = %d\n", sds_name, rank, attributes );
       
       //y cell size   
       anyHDF4Errors ( (attr_index = SDfindattr(sds_id, "stride") ) );
       anyHDF4Errors ( SDreadattr(sds_id, attr_index, bufferFloat) );
       imageInfo.yCellSize = bufferFloat[0];
       printf ("\t stride=%f\n", bufferFloat[0]);

       //ymin
       anyHDF4Errors ( (attr_index = SDfindattr(sds_id, "startValue") ) );
       anyHDF4Errors ( SDreadattr(sds_id, attr_index, bufferFloat) );
       imageInfo.ymin = bufferFloat[0];
       printf ("\t ymin=%f\n", bufferFloat[0]); 

       //ymax
       anyHDF4Errors ( (attr_index = SDfindattr(sds_id, "endValue") ) );
       anyHDF4Errors ( SDreadattr(sds_id, attr_index, bufferFloat) );
       imageInfo.ymax = bufferFloat[0];
       printf ("\t ymax=%f\n", bufferFloat[0]);  

       //row
       anyHDF4Errors ( (attr_index = SDfindattr(sds_id, "size") ) );
       anyHDF4Errors ( SDreadattr(sds_id, attr_index, bufferInt) );
       imageInfo.rows = bufferInt[0];
       printf ("\t rows=%d\n", bufferInt[0]);

       //units
       anyHDF4Errors ( (attr_index = SDfindattr(sds_id, "unitsCategory") ) );
       anyHDF4Errors ( SDattrinfo(sds_id, index, attr_name, &data_type, &count) );
       anyHDF4Errors ( SDreadattr(sds_id, attr_index, buffer) );
       tmp_str = string ( buffer );
       tmp_str = tmp_str.substr (0, 8);
       printf ("\t units = %s\n", tmp_str.c_str() );
       if (strcmp (tmp_str.c_str() , "latitude" ) != 0)
       {
          isLatLong = false;           
       }
       anyHDF4Errors ( SDendaccess(sds_id) );


       //get XDim variable attributes
       if ( imageFile.find( "MODIS" ) == string::npos  )
       {
          varName_nc = strdup ( "XDim" );
       }
       else
       {
          varName_nc = strdup ( "XDim:mod08" );
       }
       anyHDF4Errors ( index = SDnametoindex ( sd_id, varName_nc ) );
       anyHDF4Errors ( sds_id = SDselect (sd_id, index) );
       anyHDF4Errors( SDgetinfo(sds_id, sds_name, &rank, dim_sizes,  &num_type, &attributes) );

       printf("\tname = %s  rank = %d  number of attributes = %d\n", sds_name, rank, attributes );
        
       //x cell size   
       anyHDF4Errors ( (attr_index = SDfindattr(sds_id, "stride") ) );
       anyHDF4Errors ( SDreadattr(sds_id, attr_index, bufferFloat) );
       imageInfo.xCellSize = bufferFloat[0];
       printf ("\t stride=%f\n", bufferFloat[0]);

       //xmin
       anyHDF4Errors ( (attr_index = SDfindattr(sds_id, "startValue") ) );
       anyHDF4Errors ( SDreadattr(sds_id, attr_index, bufferFloat) );
       imageInfo.xmin = bufferFloat[0];
       printf ("\t xmin=%f\n", bufferFloat[0]); 

       //xmax
       anyHDF4Errors ( (attr_index = SDfindattr(sds_id, "endValue") ) );
       anyHDF4Errors ( SDreadattr(sds_id, attr_index, bufferFloat) );
       imageInfo.xmax = bufferFloat[0];
       printf ("\t xmax=%f\n", bufferFloat[0]);  

       //cols
       anyHDF4Errors ( (attr_index = SDfindattr(sds_id, "size") ) );
       anyHDF4Errors ( SDreadattr(sds_id, attr_index, bufferInt) );
       imageInfo.cols = bufferInt[0];
       printf ("\t cols=%d\n", bufferInt[0]);

       //units
       anyHDF4Errors ( (attr_index = SDfindattr(sds_id, "unitsCategory") ) );
       anyHDF4Errors ( SDattrinfo(sds_id, index, attr_name, &data_type, &count) );
       anyHDF4Errors ( SDreadattr(sds_id, attr_index, buffer) );
       tmp_str = string ( buffer );
       tmp_str = tmp_str.substr (0, 9);
       printf ("\t units = %s\n", tmp_str.c_str() );
       if (strcmp (tmp_str.c_str(), "longitude" ) != 0)
       {
          isLatLong = false;           
       }
       anyHDF4Errors ( SDendaccess(sds_id) );

       //set projection
       if ( isLatLong )
       {
          imageInfo.strProj4 =strdup ( "+proj=latlong +datum=WGS84");
          printf ( "\tProj4 projection string from image is: %s\n", imageInfo.strProj4);
       }
       else
       {
          printf ( "\tSatellite image projection is not defined.%s\n", imageFile.c_str() );
          exit ( 1 );   
       }
       
       anyHDF4Errors ( SDend(sd_id) );

/*    
       for (index = 0; index < attributes; index++)
       {
          anyHDF4Errors ( SDattrinfo(sds_id, index, attr_name, &data_type, &count) );
          printf ( "\t att_index=%d    attr_name=%s   data_type=%d  count=%d\n", index, attr_name, data_type, count );
       }
*/

       /* Access and print the name of every data set in the file. */
 /*      for (index = 0; index < n_datasets; index++) 
       {
          anyHDF4Errors ( (sds_id = SDselect(sd_id, index) ) );
          anyHDF4Errors( SDgetinfo(sds_id, sds_name, &rank, dim_sizes,  &num_type, &attributes) );

		printf("name = %s\n", sds_name);
		printf("rank = %d\n", rank);
		printf("number of attributes = %i\n", attributes);

          anyHDF4Errors ( SDendaccess(sds_id) );
	}
*/
    }
  
    return imageInfo;
}


/****************************************/
/*  3.     projectShape                 */
/****************************************/
string projectShape (string gdalBinDir, string shapeFile, char *toProj4 )
{
    string tmp_str;           //store projected shapefile
    int    i;
    const char  *psztmpFilename = NULL;    //projected Shapefile name 
    GDALDriver  *poDriver = NULL;
    string      cmd_str;              //comand to call an executable program
  
    printf("\nProjecting Shapefile...\n" );

    //create shapefile name in toProj4 projection
    
    //get random file name in the current directoty
    string ext = string ( "_prj.shp" );
    tmp_str = getRandomFileName ( ext );

    psztmpFilename = tmp_str.c_str();  //temp shapefile name and only work at the currect directory

    printf("\tProjecting %s to %s and stored it in a temp file: %s\n", shapeFile.c_str(), toProj4, psztmpFilename);

/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
    GDALAllRegister();

    //if the file does exist delete it
    if (stat(psztmpFilename, &stFileInfo) == 0)
    {
       printf( "\tTemp projected shapefile exists and delete it: %s\n", psztmpFilename );

       poDriver = GetGDALDriverManager()->GetDriverByName( pszDriverName );
       if( poDriver == NULL )
       {
          printf( "%s driver not available.\n", pszDriverName );
          exit( 1 );
       }

       if ( (poDriver->Delete( psztmpFilename )) != CE_None )
       {
          printf( "\tError in deleting temp shapefile: %s\n\n", psztmpFilename );
       }
    }

    //build command line to call ogr2ogr
    cmd_str = string( gdalBinDir );
    cmd_str.append("ogr2ogr -t_srs \"");
    cmd_str.append(toProj4);
    cmd_str.append("\"  ");
    cmd_str.append(" ");
    cmd_str.append(psztmpFilename);
    cmd_str.append(" ");
    cmd_str.append( shapeFile.c_str() );
    printf("\t%s\n", cmd_str.c_str());

    //call ogr2ogr to project the shapefile
   
    if ( system(cmd_str.c_str()) != 0 )
    {
       printf("\tError in system call: %s\n", cmd_str.c_str());
       exit (1);
    }
  
    printf("\tCompleted in projecting the shapefile.\n\n");
 
    return psztmpFilename;

}


/****************************************/
/*   4.    computeRasterResolution      */
/****************************************/
double  computeRasterResolution ( gridInfo imageInfo, gridInfo grid )
{
    double rasterResolution;

    printf( "\nComputing raster resolution to rasterize domain grids...\n");


    //get smallest cell size for image and grid domain grids
    double temp_gridR = grid.xCellSize;
    if ( grid.xCellSize > grid.yCellSize )
         temp_gridR = grid.yCellSize;

    double temp_imageR = imageInfo.xCellSize;
    if ( imageInfo.xCellSize > imageInfo.yCellSize )
         temp_imageR = imageInfo.yCellSize;

    //if grid is LATLONG
    string tmp_str = string ( grid.strProj4 );
    if ( tmp_str.find( "+proj=latlong" ) !=string::npos )
    {
       //convert 1 degree to 111.1 km
       temp_gridR = temp_gridR * 111.1 * 1000.0;
       temp_gridR = floor ( temp_gridR );
    }

    //if image is LATLONG
    tmp_str = string ( imageInfo.strProj4 );
    if ( tmp_str.find( "+proj=latlong" ) !=string::npos )
    {
       //convert 1 degree to 111.1 km
       temp_imageR = temp_imageR * 111.1 * 1000.0;
       temp_imageR = floor ( temp_imageR );
    }
 
    printf ("\ttemp_gridR=%lf  temp_imageR=%lf\n",temp_gridR, temp_imageR);
    printGridInfo (grid);
    printGridInfo (imageInfo);


    double temp_ratio = temp_gridR / temp_imageR;

    if (temp_ratio >= 8.0 || temp_imageR==30.0 )
    {
       rasterResolution  = temp_imageR;           // use image resolution.  Like 30m NLCD
    }
    else if ( temp_ratio >= 5  && temp_ratio < 8.0 )
    {
       rasterResolution = temp_imageR / 2.0;      // 1/2 image resolution
    }
    else if ( temp_ratio >= 2.0  && temp_ratio < 5.0 )
    {
       rasterResolution = temp_imageR / 4.0;      // 1/4 image resolution
    }
    else if ( temp_ratio >= 1.0  && temp_ratio < 2.0 )
    {
       rasterResolution = temp_imageR / 8.0;      // 1/8 image resolution
    }
    else if (temp_ratio >= 0.25  && temp_ratio < 1.0 )
    {
       rasterResolution = temp_gridR / 4.0;       // 1/4 grid resolution
    }
    else
    {
       rasterResolution = temp_gridR;            //use grid resolution
    }


    rasterResolution = floor ( rasterResolution );
/*
    if ( rasterResolution > 100.00)  //if NLCD 30m, keep as it is
    {
       //make resolution a factor of grid resolution
       double  temp_f = ceil ( temp_gridR /rasterResolution );     

    
       while ( fmod (temp_gridR, temp_f ) != 0 )
       {
          temp_f ++;
       } 

       rasterResolution = temp_gridR / temp_f;
    }
*/
    
    printf ("\tRaster resolution to rasterize the domain grids is: %.4lf\n",rasterResolution);

    
    return rasterResolution;

}


/****************************************/
/*  5.     computeNewRasterInfo         */
/****************************************/
gridInfo computeNewRasterInfo (string shapeFile, double rasterResolution, gridInfo grid )
{
    gridInfo            newRasterInfo;

    //OGR variables
    GDALDataset         *poDS = NULL;
    GDALDriver          *poDriver = NULL;
    OGRLayer            *poLayer = NULL;
    OGREnvelope         oExt;
    OGRSpatialReference *oSRS;

    double   xMin,xMax,yMin,yMax;  
    int      xcells, ycells;

    
    printf( "\nComputing new raster information...\n");

    printf ( "\tRaster resolution is: %.2lf\n", rasterResolution );

    //compute new raster from grid
    if ( shapeFile.size() == 0 )
    {
       
       printf ( "\tCompute new raster info from an image \n" );

       //compute new extent to match resolution
       if ( rasterResolution == 30.0 )
       {
          // /NLCD grid center at 0,30,60: x or y /30 = *.5
          xMin = ( floor(grid.xmin / rasterResolution + 0.5 ) - 0.5 ) * rasterResolution;
          yMin = ( floor(grid.ymin / rasterResolution + 0.5 ) - 0.5 ) * rasterResolution;

          xMax = ( ceill(grid.xmax / rasterResolution + 0.5 ) - 0.5 ) * rasterResolution;
          yMax = ( ceill(grid.ymax / rasterResolution + 0.5 ) - 0.5 ) * rasterResolution;
       }
       else
       {
          xMin = ( floor(grid.xmin / rasterResolution) ) * rasterResolution;
          yMin = ( floor(grid.ymin / rasterResolution) ) * rasterResolution;

          xMax = ( ceill(grid.xmax / rasterResolution ) ) * rasterResolution;
          yMax = ( ceill(grid.ymax / rasterResolution ) ) * rasterResolution;
       }

       xcells = (int) ( (xMax - xMin) / rasterResolution );
       ycells = (int) ( (yMax - yMin) / rasterResolution );
  
       printf("\tModified extent minXY: (%f, %f) maxXY: (%f, %f)\n", xMin, yMin, xMax, yMax);
       printf("\txcells = %d   ycells = %d\n",xcells,ycells);

       string tmp_str = string ( grid.strProj4 );
      

       newRasterInfo.strProj4 = strdup ( tmp_str.c_str() );
       printf ( "\tProj4 projection is: %s\n", newRasterInfo.strProj4);

       //assign info to newRasterInfo
       newRasterInfo.xCellSize = rasterResolution;
       newRasterInfo.yCellSize = rasterResolution;
       newRasterInfo.rows = ycells;
       newRasterInfo.cols = xcells;
       newRasterInfo.xmin = xMin;
       newRasterInfo.ymin = yMin;
       newRasterInfo.xmax = xMax;
       newRasterInfo.ymax = yMax;

       return newRasterInfo; 
    }

    //compute new raster info from shapeFile
    printf ( "\tCompute new raster info from a shapefile: %s\n", shapeFile.c_str() );

/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
     GDALAllRegister ();

/* -------------------------------------------------------------------- */
/*      Open projected shapfile file.                                   */
/* -------------------------------------------------------------------- */

    poDS = (GDALDataset*) GDALOpenEx( shapeFile.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL );
    if( poDS == NULL )
    {
       printf( "\tError:  Opening Shapefile file failed -  %s.\n", shapeFile.c_str() );
       exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Get layer extent to make new raster domain.                      */
/* -------------------------------------------------------------------- */
    poLayer = poDS->GetLayer( 0 );
    if( poLayer == NULL )
    {
       printf( "\tError: Couldn't fetch layer %s!\n", shapeFile.c_str() );
       exit( 1 );
    }

    if (poLayer->GetExtent(&oExt, TRUE) == OGRERR_FAILURE)
    {
       printf("\tNo extent from projected shapefile\n", shapeFile.c_str() );
       exit ( 1 );
    }

    printf("\tProjected shapefile extent min_xy: (%.0lf, %.0lf) max_xy: (%.0lf, %.0lf)\n",
                   oExt.MinX, oExt.MinY, oExt.MaxX, oExt.MaxY);

    //compute new extent to match resolution
    //compute new extent to match resolution
    if ( rasterResolution == 30.0 )
    {
       // /NLCD grid center at 0,30,60: x or y /30 = *.5
       xMin = ( floor(oExt.MinX / rasterResolution + 0.5 ) - 0.5 ) * rasterResolution;
       yMin = ( floor(oExt.MinY / rasterResolution + 0.5 ) - 0.5 ) * rasterResolution;

       xMax = ( ceill(oExt.MaxX / rasterResolution + 0.5 ) - 0.5 ) * rasterResolution;
       yMax = ( ceill(oExt.MaxY / rasterResolution + 0.5 ) - 0.5 ) * rasterResolution;
    }
    else
    {
       xMin = ( floor(oExt.MinX / rasterResolution) ) * rasterResolution;
       yMin = ( floor(oExt.MinY / rasterResolution) ) * rasterResolution;

       xMax = ( ceill(oExt.MaxX / rasterResolution ) ) * rasterResolution;
       yMax = ( ceill(oExt.MaxY / rasterResolution ) ) * rasterResolution;
    }

    xcells = (int) ( (xMax - xMin) / rasterResolution );
    ycells = (int) ( (yMax - yMin) / rasterResolution );
    printf("\tModified extent min_xy: (%f, %f) max_xy: (%f, %f)\n", xMin, yMin, xMax, yMax);
    printf("\txcells = %d   ycells = %d\n",xcells,ycells);

    if ( ( oSRS = poLayer->GetSpatialRef() ) == NULL )
    {
       printf("\tNo projection from projected shapefile\n", shapeFile.c_str() );
       exit ( 1 );
    } 

    oSRS->exportToProj4( &newRasterInfo.strProj4 );
    printf ( "\tProj4 projection string is: %s\n", newRasterInfo.strProj4);

     GDALClose ( poDS );

    //assign info to newRasterInfo
    newRasterInfo.xCellSize = rasterResolution;
    newRasterInfo.yCellSize = rasterResolution;
    newRasterInfo.rows = ycells;
    newRasterInfo.cols = xcells;
    newRasterInfo.xmin = xMin;
    newRasterInfo.ymin = yMin;
    newRasterInfo.xmax = xMax;
    newRasterInfo.ymax = yMax;


    return newRasterInfo;
}


/***********************************/
/*  6.     toRasterFile            */
/***********************************/
string toRasterFile (string gdalBinDir, gridInfo newRasterInfo, string srcRasterFile, string shapeFile, gridInfo grid )
{
 
    const char  *pszDstFilename = NULL;
    int         i;
    string      cmd_str;
    char        extStr[150], sizStr[150];

    printf("\nRasterizing projected Shapefile...\n" );

    //get random file name in the current directoty
    string ext = string ( "_img.bil" );
    string tmp_str = getRandomFileName ( ext );

    pszDstFilename = tmp_str.c_str();  //temp rasterized file in current dir
    printf ("\tRasterized grid Shapefile is stored in file: %s\n",pszDstFilename);

/* -------------------------------------------------------------------- */
/*      Create a domain raster data set to store rasterized grid data   */
/* -------------------------------------------------------------------- */

    // build arguments to call gdal_translate
    cmd_str = string(gdalBinDir);
    cmd_str.append("gdal_translate -ot UInt32 ");  //create unsigned 32 byte int images to hold grid ID

    sprintf(extStr, "-a_ullr %f %f %f %f \0",newRasterInfo.xmin,newRasterInfo.ymax,newRasterInfo.xmax,newRasterInfo.ymin);
    cmd_str.append(extStr);

    sprintf(sizStr,"-scale 0 255 0 0 -outsize %d %d -of EHdr \0",newRasterInfo.cols,newRasterInfo.rows);
    cmd_str.append( sizStr );

    cmd_str.append( srcRasterFile.c_str() );
    cmd_str.append( "  " );
    cmd_str.append( pszDstFilename );

    printf("\t%s\n", cmd_str.c_str());
    
    //call gdal_translate to create 0 value domain grids
   
    if ( system(cmd_str.c_str()) != 0 )
    {
      printf("\tError in system call: %s\n", cmd_str.c_str());
      exit (1);
    }

    printf("\tSuccessful in creating 0 value domain image for rasterizing.\n");

/* -------------------------------------------------------------------- */
/*      Rasterize input shapefile to the created domain raster file     */
/* -------------------------------------------------------------------- */

    // build arguments to call gdal_rasterize
    cmd_str = string(gdalBinDir);
    cmd_str.append("gdal_rasterize -a ");
    cmd_str.append( grid.polyID.c_str() );
    cmd_str.append(" -l ");
    cmd_str.append( shapeFile );
   
    i = cmd_str.rfind(".shp", cmd_str.length());
    cmd_str.erase(i);  //get rid of .shp for layer name

    cmd_str.append ( " ");
    cmd_str.append ( shapeFile );
    cmd_str.append ( " ");
    cmd_str.append ( pszDstFilename );

    printf("\t%s\n", cmd_str.c_str());

    //call gdal_rasterize

    if ( system(cmd_str.c_str()) != 0 )
    {
      printf("\tError in system call: %s\n", cmd_str.c_str());
      exit (1);
    }
 
    printf ("\tCompleted rasterizing the shapefile.\n\n");

    return pszDstFilename;
   
}


/****************************************/
/*   7.    deleteShapeFile              */
/****************************************/       
void deleteShapeFile ( string shapeFile )
{
   GDALDriver *poDriver = NULL;

/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
   GDALAllRegister ();

   //check the shapefile.  If it exists delete it
   if (stat(shapeFile.c_str(), &stFileInfo) == 0)
   {
       printf( "\nShapefile exists and delete it: %s\n", shapeFile.c_str() );

       poDriver = GetGDALDriverManager()->GetDriverByName( pszDriverName );
       if( poDriver == NULL )
       {
          printf( "\n%s driver not available.\n", pszDriverName );
          exit( 1 );
       }

       if ( (poDriver->Delete( shapeFile.c_str() )) != OGRERR_NONE )
       {
          printf( "\nError in deleting Shapefile: %s\n\n", shapeFile.c_str() );
          exit ( 1 );
       }
   }
}


/****************************************/
/*  8.    deleteRasterFile              */
/****************************************/
void deleteRasterFile ( string rasterFile )
{

   GDALDataset     *poRDataset;
   GDALDriver      *poDrive;

/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
   GDALAllRegister();
 
   //use .xml to check DUE TO .bil does not work properly sometimes
   string tmp_str = string( rasterFile );
   tmp_str.append(".aux.xml");

   //if the file exists delete it
   if ( stat(tmp_str.c_str(), &stFileInfo) == 0 || stat(rasterFile.c_str(), &stFileInfo) == 0 )
   {
       printf( "\nRaster file exists and delete it: %s\n", rasterFile.c_str() );
       poRDataset = (GDALDataset *) GDALOpen( rasterFile.c_str(), GA_ReadOnly );
       if( poRDataset == NULL )
       {
          printf( "\nOpening raster file failed - %s.\n", rasterFile.c_str() );
          exit( 1 );
       } 
       poDrive = poRDataset->GetDriver();
       GDALClose( (GDALDatasetH) poRDataset );

       if ( poDrive->Delete( rasterFile.c_str() )  == CE_Failure )
       {
          printf( "\nDeleting the file failed: %s\n", rasterFile.c_str() );
          exit( 1 );
       } 
   }
 
}


/*********************************************
*           9.   computeLatLong              * 
*********************************************/
projUV   computeLatLong(projPJ proj4DF, double x, double y)
{
    projUV  latlongP, xyP;

    xyP.u = x;
    xyP.v = y;

    latlongP = pj_inv(xyP, proj4DF);
    if (latlongP.u == HUGE_VAL)
    {
       printf( "Error in computing lat and long for point: %lf,%lf\n",x,y);
       exit ( 1 );
    }

    latlongP.u *= RAD_TO_DEG;
    latlongP.v *= RAD_TO_DEG;

    return latlongP;
}


/*********************************************
*  10.   computeGridCornerCenterLatLong      * 
*********************************************/
void computeGridCornerCenterLatLong ( gridInfo *grid )
{
    projPJ  proj4DF;
    projUV  latlongP;
    double xx, yy;

    string proj4Str = string(grid->strProj4);
    proj4DF = pj_init_plus (proj4Str.c_str() );
    if (proj4DF == NULL)
    {
        printf( "\tInitializing Proj4 projection string failed: .\n", grid->strProj4 );
        exit ( 1 );
    }

    //get the maximum x and y for the domain
     grid->xmax = grid->xmin + grid->xCellSize*grid->cols;
     grid->ymax = grid->ymin + grid->yCellSize*grid->rows;

     if ( pj_is_latlong(proj4DF) )
     {
        printf( "The grid domain is defined in the lat and long coordinates.\n" );
        grid->cenLon = grid->xmin + (grid->xmax - grid->xmin) / 2.0;
        grid->cenLat = grid->ymin + (grid->ymax - grid->ymin) / 2.0;

        //SW corner point
        grid->cornerLons[0] = grid->xmin + grid->xCellSize/2.0;
        grid->cornerLats[0] = grid->ymin + grid->yCellSize/2.0;

        //NW corner point
        grid->cornerLons[1] = grid->xmin + grid->xCellSize/2.0;
        grid->cornerLats[1] = grid->ymax - grid->yCellSize/2.0;

        //NE corner point
        grid->cornerLons[2] = grid->xmax - grid->xCellSize/2.0;
        grid->cornerLats[2] = grid->ymax - grid->yCellSize/2.0;

        //SE corner point
        grid->cornerLons[3] = grid->xmax - grid->xCellSize/2.0;
        grid->cornerLats[3] = grid->ymin + grid->yCellSize/2.0;
     }
     else
     {
        //get center point lat and long of WRF mass grid (cross)
        xx = grid->xmin + (grid->xmax - grid->xmin) / 2.0;
        yy = grid->ymin + (grid->ymax - grid->ymin) / 2.0;

        latlongP = computeLatLong(proj4DF,xx,yy);
        grid->cenLon = latlongP.u;
        grid->cenLat = latlongP.v;
        printf ( "\tCenter point: xx = %lf   yy = %lf  cLon = %lf  cLat = %lf\n",xx,yy,grid->cenLon,grid->cenLat);

        //SW corner point
        xx = grid->xmin + grid->xCellSize/2.0;
        yy = grid->ymin + grid->yCellSize/2.0;
        latlongP = computeLatLong(proj4DF,xx,yy);
        grid->cornerLons[0] = latlongP.u;
        grid->cornerLats[0] = latlongP.v;
        printf ( "\tSW point: xx = %lf   yy = %lf  swLon = %lf  swLat = %lf\n",xx,yy,grid->cornerLons[0],grid->cornerLats[0]);

        //NW corner point
        xx = grid->xmin + grid->xCellSize/2.0;
        yy = grid->ymax - grid->yCellSize/2.0;;
        latlongP = computeLatLong(proj4DF,xx,yy);
        grid->cornerLons[1]  = latlongP.u;
        grid->cornerLats[1] = latlongP.v;
        printf ( "\tNW point: xx = %lf   yy = %lf  nwLon = %lf  nwLat = %lf\n",xx,yy,grid->cornerLons[1],grid->cornerLats[1]);

        //NE corner point
        xx = grid->xmax - grid->xCellSize/2.0;
        yy = grid->ymax - grid->yCellSize/2.0;
        latlongP = computeLatLong(proj4DF,xx,yy);
        grid->cornerLons[2] = latlongP.u;
        grid->cornerLats[2] = latlongP.v;
        printf ( "\tNE point: xx = %lf   yy = %lf  neLon = %lf  neLat = %lf\n",xx,yy,grid->cornerLons[2],grid->cornerLats[2]);

        //SE corner point
        xx = grid->xmax - grid->xCellSize/2.0;
        yy = grid->ymin + grid->yCellSize/2.0;
        latlongP = computeLatLong(proj4DF,xx,yy);
        grid->cornerLons[3] = latlongP.u;
        grid->cornerLats[3] = latlongP.v;
        printf ( "\tSE point: xx = %lf   yy = %lf  seLon = %lf  seLat = %lf\n\n",xx,yy,grid->cornerLons[3],grid->cornerLats[3]);
     }

     pj_free ( proj4DF );

}

/*********************************************
*     11.   anyErrors                        *
* Handle NetCDF library error status         *
*********************************************/
void anyErrors(int e)
{
        if (e != NC_NOERR)
        {
                printf("\nNetCDF error: %s\n", nc_strerror(e));
                exit ( 1 );
        }
}


/*********************************************
*       12. computeGridCoordinates           *
*********************************************/
void computeGridCoordinates ( gridInfo *grid )
{
    projPJ  proj4DF;
    projUV  latlongP;
    double xx, yy;
    int  i,j;
        
    string proj4Str = string(grid->strProj4);
    proj4DF = pj_init_plus (proj4Str.c_str() );
    if (proj4DF == NULL)
    {   
        printf( "\tInitializing Proj4 projection string failed: .\n", grid->strProj4 );
        exit ( 1 );
    }   

    //x and y array 1d
    if ( (grid->x = (float*) calloc (grid->cols, sizeof(float)) ) == NULL)
    {
        printf( "Calloc x failed.\n");
        exit ( 1 );
    }
    if ( (grid->y = (float*) calloc (grid->rows, sizeof(float)) ) == NULL)
    {
        printf( "Calloc y failed.\n");
        exit ( 1 );
    }

    ///lat and long array 2d
    if ( (grid->lat = (float*) calloc (grid->rows*grid->cols, sizeof(float)) ) == NULL)
    {
        printf( "Calloc lat failed.\n");
        exit ( 1 );
    }
    if ( (grid->lon = (float*) calloc (grid->rows*grid->cols, sizeof(float)) ) == NULL)
    {
        printf( "Calloc lon failed.\n");
        exit ( 1 );
    }    

     //mass x
     for (i=0; i<grid->cols; i++)
     { 
         grid->x[i] = grid->xmin + grid->xCellSize*i + grid->xCellSize/2.0;
     }

     //mass y
     for (j=0; j<grid->rows; j++)
     {
        grid->y[j] = grid->ymin + grid->yCellSize*j + grid->yCellSize/2.0;
     }

     for(j=0; j<grid->rows; j++)
     {
        yy = grid->ymin + grid->yCellSize*j + grid->yCellSize/2.0;
        for (i=0; i<grid->cols; i++)
        {
           xx = grid->xmin + grid->xCellSize*i + grid->xCellSize/2.0;

           if ( proj4Str.find ( "latlong" ) != string::npos )
           {
              grid->lon[j*grid->cols+i] = xx;
              grid->lat[j*grid->cols+i] = yy;
           }
           else
           {
              latlongP = computeLatLong(proj4DF,xx,yy);
              grid->lon[j*grid->cols+i] = latlongP.u;
              grid->lat[j*grid->cols+i] = latlongP.v;
           }

           //printf ("xx=%f  yy=%f  long=%f  lat=%f\n", xx,yy,grid->lon[j*grid->cols+i], grid->lat[j*grid->cols+i]);
        }
     }

     pj_free ( proj4DF );

}


/*********************************************
*       13. writeWRFGridCoordinates          *
*********************************************/
void writeWRFGridCoordinates ( int ncid, int west_east_dim, int south_north_dim, gridInfo grid )
{
     int   x_id, y_id;
     int   lon_id, lat_id;
     int   fieldtype[1];
     int   dimIndex[NC_MAX_DIMS];     //dimension index array for write out array

    
     fieldtype[0] = 104;

     printf( "\nDefine grid coordinate variables in output netcdf file...\n" );

     /******************************
     * Set define mode for output  *
     *******************************/
     anyErrors( nc_redef (ncid) );

     dimIndex[0] = west_east_dim;
     anyErrors( nc_def_var(ncid, "X_M", NC_FLOAT, 1, dimIndex, &x_id) );
     anyErrors( nc_put_att_int(ncid, x_id, "FieldType", NC_INT, 1, fieldtype) );
     anyErrors( nc_put_att_text(ncid, x_id, "MemoryOorder", 2, "X ") );
     anyErrors( nc_put_att_text(ncid, x_id, "description", 14, "X on mass grid") );
     anyErrors( nc_put_att_text(ncid, x_id, "units", 6, "Meters") );
     anyErrors( nc_put_att_text(ncid, x_id, "stagger", 1, "M") );

     dimIndex[0] = south_north_dim;
     anyErrors( nc_def_var(ncid, "Y_M", NC_FLOAT, 1, dimIndex, &y_id) );
     anyErrors( nc_put_att_int(ncid, y_id, "FieldType", NC_INT, 1, fieldtype) );
     anyErrors( nc_put_att_text(ncid, y_id, "MemoryOrder", 2, "Y ") );
     anyErrors( nc_put_att_text(ncid, y_id, "description", 14, "Y on mass grid") );
     anyErrors( nc_put_att_text(ncid, y_id, "units", 6, "Meters") );
     anyErrors( nc_put_att_text(ncid, y_id, "stagger", 1, "M") );

     dimIndex[0] = south_north_dim;
     dimIndex[1] = west_east_dim;
     anyErrors( nc_def_var(ncid, "XLONG_M", NC_FLOAT, 2, dimIndex, &lon_id) );
     anyErrors( nc_def_var(ncid, "XLAT_M", NC_FLOAT, 2, dimIndex, &lat_id) );

     anyErrors( nc_put_att_int(ncid, lon_id, "FieldType", NC_INT, 1, fieldtype) );
     anyErrors( nc_put_att_text(ncid, lon_id, "MemoryOrder", 3, "XY ") );
     anyErrors( nc_put_att_text(ncid, lon_id, "description", 22, "longitude on mass grid") );
     anyErrors( nc_put_att_text(ncid, lon_id, "units", 17, "degrees longitude") );
     anyErrors( nc_put_att_text(ncid, lon_id, "stagger", 1, "M") );

     anyErrors( nc_put_att_int(ncid, lat_id, "FieldType", NC_INT, 1, fieldtype) );
     anyErrors( nc_put_att_text(ncid, lat_id, "MemoryOrder", 3, "XY ") );
     anyErrors( nc_put_att_text(ncid, lat_id, "description", 21, "latitude on mass grid") );
     anyErrors( nc_put_att_text(ncid, lat_id, "units", 16, "degrees latitude") );
     anyErrors( nc_put_att_text(ncid, lat_id, "stagger", 1, "M") );


     /*******************************
     * Leave define mode for output *
     ********************************/
     anyErrors( nc_enddef (ncid) );

     anyErrors( nc_put_var_float(ncid, x_id, grid.x) );
     printf( "\tWrote X\n" );

     anyErrors( nc_put_var_float(ncid, y_id, grid.y) );
     printf( "\tWrote Y\n" );

     anyErrors( nc_put_var_float(ncid, lon_id, grid.lon) );
     printf( "\tWrote LONG\n" );

     anyErrors( nc_put_var_float(ncid, lat_id, grid.lat) );
     printf( "\tWrote LAT\n" );

}



/*********************************************
*       14. writeWRFGlobalAtributes          *
*********************************************/
void writeWRFGlobalAtributes ( int ncid, string title, gridInfo grid, string startDateTime )
{

    //global attributes
    time_t   rawtime;
    int      gatt[1];
    float    gattf[1];

    int      proj;     //WRF projection type
    float    stdLon,stdLat;
    float    trueLat1, trueLat2;
    string   temp_proj, tmp_str;


    printf( "\nDefine global attributes in output netcdf file...\n\n" );

    //put the output in define mode
    anyErrors( nc_redef (ncid) );

    anyErrors( nc_put_att_text(ncid, NC_GLOBAL, "TITLE", title.size(), title.c_str() ) );

    time ( &rawtime ); 
    tmp_str = string ( ctime (&rawtime) );
    tmp_str = string ( "Created on ") + tmp_str;
    tmp_str.erase (tmp_str.size() -1, 1);

    anyErrors( nc_put_att_text(ncid, NC_GLOBAL, "HISTORY", tmp_str.size(), tmp_str.c_str() ) );

    anyErrors( nc_put_att_text(ncid, NC_GLOBAL, "SIMULATION_START_DATE", startDateTime.size(), startDateTime.c_str()) );

    gatt[0] = grid.cols+1;
    anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "WEST-EAST_GRID_DIMENSION", NC_INT, 1, gatt) );

    gatt[0] = grid.rows+1;
    anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "SOUTH-NORTH_GRID_DIMENSION", NC_INT, 1, gatt) );

    gatt[0] = 0;
    anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "BOTTOM-TOP_GRID_DIMENSION", NC_INT, 1, gatt) );

    gatt[0] = 1;
    anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "WEST-EAST_PATCH_START_UNSTAG", NC_INT, 1, gatt) );
    gatt[0] = grid.cols;
    anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "WEST-EAST_PATCH_END_UNSTAG", NC_INT, 1, gatt) );

    gatt[0] = 1;
    anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "SOUTH-NORTH_PATCH_START_UNSTAG", NC_INT, 1, gatt) );
    gatt[0] = grid.rows;
    anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "SOUTH-NORTH_PATCH_END_UNSTAG", NC_INT, 1, gatt) );

    anyErrors( nc_put_att_text(ncid, NC_GLOBAL, "GRIDTYPE", 1, "C") );

    gattf[0] = grid.xmin;
    anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "XMIN", NC_FLOAT, 1, gattf) );
    gattf[0] = grid.ymin;
    anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "YMIN", NC_FLOAT, 1, gattf) );
    gattf[0] = grid.xCellSize;
    anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "DX", NC_FLOAT, 1, gattf) );
    gattf[0] = grid.yCellSize; 
    anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "DY", NC_FLOAT, 1, gattf) );


    gattf[0] = grid.cenLat;
    anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "CEN_LAT", NC_FLOAT, 1, gattf) );
    gattf[0] = grid.cenLon;
    anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "CEN_LON", NC_FLOAT, 1, gattf) );
    gattf[0] = grid.cenLat;
    anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "MOAD_CEN_LAT", NC_FLOAT, 1, gattf) );

    //get projection parameters from Proj4 string
    temp_proj = string (grid.strProj4);
    proj = getProjType(temp_proj);

    
    /*Sample Proj4 definitations:
      LCC:     +proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=33 +lat_2=45 +lat_0=40 +lon_0=-97
      LATLONG: +proj=latlong +a=6370000.0 +b=6370000.0
      UPS:     +proj=stere +a=6370000.0 +b=6370000.0 +lat_ts=33 +lat_0=90 +lon_0=-97 +k_0=1.0
      MERC:    +proj=merc +a=6370000.0 +b=6370000.0 +lat_ts=33 +lon_0=0
    */


    float  MISSING = 1.e+20;
   
    if (proj == LCC)
    {
       //projType 1
       trueLat1 = getValueInProj4Str ( temp_proj, "lat_1=");
       trueLat2 = getValueInProj4Str ( temp_proj, "lat_2=");
       stdLat = getValueInProj4Str ( temp_proj, "lat_0=");
       stdLon = getValueInProj4Str ( temp_proj, "lon_0=");
    }
    else if ( proj == LATLONG )
    {
       //projType = 6;
       trueLat1 = MISSING;
       trueLat2 = MISSING;
       stdLat = 0.0;
       stdLon = 0.0;
    }
    else if ( proj == UPS )
    {
       //projType = 2;
       trueLat1 = getValueInProj4Str ( temp_proj, "lat_ts=");
       trueLat2 = MISSING;
       stdLat = trueLat1;
       stdLon = getValueInProj4Str ( temp_proj, "lon_0=");
    }
    else if ( proj == MERC )
    {
       //projType = 3;
       trueLat1 = getValueInProj4Str ( temp_proj, "lat_ts=");
       trueLat2 = MISSING;
       stdLat = 0.0;
       stdLon = getValueInProj4Str ( temp_proj, "lon_0=");
    }


    gattf[0] = trueLat1;
    anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "TRUELAT1", NC_FLOAT, 1, gattf) );

    gattf[0] = trueLat2;
    anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "TRUELAT2", NC_FLOAT, 1, gattf) );

    gattf[0] = stdLon;
    anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "STAND_LON", NC_FLOAT, 1, gattf) );

    gattf[0]= stdLat;
    anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "STAND_LAT", NC_FLOAT, 1, gattf) );


    if (grid.cornerLats[0] >= 0.0 && grid.cornerLats[1] > 0.0)
    {
       gattf[0] = 90.0;
    }
    else if (grid.cornerLats[0] < 0.0 && grid.cornerLats[1] <= 0.0)
    {
      gattf[0] = -90.0;
    }
    else
    {
      gattf[0] = MISSING;
    }
    anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "POLE_LAT", NC_FLOAT, 1, gattf) );

    gattf[0] = 0.0;
    anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "POLE_LON", NC_FLOAT, 1, gattf) );


    anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "corner_lats", NC_FLOAT, 4, grid.cornerLats) );
    anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "corner_lons", NC_FLOAT, 4, grid.cornerLons) );


    //for WRF simulation - 40 NLCD data
    string gmtS = startDateTime.substr(8, 4); 
    string yearS = startDateTime.substr(0, 4);

    tmp_str = startDateTime.substr(0, 8);
    string dateJulian = getJulianDayStr ( tmp_str );
    string dayS =  dateJulian.substr(4, 3);
    
    gattf[0] = atof ( gmtS.c_str() );
    anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "GMT", NC_FLOAT, 1, gattf) );

    gatt[0] = atoi ( yearS.c_str() );
    anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "JULYR", NC_INT, 1, gatt) );
  
    gatt[0] = atoi ( dayS.c_str() );
    anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "JULDAY", NC_INT, 1, gatt) );

    gatt[0] = proj;
    anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "MAP_PROJ", NC_INT, 1, gatt) );


    
    tmp_str = string ( "NLCD40" );
    anyErrors( nc_put_att_text(ncid, NC_GLOBAL, "MMINLU", tmp_str.size(), tmp_str.c_str() ) );    

    tmp_str = string ( "40" );
    anyErrors( nc_put_att_text(ncid, NC_GLOBAL, "NUM_LAND_CAT", tmp_str.size(), tmp_str.c_str() ) ); 
    
 

      
    /********************
    * Leave define mode *
    ********************/
    anyErrors( nc_enddef (ncid) );

}


/******************************************
*           15.   projectPoint            *
*******************************************/
projUV   projectPoint(projPJ proj4From, projPJ proj4To, double x, double y)
{
    projUV  toP;
    double  xx, yy;
    

    xx = x;
    yy = y;
   
    if ( pj_is_latlong( proj4From ) )
    {
       xx *= DEG_TO_RAD;
       yy *= DEG_TO_RAD;
    }

    if ( xx != HUGE_VAL) 
    {
       if( pj_transform( proj4From, proj4To, 1, 1, &xx, &yy, NULL ) != 0 )
       {
          xx = HUGE_VAL;
          yy = HUGE_VAL;
       }
    }

   
    if (xx != HUGE_VAL)
    {

       if ( pj_is_latlong( proj4To ) )
       {
          xx *= RAD_TO_DEG;
          yy *= RAD_TO_DEG;
       }
    }

    toP.u = xx;
    toP.v = yy;

    //printf ("\tOld: x=%lf   y=%lf\n", x,y);
    //printf ("\tNew: x=%lf   y=%lf\n", xx,yy);

    return toP;
}


/*************************************************************************/
/*    16.  get a day time step range from MCIP NetCDF time variable      */
/*************************************************************************/
void  getDayTimeStepRange ( int *dayTimeStepRange, int mcipDay, int varID, int *timeVar, int var_ndims, size_t *varDimSize )
{
    int   i, j, k;
    int   index;
    int   NODATA = -9;
    int   indexStart = NODATA;
    int   indexEnd = NODATA;

    printf ("\tvarID = %d   mcipDay = %d   varDims = %d\n", varID, mcipDay, var_ndims );

    if ( var_ndims == 3 )
    {
       //process MCIP TFLAG variable
       for (i=0; i<varDimSize[0]; i++ )
       {
          index = i * varDimSize[1] * varDimSize[2] +  varID * varDimSize[2];
          //printf ("\t\t = %d   Index= %d    date=%d   time=%d\n", i, index, timeVar [index], timeVar [index+1]);

          if ( timeVar[index] == mcipDay && indexStart == NODATA )
          {
             indexStart = i;
          }

          if ( timeVar [index] != mcipDay && indexStart != NODATA && indexEnd == NODATA )
          {
             indexEnd = i - 1;
             break;    //get out of loop
          }

       }
     
       if ( indexStart != NODATA && indexEnd == NODATA )
       {
           indexEnd = i - 1;
       }
    
       if ( indexStart == NODATA && indexEnd == NODATA )
       {
           printf ("\tNo time step date in TFLAG for this variable.\n" );
           exit ( 1 );

       }
       printf ("\tDay time step index range: %d  to  %d\n", indexStart, indexEnd);
 
       dayTimeStepRange[0] = indexStart;
       dayTimeStepRange[1] = indexEnd;      
    }

}


/*************************************************************************/
/*    17. put a NetCDF grid information in a grid variable               */
/*************************************************************************/
void getGridInfofromNC ( int ncid , gridInfo *grid, const char *ncType )
{

    int       gType;
    int       rows, cols;
    string    strProj4;
    float     lat1, lat2, lat0, long0;
    float     xmin, ymin;
    float     xcell, ycell;
    char      proj_chars[250];
    int       i,j;


    printf ( "\tGetting IOAPI file grid information...\n");
    if ( strcmp ( ncType, "IOAPI" ) == 0 )
    {

       anyErrors ( nc_get_att_int ( ncid, NC_GLOBAL, "GDTYP", &gType) );

       anyErrors ( nc_get_att_int ( ncid, NC_GLOBAL, "NCOLS", &cols) );
       anyErrors ( nc_get_att_int ( ncid, NC_GLOBAL, "NROWS", &rows) );

       anyErrors ( nc_get_att_float ( ncid, NC_GLOBAL, "P_ALP", &lat1) );
       anyErrors ( nc_get_att_float ( ncid, NC_GLOBAL, "P_BET", &lat2) );
       anyErrors ( nc_get_att_float ( ncid, NC_GLOBAL, "YCENT", &lat0) );
       anyErrors ( nc_get_att_float ( ncid, NC_GLOBAL, "P_GAM", &long0) );

       anyErrors ( nc_get_att_float ( ncid, NC_GLOBAL, "XORIG", &xmin) );
       anyErrors ( nc_get_att_float ( ncid, NC_GLOBAL, "YORIG", &ymin) );

       anyErrors ( nc_get_att_float ( ncid, NC_GLOBAL, "XCELL", &xcell) );
       anyErrors ( nc_get_att_float ( ncid, NC_GLOBAL, "YCELL", &ycell) );
 
       if (gType == 2 )
       {
          sprintf( proj_chars, "+proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=%.3f +lat_2=%.3f +lat_0=%.3f +lon_0=%.3f\0",
                   lat1, lat2, lat0, long0 );
          strProj4 = string ( proj_chars );
          //printf ( "MCIP POJ4: %s\n", strProj4.c_str() );
       }
       else
       {
          printf( "\t MCIP projection type %d is not implemented in getGridInfofromNC.\n", gType); 
          exit ( 1 );
       }   
     
       grid->rows = rows;
       grid->cols = cols;
       grid->xmin = xmin;
       grid->ymin = ymin;
       grid->xCellSize = xcell;
       grid->yCellSize = ycell;

       grid->strProj4 = strdup (strProj4.c_str() );
 
    }
    if ( strcmp ( ncType, "WRFNC" ) == 0 )
    {

       anyErrors ( nc_get_att_int ( ncid, NC_GLOBAL, "MAP_PROJ", &gType) );

       anyErrors ( nc_get_att_int ( ncid, NC_GLOBAL, "WEST-EAST_PATCH_END_UNSTAG", &cols) );
       anyErrors ( nc_get_att_int ( ncid, NC_GLOBAL, "SOUTH-NORTH_PATCH_END_UNSTAG", &rows) );

       anyErrors ( nc_get_att_float ( ncid, NC_GLOBAL, "TRUELAT1", &lat1) );
       anyErrors ( nc_get_att_float ( ncid, NC_GLOBAL, "TRUELAT2", &lat2) );
       anyErrors ( nc_get_att_float ( ncid, NC_GLOBAL, "STAND_LAT", &lat0) );
       anyErrors ( nc_get_att_float ( ncid, NC_GLOBAL, "STAND_LON", &long0) );

       anyErrors ( nc_get_att_float ( ncid, NC_GLOBAL, "XMIN", &xmin) );
       anyErrors ( nc_get_att_float ( ncid, NC_GLOBAL, "YMIN", &ymin) );

       anyErrors ( nc_get_att_float ( ncid, NC_GLOBAL, "DX", &xcell) );
       anyErrors ( nc_get_att_float ( ncid, NC_GLOBAL, "DY", &ycell) );
 
       if (gType == 1 )
       {
          sprintf( proj_chars, "+proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=%.3f +lat_2=%.3f +lat_0=%.3f +lon_0=%.3f\0",
                   lat1, lat2, lat0, long0 );
          strProj4 = string ( proj_chars );
          //printf ( "BELD4 netCDF file POJ4: %s\n", strProj4.c_str() );
       }
       else if ( gType == 6 )
       {
         sprintf( proj_chars, "+proj=latlong +a=6375000.0 +b=6375000.0 =6370000.0\0");
          strProj4 = string ( proj_chars );
          //printf ( "BELD4 netCDF file POJ4: %s\n", strProj4.c_str() );

       } 
       else
       {
          printf( "\t BELD4 netCDF file projection type %d is not implemented in getGridInfofromNC.\n", gType); 
          exit ( 1 );
       }   
     
       grid->rows = rows;
       grid->cols = cols;
       grid->xmin = xmin;
       grid->ymin = ymin;
       grid->xCellSize = xcell;
       grid->yCellSize = ycell;

       grid->strProj4 = strdup (strProj4.c_str() );
 
    }
    else if ( strcmp ( ncType, "NARR" ) == 0 )
    {
       int      ndims, nvars, ngatts, unlimdimid;
       size_t   *dimSizes, dimSize;
       char     dimName[NC_MAX_NAME+1];
       char     attName[NC_MAX_NAME+1];       //name for an attribute
       float    latCorners[4],longCorners[4];
       const char    *latVarName = "lat";
       const char    *lonVarName = "lon";
       int           var_id; 
 
       printf( "\tObtaining all dimension IDs in input NetCDF file...\n" );
       anyErrors( nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid) );
       printf("\t NARR file has: %d dims, %d variables, %d global attributes, %d unlimited variable ID\n", ndims, nvars,ngatts,unlimdimid);

       //store dim size in an arrary
       dimSizes = (size_t *) malloc(sizeof(size_t) * ndims);
       if ( dimSizes == NULL )
       {
          printf ( "\t Memory allocation malloc failed for dimSizes\n" );
          exit ( 1 );
       }

       for (i=0; i<ndims; i++)
       {
          anyErrors(  nc_inq_dim(ncid, i, dimName, &dimSize ) );
          dimSizes[i]= dimSize;
          grid->dims.push_back ( dimSize );

          printf("\tNARR file dimension: dimName = %10s   dimSize = %d\n",dimName,dimSize);
       }


       anyErrors ( nc_get_att_float ( ncid, NC_GLOBAL, "latcorners", grid->cornerLats) );
       anyErrors ( nc_get_att_float ( ncid, NC_GLOBAL, "loncorners", grid->cornerLons) );

       sprintf( proj_chars, "+proj=latlong +datum=NAD83\0");
       strProj4 = string ( proj_chars );

       grid->rows = dimSizes[1];
       grid->cols = dimSizes[2];
       grid->xmin = grid->cornerLats[0];
       grid->ymin = grid->cornerLons[0];
       grid->xCellSize = 0.3;
       grid->yCellSize = 0.3;

       grid->strProj4 = strdup (strProj4.c_str() );

       //get lat and long arrays
       grid->lat = allocateFloatDataArrayMemory ( grid->rows * grid->cols );
       grid->lon = allocateFloatDataArrayMemory ( grid->rows * grid->cols );

       //get var ID
       nc_inq_varid (ncid, lonVarName, &var_id);

       //get lon array
       anyErrors( nc_get_var_float(ncid, var_id, grid->lon) );

       //get var ID
       nc_inq_varid (ncid, latVarName, &var_id);

       //get lat array
       anyErrors( nc_get_var_float(ncid, var_id, grid->lat) );

    }

}

/******************************************/
/*  18. compare two grid information      */
/******************************************/
bool sameGrids ( gridInfo grid1, gridInfo grid2 )
{
    OGRSpatialReference   oSRS_grd1, oSRS_grd2;   //set GDAL projection classes for two grids
    
    if ( grid1.rows != grid2.rows )   return false;
    if ( grid1.cols != grid2.cols )   return false;

    if ( grid1.xmin != grid2.xmin )   return false;
    if ( grid1.ymin != grid2.ymin )   return false;

    if ( grid1.xCellSize != grid2.xCellSize ) return false;
    if ( grid1.yCellSize != grid2.yCellSize ) return false;

    oSRS_grd1.importFromProj4( grid1.strProj4 );       
    oSRS_grd2.importFromProj4( grid2.strProj4 );
   
    if (! oSRS_grd1.IsSame (&oSRS_grd2) )
    {
        printf( "\tError: Two grids have different PROJ4 projections:\n" );
        printf( "\t\tGrid1 PROJ4: %s\n", grid1.strProj4);
        printf( "\t\tGrid2 PROJ4: %s\n", grid2.strProj4);
        return false;
    }

    return true;


}


/******************************************/
/*   19. printf grid information           */
/******************************************/
void printGridInfo ( gridInfo grid )
{

   // print out key information of the gridInfo variable content
   printf( "\n\tRows=%d    Cols=%d    Total gird cells=%ld\n",grid.rows,grid.cols,grid.rows*grid.cols);
   printf( "\txmin=%f    ymin=%f \n", grid.xmin, grid.ymin);
   printf( "\txmax=%f    ymax=%f \n", grid.xmax, grid.ymax);
   printf( "\txcell=%.8f    ycell=%.8f \n",grid.xCellSize,grid.yCellSize);
   printf( "\tproj4=%s\n",grid.strProj4);

}


/*********************************************
*       20. anyHDF4Errors                    *
*   Handle HDF4 library error status         *
*********************************************/
void anyHDF4Errors( int  e )
{
   if (e == FAIL)
   {
      printf( "\nHDF4 error." );
      exit ( 1 );
   }
}

/*********************************************
*       21. createGridImage                  *
*           Generate grid raster file
*********************************************/
string  createGridImage( gridInfo grid )
{
   const char     *pszFormat = "EHdr";
   GDALDataset    *poDstDS;       
   OGRSpatialReference oSRS;
   char           *pszWKT = NULL;

   GDALDriver     *poDriver;
   GDALRasterBand *poBand;
   char           **papszOptions = NULL;
   char           **papszMetadata;


   printf("\nGenerating grid raster file for a given domain grid...\n");

   //get random file name in the current directoty
   string ext = string ( "_img.bil" );
   string imageName = getRandomFileName ( ext );

   /* -------------------------------------------------------------------- */
   /*      Register format(s).                                             */
   /* -------------------------------------------------------------------- */
   GDALAllRegister();

   poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);

   if( poDriver == NULL )
   {
      printf ( "\tCreating a GDAL format driver failed: %s\n", pszFormat);
      exit( 1 );
   }

   poDstDS = poDriver->Create( imageName.c_str(), grid.cols, grid.rows, 1, GDT_UInt32, papszOptions );
   double adfGeoTransform[6] = { grid.xmin, grid.xCellSize, 0, grid.ymax, 0, -1*grid.yCellSize };

   poDstDS->SetGeoTransform( adfGeoTransform );
    
   string strProj4_cc = string ( grid.strProj4 );

   oSRS.importFromProj4( strProj4_cc.c_str() );
   oSRS.exportToWkt( &pszWKT );

   if ( (poDstDS->SetProjection( pszWKT )) == CE_Failure )
   {
      printf ( "\tSetting projection failed for the grid raster file: %s\n", imageName.c_str() );
      exit ( 1 );
   }

   poBand = poDstDS->GetRasterBand(1);

   GUInt32 *poImage = (GUInt32 *) CPLCalloc(sizeof(GUInt32),grid.rows*grid.cols);

   //assign grid ID from LL to UR corner 1 to rows*cols, different from image: UL to LR
   int gridID = 0;
   for (int i=0; i<grid.rows; i++)
   {
      for (int j=0; j<grid.cols; j++)
      {
         gridID = (grid.rows - 1 - i) * grid.cols + j + 1;
         int pixelIndex = grid.cols * i + j;
         poImage[pixelIndex] = gridID;
         //printf ("\trow=%d  col=%d   gridID = %d     Value = %d\n",i,j, gridID,  poImage[pixelIndex] );
      }
   }

   if ( (poBand->RasterIO( GF_Write, 0, 0, grid.cols, grid.rows, poImage, 
         grid.cols, grid.rows, GDT_UInt32, 0, 0 ) ) == CE_Failure)  
   {
            printf( "\tError in writing data for image: %s.\n", imageName.c_str() );
            CPLFree (poImage);
            exit( 1 );
   }


   /* Once we're done, close properly the dataset */
   GDALClose( (GDALDatasetH) poDstDS );

   printf ( "\tCreated grid raster file: %s\n\n", imageName.c_str() );

   return ( imageName );
}


/*********************************************
*       22. resampleRaster                   *
*   re-sample a raster image                 *
*********************************************/
string resampleRaster (string imageFile, gridInfo grid)
{
   gridInfo       imageInfo;  
   string         rsImageName;
   GDALDataset    *poDataset;  
   GDALDataset    *poDstDS;       
   char           *pszWKT = NULL;

   GDALDriver     *poDriver;
   GDALRasterBand *poBand_in, *poBand;
   char           **papszOptions = NULL;
   vector<string> strVector;
   int            k,i,j;

   printf("\nRe-sampling a raster...");

   imageInfo = getImageInfo ( imageFile );

   //set re-sampled image file name
   strVector = string2stringVector ( imageFile, "/"); 
   rsImageName = string ( "rs_" )  + strVector[strVector.size() - 1];

   /* -------------------------------------------------------------------- */
   /*      Register format(s).                                             */
   /* -------------------------------------------------------------------- */
   GDALAllRegister();
   
   poDataset = (GDALDataset *) GDALOpen( imageFile.c_str() , GA_ReadOnly );
   if( poDataset == NULL )
   {
      printf( "\tOpen GDAL image file failed: %s.\n", imageFile.c_str() );
      exit( 1 );     
   }

   int numBands = poDataset->GetRasterCount();
 
   poDriver = poDataset->GetDriver(); 
   if( poDriver == NULL )
   {
      printf ( "\tGetting GDALDriver failed for image file: %s\n", imageFile.c_str() );
      exit( 1 );
   }

   //create re-sampled image

   poDstDS = poDriver->Create( rsImageName.c_str(), grid.cols, grid.rows, 1, GDT_UInt32, papszOptions );
   double adfGeoTransform[6] = { grid.xmin, grid.xCellSize, 0, grid.ymax, 0, -1*grid.yCellSize };

   poDstDS->SetGeoTransform( adfGeoTransform );
 
   pszWKT = strdup ( poDataset->GetProjectionRef() );
   poDstDS->SetProjection( pszWKT );

   for (k=1; k<=numBands; k++)
   {
      poBand_in = poDataset->GetRasterBand(k);   //in band
      poBand = poDstDS->GetRasterBand(k);        //out band

      GUInt32 *poImage_in = (GUInt32 *) CPLCalloc(sizeof(GUInt32),imageInfo.rows*imageInfo.cols);  //in array
      GUInt32 *poImage = (GUInt32 *) CPLCalloc(sizeof(GUInt32),grid.rows*grid.cols);               //out array
   
      //read band k image 
      if ( (poBand_in->RasterIO(GF_Read, 0,0,imageInfo.cols,imageInfo.rows,
           poImage_in,imageInfo.cols,imageInfo.rows,GDT_UInt32,0,0)) == CE_Failure)
        {
            printf( "\tError in reading band %d data from image: %s.\n", k,imageFile.c_str() );
            CPLFree (poImage_in);
            CPLFree (poImage);
            exit( 1 );
        }

      //grid ID from LL to UR corner 1 to rows*cols, different from image: UL to LR
      for (i=0; i<grid.rows; i++)
      {
         double y = grid.ymax - i * grid.yCellSize - grid.yCellSize / 2.0;   //get cell senter y
         int row = (int) (floor ((imageInfo.ymax - y) / imageInfo.yCellSize));   //start from 0 UL

         for ( int j=0; j<grid.cols; j++)
         {
            double x = grid.xmin + j * grid.xCellSize + grid.xCellSize / 2.0;
            int col = (int) (floor ((x - imageInfo.xmin) / imageInfo.xCellSize));  //start from 0 UL

            int pixelIndex = imageInfo.cols * row + col;
            int gridID = poImage_in[pixelIndex];

            pixelIndex = grid.cols * i + j;
            poImage[pixelIndex] = gridID;
         }  //j
      } //i

      if ( (poBand->RasterIO( GF_Write, 0, 0, grid.cols, grid.rows, 
            poImage, grid.cols, grid.rows, GDT_UInt32, 0, 0 )) == CE_Failure)
      {
          printf( "\tError in writing band %d data for image: %s.\n", k,rsImageName.c_str() );
          CPLFree (poImage_in);
          CPLFree (poImage);
          exit( 1 );
      }

      CPLFree (poImage_in);
      CPLFree (poImage);
   }  //k band

   /* Once we're done, close properly the dataset */
   GDALClose( (GDALDatasetH) poDataset );
   GDALClose( (GDALDatasetH) poDstDS );

   printf ( "\tCreated re-sampled image file: %s\n", rsImageName.c_str() );
   return rsImageName;
 
}


/*********************************************
*       23. readHDF4SatVarData               *
*   read a HDF4 file variable float data     *
*********************************************/
void readHDF4SatVarData (string imageFile, string varName, double *poImage)
{
   int32      sd_id, sds_id, istat, index;
   char       sds_name[64];
   int32      rank, dim_sizes[MAX_VAR_DIMS], num_type, attributes;
   char       *varName_nc;
   int        i,j,m,totalSize;

   float32     *poImageFloat32;
   int8        *poImageInt8;
   int16       *poImageInt16;
   uint8       *poImageUInt8;

   
   sd_id = SDstart(imageFile.c_str(), DFACC_READ);
   if (sd_id == FAIL)
   {
      printf ("\tOpen HDF4 file failed: %s.\n", imageFile.c_str() );
      exit ( 1 );
   }
    
   //get dimension size of HDF4 variable
   varName_nc = strdup ( varName.c_str() );
   anyHDF4Errors ( index = SDnametoindex ( sd_id, varName_nc ) );
   anyHDF4Errors ( sds_id = SDselect (sd_id, index) );
   anyHDF4Errors( SDgetinfo(sds_id, sds_name, &rank, dim_sizes, &num_type, &attributes) );

   int32  start[rank], edges[rank];
   totalSize = 1;
   for (i = 0; i < rank; i++) 
   {
      start[i] = 0;
      edges[i] = dim_sizes[i];
      totalSize *= dim_sizes[i];
      printf ( "\tDim%d = %d   DataType=%d\n", i+1, dim_sizes[i], num_type);
   }

   int rows;
   int cols;
   int numP=1; 
  
   if ( rank == 2 )
   {
     rows = dim_sizes[rank-2];
     cols = dim_sizes[rank-1];
   }
   else if ( rank == 3 )
   {
      //MCD43A1 products
      rows = dim_sizes[rank-3];
      cols = dim_sizes[rank-2];
      numP = dim_sizes[rank-1];
   }
   else
   {
      printf ("\tImage file has more than 3 dimension variable: dim=%d\n", rank);
      exit ( 1 );
   }
 

   if ( num_type == DFNT_FLOAT32 )
   {
      poImageFloat32 = (float32 *) CPLCalloc(sizeof( float32 ), totalSize);
      anyHDF4Errors ( SDreaddata (sds_id, start, NULL, edges , (VOIDP) poImageFloat32 ) );

      for (i=0; i<rows; i++)
      {
         for (j=0; j<cols; j++)
         {
            for ( m=0; m<numP; m++ )
            {
               index = i*cols*numP + j*numP + m;
               poImage[index] = poImageFloat32[index]; 
            }
         }
      }
      CPLFree ( poImageFloat32 );

   }
   else if ( num_type == DFNT_FLOAT64 )
   {
      anyHDF4Errors ( SDreaddata (sds_id, start, NULL, edges , (VOIDP) poImage ) );

   }
   else if ( num_type == DFNT_INT8 )
   {
     
      poImageInt8 = (int8 *) CPLCalloc(sizeof( int8 ), totalSize);
      anyHDF4Errors ( SDreaddata (sds_id, start, NULL, edges , (VOIDP) poImageInt8 ) );

      for (i=0; i<rows; i++)
      {
         for (j=0; j<cols; j++)
         {
            for ( m=0; m<numP; m++ )
            {
               index = i*cols*numP + j*numP + m;
               poImage[index] = poImageInt8[index];
            }
         }
      }
      CPLFree ( poImageInt8 ); 

   }
   else if ( num_type == DFNT_UINT8 )
   {

      poImageUInt8 = (uint8 *) CPLCalloc(sizeof( uint8 ), totalSize);
      anyHDF4Errors ( SDreaddata (sds_id, start, NULL, edges , (VOIDP) poImageUInt8 ) );

      for (i=0; i<rows; i++)
      {
         for (j=0; j<cols; j++)
         {
            for ( m=0; m<numP; m++ )
            {
               index = i*cols*numP + j*numP + m;
               poImage[index] = poImageUInt8[index];
            }
         }
      }
      CPLFree ( poImageUInt8 );

   }
   else if ( num_type == DFNT_INT16 )
   {
      poImageInt16 = (int16 *) CPLCalloc(sizeof( int16 ), totalSize);
      anyHDF4Errors ( SDreaddata (sds_id, start, NULL, edges , (VOIDP) poImageInt16 ) );

      for (i=0; i<rows; i++)
      {
         for (j=0; j<cols; j++)
         {
            for ( m=0; m<numP; m++ )
            {
               index = i*cols*numP + j*numP + m;
               poImage[index] = poImageInt16[index];
            }
         }
      }
      CPLFree ( poImageInt16 );   
   }
   else
   {
      printf ( "\tError: add data type - %d in readHDF4SatVarData.\n",num_type );
      exit ( 1 );
   }

   anyHDF4Errors ( SDendaccess(sds_id) );
   anyHDF4Errors (  SDend(sd_id) );

   printf ( "\tRead data: %s  |  %s\n", imageFile.c_str(), varName.c_str() );

}


/****************************************/
/*   24.    getHDF4VarInfo              */
/****************************************/
gridInfo getHDF4VarInfo ( string imageFile, string varName )
{

    gridInfo        imageInfo;

    printf("\n\tObtaining image variable information: %s from %s\n", varName.c_str(), imageFile.c_str()  );


    /*********************************
     *     read HDF4 variable info   *
     *********************************/
    int32      sd_id, sds_id, istat, n_datasets, n_file_attrs, index;
    char       sds_name[64]; 
    int32      rank, dim_sizes[MAX_VAR_DIMS], num_type, attributes;
    char       *varName_nc;


    sd_id = SDstart(imageFile.c_str(), DFACC_READ);
    if (sd_id == FAIL)
    {
       printf ("\tOpen HDF4 file failed: %s.\n", imageFile.c_str() );
       exit ( 1 );
    }
     
    /* Determine the contents of the file. */
    if ( SDfileinfo(sd_id, &n_datasets, &n_file_attrs) == FAIL) 
    {
        printf ("\tGetting HDF4 file info failed: %s\n", imageFile.c_str() );
        exit ( 1 );
    }

    printf ("\tNumber of the global attributes: %d\n", n_file_attrs );


    //define variables related to attributes
    char    buffer[164];
    string  valueStr;
         

    //get dimension size of HDF4 variable
    varName_nc = strdup ( varName.c_str() ); 
    anyHDF4Errors ( index = SDnametoindex ( sd_id, varName_nc ) );
    anyHDF4Errors ( sds_id = SDselect (sd_id, index) );
    anyHDF4Errors( SDgetinfo( sds_id, sds_name, &rank, dim_sizes, &num_type, &attributes ) );

    printf("\tname = %s  dims = %d  type = %d  number of attributes = %d\n", sds_name, rank, num_type, attributes );

    for (int j=0; j<rank; j++)
    {
       printf("\t\t\tDimension %d size: %d\n", j, dim_sizes[j] );
       imageInfo.dims.push_back ( dim_sizes[j] );
    }

    if ( rank >= 2 )
    {
       imageInfo.rows = dim_sizes[0];
       imageInfo.cols = dim_sizes[1];
    }
    else
    {
       printf ( "\t\tError: Variable dimension should be >= 2.\n" );
       exit ( 1 );
    }


    //get Title  1
    imageInfo.attsStr.push_back ( varName );
    //printf("\ttitle=%s\n", imageInfo.attsStr.at(0).c_str() );


    //Units 2
    valueStr = getHDF4VarAttrInfo (sds_id, "units");
    if ( valueStr.size() == 0)
    {
       valueStr = string ("None");
    }

    if ( imageFile.find( "MCD15A2H" ) != string::npos )
    {
       valueStr = string ("Fraction");
    }

    imageInfo.attsStr.push_back ( valueStr );
    //printf("\tunits=%s\n", valueStr.c_str() );
   

    //scale factor  3
    valueStr = getHDF4VarAttrInfo (sds_id, "scale_factor");
    if ( valueStr.size() == 0)
    {
       valueStr = string ("1.0");
    }

    imageInfo.attsStr.push_back ( valueStr );
    //printf("\tscale_factor=%s\n", valueStr.c_str() );


    //add_offset  4
    valueStr = getHDF4VarAttrInfo (sds_id, "add_offset");
    if ( valueStr.size() == 0)
    {
       valueStr = string ("0.0");
    }
    imageInfo.attsStr.push_back ( valueStr );
    //printf("\tadd_offset=%s\n", valueStr.c_str() );


    //_FillValue  5
    valueStr = getHDF4VarAttrInfo (sds_id, "_FillValue");
    imageInfo.attsStr.push_back ( valueStr );
    //printf("\t_FillValue=%s\n", valueStr.c_str() );


    //if ( imageFile.find( "MCD43A3" ) != string::npos ) exit ( 1 );


    //data type  6
    sprintf (buffer, "%d\0",num_type);
    imageInfo.attsStr.push_back ( string ( buffer ) );
    //printf("\tnum_type=%s\n", buffer );


    //add_offset 7 
    valueStr = getHDF4VarAttrInfo (sds_id, "long_name");
    if ( valueStr.size() != 0)
    {
       imageInfo.attsStr.push_back ( valueStr );
       //printf("\tlong_name=%s\n", valueStr.c_str() );
    }

    anyHDF4Errors ( SDendaccess(sds_id) );

    if ( imageFile.find( "MCD12Q1" ) != string::npos || imageFile.find( "MOD12Q1" ) != string::npos || 
         imageFile.find( "MOD15A2GFS" ) != string::npos || imageFile.find( "MOD15A2" ) != string::npos ||
         imageFile.find( "MCD15A2H" ) != string::npos ||
         imageFile.find( "MCD43A3" ) != string::npos || imageFile.find( "MCD43A1" ) != string::npos || 
         imageFile.find( "MCD43A2" ) != string::npos ) 
    {

       bool foundCellSize = false;   //set CHARACTERISTICBINSIZE
       for ( int32 i=0; i<n_file_attrs; i++ )
       {
         
          int32    attr_index, data_type, count;
          char     attr_name[64];
          char     *attBuff;
          string   valueStr, projStr;
          int      pos;
          std::vector<string>   vecString, metaStr;
      

          if ( imageFile.find( "MCD12Q1" ) != string::npos )
          {
             imageInfo.name = string ( "MCD12Q1" );
          }
          else if ( imageFile.find( "MOD12Q1" ) != string::npos )
          {
             imageInfo.name = string ( "MOD12Q1" );
          }
          else if ( imageFile.find( "MOD15A2GFS" ) != string::npos )
          {
             imageInfo.name = string ( "MOD15A2GFS" );
          }
          else if ( imageFile.find( "MOD15A2" ) != string::npos )
          {
             imageInfo.name = string ( "MOD15A2" );
          }
          else if ( imageFile.find( "MCD15A2H" ) != string::npos )
          {
             imageInfo.name = string ( "MCD15A2H" );
          }
          else if ( imageFile.find( "MCD43A3" ) != string::npos )
          {
             imageInfo.name = string ( "MCD43A3" );
          }
          else if ( imageFile.find( "MCD43A1" ) != string::npos )
          {
             imageInfo.name = string ( "MCD43A1" );
          }
          else if ( imageFile.find( "MCD43A2" ) != string::npos )
          {
             imageInfo.name = string ( "MCD43A2" );
          }
         

          anyHDF4Errors ( SDattrinfo(sd_id, i, attr_name, &data_type, &count) );

          //printf ( "\tAttribute Index: %d   Attribute name: %s   Attribute Type: %d   Attribute count: %d\n", i,attr_name,data_type, count);


          if ( ( attBuff = (char *) calloc (count, sizeof(char)) ) == NULL)
          {
            printf( "Calloc attBuff failed.\n");
            exit ( 1 );
          }

          anyHDF4Errors ( SDreadattr(sd_id, i, attBuff) );
          string attStr = string ( attBuff );
 
          //printf ("\t\tAttribute: %s:\n",attBuff);   
         
          metaStr = string2stringVector (attStr, "\n");


          for (int lines=0; lines<metaStr.size(); lines++)
          {
             //printf ("\tlines %d: %s\n", lines, metaStr[lines].c_str() );

             if ( ( pos = metaStr[lines].find ( "Projection=" ) ) != string::npos )
             {
                valueStr = metaStr[lines].substr( pos+11, 11 );
                printf ("\t\t\tAttribute:  Projection=%s\n", valueStr.c_str() );

                if ( valueStr.find ( "GCTP_SNSOID" ) != string::npos )
                {
                   projStr = string ( "+proj=sinu +lon_0=0 +x_0=0 +y_0=0 +a=" );
                }
                else
                {
                   printf ( "\nError: MODIS land cover file  projection is not: GCTP_SNSOID.\n" );
                   exit ( 1 );
                }
             }
             else if ( metaStr[lines].find ( "ProjParams=" )  != string::npos || metaStr[lines].find ( "UpperLeftPointMtrs=" )  != string::npos
                || metaStr[lines].find ( "LowerRightMtrs=" )  != string::npos )

             {
             
                int pos1, pos2;
 
                if ( ( pos1 = metaStr[lines].find ( "(" ) ) != string::npos && ( pos2 = metaStr[lines].find ( ")" ) ) != string::npos )
                {
                   pos1 += 1;
                   int totalChars = pos2 - pos1; 

                   valueStr = metaStr[lines].substr( pos1, totalChars );
                   vecString = string2stringVector (valueStr, ",");

                   //printf ("\t\t\tAttribute:  %s\n", valueStr.c_str() );
                }
                else
                {
                   printf("\tError: MODIS land data attribute do not have parentheses - %s\n", attStr.c_str() );
                   exit ( 1 );
                }

                if (  metaStr[lines].find ( "ProjParams=" )  != string::npos )
                {
                   printf ("\t\t\tAttribute:  ProjParams=%s\n", valueStr.c_str() );
                   projStr.append ( vecString[0] );
                   projStr.append ( " +b=" );
                   projStr.append ( vecString[0] );
                   projStr.append ( " +units=m +no_defs" );
 
                   for( int j=1;j<vecString.size();j++ )
                   {
                      if ( vecString[j].compare ( "0" ) != 0 )
                      {
                         printf ( "\tError: MODIS land cover file projection parameters are not: 6371007.181000,0,0,0,0,0,0,0,0,0,0,0,0\n" );
                         printf ("\n%s\n", vecString[j].c_str() );
                         exit ( 1 );
                      }
                   }
 
                   imageInfo.strProj4 =strdup ( projStr.c_str() );
                }

                if (  metaStr[lines].find ( "UpperLeftPointMtrs=" )  != string::npos )
                {
                   printf ("\t\t\tAttribute:  UpperLeftPointMtrs=%s\n", valueStr.c_str() );

                   imageInfo.xmin = atof( vecString[0].c_str() ); 
                   imageInfo.ymax = atof( vecString[1].c_str() );
                }

                if ( metaStr[lines].find ( "LowerRightMtrs=" )  != string::npos )
                {
                   printf ("\t\t\tAttribute:  LowerRightMtrs=%s\n", valueStr.c_str() );

                   imageInfo.xmax = atof( vecString[0].c_str() );
                   imageInfo.ymin = atof( vecString[1].c_str() );
                }
             }
          }  //end of lines

          if ( ( pos = attStr.find ( "CHARACTERISTICBINSIZE" ) ) != string::npos )
          {

            if ( attStr.find ( "CHARACTERISTICBINSIZE500M" )  != string::npos )
             {
                valueStr = attStr.substr( pos+82, 11 );
             }
             else
             {
                valueStr = attStr.substr( pos+78, 11 );
             }

             printf ("\t\t\tAttribute:  CHARACTERISTICBINSIZE=%s\n", valueStr.c_str() );
             double cellSize = atof (valueStr.c_str() );
             imageInfo.xCellSize = cellSize;
             imageInfo.yCellSize = cellSize;
               
             foundCellSize = true;
          }

          free (attBuff);

       } //end of i: process each global attribute


       //checking
       if ( !foundCellSize ) 
       {
          if ( imageFile.find( "MOD15A2GFS" ) != string::npos )
          {
             //somehow MOD15A2GFS does not have CHARACTERISTICBINSIZE

             imageInfo.xCellSize = 926.625433055556;
             imageInfo.yCellSize = 926.625433055556;

             printf ("\t\t\tAttribute:  %lf\n", imageInfo.xCellSize );
          }
          else
          {
             printf ( "\tError: Can not get cell size from the image - no CHARACTERISTICBINSIZE\n" );
             exit ( 1 );
          }
       }

       
       if ( fabs( ( (imageInfo.xmax - imageInfo.xmin) / imageInfo.xCellSize ) - imageInfo.cols ) > 1.0 )
       {
          printf ( "\tError: Columns computed from xmin-%lf, xmax-%lf, and cell size-%lf is not: %d\n",imageInfo.xmin, imageInfo.xmax, imageInfo.xCellSize, imageInfo.cols );
          
          exit ( 1 );
       }
       if (  fabs( ( (imageInfo.ymax - imageInfo.ymin) / imageInfo.yCellSize ) - imageInfo.rows ) > 1.0 )
       {
          printf ( "\tError: Rows computed from ymin-%lf, ymax-%lf, and cell size-%lf is not: %d\n", imageInfo.ymin, imageInfo.ymax, imageInfo.yCellSize, imageInfo.rows );
          exit ( 1 );
       }

    }  //for MODIS land data tile files
    else
    {
      imageInfo.strProj4 =strdup ( "+proj=latlong +datum=WGS84");
    }

    printf ( "\t\t\tProj4 projection string from image is: %s\n", imageInfo.strProj4);
       
    anyHDF4Errors ( SDend(sd_id) );

   
    return imageInfo;
}


/****************************************/
/*   25.    getHDF4VarAttrInfo          */
/****************************************/
string  getHDF4VarAttrInfo (int32 sds_id, const char *attName )
{
    string  attStr;
    char    *attName_nc;
    int32   attr_index, data_type, count;
    char    attr_name[64], buffer[164];
    char    *attBuff;
   
    //check existence of the attribute name
    attName_nc = strdup ( attName );
 
    if (  (attr_index = SDfindattr(sds_id, attName_nc) ) == FAIL )
      return attStr;

    anyHDF4Errors ( SDattrinfo(sds_id, attr_index, attr_name, &data_type, &count) );
   
    //printf ( "\tAttribute name: %s   Attribute Type: %d    Attribute count: %d\n", attr_name,data_type, count); 
    
    if ( data_type == DFNT_CHAR )      //data type = 4
    {
       if ( ( attBuff = (char *) calloc (count, sizeof(char)) ) == NULL)
       {
          printf( "Calloc attBuff failed.\n");
          exit ( 1 );
       }
 
       anyHDF4Errors ( SDreadattr(sds_id, attr_index, attBuff) );
       attStr = string ( attBuff );
       attStr.erase (count);
    }
    else if ( data_type == DFNT_FLOAT64 )   //data type = 6
    {
       float64 *bufferFloat64;
       if ( ( bufferFloat64 = (float64*) calloc (count, sizeof(float64)) ) == NULL)
       {
           printf( "Calloc bufferFloat64 failed.\n");
           exit ( 1 );
       }
       
       anyHDF4Errors ( SDreadattr(sds_id, attr_index, bufferFloat64) );
       sprintf (buffer, "%f\0",bufferFloat64[0]);
       attStr = string ( buffer );
    }
    else if ( data_type == DFNT_FLOAT32 )    //data type = 5
    {
       float32 *bufferFloat32;
       if ( ( bufferFloat32 = (float32*) calloc (count, sizeof(float32)) ) == NULL)
       {
           printf( "Calloc bufferFloat32 failed.\n");
           exit ( 1 );
       }

       anyHDF4Errors ( SDreadattr(sds_id, attr_index, bufferFloat32) );
       sprintf (buffer, "%f\0",bufferFloat32[0]);
       attStr = string ( buffer );
    }
    else if ( data_type == DFNT_INT8 )   //data type 20
    {
       int8 *bufferInt8;
       if ( ( bufferInt8 = (int8*) calloc (count, sizeof(int8)) ) == NULL)
       {
           printf( "Calloc bufferInt8 failed.\n");
           exit ( 1 );
       }

       anyHDF4Errors ( SDreadattr(sds_id, attr_index, bufferInt8) );

       sprintf (buffer, "%d\0",bufferInt8[0]);
       attStr = string ( buffer );
    }
    else if ( data_type == DFNT_UINT8 )   //data type 21
    {
       uint8 *bufferUInt8;
       if ( ( bufferUInt8 = (uint8*) calloc (count, sizeof(uint8)) ) == NULL)
       {
           printf( "Calloc bufferUInt8 failed.\n");
           exit ( 1 );
       }

       anyHDF4Errors ( SDreadattr(sds_id, attr_index, bufferUInt8) );

       sprintf (buffer, "%d\0",bufferUInt8[0]);
       attStr = string ( buffer );
    }
    else if ( data_type == DFNT_INT16 )   //data type 22
    {
       int16 *bufferInt16;
       if ( ( bufferInt16 = (int16*) calloc (count, sizeof(int16)) ) == NULL)
       {
           printf( "Calloc bufferInt16 failed.\n");
           exit ( 1 );
       }

       anyHDF4Errors ( SDreadattr(sds_id, attr_index, bufferInt16) );
       sprintf (buffer, "%d\0",bufferInt16[0]);
       attStr = string ( buffer );
    }
    else if ( data_type == DFNT_INT32 )    //data type = 24
    {
       int32 *bufferInt32;
       if ( ( bufferInt32 = (int32*) calloc (count, sizeof(int32)) ) == NULL)
       {
           printf( "Calloc bufferInt32 failed.\n");
           exit ( 1 );
       }

       anyHDF4Errors ( SDreadattr(sds_id, attr_index, bufferInt32) );
       sprintf (buffer, "%d\0",bufferInt32[0]);
       attStr = string ( buffer );
    }
    else
    {
       printf ( "\tError: need new data type in getHDF4VarAttrInfo: %d\n", data_type);
       exit ( 1 );
    }

    return attStr;
}


/****************************************/
/*   29.    getHDF5VarInfo              */
/****************************************/
gridInfo getHDF5VarInfo ( string imageFile, string varName )
{

    gridInfo   imageInfo;
    string     dataSet;
    string     groupPath;


    printf("\tObtaining image variable information...\n" );

    
    /*********************************
    *     read HDF5 variable info   *
    *********************************/
    hid_t       file_id, dataset_id, fspace_id, dtype_id, ndtype_id;  /* identifiers */
    H5T_class_t dclass_id;
    int         numDims, numMems;
    hsize_t     dims[6];     //set max dimensions to 6             
    string      valueStr;

    int         j;

    
    //open the file
    anyHDF5Errors ( file_id = H5Fopen( imageFile.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT ) );

    if ( imageFile.find ( ".he5" ) != string::npos )
    {
       groupPath = getHDF5VarGroups ( file_id, varName ); 
    }
    else
    {
       groupPath =string ("/");   //no path
    }
  
    dataSet = groupPath + varName;


    printf( "\tImage file name: %s\n", imageFile.c_str() );
    printf( "\tImage variable name: %s\n", dataSet.c_str() );

    //open data set
    anyHDF5Errors ( dataset_id = H5Dopen (file_id, dataSet.c_str(), H5P_DEFAULT ) );

    //get dimension size of HDF5 variable
    anyHDF5Errors ( fspace_id = H5Dget_space(dataset_id) );    /* Get filespace handle first. */
    anyHDF5Errors ( numDims = H5Sget_simple_extent_dims(fspace_id, dims, NULL) );
    printf ( "\t\tNumber of Dimensions = %d \n", numDims );

    for (j=0; j<numDims; j++)
    {
       printf("\t\t\tDimension %d size: %d\n", j, dims[j] );
       imageInfo.dims.push_back ( dims[j] );
    }
     
    if ( numDims >= 2 )
    {
       imageInfo.rows = dims[0];
       imageInfo.cols = dims[1];
    }
    else
    {
       printf ( "\t\tError: Variable dimension should be >= 2.  numDims = %d\n", numDims );
       exit ( 1 );
    }

    //this is for OMI NO2 L3 data
    if ( groupPath.compare ( "/" ) == 0 )
    {
       printf ( "\tProcess a hdf5 file only contains one data array - OMI NO3 L3 data set\n");
       imageInfo.cols = dims[1];
       imageInfo.rows = dims[0];
       imageInfo.xmin = -180.0;
       imageInfo.xmax = 180.0;
       imageInfo.ymin = -90.0;
       imageInfo.ymax = 90.0;
       imageInfo.xCellSize = 0.05;
       imageInfo.yCellSize = 0.05;
    }
    

    //Processing data set attributes

    //get Title  1
    imageInfo.attsStr.push_back ( varName );

    //Units 2
    valueStr = getHDF5VarAttrInfo (dataset_id, "Units");
    imageInfo.attsStr.push_back ( valueStr );

    //scale factor  3
    valueStr = getHDF5VarAttrInfo (dataset_id, "ScaleFactor");
    imageInfo.attsStr.push_back ( valueStr );

    //add_offset  4
    valueStr = getHDF5VarAttrInfo (dataset_id, "Offset");
    imageInfo.attsStr.push_back ( valueStr );

    //_FillValue  5
    valueStr = getHDF5VarAttrInfo (dataset_id, "_FillValue");
    //valueStr = getHDF5VarAttrInfo (dataset_id, "MissingValue");
    imageInfo.attsStr.push_back ( valueStr );

    //data type  6

    //get data type
    anyHDF5Errors ( dtype_id = H5Dget_type ( dataset_id ) );
    anyHDF5Errors ( ndtype_id = H5Tget_native_type ( dtype_id, H5T_DIR_ASCEND) );
    if ( H5Tequal (H5T_NATIVE_FLOAT, ndtype_id) > 0 )
    {
        //printf ( "\t\t\tNative Data type is: H5T_NATIVE_FLOAT\n");
        imageInfo.attsStr.push_back ( string ( "H5T_NATIVE_FLOAT" ) );
    }
    else if ( H5Tequal (H5T_NATIVE_DOUBLE, ndtype_id) > 0 )
    {
        //printf ( "\t\t\tNative data type is: H5T_NATIVE_DOUBLE\n");
        imageInfo.attsStr.push_back ( string ( "H5T_NATIVE_DOUBLE" ) );
    }
    else 
    {
        printf ( "\t\t\tNot suppported native data type and add it to getHDF5VarInfo in geo_function.cpp\n");
        exit ( 1 );
    }


    //add LONGNAME 7
    valueStr = getHDF5VarAttrInfo (dataset_id, "Title");
    imageInfo.attsStr.push_back ( valueStr );


    anyHDF5Errors ( H5Tclose ( ndtype_id ) );
    anyHDF5Errors ( H5Tclose ( dtype_id ) );
    anyHDF5Errors ( H5Dclose ( dataset_id ) );
    anyHDF5Errors ( H5Sclose ( fspace_id ) );
    anyHDF5Errors ( H5Fclose ( file_id ) );


    imageInfo.strProj4 =strdup ( "+proj=latlong +datum=WGS84");
    printf ( "\t\tImage Proj4 projection is set to: %s\n", imageInfo.strProj4);

    return imageInfo;

}


/****************************************/
/*   30.    getHDF5VarAttrInfo          */
/****************************************/
string  getHDF5VarAttrInfo ( hid_t dataset_id, const char *attName )
{
    string        attStr;
    hid_t         att_id, atype_id, natype_id, matype_id, aspace_id;  //identifiers 
    H5T_class_t   atype_class;
    int           numDims, rank;
    hsize_t       strSize, dims[6];                   //set max dimensions to 6
    int           i;
    char          buff[125];
    float         valueF;
    double        valueD;

    printf("\t\tAttribute name: %s\n", attName );

    //get attribute ID
    if ( ( att_id = H5Aopen_name ( dataset_id, attName ) ) < 0 )
    {
       attStr = string ("NULL");
       return attStr;
    }

    //Get attribute datatype, dataspace, rank, and dimensions.
    anyHDF5Errors ( atype_id  = H5Aget_type(att_id) );
    anyHDF5Errors ( atype_class = H5Tget_class ( atype_id ) ); 

    anyHDF5Errors ( natype_id = H5Tget_native_type ( atype_id, H5T_DIR_ASCEND) );

    anyHDF5Errors ( aspace_id = H5Aget_space(att_id) );
    anyHDF5Errors ( numDims = H5Sget_simple_extent_dims(aspace_id, dims, NULL) );

    //printf ( "\t\t\tNumber of Dimensions = %d\n", numDims );

    if ( atype_class == H5T_STRING )
    {
       //printf ( "\t\t\tNative data type is: H5T_STRING\n");
       anyHDF5Errors ( strSize = H5Aget_storage_size( att_id ) ); 
       //printf ( "\t\t\tString size: %d\n", strSize); 

       anyHDF5Errors ( H5Aread ( att_id, atype_id, buff) );
       attStr = string ( buff );
       attStr.erase( strSize, attStr.size() - strSize );   //get rid of extra spaces
    }
    else if ( atype_class == H5T_INTEGER || atype_class == H5T_FLOAT )
    {
       //printf ( "\t\t\tData type is: H5T_INTEGER or H5T_FLOAT\n");
       anyHDF5Errors ( matype_id = H5Tcopy( H5T_NATIVE_DOUBLE ) );
       anyHDF5Errors ( H5Aread ( att_id, matype_id, &valueD) );
       sprintf (buff, "%lf\0",valueD);
       attStr = string ( buff );
    }
    /*
    else if ( H5Tequal (H5T_NATIVE_FLOAT, natype_id) > 0 )
    {
        //printf ( "\t\t\tData type is: H5T_NATIVE_FLOAT\n");
        anyHDF5Errors ( H5Aread ( att_id, natype_id, &valueF) );
        sprintf (buff, "%f\0",valueF);
        attStr = string ( buff );
    }
    else if ( H5Tequal (H5T_NATIVE_DOUBLE, natype_id) > 0 )
    {
        //printf ( "\t\t\tData type is: H5T_NATIVE_DOUBLE\n");
        anyHDF5Errors ( H5Aread ( att_id, natype_id, &valueD) );
        sprintf (buff, "%lf\0",valueD);
        attStr = string ( buff );
    }*/
    else
    {
        printf ( "\t\t\tError: Not suppported native data type and add it to getHDF5VarAttrInfo in geo_function.cpp\n");
        exit ( 1 );
    }

    printf ("\t\t\tObtained value: %s\n", attStr.c_str() );

    anyHDF5Errors ( H5Tclose ( natype_id ) );
    anyHDF5Errors ( H5Tclose ( atype_id ) );
    anyHDF5Errors ( H5Sclose ( aspace_id ) );
    anyHDF5Errors ( H5Aclose ( att_id ) );


    return attStr;
}


/*********************************************
*       31. readHDF5SatVarData               *
*           read a HDF5 file variable data   *
*********************************************/
void readHDF5SatVarData (string imageFile, string varName, double *poImage)
{

    string     groupPath, dataSet;
    int        totalSize = 1;
    int        i,j;

    /*********************************
    *     read HDF5 variable info   *
    *********************************/
    hid_t       file_id, dataset_id;
    hid_t       fspace_id,  mspace_id;
    hid_t       dtype_id, ndtype_id;  /* identifiers */
    H5T_class_t dclass_id;
    int         numDims, numMems;
    hsize_t     dims[6];     //set max dimensions to 6
    string      valueStr;


    //open the file
    anyHDF5Errors ( file_id = H5Fopen( imageFile.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT ) );

     /* obtain group path under "/HDFEOS/SWATHS" */
    if ( imageFile.find ( ".he5" ) != string::npos )
    {
       groupPath = getHDF5VarGroups ( file_id, varName );
    }
    else
    {
       groupPath =string ("/");   //no path for hdf5 file like OMI NO2 L3 0.05 grids
    }

    dataSet = groupPath + varName;

    printf( "\tRead data set: %s\n", dataSet.c_str() );

    //open data set
    anyHDF5Errors ( dataset_id = H5Dopen (file_id, dataSet.c_str(), H5P_DEFAULT ) );

    //get dimension size of HDF5 variable
    anyHDF5Errors ( fspace_id = H5Dget_space(dataset_id) );    /* Get filespace handle first. */
    anyHDF5Errors ( numDims = H5Sget_simple_extent_dims(fspace_id, dims, NULL) );
    printf ( "\tNumber of Dimensions =%d \n", numDims );
    if ( numDims < 2 )
    {
       printf ( "\tError: Variable dimension should be >= 2.  numDims = %d\n", numDims );
       exit ( 1 );
    }

    totalSize = 1; 
    for (i=0; i<numDims; i++)
    {
       totalSize *= dims[i];
       printf("\t\tDimension %d size: %d\n", i, dims[i] );
    }
    printf("\t\tTotal array size to read: %d\n", totalSize );
    int   rows = dims[0];
    int   cols = dims[1];

    //Define the memory space to read dataset.
    mspace_id = H5Screate_simple(numDims,dims,NULL);
 
    /*
     * Read dataset back and display.
     */
    anyHDF5Errors ( H5Dread(dataset_id, H5T_NATIVE_DOUBLE, mspace_id, fspace_id, H5P_DEFAULT, poImage) );

    /*checking
    double  tempD[rows][cols];
    anyHDF5Errors ( H5Dread(dataset_id, H5T_NATIVE_DOUBLE, mspace_id, fspace_id, H5P_DEFAULT, tempD) );
    printf("Dataset: \n");
    for (j = 0; j < dims[0]; j++) 
    {
       for (i = 0; i < dims[1]; i++) 
       {   
          int  index = j*cols + i;
          printf("%.4lf   %.4lf", poImage[index], tempD[j][i]);
          if ( poImage[index] != tempD[j][i] )
          {
              printf("\t\tDo not match\n");
              exit ( 1 );
          }
          printf("\n");
       }
    }     
    */

   anyHDF5Errors ( H5Sclose ( mspace_id ) );
   anyHDF5Errors ( H5Sclose ( fspace_id ) );
   anyHDF5Errors ( H5Dclose ( dataset_id ) );
   anyHDF5Errors ( H5Fclose ( file_id ) );

   //printf ( "\tRead data: %s  |  %s\n", imageFile.c_str(), varName.c_str() );

}

/*************************************************
*       32. getHDF5VarGroups                     *
*           get group path for a varible in HDF5 *
**************************************************/
string getHDF5VarGroups (hid_t file_id, string varName)
{
    string      groupPath;
    hid_t       group_id;     
    hsize_t     num_obj,i;
    char        objName[50];
    size_t      size;
 

    string      startPath = string ( "/HDFEOS/SWATHS" );

    printf("\tGet variable HDF5 group path: %s\n", varName.c_str() );

    //get start level group

    /* open the group */
    //printf("\t\t\tOpen start group: %s\n", startPath.c_str() );          
    anyHDF5Errors ( group_id = H5Gopen (file_id, startPath.c_str(), H5P_DEFAULT ) );

    //get number of objects
    anyHDF5Errors ( H5Gget_num_objs( group_id, &num_obj ) );

    //printf("\t\t\tNumber of objects under %s: %d\n", startPath.c_str(), num_obj );
  
    if ( num_obj != 1 )
    {
      printf("\t\t\tMore than one groups under: %s\n", startPath.c_str() );
      exit ( 1 );
    }

    anyHDF5Errors ( H5Gget_objname_by_idx (group_id, 0, objName, 50 ) ) ;
    groupPath = startPath + string ("/") + string ( objName );

    anyHDF5Errors ( H5Gclose ( group_id ) );


    //get next level groups
    //printf("\t\t\tOpen group: %s\n", groupPath.c_str() ); 

    /* open the group */
    anyHDF5Errors ( group_id = H5Gopen (file_id, groupPath.c_str(), H5P_DEFAULT ) );

    //get number of objects
    anyHDF5Errors ( H5Gget_num_objs( group_id, &num_obj ) );

    //printf("\t\t\tNumber of objects under %s: %d\n", groupPath.c_str(), num_obj );
    string data_grp;
    string geo_grp;
    string obj_str;

    for (i=0; i<num_obj; i++)
    { 
       anyHDF5Errors ( H5Gget_objname_by_idx (group_id, i, objName, 50 ) ) ;
       obj_str = string ( objName );
       if ( obj_str.compare ( "Data Fields" ) == 0 )
       {
          data_grp = string ( "Data Fields" );
       }
       if ( obj_str.compare ( "Geolocation Fields" ) == 0 )
       {
          geo_grp = string ( "Geolocation Fields" );
       }
    }

    if ( geo_grp.size() != 0 && ( varName.compare ( "Latitude" ) == 0 || varName.compare ( "Longitude" ) == 0 ) )
    {
       //Geolocation Fields
       groupPath =  groupPath + string ("/") + geo_grp + string ("/");
    }
    else if ( data_grp.size() != 0 ) 
    {
       //Data Field
       groupPath =  groupPath + string ("/") + data_grp + string ("/");
    }
    else
    {
       printf("\t\t\tNo Data Fields or Geolocation Fields groups under: %s\n", groupPath.c_str() );
       exit ( 1 );
    }

    //printf("\t\t\tFound group path: %s\n", groupPath.c_str() );

    anyHDF5Errors ( H5Gclose ( group_id ) );
    
    return groupPath;

}


/*********************************************
*       33. anyHDF5Errors                    *
*           Handle HDF5 library error status *
*********************************************/
void anyHDF5Errors( int  e )
{
   if (e < 0 )
   {
      printf( "\nHDF5 error." );
      exit ( 1 );
   }
}


/************************************************************************/
/*      34. Get Start array to write one time step NetCDF array         */
/************************************************************************/
void getStartArraytoWriteNetCDF ( size_t *start, gridInfo imageInfo, int timeSteps )
{
   int   k;


   start[0] = timeSteps;

   for ( k=0; k<imageInfo.dims.size(); k++)
   {
        start[k+1] = 0;
   }

}



/************************************************************************/
/*      35. Get count array to write one time step NetCDF array         */
/************************************************************************/
void getCountArraytoWriteNetCDF ( size_t *count, gridInfo imageInfo, size_t south_north_len, size_t west_east_len )    
{
   int   k;

   count[0] = 1;
   count[1] = south_north_len;
   count[2] = west_east_len;

   for ( k=2; k<imageInfo.dims.size(); k++)
   {
        count[k+1] = imageInfo.dims[k];
   }

}

/************************************************************************/
/*      36. Write one time step WRF NetCDF time variables               */
/************************************************************************/
void writeWRF1StepTimeVariables (int timeSteps, float minutesPassed, string time_str, int ncid, 
                                 int time_id, int timeStr_id, size_t dateStr_len)
{
    float   times[1];       //one time step -- seconds orminutes from the start time
    char    *timesStr;      //one time stemp time string

    long    tx_start[2];
    long    tx_count[2];

    times[0] = minutesPassed;   //it is seconds

    if ( (timesStr = (char *) calloc (dateStr_len+1, sizeof(char)) ) == NULL)
    {
      printf( "Calloc timesStr failed.\n");
      exit ( 1 );
    }
    strcpy (timesStr, time_str.c_str() );   //asign time string

    printf ( "\tWrote Times float...\n" );
    //write time variables

    tx_start[0]=timeSteps;
    tx_count[0]=1;
    anyErrors( ncvarput(ncid, time_id, tx_start, tx_count, times) );

    tx_start[0]=timeSteps;
    tx_start[1]=0;
    tx_count[0]=1;
    tx_count[1]=dateStr_len;
    anyErrors( ncvarput(ncid, timeStr_id, tx_start, tx_count, timesStr) );
    printf( "\tWrote Times string...\n\n" );

    free (timesStr);

}

/************************************************************************/
/*      37. Set NetCDF Sat Variable Dimention Information               */
/************************************************************************/
void  setWRFNetCDFSatVarDimIndex ( int ncid, size_t *dims_len, int *dims_id, int *numADims, int *dimIndex, gridInfo imageInfo )
{
   int    j,k;
   int    dimID; 
   char   tmp_char [3];

   //image data: rowsXcolsXd3Xd4... -- first two geolocation dimensions
   //NetCDF var: timeXrowsXcolsXd3Xd4... -- first 3 time + geolocation dimensions

      
   //printf( "\t\tDimension: image dims=%d  numADims=%d\n", imageInfo.dims.size(), *numADims);
   for ( j=2; j<imageInfo.dims.size(); j++)
   {
      //check defined dimention
      bool  find_dim = false;

      //check match any added dimensions
      for ( k=0; k<*numADims; k++ )
      {
         if ( dims_len[k] == imageInfo.dims[j] )
         {
            dimIndex[1+j] = dims_id[k];  //assign defined dimension 
            find_dim = true;              //found it
            //printf( "\t\tDimension: j=%d  k=%d  %d\n", dims_id[k],j,k  );
            break;
         }
      } //k


      //adding new dimension
      if ( ! find_dim )
      {
         dims_len[*numADims] = imageInfo.dims[j];
         printf( "\t\tAdding dimension: %d\n", 4+*numADims );

         sprintf (tmp_char, "%d\0", 4+*numADims );
         string tmp_str = string ( "dim" ) + string ( tmp_char );

         printf( "\t\tDimension: %s\n", tmp_str.c_str() );

         anyErrors( nc_def_dim(ncid, tmp_str.c_str(), imageInfo.dims[j], &dimID) );
         dims_id[*numADims] = dimID;

         dimIndex[1+j] = dimID;   //1 is for time dim
         (*numADims) ++;
      }
   } //j

   //printf( "\t\tnumADims=%d\n",*numADims);

}


/*********************************************************/
/*      38. Define WRF NetCDF attribute variables        */
/*********************************************************/
int defineWRFNetCDFSatVar ( int ncid, string satVarName, int numDims, int *dimIndex, gridInfo imageInfo )
{
   int  satV_id;
   int  j;

   int     fieldtype[1];
   float   attValue[1];

   fieldtype[0] = 104;

   printf ("\tDefine varName=%s numDims=%d\n",satVarName.c_str(), numDims);

   anyErrors( nc_def_var(ncid, satVarName.c_str(), NC_FLOAT, numDims, dimIndex, &satV_id) );

   anyErrors( nc_put_att_int(ncid, satV_id, "FieldType", NC_INT, 1, fieldtype) );

   anyErrors( nc_put_att_text(ncid, satV_id, "MemoryOrder", 3, "XY ") );

   anyErrors( nc_put_att_text(ncid, satV_id, "description", imageInfo.attsStr[6].size(), imageInfo.attsStr[6].c_str() ));

   anyErrors( nc_put_att_text(ncid, satV_id, "units", imageInfo.attsStr[1].size(), imageInfo.attsStr[1].c_str() ) );


   if ( satVarName.find ("_LAI") != string::npos || satVarName.find ("_FPAR") != string::npos || 
        satVarName.find ("Albedo") != string::npos ||
        satVarName.find ("ALBEDO") != string::npos || satVarName.find ("Lai_1km") != string::npos || satVarName.find ("MODIS_ALB_") != string::npos  || satVarName.find ("Lai_500m") != string::npos || satVarName.find ("Fpar_500m") != string::npos )
   {
      attValue[0] = 1.0;
      anyErrors( nc_put_att_float(ncid, satV_id, "scale_factor", NC_FLOAT, 1, attValue) );

      attValue[0] = 0.0;
      anyErrors( nc_put_att_float(ncid, satV_id, "add_offset", NC_FLOAT, 1, attValue) );
   }
   else
   {
      attValue[0] = atof ( imageInfo.attsStr[2].c_str() );
      anyErrors( nc_put_att_float(ncid, satV_id, "scale_factor", NC_FLOAT, 1, attValue) );

      attValue[0] = atof ( imageInfo.attsStr[3].c_str() );
      anyErrors( nc_put_att_float(ncid, satV_id, "add_offset", NC_FLOAT, 1, attValue) );

   }


   attValue[0] = MISSIING_VALUE;
   anyErrors( nc_put_att_float(ncid, satV_id, "missing_value", NC_FLOAT, 1, attValue) );

   anyErrors( nc_put_att_text(ncid, satV_id, "stagger", 1, "M") );

   return satV_id;

}


/*****************************************************/
/*      39. define WRF NetCDF 4 dimensions           */
/*****************************************************/
void defineWRFNC4Dimensions ( int ncid, size_t dateStr_len, size_t west_east_len, size_t south_north_len,
                               int *time_dim, int *dateStr_dim, int *west_east_dim, int *south_north_dim )
{

   printf( "Define dimensions in output netcdf file...\n" );

   printf( "\tTime\n" );
   anyErrors( nc_def_dim(ncid, "Time", NC_UNLIMITED, time_dim) );

   printf( "\tDateStrLen\n" );
   anyErrors( nc_def_dim(ncid, "DateStrLen", dateStr_len, dateStr_dim) );

   printf( "\twest_east\n" );
   anyErrors( nc_def_dim(ncid, "west_east", west_east_len, west_east_dim) );

   printf( "\tsouth_north\n\n" );
   anyErrors( nc_def_dim(ncid, "south_north", south_north_len, south_north_dim) );    

}


/***************************************/
/*      40. defineWRFNCTimeVars        */
/***************************************/
void defineWRFNCTimeVars ( int ncid, int time_dim, int dateStr_dim, string startDateTime, int *time_id, int *timeStr_id)
{

   int   dimIndex[NC_MAX_DIMS];    //dimension index array for write out array

   printf( "Define time variables in output netcdf file...\n" );

   //Times_sec float variable
   printf( "\tTimes_sec\n" );
   dimIndex[0] = time_dim;
   anyErrors( nc_def_var(ncid, "Times_sec", NC_FLOAT, 1, dimIndex, time_id) );

   string tmp_str = string ("Seconds since ");
   tmp_str.append ( startDateTime );
   anyErrors( nc_put_att_text(ncid, *time_id, "description", tmp_str.size(), tmp_str.c_str() ) );
   anyErrors( nc_put_att_text(ncid, *time_id, "units", 7, "Seconds") );

   //Times string variable
   printf( "\tTimes\n" );
   dimIndex[0] = time_dim;
   dimIndex[1] = dateStr_dim;
   anyErrors( nc_def_var(ncid, "Times", NC_CHAR, 2, dimIndex, timeStr_id) );
   anyErrors( nc_put_att_text(ncid, *timeStr_id, "description", 21, "Time string (19 char)" ) );
   anyErrors( nc_put_att_text(ncid, *timeStr_id, "units", 15, "yyyymmddhhmm:ss") );

}

/*****************************************************************/
/*   41. compute domain grid indexes in sat image using ANN      */
/*****************************************************************/
bool  computeDomainGridImageIndex (int *grdIndex, double *longP, double *latP,
                                     gridInfo imageInfoLat, gridInfo newRasterInfo, double searchRadius)
{
   int       i, j, index;
   double    satXmin = 999999999.0;
   double    satXmax = -999999999.0;
   double    satYmin = 999999999.0;
   double    satYmax = -999999999.0;
   int       totalNums = 0;    //number grid cells obtained image data index
   
   /***********************************************
   *   get info from rasterized grid domain image *
   ***********************************************/
   double xMin_grd = newRasterInfo.xmin;
   double xMax_grd = newRasterInfo.xmax;

   double yMax_grd = newRasterInfo.ymax;
   double yMin_grd = newRasterInfo.ymin;

   int xCells_grd = newRasterInfo.cols;
   int yCells_grd = newRasterInfo.rows;

   double xCellSize_grd = newRasterInfo.xCellSize;
   double yCellSize_grd = newRasterInfo.yCellSize;

   /**********************************************
   * Define satellite geolocation projection     *
   **********************************************/
   projUV    xyP;
   projPJ    proj4To, proj4From;
   string    proj4Str;

   //satellite projection
   proj4Str = string(imageInfoLat.strProj4);
   printf ( "\n\tProj4From = %s\n", proj4Str.c_str() );
   proj4From = pj_init_plus( proj4Str.c_str() );
   if ( proj4From == NULL )
   {
      printf( "\tInitializing Satellite Proj4 projection failed: %s.\n",  proj4Str.c_str() );
      exit( 1 );
   }

   //grid domain projection
   proj4Str = string( newRasterInfo.strProj4 );
   printf ( "\tProj4To = %s\n", proj4Str.c_str() );
   proj4To = pj_init_plus ( proj4Str.c_str() );
   if (proj4To == NULL)
   {
      printf( "\tInitializing grid domain Proj4 projection failed: %s\n", proj4Str.c_str() );
      exit ( 1 );
   }

   //get variable attributes
   float scaleF = atof ( imageInfoLat.attsStr[2].c_str() );
   float offset = atof ( imageInfoLat.attsStr[3].c_str() );
   float SAT_MISSIING_VALUE = atof ( imageInfoLat.attsStr[4].c_str() );
   printf ( "\tGeolocation attributes: MISSIING_VALUE = %lf  scale_factor = %f  add_offset = %f\n", 
               SAT_MISSIING_VALUE,scaleF,offset);

   int rows = imageInfoLat.rows;
   int cols = imageInfoLat.cols;

   /**************************************
   * Project lat and long arrays         *
   ***************************************/
   int            nPts = rows*cols;   // actual number of data points
   ANNpointArray  dataPts;            // data points
   int            dim = 2;

   dataPts = annAllocPts(nPts, dim); // allocate data points

   //project lat and long into grid domain projection
   for (i=0; i<rows; i++)
   {
      for (j=0; j<cols; j++)
      {
         index = i*cols + j;
         if ( latP[index] != SAT_MISSIING_VALUE && longP[index] != SAT_MISSIING_VALUE )
         {
            //float latY = latP[index] * scaleF + offset;
            //float longX = longP[index] * scaleF + offset;

            float latY = latP[index];    //MODIS L1 geolocation does not have offset and scale
            float longX = longP[index];  

            xyP = projectPoint ( proj4From, proj4To, longX,latY);

            if ( xyP.u == HUGE_VAL )
            {
               printf ("Error: HUGE_VAL from projectPoint call in latlong to grid domain projection.\n");
               exit ( 1 );
            }

            dataPts[index][0] = xyP.u;
            dataPts[index][1] = xyP.v;

            satXmin = min ( satXmin, xyP.u );
            satXmax = max ( satXmax, xyP.u );

            satYmin = min ( satYmin, xyP.v );
            satYmax = max ( satYmax, xyP.v );

            //printf ("%d,%lf,%lf\n",index+1,xyP.u,xyP.v );

         }
      }
   }

   printf ("\tSatellite data extent: x(%lf, %lf)  y(%lf, %lf)\n", satXmin, satXmax, satYmin, satYmax );

   //modeling grid and satellite image does not intersect
   if ( satXmin >= xMax_grd || satXmax <= xMin_grd || satYmin >= yMax_grd || satYmax <= yMin_grd )
   {

      annDeallocPts ( dataPts );
      annClose();       // done with ANN

      return false;
   }


   /************************************
   * Use ANN to get the nearest point  *
   *************************************/
   int           k = 1;       // number of nearest neighbors
   double        eps = 0.0;   // error bound
   ANNpoint      queryPt;     // query point
   ANNidxArray   nnIdx;       // near neighbor indices
   ANNdistArray  dists;       // near neighbor distances
   ANNkd_tree    *kdTree;     // search structure

   queryPt = annAllocPt(dim); // allocate query point
   nnIdx = new ANNidx[k];     // allocate near neigh indices
   dists = new ANNdist[k];    // allocate near neighbor dists


   printf ( "\tBuild geolocation point trees.\n" );
   kdTree = new ANNkd_tree( // build search structure
                           dataPts, // the data points
                           nPts, // number of points
                           dim); // dimension of space


   /*****************************************
   * Convert rasterized domain x-y to       *
   * row and col in satellite geolocation   *
   * latlong arrays                         *
   *****************************************/
   
   for (i=0; i<yCells_grd; i++)
   {
      double y = yMax_grd - i * yCellSize_grd - yCellSize_grd / 2.0;
      for (j=0; j<xCells_grd; j++)
      {
         index = i*xCells_grd + j;
         double x = xMin_grd + j * xCellSize_grd + xCellSize_grd / 2.0;
         queryPt[0] = x;
         queryPt[1] = y;

         kdTree->annkSearch( // search
                             queryPt, // query point
                              k, // number of near neighbors
                              nnIdx, // nearest neighbors (returned)
                              dists, // distance (returned)
                              eps); // error bound
         // print summary
         dists[0] = sqrt(dists[0]); // unsquare distance
         if ( dists[0] < searchRadius)   //get value within the radius
         {
            //printf ( "\tGeolocation tree: i=%d      nnIndex=%d        nnDist=%lf\n",index,nnIdx[0], dists[0] );
            grdIndex[index] = nnIdx[0];
            totalNums ++;
         }
         else
         {
            grdIndex[index] = -999;
         }

      }  //j
   }  //i


   annDeallocPts ( dataPts );
   annDeallocPt (queryPt );
   delete [] nnIdx;   // clean things up
   delete [] dists;
   delete kdTree;
   annClose();       // done with ANN

   printf ( "\tNumber of rasterized grid cells with index numbers: %d\n",totalNums );

   if ( totalNums == 0 )
   {
      return false;
   }
   else
   {
      return true;
   }

}


/**********************************************************/
/*   42. compute domain grid satellite image value        */
/**********************************************************/
void  computeGridSatValues ( GUInt32 *poImage_grd, int *grdIndex, float *satV, double *poImage, 
                             gridInfo imageInfo, gridInfo newRasterInfo, gridInfo grid )
{
   double          halfGridCellArea;    //half of domain cell area
   int             *gridIDs=NULL;       //array to store number of pixels in each modeling grids
   double          *gridValues = NULL;  //array to store total values in each modeling grids

   int             i,j,k,m;


   /*******************************************
   *  get info from the model domain grid     *
   *******************************************/
   //total dimension size

   int gridRows = grid.rows;
   int gridCols = grid.cols;

   int  totalDomainSize = gridCols*gridRows;  //for output dimensions

   for ( i=2; i<imageInfo.dims.size(); i++ )
   {
      totalDomainSize *= imageInfo.dims[i];
   }
   //printf ( "\tTotal domain grid dimension size = %d\n", totalDomainSize );

   halfGridCellArea = grid.xCellSize * grid.yCellSize / 2.0 ;   //half of domain cell area

   /******************************
   *   Get satllite image  Info *
   *****************************/
   int     xCells = imageInfo.cols;
   int     yCells = imageInfo.rows;
   float   SAT_MISSIING_VALUE = atof ( imageInfo.attsStr[4].c_str() );

   printf ( "\tSAT_MISSIING_VALUE = %lf\n", SAT_MISSIING_VALUE);

   /***********************************************************************************
   *     Allocate memory store pixel number and total value in each modeling grid    *
   ***********************************************************************************/
   gridIDs = (int *) CPLCalloc(sizeof(int),totalDomainSize);
   gridValues = (double *) CPLCalloc(sizeof(double),totalDomainSize);


   /*************************************************
   *  get info from rasterized grid domain image    *
   *************************************************/
   int xCells_grd = newRasterInfo.cols;
   int yCells_grd = newRasterInfo.rows;
   //printf( "\tGrid domain cells are: %dx%d\n", xCells_grd, yCells_grd );

   double xCellSize_grd = newRasterInfo.xCellSize;
   double yCellSize_grd = newRasterInfo.yCellSize;
   //printf( "\tGrid domain re-sampled pixel size = (%.3lf,%.3lf)\n", xCellSize_grd, yCellSize_grd );

   double xMin_grd = newRasterInfo.xmin;
   double xMax_grd = newRasterInfo.xmax;

   double yMax_grd = newRasterInfo.ymax;
   double yMin_grd = newRasterInfo.ymin;

   //printf( "\tDoamin UL Origin = (%.6lf,%.6lf)\n", xMin_grd, yMax_grd );
   //printf( "\tDomain grid raster extent:  minXY(%.3lf,%.3lf)   maxXY(%.3lf,%.3lf)\n", xMin_grd,yMin_grd,xMax_grd,yMax_grd );


   /****************************************************
   *  re-gridding last dimension as attribute item    *
   ****************************************************/
   int          gridID, pixelIndex;
   double       value;
   double       x,y;
   int          col,row;   //in satellite image
   int          geoIndex;
   int          dmnIndex,satIndex;


   int   gridPixels = gridCols*gridRows;   //total grids in modeling domain

   //loop through rasterized domain image from UL corner
   for( i=0; i<yCells_grd; i++ )
   {
      for (j=0; j<xCells_grd; j++)
      {
         pixelIndex = i * xCells_grd + j; 
         gridID = poImage_grd[pixelIndex];   //domain grid ID from 1

         if (gridID > 0 && gridID <= gridPixels)
         {
            //get row and col indices in the model domain from LL corner
            int dmnRow = (int) ( floor ( ( gridID - 1 ) / gridCols) );   //start from 0
            int dmnCol = ( gridID - 1 ) % gridCols;                      //start from 0
            //printf ("\t\tgridID=%d    dmnRow=%d   dmnCol=%d\n",gridID,dmnRow,dmnCol);

            geoIndex = grdIndex[pixelIndex];  //get the index to get the value image rowsXcols starting from 0

            if ( geoIndex != -999 )
            {
               //get sat image geo indices for (1st and 2nd dimensions) based on grdIndex result
               //compute geo-site row, col
               int satRow = (int) ( floor (geoIndex / xCells) );   //start from 0
               int satCol = geoIndex % xCells;                     //start from 0
               //printf ("\tgeoIndex=%d    row=%d   col=%d\n",geoIndex,satRow,satCol);

               if ( imageInfo.dims.size() == 2 )
               {
                  satIndex = geoIndex;
                  dmnIndex = gridID - 1;

                  value = poImage[satIndex];
                  if ( value != SAT_MISSIING_VALUE )
                  {
                     gridIDs[dmnIndex] += 1;             //count Satellite cells
                     gridValues[dmnIndex] += value;      //sum the Satellite value for a domain grid
                  }
               }  //2dims
               else if ( imageInfo.dims.size() == 3 )
               {
                  for ( k=0; k<imageInfo.dims[2]; k++)
                  {
                     satIndex = satRow * xCells * imageInfo.dims[2] + satCol * imageInfo.dims[2] + k;
                     dmnIndex = dmnRow * gridCols * imageInfo.dims[2] + dmnCol * imageInfo.dims[2] + k;
                     value = poImage[satIndex];
                     if ( value != SAT_MISSIING_VALUE )
                     {
                        //printf ("\t\tk=%d    value=%lf\n",k,value);
                        gridIDs[dmnIndex] += 1;             //count Satellite cells
                        gridValues[dmnIndex] += value;      //sum the Satellite value for a domain grid
                     }
                  }  //k
               } //3dims

               else if ( imageInfo.dims.size() == 4 )
               {
                  for ( k=0; k<imageInfo.dims[2]; k++)
                  {
                     for ( m=0; m<imageInfo.dims[3]; m++)
                     {
                        satIndex = satRow * xCells * imageInfo.dims[2] * imageInfo.dims[3] +
                                   satCol * imageInfo.dims[2] * imageInfo.dims[3] + k * imageInfo.dims[3] + m;
                        dmnIndex = dmnRow * gridCols * imageInfo.dims[2] * imageInfo.dims[3] +
                                   dmnCol * imageInfo.dims[2] * imageInfo.dims[3] + k * imageInfo.dims[3] + m;
                        value = poImage[satIndex];
                        if ( value != SAT_MISSIING_VALUE )
                        {
                           gridIDs[dmnIndex] += 1;             //count Satellite cells
                           gridValues[dmnIndex] += value;      //sum the Satellite value for a domain grid
                        }
                     } //m
                  }  //k
               }  //4 dims

            }  //check missing geolocation index
         }  //check gridID

      }  //j
   }  //i

   //loop through modeling domain cells to fill netcdf array
   for (i=0; i<gridRows; i++)
   {
      for (j=0; j<gridCols; j++)
      {
         if ( imageInfo.dims.size() == 2 )
         {
            dmnIndex = i * gridCols + j;
            if ( gridIDs[dmnIndex] * xCellSize_grd * yCellSize_grd >=  halfGridCellArea )
            {
               value = gridValues[dmnIndex] / gridIDs[dmnIndex];    //take average for all satellite variables
               satV[dmnIndex] = value;
            }
            else
            {
               satV[dmnIndex] = MISSIING_VALUE;
            }
         }   //2dims

         else if ( imageInfo.dims.size() == 3 )
         {
            for ( k=0; k<imageInfo.dims[2]; k++)
            {
               dmnIndex = i * gridCols * imageInfo.dims[2] + j * imageInfo.dims[2] + k;
               //make sure that at least half of domain grid has values
               //double area_cover_ratio = gridIDs[dmnIndex] * xCellSize_grd * yCellSize_grd / (halfGridCellArea*2);
               //printf ("\t row=%d  col=%d  k=%d  counts=%d  total=%lf  cover_ratio=%lf \n",
               //             i, j, k,gridIDs[dmnIndex], satV[dmnIndex], area_cover_ratio);

               if ( gridIDs[dmnIndex] * xCellSize_grd * yCellSize_grd >=  halfGridCellArea )
               {
                  value = gridValues[dmnIndex] / gridIDs[dmnIndex];    //take average for all satellite variables
                  satV[dmnIndex] = value;
               }
               else
               {
                  satV[dmnIndex] = MISSIING_VALUE;
               }
            } //k
         }   //3dims

         else if ( imageInfo.dims.size() == 4 )
         {
            for ( k=0; k<imageInfo.dims[2]; k++)
            {
               for ( m=0; m<imageInfo.dims[3]; m++)
               {
                  dmnIndex = i * gridCols * imageInfo.dims[2] * imageInfo.dims[3] +
                             j * imageInfo.dims[2] * imageInfo.dims[3] + k * imageInfo.dims[3] + m;
                  if ( gridIDs[dmnIndex] * xCellSize_grd * yCellSize_grd >=  halfGridCellArea )
                  {
                     value = gridValues[dmnIndex] / gridIDs[dmnIndex];    //take average for all satellite variables
                     satV[dmnIndex] = value;
                  }
                  else
                  {
                     satV[dmnIndex] = MISSIING_VALUE;
                  }
               } //m
            } //k
         }  //4 dims

      }  //j
   } //i

   CPLFree (gridIDs);
   CPLFree (gridValues);

}


/****************************************/
/*   43. write WRF char variable        */
/****************************************/
void writeWRFCharVariable ( int ncid, int dimNum, size_t dimLen, size_t textLen, int var_id, char *charStr_epic, string arrayName)
{
   long tx_start[dimNum];
   long tx_count[dimNum];

   //printf ("dimLen=%d  textLen=%d  %s\n", dimLen, textLen,charStr);
   if ( dimNum == 2 )
   {
      tx_start[0]=0;
      tx_start[1]=0;

      tx_count[0]=dimLen;
      tx_count[1]=textLen;
   }
   else if ( dimNum == 3 )
   {
      tx_start[0]=0;
      tx_start[1]=0;
      tx_start[2]=0;

      tx_count[0]=1;
      tx_count[1]=dimLen;
      tx_count[2]=textLen;
   }
   else
   {
      printf ( "Error: char array dimensions have to be 2 or 3.\n");
      exit ( 1 );
   }

   anyErrors( ncvarput(ncid, var_id, tx_start, tx_count, charStr_epic) );

   printf( "\tWrited %s array\n", arrayName.c_str() );
 
   //free ( charStr_epic );
}


/****************************************/
/*   44. define NetCDF float variable   */
/****************************************/
int defineNCFloatVariable (int ncid, const char *varName, int numDims, int *dimIndex, const char *varDesc, const char *varUnit, float scaleFactor, float offset )
{

   int     var_id;
   int     fieldtype[1];
   float   attValue[1];

   string  tmp_str;

   fieldtype[0] = 104;

   printf ("\tDefined variable: %s\n", varName );

   anyErrors( nc_def_var(ncid, varName, NC_FLOAT, numDims, dimIndex, &var_id) );

   anyErrors( nc_put_att_int(ncid, var_id, "FieldType", NC_INT, 1, fieldtype) );
   anyErrors( nc_put_att_text(ncid, var_id, "MemoryOrder", 2, "XY") );
  
   tmp_str = string ( varDesc );
   anyErrors( nc_put_att_text(ncid, var_id, "description", tmp_str.size(), varDesc ) );

   tmp_str = string ( varUnit ) ;
   anyErrors( nc_put_att_text(ncid, var_id, "units", tmp_str.size(), varUnit ) );

   attValue[0] = scaleFactor;
   anyErrors( nc_put_att_float(ncid, var_id, "scale_factor", NC_FLOAT, 1, attValue) );

   attValue[0] = offset;
   anyErrors( nc_put_att_float(ncid, var_id, "add_offset", NC_FLOAT, 1, attValue) );

   attValue[0] =  MISSIING_VALUE;
   anyErrors( nc_put_att_float(ncid, var_id, "missing_value", NC_FLOAT, 1, attValue) );

   anyErrors( nc_put_att_text(ncid, var_id, "stagger", 1, "M") );

   return var_id;

}


/***********************************
*  45.  readIOAPIVar               *
************************************/
void readIOAPIVar ( ncVarData *mcipData,  int ncid, int var_id, const char *varName, nc_type var_type, int var_ndims, int *var_dimids, size_t *dimSizes, size_t *varDimSize )
{
     size_t     dimSize;
     int        var_dimid;
     int        totalSize;
     int        j;
   
    
     printf ("\tDimensions: " );
     totalSize = 1;
     for (j=0; j<var_ndims; j++)
     {
        //total dim size
        var_dimid = var_dimids[j];
        totalSize = totalSize * dimSizes[var_dimid];
        varDimSize[j] = dimSizes[var_dimid];             //get var dimension size
        printf ("%d ", dimSizes[var_dimid] );
     }

     printf ("   Total size = %d\n", totalSize);
    
     /************************
     * obtain int varaible   *
     ************************/
     if ( var_type == NC_INT )
     {
        //allocate mem for a IOAPI time INT variable
        if ( (mcipData->intData = (int*) calloc (totalSize, sizeof(int)) ) == NULL)
        {
           printf( "Calloc mcipData->intData failed.\n");
           exit ( 1 );
        }

        //get variable value
        anyErrors( nc_get_var_int(ncid, var_id, mcipData->intData ) );

/*        //checking data
        for (j = 1000; j<1010; j++)
        {
         printf ("\t\t%d\n", mcipData->intData [j] );
        }
*/
     }
     else if ( var_type == NC_FLOAT )
     {
        //allocate mem for a IOAPI float variable
        if ( (mcipData->floatData = (float*) calloc (totalSize, sizeof(float)) ) == NULL)
        {
           printf( "Calloc mcipData->floatData failed.\n");
           exit ( 1 );
        }

        //get variable value
        anyErrors( nc_get_var_float(ncid, var_id, mcipData->floatData) );


/*        //checking data
        for (j = 900; j<1000; j++)
        {
           printf ("\t\t%f\n", mcipData->floatData[j]);
        }
 */     

     }
     else
     {
        //not correct type in IOAPI time and other variable
        printf( "\tIOAPI variable is not int or float.\n" );
        exit ( 1 );
     }

     printf ("\tObtained variable: %s\n",varName );

}


/***************************************/
/*   46. define NetCDF char variable   */
/***************************************/
int defineNCCharVariable (int ncid, const char *varName, int numDims, int *dimIndex, const char *varDesc, const char *varUnit )
{
   int     var_id;
   string  tmp_str;

   printf ("\tDefined variable: %s\n", varName );

   anyErrors( nc_def_var(ncid, varName, NC_CHAR, numDims, dimIndex, &var_id) );

   tmp_str = string ( varDesc );
   anyErrors( nc_put_att_text(ncid, var_id, "description", tmp_str.size(), varDesc ) );

   tmp_str = string ( varUnit ) ;
   anyErrors( nc_put_att_text(ncid, var_id, "units", tmp_str.size(), varUnit ) );

   return var_id;
}



/**************************************/
/*   47. define NetCDF int variable   */
/**************************************/
int defineNCIntVariable (int ncid, const char *varName, int numDims, int *dimIndex, const char *varDesc, const char *varUnit, int scaleFactor, int offset )
{

   int     var_id;
   int     fieldtype[1];
   int     attValue[1];

   string  tmp_str;

   fieldtype[0] = 104;

   printf ("\tDefined variable: %s\n", varName );

   anyErrors( nc_def_var(ncid, varName, NC_INT, numDims, dimIndex, &var_id) );

   anyErrors( nc_put_att_int(ncid, var_id, "FieldType", NC_INT, 1, fieldtype) );

   anyErrors( nc_put_att_text(ncid, var_id, "MemoryOrder", 2, "XY") );
  
   tmp_str = string ( varDesc );
   anyErrors( nc_put_att_text(ncid, var_id, "description", tmp_str.size(), varDesc ) );

   tmp_str = string ( varUnit ) ;
   anyErrors( nc_put_att_text(ncid, var_id, "units", tmp_str.size(), varUnit ) );

   attValue[0] = scaleFactor;
   anyErrors( nc_put_att_int(ncid, var_id, "scale_factor", NC_INT, 1, attValue) );

   attValue[0] = offset;
   anyErrors( nc_put_att_int(ncid, var_id, "add_offset", NC_INT, 1, attValue) );

   attValue[0] =  MISSIING_VALUE_INT;
   anyErrors( nc_put_att_int(ncid, var_id, "missing_value", NC_INT, 1, attValue) );

   anyErrors( nc_put_att_text(ncid, var_id, "stagger", 1, "M") );

   return var_id;

}

/***********************************************************************/
/*  48.     computeNewRasterInfo_fromImage from image to be projected  */
/***********************************************************************/
gridInfo computeNewRasterInfo_fromImage ( double rasterResolution, gridInfo grid, gridInfo imageInfo )
{
    gridInfo   newRasterInfo;

    double   xMin = 999999999999.0;
    double   xMax = -999999999999.0;
    double   yMin = 999999999999.0;
    double   yMax = -999999999999.0;
  
    projPJ   proj4From, proj4To;
    double   x,y; 
    projUV   xyP;
    string   proj4Strfrom, proj4Strto, proj4Str;
    
    printf( "\nComputing new raster information from image to grid...\n");


    printf ( "\tRaster resolution is: %.2lf\n", rasterResolution );


    //input MODIS image projection
    proj4Strfrom = string(grid.strProj4);
    printf ( "\n\tProj4From = %s\n", proj4Strfrom.c_str() );
    proj4From = pj_init_plus( proj4Strfrom.c_str() );
    if ( proj4From == NULL )
    {
       printf( "\tInitializing input image Proj4 projection failed: %s.\n",  proj4Strfrom.c_str() );
       exit( 1 );
    }

    //output image projection in grid domain
    proj4Strto = string( imageInfo.strProj4 );
    printf ( "\tProj4To = %s\n", proj4Strto.c_str() );
    proj4To = pj_init_plus ( proj4Strto.c_str() );
    if (proj4To == NULL)
    {
       printf( "\tInitializing outpput imaage Proj4 projection failed: %s\n", proj4Strto.c_str() );
       exit ( 1 );
    }

    if ( compareGridProjection (grid, imageInfo) )
    {
       //same projections
       xMin = grid.xmin;
       xMax = grid.xmax;
       yMin = grid.ymin;
       yMax = grid.ymax;
    }
    else if ( proj4Strfrom.find ( "sinu" ) != string::npos && proj4Strto.find ( "sinu" ) == string::npos ) 
    {
       //PROJ4 creates errors when projecting centain area from sinu to other projections
       //assume that imageInfo contains the extent for grid to map to. E.g. map MODIS to grid domain
       //this is specific for MODIS land cover data reprojection from SINU projection

       xMin = imageInfo.xmin;
       yMin = imageInfo.ymin;

       xMax = imageInfo.xmax;
       yMax = imageInfo.ymax;

    }
    else
    {
       //compute four edges to find extent
       double x, y;
       int    i;
 
       //bottom row
       for ( i=0; i<=grid.cols; i++ )  
       {
          x = grid.xmin + i * grid.xCellSize;          
          y = grid.ymin;
           
          xyP = projectPoint ( proj4From, proj4To, x, y);

          if ( xyP.u != HUGE_VAL )
          {
            xMin = min (xMin, xyP.u);
            xMax = max (xMax, xyP.u);
            yMin = min (yMin, xyP.v);
            yMax = max (yMax, xyP.v);
          }
       }

       //top row
       for ( i=0; i<=grid.cols; i++ )
       {
          x = grid.xmin + i * grid.xCellSize;
          y = grid.ymax;

          xyP = projectPoint ( proj4From, proj4To, x, y);

          if ( xyP.u != HUGE_VAL )
          {
             xMin = min (xMin, xyP.u);
             xMax = max (xMax, xyP.u);
             yMin = min (yMin, xyP.v);
             yMax = max (yMax, xyP.v);
          }
       }

       //left column 
       for ( i=0; i<=grid.rows; i++ ) 
       {
          x = grid.xmin;
          y = grid.ymin + i * grid.yCellSize;  
  
          xyP = projectPoint ( proj4From, proj4To, x, y);

          if ( xyP.u != HUGE_VAL )
          {
             xMin = min (xMin, xyP.u);
             xMax = max (xMax, xyP.u);
             yMin = min (yMin, xyP.v);
             yMax = max (yMax, xyP.v);
          }
       }

       //right column
       for (int i=0; i<=grid.rows; i++ )
       {
          x = grid.xmax;
          y = grid.ymin + i * grid.yCellSize; 
 
          xyP = projectPoint ( proj4From, proj4To, x, y);
          
          if ( xyP.u != HUGE_VAL )
          {
             xMin = min (xMin, xyP.u);
             xMax = max (xMax, xyP.u);
             yMin = min (yMin, xyP.v);
             yMax = max (yMax, xyP.v);
          }
       }
       

       pj_free ( proj4From );
       pj_free ( proj4To );

    }

    printf("\tProjected  extent minXY: (%lf, %lf) maxXY: (%lf, %lf)\n", xMin, yMin, xMax, yMax);

    //compute new extent to match resolution
    
    xMin = ( floor(xMin / rasterResolution) ) * rasterResolution - rasterResolution;
    yMin = ( floor(yMin / rasterResolution) ) * rasterResolution - rasterResolution;

    xMax = ( ceill(xMax / rasterResolution ) ) * rasterResolution + rasterResolution;
    yMax = ( ceill(yMax / rasterResolution ) ) * rasterResolution + rasterResolution;

    int xcells = (int) ( (xMax - xMin) / rasterResolution );
    int ycells = (int) ( (yMax - yMin) / rasterResolution );
  
    printf("\tModified extent minXY: (%f, %f) maxXY: (%f, %f)\n", xMin, yMin, xMax, yMax);
    printf("\txcells = %d   ycells = %d\n",xcells,ycells);

    string tmp_str = string ( imageInfo.strProj4 );

    newRasterInfo.strProj4 = strdup ( tmp_str.c_str() );
    printf ( "\tProj4 projection is: %s\n", newRasterInfo.strProj4);

    //assign info to newRasterInfo
    newRasterInfo.xCellSize = rasterResolution;
    newRasterInfo.yCellSize = rasterResolution;
    newRasterInfo.rows = ycells;
    newRasterInfo.cols = xcells;
    newRasterInfo.xmin = xMin;
    newRasterInfo.ymin = yMin;
    newRasterInfo.xmax = xMax;
    newRasterInfo.ymax = yMax;

    if ( imageInfo.attsStr.size() > 0 )
    { 
       for ( int i=0; i<imageInfo.attsStr.size(); i++)
       {
          newRasterInfo.attsStr.push_back( imageInfo.attsStr.at(i) );
       }
    }

    return newRasterInfo;
}

/**************************************/
/*   49. project raster image         */
/**************************************/
string projectImage (string gdalBinDir, string imageFile, gridInfo inGrid, gridInfo outGrid )
{
    string  outImage;
    string  cmd_str;
    string  proj4From, proj4To;
    char     extStr[150], sizStr[150];
    int     i;
    
   
    printf( "\nProject an image data file...\n");

    proj4From = string ( inGrid.strProj4 );
    proj4To = string ( outGrid.strProj4 );
    
    //get random file name in the current directoty
    string ext = string ( "_img.bil" );
    outImage = getRandomFileName ( ext );

    printf ("\tProjected image data is stored in file: %s\n", outImage.c_str() );
 
    // build arguments to call gdalwarp

    //get image cell information
    cmd_str = string(gdalBinDir);
    cmd_str.append("gdalwarp -s_srs \"");
    cmd_str.append( proj4From );
    cmd_str.append("\"  -t_srs \"");
    cmd_str.append( proj4To );
    cmd_str.append("\"  -te ");

    sprintf(extStr, "%f %f %f %f -tr %.4lf  %.4lf \0",outGrid.xmin,outGrid.ymin,outGrid.xmax,outGrid.ymax,outGrid.xCellSize,outGrid.yCellSize);
    cmd_str.append(extStr);
    cmd_str.append( imageFile );
    cmd_str.append ( " ");
    cmd_str.append ( outImage );
    printf("\t%s\n", cmd_str.c_str());

    //call gdalwrap to clip and reproject it
    if ( system(cmd_str.c_str()) != 0 )
    {
       printf("\tError in system call: %s\n", cmd_str.c_str());
       exit (1);
    }

    printf ("\tCompleted projecting the image data.\n");


    return outImage;

}



/***************************************************/
/*  50.  Copy an image information data structure  */
/***************************************************/
gridInfo copyImageInfo ( gridInfo grid )
{
    gridInfo   copyGrid;

    printf ( "\nCopy image or grid information\n" );


    string tmp_str = string ( grid.strProj4 );
    copyGrid.strProj4 = strdup ( tmp_str.c_str() );

    //assign info
    copyGrid.xCellSize = grid.xCellSize;
    copyGrid.yCellSize = grid.yCellSize;

    copyGrid.rows = grid.rows;
    copyGrid.cols = grid.cols;

    copyGrid.xmin = grid.xmin;
    copyGrid.ymin = grid.ymin;
    copyGrid.xmax = grid.xmax;
    copyGrid.ymax = grid.ymax;

    return copyGrid;
}


/***********************************/
/*  51.  Get a random file name    */
/***********************************/
string  getRandomFileName ( string ext ) 
{
    unsigned int seed = 1;
    unsigned int  i = 0;
    string fileName;
    
    struct stat stFileInfo;
    int intStat;


    //get random number for file name
    do
    {
       srand( time(0) + seed * i);     // Initialize random number generator based on current time
       int random_int = rand();

       char random_str[256];
       sprintf(random_str, "tmp%d\0",random_int);

       fileName = string ( random_str ) + ext;
       i++;

       //check the file
        intStat = stat(fileName.c_str(),&stFileInfo);
    } while (intStat == 0 );

    return fileName;
}



/**********************************************************/
/*  52.  Get All MODIS Land Cover Tile File Information   */
/**********************************************************/
gridInfo  getAllMODISTileInfo (  std::vector<string> modisFiles, string varName )
{

   gridInfo  allInfo;
   int       i;

   //obtain extent of MODIS tile files.  Loop through each file
   for (i=0; i<modisFiles.size(); i++)
   {

      string imageFile = modisFiles.at(i);
     
      if ( i == 0 )
      {
         allInfo = getHDF4VarInfo (imageFile, varName);
         
         //printGridInfo  ( allInfo );
      }
      else
      {
         gridInfo modisInfo = getHDF4VarInfo (imageFile, varName);
        
         //printGridInfo  ( modisInfo );
      
         allInfo.xmin = min ( allInfo.xmin, modisInfo.xmin );
         allInfo.xmax = max ( allInfo.xmax, modisInfo.xmax );

         allInfo.ymin = min ( allInfo.ymin, modisInfo.ymin );
         allInfo.ymax = max ( allInfo.ymax, modisInfo.ymax );
      }
   }
   
      allInfo.cols = int ( (allInfo.xmax - allInfo.xmin) / allInfo.xCellSize );
      allInfo.rows = int ( (allInfo.ymax - allInfo.ymin) / allInfo.yCellSize );
      
      printf ("\n\nThe extent of MODIS files is: minXY: %.8lf, %.8lf  maxXY: %.8lf, %.8lf\n\n",
               allInfo.xmin, allInfo.ymin, allInfo.xmax, allInfo.ymax );
      
      return allInfo;
}



/****************************************************/
/*  53.  Extract MODIS data out for a domain area   */
/****************************************************/
string  extractDomainMODISData ( std::vector<string> modisFiles, string varName, gridInfo grid)
{

   const char     *pszFormat = "EHdr";
   GDALDataset    *poDstDS;       
   OGRSpatialReference oSRS;
   char           *pszWKT = NULL;

   GDALDriver     *poDriver;
   GDALRasterBand *poBand;
   char           **papszOptions = NULL;
   char           **papszMetadata;

   int            i,j,k,n;


   printf("\nExtracting image data for a given domain grid...\n");

   //get random file name in the current directoty
   string ext = string ( "_modis.bil" );
   string imageName = getRandomFileName ( ext );

   /* -------------------------------------------------------------------- */
   /*      Register format(s).                                             */
   /* -------------------------------------------------------------------- */
   GDALAllRegister();

   poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);

   if( poDriver == NULL )
   {
      printf ( "\tCreating a GDAL format driver failed: %s\n", pszFormat);
      exit( 1 );
   }

   poDstDS = poDriver->Create( imageName.c_str(), grid.cols, grid.rows, 1, GDT_Byte, papszOptions );
   double adfGeoTransform[6] = { grid.xmin, grid.xCellSize, 0, grid.ymax, 0, -1*grid.yCellSize };

   poDstDS->SetGeoTransform( adfGeoTransform );
    
   string strProj4_cc = string ( grid.strProj4 );

   oSRS.importFromProj4( strProj4_cc.c_str() );
   oSRS.exportToWkt( &pszWKT );

   if ( (poDstDS->SetProjection( pszWKT )) == CE_Failure )
   {
      printf ( "\tSetting projection failed for the grid raster file: %s\n", imageName.c_str() );
      exit ( 1 );
   }

   poBand = poDstDS->GetRasterBand(1);

   k = grid.rows*grid.cols;
   GByte *poGrid = (GByte *) CPLCalloc(sizeof(GByte),grid.rows*grid.cols);

   //initialize MODIS array to be unclassified
   
   for ( i=0; i<k; i++)
   {
      poGrid[i] = MODIS_LC_FillValue;
   }
   
   
   //get data from each MODIS tile
   for (n=0; n<modisFiles.size(); n++)
   {
      //get MODIS file grid info
      string imageFile = modisFiles.at(n);
      gridInfo modisInfo = getHDF4VarInfo (imageFile, varName);

      if ( modisInfo.xmin >= grid.xmax || modisInfo.xmax <= grid.xmin ||
            modisInfo.ymin >= grid.ymax || modisInfo.ymax <= grid.ymin )
      {
          printf ("\n\tMODIS land cover file does not intersect with the domain: %s\n", imageFile.c_str() );
      } 
      else
      { 
          printf ("\n\tMODIS land cover file intersects with the domain: %s\n", imageFile.c_str() );

          //get data from the MODIS file
          double *poImage = (double *) CPLCalloc(sizeof(double), modisInfo.rows*modisInfo.cols);

          readHDF4SatVarData (imageFile, varName, poImage);   

          double x,y;
          int  col,row;
          for( i = 0; i<modisInfo.rows; i++ )
          {
            
             //get cell center point y and compute row in grid image
             y = modisInfo.ymax - i * modisInfo.yCellSize - modisInfo.yCellSize / 2.0;
             row = (int) (floor ((grid.ymax - y) / grid.yCellSize));   //start from 0

             if ( row >= 0 && row < grid.rows )
             {
                for (j=0; j<modisInfo.cols; j++)
                {
          
                   //get cell center point x and compute col in grid image
                   x = modisInfo.xmin + j * modisInfo.xCellSize + modisInfo.xCellSize / 2.0;
                   col = (int) (floor ((x - grid.xmin) / grid.xCellSize));  //start from 0

                   if ( col >=0 && col < grid.cols )
                   {
                      int gridIndex = grid.cols * row + col;
                      int imageIndex = modisInfo.cols * i + j;

                      //printf ( "\tcol=%d  row=%d  gridIndex=%d\n",col,row,gridIndex );
                      
                      GByte  modisClass =  int ( poImage[imageIndex] );

                      if (  modisClass == 17 || modisClass == 254 )
                      { 
                         modisClass = 0; //set to water class. 254 is often water. But need to make sure!! 
                      }

                      poGrid[gridIndex] = int ( modisClass );  //copy image data to grid

                   }  // check col
                } //j

             }  // check row
          }  //i
         
          printf ( "\tFinished reading: %s\n\n", imageFile.c_str() );
          CPLFree (poImage);

      }  //intersect

   }  //n MODIS file



   //write array
   if ( (poBand->RasterIO( GF_Write, 0, 0, grid.cols, grid.rows, poGrid, 
         grid.cols, grid.rows, GDT_Byte, 0, 0 ) ) == CE_Failure)  
   {
            printf( "\tError in writing data for image: %s.\n", imageName.c_str() );
            CPLFree (poGrid);
            exit( 1 );
   }
   else
   {
      CPLFree (poGrid);
   }


   /* Once we're done, close properly the dataset */
   GDALClose( (GDALDatasetH) poDstDS );

   printf ( "\tCreated grid raster file: %s\n\n", imageName.c_str() );

   return ( imageName );

}



/****************************************************/
/*  54.  Project raster file into a new projection  */
/****************************************************/
string  projectRasterFile ( string imageFile, gridInfo imageInfo, gridInfo grid )
{

   const char     *pszFormat = "EHdr";
   GDALDataset    *poDstDS, *poRDataset;       
   OGRSpatialReference oSRS;
   char           *pszWKT = NULL;

   GDALDriver     *poDriver;
   GDALRasterBand *poBand, *poBandIn;
   char           **papszOptions = NULL;
   char           **papszMetadata;

   projPJ         proj4From, proj4To;
   projUV         xyP;
   string         proj4Str;

   int            i,j,k;


   printf("\nProjecting raster file: %s...\n", imageFile.c_str() );

   //get random file name in the current directoty 
   string ext = string ( "_modisp.bil" );
   string imageName = getRandomFileName ( ext );

   /* -------------------------------------------------------------------- */
   /*      Register format(s).                                             */
   /* -------------------------------------------------------------------- */
   GDALAllRegister();

   poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);

   if( poDriver == NULL )
   {
      printf ( "\tCreating a GDAL format driver failed: %s\n", pszFormat);
      exit( 1 );
   }

   poDstDS = poDriver->Create( imageName.c_str(), grid.cols, grid.rows, 1, GDT_Byte, papszOptions );
   double adfGeoTransform[6] = { grid.xmin, grid.xCellSize, 0, grid.ymax, 0, -1*grid.yCellSize };

   poDstDS->SetGeoTransform( adfGeoTransform );
    
   string strProj4_cc = string ( grid.strProj4 );

   oSRS.importFromProj4( strProj4_cc.c_str() );
   oSRS.exportToWkt( &pszWKT );

   if ( (poDstDS->SetProjection( pszWKT )) == CE_Failure )
   {
      printf ( "\tSetting projection failed for the grid raster file: %s\n", imageName.c_str() );
      exit ( 1 );
   }

   poBand = poDstDS->GetRasterBand(1);   //to write


   //define output array
   k = grid.rows*grid.cols;
   GByte *poGrid = (GByte *) CPLCalloc(sizeof(GByte),grid.rows*grid.cols);

   //initialize array 
   for ( i=0; i<k; i++)
   {
      poGrid[i] = MODIS_LC_FillValue;
   }


   //define input array
   GByte *poImage = (GByte *) CPLCalloc(sizeof(GByte), imageInfo.rows*imageInfo.cols);
   getImageGByteArray ( imageFile, imageInfo, poImage);

   
   //set projections from gridInfo to imageInfo
   //input image projection
   proj4Str = string(grid.strProj4);
   printf ( "\n\tProj4From = %s\n", proj4Str.c_str() );
   proj4From = pj_init_plus( proj4Str.c_str() );
   if ( proj4From == NULL )
   {
       printf( "\tInitializing input image Proj4 projection failed: %s.\n",  proj4Str.c_str() );
       exit( 1 );
   }

   //output image projection
   proj4Str = string( imageInfo.strProj4 );
   printf ( "\tProj4To = %s\n", proj4Str.c_str() );
   proj4To = pj_init_plus ( proj4Str.c_str() );
   if (proj4To == NULL)
   {
      printf( "\tInitializing outpput imaage Proj4 projection failed: %s\n", proj4Str.c_str() );
      exit ( 1 );
   }

  
   double x,y;
   int    col,row;
   for( i = 0; i<grid.rows; i++ )
   {
      y = grid.ymax - i * grid.yCellSize - grid.yCellSize / 2.0;

      for (j=0; j<grid.cols; j++)
      {
         int gridIndex = grid.cols * i + j;

         x = grid.xmin + j * grid.xCellSize + grid.xCellSize / 2.0;
 
         //project x and y
         xyP = projectPoint ( proj4From, proj4To, x, y );

         //compute row and col in input image
   
         if ( xyP.u != HUGE_VAL )
         {
            row = (int) (floor ((imageInfo.ymax - xyP.v) / imageInfo.yCellSize));   //start from 0
            col = (int) (floor ((xyP.u - imageInfo.xmin) / imageInfo.xCellSize));   //start from 0
     
            if ( (row >= 0 && row < imageInfo.rows) && (col >= 0 && col < imageInfo.cols) )
            {
               int imageIndex = imageInfo.cols * row + col;

               //printf ( "\tcol=%d  row=%d  imageIndex=%d\n",col,row,imageIndex );

               poGrid[gridIndex] =  poImage[imageIndex] ;   //copy image data to grid
            }  // check row and col
         }

      } //j
   } //i
         
   printf ( "\n\tFinished projecting: %s\n", imageFile.c_str() );
   CPLFree (poImage);


   //write array
   if ( (poBand->RasterIO( GF_Write, 0, 0, grid.cols, grid.rows, poGrid, 
         grid.cols, grid.rows, GDT_Byte, 0, 0 ) ) == CE_Failure)  
   {
      printf( "\tError in writing data for image: %s.\n", imageName.c_str() );
      CPLFree (poGrid);
      exit( 1 );
   }


   /* Once we're done, close properly the dataset */
   GDALClose( (GDALDatasetH) poDstDS );

   printf ( "\tCreated grid raster file: %s\n\n", imageName.c_str() );

   return ( imageName );

}


/**************************************/
/* 55.  get a GDAL image array data   */
/**************************************/
void  getImageGByteArray ( string imageFile, gridInfo imageInfo, GByte *poImage)
{
   GDALDataset    *poRDataset;
   GDALDriver     *poDriver;
   GDALRasterBand *poBand;

   /* -------------------- */
   /*      Open image      */
   /* -------------------- */
   //open the input image
   poRDataset = (GDALDataset *) GDALOpen( imageFile.c_str(), GA_ReadOnly );
   if( poRDataset == NULL )
   {
      printf( "\tError: Open raster file failed: %s.\n", imageFile.c_str() );
      exit( 1 );
   }

   /* -------------------------------------------------------------------- */
   /*    Image data type has to be GByte                                    */
   /* -------------------------------------------------------------------- */
   //get the band 1 image from the image and make sure that it is GByte image
   poBand = poRDataset->GetRasterBand( 1 );  // band 1
   GDALDataType poBandType = poBand->GetRasterDataType();
   printf ("\tImage Type = %d\n",poBandType);
   if ( poBandType != GDT_Byte )
   {
      printf( "\tError: Image data type is not GDT_Byte: %s.\n", imageFile.c_str() );
      exit ( 1 );
   }

   /* -------------------------------------------------------------------- */
   /*         get image into array                                         */
   /* -------------------------------------------------------------------- */
   if ( (poBand->RasterIO(GF_Read, 0,0,imageInfo.cols,imageInfo.rows,
         poImage,imageInfo.cols,imageInfo.rows,GDT_Byte,0,0)) == CE_Failure)
   {
      printf( "\tError: reading band 1 data from image: %s.\n", imageFile.c_str() );
      CPLFree (poImage);
      exit( 1 );
   }

}


/*******************************************/
/* 56.  compare two gridInfo projections   */
/*******************************************/
bool compareGridProjection  (gridInfo grid1, gridInfo grid2)
{

    OGRSpatialReference   oSRS1, oSRS2;
    string                tmp_str;

    tmp_str = string ( grid1.strProj4 );
    oSRS1.importFromProj4( tmp_str.c_str() );


    tmp_str = string ( grid2.strProj4 );
    oSRS2.importFromProj4( tmp_str.c_str() );

    //make sure that the image has the same projection as the first image's
    if ( oSRS1.IsSame (&oSRS2) )
    {
       printf ( "\n\tSame projection\n\n");
       return true;
    }
    else
    {
       return false;
    }

}


/*********************************************
*       57. writeWRFGridCoordinates_time     *
**********************************************/
void writeWRFGridCoordinates_time ( int ncid, int time_dim, int west_east_dim, int south_north_dim, gridInfo grid )
{
     int   x_id, y_id;
     int   lon_id, lat_id;
     int   fieldtype[1];
     int   sr_xy[1];
     int   dimIndex[NC_MAX_DIMS];     //dimension index array for write out array

    
     fieldtype[0] = 104;
     sr_xy[0] = 1;

     printf( "\nDefine grid coordinate variables in output netcdf file...\n" );

     /******************************
     * Set define mode for output  *
     *******************************/
     anyErrors( nc_redef (ncid) );

     dimIndex[0] = time_dim;
     dimIndex[1] = west_east_dim;
     anyErrors( nc_def_var(ncid, "X_M", NC_FLOAT, 2, dimIndex, &x_id) );
     anyErrors( nc_put_att_int(ncid, x_id, "FieldType", NC_INT, 1, fieldtype) );
     anyErrors( nc_put_att_text(ncid, x_id, "MemoryOorder", 2, "X ") );
     anyErrors( nc_put_att_text(ncid, x_id, "description", 14, "X on mass grid") );
     anyErrors( nc_put_att_text(ncid, x_id, "units", 6, "Meters") );
     anyErrors( nc_put_att_text(ncid, x_id, "stagger", 1, "M") );
     anyErrors( nc_put_att_int(ncid, x_id, "sr_x", NC_INT, 1, sr_xy) );
     anyErrors( nc_put_att_int(ncid, x_id, "sr_y", NC_INT, 1, sr_xy) );

     dimIndex[0] = time_dim;
     dimIndex[1] = south_north_dim;
     anyErrors( nc_def_var(ncid, "Y_M", NC_FLOAT, 2, dimIndex, &y_id) );
     anyErrors( nc_put_att_int(ncid, y_id, "FieldType", NC_INT, 1, fieldtype) );
     anyErrors( nc_put_att_text(ncid, y_id, "MemoryOrder", 2, "Y ") );
     anyErrors( nc_put_att_text(ncid, y_id, "description", 14, "Y on mass grid") );
     anyErrors( nc_put_att_text(ncid, y_id, "units", 6, "Meters") );
     anyErrors( nc_put_att_text(ncid, y_id, "stagger", 1, "M") );
     anyErrors( nc_put_att_int(ncid, y_id, "sr_x", NC_INT, 1, sr_xy) );
     anyErrors( nc_put_att_int(ncid, y_id, "sr_y", NC_INT, 1, sr_xy) );

 
     dimIndex[0] = time_dim;
     dimIndex[1] = south_north_dim;
     dimIndex[2] = west_east_dim;
     anyErrors( nc_def_var(ncid, "XLONG_M", NC_FLOAT, 3, dimIndex, &lon_id) );
     anyErrors( nc_def_var(ncid, "XLAT_M", NC_FLOAT, 3, dimIndex, &lat_id) );

     anyErrors( nc_put_att_int(ncid, lon_id, "FieldType", NC_INT, 1, fieldtype) );
     anyErrors( nc_put_att_text(ncid, lon_id, "MemoryOrder", 3, "XY ") );
     anyErrors( nc_put_att_text(ncid, lon_id, "description", 22, "longitude on mass grid") );
     anyErrors( nc_put_att_text(ncid, lon_id, "units", 17, "degrees longitude") );
     anyErrors( nc_put_att_text(ncid, lon_id, "stagger", 1, "M") );
     anyErrors( nc_put_att_int(ncid, lon_id, "sr_x", NC_INT, 1, sr_xy) );
     anyErrors( nc_put_att_int(ncid, lon_id, "sr_y", NC_INT, 1, sr_xy) );

     anyErrors( nc_put_att_int(ncid, lat_id, "FieldType", NC_INT, 1, fieldtype) );
     anyErrors( nc_put_att_text(ncid, lat_id, "MemoryOrder", 3, "XY ") );
     anyErrors( nc_put_att_text(ncid, lat_id, "description", 21, "latitude on mass grid") );
     anyErrors( nc_put_att_text(ncid, lat_id, "units", 16, "degrees latitude") );
     anyErrors( nc_put_att_text(ncid, lat_id, "stagger", 1, "M") );
     anyErrors( nc_put_att_int(ncid, lat_id, "sr_x", NC_INT, 1, sr_xy) );
     anyErrors( nc_put_att_int(ncid, lat_id, "sr_y", NC_INT, 1, sr_xy) );


     /*******************************
     * Leave define mode for output *
     ********************************/
     anyErrors( nc_enddef (ncid) );

     anyErrors( nc_put_var_float(ncid, x_id, grid.x) );
     printf( "\tWrote X\n" );

     anyErrors( nc_put_var_float(ncid, y_id, grid.y) );
     printf( "\tWrote Y\n" );

     anyErrors( nc_put_var_float(ncid, lon_id, grid.lon) );
     printf( "\tWrote LONG\n" );

     anyErrors( nc_put_var_float(ncid, lat_id, grid.lat) );
     printf( "\tWrote LAT\n" );

}


/***************************************
*       58. readWRFNCVarDataFloat      *
****************************************/
float * readWRFNCVarDataFloat ( gridInfo grid, const char * cropFName, string fileName )
{
    float *cropV;
    int i;


    /******************************/
    /*  read BELD4 file info     */
    /******************************/
    //set NetCDF variables
    int      ncid;
    int      ndims, nvars, ngatts, unlimdimid;
    size_t   *dimSizes, dimSize;
    char     dimName[NC_MAX_NAME+1];
    char     attName[NC_MAX_NAME+1];       //name for an attribute

    //read BELD4 file
    printf("\n\tReading BELD4 netCDF file: %s\n", fileName.c_str() );

    anyErrors( nc_open(fileName.c_str(), NC_NOWRITE, &ncid) );   //open BELD4 file

    printf( "\tObtaining all dimension IDs in input NetCDF file...\n" );
    anyErrors( nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid) );
    printf("\tBELD4 file has: %d dims, %d variables, %d global attributes, %d unlimited variable ID\n", ndims, nvars,ngatts,unlimdimid);

    //store dim size in an arrary
    dimSizes = (size_t *) malloc(sizeof(size_t) * ndims);
    if ( dimSizes == NULL )
    {
       printf ( "\t Memory allocation malloc failed for dimSizes\n" );
       exit ( 1 );
    }

    for (i=0; i<ndims; i++)
    {
       anyErrors(  nc_inq_dim(ncid, i, dimName, &dimSize ) );
       dimSizes[i]= dimSize;
       printf("\tBELD4 file dimension: dimName = %10s   dimSize = %d\n",dimName,dimSize);
    }


    /***********************************
    *   check BELD4 grid information   *
    ***********************************/
    gridInfo      ncGrid;   //BELD4 grid information

    //compare with input grid information
    getGridInfofromNC ( ncid , &ncGrid, "WRFNC" );

    //printGridInfo ( grid );
    printGridInfo ( ncGrid );

    if ( ! sameGrids (grid, ncGrid ) )
    {
       printf ( "\tError: User defined grids and BELD4 netCDF file grids are different.\n" );
       exit ( 1 );
    }


    /**********************************
    * read BELD4 CROPF array output   *
    ***********************************/
    char       var_name[NC_MAX_NAME+1];          //variable name
    int        var_id;                           //variable id
    nc_type    var_type;                         // variable type
    int        var_ndims;                        // number of dims
    int        var_dimids[NC_MAX_VAR_DIMS];      // dimension IDs for read in array
    int        var_dimid;
    int        var_natts;                        // number of attributes
    size_t     varDimSize[NC_MAX_VAR_DIMS];      // dimensions for a variable

    ncVarData  ncData;


    //get var ID
    nc_inq_varid (ncid, cropFName, &var_id);

    //get an input var infor
    anyErrors( nc_inq_var( ncid, var_id, var_name, &var_type, &var_ndims, var_dimids, &var_natts) );

    printf( "\n\tVariable name = %s    var_type = %d    ndims=%d\n",var_name, var_type, var_ndims );
    readIOAPIVar ( &ncData, ncid, var_id, cropFName, var_type, var_ndims, var_dimids, dimSizes, varDimSize);

    cropV = ncData.floatData;

   /********************
   * Close netCDF file *
   ********************/
   anyErrors( nc_close(ncid) );

   return (cropV);

}


/***************************************
*       59. createEPICSiteShapeFile    *
****************************************/
string  createEPICSiteShapeFile (gridInfo grid, std::vector<int> epicSiteIDs )
{

    string shapeName;

    GDALDriver  *poDriver = NULL;
    GDALDataset *poDS = NULL;
    OGRSpatialReference oSRS;
    OGRLayer *poLayer = NULL;
    char szName[33];
    OGRFeature *poFeature = NULL;
    OGRLinearRing  oRing;
    OGRPolygon oPoly;

    double xmin,xmax;
    double ymin,ymax;
    int i, j, k;
    int gid = 0;
    char *pszProj4;


    printf("\nGenerating point shapefile for all EPIC sites...\n");

    //get random file name in the current directoty
    string ext = string ( "_gis.shp" );
    shapeName = getRandomFileName ( ext );

/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
     GDALAllRegister ();

   //check the output shapefile.  If it exists delete it
   deleteShapeFile ( shapeName );

/* -------------------------------------------------------------------- */
/*      Open output shapefile.                                          */
/* -------------------------------------------------------------------- */
    poDriver = GetGDALDriverManager()->GetDriverByName( pszDriverName );
    if( poDriver == NULL )
    {
        printf( "\t%s driver not available.\n", pszDriverName );
        exit( 1 );
    }

    poDS = poDriver->Create( shapeName.c_str(), 0, 0, 0, GDT_Unknown, NULL );
    if( poDS == NULL )
    {
        printf( "\tCreating Shapefile file failed.\n" );
        exit( 1 );
    }
    printf( "\tCreated shapes.\n" );

/* -------------------------------------------------------------------- */
/*      Create output point feature.                                  */
/* -------------------------------------------------------------------- */
    string proj4Str = string(grid.strProj4);
    oSRS.importFromProj4( proj4Str.c_str() );

    poLayer = poDS->CreateLayer( shapeName.c_str(), &oSRS, wkbPoint, NULL );

    if( poLayer == NULL )
    {
        printf( "\tCreating layer failed.\n" );
        exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Create Point Attribute Name.                                  */
/* -------------------------------------------------------------------- */

    OGRFieldDefn oField( grid.polyID.c_str(), OFTInteger);
    printf ("\tCreated item.\n");

    if( poLayer->CreateField( &oField ) != OGRERR_NONE )
    {
        printf( "\tCreating Name field failed.\n" );
        exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Assign each polygon attribute and geometry.                     */
/* -------------------------------------------------------------------- */
    int siteNum = epicSiteIDs.size();

    for (k=0; k<siteNum; k++)
    {
       int  gridID = epicSiteIDs[k];

       //get row and col indices in the model domain from LL corner
       int row = (int) ( floor ( ( gridID - 1 ) / grid.cols) );   //start from 0
       int col = ( gridID - 1 ) % grid.cols;                      //start from 0
       //printf ("\t\tk= %d  gridID=%d    row=%d   col=%d\n",k,gridID,row,col);


       poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
       poFeature->SetField( grid.polyID.c_str(), gridID );

       float x = grid.x[col]; 
       float y = grid.y[row];

       OGRPoint pt;
        
       pt.setX( x );
       pt.setY( y );
 

       if ( poFeature->SetGeometry( &pt ) != OGRERR_NONE )
       {
           printf( "\tSetting a point feature failed.\n" );
           exit( 1 );
       }

       if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE )
       {
            printf( "\tFailed to create feature in shapefile.\n" );
            exit( 1 );
       }

        OGRFeature::DestroyFeature( poFeature );
    } //sites

    GDALClose( poDS );

    printf ( "\tCompleted creating EPIC site shapefile:%s\n\n", shapeName.c_str() );
    
    return shapeName;

}


/**********************************
*       60. identity2SHPFiles    *
***********************************/
string  identity2SHPFiles (string shpFile1, string shpFile2 )
{

    string       gdalBinDir;           //GDAL directory
    string       shapeName;

    GDALDriver  *poDriver = NULL;
    GDALDataset *poDS = NULL, *poDS1 = NULL,*poDS2 = NULL;
    OGRSpatialReference *oSRS1, *oSRS2;
    OGRLayer *poLayer = NULL,*poLayer1 = NULL, *poLayer2 = NULL;

    int i, j, k;



    printf("\nIntersecting %s with %s...\n", shpFile1.c_str() , shpFile2.c_str() );

  
/*-------------------------------------------------------------*/
/*   get GDALBIN environment variable defined in .cshrc file   */
/*-------------------------------------------------------------*/
    gdalBinDir =   string ( getEnviVariable("GDALBIN") );
    gdalBinDir =  processDirName( gdalBinDir );
    printf("GDAL bin directory: %s\n", gdalBinDir.c_str() );
    FileExists(gdalBinDir.c_str(), 0 );  //the dir has to exist


/*----------------------------------------------------------*/
/*   get random file name in the current directoty          */
/*----------------------------------------------------------*/
    string ext = string ( "_gis.shp" );
    shapeName = getRandomFileName ( ext );


/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
    GDALAllRegister ();

   //check the output shapefile.  If it exists delete it
   deleteShapeFile ( shapeName );


/* -------------------------------------------------------------------- */
/*      Open input shapefiles.                                          */
/* -------------------------------------------------------------------- */
    poDriver = GetGDALDriverManager()->GetDriverByName( pszDriverName );
    if( poDriver == NULL )
    {
        printf( "\t%s driver not available.\n", pszDriverName );
        exit( 1 );
    }

    poDS1 = (GDALDataset*) GDALOpenEx( shpFile1.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL );
    if( poDS1 == NULL )
    {
        printf( "\tOpening Shapefile file failed: %s\n", shpFile1.c_str() );
        exit( 1 );
    }

    poDS2 = (GDALDataset*) GDALOpenEx( shpFile2.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL );
    if( poDS2 == NULL )
    {
        printf( "\tOpening Shapefile file failed: %s\n", shpFile2.c_str() );
        exit( 1 );
    }


/* -------------------------------------------------------------------- */
/*      get layers, types, and projection                               */
/* -------------------------------------------------------------------- */

    poLayer1 = poDS1->GetLayer( 0 );

    if( poLayer1 == NULL )
    {
        printf( "\tCreating layer failed: %s\n", shpFile1.c_str() );
        exit( 1 );
    }

    poLayer2 = poDS2->GetLayer( 0 );

    if( poLayer2 == NULL )
    {
        printf( "\tCreating layer failed: %s\n", shpFile2.c_str() );
        exit( 1 );
    }


    //types
    OGRwkbGeometryType  shp1Type, shp2Type;
    shp1Type = poLayer1->GetGeomType ( );
    shp2Type = poLayer2->GetGeomType ( ); 

    if ( shp2Type != wkbPolygon )
    {
       printf( "\tSecond shapefile has to be polygon shapes: %s\n", shpFile2.c_str() );
       exit( 1 );
    }


    //projections
    char    *pszProj4_1, *pszProj4_2;

    oSRS1 = poLayer1->GetSpatialRef ( );
    oSRS2 = poLayer2->GetSpatialRef ( ); 

    oSRS1->exportToProj4( &pszProj4_1 ); 
    printf( "\tFile1 proj4=%s   Type1=%d\n",pszProj4_1, shp1Type);

    oSRS2->exportToProj4( &pszProj4_2 );       
    printf( "\tFile2 proj4=%s   Type2=%d\n",pszProj4_2, shp2Type );


    //check the projections

    string  shp2Type_prj;   //new projected shapefile 2
    if ( !oSRS1->IsSame ( oSRS2 ) )
    {
       printf ( "\tSecond shapefile has different ptojection and reprojecting it...\n" );
       

       //close the second file
        GDALClose ( poDS2 );

        string shpFile2_prj  =  projectShape ( gdalBinDir, shpFile2, pszProj4_1 );      

        shpFile2 = shpFile2_prj;
   
        //reopen the created shapefile
        poDS2 = (GDALDataset*) GDALOpenEx( shpFile2.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL );
        if( poDS2 == NULL )
        {
           printf( "\tOpening Shapefile file failed: %s\n", shpFile2.c_str() );
           exit( 1 );
        }
             
        poLayer2 = poDS2->GetLayer( 0 );

        if( poLayer2 == NULL )
        {
           printf( "\tCreating layer failed: %s\n", shpFile2.c_str() );
           exit( 1 );
        }
    }  //end of projection


/* ----------------------------------------------------------- */
/*      Create output shapefile                                */
/* ----------------------------------------------------------- */
    poDS = poDriver->Create( shapeName.c_str(), 0, 0, 0, GDT_Unknown, NULL );

    if( poDS == NULL )
    {  
        printf( "\tCreating shapefile file failed.\n" );
        exit( 1 );
    }  

    //create output features same as shpFile1
    poLayer = poDS->CreateLayer( shapeName.c_str(), oSRS1, shp1Type, NULL );
       
    if( poLayer == NULL )
    {       
        printf( "\tCreating layer failed.\n" );
        exit( 1 );
    }   


/* ------------------------------------------------------ */
/*      Identity two shapefiles                           */
/* ------------------------------------------------------ */
/*    if ( poLayer1->Identity( poLayer2, poLayer, NULL, NULL, NULL ) != OGRERR_NONE )
    { 
        printf( "\tIdentitying the shapfiles failed.\n" );
        exit( 1 );
    }
*/

    GDALClose ( poDS );
    GDALClose ( poDS1 );
    GDALClose ( poDS2 );

    printf ( "\tCompleted creating EPIC site shapefile:%s\n\n", shapeName.c_str() );
    
    return shapeName;

}


/**************************************
 *       61. getPointsInPolyItems      *
 **************************************/
void getPointsInPolyItems (gridInfo grid,  std::vector<int> epicSiteIDs, std::map<int, vector<string> > &gridEPICData, string polySHP, std::vector<string> shpItems )
{

    string       gdalBinDir;           //GDAL directory

    GDALDriver  *poDriver = NULL;
    GDALDataset *poDS = NULL;

    OGRSpatialReference *oSRS1, *oSRS2;
    OGRLayer *poLayer = NULL;

    int i, j, k;



    printf("\nExtracting polygon shapefile attributes for EPIC sites: %s...\n", polySHP.c_str() );

  
/*-------------------------------------------------------------*/
/*   get GDALBIN environment variable defined in .cshrc file   */
/*-------------------------------------------------------------*/
    gdalBinDir =   string ( getEnviVariable("GDALBIN") );
    gdalBinDir =  processDirName( gdalBinDir );
    printf("\tGDAL bin directory: %s\n", gdalBinDir.c_str() );
    FileExists(gdalBinDir.c_str(), 0 );  //the dir has to exist


/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
    GDALAllRegister ();


/* -------------------------------------------------------------------- */
/*      Open input shapefiles.                                          */
/* -------------------------------------------------------------------- */
    poDriver = GetGDALDriverManager()->GetDriverByName( pszDriverName );
    if( poDriver == NULL )
    {
        printf( "\t%s driver not available.\n", pszDriverName );
        exit( 1 );
    }


    poDS = (GDALDataset*) GDALOpenEx( polySHP.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL );
    if( poDS == NULL )
    {
        printf( "\tOpening Shapefile file failed: %s\n", polySHP.c_str() );
        exit( 1 );
    }


/* -------------------------------------------------------------------- */
/*      get layers, types, and projection                               */
/* -------------------------------------------------------------------- */

    poLayer = poDS->GetLayer( 0 );

    if( poLayer == NULL )
    {
        printf( "\tCreating layer failed: %s\n", polySHP.c_str() );
        exit( 1 );
    }


    //types
    OGRwkbGeometryType  shpType;

    shpType = poLayer->GetGeomType ( );

    if ( shpType != wkbPolygon )
    {
       printf( "\tShapefile has to be polygon shapes: %s\n", polySHP.c_str() );
       exit( 1 );
    }


    //projections
    const char *pszWKT = NULL;
    char       *pszWKT_nc = NULL;
    char       *pszProj4_1, *pszProj4_2;

    projPJ      proj4To, proj4From;
    bool        projectSite;
    

    oSRS2 = poLayer->GetSpatialRef ( );      //polygon shapefile projection
    oSRS2->exportToProj4( &pszProj4_2 );       

    printf( "\tPolygon Shapefile projection: %s\n",pszProj4_2 );
    printf( "\tGrid domain projection: %s\n",grid.strProj4);

    //check the projections
    string  prjDomain = string ( grid.strProj4 );
    string  prjSHP = string ( pszProj4_2 );


    proj4From = pj_init_plus( prjDomain.c_str() );
    if ( proj4From == NULL )
    {
       printf( "\tInitializing domain Proj4 projection failed: %s.\n",  prjDomain.c_str() );
       exit( 1 );
    }

    proj4To = pj_init_plus( prjSHP.c_str() );
    if ( proj4To == NULL )
    {
       printf( "\tInitializing shapefile Proj4 projection failed: %s.\n",  prjSHP.c_str() );
       exit( 1 );
    }


    /* old way handling different projection
    if (! comparePROJ4GDALPRJ (prjDomain, prjSHP ) )
    {
       printf ( "\tPolygon shapefile has different projection and reprojecting it...\n" );
       
       //close the second file
       GDALClose ( poDS );

       string polySHP_prj  =  projectShape ( gdalBinDir, polySHP, grid.strProj4 );      

       polySHP = polySHP_prj;
   
       //reopen the created shapefile
       poDS = poDriver->Open ( polySHP.c_str(), FALSE );
       if( poDS == NULL )
       {
          printf( "\tOpening Shapefile file failed: %s\n", polySHP.c_str() );
          exit( 1 );
       }
             
       poLayer = poDS->GetLayer( 0 );

       if( poLayer == NULL )
       {
          printf( "\tCreating layer failed: %s\n", polySHP.c_str() );
          exit( 1 );
       }
    }  //end of projection
    */


    //new way to handle the projection -- project site instead of the shapefile

    projectSite = false;  //no projection

    if (! comparePROJ4GDALPRJ (prjDomain, prjSHP ) )
    {
       projectSite = true; 
       printf ("\tEPIC site x and y will be projected into the polygon shapefile to get attributes\n");
    }   
   

    OGRGeometry * layerGeom = poLayer->GetSpatialFilter( ); 

/* -------------------------------------- */
/*      Loop through EPIC sites           */
/* -------------------------------------- */
    for ( i=0; i<epicSiteIDs.size(); i++ )
    {
        bool ptInPoly = false;

        int gridID = epicSiteIDs[i]; 
        
        //get epic info vector
        std::vector<string>   vecItems = gridEPICData[gridID];


        //if got item values from USA shapefile, do not get values from NA shapefile
        //shpItems.size() == 5 for USA shapefile - processed first
        //shpItems.size() == 2 for NA shapefile -  processed second
        //shpItems.size() == 1 for HUC8 shapefile - processed third
        //vecItems contains: GRASS, CROP, TOTAL, WATER, REG10, STATE, COUNTY, COUNTRY, STATEABB, HUC8, ELEVATION,SLOPE

        if ( vecItems.size() == 9 &&  shpItems.size() == 2 )
        {
           vecItems.clear();             
           continue;
        }
         

        //get row and col indices in the model domain from LL corner
        int row = (int) ( floor ( ( gridID - 1 ) / grid.cols) );   //start from 0
        int col = ( gridID - 1 ) % grid.cols;                      //start from 0

        float x = grid.x[col];
        float y = grid.y[row];        

 
        if ( projectSite )
        {
           projUV    xyP;

           xyP = projectPoint ( proj4From, proj4To, x, y );

           if ( xyP.u == HUGE_VAL )
           {
              printf ("\tError: HUGE_VAL from projecting EPIC site %d to the polygon shapefile.\n", gridID);
              exit ( 1 );
           }

           //printf ("gridid = %d  row=%d  col=%d   x=%f  y=%f   projx=%f  projy=%f\n", gridID, row, col, x, y, xyP.u, xyP.v);

           x = xyP.u;
           y = xyP.v;
        }
        else
        {
          // printf ("gridid = %d  row=%d  col=%d   x=%f  y=%f\n", gridID, row, col, x, y);
        }



        OGRPoint pt;
        pt.setX( x );
        pt.setY( y );              
         
        OGRGeometry *ptGeometry = pt.clone ();        

        //set sptail fiter
        poLayer->SetSpatialFilter( ptGeometry );  //for getting features intersect with the point
        
        OGRFeature *poFeature;
        poLayer->ResetReading();

/* ------------------------------------------------------ */
/*      Loop through polygons in shapefile                 */
/* ------------------------------------------------------ */
        while( (poFeature = poLayer->GetNextFeature()) != NULL )
        {
           OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
           int iField;

           OGRGeometry *poGeometry;

           poGeometry = poFeature->GetGeometryRef();
           if( poGeometry != NULL )
           {

              //set filter
                                

              ptInPoly = ptGeometry->Within ( poGeometry );

              //printf ("\tbool ptInPoly=%d\n", ptInPoly);
            
              if ( ptInPoly )
              {  
                 //find the polygon where point is within and get item values            
                 //printf ("\ti= %d  gridID=%d    row=%d   col=%d x=%f   y=%f\n",i,gridID,row,col,x,y);

                 for ( j=0; j<shpItems.size(); j++ )
                 {
                     int fieldIndex;
                     if ( ( fieldIndex = poFeature->GetFieldIndex( shpItems[j].c_str() ) ) == -1 )
                     {
                         printf ( "\tPolygon shapefile - %s does not have item: %s\n", polySHP.c_str(), shpItems[j].c_str() );
                         exit ( 1 );
                     }
                     string itemStr = string ( poFeature->GetFieldAsString(fieldIndex) ); 

                     vecItems.push_back ( itemStr );

                     //printf ( "\tItemName = %s  itemValue = %s\n", shpItems[j].c_str(), itemStr.c_str() );
                                 
                 } //number of item values to get

                 
                 poGeometry->empty();
                 OGRFeature::DestroyFeature( poFeature );
                 break;

              }  //point in polygon

           }   //got polygon geometry

           poGeometry->empty();
           OGRFeature::DestroyFeature( poFeature ); 
         
        }  //loop through shapefile polygons
     
        
        //no polygon is found and set value to "0"
        if ( ! ptInPoly )
        {
           int fillNum = shpItems.size();

           //for USA shapefile, only fill REG10, STATE, COUNTY.  COUNTRY and STATEABB will be from NA shapefile
           if ( shpItems.size() == 5 )
           {
               fillNum = 3; 
           }

           for ( j=0; j<fillNum; j++ )
           {
              string tmp_str = string ( "0" );
              vecItems.push_back ( tmp_str );
           }
        }

        gridEPICData[gridID] = vecItems;   //update EPIC site values

        pt.empty(); 
        ptGeometry->empty();
        ptGeometry->empty();

        vecItems.clear();

    }  //i site



    GDALClose ( poDS );

    printf ( "\tCompleted extracting attributes from shapefile:%s\n\n", polySHP.c_str() );

}


/**************************************
 *       62. getPointsInRasterValues
 **************************************/
void getPointsInRasterValues (gridInfo grid,  std::vector<int> epicSiteIDs, std::map<int, vector<string> > &gridEPICData, string imageFile, string outImageValue_str )
{
    int                   i;
    int                   pixelIndex;
    GDALDataset           *poRDataset;
    GDALRasterBand        *poBand;
    char                  tmp_chars[25];
    string                valueStr;
    string                gdalBinDir;           //GDAL directory

    
    printf( "\tObtaining point values from image file: %s\n", imageFile.c_str() );


    /*-------------------------------------------------------------*/
    /*   get GDALBIN environment variable defined in .cshrc file   */
    /*-------------------------------------------------------------*/
    gdalBinDir =   string ( getEnviVariable("GDALBIN") );
    gdalBinDir =  processDirName( gdalBinDir );
    printf("\tGDAL bin directory: %s\n", gdalBinDir.c_str() );
    FileExists(gdalBinDir.c_str(), 0 );  //the dir has to exist


    //get image info
    gridInfo imageInfo = getImageInfo ( imageFile );

    printf( "\tGrid domain projection: %s\n",grid.strProj4);
    printf( "\tImage projection: %s\n", imageInfo.strProj4);

    //check the projections
    string  prjDomain = string ( grid.strProj4 );
    string  prjImage = string ( imageInfo.strProj4 );

    string imageFile_prj;
    if (! comparePROJ4GDALPRJ (prjDomain, prjImage ) )
    {
       printf ( "\tImage file has different projection and reprojecting it...\n" );

       imageFile_prj = projectImage (gdalBinDir, imageFile, imageInfo, grid );

       imageFile = imageFile_prj;
       imageInfo = getImageInfo ( imageFile );

    }  //end of proje


    /* ---------------------- */
    /*      Open image        */
    /* ---------------------- */
    poRDataset = (GDALDataset *) GDALOpen( imageFile.c_str(), GA_ReadOnly );
    if( poRDataset == NULL )
    {
       printf( "\tError: Open raster file failed: %s.\n", imageFile.c_str() );
       exit( 1 );
    }

    //image information
    printGridInfo ( imageInfo );

    /* ---------------------------------------- */
    /*    Image data type has to be Int16       */
    /* ---------------------------------------- */
    //get the band 1 image from the image and make sure that it is GByte image
    poBand = poRDataset->GetRasterBand( 1 );  // band 1
    GDALDataType poBandType = poBand->GetRasterDataType();
    printf ("\tImage Type = %d\n",poBandType);
    if ( poBandType != GDT_Int16 )
    {
       printf( "\tError: Image data type is not GDT_Int16 : %s.\n", imageFile.c_str() );
       exit ( 1 );
    }

    /* -------------------------------------------------------------------- */
    /*         get image into array                                         */
    /* -------------------------------------------------------------------- */
    GInt16 *poImage = (GInt16 *) CPLCalloc(sizeof(GInt16),imageInfo.cols*imageInfo.rows);
    if ( (poBand->RasterIO(GF_Read, 0,0,imageInfo.cols,imageInfo.rows,
         poImage,imageInfo.cols,imageInfo.rows,GDT_Int16,0,0)) == CE_Failure)
    {
       printf( "\tError: reading band 1 data from image: %s\n", imageFile.c_str() );
       CPLFree (poImage);
       exit( 1 );
    }
     

    /* -------------------------------------- */
    /*      Loop through EPIC sites           */
    /* -------------------------------------- */
    for ( i=0; i<epicSiteIDs.size(); i++ )
    {                
        bool ptInImage = false;
                         
        int gridID = epicSiteIDs[i];
                     
        //get epic info vector
        std::vector<string>   vecItems = gridEPICData[gridID];

        //get row and col indices in the model domain from LL corner
        int row = (int) ( floor ( ( gridID - 1 ) / grid.cols) );   //start from 0
        int col = ( gridID - 1 ) % grid.cols;                      //start from 0

        float x = grid.x[col];
        float y = grid.y[row];
       
        //get row and col indices in the image from UL corner
        row = (int) (floor ((imageInfo.ymax - y) / imageInfo.yCellSize));   //start from 0
        col = (int) (floor ((x - imageInfo.xmin) / imageInfo.xCellSize));   //start from 0    
     
        if ( (row >= 0 && row < imageInfo.rows) && (col >=0 && col < imageInfo.cols) )
        {
           ptInImage = true;
           int pixelIndex = imageInfo.cols * row + col;
           GInt16 pixelValue = poImage[pixelIndex];   
           sprintf(tmp_chars, "%d\0", pixelValue ); 
           valueStr = string ( tmp_chars );
           vecItems.push_back ( valueStr );
        }

        
        //if site is not in image
        if ( ! ptInImage )
        {
           vecItems.push_back ( outImageValue_str );
        }

        gridEPICData[gridID] = vecItems;   //update EPIC site values

        vecItems.clear();
    }

    CPLFree (poImage);
    GDALClose( (GDALDatasetH) poRDataset );

    //delete projected image file
    if (! imageFile_prj.empty() ) 
    {
       deleteRasterFile ( imageFile_prj );
    }

    printf( "\n\tCompleted value extraction from the image.\n\n" );

}



/***************************************
*       63. getNetCDFDim               *
****************************************/
int getNetCDFDim ( char * dimName, string fileName )
{
   int dim_Size;


   /*****************************/
   /*  read netcdf  file info    */
   /*****************************/
   //set NetCDF variables
   int      ncid, dimID;
   size_t   dimSize;


   //read nc file
   printf("\n\tGet dimention size of %s from file: %s\n", dimName, fileName.c_str() );

   anyErrors ( nc_open(fileName.c_str(), NC_NOWRITE, &ncid) );   //open NC file

   anyErrors ( nc_inq_dimid(ncid, dimName, &dimID) );

   anyErrors ( nc_inq_dimlen(ncid, dimID, &dimSize) );   

   dim_Size = dimSize;

        
   /********************
   * Close netCDF file *
   ********************/
   anyErrors( nc_close(ncid) );

  
   printf ("\tDimension of %s is: %d\n", dimName, dim_Size);

   return dim_Size;

}



/********************************************************/
/*  64.  Extract MODIS LAI data out for a domain area   */
/********************************************************/
string   extractDomainLAIData ( std::vector<string> modisFiles, std::vector<string> satVars, gridInfo grid)
{

   const char     *pszFormat = "EHdr";
   GDALDataset    *poDstDS;       
   OGRSpatialReference oSRS;
   char           *pszWKT = NULL;

   GDALDriver     *poDriver;
   GDALRasterBand *poBand[satVars.size()];
   char           **papszOptions = NULL;
   char           **papszMetadata;
   GByte          *poGrid[satVars.size()];

   int            i,j,k,n,m;


   printf("\nExtracting MODIS LAI data for a given domain grid...\n");

   //get random file name in the current directoty
   string ext = string ( "_modis.bil" );
   string imageName = getRandomFileName ( ext );


   /* -------------------------------------------------------------------- */
   /*      Register format(s).                                             */
   /* -------------------------------------------------------------------- */
   GDALAllRegister();

   poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);

   if( poDriver == NULL )
   {
      printf ( "\tCreating a GDAL format driver failed: %s\n", pszFormat);
      exit( 1 );
   }

  
   //extract satVars.size() variables to put them in these many bands
   poDstDS = poDriver->Create( imageName.c_str(), grid.cols, grid.rows, satVars.size(), GDT_Byte, papszOptions );
   double adfGeoTransform[6] = { grid.xmin, grid.xCellSize, 0, grid.ymax, 0, -1*grid.yCellSize };

   poDstDS->SetGeoTransform( adfGeoTransform );
    
   string strProj4_cc = string ( grid.strProj4 );

   oSRS.importFromProj4( strProj4_cc.c_str() );
   oSRS.exportToWkt( &pszWKT );

   if ( (poDstDS->SetProjection( pszWKT )) == CE_Failure )
   {
      printf ( "\tSetting projection failed for the grid raster file: %s\n", imageName.c_str() );
      exit ( 1 );
   }

   //set bands and allocate memory for each variable
   for ( j=0; j< satVars.size(); j++ )
   {

      poBand[j] = poDstDS->GetRasterBand(j+1);

      k = grid.rows*grid.cols;

      poGrid[j] = (GByte *) CPLCalloc(sizeof(GByte),k);

      //initialize MODIS array to be unclassified
      for ( i=0; i<k; i++)
      {
         poGrid[j][i] = MODIS_LAI_FillValue;  //set to 255
      }
   } 
   

   //process each satellite file
   //get data from each MODIS tile
   for (n=0; n<modisFiles.size(); n++)
   {
      //get MODIS file grid info
      string imageFile = modisFiles.at(n);

      gridInfo modisInfo = getHDF4VarInfo (imageFile, satVars[0] );

      if ( modisInfo.xmin >= grid.xmax || modisInfo.xmax <= grid.xmin ||
            modisInfo.ymin >= grid.ymax || modisInfo.ymax <= grid.ymin )
      {
          printf ("\n\tMODIS LAI-FPAR file does not intersect with the domain: %s\n", imageFile.c_str() );
      } 
      else
      { 
          printf ("\n\tMODIS LAI-FAPR file intersects with the domain: %s\n", imageFile.c_str() );

          //read satellite variables into arrays
          double *poImage[satVars.size()];

          for ( m=0; m<satVars.size(); m++ )
          {

             //get data from the MODIS file
             poImage[m] = (double *) CPLCalloc(sizeof(double), modisInfo.rows*modisInfo.cols);

             readHDF4SatVarData (imageFile, satVars[m], poImage[m]);   
          }

          double x,y;
          int  col,row;
          for( i = 0; i<modisInfo.rows; i++ )
          {
            
             //get cell center point y and compute row in grid image
             y = modisInfo.ymax - i * modisInfo.yCellSize - modisInfo.yCellSize / 2.0;
             row = (int) (floor ((grid.ymax - y) / grid.yCellSize));   //start from 0

             if ( row >= 0 && row < grid.rows )
             {
                for (j=0; j<modisInfo.cols; j++)
                {
          
                   //get cell center point x and compute col in grid image
                   x = modisInfo.xmin + j * modisInfo.xCellSize + modisInfo.xCellSize / 2.0;
                   col = (int) (floor ((x - grid.xmin) / grid.xCellSize));  //start from 0

                   if ( col >=0 && col < grid.cols )
                   {
                      int gridIndex = grid.cols * row + col;
                      int imageIndex = modisInfo.cols * i + j;

                      //printf ( "\tcol=%d  row=%d  gridIndex=%d\n",col,row,gridIndex );
                      
                      for ( m=0; m<satVars.size(); m++ )
                      {
                         GByte  modisClass =  int ( poImage[m][imageIndex] );

                         poGrid[m][gridIndex] = int ( modisClass );  //copy image data to grid

                      }  //number satellite variables

                   }  // check col
                } //j

             }  // check row
          }  //i

         
          printf ( "\tFinished reading: %s\n\n", imageFile.c_str() );
      
          for ( m=0; m<satVars.size(); m++ )
          {
             CPLFree (poImage[m]);
          }

      }  //intersect

   }  //n MODIS file



   //write bands
   for ( j=0; j< satVars.size(); j++ )
   {         
             
      if ( (poBand[j]->RasterIO( GF_Write, 0, 0, grid.cols, grid.rows, poGrid[j], 
            grid.cols, grid.rows, GDT_Byte, 0, 0 ) ) == CE_Failure)  
      {
            printf( "\tError in writing variable %s data for image: %s.\n", satVars[j].c_str(), imageName.c_str() );
            CPLFree (poGrid[j]);
            exit( 1 );
      }
      else
      {
         CPLFree (poGrid[j]);
      }
   }


   /* Once we're done, close properly the dataset */
   GDALClose( (GDALDatasetH) poDstDS );

   printf ( "\tCreated grid raster LAI file: %s\n\n", imageName.c_str() );

   return ( imageName );

}



/********************************************************/
/*  65.  Extract MODIS Albed0 data out for a domain area   */
/********************************************************/
string   extractDomainALBData ( std::vector<string> modisFiles, std::vector<string> satVars, gridInfo grid)
{

   const char     *pszFormat = "EHdr";
   GDALDataset    *poDstDS;       
   OGRSpatialReference oSRS;
   char           *pszWKT = NULL;

   GDALDriver     *poDriver;
   char           **papszOptions = NULL;
   char           **papszMetadata;

   int            i,j,k,n,m,numP;


   printf("\nExtracting MODIS LAI data for a given domain grid...\n");

   //get random file name in the current directoty
   string ext = string ( "_modis.bil" );
   string imageName = getRandomFileName ( ext );


   //set number of bands

   int numParams = 1;   //for two dimensional variable - MCD43A2, MCD43A3 
   if ( satVars[0].find ( "_Albedo_Parameters_" ) != string::npos )
   {
      //for MCD43A1
      numParams = 3;  //for MCD43A1 - 3 dimensional variable
   }
                                                                           

   GDALRasterBand  *poBand[satVars.size()*numParams];
   GInt16          *poGrid[satVars.size()*numParams];


   /* -------------------------------------------------------------------- */
   /*      Register format(s).                                             */
   /* -------------------------------------------------------------------- */
   GDALAllRegister();

   poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);

   if( poDriver == NULL )
   {
      printf ( "\tCreating a GDAL format driver failed: %s\n", pszFormat);
      exit( 1 );
   }

  
   //extract satVars.size() variables to put them in these many bands
   poDstDS = poDriver->Create( imageName.c_str(), grid.cols, grid.rows, satVars.size()*numParams, GDT_Int16, papszOptions );
   double adfGeoTransform[6] = { grid.xmin, grid.xCellSize, 0, grid.ymax, 0, -1*grid.yCellSize };

   poDstDS->SetGeoTransform( adfGeoTransform );
    
   string strProj4_cc = string ( grid.strProj4 );

   oSRS.importFromProj4( strProj4_cc.c_str() );
   oSRS.exportToWkt( &pszWKT );

   if ( (poDstDS->SetProjection( pszWKT )) == CE_Failure )
   {
      printf ( "\tSetting projection failed for the grid raster file: %s\n", imageName.c_str() );
      exit ( 1 );
   }


   int numBand = 0;
   for ( j=0; j<satVars.size(); j++ )
   {
      for (numP=0; numP<numParams; numP++)
      {
          poBand[numBand] = poDstDS->GetRasterBand(numBand+1);

          k = grid.rows * grid.cols;   

          poGrid[numBand] = (GInt16 *) CPLCalloc(sizeof(GInt16),k);

          //initialize MODIS array to be unclassified
          for ( i=0; i<k; i++)
          {
             poGrid[numBand][i] = MODIS_ALB_FillValue;  //set to 32767
          }
          
          numBand++;       
         
      }  //numP
   }  //j
   

   printf ("\tTotal Bands to store albedo variable: %d\n", numBand );


   //process each satellite file
   //get data from each MODIS tile
   for (n=0; n<modisFiles.size(); n++)
   {
      //get MODIS file grid info
      string imageFile = modisFiles.at(n);

      printf ("\nProcess MODIS file %d: %s\n", n, imageFile.c_str() );

      gridInfo modisInfo = getHDF4VarInfo (imageFile, satVars[0] );

      if ( modisInfo.xmin >= grid.xmax || modisInfo.xmax <= grid.xmin ||
            modisInfo.ymin >= grid.ymax || modisInfo.ymax <= grid.ymin )
      {
          printf ("\tMODIS file does not intersect with the domain\n" );
      } 
      else
      { 
          printf ("\n\tMODIS file intersects with the domain: %s\n", imageFile.c_str() );

          //read satellite variables into arrays
          double *poImage[satVars.size()];

          for ( m=0; m<satVars.size(); m++ )
          {

             //get data from the MODIS file
             poImage[m] = (double *) CPLCalloc(sizeof(double), modisInfo.rows*modisInfo.cols*numParams);

             readHDF4SatVarData (imageFile, satVars[m], poImage[m]);   
          }


          double x,y;
          int  col,row;
          for( i = 0; i<modisInfo.rows; i++ )
          {
            
             //get cell center point y and compute row in grid image
             y = modisInfo.ymax - i * modisInfo.yCellSize - modisInfo.yCellSize / 2.0;
             row = (int) (floor ((grid.ymax - y) / grid.yCellSize));   //start from 0

             if ( row >= 0 && row < grid.rows )
             {
                for (j=0; j<modisInfo.cols; j++)
                {
          
                   //get cell center point x and compute col in grid image
                   x = modisInfo.xmin + j * modisInfo.xCellSize + modisInfo.xCellSize / 2.0;
                   col = (int) (floor ((x - grid.xmin) / grid.xCellSize));  //start from 0

                   if ( col >=0 && col < grid.cols )
                   {

                      numBand =0; 
                      for ( m=0; m<satVars.size(); m++ )
                      {

                         int gridIndex = row * grid.cols + col;  //each band is 2-d

                         //for MCD43A1, MCD43A2 or MCD43A3 variables
                         for ( k=0; k<numParams; k++ )
                         { 
                   
                            int imageIndex = i * modisInfo.cols * numParams + j * numParams + k;

                            //printf ( "\tcol=%d  row=%d  k=%d  gridIndex=%d\n",col,row,k,gridIndex );

                            GInt16  modisClass =  int ( poImage[m][imageIndex] );

                            poGrid[numBand][gridIndex] = modisClass;  //copy image data to grid band
                          
                            numBand ++;

                         }  //k - numParams

                      } //m -  number satellite variables
                   }  // check col
                } //j

             }  // check row
          }  //i

         
          printf ( "\tFinished reading: %s\n\n", imageFile.c_str() );
      
          for ( m=0; m<satVars.size(); m++ )
          {
             CPLFree (poImage[m]);
          }

      }  //intersect

   }  //n MODIS file

   printf ( "\tFinished reading all files\n\n" );


   //write bands
   
   numBand = 0;
   for ( j=0; j< satVars.size(); j++ )
   {         

      for ( k=0; k<numParams; k++ )
      {
             
         if ( (poBand[numBand]->RasterIO( GF_Write, 0, 0, grid.cols, grid.rows, poGrid[numBand], 
               grid.cols, grid.rows, GDT_Int16, 0, 0 ) ) == CE_Failure)  
         {
               printf( "\tError in writing variable %s data parameter %d for image: %s.\n", satVars[j].c_str(), numBand+1, imageName.c_str() );
               CPLFree (poGrid[j]);
               exit( 1 );
         }
         else
         {
            CPLFree (poGrid[numBand]);
         }

         numBand ++;
         //printf ( "\tWrote band %d\n", numBand );

      }  //k

   }  //j



   /* Once we're done, close properly the dataset */
   GDALClose( (GDALDatasetH) poDstDS );

   printf ( "\tCreated grid raster LAI file: %s\n\n", imageName.c_str() );

   return ( imageName );

}



