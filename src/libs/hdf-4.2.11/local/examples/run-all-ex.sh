#! /bin/sh
#
# Copyright by The HDF Group.
# All rights reserved.
#
# This file is part of HDF4.  The full HDF4 copyright notice, including
# terms governing use, modification, and redistribution, is contained in
# the files COPYING and Copyright.html.  COPYING can be found at the root
# of the source code distribution tree; Copyright.html can be found at the
# root level of an installed copy of the electronic HDF5 document set and
# is linked from the top-level documents page.  It can also be found at
# http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have
# access to either file, you may request a copy from help@hdfgroup.org.

#
#  This file:  run-all-ex.sh
# Written by:  Larry Knox
#       Date:  January 17, 2014
#
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                                               #
# This script will run the scripts to compile and run the installed hdf5        #
# examples.                                                                     #
#                                                                               #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

echo "Run c examples"
if ((cd c; sh ./run-c-ex.sh) && \
   (if test -d fortran; then   
       echo "Run fortran examples" 
       cd fortran; sh ./run-fortran-ex.sh 
    fi)); then
   echo "Done"
   exit 0
else
   exit 1
fi

