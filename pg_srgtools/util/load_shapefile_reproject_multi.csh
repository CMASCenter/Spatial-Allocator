#!/bin/csh -fx

# Load the original data shapefile (shp) with its projection (prj)
# Tranform to a new projection (from original 900922 to new 900921) and create gist index on it
# Check whether the shapefile data are imported correclty or not.

#setenv SA_HOME
#setenv GDALHOME
#setenv PGHOME
#setenv GDALBIN ${GDALHOME}/bin
#setenv PGBIN

set user=zeadelma
set server=localhost
set dbname=NEI2014
set schema=public
set srid=900921
set newfield=geom_${srid}          # geom_90021
set org_geom_field=wkb_geometry

set shpdir=/proj/ie/proj/EMAQ/Platform/Surrogates/2014/Spatial-Allocator/pg_srgcreate/shapefiles/
### Load county shapefile
set indir=$shpdir/Census
set shapefile=cb_2014_us_county_500k_Poly
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon          # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

### Load population and housing shapefile, and calculate density
set shapefile=ACS_2014_5YR_PopHousing
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon          # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

### Load hpms shapefile, transfer column move2014 to integer
set indir=$shpdir/HPMS
set shapefile=hpms2016
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiLineString       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

### Load pil shapefile for surrogate 205, Potential Idling Locations
set indir=$shpdir/PIL
set shapefile=pil_2018_08_17
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPoint       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh
# 275
set indir=$shpdir/2010
set shapefile=ERTAC_railyard_WRF
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPoint       # retrieve the exact geopmetry type from the table.
source load_shapefile.csh

# for 350,807,820 data shapefile: NTAD_2014_County_Pol ?
#set indir=/proj/ie/proj/EMAQ/Platform/Surrogates/2014/Spatial-Allocator/data/emiss_shp2014/NTAD
set indir=$shpdir/NTAD
set shapefile=NTAD_2014_County_Pol
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon          # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

###  Load Bus Terminals shapefile for surrogates 258 and 259
set shapefile=NTAD_2014_ipcd
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPoint       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

###  Load Railroad shapefile for surrogates 261, 271-273
set shapefile=NTAD_2014_Rail
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiLineString       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

###  Load Waterway shapefile for surrogates 807
set shapefile=NTAD_2014_Waterway
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiLineString       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

### Load NLCD shapefile ? or nlcd2011_500mv2*
set indir=$shpdir/NLCD
set shapefile=CONUS_AK_NLCD_2011_500m_WGS
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon        # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

###  Load Waterway shapefile for surrogates 807
set indir=$shpdir/TIGER
set shapefile=TIGER_2014_Rail #under emiss_shp2014
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiLineString       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

# Losd  Refineries and Tank Farms
#set indir=/proj/ie/proj/EMAQ/Platform/Surrogates/2014/Spatial-Allocator/data/emiss_shp2014/EIA
set indir=$shpdir/EIA
set shapefile=EIA_2015_US_Oil
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPoint       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set indir=$shpdir/USGS
set shapefile=USGS_2011_mines
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPoint 
#source load_shapefile.csh

set indir=$shpdir/POI
set shapefile=POI_Factory_2015_GolfCourses
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPoint 
#source load_shapefile.csh

set indir=$shpdir/US 
set shapefile=airport_area
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon
#source load_shapefile.csh

# 808 2013 Shipping Density
set indir=$shpdir/CMV    
set shapefile=CMV_2013_Vessel_Density_CONUS1km
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon
#source load_shapefile.csh

# 890 Commercial Timber 
set indir=$shpdir/USFS   
set shapefile=USFS_2016_TimberHarvest
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon
#source load_shapefile.csh

### Load Oil Gas shapefile
set indir=$shpdir/OilGas
set shapefile=AllExploratoryWells
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon    # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=AllProductionWells
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon    # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=AllWells
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon    # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=AssocGasProduction
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon    # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=CBMProduction
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon    # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=CBMWell_Counts
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon    # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=Completions_All
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon         # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=Completions_CBM
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon         # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=Completions_Oil
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon         # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=CondensateCBMProd
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon         # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=CondensateGasProd
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon         # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=FeetDrilled_All
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon         # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=GasProduction
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon         # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=GasWell_Counts
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=OilProduction
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=OilWell_Counts_20161111
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=ProducedWater_All
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=ProducedWater_CBM
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=ProducedWater_Gas
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=ProducedWater_Oil
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh
  
set shapefile=SpudCount_CBM
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=SpudCount_Gas
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=SpudCount_HF
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=SpudCount_Oil
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set shapefile=Completions_Gas
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon          # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

### Load Shipping shapefile for 805, 806
set indir=$shpdir/EPA
set shapefile=ShippingLanes_2014NEI
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon
#source load_shapefile.csh

# Ports, has ring problem? can be ignored?
set shapefile=Ports_2014NEI
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon          # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh
