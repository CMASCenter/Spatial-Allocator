#!/bin/csh -f
#**************************************************************************************
# Purpose:  to process EPIC modeling yearly average output for CMAQ bi-directional 
#           ammonia surface flux modeling 
#
#
# Written by the Institute for the Environment at UNC, Chapel Hill
# in support of the CMAS project, 2010
#
# Written by:   LR, Nov 2010
#               12/2011 modified
#               05/2014 modified - added BELD4 data and crop weighted output
#
# Program: epicYearlyAverage2CMAQ.exe
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
setenv DATA_DIR   "/nas01/depts/ie/cempd/EPIC/epic/scenarios/CMAQ12km/output4CMAQ/app/year"


#
# Define BELD4 land cover input file
#
setenv DOMAIN_BELD4_NETCDF     "/nas01/depts/ie/cempd/EPIC/epic/scenarios/CMAQ12km/share_data/beld4_cmaq12km_2006.nc"


#
# Output files: 
#
#crop specific output
setenv OUTPUT_NETCDF_FILE          /nas01/depts/ie/cempd/EPIC/epic/scenarios/CMAQ12km/output4CMAQ/app/toCMAQ/epic2cmaq_year.nc

#crop weighted output
setenv OUTPUT_NETCDF_FILE_TOTAL    /nas01/depts/ie/cempd/EPIC/epic/scenarios/CMAQ12km/output4CMAQ/app/toCMAQ/epic2cmaq_year_total.nc


#
# run the EPIC output processing program
#
../bin/64bits/extractEPICYearlyAverage2CMAQ.exe

#===================================================================
