#set  SA installtion directory and open source libraries used by Raster Tools
setenv SA_HOME [Add Installation Path Here]/sa_v4.3_012017 

#set PROJ4 libary and share data directories
setenv PROJDIR  ${SA_HOME}/src/libs/proj-4.8.0/local
setenv PROJ_LIB  ${PROJDIR}/share/proj
setenv PROJ_LIBRARY  ${PROJDIR}/lib
setenv PROJ_INCLUDE  ${PROJDIR}/include
setenv LD_LIBRARY_PATH ${PROJ_LIBRARY}:${LD_LIBRARY_PATH}

# set library for GDAL
setenv GDALHOME ${SA_HOME}/src/libs/gdal-1.11.0/local
setenv GDAL_LIB ${GDALHOME}/lib
setenv LD_LIBRARY_PATH ${GDAL_LIB}:${LD_LIBRARY_PATH}

#set envronment to run GDAL application programs
setenv GDALBIN ${GDALHOME}/bin

# Set netCDF library and include directory
setenv NETCDF  ${SA_HOME}/src/libs/netcdf-4.0.1/local
setenv NETCDF_LIB  ${NETCDF}/lib
setenv NETCDF_INC  ${NETCDF}/include
setenv LD_LIBRARY_PATH ${NETCDF_LIB}:${LD_LIBRARY_PATH}

# Set HDF4 library and include directory
setenv HDF4  ${SA_HOME}/src/libs/hdf-4.2.10/local
setenv HDF4_LIB  ${HDF4}/lib
setenv HDF4_INC  ${HDF4}/include
setenv LD_LIBRARY_PATH ${HDF4_LIB}:${LD_LIBRARY_PATH}

# Set HDF5 library and include directory
setenv HDF5  ${SA_HOME}/src/libs/hdf5-1.8.13/local
setenv HDF5_LIB  ${HDF5}/lib
setenv HDF5_INC  ${HDF5}/include
setenv LD_LIBRARY_PATH ${HDF5_LIB}:${LD_LIBRARY_PATH}

# Set GEOS library and include directory
setenv GEOS  ${SA_HOME}/src/libs/geos-3.4.2/local
setenv GEOS_LIB  ${GEOS}/lib
setenv GEOS_INC  ${GEOS}/include
setenv LD_LIBRARY_PATH ${GEOS_LIB}:${LD_LIBRARY_PATH}

#set alias
alias  ncdump   "${NETCDF}/bin/ncdump"
alias  gdalinfo "${GDALHOME}/bin/gdalinfo"
alias  h4dump   "${HDF4}/bin/ncdump"
alias  h5dump   "${HDF5}/bin/h5dump"
alias  cs2cs    "${PROJDIR}/bin/cs2cs"
