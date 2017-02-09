#! /bin/csh -f
#******************* Allocate Shapefiles Run Script **************************
# Allocates point source data into a polygon
# March 2005, BB
# Dec. 2007, LR -- projection specification changes
#*****************************************************************************

setenv DEBUG_OUTPUT Y

# Set executable
setenv EXE "$SA_HOME/bin/32bits/allocator.exe"

# Set Input Directory
setenv DATADIR $SA_HOME/data
setenv OUTPUT $SA_HOME/output

# Select method of spatial analysis
setenv MIMS_PROCESSING ALLOCATE

setenv TIME time

# Set name and path of shapefile being allocated to
setenv OUTPUT_POLY_FILE $DATADIR/cnty_tn
setenv OUTPUT_POLY_MAP_PRJN "+proj=latlong"
setenv OUTPUT_POLY_ELLIPSOID "+a=6370000.0,+b=6370000.0"
setenv OUTPUT_POLY_ATTRS FIPS_CODE,COUNTY
setenv OUTPUT_POLY_TYPE ShapeFile

# Set Shapefile from which to allocate data
setenv INPUT_FILE_NAME $DATADIR/tn_ports
setenv INPUT_FILE_TYPE ShapeFile
setenv INPUT_FILE_XCOL LONG
setenv INPUT_FILE_YCOL LAT
setenv INPUT_FILE_DELIM COMMA
setenv INPUT_FILE_MAP_PRJN "+proj=lcc,+lat_1=33,+lat_2=45,+lat_0=40,+lon_0=-97"
setenv INPUT_FILE_ELLIPSOID "+a=6370000.0,+b=6370000.0"
setenv ALLOCATE_ATTRS BERTHS
setenv ALLOC_MODE_FILE ALL_AGGREGATE

# Set name and path of resulting shapefile
setenv OUTPUT_FILE_NAME $OUTPUT/agg_berths
setenv OUTPUT_FILE_TYPE ShapeFile 
setenv OUTPUT_FILE_MAP_PRJN "+proj=latlong"
setenv OUTPUT_FILE_ELLIPSOID "+a=6370000.0,+b=6370000.0"

#echo "Allocating ports to counties"
$TIME $EXE
