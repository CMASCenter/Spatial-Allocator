#!/bin/csh -f
#******************* Beld3smk Run Script *************************************
# Runs beld3smk for sample modeling grid
# Mar 2006, LR 
# Dec. 2007, LR
#*****************************************************************************

setenv DEBUG_OUTPUT Y 

# Set executable
setenv EXE $SA_HOME/bin/32bits/beld3smk.exe
setenv ALLOCATOR_EXE $SA_HOME/bin/32bits/allocator.exe

# Set Input Directory
setenv DATADIR $SA_HOME/data

# Set output directory -- the directory has to exist
setenv OUTPUT  $SA_HOME/output

setenv TIME time

# Set program parameters
setenv OUTPUT_GRID_NAME EGRID_359X573
setenv OUTPUT_FILE_TYPE EGrid
setenv OUTPUT_FILE_ELLIPSOID "+a=6370000.0,+b=6370000.0"
setenv OUTPUT_POLY_FILE $DATADIR/egrid_arc.dat
setenv INPUT_DATA_DIR $DATADIR/beld3/
setenv GRIDDESC $DATADIR/GRIDDESC.txt
setenv TMP_DATA_DIR $SA_HOME/tmp/
setenv OUTPUT_FILE_PREFIX $OUTPUT/beld3_${OUTPUT_GRID_NAME}_output

# Create temporary data directory if needed
if(! -e $TMP_DATA_DIR) mkdir -p $TMP_DATA_DIR

$TIME $EXE 

