#! /bin/sh
#
# Copyright by The HDF Group.
# All rights reserved.
#
# This file is part of HDF4.  The full HDF4 copyright notice, including
# terms governing use, modification, and redistribution, is contained in
# the files COPYING and Copyright.html.  COPYING can be found at the root
# of the source code distribution tree; Copyright.html can be found at the
# root level of an installed copy of the electronic HDF4 document set and
# is linked from the top-level documents page.  It can also be found at
# http://hdfgroup.org/HDF4/doc/Copyright.html.  If you do not have
# access to either file, you may request a copy from help@hdfgroup.org.

#
#  This file:  run-hdf-c-ex.sh
# Written by:  Larry Knox
#       Date:  Jan 17, 2014
#
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                                               #
# This script will compile and run the c examples from source files installed   #
# in .../examples/c using h4cc.  The order for running       #
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
prefix="${prefix:-../../}"
AR="@AR@"
RANLIB="@RANLIB@"
H4TOOL="h4cc"               # The tool name
H4TOOL_BIN="${prefix}/bin/${H4TOOL}"   # The path of the tool binary

#### Run test ####
RunTest()
{
    TEST_EXEC=$1
    Test=$1".c"

    echo
    echo "#################  $1  #################"
    ${H4TOOL_BIN} -o $TEST_EXEC $Test
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
    if (
        #### hdf examples ####    
        RunTest VD_create_vdatas &&\
        rm VD_create_vdatas &&\
        RunTest VD_write_mixed_vdata &&\
        rm VD_write_mixed_vdata &&\
        RunTest VD_write_mixed_vdata_struct &&\
        rm VD_write_mixed_vdata_struct &&\
        RunTest VD_write_to_vdata &&\
        rm VD_write_to_vdata &&\
        RunTest VD_read_from_vdata &&\
        rm VD_read_from_vdata &&\
        RunTest VD_read_mixed_vdata &&\
        rm VD_read_mixed_vdata &&\
        RunTest VD_set_get_vdata_attr &&\
        rm VD_set_get_vdata_attr &&\
        RunTest VD_create_onefield_vdatas &&\
        rm VD_create_onefield_vdatas &&\
        RunTest VD_get_vdata_info &&\
        rm VD_get_vdata_info &&\
        RunTest VD_locate_vdata &&\
        rm VD_locate_vdata &&\
        RunTest VG_create_vgroup &&\
        rm VG_create_vgroup &&\
        RunTest VG_add_sds_to_vgroup &&\
        rm VG_add_sds_to_vgroup &&\
        RunTest VG_insert_vdatas_to_vgroup &&\
        rm VG_insert_vdatas_to_vgroup &&\
        RunTest VG_set_get_vgroup_attr &&\
        rm VG_set_get_vgroup_attr &&\
        RunTest VG_vgroup_contents &&\
        rm VG_vgroup_contents &&\
        RunTest VG_get_vgroup_info &&\
        rm VG_get_vgroup_info &&\
        RunTest GR_create_and_write_image &&\
        rm GR_create_and_write_image &&\
        RunTest GR_modify_image &&\
        rm GR_modify_image &&\
        RunTest GR_set_attribute &&\
        rm GR_set_attribute &&\
        RunTest GR_get_attribute &&\
        rm GR_get_attribute &&\
        RunTest GR_write_palette &&\
        rm GR_write_palette &&\
        RunTest GR_read_palette &&\
        rm GR_read_palette &&\
        RunTest GR_image_info &&\
        rm GR_image_info &&\
        RunTest GR_read_image &&\
        rm GR_read_image &&\
        RunTest GR_write_chunks &&\
        rm GR_write_chunks &&\
        RunTest GR_read_chunks &&\
        rm GR_read_chunks &&\
        RunTest AN_create_annotation &&\
        rm AN_create_annotation &&\
        RunTest AN_get_annotation_info &&\
        rm AN_get_annotation_info &&\
        RunTest AN_read_annotation &&\
        rm AN_read_annotation &&\
        #### mfhdf examples ####
        RunTest SD_create_sds &&\
        rm SD_create_sds &&\
        RunTest SD_write_to_sds &&\
        rm SD_write_to_sds &&\
        RunTest SD_write_slab &&\
        rm SD_write_slab &&\
        RunTest SD_alter_sds_values &&\
        rm SD_alter_sds_values &&\
        RunTest SD_unlimited_sds &&\
        rm SD_unlimited_sds &&\
        RunTest SD_compress_sds &&\
        rm SD_compress_sds &&\
        RunTest SD_mv_sds_to_external &&\
        rm SD_mv_sds_to_external &&\
        RunTest SD_read_from_sds &&\
        rm SD_read_from_sds &&\
        RunTest SD_read_subsets &&\
        rm SD_read_subsets &&\
        RunTest SD_get_info &&\
        rm SD_get_info &&\
        RunTest SD_find_sds_by_name &&\
        rm SD_find_sds_by_name &&\
        RunTest SD_set_get_dim_info &&\
        rm SD_set_get_dim_info &&\
        RunTest SD_dimscale_vs_sds &&\
        rm SD_dimscale_vs_sds &&\
        RunTest SD_set_attr &&\
        rm SD_set_attr &&\
        RunTest SD_get_attr &&\
        rm SD_get_attr &&\
        RunTest SD_chunking_example &&\
        rm SD_chunking_example
        ); then
        EXIT_VALUE=${EXIT_SUCCESS}
    else
        EXIT_VALUE=${EXIT_FAILURE}
    fi
fi

# Cleanup
rm *.o
rm *.hdf
echo

exit $EXIT_VALUE 

