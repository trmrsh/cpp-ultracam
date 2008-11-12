#!/usr/bin/perl -w
#
# !!begin
# !!title   Works out position changes for setting up CCDs
# !!author  T.R.Marsh
# !!created 19 November 2006
# !!root    setup
# !!index   setup
# !!descr   Works out position changes for setting up CCDs
# !!class   Scripts
# !!class   Observing
# !!class   Spectra
# !!css     ultracam.css
# !!head1   setup -- works out position changes for setting up CCDs
# 
# !!emph{setup} takes the names of two ucm files which represent two similar exposures
# but with some sort of change such as collimator position, tilt, or rotation. It also takes
# a file defining windows. It will then crop each of the two frames to match the windows file,
# collapse them, measure the positions of peaks and report the change. It gives the positions of
# the features (assumed in the middle of the window in the collapsed direction).
#
# Only works for single CCD frames, but easily modifiable if necessary. Usage of this routine is as
# follows: take a suitable setup frame (e.g. an arc which is best for defining focus, rotation and tilt). 
# Using !!ref{setwin.html}{setwin} set windows around the arc lines you want to measure. If defining tilt,
# you would want them as close to the corners of the chip as you can manage; for rotation you might want
# a couple at each end of a single line in the middle of the dispersed spectrum. This will be the file 'windows'
# that you need below. Take two exposures with something changing (hartman, rotation or tilt of the chip). Run
# this routine. Your aim should be to get the position shifts to be as small as possible.
#
# The script generates two files zzz_log1 and zzz_log2 which record the positions measured.
#
# !!head2 Arguments:
#
# !!table
# !!arg{frame1}{input frame 1 (with or without .ucm)} 
# !!arg{frame2}{input frame 2 (with or without .ucm). Enter 'null' if you don't want a second frame and
# just want absolute positions.}
# !!arg{windows}{windows isolating the features to be measured (with or without .win)}
# !!arg{dirn}{Direction to collapse frames, X or Y}
# !!arg{medfilt}{Half-width in pixels of median filter, 0 for none}
# !!arg{fwhm}{FWHM to measure peaks, unbinned pixels}
# !!arg{height}{Height threshold for peaks}
# !!tables
#
# !!end

(@ARGV == 7) or die "usage: frame1 frame2 windows dirn medfilt fwhm height\n";

my $frame1  = shift;
if($frame1 !~ /\.ucm$/){
    $frame1 .= ".ucm";
}
(-f $frame1 && -r $frame1) or die "$frame1 is not a file or is not readable\n";

my $frame2  = shift;
if($frame2 ne "null"){
    if($frame2 !~ /\.ucm$/){
	$frame2 .= ".ucm";
    }
    (-f $frame2 && -r $frame2) or die "$frame2 is not a file or is not readable\n";
}

my $windows = shift;
if($windows !~ /\.ucm$/){
    $windows .= ".win";
}
(-f $windows && -r $windows) or die "$windows is not a file or is not readable\n";

open(WIN, "$windows") or die "Failed to open $windows for input\n";
$nwin = 0;
while(<WIN>){
    if(/llx,lly = (\d+), (\d+); nx,ny = (\d+), (\d+); xbin,ybin = (\d+), (\d+)/){
	$nwin++;
	$win{$nwin} = {
	    llx  => $1,
	    lly  => $2,
	    nx   => $3,
	    ny   => $4,
	    xbin => $5,
	    ybin => $6
	    };
    }
}
close(WIN);

my $dirn    = shift;
($dirn eq 'X' || $dirn eq 'Y') or die "dirn must be either 'X' or 'Y'\n";

my $medfilt = shift;
($medfilt >= 0) or die "Median filter half-width must be >= 0\n";

my $fwhm    = shift;
($fwhm >= 2.) or die "FWHM must be >= 2\n";

my $height  = shift;
($height >= 0.) or die "height must be >= 0\n";

defined $ENV{ULTRACAM} or die "Environment variable ULTRACAM must point the location of the executables\n";

my $ULTRACAM = $ENV{ULTRACAM};

`$ULTRACAM/crop $frame1 $windows zzz_$frame1`;
`$ULTRACAM/collapse zzz_$frame1 $dirn s no min max $medfilt zzz_$frame1`;
`$ULTRACAM/ppos zzz_$frame1 $fwhm $height 1 0.5 > zzz_log1`;

# Read the first log file, save the data in a hash

open(LOG, "zzz_log1") or die "Failed to open first log file, zzz_log1\n";
while(<LOG>){
    if(/CCD (\d+), window (\d+),.*peak number (\d+), position = ([\d\.]+) \+\/- ([\d\.]+)/){
	$nccd  = $1;
	$nwin  = $2;
	$npeak = $3;
	$pos   = $4;
	$err   = $5;

	if($frame2 ne  "null"){

	    $hash{"$nccd|$nwin|$npeak"} = {
		NCCD  => $nccd,
		NWIN  => $nwin,
		NPEAK => $npeak,
		POS   => $pos,
		ERR   => $err
		};
	}else{

	    if($dirn eq 'X'){
		$wref = $win{$nwin};
		$xmid = $wref->{llx} + ($wref->{xbin}*$wref->{nx}-1)/2.;
		printf("CCD = %2d, window = %2d, peak = %2d, X,Y = %6.2f, %6.2f +/- %5.3f\n",
		       $nccd, $nwin, $npeak, $xmid, $pos, $err);
	    }else{
		$wref = $win{$nwin};
		$ymid = $wref->{lly} + ($wref->{ybin}*$wref->{ny}-1)/2.;
		printf("CCD = %2d, window = %2d, peak = %2d, X,Y = %6.2f +/- %5.3f, %6.2f\n",
		       $nccd, $nwin, $npeak, $pos, $err, $ymid);
	    }
	}
    }
}
close(LOG);

if($frame2 ne  "null"){

    `$ULTRACAM/crop $frame2 $windows zzz_$frame2`;
    `$ULTRACAM/collapse zzz_$frame2 $dirn s no min max $medfilt zzz_$frame2`;
    `$ULTRACAM/ppos zzz_$frame2 $fwhm $height 1 > zzz_log2`;

# Read the second log file, using the hash to identify matching
# entries.
    
    open(LOG, "zzz_log2") or die "Failed to open second log file, zzz_log2\n";
    while(<LOG>){
	if(/CCD (\d+), window (\d+),.*peak (\d+), position = ([\d\.]+) \+\/- ([\d\.]+)/){
	    $nccd  = $1;
	    $nwin  = $2;
	    $npeak = $3;
	    $pos   = $4;
	    $err   = $5;
	    if(defined $hash{"$nccd|$nwin|$npeak"}){
		$diff = $pos - $hash{"$nccd|$nwin|$npeak"}->{POS};
		$derr = sqrt($err**2 + $hash{"$nccd|$nwin|$npeak"}->{ERR}**2);
		if($dirn eq 'X'){
		    $wref = $win{$nwin};
		    $xmid = $wref->{llx} + ($wref->{xbin}*$wref->{nx}-1)/2.;
		    printf("CCD = %2d, window = %2d, peak = %2d, X,Y = %6.2f, %6.2f, Y pos change = %6.2f +/- %5.3f\n",
			   $nccd, $nwin, $npeak, $xmid, $pos, $diff, $derr);
		}else{
		    $wref = $win{$nwin};
		    $ymid = $wref->{lly} + ($wref->{ybin}*$wref->{ny}-1)/2.;
		    printf("CCD = %2d, window = %2d, peak = %2d, X,Y = %6.2f, %6.2f, X pos change = %6.2f +/- %5.3f\n",
			   $nccd, $nwin, $npeak, $pos, $ymid, $diff, $derr);
		}
	    }
	}
    }
    close(LOG);

}

exit;
    
