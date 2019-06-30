#set SA installation directory and open source libraries used by Raster Tools
setenv SA_HOME /opt/srgtool
setenv PGBIN  /usr/bin
setenv PG_USER pgsurg
setenv DBNAME  surrogates
setenv DBSERVER localhost

#set PROJ4 libary and share data directories
setenv PROJDIR  ${SA_HOME}/src/libs/proj-4.9.3/local
setenv PROJ_LIB  ${PROJDIR}/share/proj
setenv PROJ_LIBRARY  ${PROJDIR}/lib
setenv PROJ_INCLUDE  ${PROJDIR}/include
setenv LD_LIBRARY_PATH ${PROJ_LIBRARY}

# set library for GDAL
setenv GDALHOME ${SA_HOME}/src/libs/gdal-2.0.2/local
setenv GDAL_LIB ${GDALHOME}/lib
setenv LD_LIBRARY_PATH ${GDAL_LIB}:${LD_LIBRARY_PATH}

#set envronment to run GDAL application programs
setenv GDALBIN ${GDALHOME}/bin
setenv GDAL_DATA ${GDALHOME}/share/gdal

# Set netCDF library and include directory
setenv NETCDF  ${SA_HOME}/src/libs/netcdf-4.0.1/local
setenv NETCDF_LIB  ${NETCDF}/lib
setenv NETCDF_INC  ${NETCDF}/include
setenv LD_LIBRARY_PATH ${NETCDF_LIB}:${LD_LIBRARY_PATH}

# Set HDF4 library and include directory
setenv HDF4  ${SA_HOME}/src/libs/hdf-4.2.11/local
setenv HDF4_LIB  ${HDF4}/lib
setenv HDF4_INC  ${HDF4}/include
setenv LD_LIBRARY_PATH ${HDF4_LIB}:${LD_LIBRARY_PATH}

# Set HDF5 library and include directory
setenv HDF5  ${SA_HOME}/src/libs/hdf5-1.8.16/local
setenv HDF5_LIB  ${HDF5}/lib
setenv HDF5_INC  ${HDF5}/include
setenv LD_LIBRARY_PATH ${HDF5_LIB}:${LD_LIBRARY_PATH}

# Set GEOS library and include directory
setenv GEOS  ${SA_HOME}/src/libs/geos-3.5.0/local
setenv GEOS_LIB  ${GEOS}/lib
setenv GEOS_INC  ${GEOS}/include
setenv LD_LIBRARY_PATH ${GEOS_LIB}:${LD_LIBRARY_PATH}

#set alias
alias  ncdump   "${NETCDF}/bin/ncdump"
alias  gdalinfo "${GDALHOME}/bin/gdalinfo"
alias  h4dump   "${HDF4}/bin/ncdump"
alias  h5dump   "${HDF5}/bin/h5dump"
alias  cs2cs    "${PROJDIR}/bin/cs2cs"
