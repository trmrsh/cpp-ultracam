#!/usr/bin/perl -w
#
# !!begin
# !!title   Fits avalanche gain of L3CCD
# !!author  T.R.Marsh
# !!created 22 November 2006
# !!revised 23 November 2006
# !!root    l3gain
# !!index   l3gain
# !!descr   Fits avalanche gain of L3CCD
# !!class   Scripts
# !!class   Observing
# !!class   Spectra
# !!css     ultracam.css
# !!head1   l3gain -- fits avalanche gain of L3CCD
# 
# !!emph{l3gain} works out the avalanche gain given a frame with at most one real count per pixel
# (just a bias will often do because of CICs). It takes a histogram and then fits with an exponential
# (straight line to the log), with the gradient returning the gain. This is based upon the principle
# that given a single photon input, the avalanche gain system has an output probability distribution
# closely approximated by (1/g)*exp(-c/g) (above the bias level). Thus the distribution for a low counts
# CCD will consist of a weighted sum of the single electron pixels plus the zero electron pixels, the latter
# leading to a peak around the bias level spread by normal read noise. We can therefore deduce the gain by making
# a straight line fit to the log(histogram) as long as we exclude the zero electron events. This routine does
# so somewhat crudely by asking the user to specify the range of count levels over which to fit. 
#
# Pixels with more than one electron are likely to cause problems. These lead to a distribution which is the convolution of
# two single electron distributions. This will lead to a slower fall-off at high count levels than expected which
# could be mis-interpreted as a larger gain than is really the case. The best test data therefore will be dominated
# by zero electron pixels. Deviations from lineraity of the plot produced by this routine in the sense of a decrease in the
# slope at high count levels is an indication of this problem. A more sophisticated treatment could take this into account.
#
# !!emph{NB} This script requires that you have installed the program !!emph{ponto} and have defined
# environment variables PONTO pointing to the program and ULTRACAM pointing to the directory of
# ULTRACAM executables.
#
# !!head2 Arguments:
#
# !!table
# !!arg{x1}{The left-hand X limit of the fit. Should exclude the zero electron pixels. If you have not a clue
# of a good value, then run !!ref{stats.html}{stats} with window=ALL}
# !!arg{x1}{The right-hand X-limit of the fit. Large enough to get a decent change without including values
# with enormous errors.}
# !!arg{nbin}{Number of histogram bins to use from x1 to x2}
# !!arg{image1, image2 etc}{the test images. Use lots to build up statistics. These are sent to the 'hist' program
# which can handle them fairly quickly.} 
# !!table
#
# !!end

use strict;

my $PONTO    = defined $ENV{"PONTO"} ? $ENV{"PONTO"} : 
    die "Environment variable PONTO pointing to the equivalent program is undefined\n";

my $ULTRACAM = defined $ENV{"ULTRACAM"} ? $ENV{"ULTRACAM"} : 
    die "Environment variable ULTRACAM pointing to location of executables is undefined\n";

(@ARGV > 4) or die "usage: device x1 x2 nbin image1 image2 ..\n";

my $device= shift;
my $x1    = shift;
my $x2    = shift;
($x2 > $x1 + 50) or die "x2 must be greater than x1 by at least 50\n";
my $nbin  = shift;

@ARGV or die "No filenames loaded\n";

# create the histogram
open(FLIST, ">zzz_file_list") or die "Failed to open zzz_file_list\n";
print FLIST join("\n",@ARGV);
close(FLIST);

# generate ponto mask file

my $px1 = $x1 - ($x2-$x1)/5.;
my $px2 = $x2 + ($x2-$x1)/5.;
my $nebin = int(1.4*$nbin);

system("$ULTRACAM/hist dump=yes zzz_file_list ALL $nebin $px1 $px2 zzz_hist");


my $pmask = "zzz_pmask.msk";
open(MASK,">$pmask") or die "Could not open $pmask for writing\n";

print MASK <<END1;
Number_of_masks 2
Xmask: -1000 $px1 1
Xmask:  $px2 2000000 1

END1

close(MASK);

my $mask = "zzz_mask.msk";
open(MASK,">$mask") or die "Could not open $mask for writing\n";

print MASK <<END2;
Number_of_masks 2
Xmask: -1000 $x1 1
Xmask:  $x2 2000000 1

END2

close(MASK);

# generate ponto fit file

my $fit = "zzz_fit.fit";
open(FIT,">$fit") or die "Could not open $fit for writing\n";

print FIT <<END3;
Equation: norm - 0.43429*(x-$x1)/gain

norm = 4
gain = 10

END3
 
close(FIT);

# read in the histogram, take log

open(PONTO, "|$PONTO") or die "Failed to open pipe to ponto\n";
ponto("lfasc zzz_hist 1 1 0 2 3 Value none \"\" Number none \"\" Histogram p yes");
ponto("log10 1 11");
ponto("mask 11 $pmask");
ponto("setlims 11");
ponto("mask 11 $mask");
ponto("write temp 11 n");
ponto("ffit 11 12 0 no $fit 10 no none no 10 0.0001 2e8");
ponto('xyr @x1_plot @x2_plot @y1_plot @y2_plot');
ponto("device $device");
ponto("pl 11-12");
ponto("exit\ny\n");
close(PONTO);

exit;

sub ponto {
    my $string = shift @_;
    print PONTO "$string\n";
}
