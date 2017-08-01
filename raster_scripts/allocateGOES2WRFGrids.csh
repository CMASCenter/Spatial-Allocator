#!/bin/csh -f
#**************************************************************************************
# Purpose: to generate GOES satellite data for a given modeling domain grids from:
#     1. GOES Imager Cloud Albedo (CALB, 4km resolution)	%
#     2. GOES Imager Insolation (INSL, 4km resolution)		Watts per square meters
#     3. GOES Imager Photosynthetically Active Radiation (PARW, 4km resolution)		W/m^2
#     4. GOES Imager Photosynthetically Active Radiation (PARM, 4km resolution)		micro-mol/(s m^2)
#     5. GOES Imager Surface Albedo (SALB, 4km resolution)	%
#     6. GOES Imager Cloud Top Pressure (CTP, 4km resolution)	Pascals
#     7. GOES Imager Infrared Temperature at Cloud/Surface level (LWIR, 4km resolution)	Celsius	
#     8. GOES Sounder Cloud Top Pressure (CTP, 10km resolution)	Pascals
#     9. GOES Sounder Skin Temperature (TSKN, 10km resolution)	Celsius
#    10. GOES Sounder Total Precipitable Water Vaport at Ground/Water Surface (TPW, 10km resolution)	mm	
#    11. GOES Sounder Infrared Temperature at Cloud/Surface level (LWIR, 10km resolution)	Celsius	
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
#              Jan.  2015: added new products (LWIR, PARW, PARM) and modified for new naming convention (apb, UAH)
#                          New file names are: goes_[sensor]_[product]_v[version-number]_YYYYMMDDhhmmZ.txt
#              Feb.  2017: Removed hard coded dimensions for satellite data so that the program can be used 
#                          with old or newer versions of GOES Imager data. The coverage was extended in the 
#                          new retrievals (from 1500x800 to 1552x1300).                                (apb, UAH)
#
# Program: computeGridGOES.exe
#        Needed environment variables included in the script file to run. 
#        
#***************************************************************************************

# apb: Setting up the test case for UAH - September 15, 2014
#
source /rhome/biazar/bck/tools/spatial_allocator/sa_uah/bin/sa_setup_uah.csh

############################################################################
#
# DEFINING MODEL DOMAIN:
#
# The SA Raster Tools define the modeling domain using the following environment variables:
# GRID_PROJ – defines the domain grid projection using the PROJ4 projection description format, 
#             for a full list see: (http://spatialreference.org/) or on the PROJ4 wiki: 
#             (http://proj4.org/index.html). The following sample projection descriptions are used to match the projections in WRF:
#             Lambert Conformal Conic: +proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=33 +lat_2=45 +lat_0=40 +lon_0=-97
#             Polar stereographic: +proj=stere +a=6370000.0 +b=6370000.0 +lat_ts=33 +lat_0=90 +lon_0=-97 +k_0=1.0
#             Mercator: +proj=merc +a=6370000.0 +b=6370000.0 +lat_ts=33 +lon_0=0
#             Geographic: +proj=latlong +a=6370000.0 +b=6370000.0
#
# GRID_ROWS –      number of rows of grid cells in the domain
# GRID_COLUMNS –   number of columns of grid cells in the domain
# GRID_XCELLSIZE – grid cell size in x direction
# GRID_YCELLSIZE – grid cell size in y direction
# GRID_XMIN –      minimum x of the domain (lower left corner of the domain)
# GRID_YMIN –      minimum y of the domain (lower left corner of the domain)
# GRID_NAME –      name of the domain, which is required by some of the tools
#
# For WRF simulations, GRID_XMIN and GRID_YMIN can be computed using 
# the first point longitude and latitude from the global attributes corner_lons and corner_lats 
# in the domain’s WRF GEOGRID output file. For instance, to compute a 
# WRF Lambert Conformal Conic (LCC) domain with the GEOGRID output file attributes
#
#    :corner_lats = 20.85681f, 52.1644f, 50.63151f, 19.88695f, 20.84302f...
#    :corner_lons = -121.4918f, -135.7477f, -53.21942f, -69.02478f, -121.5451f…
#
# use the cs2cs utility in the PROJ4 library directly at the command line (after installing the SA system):
#
# cs2cs +proj=latlong +a=6370000.0 +b=6370000.0 +to +proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=33 +lat_2=45 +lat_0=40 +lon_0=-97 -121.4918 20.85681 -2622003.85 -1793999.28 0.00
#
# Minimum x and y for the domain would be computed as follows:
#
# GRID_XMIN = -2622003.85 - GRID_XCELLSIZE / 2
# GRID_YMIN = -1793999.28 - GRID_YCELLSIZE / 2
#
#
######################################################################
#
# apb: Setting up the test case for UAH - September 15, 2014
#
source /rhome/biazar/bck/tools/spatial_allocator/sa_uah/bin/sa_setup_uah.csh


#
# Define domain grids
#
setenv GRID_PROJ "+proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=33 +lat_2=45 +lat_0=40 +lon_0=-97"


## TARGET MODEL GRID
##---------------------
##
## NOTE: for WRF domain ROWS and COLUMNS are the number of unstaggered grids (mass grids) in Y and X direction,
#        but XMIN and YMIN are the coordinates for the lower left conrner (SW staggered grid cell) of the domain.
#        See the description above for calculating XMIN and YMIN for WRF.
#
# 12km CONUS domain
setenv GRID_ROWS 492
setenv GRID_COLUMNS 384

setenv GRID_XMIN  -2952000
setenv GRID_YMIN  -2304000

setenv GRID_XCELLSIZE 12000
setenv GRID_YCELLSIZE 12000

set dname = "12km"

#
# set GOES directory which containts each day data directory as yyyymmdd 
# Under GOES_DATA_DIR, each day is a directory.  For instance, files for day 20120801 are stored in 20120801 directory.
# NOTE: The files should be uncompressed.
# EXAMPLE:
#         /rstor/biazar/data/spatial_allocator/test_data/sat/goestest/gp_2012/20120801/goes_img_SALB_v01_201208010045Z.txt
#         /rstor/biazar/data/spatial_allocator/test_data/sat/goestest/gp_2012/20120801/goes_img_SALB_v01_201208010145Z.txt
#         /rstor/biazar/data/spatial_allocator/test_data/sat/goestest/gp_2012/20120801/goes_img_SALB_v01_201208010245Z.txt
#         ...
#
# Old Data: 2017 data is v00 for Imager products
setenv GOES_DATA_DIR   /rstor/biazar/data/spatial_allocator/test_data/sat/goestest/gp_2017
#
# New DATA: 2012 data is v01 for Imager products
#setenv GOES_DATA_DIR   /rstor/biazar/data/spatial_allocator/test_data/sat/goestest/gp_2012
#

#
#Set date and time range: YYYYMMDDHHMM
#
# Test dates for old data:
setenv START_DATE_TIME  201704010000
setenv END_DATE_TIME    201704012359

# Test dates for new data:
#setenv START_DATE_TIME  201208010000
#setenv END_DATE_TIME    201208012359

#INCLUDE GOES variable selection. Setting the GOES variable to YES or NO for selection
setenv INCLUDE_GOES_IMG_CALB          NO
setenv INCLUDE_GOES_IMG_CTP           NO
setenv INCLUDE_GOES_IMG_INSL          YES
setenv INCLUDE_GOES_IMG_SALB          NO
setenv INCLUDE_GOES_IMG_LWIR          YES
setenv INCLUDE_GOES_IMG_PARW          yes
setenv INCLUDE_GOES_IMG_PARM          yes
setenv INCLUDE_GOES_SND_CTP           NO
setenv INCLUDE_GOES_SND_TSKN          NO
setenv INCLUDE_GOES_SND_TPW           NO 
setenv INCLUDE_GOES_SND_LWIR          NO 


#
# Imager and Sounder grid longitude and latitude grids
# in text format and gridID from LL corner 1 to UR corner rows*cols
#
# OLD IMAGER DATA: Version 00 - goes_img_SALB_v00_201208011345Z.txt
setenv  GOES_IMAGER_POINT_LATLONG    ${SA_HOME}/data/GOES_Imager_pts_latlong.dat.1500x800
setenv  COLS_IMG 1500
setenv  ROWS_IMG 800

#NEW IMAGER DATA: Version 01 - goes_img_SALB_v01_201208011345Z.txt
#setenv  GOES_IMAGER_POINT_LATLONG    ${SA_HOME}/data/GOES_Imager_pts_latlong.dat.1552x1300
#setenv  COLS_IMG 1552
#setenv  ROWS_IMG 1300

setenv  GOES_SOUNDER_POINT_LATLONG   ${SA_HOME}/data/GOES_Sounder_pts_latlong.dat.600x320
setenv  COLS_SND 600
setenv  ROWS_SND 320


#
# Output files and output directory has to exist
#
setenv OUTPUT_NETCDF_FILE    ${SA_HOME}/output/sat/wrf${dname}_goes_${START_DATE_TIME}_${END_DATE_TIME}.nc


#===================================================================
#
# run re-gridding tool to process corrected GEoTiff files
#
${SA_HOME}/bin/64bits/computeGridGOES.exe


#===================================================================
#
#convert to WRF assimilation format

setenv INPUT_MM5_GOES_FILENAME      $OUTPUT_NETCDF_FILE
setenv OUTPUT_WRF_NETCDF_FILENAME   ${SA_HOME}/output/sat/wrf${dname}_goes2WRF_${START_DATE_TIME}_${END_DATE_TIME}.nc


#
# Run the conversion tool
#
${SA_HOME}/bin/64bits/toDataAssimilationFMT.exe
#===================================================================
