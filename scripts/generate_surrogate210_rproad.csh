#!/bin/csh -f
#******************* Generate Rural Primary Road Miles Surrogate 210 Run Script ***********
# This script generates surrogate for the spatial vector tool test case
# (8km over Tennessee).
#
# Script created by : Limei Ran, Institute for the Environment, UNC-CH
#                     Nov. 2008
#****************************************************************************
# Set Environment variables for computation of surrogate -- USA_210=Rural Primary Road Miles
setenv SURROGATE_ID 210                          

# Set installation directory with subdir: bin, output, data

setenv DEBUG_OUTPUT Y 

# Location of input shapefiles
setenv DATA $SA_HOME/data

# Set output grid domain information
setenv OUTPUT_FORMAT          SMOKE
setenv OUTPUT_FILE_TYPE       RegularGrid
setenv GRIDDESC               $DATA/GRIDDESC.txt      # Grid description file
setenv OUTPUT_GRID_NAME       M08_NASH             # Grid name
setenv OUTPUT_FILE_ELLIPSOID  "+a=6370000.0,+b=6370000.0"    # MM5 shphere, Sphere, Ellipsoid or Datum
#setenv OUTPUT_FILE_ELLIPSOID  "+a=6370000.0,+b=6370000.0"    # WRF shphere

# Set output directory
setenv OUTPUT $SA_HOME/output/$OUTPUT_GRID_NAME
if (! -d $OUTPUT) mkdir -p $OUTPUT

#Set data shapefile information for emission inventory boundareis 
setenv DATA_FILE_NAME_TYPE    ShapeFile
setenv DATA_FILE_NAME         $DATA/shapefiles/cnty_tn         # shapefile with counties 
setenv DATA_FILE_ELLIPSOID    "+a=6370000.0,+b=6370000.0"
setenv DATA_FILE_MAP_PRJN     "+proj=latlong"       # map projection for data poly file
setenv DATA_ID_ATTR           FIPS_CODE          # attribute to report surrogates by

#Set weight shapefile for surrogate computation
setenv WEIGHT_FILE_TYPE       ShapeFile
setenv WEIGHT_FILE_NAME       $DATA/shapefiles/tn_usrds_2000   # weighted shapefile
setenv WEIGHT_FILE_ELLIPSOID  "+a=6370000.0,+b=6370000.0"
setenv WEIGHT_FILE_MAP_PRJN   "+proj=lcc,+lat_1=33,+lat_2=45,+lat_0=40,+lon_0=-97"
setenv WEIGHT_FUNCTION        NONE              # can be attribute function 
setenv WEIGHT_ATTR_LIST       NONE              # can be: attributes or USE_FUNCTION when WEIGHT_FUNCTION!=NONE
setenv FILTER_FILE            $DATA/filter_USA_${SURROGATE_ID}.txt  
# FILTER_FILE can be NONE or a txt file with shapefile selection conditions
setenv FILTERED_WEIGHT_SHAPES        $OUTPUT/filtered_tn_roads_USA_${SURROGATE_ID}   
# FILTERED_WEIGHT_SHAPES is the filtered weight shapefile output when FILTER_FILE is not NONE

#Set output file information
setenv WRITE_HEADER YES
setenv WRITE_QASUM YES
setenv WRITE_SRG_NUMERATOR YES
setenv WRITE_SRG_DENOMINATOR YES
setenv DENOMINATOR_THRESHOLD 0.0005              #get rid of slivers along boundary overlays

# Set output surrogate and shapefile file names
setenv SURROGATE_FILE  $OUTPUT/USA_${SURROGATE_ID}_NOFILL.txt      #txt surrogate output 
setenv OUTPUT_FILE_NAME   $OUTPUT/grid_USA_${SURROGATE_ID}         #shapefile name with grid surrogate data 

#run the exe file with CPU and memory usage
echo "Generate Rural Primary Road Miles surrogate =" $SURROGATE_ID
time $SA_HOME/bin/32bits/srgcreate.exe

if ( $status == 0 ) then
         echo "Surrogate script run completed."
      else
         echo "Error generating surrogate for category $SURROGATE_ID"
         exit 2
endif
