#!/usr/local/bin/perl
#
# Plots reduce data files

use PGPLOT;

(@ARGV == 1) or die "usage: plot.pl device";

$device = shift;

@files = ('ouvir_run40.log', 'ouvir_run41.log');

# Plot title
$title = "ULTRACAM + WHT on OU Vir, 16/17 May 2002";

# Apertures for target and comparison
$targ = 1;
$comp = 2;

# Whether to plot a CCD, scaling factors and colours
@plot   = (1, 1, 1);
@scale  = (0.03, 0.05,0.3);
@colour = (2,3,4);

# Offset time and flux
$tcon = 52410;
$yoff = 0.;

# Plot ranges
$x1 = 0.9;
$x2 = 1.2;
$y1 = 0.;
$y2 = 2.;

$targ--;
$comp--;

foreach $file (@files){
    open(FILE, $file) or die "Failed to open $file\n";

    while(<FILE>){
	if(!/^#/){
	   @line = split(' ');
	   shift(@line);
	   $time[$n] = shift(@line) - $tcon;
	   shift(@line);
	   shift(@line);
	   shift(@line);
	   $nccd     = shift(@line)-1;
	   while(scalar(@line)){
	       $naper = shift(@line)-1;
	       shift(@line);
	       shift(@line);
	       $counts[$nccd][$naper][$n] = shift(@line);
	       $errors[$nccd][$naper][$n]  = shift(@line);
	       shift(@line); shift(@line); shift(@line);
	   }
	   if($nccd == 2){$n++;}
       }	       
    }
    close(FILE);
}

print "Loaded $n points\n";

pgopen($device);
pgsch(1.5);
pgscf(2);
pgslw(3);
pgsci(4);
pgenv($x1,$x2,$y1,$y2,0.,0.);
pgsci(2);
pglab("MJD - $tcon", "Normalised flux", $title);

# scale counts and plot

for($nccd=0; $nccd<3; $nccd++){
    for($i=0; $i<@{$counts[$nccd][$targ]}; $i++){
	$counts[$nccd][$targ][$i] /= $scale[$nccd]*$counts[$nccd][$comp][$i];
    }
    pgsci($colour[$nccd]);
    pgpoint(scalar(@{$counts[$nccd][$targ]}),\@time, \@{$counts[$nccd][$targ]}, 1);
}

pgclos();

exit;

