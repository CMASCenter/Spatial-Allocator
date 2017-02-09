#! /bin/csh -f
#******************* Overlay Run Script **************************************
#
#  Example of Overlay mode for MIMS Spatial Allocator
#  This example overlays counties with a grid
#  Created 3/30/2005 by Ben Brunk, Carolina Environmental Program
#
#  Dec. 2007, LR -- projection specification changes
#*****************************************************************************

# Set debug output
setenv DEBUG_OUTPUT N 
# Set executable
setenv EXE "$SA_HOME/bin/32bits/allocator.exe"

# Set Input Directory
setenv DATADIR $SA_HOME/data
setenv OUTPUT $SA_HOME/output

# method of spatial analysis
setenv MIMS_PROCESSING OVERLAY

setenv TIME time

setenv OVERLAY_SHAPE M08_NASH
setenv OVERLAY_TYPE RegularGrid
setenv OVERLAY_MAP_PRJN M08_NASH
setenv OVERLAY_ELLIPSOID "+a=6370000.0,+b=6370000.0"
setenv OVERLAY_ATTRS COUNTY
setenv INPUT_FILE_NAME $DATADIR/shapefiles/cnty_tn.shp
setenv INPUT_FILE_TYPE ShapeFile
setenv INPUT_FILE_ELLIPSOID "+a=6370000.0,+b=6370000.0"
setenv INPUT_FILE_MAP_PRJN "+proj=latlong"
setenv GRIDDESC $DATADIR/GRIDDESC.txt
setenv OVERLAY_OUT_TYPE Stdout 
setenv OVERLAY_OUT_NAME Stdout
setenv OVERLAY_OUT_DELIM COMMA
setenv WRITE_HEADER N


echo "Overlaying counties with a grid"
$TIME $EXE
