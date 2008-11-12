#!/usr/bin/perl
#
# !!begin
# !!title Shifts a set of apertures
# !!author T.R.Marsh
# !!created 14 June 2004
# !!root shiftaper
# !!index shiftaper
# !!descr shifts a set of apertures
# !!class 
# !!css ultracam.css
# !!head1 shiftaper -- shifts a set of apertures
# 
# Very often the only change in apertures from one run to
# the next is a shift. If this cannot be handled by reduce
# the you may just want to apply the shift with this routine.
#
# !!head2 Arguments:
#
# !!table
# !!arg{input}{input aperture file} 
# !!arg{output}{output aperture file} 
# !!arg{xshift}{Shift to add in X}
# !!arg{yshift}{Shift to add in Y}
#
# !!end

(@ARGV == 4) or die "usage: input output xshift yshift\n";

$input  = shift;
$output = shift;
$xshift = shift;
$yshift = shift;

open(INPUT, "$input") or die "Failed to open $input for input\n";
open(OUTPUT, ">$output") or die "Failed to open $output for output\n";

while(<INPUT>){
    if(/^x,y = (.*?), (.*?);/){
	$x  = $1;
	$y  = $2;
	$x += $xshift;
	$y += $yshift;
	s/^(x,y = ).*?(;.*)/$1$x, $y$2/;
    }
    print OUTPUT;
}
close(INPUT);
close(OUTPUT);

exit;
