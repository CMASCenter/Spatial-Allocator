#! /bin/csh -f
#******************* Allocate Shapefiles Run Script **************************
# Allocates a polygon to a grid, tests the discrete overlap function
# March 2005, BB
# Dec. 2007, LR -- projection specification changes
#*****************************************************************************
setenv DEBUG_OUTPUT Y

# Set executable
setenv EXE "$SA_HOME/bin/32bits/allocator.exe"

# Set Input Directory
setenv DATADIR $SA_HOME/data
setenv OUTPUT $SA_HOME/output


if(! -f $DATADIR/discrete_modes.txt) then
echo "Generating mode file"
echo "ATTRIBUTE=FIPS_CODE:DISCRETECENTROID" > $DATADIR/discrete_modes.txt
echo "ATTRIBUTE=COUNTY:DISCRETEOVERLAP" >> $DATADIR/discrete_modes.txt
endif



# Select method of spatial analysis

setenv MIMS_PROCESSING ALLOCATE

setenv TIME time

#set "data" shapefile parameters
setenv GRIDDESC $DATADIR/GRIDDESC.txt

#set parameters for file being allocated
setenv INPUT_FILE_NAME $DATADIR/shapefiles/cnty_tn
setenv INPUT_FILE_TYPE ShapeFile
setenv INPUT_FILE_MAP_PRJN "+proj=latlong"
setenv INPUT_FILE_ELLIPSOID "+a=6370000.0,+b=6370000.0"
setenv ALLOCATE_ATTRS FIPS_CODE,COUNTY
setenv ALLOC_MODE_FILE $DATADIR/discrete_modes.txt

# Set name and path of resulting shapefile
setenv OUTPUT_FILE_NAME $OUTPUT/gridded_county
setenv OUTPUT_FILE_TYPE RegularGrid
setenv OUTPUT_GRID_NAME M08_NASH
setenv OUTPUT_FILE_MAP_PRJN M08_NASH
setenv OUTPUT_FILE_ELLIPSOID "+a=6370000.0,+b=6370000.0"

#echo "Allocating counties to a grid"
$TIME $EXE
