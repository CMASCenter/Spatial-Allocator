/********************************************************************
 * This program is used: 
 *  1. to preprocess downloaded NLCD image files which include:
 *            USGS NLCD landuse, 
 *            USGS NLCD canopy,
 *            USGS NLCD imperviousness, and
 *            NOAA C-GAP NLCD landuse along the coast.
 *  2. to eliminate overlapping within each type of data covering US.
 *  3. to create all updated image in EHdr -- ESRI .hdr Labelled (ESRI BIL) format in a new directory
 *
 * Written by the Institute for the Environment at UNC, Chapel Hill
 * in support of the EPA NOAA CMAS Modeling, 2007-2008.
 * 
 * By Limei Ran, March 2008
 *
 * Usage: preProcessNLCD.exe
 *        For instance:
 *        	preProcessNLCD.exe nlcd_files.txt  "/nas/uncch/depts/cep/emc/lran/nlcd2001/nlcd/new/" pp_nlcd_files.txt
 *        	or
 *        	preProcessNLCD.exe
 *  
 *        Environment Variables needed without arguments: 
 *        INPUT_NLCDFILES_LIST -- File which contains all input NLCD images with directories and names.  
 *                     Only needed when program is called without arguments.
 *        DATADIR -- directory to store preprocessed images without overlapping.
 *        OUTPUT_NLCDFILES_LIST -- File which contains all output NLCD images with directories and names.
 *                     Only needed when program is called without arguments.
 */
#include <iostream>
#include <fstream>
#include "cpl_conv.h"
#include "cpl_string.h"
#include "gdal_priv.h"
#include <ogr_spatialref.h>
#include <vrtdataset.h>

#include "sa_raster.h"
#include "commontools.h"

static void Usage();
void  cleanOverlayCells ( std::vector<string> imageFiles, string fileType );

//define global variables
string                newDataDir;             //new directory created under the first USGS landuse image directory
string                landType = "USGS NLCD Landuse Files";
string                impeType = "USGS NLCD Urban Imperviousness Files";
string                canoType = "USGS NLCD Tree Canopy Files";
string                cgapType = "NOAA CGAP NLCD Landuse Files";
std::ofstream         n_imageFile;            //output processed image file list
OGRSpatialReference   oSRS_std;                     //set the first NLCD landuse image projection as the standard projection   
double                xCellSize_std, yCellSize_std; //set the cell size of the first NLCD landuse image as the standard size


/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main( int nArgc,  char* papszArgv[] )
{

    const int             argsN = 3;             //number of input arguments 
    string                dataFileList;          //file containing all original NLCD file names
    string                n_dataFileList;        //file containing all processed NLCD file names

    std::vector<string>   landFiles, impeFiles, canoFiles, cgapFiles; 
    std::ifstream         imageFile;             //input NLCD image file list  
    string                fileName, fileType;   //for reading in all NLCD files
 
    int                   i;                    //file index


    //print program version
    printf ("\nUsing: %s\n", prog_version);

    printf("\nPreprocessing NLCD images to create no-overlapping images... \n\n"); 
/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
    GDALAllRegister();
    
/* -------------------------------------------------------------------- */
/*      Obtain arguments and environment variables                      */
/* -------------------------------------------------------------------- */
    
    // obtain NLCD image file list text file
    //no arguments to call this program
    if ( nArgc  == 1 )
    {
       //printf( "Getting input text file with original NLCD image file list.\n");
       dataFileList = string( getEnviVariable("INPUT_NLCDFILES_LIST") );

       //printf( "Getting directory to store processed image files.\n");
       newDataDir = string( getEnviVariable("DATADIR") ); 
      
       //printf( "Getting output text file with processed NLCD image file list.\n"); 
       n_dataFileList = string( getEnviVariable("OUTPUT_NLCDFILES_LIST") );
    }
    else if( nArgc  == argsN + 1 )
    {
       dataFileList = string( papszArgv[argsN - 2] );
       newDataDir = string( papszArgv[argsN - 1] );
       n_dataFileList = string( papszArgv[argsN] );
    }
    else
    {
       Usage();
    } 

    dataFileList = trim( dataFileList ); 
    printf("Original NLCD image list file:  %s\n",dataFileList.c_str());
    FileExists(dataFileList.c_str(), 0 );  //the file has to exist.

    //make sure that dir ends with path separator
    newDataDir = trim (newDataDir);  
    newDataDir = processDirName ( newDataDir );
    printf( "Directory to store preprocessed NLCD images:  %s\n",newDataDir.c_str() );
    FileExists( newDataDir.c_str(), 2 );  //check the directory and create it if it is new 

    n_dataFileList = trim( n_dataFileList );
    printf("Preprocessed NLCD image list file:  %s\n",n_dataFileList.c_str());
    FileExists(n_dataFileList.c_str(), 1 );  //the file has to be new.
   
/* -------------------------------------------------------------------- */
/*     read in each NLCD images to get image vector for each data set  */
/* -------------------------------------------------------------------- */

    // loop through USGS 14 zone NLCD image data files to get all image extents
    i = 0;
    imageFile.open( dataFileList.c_str() );
    if (imageFile.good() )
    {
       getline( imageFile, fileName);
       
       while ( !imageFile.eof() )
       {
          fileName = trim (fileName);  //get rid of spaces at edges
          i++;  //count the line number

          //get rid of empty line
          if ( fileName.size() == 0 )
          {
             goto newloop; 
          }

          //get the type of files
          if ( fileName.find(landType) !=string::npos || fileName.find(impeType) !=string::npos 
               || fileName.find(canoType) !=string::npos || fileName.find(cgapType) !=string::npos )

          {
            fileType = fileName;
            printf( "\nline %d: %s\n",i,fileType.c_str() );
          }
          else
          {
             //put images into different vectors based on file type
             printf( "line %d: %s\n",i,fileName.c_str() );
             FileExists( fileName.c_str(), 0 );     //the file has to exist.
 
             if ( fileType.find(landType) !=string::npos )      
             {
                landFiles.push_back(fileName); 
             }
      
             if ( fileType.find(impeType) !=string::npos )
             {
                impeFiles.push_back(fileName);
             }

             if ( fileType.find(canoType) !=string::npos )
             {
                canoFiles.push_back(fileName);
             }

             if ( fileType.find(cgapType) !=string::npos )
             {
                cgapFiles.push_back(fileName);
             }
          }

          newloop: 
          getline( imageFile, fileName);
       }  // while loop

       imageFile.close(); 
    } // if good 
    else
    {     
       printf ("\tError: Open file - %s\n", dataFileList.c_str() );
       exit ( 1 );
    }   

   printf("Created a text file to store preprocessed NLCD image names:  %s\n",n_dataFileList.c_str());
   n_imageFile.open( n_dataFileList.c_str() );
   if (! n_imageFile.good() )
   { 
       printf( "Error in opening file: %s\n",n_dataFileList.c_str() );
       exit ( 1 );
   }

   //process each type of images
   printf( "\n\nNLCD Landuse\n");
   cleanOverlayCells( landFiles, landType );

   printf( "\n\nNLCD Urban Imperviousness\n");
   cleanOverlayCells( impeFiles, impeType );

   printf( "\n\nNLCD Tree Canopy\n");
   cleanOverlayCells( canoFiles, canoType );

   printf( "\n\nNLCD CGAP Landuse\n");
   cleanOverlayCells( cgapFiles, cgapType );

   n_imageFile.close();
   printf ("\n\nCompleted in proprocess all NLCD images.\n");

}
/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage()

{
    printf( "\nError in input arguments.\n");
    printf( "Usage: preProcessNLCD.exe nlcd_files.txt \"new_image_dir\"\n");
    exit( 1 );
}


/************************************************************************/
/*                  cleanOverlayCells ( vector<string> imageFiles )     */
/************************************************************************/

void   cleanOverlayCells ( std::vector<string> imageFiles, string fileType )  
{

    string                fileName;            //file to be processed
    string                newFileName,extStr;  //new files without overlapping = fileName_no.*
    string                pfileName;           //previous processed file name
    string                lineStr;             //a string line to write file
    string                cmd_str;             //command string for gdal_translate
    int                   i,j,k,nPixelCount;
   
    //GDAL related
    GDALDataset     *poRDataset, *p_poRDataset;
    GDALDriver      *poDrive;
    char            **papszFileList;
    GDALRasterBand  *poBand, *p_poBand;
    double          adfGeoTransform[6];
    double          xUL,yUL;
    int             xCells, yCells, p_xCells, p_yCells; //cells 
    double          xCellSize, yCellSize; //cell size
    std::vector<double>   extXMin,extXMax;       //x extent of images
    std::vector<double>   extYMin,extYMax;       //y extent of images
    double          xmin,xmax,ymin,ymax;         //current image extent
    double          intpart;
    GDALDriver      *p_poDrive;
    const char      *pszFormat = "EHdr";
     
    const char      *pszWKT = NULL;
    char            *pszWKT_nc = NULL;
    OGRSpatialReference oSRS;
    char            *pszProj4 = NULL;

   
    GByte           NoDataValue;                  //nodata value for i and j images


   printf( "Get rid of overlapping cells in images...\n" );

/* -------------------------------------------------------------------- */
/*      Write Data type to file with preprocessed file names   */
/* -------------------------------------------------------------------- */
    lineStr = fileType;       
    lineStr.append( ":\n" );
    n_imageFile.write( lineStr.c_str(), lineStr.size() );

/* -------------------------------------------------------------------- */
/*      Set NODATA value for each image type                            */
/* -------------------------------------------------------------------- */
   //set no data value for each type images
   if ( fileType.find(cgapType) !=string::npos )
   {
      //CGAP NLCD data have NODATA = 0 Unclassified = 1
      NoDataValue = 0;
   }
   else 
   {
      //USGS NLCD data have NODATA = 127
      NoDataValue = 127;
   }

/* -------------------------------------------------------------------- */
/*      Process one image at a time for the image vector                */
/* -------------------------------------------------------------------- */
   //loop through all images in the vector
   for ( i = 0; i<imageFiles.size(); i++)
   {
      fileName = imageFiles.at(i);
      printf( "\n%s\n",fileName.c_str() );
     
/* -------------------------------------------------------------------- */
/*      Open image i                                                    */
/* -------------------------------------------------------------------- */ 
      //open the current i image 
      poRDataset = (GDALDataset *) GDALOpen( fileName.c_str(), GA_ReadOnly );
      if( poRDataset == NULL )
      {
         printf( "Open raster file failed: %s.\n", fileName.c_str() );
         exit( 1 );
      }
/* -------------------------------------------------------------------- */
/*   get rows, columns, cell size, and image extent of image i in vector*/
/* -------------------------------------------------------------------- */
      // get rows and columns of the image
      xCells = poRDataset->GetRasterXSize();
      yCells = poRDataset->GetRasterYSize();
      printf( "Size is %dx%dx%d\n", xCells, yCells, poRDataset->GetRasterCount() );

      //get UL corner coordinates and grid size
      if( poRDataset->GetGeoTransform( adfGeoTransform ) == CE_None )
      {
         xUL = adfGeoTransform[0];
         yUL = adfGeoTransform[3];
         printf( "Origin = (%.6lf,%.6lf)\n", xUL,yUL );

         xCellSize =  adfGeoTransform[1] ;
         yCellSize =  -1 * adfGeoTransform[5] ;  // Make it positive.  Negative due to Origin is UL corner point
         printf( "Pixel Size = (%.5lf,%.5lf)\n", xCellSize, yCellSize );

         //check if UL corner coordinates are the standard NLCD grid corner
         if ( fabs( modf(xUL/xCellSize,&intpart) )!= 0.5 || fabs (modf(yUL/yCellSize,&intpart) ) != 0.5 )
         {
            printf ( "This image UL corner does not have the standard NLCD grid corner coordinates: modf(fabs(xUL/30))=0.5.\nRegrid it using gdal_translate utility.\n");  
            exit ( 1 );
         }

      }
 
      //compute extent of band 1 image (only one band for all images)
      xmin = xUL;
      xmax = xUL + xCellSize * xCells;
      extXMin.push_back ( xmin );
      extXMax.push_back ( xmax );

      ymax = yUL;
      ymin = yUL - yCellSize * yCells;
      extYMin.push_back( ymin );
      extYMax.push_back( ymax ); 
      printf( "extent:  min(%.3f,%.3f)   max(%.3f,%.3f)\n", xmin,ymin,xmax,ymax ); 

/* -------------------------------------------------------------------- */
/*   Get image projection to make sure that all images have the same    */
/*   Set the first USGS landuse image as the standard                   */
/* -------------------------------------------------------------------- */
     //get projection from the image
     if( (pszWKT = poRDataset->GetProjectionRef())  == NULL )
     {
        printf( "Projection is not defined in the NLCD file: %s.\n", fileName.c_str() );
        printf( "Can define it using gdal_translate utility.\n");
        exit( 1 );
     }

     //convert it to no const char
     pszWKT_nc =strdup ( pszWKT ); 
    
     // set the first NLCD landuse image's cell size and projection as standard projection
     if ( i == 0 && fileType.find(landType) !=string::npos ) 
     {
        xCellSize_std = xCellSize;
        yCellSize_std = yCellSize; 
      
        oSRS_std.importFromWkt( &pszWKT_nc );
        oSRS_std.exportToProj4( &pszProj4 );
        printf ( "Proj4 for the first NLCD landuse image = %s\n", pszProj4);
     }
     else  
     {
        //check consistent cell size
        if ( xCellSize_std != xCellSize || yCellSize_std != yCellSize )
        {
           printf( "This image's cell size is different from the first NLCD landuse image.\n" );
           printf( "Resmaple it using gdal_translate utility to the cell size of the first NLCD landuse image.\n" );
           exit ( 1 );
        }
        else
        {
           printf ( "The same cell size as the first NLCD landuse image's\n" );
        }

        oSRS.importFromWkt( &pszWKT_nc );
        oSRS.exportToProj4( &pszProj4 ); 
        
        //make sure that the image has the same projection as the first image's
        if (! oSRS.IsSame (&oSRS_std) )
        {
           printf( "This image's projection is different from the first NLCD landuse image's.\n" );
           printf( "Project it using gdalwarp utility to the standard projection from the first NLCD landuse image.\n" );
           printf ( "Proj4 from this image = %s\n", pszProj4);
           exit ( 1 );
        }
        else
        {
           printf ( "The same projection as the first NLCD landuse image's\n" );
        }
     }
/* -------------------------------------------------------------------- */
/*   Image data type has to be GByte                                    */
/* -------------------------------------------------------------------- */
     //get the band 1 image from the current image and make sure that data are GDT_Byte
     poBand = poRDataset->GetRasterBand( 1 );  // band 1
     GDALDataType poBandType = poBand->GetRasterDataType();
     printf ("Image Type = %d\n",poBandType);
     if ( poBandType !=GDT_Byte )
     {
         printf( "Image data type is not GDT_Byte: %s.\n", fileName.c_str() );    
         exit ( 1 );
     }  
     GDALClose( (GDALDatasetH) poRDataset );  //finished check and obtained all information from image i
     poRDataset = NULL;

/* -------------------------------------------------------------------- */
/*   Create new no-overlaypping image name from image i in new data dir */
/*   New images will be in EHdr -- ESRI .hdr Labelled format            */
/* -------------------------------------------------------------------- */
     newFileName = fileName;
     k = newFileName.rfind(".", newFileName.length());
     extStr = fileName.substr(k);     //get the image extention
     newFileName.erase(k);            //get image path and name

     //just get image name
     if ( (k=newFileName.find_last_of("/\\")) == string::npos )
     {
        printf( "Error in extract file name from: %s\n",newFileName.c_str() );
        exit (1 );
     }
     newFileName = newFileName.substr(k+1);
     newFileName = newDataDir + newFileName;
     newFileName.append(".bil");   //ESRI BIL format
     //newFileName.append("_no");
     //newFileName.append(extStr);
     printf( "No-overlapping file: %s\n",newFileName.c_str() );

     //update imageFiles vector at i to the new file name
     imageFiles.at(i) = newFileName;

     //write new name to the new text output file
     lineStr = newFileName;
     lineStr.append( "\n" );
     n_imageFile.write( lineStr.c_str(), lineStr.size() );

     // ckeck whether the new file exists, if it exists, delete it manually
     struct stat stFileInfo;
     int intStat;
     intStat = stat(newFileName.c_str(),&stFileInfo);
     if(intStat == 0) 
     {
         //exist -- delete it
         //open image
         poRDataset = (GDALDataset *) GDALOpen( newFileName.c_str(), GA_ReadOnly );
         if( poRDataset == NULL )
         {
           printf( "Open raster file failed: %s.\n", newFileName.c_str() );
           exit( 1 );
         }
         poDrive = poRDataset->GetDriver();
         GDALClose( (GDALDatasetH) poRDataset );
         poRDataset = NULL;
 
         if ( poDrive->Delete( newFileName.c_str() )  == CE_Failure )
         {
            printf( "Deleting the file failed: %s\n", newFileName.c_str() );
            exit( 1 );
         } 
         printf( "Deleted the file: %s\n", newFileName.c_str() );
     }

     //open the current i image and make a EHdr copy to store no-overlapping data
     poRDataset = (GDALDataset *) GDALOpen( fileName.c_str(), GA_ReadOnly );
     if( poRDataset == NULL )
     {
         printf( "Open raster file failed: %s.\n", fileName.c_str() );
         exit( 1 );
     }
     p_poDrive = GetGDALDriverManager()->GetDriverByName(pszFormat);  //drive for EHdr
     if ( (p_poRDataset = p_poDrive->CreateCopy(newFileName.c_str(), poRDataset, 
                                              TRUE,NULL,GDALDummyProgress,NULL) )  == NULL )
     {
         printf( "Making a copy of the file failed: %s to %s\n", fileName.c_str(),newFileName.c_str() );
         exit( 1 );
     }
     GDALClose( (GDALDatasetH) p_poRDataset );  //close due to may readable only
     p_poRDataset = NULL;
     GDALClose( (GDALDatasetH) poRDataset );
     poRDataset = NULL;

/* -------------------------------------------------------------------- */
/*   Open the copied EHdr image i for updating                          */
/* -------------------------------------------------------------------- */
     poRDataset = (GDALDataset *) GDALOpen( newFileName.c_str(), GA_Update );
     if( poRDataset == NULL )
     {
         printf( "Open raster file failed: %s.\n", newFileName.c_str() );
         exit( 1 );
     }
    
/* -------------------------------------------------------------------- */
/*   Check each previous i-1 images to get rid of overlapping in image i*/
/* -------------------------------------------------------------------- */
     for ( j=0; j<i; j++)
     {
        //get previous image j
        pfileName = imageFiles.at(j);
        printf( "\n    Previous File Names: %s\n",pfileName.c_str() );
/* -------------------------------------------------------------------- */
/*   Open previous processed EHdr image j and get rows and columns      */
/* -------------------------------------------------------------------- */        
        //open previous j image
        p_poRDataset = (GDALDataset *) GDALOpen( pfileName.c_str(), GA_ReadOnly );
        if( p_poRDataset == NULL )
        {
           printf( "Open raster file failed: %s.\n", pfileName.c_str() );
           exit( 1 );
        } 

        // get rows and columns of the image
        p_xCells = p_poRDataset->GetRasterXSize();
        p_yCells = p_poRDataset->GetRasterYSize();
        printf( "Pixel Size = %dx%d\n", p_xCells, p_yCells ); 
        printf( "extent:  min(%.3f,%.3f)   max(%.3f,%.3f)\n", extXMin.at(j),extYMin.at(j),extXMax.at(j),extYMax.at(j) ); 

/* -------------------------------------------------------------------- */
/*   Compute overlapping box indexes in both image i and j              */
/* -------------------------------------------------------------------- */
        //check whether those two images are overlaying and calculate row and col in overlay areas from both images
        int c_row1 = 0, c_row2 = 0, c_col1=0, c_col2=0;
        int c_nXSize = 0 ,c_nYSize = 0;     //overlaypping array size for image i -- current
        int p_row1 = 0, p_row2 = 0, p_col1=0, p_col2=0;
        int p_nXSize = 0 ,p_nYSize = 0;     //overlaypping array size for image j -- previous

        //if the two images intersecting
        if (! (xmin >= extXMax.at(j) || xmax <= extXMin.at(j) ||  ymin >= extYMax.at(j) || ymax <= extYMin.at(j)) )
        { 
           //Row and column start from LU corner
           //calculate intersection UL columns
           if ( xmin>extXMin.at(j) )
           {
              c_col1 = 1;
              p_col1 =  (int) ( (xmin - extXMin.at(j)) / xCellSize_std + 1);
           }
           if ( xmin<extXMin.at(j) )
           {
              c_col1 = (int) ( (extXMin.at(j) - xmin) / xCellSize_std  + 1);
              p_col1 = 1;
           }
           if (xmin==extXMin.at(j) )
           {
              c_col1 = 1;
              p_col1 = 1;  
           }

           //calculate intersection LR columns
           if ( xmax<extXMax.at(j) )
           {
              c_col2 = xCells;
              p_col2 = (int) ( (xmax - extXMin.at(j)) / xCellSize_std );
           }
           if ( xmax>extXMax.at(j) )
           {
              c_col2 = (int) ( (extXMax.at(j) - xmin) / xCellSize_std );
              p_col2 = p_xCells;
           }
           if ( xmax==extXMax.at(j) )   
           {
              c_col2 = xCells; 
              p_col2 = p_xCells; 
           }
     
           //calculate intersection LR rows 
           if ( ymin>extYMin.at(j) )
           {
              c_row2 = yCells;
              p_row2 = (int) ( (extYMax.at(j) - ymin) / yCellSize_std ); 
           }
           if ( ymin<extYMin.at(j) )
           {
              c_row2 = (int) ( (ymax - extYMin.at(j)) / yCellSize_std );
              p_row2 = p_yCells;
           }
           if ( ymin==extYMin.at(j) )
           {
              c_row2 = yCells;
              p_row2 = p_yCells;
           }

           //calculate intersection UL row
           if ( ymax<extYMax.at(j) )
           {
              c_row1 = 1;
              p_row1 = (int) ( (extYMax.at(j) - ymax) / yCellSize_std + 1); 
           }
           if ( ymax>extYMax.at(j) )
           {
              c_row1 = (int) ( (ymax - extYMax.at(j)) / yCellSize_std + 1);
              p_row1 = 1;
           }
           if ( ymax==extYMax.at(j) )
           {
              c_row1 = 1;
              p_row1 = 1; 
           }

           //compute x and y cells, min and max for QA checking in current image i
           printf ("i=%d image intesection: (col=%d row=%d)  (col=%d row=%d)\n",i,c_col1,c_row1,c_col2,c_row2);
           c_nXSize = c_col2 - c_col1 + 1;
           c_nYSize = c_row2 - c_row1 + 1; 
           printf ("\t\tnXSizei = %d  nYSize = %d\n",c_nXSize, c_nYSize);
           double c_oXmin = xmin + (c_col1 - 1) * xCellSize_std;
           double c_oXmax = xmin + c_col2 * xCellSize_std;
           double c_oYmax = ymax - (c_row1 - 1) * yCellSize_std;
           double c_oYmin = ymax - c_row2 * yCellSize_std;
           printf("Overlapping box: x: %lf - %lf    y: %lf - %lf\n",c_oXmin,c_oXmax,c_oYmin,c_oYmax); 

           //compute x and y cells, min and max for QA checking in previous image j
           printf ("j=%d image intesection: (col=%d row=%d)  (col=%d row=%d)\n",j,p_col1,p_row1,p_col2,p_row2);
           p_nXSize = p_col2 - p_col1 + 1;
           p_nYSize = p_row2 - p_row1 + 1;
           printf ("\t\tnXSize = %d  nYSize = %d\n",p_nXSize, p_nYSize);
           double p_oXmin = extXMin.at(j) + (p_col1 - 1) * xCellSize_std;
           double p_oXmax = extXMin.at(j) + p_col2 * xCellSize_std;
           double p_oYmax = extYMax.at(j) - (p_row1 - 1) * yCellSize_std;
           double p_oYmin = extYMax.at(j) - p_row2 * yCellSize_std;
           printf("Overlapping box: x: %lf - %lf    y: %lf - %lf\n",p_oXmin,p_oXmax,p_oYmin,p_oYmax);

           //make sure that overlapping box are the same in both image i and j
           if (c_nXSize != p_nXSize || c_nYSize != p_nYSize)
           {
              printf( "Error: overlapping images x and y sizes are different for images i and j.\n" );
              exit ( 1 );
           }
           if (c_oXmin != p_oXmin || c_oXmax != p_oXmax || c_oYmax != p_oYmax || c_oYmin != p_oYmin)
           {
              printf( "Error: overlapping box x and y are different for images i and j.\n" );
              exit ( 1 );
           }
 
/* -------------------------------------------------------------------- */
/*         get the band 1 image from current image i                */
/* -------------------------------------------------------------------- */
           poBand = poRDataset->GetRasterBand( 1 );  // band 1
           GByte *poImage = (GByte *) CPLCalloc(sizeof(GByte),c_nXSize*c_nYSize);
           if ( (poBand->RasterIO(GF_Read, c_col1-1,c_row1-1,c_nXSize,c_nYSize,
                 poImage,c_nXSize,c_nYSize,GDT_Byte,0,0)) == CE_Failure) 
           {
              printf( "Error: reading band 1 data error from image i: %s.\n", newFileName.c_str() );
              CPLFree (poImage);
              exit( 1 );
           }
           
           /*
           printf ( "Row=%d Col=%d\n", c_row1,c_col1);
           double x = -2185199.6;
           double y = 2370598.66;
           int oCol = (int) (floor ((x - c_oXmin) / xCellSize_std) + 1);
           int oRow = (int) (floor ((c_oYmax - y) / yCellSize_std) + 1);
           k = (oRow - 1) * c_nXSize + oCol - 1;
           printf("col=%d   row=%d   value=%d\n",oCol,oRow,(int) poImage[k]);
           k = k -1;
           printf("Left   value=%d\n",(int) poImage[k]);
           k = k + 2;
           printf("Right   value=%d\n",(int) poImage[k]);
           k = k - 1 - c_nXSize;
           printf("UP   value=%d\n",(int) poImage[k]);
           k = k + 2*c_nXSize;
           printf("Down   value=%d\n",(int) poImage[k]);
           */
           
/* -------------------------------------------------------------------- */
/*         get the band 1 image from previous image j                   */
/* -------------------------------------------------------------------- */
           p_poBand = p_poRDataset->GetRasterBand( 1 );  // band 1
           GByte *p_poImage = (GByte *) CPLCalloc(sizeof(GByte),p_nXSize*p_nYSize);
           if ( (p_poBand->RasterIO(GF_Read, p_col1-1,p_row1-1,p_nXSize,p_nYSize,
                 p_poImage,p_nXSize,p_nYSize,GDT_Byte,0,0)) == CE_Failure)
           {
              printf( "Error: reading band 1 data error from image j: %s.\n", pfileName.c_str() );
              CPLFree (p_poImage);
              exit( 1 );
           }

           /*
           printf ( "Row=%d Col=%d\n", p_row1,p_col1);
           double x = -2022780.8;
           double y = 2266528.93;
           int oCol = (int) (floor ((x - oXmin) / xCellSize_std) + 1);   
           int oRow = (int) (floor ((oYmax - y) / yCellSize_std) + 1);
           k = (oRow - 1) * p_nXSize + oCol - 1;
           printf("col=%d   row=%d   value=%d\n",oCol,oRow,(int) p_poImage[k]); 
           k = k -1;
           printf("Left   value=%d\n",(int) p_poImage[k]);
           k = k + 2;
           printf("Right   value=%d\n",(int) p_poImage[k]);
           k = k - 1 - p_nXSize;
           printf("UP   value=%d\n",(int) p_poImage[k]);
           k = k + 2*p_nXSize;
           printf("Down   value=%d\n",(int) p_poImage[k]);
           */

/* -------------------------------------------------------------------- */
/*         Set image i cells with values in previous image j to NODATA  */
/* -------------------------------------------------------------------- */
           nPixelCount = c_nXSize * c_nYSize;   //total cells in overlapping box
           for (k=0; k<nPixelCount; k++)
           {
               if (p_poImage[k] != NoDataValue)
               {
                  poImage[k] = NoDataValue;    
               }
           }
           
/* -------------------------------------------------------------------- */
/*         write update box array back to current image i band          */
/* -------------------------------------------------------------------- */
           if ( (poBand->RasterIO(GF_Write, c_col1-1,c_row1-1,c_nXSize,c_nYSize,
                 poImage,c_nXSize,c_nYSize,GDT_Byte,0,0)) == CE_Failure)
           {  
              printf( "Error: Writing band 1 data error to image i: %s.\n", newFileName.c_str() );
              CPLFree (poImage);
              exit( 1 );
           }
      
           CPLFree (poImage);
           CPLFree (p_poImage); 

        }  //end of overlapping processing
        else
        {
           printf ("j=%d image does not intersect.\n",j);
        }
     
        GDALClose( (GDALDatasetH) p_poRDataset );
     } //end of j

    poRDataset->FlushCache();
    GDALClose( (GDALDatasetH) poRDataset );

   }  //end of i

   printf ("Finished preprocessing images: %s\n",fileType.c_str());
}  //end of the function

