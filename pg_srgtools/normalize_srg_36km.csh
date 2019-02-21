#!/bin/csh -f

#cd ../prod_2014/output/dist/3HI1/
java -classpath ./bin/SurrogateTools-2.1.jar  gov.epa.surrogate.normalize.Main ./outputs/srgdesc_us12k.txt 
