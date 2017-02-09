#!/bin/csh -f
#**************************************************************************************
# Purpose:  to regrid an OMI satellite data variable for given modeling domain grids 
#           from a downloaded OMI satellite data in HDF4 format (hdf) at the Geonanni 
#           web site :
#
#     There are four steps involved in computation:
#     1. Create a domain grid raster file with GRIDID from LL 1 to UR rows*cols    
#     2. Compute resolution to rasterize the created grid domain raster file
#     3. If satellite image is in latlong projection.  It will rasterize the grid domain 
#        in its projection.  If satellite image is in a x and y projection system, it
#        will project the grid domain raster file into satellite projection and rasterize 
#        it using computed raster resolution.
#     3. Compute grid OMI image data based on rasterized domain grids.  If rasterized domain 
#        grid file has different projection from satellite image, it will project each cell 
#        center to get satellite image value.  
#     4. Output grid OMI variable data into a WRF netcdf file and a text file with GRIDID.
#
#
# Written by the Institute for the Environment at UNC, Chapel Hill
# in support of the UAE NASA Projects, 2009.
#
# Written by:   L. Ran, Sept. 2009
#
# Program: computeGridOMI.exe
#         Needed environment variables included in the script file to run. 
#        
#***************************************************************************************

#
# Define domain grids
#
setenv GRID_PROJ "+proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=33 +lat_2=45 +lat_0=40 +lon_0=-97"

setenv GRID_ROWS 299
setenv GRID_COLUMNS 459

setenv GRID_XMIN -2556000.000
setenv GRID_YMIN -1728000.000

setenv GRID_XCELLSIZE 12000
setenv GRID_YCELLSIZE 12000


#Set satellite data date -- daily average data
#
#setenv DATA_DATE  20050910

setenv DATA_DATE  20060605

#set OMI file in hdf4 format to be processed and variable name
#
#setenv SATELLIE_DATA_FILE   "../data/sat/OMI_L3/OMAEROe.003.timeAverage.${DATA_DATE}.hdf"
#setenv SATELLITE_VARIABLE   AerosolOpticalThicknessMW


#setenv SATELLIE_DATA_FILE   "../data/sat/OMI_NO2_L2G/OMNO2G.003.dimensionAverage.${DATA_DATE}.hdf"
#setenv SATELLITE_VARIABLE  ColumnAmountNO2TropCS30

#set OMI file in hdf5 format to be processed
#
setenv SATELLIE_DATA_FILE   "../data/sat/OMI_NO2_L3/2006_06_05_NO2TropCS30.hdf5"
setenv SATELLITE_VARIABLE  NO2TropCS30 


# Output files
#
#setenv OUTPUT_TEXT_FILE      ../output/sat/conus_12km_OMIL3_AOT_${DATA_DATE}.txt
#setenv OUTPUT_NETCDF_FILE    ../output/sat/conus_12km_OMIL3_AOT_${DATA_DATE}.nc

#setenv OUTPUT_TEXT_FILE      ../output/sat/conus_12km_OMIL2G_NO2_${DATA_DATE}.txt
#setenv OUTPUT_NETCDF_FILE    ../output/sat/conus_12km_OMIL2G_NO2_${DATA_DATE}.nc

setenv OUTPUT_TEXT_FILE      ../output/sat/conus_12km_OMIL3_NO2_${DATA_DATE}.txt
setenv OUTPUT_NETCDF_FILE    ../output/sat/conus_12km_OMIL3_NO2_${DATA_DATE}.nc


#run the program
#
../bin/64bits/computeGridOMI.exe

#===================================================================
