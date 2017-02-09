#!/bin/csh -f
#**************************************************************************************
# Purpose:  to process EPIC modeling output for CMAQ bi-directional ammonia surface
#           flux modeling 
#
#
# Written by the Institute for the Environment at UNC, Chapel Hill
# in support of the CMAS project, 2010
#
# Written by:   LR, July-Sept 2010
#               2012 Modified
#
# Program: extractEPIC2CMAQ.exe
#          Needed environment variables included in the script file to run.
#
#***************************************************************************************

#
# Define domain grids
#
setenv GRID_PROJ "+proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=33 +lat_2=45 +lat_0=40 +lon_0=-97"

setenv GRID_ROWS 299
setenv GRID_COLUMNS 459

setenv GRID_XMIN  -2556000.0
setenv GRID_YMIN  -1728000.0

setenv GRID_XCELLSIZE 12000
setenv GRID_YCELLSIZE 12000

setenv GRID_NAME  "CONUS12KM"
#
#set EPIC output file directory which containts each day data:
#  
setenv DATA_DIR   "/nas01/depts/ie/cempd/EPIC/epic/scenarios/CMAQ12km/output4CMAQ/app/daily"

#
#Set date and time range: YYYYMMDD
#
setenv START_DATE  20011222
setenv END_DATE    20021231

#
# Output file prefix for soil and EPIC daily output
# "prefix"_soil.nc for soil ouput and "prefix"_time"yyyymm".nc for daily EPIC output
#
setenv OUTPUT_NETCDF_FILE_PREFIX    "/nas01/depts/ie/cempd/EPIC/epic/scenarios/CMAQ12km/output4CMAQ/app/toCMAQ/epic2cmaq"


#
# run the EPIC output processing program
#
../bin/64bits/extractEPIC2CMAQ.exe

#===================================================================
