#!/bin/csh -f
#******************* Convert Shape Run Script **************************
# This script converts shape file from one map projection to another
#
# The default map projection in this script is set to that of the input
# data files in the data directory.
#
# Note: the .proj component of the shape file is not created.
# You must provide the shape file input and output names on the command
# line.
#
#****************************************************************************

# Set debug output
setenv DEBUG_OUTPUT Y

# Location of executable
setenv EXE $SA_HOME/bin/32bits/allocator.exe

if ($#argv < 2) then
  echo "Usage: convert_shape.csh input_shapefile output_shapefile (no file extensions)"
  exit 2
endif

setenv MIMS_PROCESSING CONVERT_SHAPE

setenv OUTPUT_FILE_TYPE    RegularGrid	   # Type of grid
setenv INPUT_FILE_TYPE   ShapeFile	   # Type of input data file
setenv INPUT_FILE_NAME    $argv[1]       # shape file name - no extension
setenv OUTPUT_FILE_NAME   $argv[2]    # shape file name - no extension

#set input projection for nashville grid to convert outputs to ll
#setenv INPUT_FILE_MAP_PRJN "+proj=lcc,+lat_1=30,+lat_2=60,+lat_0=40,+lon_0=-100"

#set input projection to EPA Lambert to convert surrogate input files to ll
setenv INPUT_FILE_MAP_PRJN "+proj=lcc,+lat_1=33,+lat_2=45,+lat_0=40,+lon_0=-97"
setenv INPUT_FILE_ELLIPSOID  "+a=6370000.00,+b=6370000.00"
setenv OUTPUT_FILE_MAP_PRJN   "+proj=latlong"       # map projection for data poly file
setenv OUTPUT_FILE_ELLIPSOID  "+a=6370000.00,+b=6370000.00"

# ssusuage is good on the SGI for mem & CPU usage
#setenv TIME ssusage
setenv TIME time

echo "Converting from $INPUT_FILE_MAP_PRJN to $OUTPUT_FILE_MAP_PRJN"
echo "Input file = $INPUT_FILE_NAME"
echo "Output file = $OUTPUT_FILE_NAME"
$TIME $EXE
