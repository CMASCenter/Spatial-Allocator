#!/bin/csh -f
#**************************************************************************************
# Purpose:  to regrid MODIS L2 cloud product variables for given modeling domain grids
#           from downloaded MODIS L2 cloud product files in HDF4 format (hdf) at 
#           the NASA LAADS web site.
#
#     There are five steps involved in computation:
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
# in support of the UAE NASA Projects, 2009.
#
# Written by:   LR, Oct. 2009 - June 2010
#               06/2012  Modifed 
#
# Program: computeGridMODISL2Clouds.exe
#          Needed environment variables included in the script file to run.
#
#***************************************************************************************

#
# Define domain grids
#
setenv GRID_PROJ "+proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=33 +lat_2=45 +lat_0=40 +lon_0=-97"

setenv GRID_ROWS     240
setenv GRID_COLUMNS  279

setenv GRID_XMIN  -1008000.0
setenv GRID_YMIN  -1620000.0

setenv GRID_XCELLSIZE 12000
setenv GRID_YCELLSIZE 12000

#
#set MODIS L2 aerosol or cloud product file directory which containts each day data:
# 
setenv DATA_DIR   "../data/sat/MOD06_L2_062011/"

#Set cloud variables to be extracted
setenv SATELLITE_VARIABLE Cloud_Fraction,Cloud_Top_Pressure_Infrared,Cloud_Optical_Thickness

#Set aerosol variables to be extracted
#setenv SATELLITE_VARIABLE Optical_Depth_Land_And_Ocean,Cloud_Fraction_Land,Mass_Concentration_Land 

#
#Set date and time range: YYYYMMDDHHMM
#
setenv START_DATE  200606280000
setenv END_DATE    200606302359

# Output files
setenv OUTPUT_NETCDF_FILE    ../output/sat/EUS12km_aqua_aerosl_20060628_30.nc


# run the MODIS L2 cloud/aerosol product processing program
#
../bin/64bits/computeGridMODISL2Clouds.exe

#===================================================================
