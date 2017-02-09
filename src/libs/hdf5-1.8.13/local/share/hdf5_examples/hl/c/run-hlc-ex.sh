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
#  This file:  run-hlc-ex.sh
# Written by:  Larry Knox
#       Date:  May 11, 2010
#
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                                               #
# This script will compile and run the c examples from source files installed   #
# in .../share/hdf5_examples/hl/c using h5cc or h5pc.  The order for running    #
# programs with RunTest in the MAIN section below is taken from the Makefile.   #
# The order is important since some of the test programs use data files created #
# by earlier test programs.  Any future additions should be placed accordingly. #
#                                                                               #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

# Initializations
EXIT_SUCCESS=0
EXIT_FAILURE=1

# Where the tool is installed.
# default is relative path to installed location of the tools
prefix="${prefix:-../../../../}"
PARALLEL=no             # Am I in parallel mode?
AR="ar"
RANLIB="ranlib"
if [ "$PARALLEL" = no ]; then
    H5TOOL="h5cc"               # The tool name
else
    H5TOOL="h5pcc"               # The tool name
fi
H5TOOL_BIN="${prefix}/bin/${H5TOOL}"   # The path of the tool binary

#### Run test ####
RunTest()
{
    TEST_EXEC=$1
    Test=$1".c"

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
    if (RunTest ex_lite1 &&\
        rm ex_lite1 &&\
        RunTest ex_lite2 &&\
        rm ex_lite2 &&\
        RunTest ex_lite3 &&\
        rm ex_lite3 &&\
        RunTest ptExampleFL &&\
        rm ptExampleFL &&\
        RunTest ex_image1 &&\
        rm ex_image1 &&\
        RunTest ex_image2 &&\
        rm ex_image2 &&\
        RunTest ex_table_01 &&\
        rm ex_table_01 &&\
        RunTest ex_table_02 &&\
        rm ex_table_02 &&\
        RunTest ex_table_03 &&\
        rm ex_table_03 &&\
        RunTest ex_table_04 &&\
        rm ex_table_04 &&\
        RunTest ex_table_05 &&
        rm ex_table_05 &&
        RunTest ex_table_06 &&\
        rm ex_table_06 &&\
        RunTest ex_table_07 &&\
        rm ex_table_07 &&\
        RunTest ex_table_08 &&\
        rm ex_table_08 &&\
        RunTest ex_table_09 &&\
        rm ex_table_09 &&\
        RunTest ex_table_10 &&\
        rm ex_table_10 &&\
        RunTest ex_table_11 &&\
        rm ex_table_11 &&\
        RunTest ex_table_12 &&\
        rm ex_table_12 &&\
        RunTest ex_ds1 &&\
        rm ex_ds1); then
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

