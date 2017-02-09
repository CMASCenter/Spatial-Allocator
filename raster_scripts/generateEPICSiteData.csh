#!/bin/csh -f
#******************************************************************************
# Purpose:  to generate EPIC site table for EPIC modeling from:
#     1. Grid description 
#     2. BELD4 NetCDF file - contains CROPF variable - 40 classes
#     3. US county shapefile with attributes: STATE(string), COUNTY(string), FIPS(string), COUNTRY(string), STATEABB(string), REGION10(short)
#     4. North American political state shapefile with attributes: COUNTRY(string), STATEABB(string)
#     5. US 8-digit HUC boundary shapefile with attribute: HUC_8(string)
#     6. DEM elevation data: meters and missing value=-9999
#     7. DEM slope data: 0 to 90 degree with scalar 0.01 and missing value -9999
#
#
# Written by the Institute for the Environment at UNC, Chapel Hill
# in support of the EPA CMAS contract, 2012-2013.
#
# Written by:   L. Ran,  
#               2012/11 - 10/2013
#
# Usgae: compute_EPICSiteData.csh
#        Users can modify the script file for their application
#
# Tool called: All environment variables needed by the tool 
#              are defined in this script file.
#
#*******************************************************************************

#================================================================
#
# Define domain grid
#
setenv GRID_PROJ "+proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=33 +lat_2=45 +lat_0=40 +lon_0=-97"

#CMAQ 12km

setenv GRID_ROWS 299
setenv GRID_COLUMNS 459

setenv GRID_XMIN  -2556000.0
setenv GRID_YMIN  -1728000.0

setenv GRID_XCELLSIZE 12000
setenv GRID_YCELLSIZE 12000

setenv GRID_NAME  "CMAQ12KM"


#=============================================
#
# Set minimum crop acres for site selection
# Grids selected for EPIC modeling will have at
# least one crop with acreage >= the minimum acres
#

setenv MINIMUM_CROP_ACRES  40

#=================================================================
#
# Define BELD4 input file 
#
#setenv DOMAIN_BELD4_NETCDF     ../output/beld4/beld4_4CALNEX1_2006.nc

setenv DOMAIN_BELD4_NETCDF       /nas01/depts/ie/cempd/EPIC/data/output/beld4/beld4_40/new_beld4_2006_12kmUS.nc

#
# Define US county shapefiles with     
#

setenv  FESTC_GISDIR   /nas01/depts/ie/cempd/apps/sallocator/festc/epic/common_data/gisFiles

setenv COUNTY_SHAPEFILE      ${FESTC_GISDIR}/co99_d00_conus_cmaq_epic.shp

#
# Define North American State political boundary shapefile
#
setenv COUNTRY_SHAPEFILE   ${FESTC_GISDIR}/na_bnd_camq_epic.shp

#
# US 8-digit HUC shapefile
#
setenv HUC8_SHAPEFILE   ${FESTC_GISDIR}/conus_hucs_8_cmaq.shp

#
# Define Elevation image
#
setenv ELEVATION_IMAGE   ${FESTC_GISDIR}/na_dem_epic.img


#
# Define slope image
#
setenv SLOPE_IMAGE   ${FESTC_GISDIR}/na_slope_epic.img

#=================================================================


#
# Output files
#
setenv OUTPUT_TEXT_FILE      ../output/epic/EPICSites_Info_cmaq12km.csv

setenv OUTPUT_TEXT_FILE2     ../output/epic/EPICSites_Crop_camq12km.csv

#
# Run the tool
#
../bin/64bits/compute_EPICSiteData.exe

#===================================================================
