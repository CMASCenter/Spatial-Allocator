#! /bin/csh -f
#******************* Allocate Run Script *************************************
# Allocates an I/O API file onto a different map projection
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

#set "data" shapefile parameters
setenv GRIDDESC $DATADIR/GRIDDESC.txt

#set parameters for file being allocated
setenv INPUT_FILE_NAME $DATADIR/hr1800_2001.ioapi
setenv INPUT_FILE_TYPE IoapiFile
setenv INPUT_GRID_NAME METCRO_36KM_CROS
setenv INPUT_FILE_MAP_PRJN METCRO_36KM_CROS
setenv INPUT_FILE_ELLIPSOID "+a=6370000.0,+b=6370000.0"
setenv ALLOCATE_ATTRS ALL

setenv ALLOC_MODE_FILE ALL_AVERAGE
# Whether you select ALL_AGGREGATE or ALL_AVERAGE depends on the
# type of data.  Population data would typically be aggregated, but
# emission densities for example would typically be averaged. 
# The difference is in whether the result is divided by the size of
# the output grid cell.
# setenv ALLOC_MODE_FILE ALL_AGGREGATE

# Set name and path of resulting shapefile
setenv OUTPUT_FILE_NAME $OUTPUT/newFile.ioapi
setenv OUTPUT_FILE_TYPE IoapiFile
setenv OUTPUT_GRID_NAME METCRO_cus_CROSS
setenv OUTPUT_FILE_MAP_PRJN METCRO_cus_CROSS
setenv OUTPUT_FILE_ELLIPSOID "+a=6370000.0,+b=6370000.0"

#echo "Allocating I/O API file to new map projection"
$TIME $EXE
