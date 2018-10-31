#!/bin/csh -f
#BSUB -M 96
#BSUB -q week
#BSUB -J 9AK2_711
#BSUB -o LOGS/qlog.9AK2.%J.log

setenv LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:/nas01/depts/ie/cempd/SMOKE/dyang/sa/src/vector32/libs/proj-4.6.0/local/lib

java -classpath ./bin/SurrogateTools-2.1.jar gov.epa.surrogate.SurrogateTool control_variables_pg.csv
#java -classpath ./SurrogateTools.jar gov.epa.surrogate.SurrogateTool control_variables_pg.csv
