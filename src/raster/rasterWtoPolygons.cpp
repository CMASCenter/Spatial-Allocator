/********************************************************************
 * This program is used: 
 *  1. to project a polygon shapefile into a raster image projecttion. 
 *  2. to rasterize the polygon shapefile into the raster image grids.
 *  3. to output raster value total to the polygon ID item
 *
 * Written by the Institute for the Environment at UNC, Chapel Hill
 * in support of the EPA NOAA CMAS Modeling, 2007-2008.
 * 
 * By Limei Ran, May, 2008
 *
 * Usage: ./rasterToPolygons.exe 
 *        Environment Variables needed: 
 *        GDALBIN -- GDAL bin directory
 *        POLYGON_SHAPEFILE_NAME -- polygon shapefile name
 *        POLYGON_ID -- polygon ID used to rasterize the shapefile
 *        POLYGON_RASTERFILE_NAME -- rasterized polygon image file name
 *        WEIGHT_RASTER_FILE -- weight raster image file
 *        WEIGHT_TYPE -- what kind of the data is, for example POPULATION or HOUSING UNIT
 *        OUTPUT_WIEGHT_NAME -- raster weight item name to be added to the output shapefile
 *        OUTPUT_TEXT_FILE -- Name of output text file.  It can be new or existing file.  If it exists, new item will 
 *                            be added to the text file table. 
 */
#include <map>
#include <iostream>
#include <fstream>
#include "ogrsf_frmts.h"
#include "ogr_geometry.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "ogr_api.h"
#include "gdal_priv.h"

#include "sa_raster.h"
#include "commontools.h"


/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main( int nArgc,  char* papszArgv[] )
{
    struct stat stFileInfo;

    string  pszSrcFilename;
    string  pszDstFilename;
    string  pszWRstFilename;
    string  psztmpFilename;
    string  polygonID;
    string  weightType;
    string  itemName;
    string  outTextFile;         //shapefile with added item
    string  gdalBinDir;           //GDAL directory


    //GDAL related
    GDALDataset     *poRDataset_std, *poRDataset;
    GDALRasterBand  *poBand_std, *poBand;
    GDALDriver      *poDrive;
    double          adfGeoTransform[6];
    double          xCellSize_std, yCellSize_std;  //cell size for weight image
    double          xCellSize, yCellSize;          //cell size for rasterized polygon image
    int             bGotMin, bGotMax;
    double          adfMinMax[2];
    double          xUL,yUL;
    double          xMin_std,xMax_std,yMin_std,yMax_std;  //extent from weight image  
    double          xMin,xMax,yMin,yMax;  //extent from rasterized polygon image
    OGRSpatialReference   oSRS_std;  //projection from weight image
    OGRSpatialReference   oSRS;      //projection from rasterized polygon image
    int             xCells_std, yCells_std;  //columns and rows for weight image
    int             xCells, yCells;  //columns and rows for rasterized polygon image
    const char      *pszWKT = NULL; 
    char            *pszWKT_nc = NULL;
    char            *pszProj4_std = NULL, *pszProj4 = NULL;

    bool            Rasterizing = false; 
    string          tmp_str;
    string          cmd_str;  //comand to call an executable program

    //OGR related
    OGRDataSource       *poDS = NULL;
    OGRSFDriver         *poOGRDrive = NULL;
    OGRLayer            *poLayer = NULL;
    OGRSpatialReference *oSRS_poly;      //projection from input polygon shapefile
    OGREnvelope         oExt;
    OGRFeature          *poFeature;


    int      i,j;
    char     extStr[150], sizStr[150];
   
    std::map<int,double>           polyW; 
    std::map<int,double>::iterator it;
    int                            polyID;
    double                         cellValue;

    std::vector<string>            outRows; 
    std::ifstream                  inFileS;             //input existing text file stream
    std::ofstream                  outFileS;            //new output text file stream
    string                         itemLine;            //one line with items
    string                         spolyID;             //string polyID;


    //print program version
    printf ("\nUsing: %s\n", prog_version);


    printf("\nAllocating raster weight to polygons...\n\n"); 
/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
    OGRRegisterAll();
    GDALAllRegister();
    
/* -------------------------------------------------------------------- */
/*      Processing environmental variables.               */
/* -------------------------------------------------------------------- */
   
   pszSrcFilename = string( getEnviVariable( "POLYGON_SHAPEFILE_NAME") );
   polygonID = string( getEnviVariable( "POLYGON_ID") );
   pszDstFilename = string( getEnviVariable( "POLYGON_RASTERFILE_NAME") );
   pszWRstFilename = string ( getEnviVariable( "WEIGHT_RASTER_FILE") );
   weightType = string( getEnviVariable( "WEIGHT_TYPE") );
   itemName = string ( getEnviVariable( "OUTPUT_WEIGHT_NAME") );
   outTextFile = string ( getEnviVariable( "OUTPUT_TEXT_FILE") );

   printf("Input polygon shapefile: %s\n",pszSrcFilename.c_str());
   FileExists(pszSrcFilename.c_str(), 0 );  //the file has to exist.

   printf("Polygon ID is: %s\n",polygonID.c_str());

   //the file to be created 
   printf("Output rasterized polygon shapefile: %s\n",pszDstFilename.c_str());

   printf("Weight raster file is: %s\n",pszWRstFilename.c_str() );
   FileExists(pszWRstFilename.c_str(), 0 );  //the file has to exist.

   printf("Weight type is: %s\n",weightType.c_str());
   printf("Weight item name in output shapefile:  %s\n",itemName.c_str() );

   //output text file
   printf("Output text file with added weight item:  %s\n",outTextFile.c_str());
   FileExists(outTextFile.c_str(), 3 );  //the file has to be new.

   // get GDALBIN environment variable which is set from the running program
   gdalBinDir =   string ( getEnviVariable("GDALBIN") );
   gdalBinDir =  processDirName( gdalBinDir );
   printf("GDAL bin directory: %s\n", gdalBinDir.c_str() );
   FileExists(gdalBinDir.c_str(), 0 );  //the dir has to exist
   
/* -------------------------------------------------------------------- */
/*     Get Weight raster file projection, cell size, and extent         */
/* -------------------------------------------------------------------- */
    printf( "\nObtain weight raster file projection, cell size, and extent:  %s\n",pszWRstFilename.c_str() );
    
    poRDataset_std = (GDALDataset *) GDALOpen( pszWRstFilename.c_str(), GA_ReadOnly );
    if( poRDataset_std == NULL )
    {
       printf( "\tOpen raster file failed: %s.\n", pszWRstFilename.c_str() );
       exit( 1 );
    }

    printf( "\tDriver: %s/%s\n",
            poRDataset_std->GetDriver()->GetDescription(), 
            poRDataset_std->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );

    // get rows and columns of the image
    xCells_std = poRDataset_std->GetRasterXSize();
    yCells_std = poRDataset_std->GetRasterYSize();
    printf( "Size is %dx%dx%d\n", xCells_std, yCells_std, poRDataset_std->GetRasterCount() );

    if( poRDataset_std->GetGeoTransform( adfGeoTransform ) == CE_None )
    {
        xUL = adfGeoTransform[0];
        yUL = adfGeoTransform[3];
        printf( "Origin = (%.6f,%.6f)\n", xUL,yUL );

        xCellSize_std =  adfGeoTransform[1] ;
        yCellSize_std =  -1 * adfGeoTransform[5] ;  // Make it positive.  Negative due to Origin is UL corner point
        printf( "\tPixel Size = (%.3f,%.3f)\n", xCellSize_std, yCellSize_std );
    }
    else
    {
       printf( "\tObtaining transform information failed: %s.\n", pszWRstFilename.c_str() );
       exit( 1 );
    }
   
    //compute extent of band 1 image (only one band for all images)
    xMin_std = xUL;
    xMax_std = xUL + xCellSize_std * xCells_std;
    yMax_std = yUL;
    yMin_std = yUL - yCellSize_std * yCells_std;
    printf( "\textent:  min(%.3f,%.3f)   max(%.3f,%.3f)\n", xMin_std,yMin_std,xMax_std,yMax_std );

    //compute min and max of the band
    poBand_std = poRDataset_std->GetRasterBand( 1 );  // get band 1 from raster image

    GDALComputeRasterMinMax( poBand_std, TRUE, adfMinMax );
    printf ("\tRaster value: Min=%.0lf    Max=%.0lf\n",adfMinMax[0],adfMinMax[1]);
    
    if( (pszWKT = poRDataset_std->GetProjectionRef())  == NULL )
    {
       printf( "\tProjection is not defined in weight raster file: %s.\n", pszWRstFilename.c_str() );
       exit( 1 );
    }

   pszWKT_nc =strdup ( pszWKT ); 
   oSRS_std.importFromWkt( &pszWKT_nc );
   oSRS_std.exportToProj4( &pszProj4_std ); 
        
   printf ( "\tProj4 from weight raster file = %s\n", pszProj4_std);
   
   GDALClose( (GDALDatasetH) poRDataset_std );

/* -------------------------------------------------------------------- */
/*   Check the need to rasterize polygon shapefile                      */
/* -------------------------------------------------------------------- */
   printf( "\nChecking the need to rasterize polygon shapefile...\n" );
  
   //check rasterized existence, if yes check cell size, extent, projection
   if (stat(pszDstFilename.c_str(), &stFileInfo) == 0)
   { 
      //rasterized polygon image exists     
      //get projection, cell size and extent from the image
      printf( "\tObtain rasterized polygon file cell size, extent, and projection  %s\n",pszDstFilename.c_str() );

      poRDataset = (GDALDataset *) GDALOpen( pszDstFilename.c_str(), GA_ReadOnly );
      if( poRDataset == NULL )
      {
         printf( "\tOpen raster file failed: %s.\n", pszDstFilename.c_str() );
         exit( 1 );
      }

      printf( "\tDriver: %s/%s\n",
            poRDataset->GetDriver()->GetDescription(),
            poRDataset->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );

      // get rows and columns of the image
      xCells = poRDataset->GetRasterXSize();
      yCells = poRDataset->GetRasterYSize();
      printf( "Size is %dx%dx%d\n", xCells, yCells, poRDataset->GetRasterCount() );

      if( poRDataset->GetGeoTransform( adfGeoTransform ) == CE_None )
      {
         xUL = adfGeoTransform[0];
         yUL = adfGeoTransform[3];
         printf( "Origin = (%.6f,%.6f)\n", xUL,yUL );

         xCellSize =  adfGeoTransform[1] ;
         yCellSize =  -1 * adfGeoTransform[5] ;  // Make it positive.  Negative due to Origin is UL corner point
         printf( "\tPixel Size = (%.3f,%.3f)\n", xCellSize, yCellSize );
         if ( xCellSize_std != xCellSize || yCellSize_std != yCellSize )
         {
            Rasterizing = true;      
         }
  
         //compute extent of band 1 image (only one band for all images)
         xMin = xUL;
         xMax = xUL + xCellSize * xCells;
         yMax = yUL;
         yMin = yUL - yCellSize * yCells;
         printf( "extent:  min(%.3f,%.3f)   max(%.3f,%.3f)\n", xMin,yMin,xMax,yMax );
         if (xMin != xMin_std || xMax != xMax_std || yMax != yMax_std || yMin != yMin_std)
         {
            Rasterizing = true;
         }
      }
      else
      {
         Rasterizing = true;
      }

      if( (pszWKT = poRDataset->GetProjectionRef())  == NULL )
      {
         printf( "\tProjection is not defined in rasterized polygon file: %s.\n", pszDstFilename.c_str() );
         exit( 1 );
      }

      pszWKT_nc =strdup ( pszWKT );
      oSRS.importFromWkt( &pszWKT_nc );
      oSRS.exportToProj4( &pszProj4 );
      printf ( "\tProj4 from rasterized polygon raster file = %s\n", pszProj4);
      
      //check the rasterized polygon image has the same projection as weight raster image
      if (! oSRS.IsSame (&oSRS_std) )
      {
         printf( "\tRasterized polygon image's projection is different from weight raster image's.\n" );
         Rasterizing = true;
      }
      
      poDrive = poRDataset->GetDriver();
      GDALClose( (GDALDatasetH) poRDataset );
      poRDataset = NULL;
      
      if ( Rasterizing )
      {
         if ( poDrive->Delete( pszDstFilename.c_str() )  == CE_Failure )
         {
            printf( "Deleting the file failed: %s\n", pszDstFilename.c_str() );
            exit( 1 );
         }
         printf( "\tDeleted the file: %s\n", pszDstFilename.c_str() );
      }
  }
  else
  {
      Rasterizing = true;   //rasterized polygon image does not exist
  }
  
  if ( ! Rasterizing )
  {
     printf("\tNo need to project polygon shapefile to weight image projection.\n\n");
  }
  

/* -------------------------------------------------------------------- */
/*     Project polygon shapefile into weight raster projection.         */
/* -------------------------------------------------------------------- */
    if ( Rasterizing )
    { 
       printf( "\nProjecting polygon shapefile into weight raster projection...\n" ); 
    
       //build command line to call ogr2ogr
       cmd_str = string( gdalBinDir );
       cmd_str.append("ogr2ogr -t_srs \"");
       cmd_str.append(pszProj4_std);
       cmd_str.append("\"  ");

       psztmpFilename = string(pszSrcFilename);
       i = psztmpFilename.rfind(".shp", psztmpFilename.length());
       psztmpFilename.erase(i); 
       psztmpFilename.append("_wproj.shp");

       //delete tmp projected shapefile if exists
       if (stat(psztmpFilename.c_str(), &stFileInfo) == 0) 
       {
           poDS = OGRSFDriverRegistrar::Open( psztmpFilename.c_str(), FALSE );
           if( poDS == NULL )
           {
              printf( "\tOpen shapefile file failed: %s.\n", psztmpFilename.c_str() );
              exit( 1 );
           }
           poOGRDrive = poDS->GetDriver(); 
           OGRDataSource::DestroyDataSource( poDS );

           if ( poOGRDrive->DeleteDataSource( psztmpFilename.c_str() ) != OGRERR_NONE )
           {
              printf( "\tDeleting the file failed: %s\n", psztmpFilename.c_str() );
              exit( 1 );
           }
           printf( "\tDeleted the file: %s\n", psztmpFilename.c_str() );
       }

       //check polygon shapefile's projection
       poDS = OGRSFDriverRegistrar::Open( pszSrcFilename.c_str(), FALSE );
       if( poDS == NULL )
       {
           printf( "\tOpen shapefile file failed: %s.\n", pszSrcFilename.c_str() );
           exit( 1 );
       }
       poLayer = poDS->GetLayer( 0 );
       if( poLayer == NULL )
       {
           printf( "\tFAILURE: Couldn't fetch layer %s!\n", pszSrcFilename.c_str() );
           exit( 1 );
       }
       oSRS_poly = poLayer->GetSpatialRef(); 
       if( oSRS_poly == NULL )
       {
           printf( "\tFAILURE: Projection is not defined: %s!\n", pszSrcFilename.c_str() );
           exit( 1 );
       }

       if (! oSRS_poly->IsSame (&oSRS_std) )
       {
          printf("\nProjecting %s to weight raster projection and stored in: %s\n",pszSrcFilename.c_str(), psztmpFilename.c_str() );
    
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
   
          printf("\tSuccessful in projecting the shapefile to weight image projection.\n\n");
       }
       else
       {
          psztmpFilename = pszSrcFilename;   //will rasterized from the input shapefile
          printf("\tPolygon shapefile has the same projection as weight image's.\n\n");
       }
       OGRDataSource::DestroyDataSource( poDS );
   }  
     
/* -------------------------------------------------------------------- */
/*     Rasterizing projected shapefile into weight raster grids         */
/* -------------------------------------------------------------------- */

    if ( Rasterizing )
    {

       printf("Rasterizing the shapefile to weight raster grids...\n");
   
/* -------------------------------------------------------------------- */
/*      Create a empty raster data set to store rasterized grid data   */
/* -------------------------------------------------------------------- */

       // build arguments to call gdal_translate
       cmd_str = string(gdalBinDir);
       cmd_str.append("gdal_translate -ot UInt32 ");  //create unsigned 32 byte int images to hold polygon ID
       //sprintf(extStr, "-a_ullr %f %f %f %f \0",xMin_std,yMax_std,xMax_std,yMin_std);
       //cmd_str.append(extStr);
       sprintf(sizStr,"-scale %.0lf %.0lf  0 0 -of EHdr  \0",adfMinMax[0],adfMinMax[1]);
       cmd_str.append( sizStr );
       cmd_str.append( pszWRstFilename );
       cmd_str.append( "  " );
       cmd_str.append( pszDstFilename );

       printf("\t%s\n", cmd_str.c_str());
                
       //call gdal_translate to create 0 value grid 
    
       if ( system(cmd_str.c_str()) != 0 )
       { 
         printf("\tError in system call: %s\n", cmd_str.c_str());
         exit (1);
       }
   
       printf("\tSuccessful in creating 0 value raster file for rasterizing.\n\n");

/* -------------------------------------------------------------------- */
/*      Rasterize input shapefile to the created domain raster file     */
/* -------------------------------------------------------------------- */
      // build arguments to call gdal_rasterize
      cmd_str = string(gdalBinDir);
      cmd_str.append("gdal_rasterize -a ");
      cmd_str.append( polygonID );
      cmd_str.append( " -l ");
    
      //just get image name
      if ( (i=psztmpFilename.find_last_of("/\\")) == string::npos )
      {
         printf( "Error in extract file name from: %s\n",psztmpFilename.c_str() );
         exit (1 );
      }
      cmd_str.append( psztmpFilename.substr(i+1) );

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
  
      printf ("\tCompleted in rasterizing the shapefile.\n\n");
   }
   else
   {
       printf("\tNo need to rasterizing polygon shapefile.\n\n");
   }
/* -------------------------------------------------------------------- */
/*    Allocate weight raster to polygon raster                          */
/* -------------------------------------------------------------------- */
   printf( "\nAllocating raster weight to polygons...\n" );

   //open rasterized polygon file
   poRDataset = (GDALDataset *) GDALOpen( pszDstFilename.c_str(), GA_ReadOnly );
   if( poRDataset == NULL )
   {
       printf( "\tOpen raster file failed: %s.\n", pszDstFilename.c_str() );
       exit( 1 );
   }
   poBand = poRDataset->GetRasterBand( 1 );  // get band 1 from polygon image
   //allocate space to store 1-row polygon ID image
   GUInt32 *poImage = (GUInt32 *) CPLCalloc(sizeof(GUInt32),xCells_std);

   //open weight raster file
   poRDataset_std = (GDALDataset *) GDALOpen( pszWRstFilename.c_str(), GA_ReadOnly );
   if( poRDataset_std == NULL )
   {
       printf( "\tOpen raster file failed: %s.\n", pszWRstFilename.c_str() );
       exit( 1 );
   }
   poBand_std = poRDataset_std->GetRasterBand( 1 );  // get band 1 from weight image
   //allocate double to store 1-row weight raster
   double *poImage_std = (double *) CPLCalloc(sizeof(double),xCells_std);

   for( i = 0; i<yCells_std; i++ )
   {
       //read polyon image
       if ( (poBand->RasterIO(GF_Read, 0, i, xCells_std, 1, poImage, xCells_std, 1, GDT_UInt32, 0, 0)) == CE_Failure)
        {
            printf( "\tError: Reading row = %d from polygon image.\n",i+1);
            CPLFree (poImage);
            exit( 1 );
        }

       //read weight image
        if ( (poBand_std->RasterIO(GF_Read, 0, i, xCells_std, 1, poImage_std, xCells_std, 1, GDT_Float64, 0, 0)) == CE_Failure)
        {
            printf( "\tError: Reading row = %d from weight image.\n",i+1);
            CPLFree (poImage_std);
            exit( 1 );
        }

        if (weightType.compare("ICLUS_HOUSING_DENSITY") == 0 )
        {
           for (j=0; j<xCells_std; j++)
           {
              polyID = poImage[j] ;          
              cellValue = poImage_std[j] ;
              if (cellValue >= 0.0 )
              {
                if ( polyW.count( polyID ) <= 0 )        
                {
                   //first time 
                   polyW[polyID] = cellValue / 1000.0;
                }
                else
                {
                  cellValue = polyW[polyID] + cellValue /1000.0; 
                  polyW[polyID] = cellValue;  
                }
              }  //value weight
           } //end of j
        }  //Housing Density weight

   }  //end of i


   /*
   for ( it=polyW.begin() ; it != polyW.end(); it++ )
   { 
      printf ("POLYON_ID = %d     HU00 = %.2lf\n", (*it).first , (*it).second);
   }
   */

   GDALClose( (GDALDatasetH) poRDataset );
   CPLFree ( poImage );

   GDALClose( (GDALDatasetH) poRDataset_std );
   CPLFree ( poImage_std );
   printf( "\tFinished allocating raster weight to polygons.\n\n" );

/* -------------------------------------------------------------------- */
/*    Add weight item to an output csv file                             */
/* -------------------------------------------------------------------- */

   printf( "Adding computed weight to the output text file.\n" );

   // if the output csv file exists
   if (stat(outTextFile.c_str(), &stFileInfo) == 0)
   {
      inFileS.open( outTextFile.c_str() );
      if (inFileS.good() )
      {
         getline( inFileS, itemLine);
         while ( !inFileS.eof() )
         {
            itemLine = trim (itemLine);  //get rid of spaces at edges
            outRows.push_back(itemLine);
            getline( inFileS, itemLine);
         }  // while loop

         inFileS.close();
      }  //end of good
 
      //remove the file
      if (remove(outTextFile.c_str( )) !=0)
      {
         printf( "\tFailed to delete file: %s\n",outTextFile.c_str( ) );
         exit ( 1 );
      }
   }

   //open output file to write
   outFileS.open( outTextFile.c_str( ) ); 
   if (! outFileS.good() )
   {
       printf( "\tError in opening file: %s\n",outTextFile.c_str() );
       exit ( 1 );
   }

   //output file existing 
   if ( outRows.size() > 0 ) 
   {
      //write title
      itemLine = outRows.at(0); 
      itemLine.append( "," );
      itemLine.append( itemName );
      itemLine.append( "\n" );
      outFileS.write( itemLine.c_str(), itemLine.size() );

      for ( i=1; i<outRows.size(); i++)
      {
        itemLine = outRows.at(i);

        //get POLYGON_ID
        j = itemLine.find_first_of(","); 
        spolyID = string ( itemLine );        
        spolyID.erase ( j );
        polyID = atoi( spolyID.c_str() );

        cellValue =  polyW[polyID];
        sprintf(extStr, ",%.4lf\n",cellValue);
 
        itemLine.append( extStr );        
        outFileS.write( itemLine.c_str(), itemLine.size() );  
      }
   }
   else  //new output file
   {
      //write title
      itemLine = string ( "POLYGON_ID" );
      itemLine.append( "," );
      itemLine.append( itemName );
      itemLine.append( "\n" );
      outFileS.write( itemLine.c_str(), itemLine.size() );

      for ( it=polyW.begin() ; it != polyW.end(); it++ )
      {  
         sprintf (extStr,"%d,%.4lf\n", (*it).first , (*it).second);
         itemLine = string ( extStr );
         outFileS.write( itemLine.c_str(), itemLine.size() ); 
      }   
   }

   outFileS.close ();

   /*
   //open output text file
   poDS = OGRSFDriverRegistrar::Open( out_polyFile.c_str(), TRUE );
   if( poDS == NULL )
   {
       printf( "\tOpen shapefile file failed: %s.\n", out_polyFile.c_str() );
       exit( 1 );
   }
   poLayer = poDS->GetLayer( 0 );
   if( poLayer == NULL )
   {
      printf( "\tFAILURE: Couldn't fetch layer %s!\n", out_polyFile.c_str() );
      exit( 1 );
   }
   
   //create weight item to the attribute 
   OGRFieldDefn oField( itemName.c_str(), OFTReal);
   printf ("\tBuiled field item: %s\n", itemName.c_str() );

   if( poLayer->CreateField( &oField, TRUE ) != OGRERR_NONE )
   {
      printf( "\tFailed to create field: %s\n",itemName.c_str() );
      exit( 1 );
   }

   while( (poFeature = poLayer->GetNextFeature()) != NULL )
   {
      int idIndex = poFeature->GetFieldIndex ( polygonID.c_str() );
      if ( idIndex == -1 ) 
      {
          printf( "\tFailed to get index for shapefile ID field: %s\n", polygonID.c_str() );
          exit( 1 );
      }
    
      int itemIndex = poFeature->GetFieldIndex ( itemName.c_str() );
      if ( itemIndex == -1 )
      {
          printf( "\tFailed to get index for added item shapefile field: %s\n", itemName.c_str() );
          exit( 1 );
      }
     
      polyID = poFeature->GetFieldAsInteger( idIndex );
      cellValue =  polyW[polyID];
      poFeature->SetField(itemIndex, cellValue); 
   }
   */

   printf( "\tFinished adding weight item to the output text file.\n\n" );

   printf( "Program completed.\n" );

}

