#!/bin/csh -fx

# prerequisites
# - county shapefile, weight multiline shapefile (hpms roads), and grid cells geometries loaded
# - indices on all geometry columns
# - roads split by county boundaries 


VAR_DEFINITIONS

set grid_table=$schema.$grid
set geom_grid=$grid_table.gridcell
set data_table=$schema.$data_shape
set weight_table=$schema.$weight_shape


# intersect with grid table
printf "DROP TABLE IF EXISTS $schema.wp_cty_cell_${surg_code}_${grid}; \n" > ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "CREATE TABLE $schema.wp_cty_cell_${surg_code}_${grid}  ($data_attribute varchar(5) not null,\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\tcolnum integer not null,\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\trownum integer not null,\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
  printf "\tlength_wp_cty_cell double precision default 0.1) ;\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql

printf "SELECT AddGeometryColumn('${schema}', 'wp_cty_cell_${surg_code}_${grid}', 'geom_${grid_proj}', ${grid_proj},'MultiLineString', 2);\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql

printf "insert into $schema.wp_cty_cell_${surg_code}_${grid}\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "SELECT ${data_attribute},\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\tcolnum,\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\trownum,\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
  printf "\t0.0,\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\tCASE\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\twhen ST_CoveredBy(${weight_table}.geom_${grid_proj},${grid_table}.gridcell)\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\tTHEN ${weight_table}.geom_${grid_proj}\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\tELSE	\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\t\tST_Multi(ST_Intersection(${weight_table}.geom_${grid_proj},${grid_table}.gridcell)) \n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "\tEND AS geom_${grid_proj} \n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "  FROM ${weight_table}\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "  JOIN ${grid_table} \n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "    ON (NOT ST_Touches(${weight_table}.geom_${grid_proj},${grid_table}.gridcell)\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "    	AND ST_Intersects(${weight_table}.geom_${grid_proj},${grid_table}.gridcell))\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "    WHERE ${weight_table}.${filter_function}; \n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql

printf "create index on $schema.wp_cty_cell_${surg_code}_${grid} using GIST(geom_${grid_proj});\n" >>  ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql

printf "vacuum analyze $schema.wp_cty_cell_${surg_code}_${grid};\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql

printf "UPDATE $schema.wp_cty_cell_${surg_code}_${grid}\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
printf "   SET length_wp_cty_cell = ST_Length(geom_${grid_proj});\n" >> ${output_dir}/temp_files/${surg_code}_create_wp_cty_cell.sql
echo "CREATE TABLE $schema.wp_cty_cell_${surg_code}_${grid}; insert geometries, create index GIST; vacuum & analyze; compute length"
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
printf "\tSUM(length_wp_cty_cell) AS numer\n"  >> ${output_dir}/temp_files/${surg_code}_numer.sql
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
printf "\tSUM(length_wp_cty_cell) AS denom\n"  >> ${output_dir}/temp_files/${surg_code}_denom.sql
printf "  FROM $schema.wp_cty_cell_${surg_code}_${grid}\n" >> ${output_dir}/temp_files/${surg_code}_denom.sql
printf " GROUP BY $data_attribute;\n" >> ${output_dir}/temp_files/${surg_code}_denom.sql
echo "CREATE TABLE $schema.denom_${surg_code}_${grid}; create primary key"
$PGBIN/psql -h $server -d $dbname -U $user -f ${output_dir}/temp_files/${surg_code}_denom.sql

# Calculate surrogate
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

# Export surrogate
echo "Exporting surrogates $schema.surg_${surg_code}_${grid}; "
echo "#GRID" > ${output_dir}/USA_${surg_code}_NOFILL.txt
$PGBIN/psql -U $user --field-separator '	' -t --no-align ${dbname} << END >> ${output_dir}/USA_${surg_code}_NOFILL.txt 

SELECT surg_code, ${data_attribute}, colnum, rownum, surg, '!', numer, denom
  FROM $schema.surg_${surg_code}_${grid}
  order by ${data_attribute}, colnum, rownum;
END
