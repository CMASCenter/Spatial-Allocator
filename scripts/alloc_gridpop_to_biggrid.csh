#! /bin/csh -f
#******************* Allocate Shapefiles Run Script **************************
# Allocates a polygon shapefile's data to a Regular Grid
# March 2005, BB
# Dec. 2007, LR -- projection specification changes
#*****************************************************************************

#if grid_pophous does not exist, run the alloc_census_tracts_grid.csh file

setenv DEBUG_OUTPUT Y

# Set executable
setenv EXE "$SA_HOME/bin/32bits/allocator.exe"

# Set Input Directory
setenv DATADIR $SA_HOME/data
setenv OUTPUT $SA_HOME/output

if( ! -f $OUTPUT/grid_pophous.dbf ) then
./alloc_census_tracts_to_grid.csh
endif

# Select method of spatial analysis

setenv MIMS_PROCESSING ALLOCATE

setenv TIME time

#set "data" shapefile parameters
setenv GRIDDESC $DATADIR/GRIDDESC.txt

#set parameters for file being allocated
setenv INPUT_FILE_NAME $OUTPUT/grid_pophous
setenv INPUT_FILE_TYPE ShapeFile
setenv INPUT_FILE_MAP_PRJN M08_NASH
setenv INPUT_FILE_ELLIPSOID "+a=6370000.0,+b=6370000.0"
setenv ALLOCATE_ATTRS POP2000,HOUSEHOLDS
setenv ALLOC_MODE_FILE ALL_AGGREGATE

# Set name and path of resulting shapefile
setenv OUTPUT_FILE_NAME $OUTPUT/16km_pophous
setenv OUTPUT_FILE_TYPE RegularGrid
setenv OUTPUT_GRID_NAME M_16_99NASH
setenv OUTPUT_FILE_MAP_PRJN M_16_99NASH
setenv OUTPUT_FILE_ELLIPSOID "+a=6370000.0,+b=6370000.0"

#echo "Allocating gridded census tracts to larger grid"
$TIME $EXE
