#! /bin/csh -f
#******************* Overlay Run Script **************************************
#
#  Example of Overlay mode for MIMS Spatial Allocator
#  Example uses a Bounding Box (grid) overlaying a point shapefile
#  Created 3/30/2005 by Ben Brunk, Carolina Environmental Program
#
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

setenv OVERLAY_SHAPE 1000000.0,-536000.0,1368000.0,-200000.0
setenv OVERLAY_TYPE BoundingBox
setenv OVERLAY_MAP_PRJN "+proj=lcc,+lon_0=-100.0,+lat_1=30.0,+lat_2=60.0,+lat_0=40.0"
setenv OVERLAY_ELLIPSOID "+a=6370000.00,+b=6370000.00"
setenv OVERLAY_ATTRS NAME,BERTHS,LAT,LONG
setenv INPUT_FILE_NAME $DATADIR/shapefiles/tn_ports
setenv INPUT_FILE_TYPE ShapeFile
setenv INPUT_FILE_ELLIPSOID "+a=6370000.00,+b=6370000.00"
setenv INPUT_FILE_MAP_PRJN "+proj=lcc,+lon_0=-100.0,+lat_1=30.0,+lat_2=60.0,+lat_0=40.0"
setenv OVERLAY_OUT_TYPE DelimitedFile
setenv OVERLAY_OUT_NAME $OUTPUT/ports_over_grid.csv
setenv OVERLAY_OUT_DELIM COMMA
setenv WRITE_HEADER Y

echo "Overlaying ports with a grid (bounding box of grid)"
$TIME $EXE
