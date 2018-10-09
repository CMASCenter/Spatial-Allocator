[<< Previous Chapter](SA_ch03_vector.md) - [Home](README.md) - [Next Chapter >>](SA_ch05_surrogate.md)
***

# Chapter 4. Raster Tools

<!-- toc -->
## Contents
-  [Introduction](#intro4)
-  [Compiling and Installation](#compiling)
-  [Defining Domains](#domains)
-  [Land Cover Data Processing Tools](#landcover)
	*   [NLCD and MODIS Land Cover Generation](#nlcd_LC)
	*   [NLCD and MODIS Land Cover and MODIS LAI Generation](#nlcd_LC_LAI)
	*   [BELD4 Land Cover Generation](#beld4)
	*   [Current and Future Development fo rthe Land Cover Data Processing Tools](#plans)
-   [Satellite Cloud and Aerosol Product Processing Tools](#satdat)
	*   [GOES Cloud Product Processing Tool](#goes)
	*   [MODIS Level 2 Cloud/Aerosol Products Tool](#modis)
	*   [OMI Level 2 Product Tool](#omi)
	*   [OMI L2G and L3 Product Tools](#omil2g)
-   [Agricultural Fertilizer Modeling Tools](#agtools)
	*   [EPIC Site Information Generation Tool](#epic)
	*   [MCIP/CMAQ-to-EPIC Tool](#toepic)
	*   [EPIC-to-CMAQ Tool](#tocmaq)
	*   [EPIC Yearly Extraction Tool](#epic_yearly)
-   [Other Tools and Utilities](#othertools)
	*   [Domain Grid Shapefile Generation Tool](#shape)
	*   [Other Utilities](#other)
-   [Acknowledgements](#acknowledge4)

---

<a id="intro4"><a/>
## Introduction

The Spatial Allocator (SA) Raster Tools system is designed to process
image or raster spatial data sets in SA. It contains programs to process
various kinds of spatial data for meteorological and air quality
modeling, particularly within the Weather Research and Forecasting (WRF)
(<http://www2.mmm.ucar.edu/wrf/users/>) and Community Multiscale Air
Quality (CMAQ) (<http://www.cmascenter.org/cmaq/>) modeling systems. The
Raster Tools include land cover data processing tools, satellite cloud
and aerosol product processing tools, agricultural fertilizer modeling
tools, a domain grid shapefile generation tool, and other utilities.

All sample script files for the SA Raster Tools are stored in the
raster_scripts directory of the installed Spatial Allocator system.

----
<a id="compiling"><a/>
## Compiling and Installation ##

Users who have difficulties running the tools with the compiled libraries contained within the downloaded Spatial Allocator system should do the following:

-   delete installed open-source library directories under the ./src/libs directory
-   download new source packages and install them under the ./libs directory
-   compile downloaded packages and install them under {package_path}/local, following the src/libs/README file
-   modify paths in ./bin/sa_setup.csh and ./src/raster/Makefile
-   in ./src/raster, do the following:

```
make clean
make
make install
```

-----
<a id="domains"><a/>
## Defining Domains

The SA Raster Tools define the modeling domain using the following
environment variables:

-   `GRID_PROJ` – defines the domain grid projection using the PROJ4
    projection description format, for a full list see:
    (<http://spatialreference.org/>) or on the PROJ4 wiki: (<http://proj4.org/index.html>). The following
    sample projection descriptions are used to match the projections in
    WRF:
    -   Lambert Conformal Conic: `+proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=33 +lat_2=45 +lat_0=40 +lon_0=-97`
    -   Polar stereographic: `+proj=stere +a=6370000.0 +b=6370000.0 +lat_ts=33 +lat_0=90 +lon_0=-97 +k_0=1.0`
    -   Mercator: `+proj=merc +a=6370000.0 +b=6370000.0 +lat_ts=33 +lon_0=0`
    -   Geographic: `+proj=latlong +a=6370000.0 +b=6370000.0`


-   `GRID_ROWS` – number of rows of grid cells in the domain
-   `GRID_COLUMNS` – number of columns of grid cells in the domain
-   `GRID_XCELLSIZE` – grid cell size in *x* direction
-   `GRID_YCELLSIZE` – grid cell size in *y* direction
-   `GRID_XMIN` – minimum *x* of the domain (lower left corner of the
    domain)
-   `GRID_YMIN` – minimum *y* of the domain (lower left corner of the
    domain)
-   `GRID_NAME` – name of the domain, which is required by some of the
    tools

For WRF simulations, GRID_XMIN and GRID_YMIN can be computed using the
first point longitude and latitude from the global attributes
corner_lons and corner_lats in the domain’s WRF GEOGRID output file.
For instance, to compute a WRF Lambert Conformal Conic (LCC) domain with
the GEOGRID output file attributes
```
 :corner_lats = 20.85681f, 52.1644f, 50.63151f, 19.88695f, 20.84302f...
 :corner_lons = -121.4918f, -135.7477f, -53.21942f, -69.02478f, -121.5451f…
```
use the cs2cs utility in the PROJ4 library directly at the command line (after installing the SA system):

```
cs2cs +proj=latlong +a=6370000.0 +b=6370000.0 +to +proj=lcc +a=6370000.0 +b=6370000.0 +lat_1=33 +lat_2=45 +lat_0=40 +lon_0=-97 -121.4918 20.85681 -2622003.85 -1793999.28 0.00
```

Minimum *x* and *y* for the domain would be computed as follows:

```
GRID_XMIN = -2622003.85 - GRID_XCELLSIZE / 2
GRID_YMIN = -1793999.28 - GRID_YCELLSIZE / 2
```

---
<a id="landcover"><a/>
## Land Cover Data Processing Tools

There are three land cover processing tools in the SA Raster Tools:

-   [NLCD and MODIS land cover generation tool](#nlcd_LC)
-   [NLCD and MODIS land cover and MODIS LAI generation tool](#nlcd_LC_LAI)
-   [Biogenic Emissions Landcover Database, version 4 (BELD4) land cover generation tool.](#beld4)

All of the example scripts listed in this section are in the SA_HOME/raster_scripts directory.

<a id="nlcd_LC"><a/>
### NLCD and MODIS Land Cover Generation

The computeGridLandUse.exe tool is used to generate land cover data for
the upgraded WRF/CMAQ Pleim-Xiu Land Surface Model (PX LSM) in the
current WRF model release, by directly using downloaded 2001, 2006, or
2011 National Land Cover Data (NLCD) land cover data, the NASA
Moderate Resolution Imaging Spectroradiometer (MODIS) land cover
products MCD12Q1 or MOD12Q1, and NASA MODIS LAI/FPAR products (e.g. MCD15A2H, MOD15A2, MOD15A2GFS).
This tool generates 40 land cover classes
(20 from MODIS and 20 from NLCD), and MODIS LAI/FPAR data for each land cover type and whole grid area.

This tool requires the following data sets:

-   NLCD land cover, canopy and imperviousness data:
    -   SA Scripts: NLCD_MODIS_processor.csh and landuseTool_WRFCMAQ_BELD4.csh
    -   URL: http://www.mrlc.gov/nlcd2006.php
    -   Instructions: Download NLCD 2006 Land Cover (2011 Edition)

-   MODIS tiled land cover data: MCD12Q1
    -   SA Scripts: NLCD_MODIS_processor.csh and landuseTool_WRFCMAQ_BELD4.csh
    -   URL: https://ladsweb.nascom.nasa.gov/data/search.html
    -   Instructions:  Select the following from the download page:
        -   Combined Terra & Aqua MODIS
        -   Combined Land Level 3/Level4 yearly Tiled Products
        -   MCD12Q1 - MODIS/Terra+Aqua Land Cover Type Yearly L3 Global 500m SIN Grid
        -   Temporal Type: Date and Time Range
        -    Set time period for downloading
        -   Collection: 51 - MODIS Collection 5.1
        -   Coordinate System: Latitude/Longitude
        -   Type in extents in degree or use a predefined region
        -   Search
        -   View All
        -   Order Files Now

- List of land cover data sets to be processed – this file has to have fixed header formats.  Provided in the data directory are sample files for CMAQ 12-km domain 2001, 2006 and 2011 modeling: 

```
nlcd_modis_files_2001.txt
nlcd_modis_files_2006.txt
nlcd_modis_files_2011.txt.  
```

Users have to modify the list file based on their NLCD and MODIS data location and names.

To run the computeGridLandUse tool, users can use the following script file, which has all of the required environment variables:

**NLCD_MODIS_processor.csh**

The tool generates one ASCII file and one NetCDF file:

-   The ASCII file contains the imperviousness, canopy, and land cover percent variables (if the user set all land cover data to “YES” when running the script file) for each grid cell, in comma-separated-values (CSV) format.
-   The NetCDF file contains imperviousness, canopy, and land cover fraction variables plus land/water mask and other variables that are similar to those in the WRF GEOGRID land cover output files. The land cover percentage variable contains the 40 classes in [Table 1](#Table-1).

<a id=Table-1></a>
**Table 1. NLCD/MODIS output land cover classes from the
computeGridLandUse tool.**

|**Array Index**|**MODIS Class IGBP (Type 1)**|**Class Name**|**Array Index**|**NLCD Class**|**Class Name**|
|---|---|---|---|---|---|
|1|1|Evergreen Needleleaf forest|21| 11|Open Water
|2|2|Evergreen Broadleaf forest | 22 | 12  | Perennial Ice/Snow|
|3|3|Deciduous Needleleaf forest | 23 |           21|Developed - Open Space|
|4 |4 |Deciduous Broadleaf forest | 24 |22 |Developed - Low Intensity|
|5|5 | Mixed forest| 25|23|Developed - Medium Intensity|
|6|6|Closed shrublands| 26| 24| Developed High Intensity|
|7|7 |Open shrublands| 27|31|Barren Land (Rock/Sand/Clay)|
|8|8 |Woody savannas| 28|41|Deciduous Forest
|9|9|Savannas| 29|42|Evergreen Forest
|10|10| Grasslands| 30 |43| Mixed Forest|
|11|11|Permanent wetlands|31|51|Dwarf Scrub
|12|12| Croplands| 32|52|Shrub/Scrub|
|13|13 |Urban and built-up|33|71| Grassland/Herbaceous|
|14|14 |Cropland/Natural vegetation mosaic |  34|72 |Sedge/Herbaceous|
|15 |15|Snow and ice| 35|73|Lichens|
|16 |16|Barren or sparsely vegetated |        36|74|Moss|
|17 |0| Water | 37 |81|Pasture/Hay|
|18|18|Reserved (e.g., Unclassified)|  38| 82 |Cultivated Crops|
|19|19 |Reserved (e.g., Fill Value ) |  39 |90|Woody Wetlands|
|20|20| Reserved| 40 | 95|Emergent Herbaceous Wetlands|


<a id="nlcd_LC_LAI"><a/>
### NLCD and MODIS Land Cover and MODIS LAI Generation


The computeGridLandUse\_LAI\_MODIS.exe tool is used to generate land
cover data for the WRF/CMAQ Pleim-Xiu Land Surface Model (PX LSM) in the
current WRF model release, by directly using downloaded 2001, 2006, or
2011 National Land Cover Data (NLCD) land cover data, the NASA Moderate
Resolution Imaging Spectroradiometer (MODIS) land cover products MCD12Q1
or MOD12Q1, and NASA MODIS LAI/FPAR products (e.g. MCD15A2H, MOD15A2,
MOD15A2GFS). This tool generates 40 land cover classes (20 from MODIS
and 20 from NLCD) and MODIS LAI/FPAR data for each land cover type and
whole grid area.

This tool requires the following data sets:

  - NLCD land cover, canopy, and imperviousness data – can be obtained
    from <https://www.mrlc.gov/>.

  - MODIS land cover data sets – can be obtained through
    <https://lpdaac.usgs.gov/dataset_discovery/modis/modis_products_table/mcd12q1>.
    The tool can process MCD12Q1 data at 500 m from Combined Terra and
    Aqua MODIS, or can process MOD12Q1 data at 1 km from Terra MODIS.

  - MODIS LAI/FPAR products – can be obtained through
    <https://lpdaac.usgs.gov/dataset_discovery/modis/modis_products_table/mod15a2h_v006.>The
    tool can process any of MCD15A2H, MOD15A2, and MOD15A2GFS MODIS
    vegetation products.

    List of land cover data sets to be processed – this file has to have fixed header formats. Provided in the data directory are sample files for CMAQ 12-km domain 2001, 2006 and 2011 modeling: nlcd\_modis\_files\_2001.txt, nlcd\_modis\_files\_2006.txt, and nlcd\_modis\_files\_2011.txt. Users have to modify the list file based on their NLCD and MODIS data location and names.

To run the computeGridLandUse\_LAI\_MODIS tool, users can use the
following sample script file, which has all of the required environment
variables:

**MODIS\_landcover\_LAI\_data\_processor.csh**

The tool generates one ASCII file and one NetCDF file:

  - The ASCII file contains the imperviousness, canopy, and land cover percent variables (if the user set all land cover data to “YES” when running the script file) for each grid cell, in comma-separated-values (CSV) format.

  The NetCDF file contains imperviousness, canopy, and land cover fraction variables plus land/water mask and other variables that are similar to those in the WRF GEOGRID land cover output files. The land cover percentage variable contains the 40 classes as listed in Table 1. In addition, MODIS LAI and FPAR variables for each landcover type and average at each grid cell are included in the NetCDF file.

<a id="beld4"><a/>
### BELD4 Land Cover Generation

The BELD4 data with land cover, tree, and crop percentages can be
computed using the computeGridLandUse_beld4.exe tool with directly
downloaded USGS NLCD data sets, NASA MODIS land cover (MCD12Q1 or
MOD12Q1) data tiles and tree and crop fractions at the county level. The
following sample script file contains all of the required environment
variables for running the tool: 

**landuseTool_WRFCMAQ_BELD4.csh**.

This tool requires the following data sets:

-   Downloaded USGS NLCD data sets, including land cover,
    imperviousness, and canopy, can be obtained from the NLCD web site:
    <http://www.mrlc.gov/nlcd2006.php>.
-   MODIS land cover tiles (MCD12Q1 or MOD12Q1) – can be obtained from
    the NASA MODIS land products web site:
    <http://modis-land.gsfc.nasa.gov/landcover.html>.
-   List of land cover data sets to be processed – this file has to be
    fixed format with the data set headers included. Provided in the
    data directory are sample files for CMAQ 12-km domain 2001, 2006 and
    2011 modeling: nlcd_modis_files_2001.txt,
    nlcd_modis_files_2006.txt, and nlcd_modis_files_2011.txt.
    Users have to modify the list file based on their NLCD and MODIS
    data location and names.
-   BELD3 FIA tree fraction table at county level – provided in the data
    directory: beld3-fia.dat.
-   National Agricultural Statistics Service (NASS) crop fraction tables
    at county level – provided in the data directory:
    nass2001_beld4_ag.dat for the 2001 NASS; nass2006_beld4_ag.dat
    for the 2006 NASS.
-   Canada crop fraction table at Census-division level – provided in
    the data directory: can01_beld4_ag.dat for the 2001 Census of
    Agriculture; can06_beld4_ag.dat for the 2006 Census of
    Agriculture.
-   List of land cover, tree, and crop classes for the BELD4 tool –
    provided in the data direc­tory: beld4_class_names_40classes.txt.
-   U.S. county shapefile – provided in the data directory:
    county_pophu02_48st.shp.
-   Canada Census-division shapefiles – provided in the data directory:
    can2001_cd_sel.shp for the 2001 Census; can2006_cd_sel.shp for
    the 2006 Census.

The tool generates one ASCII file and one NetCDF file:

-   The ASCII file contains the imperviousness, canopy, and land cover
    fraction variables (if the user set all land cover data to “YES”
    when running the script file) for each grid cell, in CSV format.
-   The NetCDF file contains imperviousness, canopy, land cover, tree,
    and crop percentage variables as well as land/water mask and other
    variables that are similar to those in the WRF GEOGRID land cover
    output files.

The land cover data generated by applying this tool are used in CMAQ
bidirectional ammonia flux modeling and are used in CMAQ biogenic, land
surface, and dry deposition modeling. The land cover percentage array in
the output contains 20 NLCD land cover classes and 20 MODIS IGBP land
cover classes (see [Table 1](#Table-1)). The tree percentage variable in the NetCDF
output file contains the 194 BELD4 tree classes shown in [Table 2](#Table-2), and
the crop percentage variable contains the 42 crops listed in [Table 3](#Table-3).

<a id=Table-2></a>
**Table 2. BELD4 tree classes.**

|**Index**|**Variable**|**Index**|**Variable**|**Index**|**Variable**|**Index**|**Variable**|
|---|---|---|---|---|---|---|---|
|1|Acacia|40|Hackberry|79|Oak_bur|118| Paulownia|157|Pine_whitebark|
|2|Ailanthus|41|Hawthorn|80|Oak_CA_black|119|Pawpaw|158|Pine_white|
|3|Alder|42|Hemlock|81|Oak_CA_live|120|Persimmon|159|Pine_yellow|
|4|Apple|43|Hickory|82|Oak_CA_white|121|Pine_Apache|160|Populus|
|5|Ash|44|Holly_American|83|Oak_canyon_live|122|Pine_Austrian|161|Prunus|
|6|Basswood|45|Hornbeam|84|Oak_chestnut|123|Pine_AZ|162|Redbay|
|7|Beech|46|Incense_cedar|85|Oak_chinkapin|124|Pine_Bishop|163|Robinia_locust|
|8|Birch|47|Juniper|86|Oak_delta_post|125|Pine_blackjack|164|Sassafras|
|9|Bumelia_gum|48|KY_coffeetree|87|Oak_Durand|126|Pine_brstlcone|165|Sequoia|
|10|Cajeput|49|Larch|88|Oak_Emery|127|Pine_chihuahua|166|Serviceberry|
|11|Califor-laurel|50|Loblolly_bay|89 |Oak_Engelmann|128|Pine_Coulter|167|Silverbell|
|12|Cascara-buckthor|51|Madrone|90|Oak_evergreen_sp|129|Pine_digger|168|Smoketree|
|13|Castanea|52|Magnolia|91|Oak_Gambel|130|Pine_Ewhite|169|Soapberry_westrn|
|14|Catalpa|53|Mahogany|92|Oak_interio_live|131|Pine_foxtail|170|Sourwood|
|15|Cedar_chamaecyp|54|Maple_bigleaf|93|Oak_laurel|132|Pine_jack|171|Sparkleberry|
|16|Cedar_thuja|55|Maple_bigtooth|94|Oak_live|133|Pine_Jeffrey|172|Spruce_black|
|17|Chestnut_buckeye|56|Maple_black|95|Oak_Mexicanblue|134|Pine_knobcone|173|Spruce_blue|
|18|Chinaberry|57|Maple_boxelder|96|Oak_Northrn_pin|135|Pine_limber|174|Spruce_Brewer|
|19|Cypress_cupress|58|Maple_FL|97|Oak_Northrn_red|136|Pine_loblolly|175|Spruce_Englemann|
|20|Cypress_taxodium|59|Maple_mtn|98|Oak_nuttall|137|Pine_lodgepole|176|Spruce_Norway
|21|Dogwood|60|Maple_Norway|99|Oak_OR_white|138|Pine_longleaf|177|Spruce_red|
|22|Douglas_fir|61|Maple_red|100|Oak_overcup|139|Pine_Monterey|178|Spruce_Sitka|
|23|East_hophornbean|62|Maple_RkyMtn|101|Oak_pin|140|Pine_pinyon|179|Spruce_spp|
|24|Elder|63|Maple_silver|102|Oak_post|141|Pine_pinyon_brdr|180|Spruce_white|
|25|Elm|64|Maple_spp|103|Oak_scarlet|142|Pine_pinyon_cmn|181|Sweetgum|
|26|Eucalyptus|65|Maple_striped|104|Oak_scrub|143|Pine_pitch|182|Sycamore|
|27|Fir_balsam|66|Maple_sugar|105|Oak_shingle|144|Pine_pond|183|Tallowtree-chins|
|28|Fir_CA_red|67|Mesquite|106|Oak_Shumrd_red|145|Pine_ponderosa|184|Tamarix|
|29|Fir_corkbark|68|Misc-hardwoods|107|Oak_silverleaf|146|Pine_red|185|Tanoak|
|30|Fir_fraser|69|Mixed_conifer_sp|108|Oak_Southrn_red|147|Pine_sand|186|Torreya|
|31|Fir_grand|70|Mountain_ash|109|Oak_spp|148|Pine_scotch|187|Tung-oil-tree|
|32|Fir_noble|71|Mulberry|110|Oak_swamp_cnut|149|Pine_shortleaf|188|Unknown_tree|
|33|Fir_Pacf_silver|72|Nyssa|111|Oak_swamp_red|150|Pine_slash|189|Walnut|
|34|Fir_SantaLucia|73|Oak_AZ_white|112|Oak_swamp_white|151|Pine_spruce|190|Water-elm|
|35|Fir_Shasta_red|74|Oak_bear|113|Oak_turkey|152|Pine_sugar|191|Willow|
|36|Fir_spp|75|Oak_black|114|Oak_water|153|Pine_Swwhite|192|Yellow_poplar|
|37|Fir_subalpine|76|Oak_blackjack|115|Oak_white|154|Pine_tablemtn|193|Yellowwood|
|38|Fir_white|77|Oak_blue|116|Oak_willow|155|Pine_VA|194|Yucca_Mojave
|39|Gleditsia_locust|78|Oak_bluejack|117|Osage-orange|156|Pine_Washoe|                  

<a id=Table-3></a>
**Table 3. BELD4 crop classes.**

|**Index**|**Variable**|**Index**|**Variable**|**Index**|**Variable**|
|---|---|---|---|---|---|
|1|Hay|15|Cotton|29|SorghumSilage|
|2|Hay_ir|16|Cotton_ir|30|SorghumSilage_ir|
|3|Alfalfa|17|Oats|31|Soybeans|
|4|Alfalfa_ir|18|Oats_ir|32|Soybeans_ir|
|5|Other_Grass|19|Peanuts|33|Wheat_Spring|
|6|Other_Grass_ir|20|Peanuts_ir|34|Wheat_Spring_ir|
|7|Barley|21|Potatoes|35|Wheat_Winter|
|8|Barley_ir|22|Potatoes_ir|36|Wheat_Winter_ir|
|9|BeansEdible|23|Rice|37|Other_Crop|
|10|BeansEdible_ir|24|Rice_ir|38|Other_Crop_ir|
|11|CornGrain|25|Rye|39|Canola|
|12|CornGrain_ir|26|Rye_ir|40|Canola_ir|
|13|CornSilage|27|SorghumGrain|41|Beans|
|14|CornSilage_ir|28|SorghumGrain_ir|42|Beans_ir|


<a id="plans"><a/>
### Current and Future Development for the Land Cover Data Processing Tools

-   Enhance the tool to use the released NLCD 2011 data sets with
created 2011 crop tables for both US and Canada.
-   Use USDA’s NLCD Cropland Data Layer (CDL) data instead
of NASS crop fractions at the county level for the BELD4 data tool. This
will support the use of USDA crop spatial coverage NLCD data instead of
county-based crop census data in computing crop fractions within each
grid cell.

----
<a id="satdat"><a/>
## Satellite Cloud and Aerosol Product Processing Tools

<a id="goes"><a/>
### GOES Cloud Product Processing Tool

The GOES data tool processes the Geostationary Operational Environmental Satellite (GOES) 
data downloaded from the Earth System Science Center (ESSC) at the University of Alabama in Huntsville. 
The GEOS data web site is <http://satdas.nsstc.nasa.gov>

Downloaded GOES data need to be stored under subdirectories named using
this format: gp_YYYYMMDD. The ./util/goes_untar.pl utility can be used
to unzip downloaded GOES data (daily tar files) into the daily
directories required by the tool.

The following sample script file contains all of the required
environment variables for running the tool: 

**allocateGOES2WRFGrids.csh**.

The tool contains the following three programs:

-   `correctGOESHeader.exe` – to correct GOES data position shifting by
    redefining a new Earth radius and new image extent. The program
    converts GOES data in Grib (i.e., .grb) format to files in ERDAS
    Imagine (i.e., .img) format with corrections.
-   `computeGridGOES.exe` – to regrid corrected Imagine-format GOES data
    to a defined grid domain.
-   `toDataAssimilationFMT.exe` – to convert the gridded NetCDF file into
    a format suitable for WRF assimilation.

The released GOES data has changed to ASCII format from GRIB format last
year. We plan to update the tool in the coming months.

When running the GOES cloud product processing tool, the
Geospatial Data Abstraction Library (GDAL) will generate the following
messages:

```
Warning: Inside GRIB2Inventory, Message # 2
ERROR: Ran out of file reading SECT0
```

These messages do not indicate any errors in regridding and so can be
ignored.

<a id="modis"><a/>
### MODIS Level 2 Cloud/Aerosol Products Tool

The MODIS Level 2 (swath) cloud and aerosol products tool processes
MODIS L2 cloud or aerosol products for a defined grid domain. MODIS data
in HDF4 format can be downloaded from the NASA Level 1 and Atmosphere
Archive and Distribution System (LAADS) web site:
<http://ladsweb.nascom.nasa.gov/data/search.html>.

MODIS cloud product variables contain 5-km and 1-km data. To use this
regridding tool, users need to download the following cloud data and
Level 1 Geolocation 1-km data into the input directory:

-   MOD06_L2 and MOD03 (Level 1 Geolocation 1-km ) for Terra, or
-   MYD06_L2 and MYD03 (Level 1 Geolocation 1-km ) for Aqua

The following download options can be selected during the download process:

MODIS Cloud:
- Select Level 2 products and select L2 Cloud products
- Select time: "your download time period"
- Collection 5
- Select Latitude/Longitude with the above geographic extent
- Coverage options: select day, night, and both (all)
- Select all other defaults and click search
- Display all files
- Download files into one directory

MODIS Geolocation 1-km:
- Select Level 1 products and select O3 Geolocation - 1km
- Select time: "same as cloud products"
- Collection 5
- Select Latitude/Longitude with the above geographic extent
- Coverage options: select day, night,and both (all)
- Display all files
- Download all files into the MODIS Cloud file directory

MODIS aerosol products contain variable data at 10-km resolution
(nadir). Users need to download MOD04 for Terra or MYD04 for Aqua into
the input data directory. The download options below can be selected
when downloading Terra aerosol products. Downloading Aqua aerosol
products involves similar options. The tool generates one NetCDF file
for the defined domain.

#### Aerosol Product Data Access:
-   Select Terra MODIS
-   MODIS Aerosol products
-   Select Level 2 products and select L2 aerosol poduct
-   Select time: "your download time period"
-   Collection: 5
-   Select Latitude/Longitude with area longitude and latitude extent
-   Coverage options: select day, night, and both (all)
-   Select all other defaults and click search
-   Display all files
-   Download all files into one directory

Users can modify the following sample script file provided for regridding the MODIS cloud data:

**allocateMODISL2CloudVars2Grids.csh**


<a id="omi"><a/>
### OMI Level 2 Product Tool

The OMI Level 2 product (swath) tool is used to regrid Ozone Monitoring
Instrument (OMI) L2 aerosol and NO~2~ products for a defined grid
domain. The input data can be downloaded from the NASA mirador site: 

[http://mirador.gsfc.nasa.gov/cgi-bin/mirador/presentNavigation.pl?tree=
project\&project=OMI](http://mirador.gsfc.nasa.gov/cgi-bin/mirador/presentNavigation.pl?tree=project&project=OMI).

The downloaded data are in HDF5 format and should be stored in one
directory, which is defined in the following sample script file:

**allocateOMIL2vars2Grids.csh**

<a id="omil2g"><a/>
### OMI L2G and L3 Product Tools

The OMI L2G and L3 product tools process the following OMI products:

-   OMI L3 aerosol products (OMAEROe) in HDF4
-   OMI NO2 L2G products (OMNO2G) in HDF4
-   OMI NO2 L3 products (NO2TropCS30) in HDF5

The data can be downloaded from the NASA Giovanni web site:
<http://gdata1.sci.gsfc.nasa.gov/daac-bin/G3/gui.cgi?instance_id=omi>

OMI product information can be viewed from
[http://disc.sci.gsfc.nasa.gov/giovanni/additional/
users-manual/G3\_manual\_Chapter\_10\_OMIL2G.shtml\#what\_l2g](http://disc.sci.gsfc.nasa.gov/giovanni/additional/users-manual/G3_manual_Chapter_10_OMIL2G.shtml#what_l2g)
and from [ftp://aurapar2u.ecs
.nasa.gov/data/s4pa//Aura\_OMI\_Level2/OMAERUV.003/doc/README.OMI\_DUG.pdf](ftp://aurapar2u.ecs.nasa.gov/data/s4pa/Aura_OMI_Level2/OMAERUV.003/doc/README.OMI_DUG.pdf)

The following sample script can be modified for regridding:

**allocateOMIvar2Grids.csh**

---
<a id="agtools"><a/>
## Agricultural Fertilizer Modeling Tools

There are four tools that can be used when performing Environmental
Policy Integrated Climate (EPIC) modeling; they generate gridded
agricultural fertilizer data to be used in CMAQ bidirectional NH~3~ flux
modeling. These tools are the [EPIC site information generation tool](#epic), the
[MCIP/CMAQ-to-EPIC tool](#toepic), the [EPIC-to-CMAQ tool](#tocmaq), and the EPIC yearly
extraction tool](#epic_yearly). They can be called from the
Fertilizer Emission Scenario Tool for CMAQ (FEST-C) interface
(<http://www.cmascenter.org/fest-c/>) based on user input information,
and can be run by script files with defined environment variables at the
command line.

<a id="epic"><a/>
### EPIC Site Information Generation Tool

This tool generates three CSV data files that are needed to create EPIC
site databases for a user-defined domain:

-   EPICSites_Info.csv – contains GRIDID, XLONG, YLAT, ELEVATION,
    SLOPE_P, HUC8, REG10, STFIPS, CNTYFIPS, GRASS, CROPS, CROP_P,
    COUNTRY, and COUNTRY-PROVINCE items.
-   EPICSites_Crop.csv – contains GRIDID, 42 crop acreages, COUNTRY,
    and HUC8 items.
-   allSites_Crop.csv - contains GRIDID, 42 crop acreages, COUNTRY, and HUC8 items for all grid cells.

The tool processes the set of input spatial data files below, which have
been modified specifically for use with the tool and can be obtained
from the CMAS:

-   BELD4 file for the domain (beld4_cmaq12km_2006.nc)
-   U.S. county shapefiles (co99_d00_conus_cmaq_epic.shp)
-   North American State political boundary shapefile
    (na_bnd_camq_epic.shp)
-   U.S. 8-digit HUC shapefile (conus_hucs_8_cmaq.shp)
-   Elevation image file (na_dem_epic.img)
-   Slope image file (na_slope_epic.img)

Users can follow the sample script file below, which has all of the
environment variables required for running the tool from the command
line:

**generateEPICSiteData.csh**

<a id="toepic"><a/>
### MCIP/CMAQ-to-EPIC Tool

This tool generates EPIC daily weather and nitrogen deposition data files from MCIP meteorology 
and CMAQ nitrogen deposition files for EPIC modeling sites. The input MCIP and CMAQ data are stored in two
directories defined by the environment variables DATA_DIR and DATA_DIR_CMAQ.

MCIP output files must have names of the format METCRO2D*{date} (e.g.,
METCRO2D_020725). The date format can be in one of the following
formats:

`YYYYMMDD *or* YYMMDD *or* YYYYDDD *or* YYDDD`

CMAQ dry and wet deposition files must have names of the format
\*DRYDEP\*{date} and \*WETDEP\*{date} (e.g.,
CCTM_N4a_06emisv2soa_12km_wrf.DRYDEP.20020630 and
CCTM_N4a_06emisv2soa_12km_wrf.WETDEP1.20020630). The date can be in
any of the formats listed above.

Deposition inputs for EPIC modeling can take one of the following three
inputs:

-   Directory containing a CMAQ dry and wet deposition file
-   Zero – assume zero nitrogen deposition
-   Default – assume nitrogen mix ratio of 0.8 ppm for wet default deposition computation

The input site location file defined by the environment variable
*EPIC_SITE_FILE* has to be a CSV file, with the first three items being
site ID, longitude, and latitude. 

allSites_Crop.csv generated by the EPIC Site Information Generation Tool described above is used as EPIC_SITE_FILE for FEST-C applications.

The tool generates three outputs:

-   dailyWETH directory containing EPIC daily weather and nitrogen
    deposition files with names of the format “grid ID”.dly (e.g.,
    96.dly). The daily file contains the 14 variables listed in [Table 4](#Table-4).
-   NetCDF file with daily weather and nitrogen deposition data for all
    grid cells.
-   EPICW2YR.2YR, to be used for daily weather file input list in EPIC
    modeling.

<a id=Table-4></a>
**Table 4. EPIC daily weather and nitrogen deposition variables.**

|**Index**|**Variable**|**Index**|**Variable**|
|---|---|---|---|
|1|Year|8|Daily Average Relative Humidity|
|2|Month|9|Daily Average 10m Windspeed (m s^-1)|
|3|Day|10|Daily Total Wet Oxidized N (g/ha)|
|4|Daily Total Radiation (MJ m^02)|11|Daily Total Wet Reduced N (g/ha)|
|5|Daily Maximum 2m Temperature (C)|12|Daily Total Dry Oxidized N (g/ha)|
|6|Daily minimum 2m temperature (C)|13|Daily Total Dry Reduced N (g/ha)|
|7|Daily Total Precipitation (mm)|14|Daily Total Wet Organic N (g/ha)|

Users can follow the sample script file below, which has all of the
environment variables required for running the tool from the command
line window:

**generateEPICsiteDailyWeatherNdep.csh**

The following are two versions of the tool **computeSiteDailyWeather.cpp**

**computeSiteDailyWeather.cpp_beforecmaq52**

**computeSiteDailyWeather.cpp_cmaq52**

The default version is linked to the version: **computeSiteDailyWeatehr.cpp_cmaq52**. 

Users should change the link to the **computeSiteDailyWeather.cpp_beforecmaq52** if a version of CMAQ prior to CMAQv5.2 was used to generate the N deposition input files.

<a id="toCMAQ"><a/>
### EPIC-to-CMAQ Tool

This tool processes merged daily output from EPIC simulations for the 42
crops defined for the BELD4 tool output. It generates two types of
outputs in NetCDF format for CMAQ bidirectional NH~3~ modeling:

-   soil output file
-   EPIC daily output files

The 13 variables contained in the soil output file are listed in [Table 5](#Table-5).

<a id=Table-5></a>
**Table 5. EPIC-to-CMAQ soil output variables.**

|**Index**|**Name**|**Soil Variable**|**Index**|**Name**|**Soil Variable**|
|---|---|---|---|---|---|
|1|L1_SoilNum|Soil Number (none)|8|L2_Bulk_D|Layer2 Bulk Density (t/m**3)|
|2|L1_Bulk_D|Layer1 Bulk Density (t/m**3)|9|L2_Wilt_P|Layer2 Wilting Point (m/m)|
|3|L1_Wilt_P|Layer1 Wilting Point(m/m)|10 |     L2_Field_C|Layer2 Field Capacity (m/m)|
|4|L1_Field_C|Layer1 Field Capacity (m/m)|11|L2_Porocity|Layer2 Porocity (%)|
|5|L1_Porocity|Layer1 Porocity (%)|12|L2_PH|Layer2 PH (none)|
|6|L1_PH|Layer1 PH (none)|13|L2_Cation|Layer2 Cation Ex (cmol/kg)|
|7|L1_Cation|Layer1 Cation Ex (cmol/kg)|                       

EPIC daily output files for CMAQ contain the 41 variables listed in [Table 6](#Table-6).

The following sample script file with all of the required environment variables can be modified and run at the command line:

**epic2CMAQ.csh**


<a id=Table-6></a>
**Table 6. EPIC for CMAQ daily output variables.**

|**Index**|**Name**|**Variable**|**Index**|**Name**|**Variable**|
|---|---|---|---|---|---|
|1|DN|N-NO3 Denitrification (kg/ha)|22|L2_NH3|Layer2 Ammonia (kg/ha)|
|2|DN2*|N-N2O from NO3 Denitrification (kg/ha)|23|L2_ON|Layer2 Organic N (kg/ha)|
|3|HMN|OC Change by Soil Respiration (kg/ha)|24|L2_C |     Layer2 Carbon (kg/ha)|
|4|NFIX|N Fixation (kg/ha)|25|L2_NITR|Layer2 N - Nitrified NH3 (kg/ha)|
|5|GMN|N Mineralized (kg/ha)|26|T1_DEP|Layer Depth (m)|
|6|YW|Wind Erosion (ton/ha)|27|T1_BD|Layer Bulk Density (t/m**3)|
|7|FPO|Organic P Fertilizer (kg/ha)|28|T1_NO3|Layer N - Nitrate (kg/ha)|
|8|FPL|Labile P Fertilizer (kg/ha)|29|T1_NH3|Layer N - Ammonia (kg/ha)|
|9|MNP|P Mineralized (kg/ha)|30|T1_ON|Layer Organic N (kg/ha)|
|10|L1_DEP|Layer1 Depth (m)|31|T1_C|Layer Mineral C (kg/ha)|
|18|L1_BD|Layer1 Bulk Density (t/m**3)|32|T1_NITR|Layer N - Nitrified NH3 (kg/ha)
|12|L1_SW|Layer1 Soil Moisture (mm)|33|T1_ANO3|Layer1 N-NH3 AppRate (kg/ha)|
|13|L1_NO3|Layer1 N - Nitrate (kg/ha)|34|T1_ANH3|Layer1 N-NH3 AppRate (kg/ha)|
|14|L1_NH3|Layer1 N - Ammonia (kg/ha)|35|T1_AON|Layer1 ON AppRate (kg/ha)|
|15|L1_ON|Layer1 Organic N (kg/ha)|36|L2_ANH3|Layer2 N-NO3 AppRate (kg/ha)|
|16|L1_C|Layer1 Carbon (kg/ha)|37|L2_ANH3|Layer2 N-NH3 AppRate (kg/ha)|
|17|L1_NITR|Layer1 N - Nitrified NH3 (kg/ha)|38|L2_AON|Layer2 ON AppRate (kg/ha)|
|18|L2_DEP|Layer1 Depth (m)|39|LAI|Leaf Area Index (none)|
|19|L2_BD|Layer2 Bulk Density (t/m**3)|40|CPHT|Crop Height(m)|
|20|L2_SW|Layer2 Soil Moisture (mm)|41|FBARE|Bare Land Fraction for Wind Erosion (Fraction)
|21|L2_NO3|Layer2 N - Nitrate (kg/ha)|

Note: EPIC is a daily timestep model while the CMAQ bidirectional NH3 flux model is at a time scale which could be less than 10 minutes.

<a id="epic-yearly"><a/>
### EPIC Yearly Extraction Tool

This tool is used primarily to provide data for performing quality
assurance (QA) for EPIC runs.

-   For EPIC spin-up runs, it extracts average EPIC values from the last
    five years of the spin-up simulations.
-   For EPIC application runs, it extracts application-year EPIC
    variables.

In both cases, the tool outputs one crop-specific NetCDF file with 48
variables and one crop-weighted NetCDF file with 39 variables; [Table 7](#Table-7)
shows the two lists of variables.

<a id=Table-7></a>
**Table 7. EPIC yearly extraction output variables.**

epic2cmaqyear.nc - crop specific output

|**Index**|**Name**|**Variable**|**Index**|**Name**|**Variable**|
|---|---|---|---|---|---|
|1|GMN|N Mineralized (kg/ha)|25|FTP|P Applied (kg/ha)|
|2|NMN|Humus Mineralization (kg/ha)|26|IRGA*|Irrigation Volume Applied (mm)|
|3|NFIX|N Fixation (kg/ha)|27|WS|Water Stress Days (days)|
|4|NITR|N - Nitrified NH3 (kg/ha)|28|NS|N Stress Days (days)|
|5|AVOL|N - Volatilization (kg/ha)|29|IPLD|Planting Date (Julian Date)|
|6|DN|N-NO3 Denitrification (kg/ha)|30|IGMD|Germination Date (Julian Date)|
|7|YON|N Loss with Sediment (kg/ha)|31|IHVD|Harvest Date (Julian Date)|
|8|QNO3|N Loss in Surface Runoff (kg/ha)|32|YP|P Loss with Sediment (kg/ha)|
|9|SSFN|N in Subsurface Flow (kg/ha)|33|QAP|Labile P Loss in Runoff (kg/ha)|
|10|PRKN|N Loss in Percolate (kg/ha)|34|YW|Wind Erosion (ton/ha)
|11|FNO|N - Organic Fertilizer (kg/ha)|35|Q*|Runoff (mm)|
|12|FNO3|N - Nitrate Fertilize (kg/ha)|36|SSF|Subsurface flow (mm)|
|13|FNH3|N - Ammonia Fertilize (kg/ha)|37|PRK|Percolation (mm)|
|14|OCPD|Organic Carbon in Plow Layer (mt/ha)|38|PRCP|Rainfall (mm)|
|15|TOC|Organic Carbon in Soil Profile (mt/ha)|39|PET|Potential Evapotranspiration (mm)|
|16|TNO3|Total NO3 in Soil Profile (kg/ha)|40|ET|Evapotranspiration (mm)|
|17|DN2|N-N2O from NO3 Denitrification (kg/ha)|41|QDRN|Drain Tile Flow (mm)|
|18|YLDG|Grain Yield (t/ha)|42|MUSL|Water erosion (ton/ha)|
|19|T_YLDG|T-Grain Yield (1000ton)|43|DRNN|Nitrogen in drain tile flow (kg/ha)|
|20|YLDF|Forage Yield (t/ha)|44|DRNP|P in Drain Tile Flow (kg/ha)|
|21|T_YLDF|T-Forage Yield (1000ton)|45|PRKP|P in Percolation (kg/ha)|
|22|YLN|N Used by Crop (kg/ha)|46|FPO|Organic P Fertilizer (kg/ha)|
|23|YLP|P Used by Crop (kg/ha)|47|FPL|Labile P Fertilizer (kg/ha)|
|24|FTN|N Applied (kg/ha)|48|MPN|P Mineralized (kg/ha)

**epic2cmaq_year_total.nc** - crop weighted output

|**Index**|**Name**|**Variable**|**Index**|**Name**|**Variable**|
|---|---|---|---|---|---|
|1|T_GMN|N Mineralized (mt - metric ton)|21|T_FTP|P Applied (mt)|
|2|T_NMN|Humus Mineralization (mt)|22|T_IRGA*|Irrigation Volume Applied (mm)|
|3|T_NFIX|N Fixation (mt)|23|T_YP|T -P Loss with Sediment (mt)|
|4|T_NITR|N - Nitrified NH3 (mt)|24|T_QAP|T -Labile P Loss in Runoff (mt)|
|5|T_AVOL|N - Volatilization (mt)|25|T_YW|T -Wind Erosion (1000ton)|
|6|T_DN|N-NO3 Denitrification (mt)|26|T_Q*|T -Runoff (mm)|
|7|T_YON|N Loss with Sediment (mt)|27|T_SSF|T - Subsurface flow (mm)|
|8|T_QNO3|N Loss in Surface Runoff (mt)|28|T_PRK|T - Percolation (mm)|
|9|T_SSFN|N in Subsurface Flow (mt)|29|T_PRCP|T-Rainfall (mm)
|10|T_PRKN|N Loss in Percolate (mt)|30|T_PET|T - Potential Evapotranspiration (mm)|
|11|T_FNO|N - Organic Fertilizer (mt)|31|T_ET|T - Evapotranspiration (mm)|
|12|T_FNO3|N - Nitrate Fertilizer (mt)|32|T_QDRN|T - Drain Tile Flow (mm)|
|13|T_FNH3|N - Ammonia Fertilizer (mt)|33|T_MUSL|T - Water erosion (ton/ha)|
|14|T_OCPD|Organic Carbon in Plow Layer (1000mt)|34|T_DRNN|T - N in drain tile flow (kg/ha)|
|15|T_TOC|Organic Carbon in Soil Profile (1000mt)|35|T_DRNP|T - P in Drain Tile Flow (kg/ha)|
|16|T_TNO3|Total NO3 in Soil Profile (mt)|36|T_PRKP|T-P in Percolation (kg/ha)|
|17|T_DN2|N-N2O from NO3 Denitrification (mt)|37|T_FPO|T - Organic P Fertilizer (kg/ha)|
|18|T_YLN|N Used by Crop (mt)|38|T_FPL|T - Labile P Fertilizer (kg/ha)|
|19|T_YLP|P Used by Crop (mt)|39|T_MNP|T - P Mineralized (kg/ha)|
|20|T_FTN|N Applied (mt)|
\*Water on agricultural lands.

The following sample script file, which is contained in the Raster Tools script directory, has all required environment variables and can be modified and run at the command line:

**epicYearlyAverage4QA.csh**

----
<a id="othertools"><a/>
## Other Tools and Utilities

### Domain Grid Shapefile Generation Tool

Users can apply the domain grid shapefile generation tool to generate a
polygon shapefile for a defined grid domain with the GRIDID attribute.
The GRIDID attribute has values ranging from 1 for the grid cell in the
lower left corner of the domain to the maximum number of cells for the
grid cell in the upper right.

The following sample script file can be modified for domain shapefile generation.

**generateGridShapefile.csh** 

### Other Utilities

The following utility programs are stored in the SA_HOME/util directory:

-   **goes_untar.pl** – used to untar downloaded GOES data into the
    format required for the GOES cloud product processing tool.

-   **updateWRFinput_landuse.R** – used to update the wrfinput file
    using generated land use data by the [NLCD and MODIS land cover
    generation tool](#nlcd_LC). The updated wrfinput file can be
    used in WRF simulations with the WRF Pleim-Xiu Land Surface Model,
    using the 40 classes of NLCD/MODIS land cover data shown in [Table 1](#Table-1).

----
<a id="acknowledge4"><a/>
## Acknowledgments ##

The SA Raster Tools were developed with support from multiple projects:

-   Work assignments from the U.S. EPA under Contract No. EP-W-09-023,
    “Operation of the Center for Community Air Quality Modeling and
    Analysis (CMAS)”
-   NASA Research Opportunities in Space and Earth Sciences (ROSES)
    projects awarded to (1) the Institute for the Environment at the
    University of North Carolina at Chapel Hill (contract number
    NNX08AL28G) and (2) the National Space Science and Technology Center
    at the University of Alabama in Huntsville (contract number
    NNX09AT60G).

****
[<< Previous Chapter](SA_ch03_vector.md) - [Home](README.md) - [Next Chapter >>](SA_ch05_surrogate.md)<br>

Spatial Allocator User Manual (c) 2016<br>
