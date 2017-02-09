#!/bin/csh -f
#*******************************************************************************
# Purpose:  generate landuse information for a given modeling domain grids from:
#     1. USGS NLCD 30m Landuse Files
#     2. USGS NLCD 30m Urban Imperviousness files  
#     3. USGS NLCD 30m Tree Canopy Files
#     4. tiled MODIS Landcove files
#
#     NLCD data can be downloaded from: 
#              http://www.mrlc.gov/nlcd2001.php
#     MODIS land cover tiles (e.g. MCD12Q1) can be downloaded from: 
#              http://ladsweb.nascom.nasa.gov/data/search.html
#
#     Crop and FIA data used for more crop and tree classes
#     1. US FIA tree species fractions at county
#     2. US NASS crop fractions at county
#     3. CAN crop fractions at crop divisions
#     4. Class names for all crops and trees
#
#     FIA, crop tables at county level are processed from FIA and crop census data from US and CAN
#
#     Shapefiles used:
#     1. US county shapefile and ID attribute
#     2. CAN census division shapefile and ID attribute
#
# Written by the Institute for the Environment at UNC, Chapel Hill
# in support of the EPA CMAS Modeling, 2012-2013.
#
# Developed by: L.R., 2011
#               2012-2013 updated for MODIS tiled land cover data sets
#               2014 updated for NLCD2001
#
# Usgae: landuseTool_WRFCMAQ_BELD4.csh
#        It calls computeGridLandUse_beld4.exe
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

setenv GRID_NAME  "TX4KM"

#=================================================================
#
# Define input file which contains NLCD and MODIS land cover files 
# 
#
#setenv INPUT_FILE_LIST         "../data/nlcd_modis_files_2006.txt"
setenv INPUT_FILE_LIST         "../data/nlcd_modis_files_2006_20141010.txt"


#
#INCLUDE data selection
#
setenv INCLUDE_NLCD                  YES
setenv INCLUDE_MODIS                 YES


#===================================================================
#
# Define county shapefiles, FIA data set, NASS data set, CAN crop data
#
setenv COUNTY_SHAPEFILE      "../data/county_pophu02_48st.shp"
setenv COUNTY_FIPS_ATTR      CNTYID

#
# BELD3 FIA tree fractions at county level for 1990s
#
setenv US_COUNTY_FIA_FILE    "../data/beld3-fia.dat"

#
#new NASS crop/pasture fractions at county level for 2001 or 2006
#
setenv US_COUNTY_NASS_FILE   "../data/nass2006_beld4_ag.dat"


#
#CAN crop division or census division shapefile
#Only divisions have crop data are incldued in order to reduce the size of 30m rasterized shapefile
#
setenv CAN_COUNTY_SHAPEFILE      "../data/can2006_cd_sel.shp"
setenv CAN_COUNTY_FIPS_ATTR      AGUID

#
# CAN crop fraction table at crop divisions for 2001 or 2006
#
setenv CAN_CROP_FILE  "../data/can2006_beld4_ag.dat"


#
#table contains class names for land cover data and canopy FIA trees
#
setenv BELD4_CLASS_NAMES     "../data/beld4_class_names_40classes.txt"


#===================================================================
#
# Output files
#
setenv OUTPUT_LANDUSE_TEXT_FILE      ../output/beld4/beld4_2006_tx4km.txt
setenv OUTPUT_LANDUSE_NETCDF_FILE    ../output/beld4/beld4_2006_tx4km.nc


#
# run the tool
#
../bin/64bits/computeGridLandUse_beld4.exe

#===================================================================
