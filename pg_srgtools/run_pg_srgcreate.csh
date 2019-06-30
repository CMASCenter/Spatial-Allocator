#!/bin/csh -f

source pg_setup.csh
java -classpath ./SurrogateTools-2.1.jar gov.epa.surrogate.ppg.Main control_variables_pg.csv

#merge and gapfilling
java -classpath ./SurrogateTools-2.1.jar gov.epa.surrogate.SurrogateTool control_variables_pg.csv

