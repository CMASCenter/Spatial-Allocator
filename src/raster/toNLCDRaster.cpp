/********************************************************************
 * This program is used: 
 *  1. to project a polygon shapefile into NLCD NAD83 projection
 *     IMPORTANT NOTE: When project a shapefile with sphere R to WGS84, 
 *                     there is no datum transformation here from PROJ4.6. 
 *                     Since WRF internal spatail data (USGS GLCC
 *                     , soil, and elevation) are in WGS84 datum, datum transformation 
 *                     will be performed from WGS84 to NAD83.  But, there is no   
 *                     datum transformation from Sphere R to WGS84 for 
 *                     matching WRF internal spatial data.
 *  2. to rasterize the polygon into 30 meter NLCD grids using GDAL library.
 *  3. to project and clip MODIS 1km image into the modeling domain
 *
 * Written by the Institute for the Environment at UNC, Chapel Hill
 * in support of the EPA NOAA CMAS Modeling, 2007-2008.
 * 
 * By Limei Ran, Feb, 2008
 *
 * Usage: 1. toNLCDRaster.exe in_shapeFile polyID out_rasterFile processedNLCDDir in_modisFile out_modisFile
 *        For instance:
 *        toNLCDRaster.exe  wrf12km.shp GRIDID wrf12km_30m.bil /nas/uncch/depts/cep/emc/lran/nlcd2001/nlcd/new/ /nas/uncch/depts/cep/emc/lran/nlcd2001/modis/gl_sin.bsq /nas/uncch/depts/cep/emc/lran/nlcd2001/modis/wrf12km_modis.bsq 
 *
 *        Environment Variables needed: 
 *        GDALBIN -- GDAL bin directory
 *
 *        or
 *        2. toNLCDRaster.exe
 *    
 *        Environment Variables needed without arguments:
 *        GRID_SHAPEFILE_NAME -- grid polygon shapefile name
 *        POLYGON_ID -- item used to rasterize the shapefile
 *        GRID_RASTERFILE_NAME -- grid rasterized image file name
 *        DATADIR -- directory of preprocessed image data: USGS and NOAA NLCD      
 *        INPUT_MODISFILE -- Name of original MODIS file 
 *        OUTPUT_MODISFILE -- Name of projected and clipped MODIS file for modeling domain
 *        GDALBIN -- GDAL bin directory
 *     
 *        The program will create temp_grdshape_nlcd.shp which projected grid polygon shapefile in NLCD
 *        projection and it will be deleted at the end of the program.  
 *
********************************************************************/
#include <dirent.h>
#include "ogrsf_frmts.h"
#include "ogr_geometry.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "ogr_api.h"
#include "gdal_priv.h"

#include "sa_raster.h"
#include "commontools.h"

static void Usage();

/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main( int nArgc,  char* papszArgv[] )
{
    struct stat stFileInfo;
    const int   argsN = 6;  //number of input arguments required
    const char  *pszSrcFilename = NULL;
    const char  *polyID = NULL;
    const char  *pszDstFilename = NULL;
    const char  *psztmpFilename = NULL;
    char        *pszProj4 = NULL, *m_pszProj4=NULL;

    string      gdalBinDir;           //GDAL directory
    string      dataDir, in_modisFile,out_modisFile;    //image directory and MODIS file 
    string      cmd_str;  //comand to call an executable program
    string      tmp_str,imageStr;
    DIR         *dp;      //image data directory
    struct dirent *dirp;

    //GDAL related
    const char *pszWKT = NULL; 
    char       *pszWKT_nc = NULL;
    OGRSpatialReference oSRS;
    string          rDataFile;
    GDALDataset     *poRDataset;
    GDALDriver      *poDrive;
    GDALRasterBand  *poBand;
    double          adfGeoTransform[6];
    double          xCellSize, yCellSize;
    int             nBlockXSize, nBlockYSize;
    int             bGotMin, bGotMax;
    double          adfMinMax[2];

    //OGR related
    GDALDataset         *poDS = NULL;
    const char          *pszDriverName = "ESRI Shapefile";
    GDALDriver          *poDriver = NULL;
    const char          *layername = NULL;
    OGRLayer            *poLayer = NULL;
    OGREnvelope         oExt;

    int      i;
    double   xMin,xMax,yMin,yMax;  //domain extent in NLCD 30m grid  *.5 if divided by 30 
    int      xcells, ycells;
    char     extStr[150], sizStr[150];

  //print program version
   printf ("\nUsing: %s\n", prog_version);

   printf("\nProcessing grid shapefile and MODIS image to NLCD projection...\n\n"); 
/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
    GDALAllRegister();
    
/* -------------------------------------------------------------------- */
/*      Processing arguments and environmental variables.               */
/* -------------------------------------------------------------------- */
   
    //printf("\n Arguments = %d\n",nArgc);
    if( nArgc == argsN + 1)
    {
        pszSrcFilename = papszArgv[1];
        polyID = papszArgv[2];
        pszDstFilename = papszArgv[3]; 
        dataDir = string( papszArgv[4] );  
        in_modisFile = string( papszArgv[5] );
        out_modisFile = string( papszArgv[6] );
    }
    else if (nArgc == 1)
    {
       pszSrcFilename = getEnviVariable( "GRID_SHAPEFILE_NAME");
       polyID = getEnviVariable( "POLYGON_ID");
       pszDstFilename = getEnviVariable( "GRID_RASTERFILE_NAME");
       dataDir = string ( getEnviVariable( "DATADIR") );
       in_modisFile = string ( getEnviVariable( "INPUT_MODISFILE") );
       out_modisFile = string ( getEnviVariable( "OUTPUT_MODISFILE") );
    }
    else 
    {
          printf("\nError in input arguments.  Should have %d arguments.\n", argsN);
          Usage();
          exit( -nArgc );
    } 

   printf("Input shapefile: %s\n",pszSrcFilename);
   FileExists(pszSrcFilename, 0 );  //the file has to exist.

   printf("Output raster file: %s\n",pszDstFilename);
   //use .xml to check to check due to .bil does not work properly
   tmp_str = string(pszDstFilename);
   tmp_str.append(".aux.xml");
   //if the file exists delete it
   if (stat(tmp_str.c_str(), &stFileInfo) == 0)
   {
       printf( "\tOutput 30m grid file exists and delete it: %s\n", pszDstFilename );
       poRDataset = (GDALDataset *) GDALOpen( pszDstFilename, GA_ReadOnly );
       if( poRDataset == NULL )
       {
          printf( "\tOpen raster file failed: %s.\n", pszDstFilename );
          exit( 1 );
       }  
       poDrive = poRDataset->GetDriver();
       GDALClose( (GDALDatasetH) poRDataset );
       if ( poDrive->Delete( pszDstFilename )  == CE_Failure )
       {
          printf( "Deleting the file failed: %s\n", pszDstFilename );
          exit( 1 );
       }  
       printf( "\tDeleted existing file: %s\n", pszDstFilename );
       
   }

   dataDir = processDirName(dataDir );  //make sure that dir ends with path separator
   printf("Preprocessed NLCD file directory: %s\n",dataDir.c_str() );
   FileExists(dataDir.c_str(), 0 );  //the dir has to exist.

   in_modisFile = trim ( in_modisFile );
   printf("Input MODIS file:  %s\n",in_modisFile.c_str() );
   FileExists(in_modisFile.c_str(), 0 );  //the file has to exist.

   out_modisFile = trim ( out_modisFile );
   printf("Output MODIS file:  %s\n",out_modisFile.c_str());
   //if the file exists delete it
   if (stat(out_modisFile.c_str(), &stFileInfo) == 0)
   {
       printf( "\tOutput MODIS File exists and delete it: %s\n", out_modisFile.c_str() );
       poRDataset = (GDALDataset *) GDALOpen( out_modisFile.c_str(), GA_ReadOnly );
       if( poRDataset == NULL )
       {
          printf( "\tOpen raster file failed: %s.\n", out_modisFile.c_str() );
          exit( 1 );
       }  
       poDrive = poRDataset->GetDriver();
       GDALClose( (GDALDatasetH) poRDataset );
       if ( poDrive->Delete( out_modisFile.c_str() )  == CE_Failure )
       {
          printf( "Deleting the file failed: %s\n", out_modisFile.c_str() );
          exit( 1 );
       }  
       printf( "\tDeleted existing file: %s\n", out_modisFile.c_str() );
       
   }

   // get GDALBIN environment variable which is set from the running program
   gdalBinDir =   string ( getEnviVariable("GDALBIN") );
   gdalBinDir =  processDirName( gdalBinDir );
   printf("GDAL bin directory: %s\n", gdalBinDir.c_str() );
   FileExists(gdalBinDir.c_str(), 0 );  //the dir has to exist
   
/* -------------------------------------------------------------------- */
/*     Get NLCD projection and cell size from an image file.            */
/* -------------------------------------------------------------------- */
    //open image data directory to get an image file
    if((dp  = opendir(dataDir.c_str())) == NULL) 
    {
        printf( "Error in opening directory: %s\n",dataDir.c_str() );
        exit ( 1 );
    }

    //get a *.bil image file
    while ((dirp = readdir(dp)) != NULL) 
    {
        tmp_str = string(dirp->d_name);
        tmp_str = trim(tmp_str);
        if ( tmp_str.find(".bil") !=string::npos && tmp_str.find("landcover") !=string::npos)
        {
           i = tmp_str.length() - 4;  //.bil position
           //printf ("lenght %s = %d\n", tmp_str.c_str(),i);
           string bil_str = tmp_str.substr(i,4);  
           if ( bil_str.compare(".bil" ) == 0 )
           {
              imageStr = tmp_str;
              break;  //get out of loop 
           }
        }
    }   
    closedir(dp);

    if ( imageStr.empty() )
    {
       printf ( "Error: no *.bil image file in directory: %s\n",dataDir.c_str() );
       exit ( 1 );
    }
    
    // build arguments to call gdal_translate
    rDataFile = string( dataDir );
    rDataFile.append( imageStr );  // use one of image files in preprocessed image dir
    printf( "\nObtain NLCD projection and cell information from image:  %s\n",rDataFile.c_str() );

    
    poRDataset = (GDALDataset *) GDALOpen( rDataFile.c_str(), GA_ReadOnly );
    if( poRDataset == NULL )
    {
       printf( "\tOpen raster file failed: %s.\n", rDataFile.c_str() );
       exit( 1 );
    }

    printf( "\tDriver: %s/%s\n",
            poRDataset->GetDriver()->GetDescription(), 
            poRDataset->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );

    printf( "\tSize is %dx%dx%d\n", 
            poRDataset->GetRasterXSize(), poRDataset->GetRasterYSize(),
            poRDataset->GetRasterCount() );

    if( poRDataset->GetGeoTransform( adfGeoTransform ) == CE_None )
    {
        printf( "\tOrigin = (%.6f,%.6f)\n",
                adfGeoTransform[0], adfGeoTransform[3] );

        xCellSize =  adfGeoTransform[1] ;
        yCellSize =  -1 * adfGeoTransform[5] ;  // Make it positive.  Negative due to Origin is UL corner point
        printf( "\tPixel Size = (%.3f,%.3f)\n", xCellSize, yCellSize );
       
    }

   if( (pszWKT = poRDataset->GetProjectionRef())  == NULL )
   {
      printf( "\tProjection is not defined in the NLCD file: %s.\n", rDataFile.c_str() );
      exit( 1 );
   }

   pszWKT_nc =strdup ( pszWKT ); 
   oSRS.importFromWkt( &pszWKT_nc );
   oSRS.exportToProj4( &pszProj4 ); 
   printf ( "\tProj4 from NLCD = %s\n", pszProj4);
   
   GDALClose( (GDALDatasetH) poRDataset );


/* -------------------------------------------------------------------- */
/*     Project shapefile into NLCD projection.                          */
/* -------------------------------------------------------------------- */

    //create shapefile name in NLCD projection
    tmp_str = string (pszSrcFilename);
    i = tmp_str.rfind("/", tmp_str.length() );
    if (i != string::npos)
    {
      tmp_str.erase(0,i+1);  //get rid of dir part 
    }
    i = tmp_str.rfind(".shp", tmp_str.length());
    tmp_str.erase(i);   //get rid of .shp for layer name
    tmp_str.append( "_nlcd.shp" );

    psztmpFilename = tmp_str.c_str();  //temp shapefile name
    //printf (" TEMP NLCD File = %s\n",psztmpFilename);

    printf("\nProjecting %s to NLCD projection and stored in: %s\n",pszSrcFilename, psztmpFilename);


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
       if ( (poDriver->Delete( psztmpFilename )) == CE_Failure )
       {
          printf( "\tError in deleting temp shapefile: %s\n\n", psztmpFilename );
       }
    }


    //build command line to call ogr2ogr
    cmd_str = string( gdalBinDir );
    cmd_str.append("ogr2ogr -t_srs \"");
    cmd_str.append(pszProj4);
    cmd_str.append("\"  ");
    cmd_str.append(" ");
    cmd_str.append(psztmpFilename);
    cmd_str.append(" ");
    cmd_str.append(pszSrcFilename);
    printf("\t%s\n", cmd_str.c_str());

    //call ogr2ogr to project the shapefile
    
    if ( system(cmd_str.c_str()) != 0 )
    {
       printf("\tError in system call: %s\n", cmd_str.c_str());
       exit (1);
    }
   
    printf("\tSuccessful in projecting the shapefile to NLCD format.\n\n");
    

/* -------------------------------------------------------------------- */
/*     Rasterizing projected shapefile into NLCD 30m grids              */
/* -------------------------------------------------------------------- */

    printf("Rasterizing projected the shapefile to NLCD 30m grids...\n");
   
    //open the projected shapefile to get map extent  
/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
     GDALAllRegister ();

/* -------------------------------------------------------------------- */
/*      Open projected shapfile file.                                   */
/* -------------------------------------------------------------------- */

    poDS = (GDALDataset*) GDALOpenEx( psztmpFilename, GDAL_OF_VECTOR, NULL, NULL, NULL );
    if( poDS == NULL )
    {
      printf( "\tOpen shapefile file failed: %s.\n", psztmpFilename );
      exit( 1 );
    }
    
/* -------------------------------------------------------------------- */
/*      Get layer extent to make new raster domain.                      */
/* -------------------------------------------------------------------- */
    poLayer = poDS->GetLayer( 0 );
    if( poLayer == NULL )
    { 
        printf( "\tFAILURE: Couldn't fetch layer %s!\n", psztmpFilename );
                exit( 1 );
    }

    if (poLayer->GetExtent(&oExt, TRUE) == OGRERR_NONE)
    {
      printf("\tProject shapefile extent min_xy: (%.0lf, %.0lf) max_xy: (%.0lf, %.0lf)\n",
                   oExt.MinX, oExt.MinY, oExt.MaxX, oExt.MaxY);
    }

    //NLCD grid center at 0,30,60: x or y /30 = *.5
    xMin = ( floor(oExt.MinX / xCellSize + 0.5)  - 0.5 ) * xCellSize;   
    yMin = ( floor(oExt.MinY / yCellSize + 0.5)  - 0.5 ) * yCellSize;

    xMax = ( ceill(oExt.MaxX / xCellSize + 0.5)  - 0.5 ) * xCellSize;
    yMax = ( ceill(oExt.MaxY / yCellSize + 0.5)  - 0.5 ) * yCellSize;

    xcells = (int) ( (xMax - xMin) / xCellSize );
    ycells = (int) ( (yMax - yMin) / xCellSize );
    printf("\tModified extent min_xy: (%f, %f) max_xy: (%f, %f)\n", xMin, yMin, xMax, yMax);
    printf("\txcells = %d   ycells = %d\n",xcells,ycells);

    
     GDALClose ( poDS );

/* -------------------------------------------------------------------- */
/*      Create a domain raster data set to store rasterized grid data   */
/* -------------------------------------------------------------------- */

    // build arguments to call gdal_translate
    cmd_str = string(gdalBinDir);
    cmd_str.append("gdal_translate -ot UInt32 ");  //create unsigned 32 byte int images to hold grid ID
    sprintf(extStr, "-a_ullr %f %f %f %f \0",xMin,yMax,xMax,yMin);
    cmd_str.append(extStr);
    sprintf(sizStr,"-scale 0 255 0 0 -outsize %d %d -of EHdr \0",xcells,ycells);
    cmd_str.append( sizStr );
    cmd_str.append( rDataFile );
    cmd_str.append( "  " );
    cmd_str.append( pszDstFilename );

    printf("\t%s\n", cmd_str.c_str());
                
    //call gdal_translate to create 0 value domain 30m grid 
    
    if ( system(cmd_str.c_str()) != 0 )
    { 
      printf("\tError in system call: %s\n", cmd_str.c_str());
      exit (1);
    }
   
   printf("\tSuccessful in creating 0 value domain 30m grid for rasterizing.\n\n");

/* -------------------------------------------------------------------- */
/*      Rasterize input shapefile to the created domain raster file     */
/* -------------------------------------------------------------------- */
    // build arguments to call gdal_rasterize
    cmd_str = string(gdalBinDir);
    cmd_str.append("gdal_rasterize -a ");
    cmd_str.append(polyID);
    cmd_str.append(" -l "); 
    cmd_str.append( psztmpFilename );
    
    i = cmd_str.rfind(".shp", cmd_str.length());
    cmd_str.erase(i);  //get rid of .shp for layer name

    cmd_str.append ( " ");
    cmd_str.append ( psztmpFilename ); 
    cmd_str.append ( " ");
    cmd_str.append ( pszDstFilename );

    printf("\t%s\n", cmd_str.c_str());

    //call gdal_rasterize 
 
    if ( system(cmd_str.c_str()) != 0 )
    {
      printf("\tError in system call: %s\n", cmd_str.c_str());
      exit (1);
    }    
  
    //delete temp projected file
    poDriver = GetGDALDriverManager()->GetDriverByName( pszDriverName );
    if( poDriver == NULL )
    {
       printf( "%s driver not available.\n", pszDriverName );
       exit( 1 );
    }
    if ( (poDriver->Delete( psztmpFilename )) == CE_Failure )
    {
       printf( "\tError in deleting temp shapefile: %s\n\n", psztmpFilename );
    }
        

    printf ("\tCompleted in rasterizing the shapefile.\n\n");
  

/* -------------------------------------------------------------------- */
/*      clip and reproject MODIS data into the modeling doamin          */
/* -------------------------------------------------------------------- */
    //open MODIS image to get cell size and projection info
    printf ("Projecting and clipping MODIS image to NLCD projection and grid domain.\n");

    poRDataset = (GDALDataset *) GDALOpen( in_modisFile.c_str(), GA_ReadOnly );
    if( poRDataset == NULL )
    {
       printf( "\tOpen raster file failed: %s.\n", in_modisFile.c_str() );   
       exit( 1 );
    }
    
    printf( "\tDriver: %s/%s\n",
            poRDataset->GetDriver()->GetDescription(), 
            poRDataset->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );
    
    printf( "\tSize is %dx%dx%d\n",
            poRDataset->GetRasterXSize(), poRDataset->GetRasterYSize(),
            poRDataset->GetRasterCount() );
    
    if( poRDataset->GetGeoTransform( adfGeoTransform ) == CE_None )
    {   
        printf( "\tOrigin = (%.6lf,%.6lf)\n",
                adfGeoTransform[0], adfGeoTransform[3] );
           
        xCellSize =  adfGeoTransform[1] ;
        yCellSize =  -1 * adfGeoTransform[5] ;  // Make it positive.  Negative due to Origin is UL corner point
        printf( "\tModis Image Pixel Size = (%.3lf,%.3lf)\n", xCellSize, yCellSize );
           
    }         
              
   if( (pszWKT = poRDataset->GetProjectionRef())  == NULL )
   {    
      printf( "\tProjection is not defined in the MODIS file: %s.\n", in_modisFile.c_str() );
      exit( 1 );
   }
       
   pszWKT_nc =strdup ( pszWKT );
   oSRS.importFromWkt( &pszWKT_nc );
   oSRS.exportToProj4( &m_pszProj4 );
   if ( strstr(m_pszProj4, "+proj=") == NULL )
   {
      //m_pszProj4 = strdup("+proj=laea +lat_0=50 +lon_0=-100 +a=6371007.181 +b=6371007.181");
      m_pszProj4 = strdup(pszProj4);
      printf ( "\tMODIS image projection is not defined.  Use default projection: %s\n",m_pszProj4);
      //printf( "Can define it using gdal_translate utility.\n");
      //exit( 1 );
   }
   GDALClose( (GDALDatasetH) poRDataset );


    // build arguments to call gdalwarp

    //get image cell information
    cmd_str = string(gdalBinDir);
    cmd_str.append("gdalwarp -s_srs \"");
    cmd_str.append(m_pszProj4);
    cmd_str.append("\"  -t_srs \"");
    cmd_str.append(pszProj4);
    cmd_str.append("\"  -te ");

    xCellSize = ceil ( xCellSize );
    yCellSize = ceil ( yCellSize ); 
    printf( "\tNew Modis Image Pixel Size = (%.3f,%.3f)\n", xCellSize, yCellSize );

    xMax = xMin + ceil ( (xMax - xMin) / xCellSize ) * xCellSize;  //round max x to xSize of image
    yMax = yMin + ceil ( (yMax - yMin) / yCellSize ) * yCellSize;  //round max y to ySize of image
    printf("\tComputed MODIS Extent: min_xy: %f %f  max_xy: %f %f \n",xMin,yMin,xMax,yMax);

    sprintf(extStr, "%f %f %f %f -tr %.0lf  %.0lf \0",xMin,yMin,xMax,yMax,xCellSize,yCellSize);
    cmd_str.append(extStr);
    cmd_str.append( in_modisFile );
    cmd_str.append ( " ");
    cmd_str.append (out_modisFile);
    printf("\t%s\n", cmd_str.c_str());

    //call gdalwrap to clip and reproject it
    if ( system(cmd_str.c_str()) != 0 )
    {
       printf("\tError in system call: %s\n", cmd_str.c_str());
       exit (1);
    }

    printf ("\tCompleted projecting and clipping MODIS data.\n");


}
/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage()

{
    printf( "\nError in input arguments.\n");
    printf( "Usage: toNLCDRaster.exe  grid_shapefile GRIDID grid_shapefile_raster.bil NLCD_data_dir MODIS_data.bsq grid_modis.bsq\n");
    exit( 1 );
}

