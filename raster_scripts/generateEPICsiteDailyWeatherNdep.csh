#!/bin/csh -f
#**************************************************************************************
# Purpose:  1. to extract six daily weather variable data from MCIP data for EPIC modeling 
#           sites and to create WXPMRUN.DAT run data file for WXPM3020 to compute 
#           monthly climate data.
#           2. to extract five wet and dry N g/ha deposition data from CMAQ organix 
#              and inorganic N species 
#           
#   1. Radiation (MJ m^02) - daily total                                 
#   2. Tmax (C) - 2m                                                     
#   3. Tmin (C) - 2m                                                     
#   4. Precipitation (mm) - daily total                                  
#   5. Relative humidity (fraction) - daily average                      
#   6. Windspeed (m s^-1) - 10m daily average                            
#   7. Wet oxidized N - g/ha daily total                                 
#   8. Wet reduced N - g/ha daily total                                  
#   9. Dry oxdized N - g/ha daily total                                  
#   10. Dry reduced N - g/ha daily total                                
#   11. Wet organic N - g/ha daily total  
#
#
#     There are three steps involved in extraction:
#     1. read in EPIC site lat and long location data, project them into MCIP data 
#        project and convert them into column and row in MCIP grids.
#     2. loop through each day MCIP and CMAQ data to extract daily weather and N deposition 
#        variables for each EPIC site.  Write them into each EPIC site daily weather file.   
#     3. create EPICW2YR.2YR file for running EPIC application simulation and all generated EPIC daily weather files in order to run 
#        WXPM3020.DRB program. The program computes monthly weather data to be named in monthly weather list file like WPM1US.DAT.
#     4. create two years (previous year+current year) daily weather files
#
# Output files:
#     1. "siteId".dly - EPIC daily weather input file                      
#     2. EPICW2YR.2YR  - to run EPIC and WXPM3020.DRB for computing EPIC monthly weather
#     3. OUTPUT_NETCDF_FILE file in IOAPI format
#
# Written by the Institute for the Environment at UNC, Chapel Hill
# in support of the EPA CMAS Modeling, 2009.
#
# Written by:   L.R., Aug. 2009
# Modified by:  L.R., Aug. 200
#               L.R., Mar-Apr, 2013
#
# Call program: computeSiteDailyWeather.exe
#               Needed environment variables listed in this run script file. 
#        
# Usage: ./generateEPICsiteDailyWeatherfromMCIP.csh
#        All required environment variables are set in this sample file
#***************************************************************************************

#
# Define MCIP/CMAQ domain grid information
#
setenv GRID_PROJ "+proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=33 +lat_2=45 +lat_0=40 +lon_0=-97"


#CALNEX 4km domain

setenv GRID_ROWS   317
setenv GRID_COLUMNS  236

setenv GRID_XMIN   -2416000.0
setenv GRID_YMIN    -832000.0

setenv GRID_XCELLSIZE 4000
setenv GRID_YCELLSIZE 4000

setenv GRID_NAME  "4CALNEX1"


#
#Set MCIP data directory which containts daily MCIP files 
#Daily MCIP data files have to have names with METCRO2D*"date"
#The "date" can be in one of the format: YYYYMMDD, YYMMDD, YYYYDDD, YYDDD
#
setenv DATA_DIR   "/nas01/depts/ie/cempd/EPIC/mcip/calnex0910/"

#
#Set CMAQ output dry/wet deposition data directory which containts daily CMAQ files
# three options: CMAQ output
#                Zero 
#                Default - assume N mix ratio 0.8ppm for wet default deposition computation
#
#setenv DATA_DIR_CMAQ   "/nas01/depts/ie/cempd/EPIC/cmaq/deposistions/dep_2003/"
#setenv DATA_DIR_CMAQ  Zero
setenv DATA_DIR_CMAQ   Default


#
#Set date range: YYYYMMDD
#
setenv START_DATE  20100101
setenv END_DATE    20101231

#
#Set input EPIC site data file in ascii csv format as:  site_name,longitude,latitude
#
setenv EPIC_SITE_FILE  "/nas01/depts/ie/cempd/EPIC/epic/scenarios/Calnex4km_2010/share_data/EPICSites_Info.csv"

#
# Set output directory which will store created EPIC site daily weather and N deposition data files:
#   1. "SITE_NAME".dly   2. EPICW2YR.2YR 
#
setenv OUTPUT_DATA_DIR ../output/epic

#
# Set NetCDF output file to store computed EPIC site daily weather and N deposition data 
# Only set this variable when EPIC grids are the same as MCIP/CMAQ grids.  
# Otherwise, set it to NONE for no NetCDF file output
#
setenv OUTPUT_NETCDF_FILE  ${OUTPUT_DATA_DIR}calnex_met_dep_${START_DATE}_to_${END_DATE}.nc


#
# Run the tool
#
../bin/64bits/computeSiteDailyWeather.exe

#===================================================================
