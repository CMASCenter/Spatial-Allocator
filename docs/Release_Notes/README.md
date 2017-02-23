Spatial Allocator Tools Release Notes Readme 
====
D. Yang,  CEMPD, Institute for the Environment, UNC-Chapel Hill

Changes
-----

### 01/16/2017 Update notes for SA_v4.3_012017

#### Vector Tools: 
        * Update Earth Radius to 6370000
        * Modify Polar Stereographic Surrogate Header
        * Added New Tool Beld4SMK 
        * Added 64bits version 
        * Update reference data

### 05/31/2016 Update notes for SA 4.2:

#### * Raster Tools:
        * EPIC site info tool update for elevation output

#### Vector Tools:
        * Update projection radius to WRF 6370000
        * Bug fix for make file


### 02/05/2016 Update Update Release Notes for SA 4.2:

#### Raster Tools:
        * Updated elevation output format for EPIC site information 
        * Modified EPIC extraction tools for added EPIC output variables

#### Vector Tools:
        * Fixed an error in polygon surrogate computation


### 09/30/2015 Update Release Notes for SA 4.2:

#### Vector Tools:
        * Fixed an error in re-gridding the BELD3 data for CMAQ.
        * Fixed an error in surroagte computation related to holes in polyogns.

#### Raster Tools:
        *  Updated extraction tools for FEST-C output because of changed format and variable name definitions.

#### Surrogate Tools:
        * Fixed a bug related with polygon surrogate normaliztion when st+county code is 4 digit.
        * Fixed a bug in surrogate normalization in reading surrogate data lines with leading spaces and no "!" sign.
        * Added QA threshold for reporting.
        * Updated the surrogate Merging Tool to output QA information on the source surrogates.
        * Added leading "0" for one digit state codes in both merging and normalization tools.

Quick Start
------

Complete Documentation: https://www.cmascenter.org/help/documentation.cfm?model=sa-tools&version=4.3

#### Set up the SA environment:
        * Modify paths in ./bin/sa_setup.csh to installation directory
        * source sa_setup.csh or include sa_setup.csh in your .cshrc

#### Vector tools are stored in: 
        * bin/32bits and sample vector script files are in sa_052014/scripts.  
        * Users normally do not need to recompile the Vector Tools as they are statically compiled.
        * The Raster Tools often need to be re-compiled as they are not statically built.

#### Important Notes:
        * tmp\*.\* files created under ./raster_script can be deleted after the completion of the run

#### Location of data
        * Sample land use and satellite data can be stored in ./data/sat.  
        * See ./data/sat/README for details on where to obtain data for these tools.

#### SA Raster Tools for Satellite data and land use data processing:
        * Sample Raster Tools running script files in ./raster_scripts:
        * NLCD_MODIS_processor.csh - generate WRF grid NLCD and MODIS landuse data.  
        * allocateGOES2WRFGrids.csh -- GOES data processing tool.
        * allocateMODISL2CloudVars2Grids.csh -- MODIS L2 cloud and aerosol product processing tool.
        * allocateOMIL2vars2Grids.csh - OMI L2 aerosol and NO2 product processing tool.
        * allocateOMIvar2Grids.csh - OMI L2G and L3 aerosol and NO3 processing tool.
        * The tool is also may also be used process MODIS L3 products (not tested well) from NASA MODIS web site.
        * landuseTool_WRFCMAQ_BELD4.csh - generate BELD4 data from 2001 or 2006 NLCD/MODIS and crop tables.  
        * Processed crop and tree tables and shapefiles are stored under "data" directory.   

Troubleshooting
-----

#### Troubleshooting for library errors in the SA Raster Tools:
        * Recompile all libraries under src/libs following instructions in the src/libs/README file.
        * Modiy src/raster/Makefile for correct paths.
        * Type "make clean" to clean previous compiled programs.
        * Type "make" to compile the tools.
        * Type "make -B install" or "make install" to install compiled tools.

#### Troubleshooting for library errors in the SA Vector Tools:
        * Modiy src/vector/Makefile for correct library paths. Users can use src/vector/libs_32bits
        * Type "make clean" to clean previous compiled programs.
        * Type "make" to compile the tools.
        * Type "make install" to install compiled tools.
