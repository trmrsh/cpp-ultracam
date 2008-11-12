#!/usr/bin/perl
#
# !!begin
# !!title Fixes old apertures files so that they are still readable
# !!author T.R.Marsh
# !!created 15 June 2004
# !!root updateaper
# !!index updateaper
# !!descr updates a set of apertures
# !!class Scripts
# !!css ultracam.css
# !!head1 updateaper -- updates aperture files
# 
# Version 3.1.0 modified the aperture file format. This script allows
# you to convert old files into the new format so that they are still 
# readable.
#
# !!head2 Arguments:
#
# !!table
# !!arg{input}{input old-style aperture file} 
# !!arg{output}{output new-style aperture file} 
# !!table
#
# !!end

(@ARGV == 2) or die "usage: input output\n";

$input  = shift;
$output = shift;

open(INPUT, "$input")    or die "Failed to open $input for input\n";
open(OUTPUT, ">$output") or die "Failed to open $output for output\n";

while(<INPUT>){
    if(/^x,y =.*, ref =/){
	s/(.*),( ref =.*)/$1;$2/;	
	s/(.*),( state =.*)/$1;$2/;	
	s/(.*state = .)$/$1; masked =; extra =/;
	s/(.*state = .),( masked =.*)/$1;$2; extra =/;
    }
    print OUTPUT;
}
close(INPUT);
close(OUTPUT);

exit;
