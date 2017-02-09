#!/bin/csh -f
#BSUB -J beld4smk
#BSUB -o beld4smk_32bits.log
#BSUB -P EPIC
#BSUB -q week
#BSUB -R mem96 -M 24
#
#******************* Beld4smk Run Script *************************************
# Runs beld4smk for sample modeling grid
# Sep 2015, LR 
#*****************************************************************************

setenv DEBUG_OUTPUT Y 

# Set executable
setenv  EXE $SA_HOME/bin/32bits/beld4smk.exe
setenv  ALLOCATOR_EXE $SA_HOME/bin/32bits/allocator.exe
#setenv  EXE /nas01/depts/ie/cempd//SMOKE/dyang/sa/src/vector64_pgi/Linux2_x86pg_pgcc_nomp/beld4smk.exe
#setenv  ALLOCATOR_EXE /nas01/depts/ie/cempd//SMOKE/dyang/sa/src/vector64_pgi/Linux2_x86pg_pgcc_nomp/allocator.exe

# Set Input Directory
setenv DATADIR $SA_HOME/data

# Set output Directory -- the directory has to exist!
setenv OUTPUT  $SA_HOME/output/beld4_32bits

setenv TIME time

# Set program parameters
#setenv OUTPUT_GRID_NAME M08_NASH
setenv OUTPUT_GRID_NAME US36
setenv OUTPUT_FILE_TYPE RegularGrid
setenv OUTPUT_FILE_ELLIPSOID "+a=6370000.00,+b=6370000.00"
setenv INPUT_DATA_DIR $DATADIR/beld4/
setenv GRIDDESC $DATADIR/GRIDDESC.txt
setenv TMP_DATA_DIR $OUTPUT/tmp/
setenv INPUT_FILE_PREFIX  b4.1_master
setenv OUTPUT_FILE_PREFIX $OUTPUT/beld4_${OUTPUT_GRID_NAME}_output

# Create temporary data directory if needed
if(! -e $TMP_DATA_DIR) mkdir -p $TMP_DATA_DIR

$TIME $EXE 

