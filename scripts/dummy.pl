#!/usr/bin/perl
#
# !!begin
# !!title Add a dummy aperture for scattered light correction
# !!author T.R.Marsh
# !!created 28 Jan 2002
# !!root dummy
# !!index dummy
# !!descr add a dummy aperture for scattered light correction
# !!class Scripts
# !!css ultracam.css
# !!head1 dummy -- add a dummy aperture for scattered light correction
# 
# !!emph{dummy} is a script to set the position of a dummy aperture to correct
# for scattered light from a bright star near a target.
# An input aperture file must be created with one aperture
# placed on the main target, and another placed on the bright
# star. These must be the same ones for each CCD. Then the script
# will modify the bright star aperture to be a dummy aperture the
# same distance from the bright star as the target but rotated
# to a new position. It is up to the user to ensure a rotaation angle
# that places the dummy in a blank area. Neither the target nor the dummy
# apertures should be linked in the input file, but after the output file is
# produced you should use !!ref{setaper.html}{setaper} to link the dummy
# aperture to a real star to prevent any attempt at repositioning it
# by !!ref{reduce.html}{reduce}.
#
# Arguments:
#
# !!table
# !!arg{input}{input aperture file} 
# !!arg{output}{output aperture file} 
# !!arg{angle}{Angle in degrees to rotate by, anti-clockwise}
# !!arg{target}{Aperture number of target}
# !!arg{dummy}{Aperture number of the bright star which will become the dummy aperture number}
# !!table
#
# !!end

(@ARGV == 5) or die "usage: input output angle target dummy\n";

$input  = shift;
$output = shift;
$angle  = shift;
$target = shift;
$dummy  = shift;

# Rotation matrix

$angle *= 8.*atan2(1.,1.)/360.;
$r11    = +cos($angle);
$r12    = -sin($angle);
$r21    = -$r12;
$r22    = $r11;

open(INPUT, "$input") or die "Failed to open $input for input\n";
open(OUTPUT, ">$output") or die "Failed to open $output for output\n";

while(<INPUT>){
    $store[$n] = $_;
    if(/CCD (.*):/){
	if(defined $tx && defined $ty && defined $dx && defined $dy){

	    $xoff = $tx - $dx;
	    $yoff = $ty - $dy;
	    $dx  += $r11*$xoff + $r12*$yoff; 
	    $dy  += $r21*$xoff + $r22*$yoff;
 
	    $dx   = int(1000*$dx+0.5)/1000;
	    $dy   = int(1000*$dy+0.5)/1000;
	    for($i=0; $i<@store; $i++){
		$line = $store[$i];
		if($i == $nd){
		    $line =~ s/^x,y = .*?, .*?\;(.*)$/x,y = $dx, $dy\; $1/;
		}
		print OUTPUT $line;
	    }
	    undef @store;
	    undef $tx;
	    undef $ty;
	    undef $dx;
	    undef $dy;
	    undef $nccd;
	    undef $naper;
	    undef $n;
	}
	$nccd  = $1;
	   
    }

    if(defined $nccd && /Aperture (.*):/){
	$naper = $1;
    }

    if(defined $naper && $naper == $target && /^x,y = (.*?), (.*?)\; x_off,y_off = (.*?), (.*?)\;/){
	($3 == 0 && $4 == 0) or die "Target aperture of CCD $nccd is linked!\n";
	$tx = $1;
	$ty = $2;
    }

    if(defined $naper && $naper == $dummy && /^x,y = (.*?), (.*?)\; x_off,y_off = (.*?), (.*?)\;/){
	($3 == 0 && $4 == 0) or die "Dummy aperture of CCD $nccd is linked!\n";
	$dx = $1;
	$dy = $2;
	$nd = $n
    }

    $n++;
}
close(INPUT);

# finish off last one too

if(defined $tx && defined $ty && defined $dx && defined $dy){
    
    $xoff = $tx - $dx;
    $yoff = $ty - $dy;
    $dx  += $r11*$xoff + $r12*$yoff; 
    $dy  += $r21*$xoff + $r22*$yoff;
    $dx   = int(1000*$dx+0.5)/1000;
    $dy   = int(1000*$dy+0.5)/1000;

    for($i=0; $i<@store; $i++){
	$line = $store[$i];
	if($i == $nd){
	    $line =~ s/^x,y = .*?, .*?\;(.*)$/x,y = $dx, $dy\; $1/;
	}
	print OUTPUT $line;
    }
    undef @store;
}

# print out remainder of store if any.

for($i=0; $i<@store; $i++){
    print OUTPUT $store[$i];
}

close(OUTPUT);

exit;
