#! /bin/csh -f 
#******************* Beld3smk Run Script *************************************
# Runs beld3smk for sample modeling grid
# July 2005, CAS 
#*****************************************************************************

setenv DEBUG_OUTPUT N 

# Set executable
setenv EXE $SA_HOME/bin/32bits/diffioapi.exe

# Set Input Directory
setenv DATADIR $SA_HOME/data/beld
setenv OUTPUT  $SA_HOME/output/beld3

# set script variables
set grid = M08_NASH
set eflag = 0

# Set program parameters
setenv TOLERANCE 0.01

# compare "a" file 
setenv ORIG_FILE $DATADIR/beld3_${grid}_output_a.ncf
setenv NEW_FILE $OUTPUT/beld3_${grid}_output_a.ncf

echo "Comparing $NEW_FILE"
echo "to $ORIG_FILE"

$EXE 

if($status != 0) then
  echo "ERROR comparing beld3_${grid}_output_a.ncf files."
  set eflag = 1
endif

# compare "b" file
setenv ORIG_FILE $DATADIR/beld3_${grid}_output_b.ncf
setenv NEW_FILE $OUTPUT/beld3_${grid}_output_b.ncf

echo "Comparing $NEW_FILE"
echo "to $ORIG_FILE"

$EXE 

if($status != 0) then
  echo "ERROR comparing beld3.${grid}_output_b.ncf files."
  set eflag = 1
endif

# compare "tot" file
setenv ORIG_FILE $DATADIR/beld3_${grid}_output_tot.ncf
setenv NEW_FILE $OUTPUT/beld3_${grid}_output_tot.ncf

echo "Comparing $NEW_FILE"
echo "to $ORIG_FILE"

$EXE 

if($status != 0) then
  echo "ERROR comparing beld3_${grid}_output_tot.ncf files."
  set eflag = 1
endif

if($eflag == 1) then
  echo ""
  echo "RESULTS: At least one pair of files had differences larger than the tolerance - see above for details."
else
  echo "RESULTS: All files were the same within the specified tolerance."
endif

exit(0)

