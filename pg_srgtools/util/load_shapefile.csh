##!/bin/csh -fx

# Loading the data shapefile first and then reproject to the output modeling domain
# Loading once for processing multiple modeling domains

# 1. Load the original data shapefile (shp) with its projection (prj)

setenv shp_tbl `echo $shapefile | tr "[:upper:]" "[:lower:]"`

echo $shapefile
#if ( $shp_tbl == "acs_2014_5yr_pophousing" ) then
  $GDALBIN/ogr2ogr -f "PostgreSQL" "PG:dbname=$dbname user=$user host=$server" $indir/$shapefile.shp -lco PRECISION=NO -nlt PROMOTE_TO_MULTI -nln $schema.$table -overwrite
# $GDALBIN/ogr2ogr -f "PostgreSQL" "PG:dbname=$dbname user=$user" $indir/$shapefile.shp -lco UPLOAD_GEOM_FORMAT=wkt -nlt PROMOTE_TO_MULTI -nln $schema.$table -overwrite
#endif

#/usr/bin/ogr2ogr -f "PostgreSQL" "PG:dbname=$dbname user=$user" $indir/$shapefile.shp -lco UPLOAD_GEOM_FORMAT=wkt -nlt PROMOTE_TO_MULTI -nln $schema.$table -overwrite
# 2. Tranform to a new projection (from original 900922 to new 900921) and create gist index on it
$PGBIN/psql -h $server -U $user -q $dbname << END1
ALTER TABLE $schema.$table ADD COLUMN $newfield geometry($geomtype, $srid);
-- CREATE INDEX ON $schema.$table USING GIST ($newfield);
UPDATE $schema.$table SET $newfield = ST_Transform($org_geom_field, $srid);
drop index if exists $schema.${table}_${org_geom_field}_geom_idx;
create index on $schema.${table} using GIST(geom_900921);
END1


# add columns
if ( $table == "hpms2016" ) then
$PGBIN/psql -h $server -U $user -q $dbname << END1
  ALTER TABLE $schema.$table ALTER COLUMN moves2014 TYPE INT USING moves2014::integer;
  update $schema.$table set fips='46113'  where fips='46102';
  ALTER TABLE $schema.$table
    add column length_900921 double precision,
    add column aadt_dens double precision;
  update $schema.$table
    set length_900921=ST_Length(geom_900921),
    aadt_dens=aadt/length_900921;
END1
endif


# add column geoid (stctyfips) to "ntad_2014_rail" table
if ( $table == "ntad_2014_rail" ) then
$PGBIN/psql -h $server -U $user -q $dbname << END1
  ALTER TABLE $schema.$table add column geoid varchar(5);
  update $schema.$table set geoid=concat(statefips, cntyfips);
END1
endif

#
#if ( $table == "ntad_2014_ipcd" ) then
#psql  -h $server -U $user -q $dbname << END1
#  ALTER TABLE $schema.$table RENAME pt_id_lon TO longitude;
#  ALTER TABLE $schema.$table RENAME pt_id_lat TO latitude;
#END1
#endif

#if ( $table == "poi_factory_2015_golfcourses" ) then
#psql  -h $server -U $user -q $dbname << END1
#  ALTER TABLE $schema.$table RENAME lon TO longitude;
#  ALTER TABLE $schema.$table RENAME lat TO latitude;
#END1
#endif
#if ( $table == "ertac_railyard_wrf" ) then
#psql -h $server -U $user -q $dbname << END1
#  ALTER TABLE $schema.$table RENAME lon TO longitude;
#  ALTER TABLE $schema.$table RENAME lat TO latitude;
#END1
#endif


# Calculate density
if ( $table == "acs_2014_5yr_pophousing" ) then
$PGBIN/psql -h $server -U $user -q $dbname << END1
  ALTER TABLE $schema.$table
        add column area_900921 double precision,
        add column pop2014_dens double precision,
        add column hu2014_dens double precision,
        add column popch14_10_dens double precision,
        add column huch14_10_dens double precision,
        add column pop2010_dens double precision,
        add column hu2010_dens double precision,
        add column pop2000_dens double precision,
        add column hu2000_dens double precision,
        add column util_gas_dens double precision,
        add column wood_dens double precision,
        add column fuel_oil_dens double precision,
        add column coal_dens double precision,
        add column lp_gas_dens double precision,
        add column elec_dens double precision,
        add column solar_dens double precision;
  update $schema.$table
        set area_900921=ST_Area(geom_900921);
  update $schema.$table set
        pop2014_dens=pop2014 / area_900921,
        hu2014_dens=hu2014 / area_900921,
        popch14_10_dens=popch14_10 / area_900921,
        huch14_10_dens=huch14_10 / area_900921,
        pop2010_dens=pop2010 / area_900921,
        hu2010_dens=hu2010 / area_900921,
        pop2000_dens=pop2000 / area_900921,
        hu2000_dens=hu2000 / area_900921,
        util_gas_dens=util_gas / area_900921,
        wood_dens=wood / area_900921,
        fuel_oil_dens=fuel_oil / area_900921,
        coal_dens=coal / area_900921,
        lp_gas_dens=lp_gas / area_900921,
        elec_dens=elec / area_900921,
        solar_dens=solar / area_900921;
END1
endif

if ( $table == "shippinglanes_2014nei" || $table == "ports_2014nei" ) then
psql -h $server -U $user -q $dbname << END1
  ALTER TABLE $schema.$table
        add column area_900921 double precision,
        add column area_sqmi_dens double precision,
        add column activitykw_dens double precision;
  update $schema.$table
        set area_900921=ST_Area(geom_900921);
  update $schema.$table set
        area_sqmi_dens=area_sqmi/area_900921,
        activitykw_dens=activitykw/area_900921;
END1
endif

if ( $table == "fema_bsf_2002bnd" ) then
psql -h $server -U $user -q $dbname << END1
  ALTER TABLE ${schema}.${table}
        add column su_500 double precision,
        add column su_505 double precision,
        add column su_506 double precision,
        add column su_507 double precision,
        add column su_510 double precision,
        add column su_515 double precision,
        add column su_520 double precision,
        add column su_526 double precision,
        add column su_535 double precision,
        add column su_555 double precision,
        add column su_575 double precision,
        add column su_596 double precision;
  update ${schema}.${table} set
       su_500=com1+com2+com3+com4+com5+com6+com7+com8+com9,
       su_505=ind1+ind2+ind3+ind4+ind5+ind6,
       su_506=edu1+edu2,
       su_507=ind1+ind2+ind6,
       su_510=com1+com2+com3+com4+com5+com6+com7+com8+com9+ind1+ind2+ind3+ind4+ind5+ind6,
       su_515=com1+com2+com3+com4+com5+com6+com7+com8+com9+res5+res6+edu1+edu2+rel1,
       su_520=com1+com2+com3+com4+com5+com6+com7+com8+com9+ind1+ind2+ind3+ind4+ind5+ind6+res5+res6+edu1+edu2+rel1,
       su_526=RES1+RES2+RES3,
       su_535=com1+com2+com3+com4+com5+com6+com7+com8+com9+ind1+ind2+ind3+ind4+ind5+ind6+edu1+edu2+rel1+gov1+gov2+res1+res2+res3+res4,
       su_555=com4+gov1,
       su_575=ind2+ind5,
       su_596=ind1+ind2+ind3+ind4+ind5+ind6+com6+edu1+edu2+rel1+res5+res6;
    ALTER TABLE ${schema}.${table}
        add column area_900921 double precision,
        add column su_500_dens double precision,
        add column su_505_dens double precision,
        add column su_506_dens double precision,
        add column su_507_dens double precision,
        add column su_510_dens double precision,
        add column su_515_dens double precision,
        add column su_520_dens double precision,
        add column su_526_dens double precision,
        add column su_535_dens double precision,
        add column su_555_dens double precision,
        add column su_575_dens double precision,
        add column su_596_dens double precision;
  update ${schema}.${table}
        set area_900921=ST_Area(geom_900921);
  update ${schema}.${table} set
        su_500_dens=su_500 / area_900921,
        su_505_dens=su_505 / area_900921,
        su_506_dens=su_506 / area_900921,
        su_507_dens=su_507 / area_900921,
        su_510_dens=su_510 / area_900921,
        su_515_dens=su_515 / area_900921,
        su_520_dens=su_520 / area_900921,
        su_526_dens=su_526 / area_900921,
        su_535_dens=su_535 / area_900921,
        su_555_dens=su_555 / area_900921,
        su_575_dens=su_575 / area_900921,
        su_596_dens=su_596 / area_900921;
  ALTER TABLE ${schema}.${table}
        add column res1_dens double precision,
        add column com1_dens double precision,
        add column com3_dens double precision,
        add column com6_dens double precision,
        add column ind1_dens double precision,
        add column ind2_dens double precision,
        add column ind3_dens double precision,
        add column ind4_dens double precision;
  update ${schema}.${table} set
        res1_dens=res1 / area_900921,
        com1_dens=com1 / area_900921,
        com3_dens=com3 / area_900921,
        com6_dens=com6 / area_900921,
        ind1_dens=ind1 / area_900921,
        ind2_dens=ind2 / area_900921,
        ind3_dens=ind3 / area_900921,
        ind4_dens=ind4 / area_900921;
END1
endif

# polygons, oil gas
echo $attr   $geomtype
if ( $attr != "" && $geomtype == "MultiPolygon" ) then
psql -h $server -U $user -q $dbname << END1
  ALTER TABLE ${schema}.${table}
        add column ${attr}_dens double precision,
        add column area_900921 double precision;
  update ${schema}.$table
        set area_900921=ST_Area(geom_900921);
  update ${schema}.$table set
        ${attr}_dens=${attr}/area_900921;
END1
endif


# 3. Check whether the shapefile data are imported correclty or not.
$PGBIN/psql -h $server -U $user -q $dbname << END1
update ${schema}.${table}
        SET ${newfield} = ST_MakeValid(${newfield})
       WHERE NOT ST_IsValid(${newfield});
END1

