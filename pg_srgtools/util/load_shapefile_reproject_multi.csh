#!/bin/csh -fx

# Load the original data shapefile (shp) with its projection (prj)
# Tranform to a new projection (from original 900922 to new 900921) and create gist index on it
# Check whether the shapefile data are imported correclty or not.

#setenv SA_HOME
#setenv GDALHOME
#setenv PGHOME
#setenv GDALBIN ${GDALHOME}/bin
#setenv PGBIN
#setenv PGDATA
setenv PGDATA /Users/lizadams/Spatial-Allocator/data/pg_shapefiles

set user=lizadams
set server=localhost
set dbname=surrogates
set schema=public
set srid=900921
set newfield=geom_${srid}          # geom_90021
set org_geom_field=wkb_geometry

### Load county shapefile
set indir=$PGDATA/emiss_shp2014/Census/
set shapefile=cb_2014_us_county_500k_Poly
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon          # retrieve the exact geopmetry type from the table.
source load_shapefile.csh

### Load population and housing shapefile, and calculate density
set indir=$PGDATA/emiss_shp2014/Census/
set shapefile=ACS_2014_5YR_PopHousing
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon          # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

### Load hpms shapefile, transfer column move2014 to integer
set indir=$PGDATA/emiss_shp2016/HPMS
set shapefile=hpms2016
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiLineString       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

### Load pil shapefile for surrogate 205, Potential Idling Locations
set indir=$PGDATA/emiss_shp2018/PIL
set shapefile=pil_2018_08_17
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPoint       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

# for 350,807,820 data shapefile: NTAD_2014_County_Pol ?
set indir=$PGDATA/emiss_shp2014/NTAD
#set indir=$PGDATA/emiss_shp2014/NTAD_tem
set shapefile=NTAD_2014_County_Pol
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon          # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

###  Load Bus Terminals shapefile for surrogates 258 and 259
set shapefile=NTAD_2014_ipcd
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=Point       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

###  Load Railroad shapefile for surrogates 260, 261, 271-273
set shapefile=NTAD_2014_Rail
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiLineString       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

###  Load Waterway shapefile for surrogates 807
set shapefile=NTAD_2014_Waterway
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiLineString       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

### Load Oil Gas shapefile
set indir=$PGDATA/emiss_shp2014/OilGas
set shapefile=Completions_Gas
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiLineString       # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

### Load Shipping shapefile for 805, 806
set shapefile=ShippingLanes_2014NEI
set indir=$PGDATA/emiss_shp2014/EPA
#set indir=/proj/ie/proj/SA/pg_srgcreate/shp_export
set shapefile=ShippingLanes_2014NEI
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon
#source load_shapefile.csh

# Ports, has ring problem?
set indir=$PGDATA/emiss_shp2014/EPA
set shapefile=Ports_2014NEI
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon          # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set indir=$PGDATA/emiss_shp2014/NLCD
set shapefile=nlcd2011_500mv2
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon          # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh

set indir=$PGDATA/emiss_shp2010/fema
set shapefile=fema_bsf_2002bnd
set table=`echo $shapefile | tr "[:upper:]" "[:lower:]"`
set geomtype=MultiPolygon          # retrieve the exact geopmetry type from the table.
#source load_shapefile.csh




