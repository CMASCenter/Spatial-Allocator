/********************************************************************
 * This program is used to create a grid polygon shapefile with user's specifiied
 * modeling grid domain and projection using GDAL library.
 *
 * Written by the Institute for the Environment at UNC, Chapel Hill
 * in support of the EPA NOAA CMAS Modeling, 2007-2008.
 * 
 * By Limei Ran, Feb, 2008
 * Mofidied by L. Ran, June 2009
 *
 * Usage: create_gridPolygon "PROJ4 project_definitation" xmin ymin xcell_size ycell_size row col out_shapefile
 *        For instance:
 *        create_gridPolygon.exe "+proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=33 +lat_2=45 +lat_0=40 +lon_0=-97" -1080000.0 -1692000.0  12000 12000 250 289 GRIDID wrf12km.shp
 *        or
 *        create_gridPolygon.exe
 *
 *        Environment Variables needed without arguments:  
 *            GRID_PROJ -- PROJ4 projection definition 
 *            GRID_XMIN -- grid x min
 *            GRID_YMIN -- grid y min
 *            GRID_XCELLSIZE -- grid x cell size
 *            GRID_YCELLSIZE -- grid y cell sixe
 *            GRID_ROWS -- grid rows
 *            GRID_COLUMNS -- grid columns
 *            POLYGON_ID -- grid shape ID name (optional, default is set to GRIDID)
 *            GRID_SHAPEFILE_NAME -- output grid shapefile name
 */
#include <dirent.h>
#include "sa_raster.h"
#include "geotools.h"
#include "commontools.h"


static void Usage();

/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main( int nArgc,  char* papszArgv[] )
{
    const int   argsN = 9;  //number of input arguments required
    string      shapeFile;
    gridInfo    grid;            //data structure to store a modeling domain information


    //print program version
    printf ("\nUsing: %s\n", prog_version);

/* -------------------------------------------------------------------- */
/*      Processing command line arguments.                              */
/* -------------------------------------------------------------------- */
   
    printf("\nSetting variables to run the program:\n\n"); 
    if( nArgc == argsN + 1)
    {
      for( int iArg = 1; iArg < nArgc; iArg++ )
      {
        if( iArg  == 1 )
        {
            grid.strProj4 = papszArgv[iArg];
        }
        else if( iArg == 2  )
        {
            grid.xmin = atof( papszArgv[iArg] );
        }
        else if( iArg == 3) 
        {
            grid.ymin = atof( papszArgv[iArg] );
        }
        else if( iArg == 4 )
        {
           grid.xCellSize = atof( papszArgv[iArg] );
        }
        else if( iArg == 5 )
        {
            grid.yCellSize = atof( papszArgv[iArg] );
        }
        else if( iArg == 6 )
        {
            grid.rows = atoi( papszArgv[iArg] );
        }
        else if( iArg == 7 )
        {
           grid.cols = atoi( papszArgv[iArg] ); 
        }
        else if (iArg == 8 )
        {
           grid.polyID = string ( papszArgv[iArg] );
        }
        else
        {
           grid.name = string ( papszArgv[iArg] );
        }
      }
    }
    else if ( nArgc == 1)
    {
       grid.strProj4  = getEnviVariable("GRID_PROJ");
       grid.xmin = atof( getEnviVariable("GRID_XMIN") );
       grid.ymin = atof( getEnviVariable("GRID_YMIN") );
       grid.xCellSize = atof( getEnviVariable("GRID_XCELLSIZE") );
       grid.yCellSize = atof( getEnviVariable("GRID_YCELLSIZE") );
       grid.rows = atoi( getEnviVariable("GRID_ROWS") );
       grid.cols = atoi( getEnviVariable("GRID_COLUMNS") );

       //get polygon ID item in Shapefile
       if (getenv ( "POLYGON_ID" ) != NULL)
       {
          grid.polyID = string ( getEnviVariable( "POLYGON_ID" ) );
       }
       else
       {
          //created Shapefile will use GRIDID item
          grid.polyID = string ( "GRIDID" );   //created domain Shapefile uses GRIDID
       }
       grid.name = string ( getEnviVariable("GRID_SHAPEFILE_NAME") );
    }
    else
    {
        printf("\nError in input arguments.  Missing %d args.\n",argsN + 1 - nArgc);
        Usage();
        exit( -nArgc );
    }

    printf("\tPROJ = %s\n",grid.strProj4);
    printf("\tDomain Lower Left Corner:  xmin = %.2lf  ymin = %.2lf\n",grid.xmin,grid.ymin);
    printf("\tDomain Grid Size:  xCellsize = %.2lf  yCellsize = %.2lf\n",grid.xCellSize,grid.yCellSize);
    printf("\tDomain Gird Number:  rows = %d  cols = %d\n",grid.rows,grid.cols);
    printf( "\tShapefile polygon ID item: %s\n",grid.polyID.c_str() );
    printf("\tOutput Shapefile name: %s\n",grid.name.c_str());

   //Error checking grid cell size
   if (grid.xCellSize <= 0 || grid.yCellSize <= 0)
   {
      printf( "\tError: cell size has to > 0\n" );
     exit ( 1 );
   }

   //Error checking rows and columns
   if ( grid.rows < 1 || grid.cols < 1 )
   {
      printf( "\tError: rows and columns have to >= 1\n");
      exit ( 1 );
   }

/*********************************/
/*      Create Shapefile         */
/*********************************/
    shapeFile = createGridShapes ( grid );

    printf ("Completed in creating the grid shapefile.\n\n");
}

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage()

{
    printf( "\nError in input arguments.\n");
    printf( "Usage: create_gridPolygon \"PROJ4 project_definitation\" xmin ymin xcell_size ycell_size row col polyID out_shapefile\n");
    exit( 1 );
}

