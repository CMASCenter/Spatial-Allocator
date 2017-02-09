/*************************************************************************
* This program converts WRF-ready NetCDF GOES file into the data         * 
* assimilation format.  It copies the original NetCDF file and add       *
* additional variable for each 3-d variable with the same value          *
* but one time step ahead.                                               *
*                                                                        *
* Written by the Institute for the Environment at UNC, Chapel Hill in    *
* support of the EPA NOAA CMAS Modeling, 2008-2009.                      *
*                                                                        *
* Build: Adjust settings in Makefile, then 'make'                        *
* Run:   1. toDataAssimilationFMT.exe  input_goes_file output_goes_file  *
*        or                                                              *
*        2. toDataAssimilationFMT.exe                                    *
*                                                                        *
*        Environment Variables needed when running without arguments:    *
*        INPUT_WRF_GOES_FILENAME                                         *
*        OUTPUT_WRF_NETCDF_FILENAME                                      *
*                                                                        *
* Revision history:                                                      *
* Date        Programmer                   Description of change         *
* ----        ----------                   ---------------------         *
* April 2009    Limei Ran, UNC-IE                                        *
*************************************************************************/
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "sa_raster.h"
#include "commontools.h"
#include "geotools.h"

//gloabl variables
static void Usage();

/***************************
*********** MAIN  **********
***************************/
int main(int nArgc, char *Argv[])
{
	/*****************
	* File variables *
	*****************/
        int argsN = 2;   //two arguments to run the program
        char cmd[255];
	const char *inputFile;
	const char *outputFile;

        /***********************************
        * NetCDF variable array pointers   *
        ************************************/
        float      *x, *y;      //1d array  time*cols  time*rows
        float      *lat, *lon;  //2d array  time*rows*cols
        char       *goesStr;    //char array     
        float      *goesV, *goesVold, *goesVnew;     //float array

        int i,j,k;

        //print program version
        printf ("\nUsing: %s\n", prog_version);
	/*********************************
	* Obtain environmental variables *
	*********************************/
        if( nArgc == argsN+1 )
        {   
           inputFile = Argv[1];
           outputFile = Argv[2];
        }
        else if (nArgc == 1)
        {  
           inputFile = getEnviVariable( "INPUT_MM5_GOES_FILENAME" );
           outputFile = getEnviVariable( "OUTPUT_WRF_NETCDF_FILENAME" );
        }
        else
        {     
           printf("\nError in input arguments.  Should have %d arguments.\n", argsN);
           Usage();
           exit( -nArgc );
        }


        printf( "Input MM5 Netcdf file name: %s\n",inputFile );
        FileExists(inputFile, 0 );  //the file has to exist.

        printf( "Output NetCDF file name: %s\n",outputFile );
        FileExists(outputFile, 3 );  //the file has to be new.

        //copy input file to output file
        printf( "Copying file: %s to %s\n", inputFile, outputFile);
        sprintf(cmd,"cp -f  %s   %s",inputFile, outputFile);
        system(cmd); 
        

	/***********************************************
        * Read WRF netcdf GOES file                    *
	* Write WRF landuse data file in netCDF format *
	***********************************************/

	/*****************************
	* Open input and output files*
	******************************/
        int ncid, ncid_in;
        
        printf("Reading input file '%s'...\n", inputFile);
        printf("Writing output file '%s'...\n", outputFile);

        anyErrors( nc_open(inputFile, NC_NOWRITE, &ncid_in) );   //open input file
        anyErrors( nc_open(outputFile, NC_WRITE, &ncid) );   //open output file
 

        /*******************************************
        * Define general diemntions of input file  *
        ********************************************/
        int ndims_in, nvars_in, ngatts_in, unlimdimid_in; 


	/********************
	* Define dimensions *
	********************/
        size_t *dimSizes;
        size_t dimSize;
 
        char dimName[NC_MAX_NAME+1];
        
        /*******************
        * Define variables *
        *******************/
        int varID_out_new;
        char attName[NC_MAX_NAME+1];       //name for an attribute


        printf( "Obtaining all dimension IDs in input WRF NetCDF file...\n" );

        anyErrors( nc_inq(ncid_in, &ndims_in, &nvars_in, &ngatts_in, &unlimdimid_in) );
 
        printf("\tInput file %s has: %d dims, %d variables, %d global attributes, %d unlimited variable\n", inputFile, ndims_in, nvars_in,ngatts_in,unlimdimid_in);

        //store dim size in an arrary
        dimSizes = (size_t *) malloc(sizeof(size_t) * ndims_in);
        if ( dimSizes == NULL )
        { 
             printf ( "\t Memory allocation malloc failed for dimSizes\n" );
             exit ( 1 );
        }

        for (i=0; i<ndims_in; i++)
        {
           anyErrors(  nc_inq_dim(ncid_in, i, dimName, &dimSize ) );
           dimSizes[i]= dimSize;
           printf("\t Input dimension:  dimName = %s dimSize=%d  dimID=%d \n",dimName,dimSize,i);
        }

        /*****************************
        * Define variables for output*
        ******************************/ 
        char       var_name[NC_MAX_NAME+1];               //variable name
        char       var_name_new[NC_MAX_NAME+1];           //new variable name
        nc_type    var_type;                              // variable type 
        int        var_ndims;                             // number of dims 
        int        var_dimids[NC_MAX_VAR_DIMS];           // dimension IDs for read in array
        int        var_dimid;
        int        var_natts;                             // number of attributes 
        int        totalSize;                             //total dim size
        int        fieldtype[1];
        int        k1,k2,kk,kkOld;                        //dimension indexes


        printf( "Reading input variables, shfiting 1 time step ahead, and adding it to output NetCDF file...\n" );
 
        /************************************
        * Read each variable                *
        * Define new variable and attributes*
        * Write the variable in output      *
        *************************************/

        fieldtype[0] = 104;
        for (i=0; i<nvars_in; i++)
        {
           //get an input var infor
           anyErrors( nc_inq_var( ncid_in, i, var_name, &var_type, &var_ndims, var_dimids, &var_natts) ); 
           printf( "Variable Name = %s  var_type = %d  ndims=%d\n",var_name, var_type, var_ndims );

           totalSize = 1;
           for (j=0; j<var_ndims; j++)
           {
              //total dim size
              var_dimid = var_dimids[j];
              totalSize = totalSize * dimSizes[var_dimid];
           }

           if ( var_ndims == 3 )
           { 
              
              anyErrors( nc_redef (ncid) );

              //define the new varaible with one time step ahead
              sprintf( var_name_new, "%s_NEW\0",var_name );
              anyErrors( nc_def_var(ncid, var_name_new, var_type, var_ndims, var_dimids, &varID_out_new) );


              //copy variable attributes
              for ( k=0; k<var_natts; k++ )
              {
                 anyErrors( nc_inq_attname( ncid_in, i, k, attName) );

                 printf( "\tCopy variable attributes: %s\n",var_name_new);
                 anyErrors( nc_copy_att (ncid_in, i, attName, ncid, varID_out_new ) );
              }

              /************************
              * obtain float varaible *
              ************************/
              //allocate mem for a GOES variable
              if ( (goesV = (float*) calloc (totalSize, sizeof(float)) ) == NULL)
              {
                  printf( "Calloc goesV failed.\n");
                  exit ( 1 );
              }
                 
              //get variable value
              anyErrors( nc_get_var_float(ncid_in, i, goesV) );

              printf ("Variable name: %s    Total size = %d\n", var_name_new, totalSize); 

               /****************************************
                * Create new variable and new array    *
                * with one time step ahead             *
                ****************************************/
               //get WRF format x and y dimension
               var_dimid = var_dimids[0];
               int timeDim = dimSizes [var_dimid];

               var_dimid = var_dimids[1];
               int yRows = dimSizes [var_dimid];

               var_dimid = var_dimids[2];
               int xCols = dimSizes [var_dimid];

               printf( "\tTime= %d   yRows = %d   xCols = %d\n",timeDim, yRows, xCols);

               for (k=0; k<timeDim - 1;  k++)
               {
                   for (k1=0; k1<yRows; k1++)
                   {
                       for (k2=0; k2<xCols; k2++)
                       {
                                kk = k * yRows * xCols + k1 * xCols + k2;   // new index
                                kkOld = (k+1) * yRows * xCols + k1 * xCols + k2;  //one step ahead index
                                goesV[kk] = goesV[kkOld];   //move a step head
                           
                        }  //xCols
                   } //yRows
                } //k    


                /*******************************
                * Leave define mode for output *
                ********************************/
                anyErrors( nc_enddef (ncid) );


                /****************************
                 * Write new float varaible *
                 ****************************/
                //write new variable if the Time is the first dimension
                anyErrors( nc_put_var_float(ncid, varID_out_new, goesV) ); 

                printf( "\tWrited variable: %s\n",var_name_new );

                nc_sync (ncid);
                free( goesV );

            }  // var_ndims = 3
         
        }  // i

        
	/********************
	* Close netCDF file *
	********************/
        anyErrors( nc_close(ncid_in) );
        anyErrors( nc_close(ncid) );


        
        printf( "\nFinished writing output WRF NetCDF file: %s\n\n", outputFile);
}



/****    End of Main program   ***/
/**************************************************************************************/


/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage()

{
    printf( "\nError in input arguments.\n");
    printf( "Usage: toDataAssimilationFMT.exe  input_WRF_NetCDFfile output_WRF_NetCDFfile\n");
    exit( 1 );
}
