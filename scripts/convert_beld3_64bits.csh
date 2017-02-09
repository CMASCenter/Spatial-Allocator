#!/bin/csh -f
#BSUB -J beld3smk
#BSUB -o beld3smk_64bits.log
#BSUB -P EPIC
#BSUB -q week

#******************* Beld3smk Run Script *************************************
# Runs beld3smk for sample modeling grid
# June 2006, LR 
# Modified Dec. 11, 2007
#*****************************************************************************

setenv DEBUG_OUTPUT Y 

# Set executable
setenv EXE $SA_HOME/bin/64bits/beld3smk.exe
setenv ALLOCATOR_EXE $SA_HOME/bin/64bits/allocator.exe

# Set Input Directory
setenv DATADIR $SA_HOME/data

# Set output Directory -- the directory has to exist!
setenv OUTPUT  $SA_HOME/output/beld3_64bits

setenv TIME time

# Set program parameters
#setenv OUTPUT_GRID_NAME M08_NASH
setenv OUTPUT_GRID_NAME US36    
setenv OUTPUT_FILE_TYPE RegularGrid
setenv OUTPUT_FILE_ELLIPSOID "+a=6370000.00,+b=6370000.00"
setenv INPUT_DATA_DIR $DATADIR/beld3/
setenv GRIDDESC $DATADIR/GRIDDESC.txt
setenv TMP_DATA_DIR $SA_HOME/tmp/
setenv OUTPUT_FILE_PREFIX $OUTPUT/beld3_${OUTPUT_GRID_NAME}_output

# Create temporary data directory if needed
if(! -e $TMP_DATA_DIR) mkdir -p $TMP_DATA_DIR

$TIME $EXE 

