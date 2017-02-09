#!/bin/csh -f
#*******************************************************************************
# Purpose: to get rid of image overlapping areas among downloaded USGS NLCD 
#           images (landuse, imperviousness, canopy) and NOAA coastal NLCD images.
#           All image names and locations are stored in a file defined by
#           INPUT_NLCDFILES_LIST.  Users only need to run this program once if 
#           they obtain original image data.   
#
# Output: All preprocessed images will be stored in directory defined by
#          DATADIR.  New image file names are listed in file defined by
#          OUTPUT_NLCDFILES_LIST.
#
# Written by:  L. Ran, August 2008
#              the Institute for the Environment at UNC, Chapel Hill
#              in support of the EPA NOAA CMAS Modeling, 2007-2008.
#
# Usage: preProcessLanduseImages.csh
#
#*******************************************************************************

#
#  Preprocessing image files to get rid of overlapping pixels
#  Important: INPUT_NLCDFILES_LIST file must have file type lines provided in the release sample file.
#
setenv INPUT_NLCDFILES_LIST ../data/nlcd2006_files.txt
setenv DATADIR "../data/sat/pp_NLCD2006"
setenv OUTPUT_NLCDFILES_LIST ../data/nlcd2006_files_pp.txt
../bin/64bits/preProcessNLCD.exe
