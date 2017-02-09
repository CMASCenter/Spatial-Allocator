#! /bin/csh -f
#******************* Allocate grid to grid script **************************
# Allocates an IOAPI grid to another IOAPI grid
# March 2005, BB
# Dec. 2007, LR 
# May 2013, LR
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

setenv GRIDDESC ../data/GRIDDESC.txt

#set parameters for file being allocated
setenv INPUT_FILE_NAME $DATADIR/GRIDCRO2D_Texas2000
setenv INPUT_FILE_TYPE IoapiFile
setenv INPUT_FILE_MAP_PRJN 36km_Texas2000
setenv INPUT_FILE_ELLIPSOID "+a=6370000.0,+b=6370000.0"
setenv ALLOCATE_ATTRS ALL
setenv ALLOC_MODE_FILE ALL_AGGREGATE

# Set name and path of resulting output file
setenv OUTPUT_FILE_NAME $OUTPUT/CRO2D_test
setenv OUTPUT_FILE_TYPE IoapiFile
setenv OUTPUT_GRID_NAME US36
setenv OUTPUT_FILE_MAP_PRJN US36
setenv OUTPUT_FILE_ELLIPSOID "+a=6370000.0,+b=6370000.0"

if (-e  $OUTPUT/CRO2D_test) rm $OUTPUT/CRO2D_test

$TIME $EXE
