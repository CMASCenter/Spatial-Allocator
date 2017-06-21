# for Linux or any gcc/g++ compiler
CC = gcc
CPP = g++

#Optional flags - uncomment one OPTFLAGS line
OPTFLAGS = -O2
#OPTFLAGS = -O -DDEBUG 

#set SA HOME
SA_HOME = /proj/ie/proj/apps/sallocator/sa_v4.3.1_062017/src

#set GDAL library and include directory compiled using gcc
GDAL = ${SA_HOME}/libs/gdal-1.11.0/local
GDAL_LIB = ${GDAL}/lib
GDAL_INCLUDE = ${GDAL}/include

#set PROJ4 libary and share data directories compiled using gcc
PROJ4 = ${SA_HOME}/libs/proj-4.8.0/local
PROJ_LIB = ${PROJ4}/lib
PROJ_INCLUDE = ${PROJ4}/include

#set GEOS libary and share data directories compiled using gcc
GEOS = ${SA_HOME}/libs/geos-3.4.2/local
GEOS_LIB = ${GEOS}/lib
GEOS_INCLUDE = ${GEOS}/include

# Set netCDF library and include directory compiled using gcc
NETCDF = ${SA_HOME}/libs/netcdf-4.0.1/local
NETCDF_LIB = ${NETCDF}/lib 
NETCDF_INCLUDE = ${NETCDF}/include

# Set HDF4 library and include directory compiled using gcc
HDF4 = ${SA_HOME}/libs/hdf-4.2.10/local
HDF4_LIB = ${HDF4}/lib
HDF4_INCLUDE = ${HDF4}/include

# Set HDF5 library and include directory compiled using gcc
HDF5 = ${SA_HOME}/libs/hdf5-1.8.13/local
HDF5_LIB = ${HDF5}/lib
HDF5_INCLUDE = ${HDF5}/include

#set ANN libary compiled using gcc
ANN111 = ${SA_HOME}/libs/ann_1.1.1
ANN_LIB = ${ANN111}/lib
ANN_INCLUDE = ${ANN111}/include


# Library paths
LIBPATH = -L$(GDAL_LIB) 
LIBPATH_PROJ = -L$(PROJ_LIB)
LIBPATH_GEOS = -L$(GEOS_LIB)
LIBPATH_NCF = -L$(NETCDF_LIB)
LIBPATH_HDF4 = -L$(HDF4_LIB)
LIBPATH_HDF5 = -L$(HDF5_LIB)
LIBPATH_ANN = -L$(ANN_LIB)


# Bstatic compiling - static compiling does not work for GEOS,GeoTiff
LIBS =  -Bstatic -pthread $(LIBPATH) -lgdal $(LIBPATH_PROJ) -lproj $(LIBPATH_GEOS) -lgeos_c -lgeos $(LIBPATH_NCF) -lnetcdf_c++ -lnetcdf $(LIBPATH_HDF4) -lmfhdf -ldf $(LIBPATH_HDF5) -lhdf5 -lhdf5_hl -lhdf5_cpp -lhdf5_hl_cpp $(LIBPATH_ANN) -lANN  -ldl


#on Linux
CPPFLAGS = -g $(OPTFLAGS) -I${GDAL_INCLUDE} -I${PROJ_INCLUDE} -I${GEOS_INCLUDE} -I${NETCDF_INCLUDE} -I${HDF4_INCLUDE} -I${HDF5_INCLUDE} -I${ANN_INCLUDE}


#.cpp.o:
##	$(CPP) $(CFLAGS) -c $<
#
#.c.o:
#	$(CC) $(CFLAGS) -c $<


all: create_gridPolygon.exe toNLCDRaster.exe preProcessNLCD.exe computeGridLandUse.exe computeGridLandUse_beld4.exe rasterWtoPolygons.exe txt2ncf.exe toDataAssimilationFMT.exe computeGridGOES.exe computeGridOMI.exe computeGridOMIL2.exe computeGridMODISL2Clouds.exe computeSiteDailyWeather.exe compute_EPICSiteData.exe extractEPIC2CMAQ.exe extractEPICYearlyAverage2CMAQ.exe

create_gridPolygon.exe:  create_gridPolygon.o geo_functions.o utilities.o
	$(CPP) -o $@  create_gridPolygon.o geo_functions.o utilities.o $(LIBS)

toNLCDRaster.exe:  toNLCDRaster.o utilities.o
	$(CPP) -o $@ toNLCDRaster.o  utilities.o $(LIBS)

preProcessNLCD.exe:  preProcessNLCD.o utilities.o
	$(CPP) -o $@ preProcessNLCD.o utilities.o $(LIBS)

computeGridLandUse.exe:  computeGridLandUse.o geo_functions.o  utilities.o
	$(CPP) -o $@ computeGridLandUse.o geo_functions.o  utilities.o $(LIBS)

computeGridLandUse_beld4.exe:  computeGridLandUse_beld4.o geo_functions.o  utilities.o
	$(CPP) -o $@ computeGridLandUse_beld4.o geo_functions.o  utilities.o $(LIBS)

rasterWtoPolygons.exe:  rasterWtoPolygons.o utilities.o
	$(CPP) -o $@ rasterWtoPolygons.o utilities.o $(LIBS)

txt2ncf.exe:  txt2ncf.o geo_functions.o utilities.o
	$(CPP) -o $@ txt2ncf.o geo_functions.o utilities.o $(LIBS)

toDataAssimilationFMT.exe:  toDataAssimilationFMT.o geo_functions.o  utilities.o
	$(CPP) -o $@ toDataAssimilationFMT.o geo_functions.o utilities.o $(LIBS)

computeGridGOES.exe:  computeGridGOES.o geo_functions.o utilities.o
	$(CPP) -o $@ computeGridGOES.o geo_functions.o utilities.o $(LIBS)

computeGridOMI.exe:  computeGridOMI.o geo_functions.o utilities.o
	$(CPP) -o $@ computeGridOMI.o geo_functions.o utilities.o $(LIBS)

computeGridOMIL2.exe:  computeGridOMIL2.o geo_functions.o utilities.o
	$(CPP) -o $@ computeGridOMIL2.o geo_functions.o utilities.o $(LIBS)

computeGridMODISL2Clouds.exe:  computeGridMODISL2Clouds.o geo_functions.o utilities.o
	$(CPP) -o $@ computeGridMODISL2Clouds.o geo_functions.o utilities.o $(LIBS)

computeSiteDailyWeather.exe:  computeSiteDailyWeather.o geo_functions.o utilities.o m3io.o
	$(CPP) -o $@ computeSiteDailyWeather.o geo_functions.o utilities.o m3io.o $(LIBS) 

compute_EPICSiteData.exe:  compute_EPICSiteData.o geo_functions.o utilities.o 
	$(CPP) -o $@ compute_EPICSiteData.o geo_functions.o utilities.o $(LIBS) 

extractEPIC2CMAQ.exe:  extractEPIC2CMAQ.o geo_functions.o utilities.o m3io.o
	$(CPP) -o $@ extractEPIC2CMAQ.o geo_functions.o utilities.o m3io.o $(LIBS)

extractEPICYearlyAverage2CMAQ.exe:  extractEPICYearlyAverage2CMAQ.o geo_functions.o utilities.o m3io.o
	$(CPP) -o $@ extractEPICYearlyAverage2CMAQ.o geo_functions.o utilities.o m3io.o $(LIBS)


install:
	if test ! -d ../../bin/64bits; then mkdir ../../bin/64bits; fi
	cp -f *.exe ../../bin/64bits

clean:
	 -rm *.o  *.exe

create_gridPolygon.o: create_gridPolygon.cpp
	$(CPP) $(CPPFLAGS) -c create_gridPolygon.cpp
toNLCDRaster.o: toNLCDRaster.cpp
	$(CPP) $(CPPFLAGS) -c toNLCDRaster.cpp
preProcessNLCD.o: preProcessNLCD.cpp
	$(CPP) $(CPPFLAGS) -c preProcessNLCD.cpp
computeGridLandUse.o: computeGridLandUse.cpp
	$(CPP) $(CPPFLAGS) -c computeGridLandUse.cpp
computeGridLandUse_beld4.o: computeGridLandUse_beld4.cpp
	$(CPP) $(CPPFLAGS) -c computeGridLandUse_beld4.cpp
rasterWtoPolygons.o: rasterWtoPolygons.cpp
	$(CPP) $(CPPFLAGS) -c rasterWtoPolygons.cpp
txt2ncf.o: txt2ncf.cpp
	$(CPP) $(CPPFLAGS) -c txt2ncf.cpp
toDataAssimilationFMT.o: toDataAssimilationFMT.cpp
	$(CPP) $(CPPFLAGS) -c toDataAssimilationFMT.cpp
computeGridGOES.o: computeGridGOES.cpp
	$(CPP) $(CPPFLAGS) -c computeGridGOES.cpp
computeGridOMI.o: computeGridOMI.cpp
	$(CPP) $(CPPFLAGS) -c computeGridOMI.cpp
computeGridOMIL2.o: computeGridOMIL2.cpp
	$(CPP) $(CPPFLAGS) -c computeGridOMIL2.cpp
computeGridMODISL2Clouds.o: computeGridMODISL2Clouds.cpp
	$(CPP) $(CPPFLAGS) -c computeGridMODISL2Clouds.cpp
computeSiteDailyWeather.o: computeSiteDailyWeather.cpp
	$(CPP) $(CPPFLAGS) -c computeSiteDailyWeather.cpp
compute_EPICSiteData.o: compute_EPICSiteData.cpp
	$(CPP) $(CPPFLAGS) -c compute_EPICSiteData.cpp
extractEPIC2CMAQ.o: extractEPIC2CMAQ.cpp
	$(CPP) $(CPPFLAGS) -c extractEPIC2CMAQ.cpp
extractEPICYearlyAverage2CMAQ.o: extractEPICYearlyAverage2CMAQ.cpp
	$(CPP) $(CPPFLAGS) -c extractEPICYearlyAverage2CMAQ.cpp
utilities.o: utilities.cpp
	$(CPP) -c utilities.cpp
geo_functions.o: geo_functions.cpp
	$(CPP) $(CPPFLAGS) -c geo_functions.cpp
m3io.o: m3io.cpp
	$(CPP) $(CPPFLAGS) -c m3io.cpp
