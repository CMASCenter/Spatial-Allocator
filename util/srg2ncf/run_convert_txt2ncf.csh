#!/bin/csh  -f

setenv CASE NHEMI108
set BIN = $cwd
setenv INDIR /nas01/Surrogates/2014/Spatial-Allocator/srgtools/output/$CASE
setenv OUTDIR $cwd/$CASE

if ( ! -d $OUTDIR ) mkdir $OUTDIR
cd $INDIR
foreach srg ( `ls USA_2*_NOFILL.txt`   )
   set surg = $srg    #USA_${srg}_NOFILL.txt
   echo $surg
   setenv INPUTF $INDIR/$surg
   setenv OUT_NETCDF $OUTDIR/$surg.ncf
echo '########################################################################'
echo $INPUTF

$BIN/convert_txt2ncf.x

end
