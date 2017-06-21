# Spatial Allocator Tools Release Notes

Institute for the Environment, UNC-Chapel Hill

### June 2017 Update notes for SA 4.3.1

The Spatial Allocator is now being distributed through GitHub (https://github.com/CMASCenter/Spatial-Allocator). 
* Releases available on github: 4.3 and 4.3.1. 
* Past releases are available from the [CMAS Center](http://www.cmascenter.org).

06/2017 Update Release Notes for SA 4.3.1
#### Raster Tools:  
* Added input data files for the FESTC release, includes crop fraction data for US and Canada for 42 different crops for the year 2011 which is used for 2011 BELD4 data generation within FEST-C.
* Updated executables and source code for extractEPIC2CMAQ and extractEPICYearlyAverage2CMAQ  
(Minor modification of the source code/executables for the averaging and the extraction of EPIC output for CMAQ)

02/2017 Update Release Notes for SA 4.3

#### Vector Tools: 
* Update Earth Radius to 6370000
* Correct bug in Polar Stereographic grid description format
* Added Beld4SMK tool to the release
* Added 64-bit executables
* Update reference data

### Documentation
* All documentation ported to GitHub Markdown

### 05/31/2016 Update notes for SA 4.2:

#### Raster Tools:
* EPIC site info tool update for elevation output

#### Vector Tools:
* Update projection radius to WRF 6370000
* Bug fix for makefile

### 02/05/2016 Update Update Release Notes for SA 4.2:

#### Raster Tools:
* Updated elevation output format for EPIC site information 
* Modified EPIC extraction tools for added EPIC output variables

#### Vector Tools:
* Fixed an error in polygon surrogate computation

### 09/30/2015 Update Release Notes for SA 4.2:

#### Vector Tools:
* Fixed an error in re-gridding the BELD3 data for CMAQ.
* Fixed an error in surroagte computation related to holes in polyogns.

#### Raster Tools:
*  Updated extraction tools for FEST-C output because of changed format and variable name definitions.

#### Surrogate Tools:
* Fixed a bug related with polygon surrogate normaliztion when st+county code is 4 digit.
* Fixed a bug in surrogate normalization in reading surrogate data lines with leading spaces and no "!" sign.
* Added QA threshold for reporting.
* Updated the surrogate Merging Tool to output QA information on the source surrogates.
* Added leading "0" for one digit state codes in both merging and normalization tools.

### New Features Added in May 2014 Release

-   No updates in Vector Tools.
-   Updated Raster Tools.
-   Updated Surrogate Tools (e.g., added normalization to polygon surrogates and fixed a bug in merging polygon surrogates).

### New Features Added in October 2013 Release

-   New BELD4 tool for 2001 or 2006 NLCD/MODIS land cover and crop data.
-   Updated Java-based merging and gapfilling tools to work for polygon shapefile surrogate computation (e.g. census tracts).

### New Features Added in May 2013

The Spatial Allocator set of tools have been updated since release 3.5, Sept. 2008. The following updates have been made:

1.  Raster Tools to process land use data and satellite images and to integrate WRF/CMAQ with EPIC were developed.
2.  Reorganized directory structure:

### New Features Added in December 2007

In the fall 2007, the following changes were made:

-   Updated PROJ4.0 library to the current version PROJ4.5.0. **NOTE: due to this change, the map projection called LATLON and ellipsoid called SPHERE are no longer suppored**.
-   Added datum transformation files to the PROJ4.5.0 installation
-   Changed the Spatail Allocator projection codes to use the new function in PROJ4.5.0 to handle datum transformation while converting projections
-   Modified all scripts in the release with the new projection specification.

### New Features Added in October 2006

In October 2006, the following changes were made:

-   Added support for CMAQ OCEANfile and area percentage allocation mode.

### New Features Added in Spring and Summer 2006

In the spring and summer of 2006, the following changes were made:

-   Support for the rotated lat-lon WRF-nmm E-Grid was added to allocator, srgcreate, and beld3smk.
-   Support for polygon-based surrogates (e.g. for census tracts) was added to srgcreate and srgmerge.-   Many robustness improvements were made to the software.
-   Corrections were made to properly process input Shapefiles have multiple Shape entries in for the same Shape-   Srgcreate supports the DENOMINATOR_THRESHOLD variable to specify when shapes should be ignored, so that small slivers due to data and weight Shapefiles that don't quite line up (e.g. for census tracts and cou
nties for urban populoation surrogate) don't generate actual values of surrogates and prevent proper gapfi
lling.
-   Srgmerge updates 8 decimal place surrogates.
-   When a surrogate does not sum to 1 for a county, a comment line is added reporting this.
-   Comment lines are output if there is no data polygon ID found for a row in the surrogate file.
-   The SurrogateTools Java package was added to the software system, with tools for:
    -   Automatically generating dozens of surrogates
    -   Generating Quality Assurance reports
    -   Normalizing Surrogates
    -   Gapfilling Surrogates
    -   Merging Surrogates

### New Features Added in June/July 2005

The source code for the Spatial Allocator has undergone significant revision. The main executable, mims_spatial.exe, has been divided into separate utilities for surrogate processing and spatial allocation (srgcreate.exe and allocator.exe), and the supporting source files have been reorganized, with many functions now residing in their own modules. All supporting source files that are not associated with external librari
es have been placed in a library called libspatial. These changes allow for better full-life-cycle mainten
ance of the software.

The beld3smk program is new and can be used to create SMOKE biogenic input files.

The diffioapi utility is new and can be used to compare I/O API files.

The dbf2asc utility is new and can be used to convert DBF files to ASCII .csv files.

The environment variables for all modes were renamed to make them more understandable.

For general spatial allocation not involving surrogates, the AGGREGATE and AVERAGE modes have been replaced with an ALLOCATE mode, with a new allocation mode file allowing the user to specify different allocation functions to use for different attributes.

Discrete attributes (e.g. county name) can now be processed as part of ALLOCATE mode using one of two options: maximum overlap or centroid.

General spatial allocation now includes an OVERLAY mode, which allows the user to accept a point, line, or polygon shapefile or point file and overlay a grid, polygon, or bounding box and print the attributes that fall within the overlaying region(s). Output options are shapefile, I/O API file, delimited ASCII file (delimiter can be user-selected), or standard output (stdout).

The MAX_LINE_SEG variable can now be used to specify a maximum length of a line segment. This is useful when gridded data is converted between different map projections - as the precision of the spatial allocation computation is increased over using just four points per grid cell.

Debugging output can now be turned on or off using the DEBUG_OUTPUT environment variable. All processing information except error messages can be suppressed, which allows data results to be clearly displayed in standard output in certain processing modes.

A map projection can now be specified using a grid name.

There is additional support for new input and output file types. New input types are:

-   PointFile
-   I/O API file
-   Polygon file (ASCII text file that lists the coordinates of polygons - used in OVERLAY mode only)

New output types are:

-   Delimited ASCII file with choice of  ',' (comma), '|' (pipe), ';' (semicolon), or  ' ' (space) as delimiter (output from OVERLAY mode)
-   I/O API file

To ease memory overhead, a new variable called MAX_INPUT_FILE_SHAPES can be specified allowing the user to specify a maximum read buffer that will be processed, so that large files may be processed in multiple passes instead of being read in all at once. This feature is currently supported only in OVERLAY mode.

### New Features Added in January 2005

-   Shapefiles can be filtered to produce new shapefiles that contain only the shapes that matched the user-specified criteria.
-   A filter can be applied to a weight file prior to creating a surrogate using the filtered weights.
-   A function of attributes can be specified as the weight to use for computing a surrogate.
-   Existing surrogates can be merged (e.g., using weighted averages) to create new surrogates.
-   Existing surrogates can be gap-filled with more general surrogates so that entries are available for all counties of interest.
-   Surrogates can be created using the Mercator map projection.

### Revisions Made in December 2003

-   Corrected a bug that caused line objects outside the modeling domain but within the counties of the modeling domain to be excluded from the denominator of surrogate computations.
-   Tested the software with stereographic and polar stereographic map projections.
-   Corrected a bug that caused polygon holes to be treated improperly. This often caused problems with water surrogates in coastal areas where holes were commonly required to represent the geography.
-   Implemented a feature to warn the user about counties for which the surrogates do not sum to one and the corresponding grid cells. A sum less than one is legitimate if the county is only partially covered by the grid.
-   Added a date and version number that is printed at the beginning of execution.
