[Home](README.md) 
***

# Spatial Allocator Quick Start 

## Set up the SA environment:
- SA_HOME is the base instllation directory
- Modify paths in SA_HOME/bin/sa_setup.csh to installation directory
- source sa_setup.csh or include sa_setup.csh in your .cshrc

## Vector tools are stored in:
- SA_HOME/bin/32bits and sample vector script files are in SA_HOME/scripts.
- Users normally do not need to recompile the Vector Tools as they are statically compiled.
- The Raster Tools may need to be re-compiled as they are not statically built.

## Location of data
- Download Input/Reference data for the SA installation from the [CMAS Center](http://www.cmascenter.org/sa-tools)
  - This archive contains the full set of North American BELD3 and BELD4 landuse data
  - North American data for creating a CMAQ Ocean file are contained in this data archive
- Sample land use and satellite data will be stored in SA_HOME/data/sat
  - See [Raster Tools Documentation](./SA_ch04_raster.md) for details on where to obtain data for these tools.
- Sample shapefiles are located in SA_HOME/data/shapefiles

## SA Raster Tools for Satellite data and land use data processing:
- Sample Raster Tools running script files in SA_HOME/raster_scripts:
- NLCD_MODIS_processor.csh - generate WRF grid NLCD and MODIS landuse data.
- allocateGOES2WRFGrids.csh -- GOES data processing tool.
- allocateMODISL2CloudVars2Grids.csh -- MODIS L2 cloud and aerosol product processing tool.
- allocateOMIL2vars2Grids.csh - OMI L2 aerosol and NO2 product processing tool.
- allocateOMIvar2Grids.csh - OMI L2G and L3 aerosol and NO3 processing tool.
- The tool is also may also be used process MODIS L3 products (not tested well) from NASA MODIS web site.
- landuseTool_WRFCMAQ_BELD4.csh - generate BELD4 data from 2001 or 2006 NLCD/MODIS and crop tables.
- Processed crop and tree tables and shapefiles are stored in the SA_HOME/data directory.


***
[Home](README.md)<br> 
Spatial Allocator User Manual (c) 2016<br>

