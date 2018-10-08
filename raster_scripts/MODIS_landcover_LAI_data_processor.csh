#!/bin/csh -f
#*******************************************************************************
# Purpose:  generate landuse information for a given modeling domain grids from:
#     1. USGS NLCD 30m Landuse Files
#     2. USGS NLCD 30m Urban Imperviousness files
#     3. USGS NLCD 30m Tree Canopy Files
#     4. MODIS Landcove files (500m MCD12Q1)
#     5. MODIS LAI (500m 8-day MCD15A2H, MOD15A2, MOD15A2GFS)
#
#     NLCD data can be downloaded from:
#              http://www.mrlc.gov/nlcd2001.php
#     MODIS land cover tiles and LAI date can be downloaded from:
#              http://ladsweb.nascom.nasa.gov/data/search.html
#
#
# Written by Limei Ran
#	     EPA/ORD/CED 
#            2018-09-20
#
# Usgae:  ./MODIS_landcover_LAI_data_processor.csh
#         calls computeGridLandData_LAI_MODIS.exe
#*******************************************************************************

set year = 2016

#===================================================================
#
# Define domain grid
#

#12km WRF
setenv GRID_PROJ "+proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=33 +lat_2=45 +lat_0=40 +lon_0=-97"

setenv GRID_ROWS 311
setenv GRID_COLUMNS 471

setenv GRID_XMIN  -2628000
setenv GRID_YMIN  -1800000

setenv GRID_XCELLSIZE 12000
setenv GRID_YCELLSIZE 12000

setenv GRID_NAME  "US1_WRF12KM"


#=================================================================
#
# Define input file which contains NLCD and MODIS land cover files 
#

#File contains all preprocessed NLCD image files.
setenv INPUT_FILE_LIST         "../data/nlcd_modis_files_2011.txt"


#
#INCLUDE data selection
#
setenv INCLUDE_USGS_LANDUSE          NO
setenv INCLUDE_USGS_IMPERVIOUSNESS   NO
setenv INCLUDE_USGS_CANOPY           NO
setenv INCLUDE_MODIS                 YES


#=================================================================
#
# Define MODIS LAI/FAR input file directory
#

setenv DATA_DIR         "/work/MOD3APP/festc/sa_052014/data/sat/MCD15A/${year}_A2H/"

#
# set variables to extract
#A2H LAI/FPAR data
#
setenv SATELLITE_VARIABLE  Fpar_500m,Lai_500m

#
#Set date and time range: YYYYMMDDHHMM for the data sets
#
setenv START_DATE  ${year}01010000
setenv END_DATE    ${year}12310000


#===================================================================
#
# Output files
#

setenv OUTPUT_LANDUSE_TEXT_FILE      ../output/sat/${GRID_NAME}_MCD15A2H_c06_${START_DATE}_${END_DATE}.txt
setenv OUTPUT_LANDUSE_NETCDF_FILE    ../output/sat/${GRID_NAME}_MCD15A2H_c06_${START_DATE}_${END_DATE}.nc

#===================================================================
#
# run the tool
#
../src/raster/computeGridLandUse_LAI_MODIS.exe

#===================================================================
