#!/bin/csh -f
#*******************************************************************************
# Purpose:  generate landuse information for a given modeling domain grids from:
#     1. USGS NLCD 30m Landuse Files
#     2. USGS NLCD 30m Urban Imperviousness files
#     3. USGS NLCD 30m Tree Canopy Files
#     4. MODIS Landcove files
#
#     NLCD data can be downloaded from:
#              http://www.mrlc.gov/nlcd2001.php
#     MODIS land cover tiles (e.g. MCD12Q1) can be downloaded from:
#              http://ladsweb.nascom.nasa.gov/data/search.html
#
#
# Written by the Institute for the Environment at UNC, Chapel Hill
# in support of the EPA NOAA CMAS Modeling, 2007-2011.
#
# Developed by: LR
#               2008 original, 2011-2012 updated
#               05/2013  updated
#
# Usgae: NLCD_MODIS_processor.csh
#        It calls computeGridLandUse.exe
#*******************************************************************************

#===================================================================
#
# Define domain grid
#

# CMAQ Lambert Conformal Conic
setenv GRID_PROJ "+proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=33 +lat_2=45 +lat_0=40 +lon_0=-97"


#polar stereographic
#setenv GRID_PROJ "+proj=stere +a=6370000.0 +b=6370000.0 +lat_ts=33 +lat_0=90 +lon_0=-97 +k_0=1.0"

#Mercator
#setenv GRID_PROJ "+proj=merc +a=6370000.0 +b=6370000.0 +lat_ts=33 +lon_0=0"

#LONGLAT
#setenv GRID_PROJ  "+proj=latlong +a=6370000.0 +b=6370000.0"

setenv GRID_ROWS    183
setenv GRID_COLUMNS 165

setenv GRID_XMIN  -216000
setenv GRID_YMIN  -1392000

setenv GRID_XCELLSIZE 4000
setenv GRID_YCELLSIZE 4000



#=================================================================
#
# Define input file which contains NLCD and MODIS land cover files 
#

#File contains all preprocessed NLCD image files.
setenv INPUT_FILE_LIST         "../data/nlcd_modis_files_2006.txt"

#
#INCLUDE data selection
#
setenv INCLUDE_USGS_LANDUSE          YES
setenv INCLUDE_USGS_IMPERVIOUSNESS   YES
setenv INCLUDE_USGS_CANOPY           YES
setenv INCLUDE_MODIS                 YES

#===================================================================
#
# Output files
#
setenv OUTPUT_LANDUSE_TEXT_FILE      ../output/lc/lcc_wrf_tx4km.txt
setenv OUTPUT_LANDUSE_NETCDF_FILE    ../output/lc/lcc_wrf_tx4km.nc


#
# run the tool
#
../bin/64bits/computeGridLandUse.exe

#===================================================================
