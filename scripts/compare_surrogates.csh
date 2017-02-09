#!/bin/csh -f
#******************* Compare Surrogates Run Script **************************
# This script takes two sets of surrogates and compares them to one another.
# The user sets an allowable difference tolerance and the program calculates
# the differences between the two files for every entry in both surrogates 
# files.
#
# If the differences are greater than the allowable difference tolerance, 
# the program alerts the user, otherwise, the program gives a message of 
# "Success!".
#
# Script created by : C. Seppanen, MCNC-Environmental Modeling Center
# Last edited : June 2005
#
#****************************************************************************


# Set installation directory

setenv SA_HOME ..

# Set grid name
set grid = M08_NASH  # state for running surrogates

# Location of executable
set EXE = $SA_HOME/bin/64bits/diffsurr.exe

setenv NEW_SRGS  $SA_HOME/output/srg_${grid}.txt    # new merged surrogate file 
setenv REFERENCE_SRGS  $SA_HOME/data/srg_nash_ref.txt   # reference surrogates
echo "Comparing $NEW_SRGS"
echo "to $REFERENCE_SRGS"

set tolerance      = 1e-5      # difference tolerance
set eflag    = 0         # 1 - problem comparing surrogates
set category = 2         # current surrogate category 

# Loop over surrogates categories
while ( $category <= 8 )
   echo "Testing Category" $category
 
   $EXE $NEW_SRGS $category $REFERENCE_SRGS $category $tolerance

   if ( $status != 0 ) then
       echo "ERROR comparing category " $category " surrogates."
       set eflag = 1
#       goto error
   endif

   @ category = $category + 1
end

error:
if ( $eflag == 1 ) then
   echo Test suite failed!
else
   echo Test suite successful!
endif

exit ( 0 )

