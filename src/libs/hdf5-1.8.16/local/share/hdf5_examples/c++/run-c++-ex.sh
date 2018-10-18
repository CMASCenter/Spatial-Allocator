#! /bin/sh
#
# Copyright by The HDF Group.
# All rights reserved.
#
# This file is part of HDF5.  The full HDF5 copyright notice, including
# terms governing use, modification, and redistribution, is contained in
# the files COPYING and Copyright.html.  COPYING can be found at the root
# of the source code distribution tree; Copyright.html can be found at the
# root level of an installed copy of the electronic HDF5 document set and
# is linked from the top-level documents page.  It can also be found at
# http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have
# access to either file, you may request a copy from help@hdfgroup.org.

#
#  This file:  run-c++-ex.sh
# Written by:  Larry Knox
#       Date:  May 11, 2010
#
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                                               #
# This script will compile and run the c++ examples from source files           #
# installed in .../share/hdf5_examples/c++ using h5c++.  The                    #
# order for running programs with RunTest in the MAIN section below is taken    #
# from the Makefile.  The order is important since some of the test programs    #
# use data files created by earlier test programs.  Any future additions should #
# be placed accordingly.                                                        #
#                                                                               #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

# Initializations
EXIT_SUCCESS=0
EXIT_FAILURE=1

# Where the tool is installed.
# default is relative path to installed location of the tools
prefix="${prefix:-../../..}"
AR="ar"
RANLIB="ranlib"
H5TOOL="h5c++"                  # The tool name
H5TOOL_BIN="${prefix}/bin/${H5TOOL}"   # The path of the tool binary

#### Run test ####
RunTest()
{
    TEST_EXEC=$1
    Test=$1".cpp"

    echo
    echo "#################  $1  #################"
    ${H5TOOL_BIN} -o $TEST_EXEC $Test
    if [ $? -ne 0 ]
    then
        echo "messed up compiling $Test"
        exit 1
    fi
    ./$TEST_EXEC
}



##################  MAIN  ##################

# Run tests
if [ $? -eq 0 ]
then
    echo "*************************************************"
    echo "**            Run C++ API Examples             **"
    echo "**                                             **"
    echo "*************************************************"
    if (RunTest create &&\
        rm create &&\
        RunTest readdata &&\
        rm readdata &&\
        RunTest writedata &&\
        rm writedata &&\
        RunTest compound &&\
        rm compound &&\
        RunTest extend_ds &&\
        rm extend_ds &&\
        RunTest chunks &&\
        rm chunks &&\
        RunTest h5group &&\
        rm h5group); then
        EXIT_VALUE=${EXIT_SUCCESS}
    else
        EXIT_VALUE=${EXIT_FAILURE}
    fi
    echo
    echo "***************************************************"
    echo "**             Run Tutorial Examples             **"
    echo "**                                               **"
    echo "***************************************************"
    if (RunTest h5tutr_crtdat &&\
	rm h5tutr_crtdat &&\
	RunTest h5tutr_rdwt &&\
	rm h5tutr_rdwt &&\
	RunTest h5tutr_crtatt &&\
	rm h5tutr_crtatt &&\
	RunTest h5tutr_crtgrp &&\
	rm h5tutr_crtgrp &&\
	RunTest h5tutr_crtgrpar &&\
	rm h5tutr_crtgrpar &&\
	RunTest h5tutr_crtgrpd &&\
	rm h5tutr_crtgrpd &&\
	RunTest h5tutr_extend &&\
	rm h5tutr_extend &&\
	RunTest h5tutr_subset &&\
	rm h5tutr_subset &&\
	RunTest h5tutr_cmprss &&\
	rm h5tutr_cmprss ); then
	EXIT_VALUE=${EXIT_SUCCESS}
    else
	EXIT_VALUE=${EXIT_FAILURE}
    fi
fi

# Cleanup
rm *.o
rm *.h5
echo

exit $EXIT_VALUE 

