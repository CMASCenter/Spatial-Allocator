/*************************************************************************
* This program converts WRF gridded landuse text files to netCDF format. *
*                                                                        *
* Written by the Institute for the Environment at UNC, Chapel Hill in    *
* support of the EPA NOAA CMAS Modeling, 2007-2008.                      *
*                                                                        *
* Build: Adjust settings in Makefile, then 'make'                        *
* Run:   txt2ncf.exe                                                     *
* Environment Variables need:                                            *
*		INPUT_LANDUSE_TEXT_FILENAME                              *
*		OUTPUT_LANDUSE_NETCDF_FILENAME                           *
*		GRID_ROWS                                                *
*		GRID_COLUMNS                                             *
*		GRID_XMIN                                                *
*		GRID_YMIN                                                *
*		GRID_XCELLSIZE                                           *
*		GRID_YCELLSIZE                                           *
*		GRID_PROJ                                                *
*		POLE_LATITUDE                                            *
*		POLE_LONGITUDE                                           *
*                                                                        *
* Revision history:                                                      *
* Date        Programmer                   Description of change         *
* ----        ----------                   ---------------------         *
* 05/31/08    Craig A. Mattocks, UNC-IE    Wrote original code           *
* 06/2008     L. Ran, UNC IE               Modified
*************************************************************************/
#include <iostream>
#include <fstream>
#include <sstream>

#include "sa_raster.h"
#include "commontools.h"
#include "geotools.h"

#define TIMES 1

void tokenize   (const string &str, vector<string> &tokens, const string &delimiters);


//gloabl variables

bool INCLUDE_IMPERV = false;   // IMPERV in data set indicator
bool INCLUDE_CANOPY = false;   // CANOPY in data set indicator
bool INCLUDE_LANDUSE = false;  // LANDUSE in data set indicator NLCD or MODIS

int land_cat_len = 0;          //landuse items


/***************************
*********** MAIN  **********
***************************/
int main(int argc, char *argv[])
{
	/*****************
	* File variables *
	*****************/
	const char *inputFile;
	const char *outputFile;

	/*****************
	* Grid variables *
	*****************/
	int rows, cols;
	float x0, y0;
	float dx, dy;
        //domain extent in lat and long
        float cLat, cLon;
        float poleLat, poleLon;
        float swLat,nwLat, neLat,seLat, swLon,nwLon, neLon,seLon;
        double xmax,ymax,xx,yy;

	/***************************
	* Map projection variables *
	***************************/
        projPJ proj4DF;
        projUV  xyP, latlongP;
        char *pszProj4;
        string projInfo;
        int proj;     //WRF projection type
        float stdLon,stdLat;
        float trueLat1, trueLat2;
        //temp vars
        string temp_proj;
        int pos1,pos2;

        /***********************************
        * NetCDF variable array pointers   *
        ************************************/
        char       *dateStr;    //char array
        float      *x, *y;      //2d array  time*cols  time*rows
        float      *lat, *lon;  //3d array  time*rows*cols
        int        *lu_list;    //landuse class list times*classes
        float      *imperv;     //3d    time*rows*cols
        float      *canopy;     //3d    time*rows*cols
        float      *luf;        //landuse fraction array 4d  time*classes*rows*cols
        float        *lum;        //landuse mask 3d array  time*rows*cols
        float        *lud;        //landuse dominant class 3d  time*rows*cols

        int i,j,k;

        //print program version
        printf ("\nUsing: %s\n", prog_version);
	/*********************************
	* Obtain environmental variables *
	*********************************/
        //get input and output files
        inputFile = getEnviVariable( "INPUT_LANDUSE_TEXT_FILENAME" );
        outputFile = getEnviVariable( "OUTPUT_LANDUSE_NETCDF_FILENAME" );

        printf( "Input text file name: %s\n",inputFile );
        FileExists(inputFile, 0 );  //the file has to exist.

        printf( "Output NetCDF file name: %s\n",outputFile );
        FileExists(outputFile, 3 );  //the file has to be new.

        //rows and columns of the domain
        rows = atoi( getEnviVariable("GRID_ROWS") );
        cols = atoi( getEnviVariable("GRID_COLUMNS") );
        printf( "Rows=%d    Cols=%d \n",rows,cols);

        //get xmin and ymin for the domain
        x0 = (float) atof( getEnviVariable("GRID_XMIN") );
        y0 = (float) atof( getEnviVariable("GRID_YMIN") );
        printf( "xmin=%f    ymin=%f \n", x0, y0);

        //get x and y cell size
        dx = (float) atof( getEnviVariable("GRID_XCELLSIZE") );
        dy = (float) atof( getEnviVariable("GRID_YCELLSIZE") );
        printf( "xcell=%.0f    ycell=%.0f \n",dx,dy);

        //get projection info
        pszProj4 = getEnviVariable("GRID_PROJ");
        printf( "proj4=%s\n",pszProj4);

        poleLon = (float) atof( getEnviVariable("POLE_LONGITUDE") );
        poleLat = (float) atof( getEnviVariable("POLE_LATITUDE") );
        printf( "Polar point: longitude=%f   latitude =%f \n",poleLon,poleLat);

        //get projection parameters from Proj4 string
        temp_proj = string (pszProj4);
        proj = getProjType(temp_proj);
        printf( "proj type = %d \n",proj);
        trueLat1 = getValueInProj4Str ( temp_proj, "lat_1=");
        trueLat2 = getValueInProj4Str ( temp_proj, "lat_2=");
        stdLat = getValueInProj4Str ( temp_proj, "lat_0=");
        stdLon = getValueInProj4Str ( temp_proj, "lon_0=");

       
        /*****************************************************
        * Compute center and domain corner lat and long data *
        ******************************************************/ 
        string proj4Str = string(pszProj4);
        proj4DF = pj_init_plus (proj4Str.c_str() );
        if (proj4DF == NULL)
        {
            printf( "Initializing Proj4 projection string failed: .\n",pszProj4 );
            exit ( 1 );
        }

        //get the maximum x and y for the domain
        xmax = x0 + dx*cols;
        ymax = y0 + dy*rows;  

        if ( pj_is_latlong(proj4DF) )
        {
            printf( "The grid domain is defined in the lat and long coordinates.\n" );
            cLon = x0 + (xmax - x0) / 2.0;
            cLat = y0 + (ymax - y0) / 2.0; 

            //SW corner point
            swLon = x0 + dx/2.0;
            swLat = y0 + dy/2.0;

            //NW corner point
            nwLon = x0 + dx/2.0;
            nwLat = ymax - dy/2.0;

            //NE corner point
            neLon = xmax - dx/2.0;
            neLat = ymax - dy/2.0;

            //SE corner point
            seLon = xmax - dx/2.0;
            seLat = y0 + dy/2.0;
        }
        else
        {
          //get center point lat and long of WRF mass grid (cross)
          xx = x0 + (xmax - x0) / 2.0;
          yy = y0 + (ymax - y0) / 2.0;
          
          latlongP = computeLatLong(proj4DF,xx,yy);
          cLon = latlongP.u;
          cLat = latlongP.v;
          printf ( "Center point: xx = %lf   yy = %lf  cLon = %lf  cLat = %lf\n",xx,yy,cLon,cLat);

          //SW corner point
          xx = x0 + dx/2.0;
          yy = y0 + dy/2.0;
          latlongP = computeLatLong(proj4DF,xx,yy);
          swLon = latlongP.u;
          swLat = latlongP.v; 
          printf ( "SW point: xx = %lf   yy = %lf  swLon = %lf  swLat = %lf\n",xx,yy,swLon,swLat);

          //NW corner point
          xx = x0 + dx/2.0;
          yy = ymax - dy/2.0;;
          latlongP = computeLatLong(proj4DF,xx,yy);
          nwLon = latlongP.u; 
          nwLat = latlongP.v;
          printf ( "NW point: xx = %lf   yy = %lf  nwLon = %lf  nwLat = %lf\n",xx,yy,nwLon,nwLat);

          //NE corner point
          xx = xmax - dx/2.0;
          yy = ymax - dy/2.0;
          latlongP = computeLatLong(proj4DF,xx,yy);
          neLon = latlongP.u;
          neLat = latlongP.v;
          printf ( "NE point: xx = %lf   yy = %lf  neLon = %lf  neLat = %lf\n",xx,yy,neLon,neLat);

          //SE corner point
          xx = xmax - dx/2.0;
          yy = y0 + dy/2.0;
          latlongP = computeLatLong(proj4DF,xx,yy);
          seLon = latlongP.u;
          seLat = latlongP.v;
          printf ( "SE point: xx = %lf   yy = %lf  seLon = %lf  seLat = %lf\n",xx,yy,seLon,seLat);
        }
        
	/**************************
	* Arrays for landuse data *
	**************************/
        int times = TIMES;

        //x and y array 2d
        if ( (x = (float*) calloc (times*cols, sizeof(float)) ) == NULL)
        {
           printf( "Calloc x failed.\n");
           exit ( 1 );
        }
        if ( (y = (float*) calloc (times*rows, sizeof(float)) ) == NULL)
        { 
           printf( "Calloc y failed.\n");
           exit ( 1 );
        } 

        //mass x   
        for (i=0; i<cols; i++)
        {
           x[i] = x0 + dx*i + dx/2.0;
        }
        //mass y
        for (j=0; j<rows; j++)
        {
           y[j] = y0 + dy*j + dy/2.0;
        }

        ///lat and long array 3d
        if ( (lat = (float*) calloc (times*rows*cols, sizeof(float)) ) == NULL)
        {
           printf( "Calloc lat failed.\n");
           exit ( 1 );
        }
        if ( (lon = (float*) calloc (times*rows*cols, sizeof(float)) ) == NULL)
        {
           printf( "Calloc lon failed.\n");
           exit ( 1 );
        }

        for(j=0; j<rows; j++)
        {
           yy = y0 + dy*j + dy/2.0;
           for (i=0; i<cols; i++)
           {
              xx = x0 + dx*i + dx/2.0;         
              latlongP = computeLatLong(proj4DF,xx,yy);
              lon[j*cols+i] = latlongP.u;
              lat[j*cols+i] = latlongP.v;
           }
        }
         
	/***************************************
	* Read WRF landuse data from text file *
	***************************************/
	printf("Reading input file '%s'...\n", inputFile);


        string                txtFileName;
        std::ifstream         luStream;             //input txt stream
	string                header, line, dataStr, fieldName;
        vector<string>        vHeader, vecString;
        double                waterP;   //total water class
        double                maxP;     //maximum percent class
	int                   r, c;
        int                   idx, maxIndex;


        // read landuse txt file 
        txtFileName = string (inputFile);
        luStream.open( txtFileName.c_str() ); 
        i = 0;  //initialize line number
        if (luStream.good() )
        {
           getline( luStream, line);
           while ( !luStream.eof() )
           {
             line = trim (line);  //get rid of spaces at edges
             i++;  //count the line number

             //get rid of empty line
             if ( line.size() == 0 )
             {
                goto newloop;
             }

             //printf( "\nline %d: %s\n",i,line.c_str() );
             //get title of table
             if ( line.find("GRIDID") !=string::npos )
             {
                tokenize(line, vHeader, ",");  //put header items into a vector
                idx = 2;   //first three items fixed
                if ( line.find("IMPERV") !=string::npos )
                { 
                   idx ++;
                   INCLUDE_IMPERV = true;    
                   if ( (imperv = (float *) calloc (times*rows*cols, sizeof(float)) ) == NULL)
                   {
                      printf( "Calloc imperv failed.\n");
                      exit ( 1 );
                   }
                } // end of imperv part

                if ( line.find("CANOPY") !=string::npos )
                {  
                   idx++;
                   INCLUDE_CANOPY = true;
                   if ( (canopy = (float *) calloc (times*rows*cols, sizeof(float)) ) == NULL)
                   {
                      printf( "Calloc canopy failed.\n");
                      exit ( 1 );
                   }
                }  //end of canopy part
 
                if ( line.find("111") !=string::npos || line.find("211") !=string::npos )
                {
                   INCLUDE_LANDUSE = true;
                   idx++;
                   land_cat_len = vHeader.size() - idx;   //total landuse classes

                   printf( "idx = %d vHeader size = %d  classes = %d \n", idx, vHeader.size(), land_cat_len);
                   if ( (lu_list = (int*) calloc (times*land_cat_len, sizeof(int)) ) == NULL)
                   {
                      printf( "Calloc lu_list failed.\n");
                      exit ( 1 );
                   }
 
                   int kk = 0;  // landuse item count
                   for (j=idx; j<vHeader.size(); j++)
                   {
                      fieldName =  vHeader[j];
                      lu_list[kk] = atoi ( fieldName.c_str() );
                      kk++;
                   }
              
                   //allocate mem to landuse fraction array
                   if ( (luf = (float*) calloc (times*land_cat_len*rows*cols, sizeof(float)) ) == NULL)
                   {
                      printf( "Calloc luf failed.\n");
                      exit ( 1 );
                   }

                   //allocate mem to landuse mask array
                   if ( (lum = (float*) calloc (times*rows*cols, sizeof(float)) ) == NULL)
                   {
                      printf( "Calloc lum failed.\n");
                      exit ( 1 );
                   }

                   //allocate mem to landuse dominant class array
                   if ( (lud = (float*) calloc (times*rows*cols, sizeof(float)) ) == NULL)
                   {
                      printf( "Calloc lud failed.\n");
                      exit ( 1 );
                   }
                } //end of landuse part
                //printf( "Got header.\n");
             }
             else
             {
                tokenize(line, vecString, ",");  //put data line items into a vector
                if ( vHeader.size() != vecString.size() )
                {
                    printf( "Data file line has different items from header line: %d\n",i); 
                    exit ( 1 );
                }
              
                //get row and col 
                dataStr = vecString[1]; 
                r = atoi ( dataStr.c_str() );  //start from 1

                dataStr = vecString[2];
                c = atoi ( dataStr.c_str() );  // start from 1

                idx = 2;  //first three items fixed
                if ( INCLUDE_IMPERV )
                {
                   idx ++;
                   dataStr = vecString[idx];
                   k = (r - 1) * cols + c - 1 ;  //index in 1d array   
                   imperv[k] = atof ( dataStr.c_str() );    
                }

                if ( INCLUDE_CANOPY )
                {
                   idx ++;
                   dataStr = vecString[idx]; 
                   k = (r - 1) * cols + c - 1;  //index in 1d array
                   canopy[k] = atof ( dataStr.c_str() );
                 }
             
                 //printf ( "row = %d  col = %d  k = %d imperv = %f    canopy = %f\n",r,c,k, imperv[k], canopy[k]);
                 if ( INCLUDE_LANDUSE )              
                 { 
                    idx++;
                    int kk = 0;  // landuse class count
                    waterP = 0.0;    //initlize it
                    maxP  = -100.0;  //initialize it
                    for (j=idx; j<vecString.size(); j++)
                    {
                       dataStr = vecString[j];
                       k = kk * rows * cols + (r - 1) * cols + c - 1;
                       luf[k] = atof ( dataStr.c_str() );
                       fieldName =  vHeader[j];
                       if ( fieldName.compare("111") == 0 || fieldName.compare("20") == 0 || 
                            fieldName.compare("217") == 0 || fieldName.compare("2255") == 0 )
                       {
                          //get total water percent
                          waterP += luf[k];
                       }
                       if (luf[k] > maxP)
                       {
                          //get dominant class 
                          maxP = luf[k];
                          maxIndex = j;  //field index with max percent
                       }
                       kk++;  //number of landuse classes read
                    }

                    k = (r - 1) * cols + c - 1 ;  //index in array  
                    //land = 1 and water = 0  mask
                    if (waterP > 50.00)
                    {
                       lum[k] = 0.0; 
                    }
                    else
                    {
                       lum[k] = 1.0;
                    }

                    fieldName = vHeader[maxIndex]; 
                    lud[k] = atof ( fieldName.c_str() );   //get class number with maximum percent
                 }  //landuse parts

                 vecString.clear();   //clean the string
              }  //data lines

             //printf( "line %d  row=%d  col=%d\n",i,r,c );
             newloop:
             getline( luStream, line);
           }  // while loop

           luStream.close();

           //checking 
           //printf ( " r1 c7 IMP=%f  CAN=%lf  LUF=%lf  LUM=%d  LUD=%d\n",imperv[6], canopy[6],luf[2456506],lum[6],lud[6]);
           //printf ( " r1 c7 IMP=%f  CAN=%lf  LUF=%lf  LUM=%d  LUD=%d\n",imperv[34999], canopy[34999],luf[179499],lum[34999],lud[34999]);
           //printf ( " r1 c7 IMP=%f  CAN=%lf  LUF=%lf  LUM=%d  LUD=%d\n",imperv[72249], canopy[72249],luf[2962249],lum[72249],lud[72249]);

        } // if good
        else
        {
           printf( "Unable to open file: %s\n", inputFile);
           exit( 1 );
        }

        pj_free ( proj4DF );
        printf ( "Finished reading file: %s\n", inputFile); 

	/***********************************************
	* Write WRF landuse data file in netCDF format *
	***********************************************/

	/********************
	* Dimension lengths *
	********************/
	size_t west_east_len   = cols;
	size_t south_north_len = rows;
        size_t time_len = TIMES;
        size_t dateStr_len = 19;
	/********************
	* Enter define mode *
	********************/
        int ncid;
	anyErrors( nc_create(outputFile, NC_CLOBBER, &ncid) );

	/********************
	* Define dimensions *
	********************/
        int dateStr_dim;
        int time_dim;
        int west_east_dim;
        int south_north_dim;
        int land_cat_dim;

        printf( "Defining dimensions in output netcdf file...\n" );

        anyErrors( nc_def_dim(ncid, "Time", NC_UNLIMITED, &time_dim) );
        anyErrors( nc_def_dim(ncid, "DateStrLen", dateStr_len, &dateStr_dim) );
	anyErrors( nc_def_dim(ncid, "west_east", west_east_len, &west_east_dim) );
	anyErrors( nc_def_dim(ncid, "south_north", south_north_len, &south_north_dim) );
	anyErrors( nc_def_dim(ncid, "land_cat", land_cat_len, &land_cat_dim) );

	/*******************
	* Define variables *
	*******************/
        int times_id;
        int x_id;
        int y_id;
        int lon_id;
        int lat_id;
        int lu_list_id;
        int imperv_id;
        int canopy_id;
        int lum_id;
        int luf_id;
        int lud_id;

	int dims2_x[2],dims2_y[2],dims3[3], dims2_lu[2],dims4[4];  //dimention definition 
        
        printf( "Defining variables in output netcdf file...\n" );

        dims2_x[0] = time_dim;
        dims2_x[1] = dateStr_dim;
        anyErrors( nc_def_var(ncid, "Times", NC_CHAR, 2, dims2_x, &times_id) );

        dims2_x[0] = time_dim;
        dims2_x[1] = west_east_dim;
	anyErrors( nc_def_var(ncid, "X_M", NC_FLOAT, 2, dims2_x, &x_id) );

        dims2_y[0] = time_dim;
	dims2_y[1] = south_north_dim;
	anyErrors( nc_def_var(ncid, "Y_M", NC_FLOAT, 2, dims2_y, &y_id) );

        dims3[0] = time_dim;
        dims3[1] = south_north_dim;
        dims3[2] = west_east_dim;
        anyErrors( nc_def_var(ncid, "XLONG_M", NC_FLOAT, 3, dims3, &lon_id) );
        anyErrors( nc_def_var(ncid, "XLAT_M", NC_FLOAT, 3, dims3, &lat_id) );
     
        if ( INCLUDE_IMPERV )
        {
	   anyErrors( nc_def_var(ncid, "IMPERV", NC_FLOAT, 3, dims3, &imperv_id) );
        }
       
        if ( INCLUDE_CANOPY )
        { 
	  anyErrors( nc_def_var(ncid, "CANOPY", NC_FLOAT, 3, dims3, &canopy_id) );
        }

        if ( INCLUDE_LANDUSE )
        {
           dims2_lu[0] = time_dim;
           dims2_lu[1] = land_cat_dim;
           anyErrors( nc_def_var(ncid, "LU_LIST", NC_INT, 2, dims2_lu, &lu_list_id) );

           dims4[0] = time_dim;
           dims4[1] = land_cat_dim;
           dims4[2] = south_north_dim;
           dims4[3] = west_east_dim;
	   anyErrors( nc_def_var(ncid, "LANDUSEF", NC_FLOAT, 4, dims4, &luf_id) );
           printf( "Defined luf\n" );

	   anyErrors( nc_def_var(ncid, "LANDMASK", NC_FLOAT, 3, dims3, &lum_id) );
           printf( "Defined lum\n" );

	   anyErrors( nc_def_var(ncid, "LU_INDEX", NC_FLOAT, 3, dims3, &lud_id) );
           printf( "Defined lud\n" );

        }
	/********************
	* Assign attributes *
	********************/
        printf( "Assigning variable attributes in output netcdf file...\n" );
        int fieldtype[1];

	// x values
	fieldtype[0] = 104;
	anyErrors( nc_put_att_int(ncid, x_id, "FieldType", NC_INT, 1, fieldtype) );
	anyErrors( nc_put_att_text(ncid, x_id, "MemoryOrder", 2, "X ") );
	anyErrors( nc_put_att_text(ncid, x_id, "Description", 14, "X on mass grid") );
	anyErrors( nc_put_att_text(ncid, x_id, "Units", 1, "Meters") );
	anyErrors( nc_put_att_text(ncid, x_id, "stagger", 1, "M") );

	// y values
	anyErrors( nc_put_att_int(ncid, y_id, "FieldType", NC_INT, 1, fieldtype) );
	anyErrors( nc_put_att_text(ncid, y_id, "MemoryOorder", 2, "Y ") );
	anyErrors( nc_put_att_text(ncid, y_id, "Description", 14, "Y on mass grid") );
	anyErrors( nc_put_att_text(ncid, y_id, "Units", 1, "Meters") );
	anyErrors( nc_put_att_text(ncid, y_id, "stagger", 1, "M") );


        // Grid longitudes
        anyErrors( nc_put_att_int(ncid, lon_id, "FieldType", NC_INT, 1, fieldtype) );
        anyErrors( nc_put_att_text(ncid, lon_id, "MemoryOrder", 3, "XY ") );
        anyErrors( nc_put_att_text(ncid, lon_id, "Units", 17, "degrees longitude") );
        anyErrors( nc_put_att_text(ncid, lon_id, "Description", 22, "longitude on mass grid") );
        anyErrors( nc_put_att_text(ncid, lon_id, "Stagger", 1, "M") );

        // Grid latitudes
        anyErrors( nc_put_att_int(ncid, lat_id, "FieldType", NC_INT, 1, fieldtype) );
        anyErrors( nc_put_att_text(ncid, lat_id, "MemoryOrder", 3, "XY ") );
        anyErrors( nc_put_att_text(ncid, lat_id, "Units", 16, "degrees latitude") );
        anyErrors( nc_put_att_text(ncid, lat_id, "Description", 21, "latitude on mass grid") );
        anyErrors( nc_put_att_text(ncid, lat_id, "Stagger", 1, "M") );

	// Impervious surface coverage
        if ( INCLUDE_IMPERV )
        {
	   anyErrors( nc_put_att_int(ncid, imperv_id, "FieldType", NC_INT, 1, fieldtype) );
           anyErrors( nc_put_att_text(ncid, imperv_id, "MemoryOrder", 3, "XY ") );
	   anyErrors( nc_put_att_text(ncid, imperv_id, "Description", 41, "percent of impervious surface in WRF grid") );
           anyErrors( nc_put_att_text(ncid, imperv_id, "Units", 7, "Percent") );
           anyErrors( nc_put_att_text(ncid, imperv_id, "Stagger", 1, "M") );
        }

	// Forest canopy
        if ( INCLUDE_CANOPY )
        {
           anyErrors( nc_put_att_int(ncid, canopy_id, "FieldType", NC_INT, 1, fieldtype) );
           anyErrors( nc_put_att_text(ncid, canopy_id, "MemoryOrder", 3, "XY ") );
           anyErrors( nc_put_att_text(ncid, canopy_id, "Description", 41, "percent of forest tree canopy in WRF grid") );
           anyErrors( nc_put_att_text(ncid, canopy_id, "Units", 7, "Percent") );
           anyErrors( nc_put_att_text(ncid, canopy_id, "stagger", 1, "M") );
        }

	// 2001 landuse 
        if (INCLUDE_LANDUSE )
        {
           // landuse classes list
           anyErrors( nc_put_att_int(ncid, lu_list_id, "FieldType", NC_INT, 1, fieldtype) );
           anyErrors( nc_put_att_text(ncid, lu_list_id, "MemoryOrder", 3, "0  ") );
           anyErrors( nc_put_att_text(ncid, lu_list_id, "Description", 42, "2001 USGS NLCD and MODIS IGBP class number") );
           anyErrors( nc_put_att_text(ncid, lu_list_id, "Units", 8, "Category") );
           anyErrors( nc_put_att_text(ncid, lu_list_id, "Stagger", 1, "") );

           // 2001 landuse fraction
           anyErrors( nc_put_att_int(ncid, luf_id, "FieldType", NC_INT, 1, fieldtype) );
           anyErrors( nc_put_att_text(ncid, luf_id, "MemoryOrder", 3, "XY ") );
           anyErrors( nc_put_att_text(ncid, luf_id, "Description", 31, "percent of land use in WRF grid") );
           anyErrors( nc_put_att_text(ncid, luf_id, "Units", 7, "Percent") );
           anyErrors( nc_put_att_text(ncid, luf_id, "stagger", 1, "M") );

           //land mask 
           anyErrors( nc_put_att_int(ncid, lum_id, "FieldType", NC_INT, 1, fieldtype) );
           anyErrors( nc_put_att_text(ncid, lum_id, "MemoryOrder", 3, "XY ") );
           anyErrors( nc_put_att_text(ncid, lum_id, "Units", 17, "None") );
           anyErrors( nc_put_att_text(ncid, lum_id, "Description", 26, "Landmask : 1=land, 0=water") );
           anyErrors( nc_put_att_text(ncid, lum_id, "Stagger", 1, "M") );

           // dominant land class
           anyErrors( nc_put_att_int(ncid, lud_id, "FieldYype", NC_INT, 1, fieldtype) );
           anyErrors( nc_put_att_text(ncid, lud_id, "MemoryOrder", 3, "XY ") );
           anyErrors( nc_put_att_text(ncid, lud_id, "Units", 16, "Category") );
           anyErrors( nc_put_att_text(ncid, lud_id, "Description", 17, "Dominant category") );
           anyErrors( nc_put_att_text(ncid, lud_id, "Stagger", 1, "M") );
        }
	/**************************
	* Store global attributes *
	**************************/
        //global attributes
        int gatt[1];
        float gattf[1];
        float cdf_corner_lats[4];
        float cdf_corner_lons[4];

        printf( "Defining global attributes in output netcdf file...\n" );

	anyErrors( nc_put_att_text(ncid, NC_GLOBAL, "title", 23, "WRF GRID LANDUSE DATA FROM computeGridLandUse.exe") );
	anyErrors( nc_put_att_text(ncid, NC_GLOBAL, "SIMULATION_START_DATE", 19, "0000-00-00_00:00:00") );
       
        gatt[0] = cols+1; 
	anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "WEST-EAST_GRID_DIMENSION", NC_INT, 1, gatt) );
        gatt[0] = rows+1;
	anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "SOUTH-NORTH_GRID_DIMENSION", NC_INT, 1, gatt) );
        gatt[0] = 0;
	anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "BOTTOM-TOP_GRID_DIMENSION", NC_INT, 1, gatt) );
	anyErrors( nc_put_att_text(ncid, NC_GLOBAL, "GRIDTYPE", 1, "C") );

        gattf[0] = x0;
        anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "XMIN", NC_FLOAT, 1, gattf) );
        gattf[0] = y0;
        anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "YMIN", NC_FLOAT, 1, gattf) );
        gattf[0] = dx;
	anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "DX", NC_FLOAT, 1, gattf) );
        gattf[0] = dy;
	anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "DY", NC_FLOAT, 1, gattf) );

        gattf[0] = cLat;
	anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "CEN_LAT", NC_FLOAT, 1, gattf) );
        gattf[0] = cLon;
	anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "CEN_LON", NC_FLOAT, 1, gattf) );
	gattf[0] = trueLat1;
	anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "TRUELAT1", NC_FLOAT, 1, gattf) );
        gattf[0] = trueLat2;
	anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "TRUELAT2", NC_FLOAT, 1, gattf) );
        gattf[0] = cLat;
	anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "MOAD_CEN_LAT", NC_FLOAT, 1, gattf) );
        gattf[0] = stdLon;
	anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "STAND_LON", NC_FLOAT, 1, gattf) );
        gattf[0]= stdLat;
        anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "STAND_LAT", NC_FLOAT, 1, gattf) );
        gattf[0] = poleLat;
	anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "POLE_LAT", NC_FLOAT, 1, gattf) );
        gattf[0] = poleLon;
	anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "POLE_LON", NC_FLOAT, 1, gattf) );

        cdf_corner_lats[0] = swLat;
        cdf_corner_lats[1] = nwLat;
        cdf_corner_lats[2] = neLat;
        cdf_corner_lats[3] = seLat;
	anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "corner_lats", NC_FLOAT, 4, cdf_corner_lats) );
        cdf_corner_lons[0] = swLon;
        cdf_corner_lons[1] = nwLon;
        cdf_corner_lons[2] = neLon;
        cdf_corner_lons[3] = seLon;
	anyErrors( nc_put_att_float(ncid, NC_GLOBAL, "corner_lons", NC_FLOAT, 4, cdf_corner_lons) );

        gatt[0] = proj; 
	anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "MAP_PROJ", NC_INT, 1, gatt) );
	anyErrors( nc_put_att_text(ncid, NC_GLOBAL, "MMINLU", 4, "USGS") );

        /*
        gatt[0] = 1; 
	anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "grid_id", NC_INT, 1, gatt) );
        gatt[0] = 1;
	anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "parent_id", NC_INT, 1, gatt) );
	anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "i_parent_start", NC_INT, 1, gatt) );
	anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "j_parent_start", NC_INT, 1, gatt) );
        gatt[0] = cols+1;
	anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "i_parent_end", NC_INT, 1, gatt) );
        gatt[0] = rows + 1;
	anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "j_parent_end", NC_INT, 1, gatt) );
        gatt[0] = 1;
	anyErrors( nc_put_att_int(ncid, NC_GLOBAL, "parent_grid_ratio", NC_INT, 1, gatt) );
        */

	/********************
	* Leave define mode *
	********************/
	anyErrors( nc_enddef (ncid) );

	/*********************************
	* Write variables to netCDF file *
	*********************************/
        printf( "Writing variable data in output netcdf file...\n" );
       
        //     Store Times
        //allocate mem for char var
        if ( (dateStr = (char *) calloc (1*dateStr_len, sizeof(char)) ) == NULL)
        {
            printf( "Calloc dateStr failed.\n");
            exit ( 1 );
        }

        long tx_start[2];
        long tx_count[2];
        tx_start[0]=0;
        tx_start[1]=0;
        tx_count[0]=time_len;
        tx_count[1]=dateStr_len;
        strcpy (dateStr,"0000-00-00_00:00:00");

        anyErrors( ncvarput(ncid, times_id, tx_start, tx_count, dateStr) );
        printf( "\tWrited Times\n" );
        free( dateStr );   

	//	Store x
	anyErrors( nc_put_var_float(ncid, x_id, x) );
        printf( "Writed x\n" );

	//	Store y
	anyErrors( nc_put_var_float(ncid, y_id, y) );
        printf( "Writed y\n" );

        //      Store lon
        anyErrors( nc_put_var_float(ncid, lon_id, lon) );
        printf( "Writed lon\n" );

        //      Store lat
        anyErrors( nc_put_var_float(ncid, lat_id, lat) );
        printf( "Writed lat\n" );

	//	Store imperv
        if ( INCLUDE_IMPERV )
        {
	   anyErrors( nc_put_var_float(ncid, imperv_id, imperv) );
           printf( "Writed imperv\n" );

           free(imperv);
        }

	//	Store canopy
        if ( INCLUDE_CANOPY )
        {
	   anyErrors( nc_put_var_float(ncid, canopy_id, canopy) );
           printf( "Writed canopy\n" );

           free(canopy);
        }

        //landuse
        if ( INCLUDE_LANDUSE )
        {
           //      Store lu_list
           anyErrors( nc_put_var_int(ncid, lu_list_id, lu_list) );
           printf( "Writed lu_list\n" );

           //	Store luf
           anyErrors( nc_put_var_float(ncid, luf_id, luf) );
           printf( "Writed luf\n" );

           //	Store lum
           anyErrors( nc_put_var_float(ncid, lum_id, lum) );
           printf( "Writed lum\n" );
 
           //      Store lud 
           anyErrors( nc_put_var_float(ncid, lud_id, lud) );
           printf( "Writed lud\n" );

           free(lu_list);
           free(luf);
           free(lum);
           free(lud);
        }


	/********************
	* Close netCDF file *
	********************/
	anyErrors( nc_close(ncid) );

        free(x);
        free(y);
        free(lat);
        free(lon);
        
        printf( "Finished writing output NetCDF file: %s\n\n", outputFile);
}

/****    End of Main program   ***/
/**************************************************************************************/



/*******************
* String tokenizer *
*******************/
void tokenize	(const string &str,
					vector<string> &tokens,
					const string &delimiters)
{
	// Skip delimiters at beginning.
	string::size_type lastPos = str.find_first_not_of(delimiters, 0);

	// Find first "non-delimiter".
	string::size_type pos = str.find_first_of(delimiters, lastPos);

	while (string::npos != pos || string::npos != lastPos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));

		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);

		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}
}
