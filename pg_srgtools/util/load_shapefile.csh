#!/bin/csh -fx

# Loading the data shapefile first and then reproject to the output modeling domain
# Loading once for processing multiple modeling domains

# 1. Load the original data shapefile (shp) with its projection (prj)

setenv shp_tbl `echo $shapefile | tr "[:upper:]" "[:lower:]"`

echo $shapefile
#if ( $shp_tbl == "acs_2014_5yr_pophousing" ) then
  $GDALBIN/ogr2ogr -f "PostgreSQL" "PG:dbname=$dbname user=$user" $indir/$shapefile.shp -lco PRECISION=NO -nlt PROMOTE_TO_MULTI -nln $schema.$table -overwrite
#else
#  $GDALBIN/ogr2ogr -f "PostgreSQL" "PG:dbname=$dbname user=$user" $indir/$shapefile.shp -lco UPLOAD_GEOM_FORMAT=wkt -nlt PROMOTE_TO_MULTI -nln $schema.$table -overwrite
#endif

#/usr/bin/ogr2ogr -f "PostgreSQL" "PG:dbname=$dbname user=$user" $indir/$shapefile.shp -lco UPLOAD_GEOM_FORMAT=wkt -nlt PROMOTE_TO_MULTI -nln $schema.$table -overwrite
# 2. Tranform to a new projection (from original 900922 to new 900921) and create gist index on it

if ( $table == "hpms2016" ) then
  ALTER TABLE $table ALTER COLUMN moves2014 TYPE INT USING moves2014::integer;
endif 


$PGBIN/psql -U $user -q $dbname << END1
ALTER TABLE $schema.$table ADD COLUMN $newfield geometry($geomtype, $srid);
-- CREATE INDEX ON $schema.$table USING GIST ($newfield);
UPDATE $schema.$table SET $newfield = ST_Transform($org_geom_field, $srid);
drop index if exists $schema.${table}_${org_geom_field}_geom_idx;
create index on $schema.${table} using GIST(geom_900921);
END1

# Calculate density
if ( $table == "acs_2014_5yr_pophousing" ) then
psql -U $user -q $dbname << END1
  ALTER TABLE public.$table
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
  update public.$table
        set area_900921=ST_Area(geom_900921);
  update public.$table set
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
psql -U $user -q $dbname << END1
  ALTER TABLE public.$table
        add column area_900921 double precision,
        add column area_sqmi_dens double precision,
        add column activitykw_dens double precision;
  update public.$table
        set area_900921=ST_Area(geom_900921);
  update public.$table set
        area_sqmi_dens=area_sqmi/area_900921,
        activitykw_dens=activitykw/area_900921;
END1
endif

if ( $table == "fema_bsf_2002bnd" ) then
psql -U $user -q $dbname << END1
  ALTER TABLE public.fema_bsf_2002bnd
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
  update public.fema_bsf_2002bnd set
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
    ALTER TABLE public.fema_bsf_2002bnd
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
  update public.fema_bsf_2002bnd
        set area_900921=ST_Area(geom_900921);
  update public.fema_bsf_2002bnd set
        su_500_dens=su_500 / area_900921,
        su_505_dens=su_500 / area_900921,
        su_506_dens=su_500 / area_900921,
        su_507_dens=su_500 / area_900921,
        su_510_dens=su_500 / area_900921,
        su_515_dens=su_500 / area_900921,
        su_520_dens=su_500 / area_900921,
        su_526_dens=su_500 / area_900921,
        su_535_dens=su_500 / area_900921,
        su_555_dens=su_500 / area_900921,
        su_575_dens=su_500 / area_900921,
        su_596_dens=su_505 / area_900921;
END1
endif

# 3. Check whether the shapefile data are imported correclty or not.
$PGBIN/psql -U $user -q $dbname << END1
update ${schema}.${table}
        SET ${newfield} = ST_MakeValid(${newfield})
       WHERE NOT ST_IsValid(${newfield});
END1

