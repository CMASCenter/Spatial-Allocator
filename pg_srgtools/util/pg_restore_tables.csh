#!/bin/csh -f
#SBATCH -p general
#SBATCH -N 1
#SBATCH -n 1
#SBATCH --mem=8g
#SBATCH -t 8-00:00:00  
#SBATCH -o pg_dump_tables.log
#SBATCH -J PG_SRG

source ../pg_setup.csh
$PGHOME/pg_setup_dumptest.csh stop
sleep 20
$PGHOME/pg_setup_dumptest.csh start
sleep 20

$PGBIN/psql $DBNAME -f create_900921.sql
#foreach table ( cb_2014_us_county_500k_Poly  airport_area ACS_2014_5YR_PopHousing )
#foreach table ( cb_2014_us_county_500k_Poly ACS_2014_5YR_PopHousing  us36k_172x148 ) 
foreach table ( `cat "table_list_2014.txt" ` )
   echo $table
  $PGBIN/psql $DBNAME -f  ${PGDATA}/pg_tables/${table}.sql
end

$PGHOME/pg_setup_dyang.csh stop
