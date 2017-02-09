#! /bin/csh -f
#******************* Overlay Shapefiles Run Script **************************
# Overlays a set of points (airports)  with a Regular Grid
# August 2006, AME
# Dec. 2007, LR -- projection specification changes
#*****************************************************************************


setenv DEBUG_OUTPUT Y

# Set executable
setenv EXE "$SA_HOME/bin/64bits/allocator.exe"

# Set Input Directory
setenv DATADIR $SA_HOME/data
setenv OUTPUT $SA_HOME/output

# Select method of spatial analysis

setenv MIMS_PROCESSING OVERLAY

setenv TIME time

#set "data" shapefile parameters
setenv GRIDDESC $DATADIR/GRIDDESC.txt

#set parameters for file being allocated
setenv INPUT_FILE_NAME $DATADIR/airports.txt
setenv INPUT_FILE_DELIMITER COMMA
setenv INPUT_FILE_TYPE PointFile
setenv INPUT_FILE_MAP_PRJN "+proj=latlong"
setenv INPUT_FILE_XCOL Longitude
setenv INPUT_FILE_YCOL Latitude
setenv INPUT_FILE_ELLIPSOID "+a=6370000.0,+b=6370000.0"
setenv INPUT_FILE_DELIM COMMA

setenv OVERLAY_TYPE RegularGrid
setenv OVERLAY_SHAPE M08_NASH
setenv OVERLAY_MAP_PRJN M08_NASH
setenv OVERLAY_ELLIPSOID "+a=6370000.0,+b=6370000.0"
setenv OVERLAY_ATTRS ALL
setenv OVERLAY_OUT_TYPE DelimitedFile
setenv OVERLAY_OUT_NAME $OUTPUT/Airports_overlay.csv
setenv OVERLAY_OUT_DELIM COMMA
setenv OVERLAY_OUT_CELLID YES
setenv WRITE_HEADER Y

echo "Overlay airports with a grid"
$TIME $EXE
