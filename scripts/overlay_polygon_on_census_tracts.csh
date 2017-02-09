#! /bin/csh -f
#******************* Overlay Run Script **************************************
#
#  Example of Overlay mode for MIMS Spatial Allocator
#  This example overlays census tracts with a polygon file containing
#  the coordinates for the bounding box of the M08_NASH grid
#  Note:  A polygon file is an ASCII csv file and is not the same as
#  a polygon shapefile, which is a binary file
#  Created 4/18/2005 by Ben Brunk, Carolina Environmental Program
#
#  Dec. 2007, LR -- projection specification changes
#*****************************************************************************

# Set debug output
setenv DEBUG_OUTPUT Y

# Set executable
setenv EXE "$SA_HOME/bin/64bits/allocator.exe"

# Set Input Directory
setenv DATADIR $SA_HOME/data
setenv OUTPUT $SA_HOME/output

# Select method of spatial analysis

setenv MIMS_PROCESSING OVERLAY

setenv TIME time

setenv OVERLAY_SHAPE $DATADIR/polygonfile.csv
setenv OVERLAY_TYPE PolygonFile
setenv OVERLAY_MAP_PRJN "+proj=lcc,+lon_0=-100.0,+lat_1=30.0,+lat_2=60.0,+lat_0=40.0"
setenv OVERLAY_DELIM COMMA
setenv OVERLAY_ELLIPSOID "+a=6370000.0,+b=6370000.0"
setenv OVERLAY_ATTRS FIPSSTCO,POP2000,HOUSEHOLDS
setenv INPUT_FILE_NAME $DATADIR/shapefiles/tn_pophous
setenv INPUT_FILE_TYPE ShapeFile
setenv INPUT_FILE_ELLIPSOID "+a=6370000.0,+b=6370000.0"
setenv INPUT_FILE_MAP_PRJN "+proj=lcc,+lon_0=-100.0,+lat_1=30.0,+lat_2=60.0,+lat_0=40.0"
setenv INPUT_FILE_DELIM COMMA
setenv GRIDDESC $DATADIR/GRIDDESC.txt
setenv OVERLAY_OUT_TYPE DelimitedFile
setenv OVERLAY_OUT_NAME $OUTPUT/overlay_census_tracts_over_polygonfile.csv
setenv OVERLAY_OUT_DELIM COMMA
setenv WRITE_HEADER Y


echo "Overlaying census tracts with a polygon"
$TIME $EXE
