#!/bin/csh -f
#SBATCH -p general
#SBATCH -N 1
#SBATCH -n 1
#SBATCH --mem=8g
#SBATCH -t 8-00:00:00  
#SBATCH -o LOGS/pgsrg.%J.log
#SBATCH -J PG_SRG

# start server
/proj/ie/proj/EMAQ/Platform/Surrogates/2014/Spatial-Allocator/pg_srgcreate/pg_setup_dyang.csh stop
sleep 20
/proj/ie/proj/EMAQ/Platform/Surrogates/2014/Spatial-Allocator/pg_srgcreate/pg_setup_dyang.csh start
sleep 20

#source pg_setup.csh
#java -classpath ./SurrogateTools-2.1.jar gov.epa.surrogate.ppg.Main control_variables_pg_36km.csv
#./util/generate_modeling_grid.sh
java -classpath ./SurrogateTools-2.1.jar gov.epa.surrogate.ppg.Main control_variables_pg_4km.csv

# For merge and gapfilling
#java -classpath ./SurrogateTools-2.1.jar gov.epa.surrogate.SurrogateTool control_variables_pg_36km.csv

# stop server
/proj/ie/proj/EMAQ/Platform/Surrogates/2014/Spatial-Allocator/pg_srgcreate/pg_setup_dyang.csh stop
