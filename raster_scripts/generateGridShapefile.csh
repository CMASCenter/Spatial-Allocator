#!/bin/csh -f
#*******************************************************************************
# Purpose: to generate modeling domain regular grid polygon shapefile.
#          The created shapefile has GRIDID which goes from 1 (LL grid)
#          to GRID_ROWS*GRID_COLUMNS (UR grid) from left to right and up.
#
#
# Written by: L. Ran, August 2008
#                the Institute for the Environment at UNC, Chapel Hill
#                in support of the EPA NOAA CMAS Modeling, 2007-2008.
#
#
# Usgae: generateGridShapefile.csh
#*******************************************************************************

#
# Create a polygon shapefile for a given modeling domain grids
#
setenv GRID_PROJ "+proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=33 +lat_2=45 +lat_0=40 +lon_0=-97"

setenv GRID_ROWS 299
setenv GRID_COLUMNS 459

setenv GRID_XMIN  -2556000.0
setenv GRID_YMIN  -1728000.0

setenv GRID_XCELLSIZE 12000
setenv GRID_YCELLSIZE 12000

setenv GRID_SHAPEFILE_NAME ../output/conus_cmaq12km.shp

#
# run the toll
#
../bin/64bits/create_gridPolygon.exe
