#!/usr/bin/perl -w
#
# The script produces time, target/comparison, uncertainty, flag where the times are 
# in MJD at the centre of each exposure. The flag takes values of 'OK', 'NO_SKY_BACKROUND' 
# (i.e there was no background estimate for some reason), 'SATURATED' and 'PEPPERED'. 
# 'PEPPERED' indicates that the peak height on one or both stars exceeded the level at 
# which the chips can show non-linear behaviour. The routine prints the data to stdout;
# pipe via grep and re-direct to save to disk while selecting by the flag value.
#
# Invocation: ult2asc logfile ccd target aperture
#
# Arguments
#
# logfile    --- name of log file, e.g. run009.log
# ccd        --- CCD number, 1, 2 or 3
# target     --- target aperture number (usually 1)
# comparison --- comparison aperture number (usually, but not always 2)
#
#
# !!begin
# !!title    ult2asc
# !!author   T.R. Marsh
# !!created  22 Mar 2006
# !!root     ult2asc
# !!index    ult2asc
# !!descr    simple converter to ASCII
# !!class    Scripts
# !!css      ultracam.css
# !!head1    ult2asc converts reduce log files to ASCII
#
# !!emph{ult2asc} is a simple script to make it easy for people to access their data if sent 
# as a giant ULTRACAM ASCII file produced by !!ref{reduce.html}{reduce}.
#
# The script produces time, target/comparison, uncertainty, flag
# where the times are in MJD at the centre of each exposure. The flag takes values
# of 'OK', 'NO_SKY_BACKROUND' (i.e there was no background estimate for some reason),
# 'SATURATED' and 'PEPPERED'. 'PEPPERED' indicates that the peak height on one or both
# stars exceeded the level at which the chips can show non-linear behaviour.
#
# The routine prints the data to stdout; pipe via grep and re-direct to save to disk 
# while selecting by the flag value.
#
# !!head2 Invocation
#
# ult2asc logfile ccd target aperture
#
# !!head2 Arguments
#
# !!table
# !!arg{logfile}{name of log file, e.g. run009.log}
# !!arg{ccd}{CCD number, 1, 2 or 3}
# !!arg{target}{target aperture number (usually 1)}
# !!arg{comparison}{comparison aperture number (usually, but not always 2)}
# !!table
#
# !!end

use strict;

(@ARGV == 4) or die "usage: logfile ccd target comparison\n";

my $logfile    = shift;
-e $logfile or die "$logfile does not exists.\n";

my $ccd        = shift;
($ccd >= 1 && $ccd <= 3) or die "CCD number must be 1, 2 or 3\n";

my $target     = shift;
($target > 0) or die "Target aperture must be > 0\n";

my $comparison = shift;
($comparison > 0 && $comparison != $target) or die "Comparison aperture must be > 0 and different from the target aperture\n";

my $apmax = $target > $comparison ? $target : $comparison;

open(LOG, $logfile);

my @field;

my ($time,$nccd,$d1,$d2,$d3,$d4,$d5,$d6);

while(<LOG>){
    if(!/^\#/ && !/^\s*$/){
	($d1,$time,$d1,$d2,$d3,$nccd,$d5,$d6,@field) = split(' ');
	if($nccd == $ccd){
	    (@field >= 14*$apmax) or die "Did not find enough entries for CCD $ccd given maximum aperture number = $apmax\n";
	    my $targ_counts = $field[14*($target-1)+7];
	    my $targ_errors = $field[14*($target-1)+8];
	    my $comp_counts = $field[14*($comparison-1)+7];
	    my $comp_errors = $field[14*($comparison-1)+8];
	    if($targ_errors > 0 || $comp_errors > 0){

		my $targ = $targ_counts / $comp_counts;
		my $err  = $targ*sqrt(($targ_errors/$targ_counts)**2 + ($comp_errors/$comp_counts)**2);

		my $targ_flag = $field[14*($target-1)+13];
		my $comp_flag = $field[14*($comparison-1)+13];

		my $ecode;
		if($targ_flag > 8 || $comp_flag > 8){
		    next;
		}elsif($targ_flag == 8 || $comp_flag == 8){ 
		    $ecode = "SATURATED";
		}elsif($targ_flag == 6 || $comp_flag == 6){ 
		    $ecode = "NO_SKY_BACKGROUND";
		}elsif($targ_flag == 5 || $comp_flag == 5){ 
		    $ecode = "PEPPERED";
		}else{
		    $ecode = "OK";
		}
		
		print "$time $targ $err $ecode\n";
	    }
	}
    }
}

exit;
