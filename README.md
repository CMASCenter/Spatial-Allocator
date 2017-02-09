Spatial Allocator Tools README, 1/20/2017
D. Yang,  CEMPD, Institute for the Environment, UNC-Chapel Hill

==============
Changes
==============

01/16/2017
Update notes for SA_v4.3_012017
        Vector Tools: 
        (1) Update Earth Radius to 6370000
        (2) Modify Polar Stereographic Surrogate Header
        (3) Added New Tool Beld4SMK 
        (4) Added 64bits version 
        (5) Update reference data

05/31/2016
Update notes for SA 4.2:
	Raster Tools:
	(1) EPIC site info tool update for elevation output

	Vector Tools:
	(1) Update projection radius to WRF 6370000
	(2) Bug fix for make file


02/05/2016
Update Update Release Notes for SA 4.2:

	Raster Tools:
	(1) Updated elevation output format for EPIC site information 
        (2) Modified EPIC extraction tools for added EPIC output variables

	Vector Tools:
        (1) Fixed an error in polygon surrogate computation


09/30/2015
Update Release Notes for SA 4.2:
	This update release has the following updates:

        Vector Tools:
        (1) Fixed an error in re-gridding the BELD3 data for CMAQ.
        (2) Fixed an error in surroagte computation related to holes in polyogns.

	Raster Tools:
	(1) Updated extraction tools for FEST-C output because of changed format and 
            variable name definitions.

        Surrogate Tools:
        (1) Fixed a bug related with polygon surrogate normaliztion when st+county code is 4 digit.
 	(2) Fixed a bug in surrogate normalization in reading surrogate data lines with leading spaces and no "!" sign.
        (3) Added QA threshold for reporting.
        (4) Updated the surrogate Merging Tool to output QA information on the source surrogates.
        (5) Added leading "0" for one digit state codes in both merging and normalization tools.

==============
Quick Start
==============

Complete Documentation: https://www.cmascenter.org/help/documentation.cfm?model=sa-tools&version=4.3

1. Set up the SA environment:
	(1) Modify paths in ./bin/sa_setup.csh to installation directory

	(2) source sa_setup.csh or include sa_setup.csh in your .cshrc

2. Vector tools are stored in: bin/32bits and sample vector script files are in sa_052014/scripts.  
   Users normally do not need to recompile the Vector Tools as they are statically compiled.
   The Raster Tools often need to be re-compiled as they are not statically built.

3. Important Notes:
   tmp*.* files created under ./raster_script can be deleted after the completion of the run

4. Sample land use and satellite data can be stored in ./data/sat.  See ./data/sat/README for details on where to obtain data for these tools.
   Sample Raster Tools running script files in ./raster_scripts:

5. SA Raster Tools for Satellite data and land use data processing:

(1) NLCD_MODIS_processor.csh - generate WRF grid NLCD and MODIS landuse data.  

(2) allocateGOES2WRFGrids.csh -- GOES data processing tool.

(3) allocateMODISL2CloudVars2Grids.csh -- MODIS L2 cloud and aerosol product processing tool.

(4) allocateOMIL2vars2Grids.csh - OMI L2 aerosol and NO2 product processing tool.

(5) allocateOMIvar2Grids.csh - OMI L2G and L3 aerosol and NO3 processing tool.
        The tool is also may also be used process MODIS L3 products (not tested well) from NASA MODIS web site.

(6) landuseTool_WRFCMAQ_BELD4.csh - generate BELD4 data from 2001 or 2006 NLCD/MODIS and crop tables.  
	Processed crop and tree tables and shapefiles are stored under "data" directory.   

==============
Troubleshooting
==============

1. Troubleshooting for library errors in the SA Raster Tools:
   >Recompile all libraries under src/libs following instructions in the src/libs/README file.
   >Modiy src/raster/Makefile for correct paths.
   >Type "make clean" to clean previous compiled programs.
   >Type "make" to compile the tools.
   >Type "make -B install" or "make install" to install compiled tools.

2. Troubleshooting for library errors in the SA Vector Tools:
   >Modiy src/vector/Makefile for correct library paths. Users can use src/vector/libs_32bits
   >Type "make clean" to clean previous compiled programs.
   >Type "make" to compile the tools.
   >Type "make install" to install compiled tools.
