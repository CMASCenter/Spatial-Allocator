#!/bin/csh -f
#**************************************************************************************
# Purpose: to generate GOES satellite data for a given modeling domain grids from:
#     1. GOES Imager Cloud Albedo (CALB, 4km resolution)	%
#     2. GOES Imager Insolation (INSL, 4km resolution)		Watts per square meters
#     3. GOES Imager Surface Albedo (SALB, 4km resolution)	%
#     4. GOES Imager Cloud Top Pressure (CTP, 4km resolution)	Pascals
#     5. GOES Imager Infrared Temperature at Cloud/Surface level (IR, 4km resolution)	Celsius	
#     6. GOES Sander Cloud Top Pressure (CTP, 10km resolution)	Pascals
#     7. GOES Sander Skin Temperature (TSKN, 10km resolution)	Celsius
#     8. GOES Sander Total Precipitable Water Vaport at Ground/Water Surface (TPW, 10km resolution)	mm	
#
#
#     There are three steps involved in computation:
#     1. create a regular domain grid shapefile with GRIDID item.    
#     2. project the grid domain shapefile into GOES image projection and rasterize the grid shapefile 
#        into a computed raster resolution.  
#     3. compute grid GOES image data based on rasterized domain grids and output grid GOES variable data into a netcdf file.
#
#
# Written by the Institute for the Environment at UNC, Chapel Hill
# in support of the EPA NOAA CMAS Modeling, 2008-2009.
#
# Written by:  LR, Oct. 2008  - 03, 2009
# Modified by: Jun. 2009
#              Sept. 2012 
#              April 2012
#              Sept. 2014: add text format reading and use Imager and Sounder lat/long grids
#
# Program: computeGridGOES.exe
#        Needed environment variables included in the script file to run. 
#        
#***************************************************************************************

#
# Define domain grids
#
setenv GRID_PROJ "+proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=33 +lat_2=45 +lat_0=40 +lon_0=-97"

setenv GRID_ROWS 311
setenv GRID_COLUMNS 471

setenv GRID_XMIN  -2628000
setenv GRID_YMIN  -1800000

setenv GRID_XCELLSIZE 12000
setenv GRID_YCELLSIZE 12000


#
#set GOES directory which containts each day data directory as gp_yyyymmdd 
#Under GOES_DATA_DIR, each day is a directory.  For instance, files for day 20060801 are stored in gp_20060801 directory.
#
setenv DATA_DIR   ${SA_HOME}/data/sat/goes082006/


#
#Set date and time range: YYYYMMDDHHMM
#
setenv START_DATE_TIME  200608010000
setenv END_DATE_TIME    200608022359

#INCLUDE GOES variable selection. Setting the GOES variable to YES or NO for selection
setenv INCLUDE_GOES_IMG_CALB          YES
setenv INCLUDE_GOES_IMG_CTP           YES
setenv INCLUDE_GOES_IMG_INSL          YES
setenv INCLUDE_GOES_IMG_SALB          YES
setenv INCLUDE_GOES_IMG_IR            YES
setenv INCLUDE_GOES_SND_CTP           YES
setenv INCLUDE_GOES_SND_TSKN          YES
setenv INCLUDE_GOES_SND_TPW           YES 


#
# Image and Sounder grid longitude and latitude grids
# in text format and gridID from LL corner 1 to UR corner rows*cols
#

setenv  GOES_IMAGER_POINT_LATLONG    ${SA_HOME}/data/GOES_Imager_pts_latlong.dat
setenv  GOES_SOUNDER_POINT_LATLONG   ${SA_HOME}/data/GOES_Sounder_pts_latlong.dat


#
# Output files and output directory has to exist
#
setenv OUTPUT_NETCDF_FILE    ${SA_HOME}/output/sat/wrf12km_goes_${START_DATE_TIME}_${END_DATE_TIME}.nc


#===================================================================
#
# run re-gridding tool to process corrected GEoTiff files
#
${SA_HOME}/bin/64bits/computeGridGOES.exe


#===================================================================
#
#convert to WRF assimilation format

setenv INPUT_MM5_GOES_FILENAME      $OUTPUT_NETCDF_FILE
setenv OUTPUT_WRF_NETCDF_FILENAME   ${SA_HOME}/output/sat/wrf12km_goes2WRF_${START_DATE_TIME}_${END_DATE_TIME}.nc


#
# Run the conversion tool
#
${SA_HOME}/bin/64bits/toDataAssimilationFMT.exe
#===================================================================
