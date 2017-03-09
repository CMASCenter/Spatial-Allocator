[Home](README.md) - [Next Chapter >>](SA_ch02_install.md)
***

# Chapter 1. Introduction

The Spatial Allocator (SA) is a set of tools that helps users manipulate and generate data files related to emissions and air quality modeling. The tools perform functions similar to Geographic Information Systems, but are provided to the modeling community free of charge. In addition, the tools are designed to support some of the unique aspects of the file formats used for [Community Multiscale Air Quality (CMAQ)](http://www.epa.gov/cmaq), [Sparse Matrix Operator Kernel Emissions (SMOKE)](http://www.smoke-model.org), and [Weather Research and Forecasting (WRF)](http://www.wrf-model.org) modeling.

The SA is licensed as open-source software and its development was sponsored by EPA. The Spatial Allocator uses GIS industry standard ESRI shapefiles, image files supported by GDAL, netCDF files and plain text data files as input and output data. The SA system includes three components developed for specific applications:

[Vector Tools:](SA_ch03_vector.md): The Vector Tools is a suite of programs to develop individual spatial surrogates, to change the map projection of Shapefiles, to remap spatial data from one spatial domain to another (e.g. counties to grids, fine grids to coarse grids), and to perform other types of spatial manipulation without using a GIS. Written in the C language the Vector Tools are distributed as both 32-bit and 64-bit binary pre-compiled executables for the Linux operating systems.

[Raster Tools:](SA_ch04_raster.md) The Raster Tools is suite of programs to process land use data, satellite data, and agricultural fertilizer application data for meteorological and air quality modeling, particularly within the WRF/CMAQ modeling systems. Written in C++ language and developed for Linux systems, they are distributed as pre-compiled 64-bit binaries for Linux operating systems.

[Surrogate Tools](SA_ch05_surrogate.md): The Surrogate Tools manage the creation of spatial surrogates for mapping emissions sources to modeling domains. The also include functions for merging and gapfilling of surrogates. The Surrogate Tools are written in Java and are provided in a java archive (.jar file) that can be used on Linux systems on which Java 1.5/later and the Vector Tools are installed.

Open source 3rd-party packages used:

- Vector Tools: I/O API and NetCD libraries compiled using pgcc, but PROJ4 compiled with gcc.
- Raster Tools: GDAL, GEOS, PROJ4, NETCDF, HDF4, HDF5, and ANN in the C++ language compiled with gcc for Linux systems.

The SA code, scripts, and executables are organized in the following directories:

- **bin** - SA 32-bit and 64-bit Linux executables
- **data** - Input data for testing the SA installation (download from the [CMAS Center](http://www.cmascenter.org))
- **docs** - SA Documentation
- **raster_scripts** - C-shell scripts for running the [Raster Tools](docs/User_Manual/SA_ch04_raster.md)
- **ref** - location of reference data for verifying the SA installation (download from the [CMAS Center](http://www.cmascenter.org))
- **scripts** - C-shell scripts for running the [Vector Tools](docs/User_Manual/SA_ch03_vector.md)
- **src** - SA source code and 3rd-party libraries
- **srgtools** - C-shell scripts for running the [Surrogate Tools](docs/User_Manual/SA_ch05_surrogate.md)
- **util** - miscellaneous utility scripts to support the SA tools

***
[Home](README.md) - [Next Chapter >>](SA_ch02_install.md)<br>
Spatial Allocator User Manual (c) 2016<br>
