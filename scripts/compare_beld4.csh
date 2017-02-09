#! /bin/csh -f 
#******************* Beld3smk Run Script *************************************
# Runs beld3smk for sample modeling grid
# July 2005, CAS 
#*****************************************************************************

setenv DEBUG_OUTPUT N 

# Set executable
setenv EXE $SA_HOME/bin/32bits/diffioapi.exe

# Set Input Directory
setenv DATADIR $SA_HOME/data
setenv OUTPUT  $SA_HOME/output

# set script variables
set grid = M08_NASH
set eflag = 0

# Set program parameters
setenv TOLERANCE 0.01

# compare "a" file 
setenv ORIG_FILE $DATADIR/beld4_${grid}_output.ncf
setenv NEW_FILE $OUTPUT/beld4/beld4_${grid}_output.ncf

echo "Comparing $NEW_FILE"
echo "to $ORIG_FILE"

$EXE 

if($status != 0) then
  echo "ERROR comparing beld4_${grid}_output.ncf files."
  set eflag = 1
endif

if($eflag == 1) then
  echo ""
  echo "RESULTS: At least one pair of files had differences larger than the tolerance - see above for details."
else
  echo "RESULTS: All files were the same within the specified tolerance."
endif

exit(0)

