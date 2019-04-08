#!/bin/csh -f

#java -classpath ./SurrogateTools-2.1.jar  gov.epa.surrogate.normalize.Main ./outputs/srgdesc_us12k.txt 
java -classpath ./SurrogateTools-2.1.jar  gov.epa.surrogate.normalize.Main ./outputs/srgdesc_us36k.txt  exclude_list 0.0005
