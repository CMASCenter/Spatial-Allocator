#!/bin/csh -f
#SBATCH -p general
#SBATCH -N 1
#SBATCH -n 1
#SBATCH --mem=8g
#SBATCH -t 8-00:00:00  
#SBATCH -o pg_dump_tables.log
#SBATCH -J pg_dump

source ../pg_setup.csh
$PGHOME/pg_setup_dyang.csh stop
sleep 20
$PGHOME/pg_setup_dyang.csh start
sleep 20

if ( ! -e ${PGDATA}/pg_tables ) mkdir -p ${PGDATA}/pg_tables
echo ${PGDATA}/pg_tables
#foreach table ( cb_2014_us_county_500k_Poly  airport_area ACS_2014_5YR_PopHousing )
#foreach table ( pil_2018_08_17 hpms2016  ERTAC_railyard_WRF NTAD_2014_County_Pol )
#foreach table ( cb_2014_us_county_500k_Poly ACS_2014_5YR_PopHousing  us36k_172x148  us12k_444x336 us4k_1332x1008 )
foreach table ( `cat "table_list_2014.txt" ` )
  echo $table
  $PGBIN/pg_dump --no-owner -t $table -U $PG_USER $DBNAME -f  ${PGDATA}/pg_tables/${table}.sql
end

$PGHOME/pg_setup_dyang.csh stop
