#!/usr/bin/perl
###################################################################################
#Program: goes_untar.pl
#Run:     goes_untar.pl goes_tar_files.dat 
#
#Purpose: untar download GOES date.tar file into each directory for a day. 
#
#         LR     Jan., 2009
##########################################################################################

print "\nUntar each day GOES tar file into a directory...\n";

#read data in: filename which contains all each day GOES tar
$inputdata = $ARGV[0];

open(INDATA, "./${inputdata}") || die "Can't open ${inputdata}: $!\n";

$lines = 0;
while (<INDATA>)
  {
     $lines = $lines + 1;
     chomp;
     ($name,$ext) = split(/\./, $_);
     $name = trim($name);
     print "Untar file: $_\n";
 
     system ("mkdir $name");
     system ("tar -C ./$name -xvf $_");
  }
close (INDATA);
 
print "\nThe program finished.\n";

#end of the file.
##############################################################

sub trim 
{
    my @out = @_;
    for (@out) 
    {
        s/^\s+//;
        s/\s+$//;
    }
    return wantarray ? @out : $out[0];
}
