#!/bin/csh -fx
#******************* Filtered Surrogate Run Script **************************
# This script generates a filtered surrogate based on the Spatial 
# Allocator test case (8km over Tennessee).
#
# Be sure to specify FILTERED_WEIGHT_SHAPES and FILTER_FILE, as those are the
# main things different from generating an ordinary surrogate.
# 
# Script created by : Alison Eyth, Carolina Environmental Program
# Last edited : June 2005
# 
# Dec. 2007, LR -- projection specification changes
#*********************************************************************

# Set installation directory

# Set directory for output surrogate and shape files

setenv WORK_DIR $SA_HOME/output

# Set Location of shapefiles
setenv DATA $SA_HOME/data

mkdir $WORK_DIR
setenv DEBUG_OUTPUT Y

# Set Grid settings
setenv GRIDDESC $DATA/GRIDDESC.txt
setenv OUTPUT_GRID_NAME M08_NASH

# Set Location of executable
setenv EXE $SA_HOME/bin/32bits/srgcreate.exe

# Set name and path to temporary surrogate file
setenv SURROGATE_FILE $WORK_DIR/tmp_srg.$OUTPUT_GRID_NAME.txt

# Set name and path to final merged surrogate file from spatial tool  
setenv SRG_FILE $WORK_DIR/filtered_srg_$OUTPUT_GRID_NAME.txt

# mode of operation for program
#setenv MIMS_PROCESSING SURROGATE

# WRITE_QASUM=YES prints surrogate sums by county in file
# WRITE_SRG_NUMERATOR=YES writes surrogate numerator as comment in file
# WRITE_SRG_DENOMINATOR=YES writes denminator (county totals) for srg weight
setenv WRITE_QASUM YES
setenv WRITE_SRG_NUMERATOR YES
setenv WRITE_SRG_DENOMINATOR YES

# Print header info
setenv WRITE_HEADER NO

# Specify type of data files to use
setenv OUTPUT_FILE_TYPE RegularGrid
setenv OUTPUT_FILE_ELLIPSOID  "+a=6370000.0,+b=6370000.0"
setenv DATA_FILE_NAME_TYPE ShapeFile
setenv WEIGHT_FILE_TYPE ShapeFile

# The data polygons should be the shape file containing polygons for the grid
setenv DATA_FILE_NAME $DATA/cnty_tn
setenv DATA_ID_ATTR FIPS_CODE
setenv DATA_FILE_MAP_PRJN "+proj=latlong"
setenv DATA_FILE_ELLIPSOID "+a=6370000.0,+b=6370000.0"

# An example of using a non-default named ellipsoid:
# set DATA_FILE_ELLIPSOID +datum=NAD27

# Set weight projection to that of the EPA files
setenv WEIGHT_FILE_MAP_PRJN "+proj=lcc,+lat_1=33,+lat_2=45,+lat_0=40,+lon_0=-97"
setenv WEIGHT_FILE_ELLIPSOID "+a=6370000.0,+b=6370000.0"

# Set the WEIGHT_FUNCTION and FILTER_FILE to NONE in case they were set
# from earlier executions of scripts
setenv WEIGHT_FUNCTION NONE
setenv FILTER_FILE NONE

date +%r

# Generate surrogate header line
echo Writing header
$EXE -header > $SRG_FILE

if ( $status != 0 ) then
   echo "Error writing surrogate header"
   exit
endif

echo Generating filtered population surrogate
setenv SURROGATE_ID 8
setenv WEIGHT_FILE_NAME $DATA/tn_pophous
setenv WEIGHT_ATTR_LIST POP2000
setenv FILTER_FILE $SA_HOME/data/filter_tn_pop.txt
setenv FILTERED_WEIGHT_SHAPES $WORK_DIR/tn_filtered_pop
setenv OUTPUT_FILE_NAME $WORK_DIR/filtered_pop_$OUTPUT_GRID_NAME

date +%r
$EXE

if ( $status == 0 ) then 
   more $SURROGATE_FILE >> $SRG_FILE
   rm $SURROGATE_FILE
endif
