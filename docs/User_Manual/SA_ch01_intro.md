The Spatial Allocator (SA) is a set of tools that helps users manipulate and generate data files related to emissions and air quality modeling. The tools perform functions similar to Geographic Information Systems, but are provided to the modeling community free of charge. In addition, the tools are designed to support some of the unique aspects of the file formats used for Community Multiscale Air Quality (CMAQ), Sparse Matrix Operator Kernel Emissions (SMOKE), and Weather Research and Forecasting (WRF) modeling.

The MIMS Spatial Allocator is licensed as open-source software and provided at no cost because its development was sponsored by EPA. The Spatial Allocator uses GIS industry standard ESRI shapefiles, image files supported by GDAL, netCDF files and plain text data files as input and output data. The SA system includes three components developed for specific applications:


[Vector Tools:](SA_ch03_vector.md): The Vector Tools allow you to develop individual spatial surrogates, to change the map projection of Shapefiles, to remap spatial data from one spatial domain to another (e.g. counties to grids, fine grids to coarse grids), and to perform other types of spatial manipulation without using a GIS. Written in C language and developed for 32-bit Linux systems. Provides binaries are built on a 32-bit Linux operating system.

[Raster Tools:](SA_ch04_raster.md) The Raster Tools allow you to process land use data, satellite data, and agricultural fertilizer application data for meteorological and air quality modeling, particularly within the WRF/CMAQ modeling systems. Written in C++ language and developed for Linux systems.  Provides binaries are built on a 64-bit Linux operating system.

[Surrogate Tools](SA_ch05_surrogate.md): The Surrogate Tools allow you to manage the creation of large sets of spatial surrogates and also supports merging and gapfilling of surrogates. Surrogate Tools are written in Java they are provided in a java archive (.jar file) that can be used on Linux systems for which Java 1.5/later and Vector Tools are installed.

Open source packages used:

    Vector SA uses: I/O API and NetCD libraries compiled using pgcc, but PROJ4 compiled with gcc.
    Raster SA uses: GDAL, GEOS, PROJ4, NETCDF, HDF4, HDF5, and ANN in the C++ language compiled with gcc for Linux systems.

Instructions for downloading and installing.

Release Notes: SA 4.3 contains updated Raster Tools and Surrogate Tools.

The installation directory is set by the environment variable SA_HOME in the $SA_HOME/bin/sa_setup.csh file. This setup file also contains all open-source library paths. Users need to include the following line in their .cshrc files:

source {SA_HOME directory}/bin/sa_setup.csh

The SA system contains the following folders:

    bin – contains 64-bit executable raster tools, 32-bit executable vector tools, and set up file
    src – contains raster, vector, and library source codes
    data – contains required and sample data files. The sub-directory sat is used to contain land use and satellite input data
    raster_scripts – contains sample script files to run raster tools
    scripts – contains sample script files to run vector tools
    srgtools – contains the Surrogate Tools Java package and sample files to compute multiple surrogates
    output – output directory
    util – utility programs
