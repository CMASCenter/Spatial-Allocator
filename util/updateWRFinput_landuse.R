#-----------------------------------------------------------
#  Written by Robert Gilliam at US EPA ASMD, 2012
#  Modified  by LR   02/2013
#
#-----------------------------------------------------------
# R BATCH --no-save < /work/MOD3DEV/pleim/wrf_lu_test/MODIS2006/geogrid_nlcd40_LR.R
# setenv WRF_NLCD /work/MOD3DEV/pleim/wrf_lu_test/MODIS2006/wrf_conus12km_modis06.nc
# setenv WRF_NLCD /work/MOD3DEV/pleim/wrf_lu_test/MODIS2006/wrf_tx4km_modis06.nc
# setenv WRF_NLCD /work/MOD3DEV/pleim/wrf_lu_test/MODIS2006/wrf_tx1km_modis06.nc
# setenv WRF_INPUT   d01
  require(ncdf)

 wrf_input<-Sys.getenv("WRF_INPUT")         
# wrf_input<-"d01"

 
 nlcd_file<-Sys.getenv("WRF_NLCD")         
 fnlcd  <-open.ncdf(nlcd_file)
 	LUF <- get.var.ncdf(fnlcd,varid="LANDUSEF")
 	LM <- get.var.ncdf(fnlcd,varid="LANDMASK")
 	LI <- get.var.ncdf(fnlcd,varid="LU_INDEX")
 close.ncdf(fnlcd)

   kwat0<-17
   kwat1<-18
   kwat2<-19
   kwat3<-21
   kice0<-15
   kice1<-22
   wrf_lu_index<-seq(1,40)
   nlcd_index<-c(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,
                 21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40) 
 
 LUF[,,kwat0]<- LUF[,,kwat0] + LUF[,,kwat1] + LUF[,,kwat2] + LUF[,,kwat3] 
 LUF[,,kwat1]<-0.0000
 LUF[,,kwat2]<-0.0000
 LUF[,,kwat3]<-0.0000
 
 LUF[,,kice0]<- LUF[,,kice0] + LUF[,,kice1] 
 LUF[,,kice1]<-0.0000

 LI_NEW<-LI

 LI<- LI_NEW
 LI<-ifelse(LI == kwat1, kwat0, LI)
 LI<-ifelse(LI == kwat2, kwat0, LI)
 LI<-ifelse(LI == kwat3, kwat0, LI)
 LI<-ifelse(LI == kice1, kice0, LI)
 
 XLAND<-ifelse(LM == 0, 2, 1)
  
 fgeo  <-open.ncdf(paste("wrfinput_",wrf_input,sep=""), write=T)
 	      
        put.var.ncdf( fgeo, "LANDUSEF", LUF/100)
        put.var.ncdf( fgeo, "LANDMASK", LM)
        put.var.ncdf( fgeo, "LU_INDEX", LI)
        put.var.ncdf( fgeo, "XLAND", XLAND)
        put.var.ncdf( fgeo, "IVGTYP", LI)
        
	att.put.ncdf( fgeo , 0, "MMINLU", "NLCD40" )
	att.put.ncdf( fgeo , 0, "ISWATER", kwat0 )
	att.put.ncdf( fgeo , 0, "ISICE", kice0 )

 close.ncdf(fgeo)
quit(save="no")
