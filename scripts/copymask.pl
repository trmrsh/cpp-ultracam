#!/usr/bin/perl
#
# !!begin
# !!title Copy a star mask
# !!author T.R.Marsh
# !!created 28 Jan 2002
# !!root copymask
# !!index copymask
# !!descr copies a mask from one aperture to another
# !!class 
# !!css ultracam.css
# !!head1 copymask -- copies a mask from one aperture to another
# 
# !!emph{copymask} copies a mask from one aperture to another. i.e. if the same 
# star affects more than one aperture, it should be masked identically from them
# all. This is possibly more common that one might imagine if one is forced to use
# dummy apertures to correct for scattered light from a nearby bright star.
#
# NB copymask only handles the first mask of an aperture.
#
# Arguments:
#
# !!table
# !!arg{input}{input aperture file} 
# !!arg{output}{output aperture file} 
# !!arg{mask}{Aperture number which is already masked}
# !!arg{copy}{Aperture number to mask}
#
# !!end

(@ARGV == 4) or die "usage: input output mask copy\n";

$input  = shift;
$output = shift;
$mask   = shift;
$copy   = shift;

open(INPUT, "$input") or die "Failed to open $input for input\n";
open(OUTPUT, ">$output") or die "Failed to open $output for output\n";

while(<INPUT>){
    $store[$n] = $_;
    if(/CCD (.*):/){
	if(defined $xcopy && defined $xmask){

	    $xoff = $xmask - $xcopy;
	    $yoff = $ymask - $ycopy;

	    for($i=0; $i<@store; $i++){
		$line = $store[$i];
		if($i == $nd){
		    chop $line;
		    $line = $line . ", masked = $xoff $yoff $rmask\n";
		}
		print OUTPUT $line;
	    }
	    undef @store;
	    undef $xmask;
	    undef $xcopy;
	    undef $n;
	}
	$nccd  = $1;
	   
    }

    if(defined $nccd && /Aperture (.*):/){
	$naper = $1;
    }

    if(defined $naper && $naper == $mask && /^x,y = (.*?), (.*?)\; x_off,y_off = (.*?), (.*?).*masked = (\S*) (\S*) (\S*)/){
	$xmask = $1 + $3 + $5;
	$ymask = $2 + $4 + $6;
	$rmask = $7;
    }

    if(defined $naper && $naper == $copy && /^x,y = (.*?), (.*?)\; x_off,y_off = (.*?), (.*?)\;.*state = .\s*$/){
	$xcopy = $1 + $3;
	$ycopy = $2 + $4;
	$nd    = $n;
    }

    $n++;
}
close(INPUT);

# finish off last one too

if(defined $xcopy && defined $xmask){
    
    $xoff = $xmask - $xcopy;
    $yoff = $ymask - $ycopy;
    
    for($i=0; $i<@store; $i++){
	$line = $store[$i];
	if($i == $nd){
	    chop $line;
	    $line = $line . ", masked = $xoff $yoff $rmask\n";
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
