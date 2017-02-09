#!/bin/csh -f
#******* Generate Population Surrogate 100 Run Script for polygons**********
# This script generates surrogate for the spatial vector tool test case
# (8km over Tennessee).
#
# Script created by : Limei Ran, Institute for the Environment, UNC-CH
#                     Nov. 2008
#****************************************************************************
# Set Environment variables for computation of surrogate -- USA_100=Population
setenv SURROGATE_ID 100                          #100 - Population surrogate

# Set installation directory with subdir: bin, output, data

setenv DEBUG_OUTPUT Y 

# Location of input shapefiles
setenv DATA $SA_HOME/data

# Set output grid domain information
setenv OUTPUT_FORMAT          SMOKE
setenv OUTPUT_FILE_TYPE       Polygon
setenv OUTPUT_POLY_FILE       $DATA/shapefiles/tn_pophous                        #census tract polygons as outoput grids
setenv OUTPUT_POLY_ATTR       STFID                                   #Polygon ID for surrogate output
setenv OUTPUT_FILE_MAP_PRJN   "+proj=lcc,+lat_1=33,+lat_2=45,+lat_0=40,+lon_0=-97"
setenv OUTPUT_FILE_ELLIPSOID  "+a=6370000.0,+b=6370000.0"    # WRF shphere

# Set output directory
setenv OUTPUT $SA_HOME/output/polygon
if (! -d $OUTPUT) mkdir -p $OUTPUT

#Set data shapefile information for emission inventory boundareis 
setenv DATA_FILE_NAME_TYPE    ShapeFile
setenv DATA_FILE_NAME         $DATA/shapefiles/cnty_tn         # shapefile with counties 
setenv DATA_FILE_ELLIPSOID    "+a=6370000.0,+b=6370000.0"
setenv DATA_FILE_MAP_PRJN     "+proj=latlong"       # map projection for data poly file
setenv DATA_ID_ATTR           FIPS_CODE          # attribute to report surrogates by

#Set weight shapefile for surrogate computation
setenv WEIGHT_FILE_TYPE       ShapeFile
setenv WEIGHT_FILE_NAME       $DATA/shapefiles/tn_pophous    # weighted shapefile
setenv WEIGHT_FILE_ELLIPSOID  "+a=6370000.0,+b=6370000.0"
setenv WEIGHT_FILE_MAP_PRJN   "+proj=lcc,+lat_1=33,+lat_2=45,+lat_0=40,+lon_0=-97"
setenv WEIGHT_FUNCTION        NONE               #can be ATTR1+ATTR2
setenv WEIGHT_ATTR_LIST       POP2000            # Can also be: NONE or USE_FUNCTION when WEIGHT_FUNCTION!=NONE
setenv FILTER_FILE            NONE               #txt file with shapefile selection conditions


#Set output file information
setenv WRITE_HEADER YES
setenv WRITE_QASUM YES
setenv WRITE_SRG_NUMERATOR YES
setenv WRITE_SRG_DENOMINATOR YES
setenv DENOMINATOR_THRESHOLD 0.0005              #get rid of slivers along boundary overlays

# Set output surrogate and shapefile file names
setenv SURROGATE_FILE  $OUTPUT/USA_${SURROGATE_ID}_POLY_NOFILL.txt      #txt surrogate output 
setenv OUTPUT_FILE_NAME   $OUTPUT/poly_USA_${SURROGATE_ID}         #shapefile name with grid surrogate data 

#run the exe file with CPU and memory usage
echo "Generate Population surrogate =" $SURROGATE_ID
time $SA_HOME/bin/32bits/srgcreate.exe

if ( $status == 0 ) then
     echo "Surrogate script run completed."
else
     echo "Error generating surrogate for category $SURROGATE_ID"
     exit 2
endif
