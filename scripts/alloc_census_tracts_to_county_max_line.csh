#! /bin/csh -f
#******************* Allocate Shapefiles Run Script **************************
# Example of how to allocate data from one polygon shapefile to another
# polygon shapefile with added resolution using MAX_LINE_SEG env. variable.
# Note:  Output will remain the same except for greater resolution in shapes.
#
# May 2005, BDB
# Dec. 2007, LR  projection specification changes
#*****************************************************************************

setenv DEBUG_OUTPUT Y

setenv MAX_LINE_SEG 1000

# Set executable
setenv EXE "$SA_HOME/bin/64bits/allocator.exe"

# Set Input Directory
setenv DATADIR $SA_HOME/data
setenv OUTPUT $SA_HOME/output

# Select method of spatial analysis

setenv MIMS_PROCESSING ALLOCATE

setenv TIME time

# Set name and path of shapefile having data allocated to it
setenv OUTPUT_POLY_FILE $DATADIR/shapefiles/cnty_tn
setenv OUTPUT_POLY_MAP_PRJN "+proj=latlong"
setenv OUTPUT_POLY_ELLIPSOID "+a=6370000.0,+b=6370000.0"
setenv OUTPUT_POLY_ATTRS COUNTY,FIPS_CODE
setenv OUTPUT_POLY_TYPE ShapeFile

# Set Shapefile from which to allocate data
setenv INPUT_FILE_NAME $DATADIR/shapefiles/tn_pophous
setenv INPUT_FILE_TYPE ShapeFile
setenv INPUT_FILE_MAP_PRJN "+proj=lcc,+lat_1=33,+lat_2=45,+lat_0=40,+lon_0=-97"
setenv INPUT_FILE_ELLIPSOID "+a=6370000.0,+b=6370000.0"
setenv ALLOCATE_ATTRS POP2000,HOUSEHOLDS,HDENS
setenv ALLOC_MODE_FILE $DATADIR/atts_pophous.txt

# Set name and path of resulting shapefile
setenv OUTPUT_FILE_NAME $OUTPUT/county_pophous_line1000
setenv OUTPUT_FILE_TYPE ShapeFile 
setenv OUTPUT_FILE_MAP_PRJN "+proj=latlong"
setenv OUTPUT_FILE_ELLIPSOID  "+a=6370000.0,+b=6370000.0"


echo "Allocating census population tracts to counties"
$TIME $EXE
