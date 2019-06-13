#!/bin/csh -fx
# acs_2014_grid_script.sh
# AFTER sucessful completion of acs_2014_prcs_script.sh; performs grid overlay and related calculations
# handles reprojections of geometry objects (geographic polygons), vacuum, analyze, and cluster as appropriate

VAR_DEFINITIONS

setenv cluster false					
setenv shp_tbl_d `echo $data_shape | tr "[:upper:]" "[:lower:]"` 
setenv shp_tbl_w `echo $weight_shape | tr "[:upper:]" "[:lower:]"` 
set data_table=$schema.${shp_tbl_d}
set weight_table=$schema.${shp_tbl_w}
set grid_table=${schema_name}.${grid}
set geom_grid=$grid_table.gridcell
set geom_data=${data_table}.geom_${srid_final}
set geom_weight=${weight_table}.geom_${srid_final}


echo $data_table
# cut with geographic boundaries
printf "DROP TABLE IF EXISTS ${schema_name}.wp_cty_${surg_code};\n" > ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "CREATE TABLE ${schema_name}.wp_cty_${surg_code}\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "\t(${data_attribute} varchar (6) not null,\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "\t${weight_attribute} double precision default 0.0,\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "\t${weight_attribute}_dens double precision default 0.0,\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "\tarea_${srid_final} double precision default 0.0);\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "SELECT AddGeometryColumn('${schema_name}', 'wp_cty_${surg_code}', 'geom_${srid_final}', ${srid_final}, 'MultiPolygon', 2);\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql

printf "insert into ${schema_name}.wp_cty_${surg_code}\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "SELECT ${data_table}.${data_attribute},\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "\t${weight_attribute},\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "\t${weight_attribute}_dens,\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "        0.0,\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
# When data shape file and weight shapfile are same
if ( ${data_table} == ${weight_table} ) then 
  printf "        geom_${srid_final}\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
  printf "FROM ${data_table};\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
else 
  printf "\tCASE\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
  printf "\t\twhen ST_CoveredBy(${geom_weight},${geom_data})\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
  printf "\t\tTHEN ${geom_weight}\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
  printf "\tELSE\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
  printf "\t\tST_CollectionExtract(ST_Multi(ST_Intersection(${geom_weight},${geom_data})), 3)\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
  printf "\tEND AS geom_${srid_final}\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
  printf "FROM ${data_table}\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
  printf "\tJOIN ${weight_table}\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
  printf "\tON (NOT ST_Touches(${geom_weight},${geom_data})\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
  printf "\t\tAND ST_Intersects(${geom_weight},${geom_data}));\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
endif
printf "update  ${schema_name}.wp_cty_${surg_code} set area_${srid_final}=ST_Area(geom_${srid_final});\n"  >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "update  ${schema_name}.wp_cty_${surg_code} set\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "\t${weight_attribute}=${weight_attribute}_dens * area_${srid_final};\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "create index on ${schema}.wp_cty_${surg_code} using GIST(geom_${grid_proj});\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "\tvacuum analyze ${schema_name}.wp_cty_${surg_code};" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
echo "Cutting by data shapefile boundaries"
$PGBIN/psql -h $server -d $dbname -U $user password='sepia6#vibes' -f ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql


# create query to grid weight data
printf "DROP TABLE IF EXISTS wp_cty_cell_${surg_code}_${grid}; \n" > ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
echo "CREATE TABLE ${schema_name}.wp_cty_cell_${surg_code}_${grid} (" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\t${data_attribute} varchar (6) not null,\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\tcolnum integer not null,\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\trownum integer not null,\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\tarea_${srid_final} double precision default 1.0,\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\t${weight_attribute} double precision default 0.0,\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql 
printf "\t${weight_attribute}_dens double precision default 0.0);\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql 

printf "SELECT AddGeometryColumn('${schema_name}', 'wp_cty_cell_${surg_code}_${grid}', 'geom_${srid_final}', ${srid_final}, 'MultiPolygon', 2);\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
	
printf "INSERT INTO ${schema_name}.wp_cty_cell_${surg_code}_${grid}\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\tSELECT ${data_attribute}, colnum, rownum,\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "        0.0,\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\t${weight_attribute},\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\t${weight_attribute}_dens,\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\tCASE\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\twhen ST_CoveredBy(${schema}.wp_cty_${surg_code}.geom_${grid_proj}, ${grid_table}.gridcell)\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\t\tTHEN wp_cty_${surg_code}.geom_${grid_proj}\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\t\tELSE\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\t\t\tST_CollectionExtract(ST_Multi(ST_Intersection(${schema}.wp_cty_${surg_code}.geom_${grid_proj}, ${grid_table}.gridcell)),3)\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\t\tEND AS geom_${srid_final}\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\tFROM ${schema}.wp_cty_${surg_code}\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\tJOIN ${grid_table}\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\tON (NOT ST_Touches(${schema}.wp_cty_${surg_code}.geom_${grid_proj}, ${grid_table}.gridcell)\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\t\tAND ST_Intersects(${schema}.wp_cty_${surg_code}.geom_${grid_proj}, ${grid_table}.gridcell));\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "UPDATE ${schema_name}.wp_cty_cell_${surg_code}_${grid} set area_${srid_final}=ST_Area(geom_${srid_final});\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "UPDATE ${schema_name}.wp_cty_cell_${surg_code}_${grid} set\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\t${weight_attribute}=${weight_attribute}_dens * area_${srid_final};\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "create index  on $schema.wp_cty_cell_${surg_code}_${grid} using GIST(geom_${grid_proj});\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\tvacuum analyze ${schema_name}.wp_cty_cell_${surg_code}_${grid};" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
echo "Gridding weight data to modeling domain"
$PGBIN/psql -h $server -d $dbname -U $user -f ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql

# Create numerater table
printf "DROP TABLE IF EXISTS $schema.numer_${surg_code}_${grid}; \n" > ${output_dir}/temp_files/${surg_code}_numer.sql
printf "CREATE TABLE $schema.numer_${surg_code}_${grid} ($data_attribute varchar(5) not null,\n" >> ${output_dir}/temp_files/${surg_code}_numer.sql
printf "\tcolnum integer not null,\n" >> ${output_dir}/temp_files/${surg_code}_numer.sql
printf "\trownum integer not null,\n" >> ${output_dir}/temp_files/${surg_code}_numer.sql
printf "\tnumer double precision,\n" >> ${output_dir}/temp_files/${surg_code}_numer.sql
printf "\tprimary key ($data_attribute, colnum, rownum));\n" >> ${output_dir}/temp_files/${surg_code}_numer.sql
printf "insert into $schema.numer_${surg_code}_${grid}\n" >> ${output_dir}/temp_files/${surg_code}_numer.sql
printf "SELECT $data_attribute,\n" >> ${output_dir}/temp_files/${surg_code}_numer.sql
printf "\tcolnum,\n" >> ${output_dir}/temp_files/${surg_code}_numer.sql
printf "\trownum,\n" >> ${output_dir}/temp_files/${surg_code}_numer.sql
printf "\tSUM($weight_attribute) AS numer\n" >> ${output_dir}/temp_files/${surg_code}_numer.sql
printf "  FROM $schema.wp_cty_cell_${surg_code}_${grid}\n" >> ${output_dir}/temp_files/${surg_code}_numer.sql
printf " GROUP BY $data_attribute, colnum, rownum;\n" >> ${output_dir}/temp_files/${surg_code}_numer.sql
echo "CREATE TABLE $schema.numer_${surg_code}_${grid}"
$PGBIN/psql -h $server -d $dbname -U $user -f ${output_dir}/temp_files/${surg_code}_numer.sql

# Calculate donominator
printf "DROP TABLE IF EXISTS $schema.denom_${surg_code}_${grid}; \n" > ${output_dir}/temp_files/${surg_code}_denom.sql
printf "CREATE TABLE $schema.denom_${surg_code}_${grid} ($data_attribute varchar(5) not null,\n" >> ${output_dir}/temp_files/${surg_code}_denom.sql
printf "\tdenom double precision,\n" >> ${output_dir}/temp_files/${surg_code}_denom.sql
printf "\tprimary key ($data_attribute));\n" >> ${output_dir}/temp_files/${surg_code}_denom.sql
printf "insert into $schema.denom_${surg_code}_${grid}\n" >> ${output_dir}/temp_files/${surg_code}_denom.sql
printf "SELECT $data_attribute,\n" >> ${output_dir}/temp_files/${surg_code}_denom.sql
printf "\tSUM($weight_attribute) AS denom\n" >> ${output_dir}/temp_files/${surg_code}_denom.sql
printf "  FROM $schema.wp_cty_cell_${surg_code}_${grid}\n" >> ${output_dir}/temp_files/${surg_code}_denom.sql
printf " GROUP BY $data_attribute;\n" >> ${output_dir}/temp_files/${surg_code}_denom.sql
echo "CREATE TABLE $schema.denom_${surg_code}_${grid}; create primary key"
$PGBIN/psql -h $server -d $dbname -U $user -f ${output_dir}/temp_files/${surg_code}_denom.sql

# Calculate surrogate
printf "DROP TABLE IF EXISTS $schema.surg_${surg_code}_${grid}; \n" > ${output_dir}/temp_files/${surg_code}_surg.sql
printf "CREATE TABLE $schema.surg_${surg_code}_${grid} (surg_code integer not null,\n" >> ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\t$data_attribute varchar(5) not null,\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\t      colnum integer not null,\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\t      rownum integer not null,\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\t      surg double precision,\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\t      numer double precision,\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\t      denom double precision,\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\t      primary key ($data_attribute, colnum, rownum));\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "insert into $schema.surg_${surg_code}_${grid}\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "SELECT CAST('$surg_code' AS INTEGER) AS surg_code,\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\td.$data_attribute,\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\tcolnum,\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\trownum,\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\tnumer / denom AS surg,\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\tnumer,\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\tdenom\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "  FROM $schema.numer_${surg_code}_${grid} n\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "  JOIN $schema.denom_${surg_code}_${grid} d\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf " USING ($data_attribute)\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf " WHERE numer != 0\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "   AND denom != 0\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf " GROUP BY d.$data_attribute, colnum, rownum, numer, denom\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf " ORDER BY d.$data_attribute, colnum, rownum;\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
echo "CREATE TABLE $schema.surg_${surg_code}_${grid}; add primary key"
$PGBIN/psql -h $server -d $dbname -U $user -f ${output_dir}/temp_files/${surg_code}_surg.sql

# Export surrogate
echo "Exporting surrogates $schema.surg_${surg_code}_${grid}; "
echo "#GRID" > ${output_dir}/USA_${surg_code}_NOFILL.txt
$PGBIN/psql -h $server -d $dbname -U $user --field-separator '	' -t --no-align ${dbname} << END >> ${output_dir}/USA_${surg_code}_NOFILL.txt

SELECT surg_code, ${data_attribute}, colnum, rownum, surg, '!', numer, denom
  FROM $schema.surg_${surg_code}_${grid}
  order by ${data_attribute}, colnum, rownum;
END
