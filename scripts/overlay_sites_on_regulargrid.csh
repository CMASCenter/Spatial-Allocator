#! /bin/csh -f
#******************* Overlay Run Script **************************************
#
#  Example of Overlay mode for Spatial Allocator
#  This example uses a shapefile overlaying a Regular Grid
#
#  Updated:  March 2006 provided support for egrid.  LR
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

setenv OVERLAY_SHAPE US36
setenv OVERLAY_TYPE RegularGrid
setenv OVERLAY_MAP_PRJN US36
setenv OVERLAY_ELLIPSOID "+a=6370000.0,+b=6370000.0"
setenv GRIDDESC $DATADIR/GRIDDESC.txt
setenv OVERLAY_OUT_TYPE DelimitedFile
setenv OVERLAY_OUT_NAME $OUTPUT/us36_grid_point.csv
setenv OVERLAY_OUT_DELIM COMMA
setenv OVERLAY_OUT_CELLID YES
# will print row and col
# problem with this method is that ROW and COL are not avaiable to pass through
#setenv OVERLAY_ATTRS Site_ID
setenv OVERLAY_ATTRS ALL
setenv INPUT_FILE_NAME $DATADIR/CASTNET_locations.txt
setenv INPUT_FILE_TYPE PointFile
setenv INPUT_FILE_XCOL Longitude
setenv INPUT_FILE_YCOL Latitude
setenv INPUT_FILE_DELIM SEMICOLON
setenv INPUT_FILE_MAP_PRJN "+proj=latlong"
setenv INPUT_FILE_ELLIPSOID "+a=6370000.0,+b=6370000.0"
setenv WRITE_HEADER Y

echo "Overlaying egrid with PointFile"
$TIME $EXE
