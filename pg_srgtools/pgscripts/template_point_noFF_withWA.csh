#!/bin/csh -fx

# prerequisites
# - county shapefile, weight multiline shapefile (hpms roads), and grid cells geometries loaded
# - indices on all geometry columns

#setenv LD_LIBRARY_PATH "${LD_LIBRARY_PATH}:/nas02/apps/gcc-6.1.0/lib:/proj/ie/proj/CMAS/SA/Spatial-Allocator/pg_srgcreate/libs/geos-3.5.1/local/lib:/proj/ie/proj/CMAS/SA/Spatial-Allocator/pg_srgcreate/libs/proj-4.9.3/local/lib:/proj/ie/proj/CMAS/SA/Spatial-Allocator/pg_srgcreate/libs/json-c-json-c-0.12.1-20160607/local/lib: /proj/ie/proj/CMAS/SA/spatial-Allocator/pg_srgcreate/libs/libxml2-2.9.4/local/lib:/proj/ie/proj/CMAS/SA/Spatial-Allocator/pg_srgcreate/libs/gdal-2.1.3/local/lib:/proj/ie/proj/CMAS/SA/Spatial-Allocator/pg_srgcreate/postgresql-9.5.3/lib"


VAR_DEFINITIONS

set grid_table=$schema.$grid
set geom_grid=$grid_table.gridcell
#set cty_table=$schema.cb_2014_us_county_500k_Poly
set data_table=$schema.$data_shape
set weight_table=$schema.$weight_shape
set geom_wp="$weight_table.geom_$grid_proj"
#set weight_attribute="$weight_table.$weight_attribute"

# Create wp_cty intersection table
echo "CREATE TABLE $schema.wp_cty_${surg_code} "
#printf "DROP TABLE IF EXISTS $schema.wp_cty_${surg_code}; \n" > ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "DROP TABLE IF EXISTS $schema.wp_cty_${surg_code}; \n" > ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "CREATE TABLE $schema.wp_cty_${surg_code}(\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "\t$data_attribute varchar(5),\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "\t$weight_attribute double precision);\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "SELECT AddGeometryColumn('${schema}', 'wp_cty_${surg_code}', 'geom_${grid_proj}', ${grid_proj}, 'MULTIPOINT', 2);\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "INSERT INTO $schema.wp_cty_${surg_code} ($weight_attribute, geom_${grid_proj}) \n">> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "SELECT  \n">> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "\t${weight_table}.$weight_attribute, \n">> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "\t${weight_table}.geom_${grid_proj} \n">> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "FROM ${weight_table};\n">> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "update $schema.wp_cty_${surg_code} \n">> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "set $data_attribute=${data_table}.$data_attribute\n">> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "FROM ${data_table}\n">> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "WHERE ST_Contains(${data_table}.geom_$grid_proj, $schema.wp_cty_${surg_code}.geom_${grid_proj});\n">> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "create index on ${schema}.wp_cty_${surg_code} using GIST(geom_${grid_proj});\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "vacuum analyze $schema.wp_cty_${surg_code};\n">> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
printf "DELETE FROM $schema.wp_cty_${surg_code} where $schema.wp_cty_${surg_code}.$data_attribute IS NULL;\n">> ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql
$PGBIN/psql -h $server -d $dbname -U $user -f ${output_dir}/temp_files/${surg_code}_create_wp_cty.sql

# Create wp_cty_cell intersection table
printf "DROP TABLE IF EXISTS $schema.wp_cty_cell_${surg_code}_${grid}; \n" > ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "CREATE TABLE $schema.wp_cty_cell_${surg_code}_${grid}(\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\t$data_attribute varchar(5) not null, \n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\tcolnum integer not null,\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\trownum integer not null,\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\t$weight_attribute double precision) ;\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "SELECT AddGeometryColumn('${schema}', 'wp_cty_cell_${surg_code}_${grid}', 'geom_${grid_proj}', ${grid_proj}, 'MULTIPOINT', 2);\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql

printf "insert into $schema.wp_cty_cell_${surg_code}_${grid} ( $data_attribute, colnum, rownum, $weight_attribute, geom_${grid_proj}) \n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "SELECT $schema.wp_cty_${surg_code}.${data_attribute},\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\t${grid_table}.colnum,\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\t${grid_table}.rownum,\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\t$weight_attribute,\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\t$schema.wp_cty_${surg_code}.geom_${grid_proj} \n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "  FROM $schema.wp_cty_${surg_code} \n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "  JOIN ${grid_table}\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "    ON (ST_Contains(${geom_grid}, $schema.wp_cty_${surg_code}.geom_${grid_proj}));\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "create index on ${schema}.wp_cty_cell_${surg_code}_${grid} using GIST(geom_${grid_proj});\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "vacuum analyze $schema.wp_cty_cell_${surg_code}_${grid};\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
$PGBIN/psql -h $server -d $dbname -U $user -f ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql

# Create denominator table
printf "DROP TABLE IF EXISTS $schema.denom_${surg_code}_${grid}; \n" > ${output_dir}/temp_files/${surg_code}_denom.sql
printf "CREATE TABLE $schema.denom_${surg_code}_${grid} ($data_attribute varchar(5) not null,\n" >> ${output_dir}/temp_files/${surg_code}_denom.sql
printf "\tdenom double precision,\n" >> ${output_dir}/temp_files/${surg_code}_denom.sql
printf "\tprimary key ($data_attribute));\n" >> ${output_dir}/temp_files/${surg_code}_denom.sql
printf "insert into $schema.denom_${surg_code}_${grid}\n" >> ${output_dir}/temp_files/${surg_code}_denom.sql
printf "SELECT $data_attribute,\n" >> ${output_dir}/temp_files/${surg_code}_denom.sql
printf "       SUM($weight_attribute) AS denom\n" >> ${output_dir}/temp_files/${surg_code}_denom.sql
printf "  FROM $schema.wp_cty_cell_${surg_code}_${grid}\n" >> ${output_dir}/temp_files/${surg_code}_denom.sql
printf " GROUP BY $data_attribute;\n" >> ${output_dir}/temp_files/${surg_code}_denom.sql
echo "CREATE TABLE $schema.denom_${surg_code}_${grid}; create primary key"
$PGBIN/psql -h $server -d $dbname -U $user -f ${output_dir}/temp_files/${surg_code}_denom.sql

# Create numerator table
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

# Create surrogate table
printf "DROP TABLE IF EXISTS $schema.surg_${surg_code}_${grid}; \n" > ${output_dir}/temp_files/${surg_code}_surg.sql
printf "CREATE TABLE $schema.surg_${surg_code}_${grid} (surg_code integer not null,\n" >> ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\t$data_attribute varchar(5) not null,\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\t	colnum integer not null,\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\t	rownum integer not null,\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\t	surg double precision,\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\t	numer double precision,\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\t	denom double precision,\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
printf "\t	primary key ($data_attribute, colnum, rownum));\n" >>  ${output_dir}/temp_files/${surg_code}_surg.sql
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

echo "Exporting surrogates $schema.surg_${surg_code}_${grid}; "
echo "#GRID" > ${output_dir}/USA_${surg_code}_NOFILL.txt
$PGBIN/psql -U $user --field-separator '	' -t --no-align ${dbname} << END >> ${output_dir}/USA_${surg_code}_NOFILL.txt 

SELECT surg_code, ${data_attribute}, colnum, rownum, surg, '!', numer, denom
  FROM $schema.surg_${surg_code}_${grid}
  order by $data_attribute, colnum, rownum;
END

