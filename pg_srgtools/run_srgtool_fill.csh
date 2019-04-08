#!/bin/csh -f

#setenv LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:/nas01/depts/ie/cempd/SMOKE/dyang/sa/src/vector32/libs/proj-4.6.0/local/lib

#java -classpath ./SurrogateTools-2.1.jar gov.epa.surrogate.SurrogateTool control_variables_pg.csv
java -classpath ./SurrogateTools-2.1.jar gov.epa.surrogate.SurrogateTool control_variables_pg_36km.csv
