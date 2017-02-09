#!/bin/csh -f
#**************************************************************************************
# Purpose:  to regrid OMI L2 AOD product variables for given modeling domain grids
#           from downloaded OMI L2 AOD product files in HDF5 format (he5) at 
#           the NASA Mirador web site.
#
#     There are four steps involved in computation:
#     1. Create a domain grid raster file with GRIDID from LL 1 to UR rows*cols
#     2. Compute resolution to rasterize the created grid domain raster file
#     3. If satellite image is in latlong projection.  It will rasterize the grid domain
#        in its projection.  If satellite image is in a x-y projection system, it
#        will project the grid domain raster file into satellite projection and rasterize
#        it using computed raster resolution.
#     3. Loop through day and its files to compute grid satellite data based on 
#        rasterized domain grids.  If rasterized domain grid file has different 
#        projection from satellite image, it will project each cell center 
#        to get satellite image value.
#     4. Output grid satellite variable data into a WRF netcdf file.
#
#
# Written by the Institute for the Environment at UNC, Chapel Hill
# in support of the UAE and NASA Projects, 2009.
#
# Written by:   LR, Dec. 2009 - Mar. 2010
#               05/2013 modified
#
# Program: computeGridOMIL2.exe
#         Needed environment variables included in the script file to run.
#
#***************************************************************************************

#
# Define domain grids
#
setenv GRID_PROJ "+proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=33 +lat_2=45 +lat_0=40 +lon_0=-97"

setenv GRID_ROWS     112
setenv GRID_COLUMNS  148

setenv GRID_XMIN  -2754000.0
setenv GRID_YMIN  -2106000.0

setenv GRID_XCELLSIZE 36000
setenv GRID_YCELLSIZE 36000

#
#set OMI L2 AOT product file directory which containts each day data:
#  
#setenv DATA_DIR   "../data/sat/OMI_L2/"

#Set dataset variables to be extracted
#
#setenv SATELLITE_VARIABLE FinalAerosolAbsOpticalDepth,FinalAerosolOpticalDepth,FinalAerosolSingleScattAlb
#
#Set date and time range: YYYYMMDDHHMM
#
#setenv START_DATE  200607010000
#setenv END_DATE    200607052359



#
#set OMI L2 NO2 product file directory which containts each day data:
#
setenv DATA_DIR   "../data/sat/OMI_NO2_L2/"

#Set dataset variables to be extracted
#
setenv SATELLITE_VARIABLE ColumnAmountNO2

#
#Set date and time range: YYYYMMDDHHMM
#
setenv START_DATE  200604220000
setenv END_DATE    200604222359


#
# Output files
#
#setenv OUTPUT_NETCDF_FILE    ../output/sat/wrap36km_OMI_us_20060422_20060423.nc

setenv OUTPUT_NETCDF_FILE    ../output/sat/wrap36km_OMI_NO2_us_20060422.nc

#
# run the OMI L2 poduct processing program
#
../bin/64bits/computeGridOMIL2.exe

#===================================================================
