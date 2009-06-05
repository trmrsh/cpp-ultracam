#!/warwick/starlink/humu/x86_64/star/Perl/bin/perl -w
#
# !!begin
# !!title Makes average pictures of runs
# !!author T.R. Marsh
# !!created 08 March 2006
# !!revised 14 March 2006
# !!root binpic
# !!descr makes average pictures of runs
# !!class Scripts
# !!css ultracam.css
# !!head1 binpic -- makes average pictures of runs
#
# This script divides up a run into nsec sections of at most
# nmax frames, evenly spaced from first to last. It then 
# medians these together, subtracts their background and plots a 
# full frame png file of all CCDs for each section. It finally generates
# an html page called runNNN.html where 'runNNN' is the run name
# which gives links to the pictures. The pictures and the html page 
# are stored in a subdirectory of whatever directory this is run in.
# The subdirectory is called 'pictures'.
#
# This is a specific routine to help out making the data archive but is
# included in the distribution in case it, or an adaptation of it, proves
# useful.
#
# !!head2 Arguments
#
# !!table
# !!arg{nsec}{Number of sections to divide a run into}
# !!arg{nmax}{The maximum number of frames per section. This is limited by the maximum number
# that !!ref{combine.html}{combine} can handle which is of order 300 I think.}
# !!arg{run}{the run name which must be of the form 'runNNN' where N is a digit}
#
# !!end

use strict;

use PGPLOT;

(@ARGV == 3) or die "usage: nsec nmax run\n";

my $nsec = shift;
($nsec > 0) or die "Number of section nsec must be > 0\n";

my $nmax = shift;
($nmax > 0 && $nmax <= 300) or die "Maximum frames per section must lie from 1 to 300\n";

my $run  = shift;

$run =~ /run\d\d\d$/ or die "For safety, only names of the form 'runNNN' where N are digits are supported\n";

(-e "$run.xml" && -e "$run.dat") or die "Could not find one or both of $run.xml and $run.dat\n";

(-r "$run.xml" && -r "$run.dat") or die "One or both of $run.xml and $run.dat is/are not readable\n";


my $run_end = $run;
$run_end =~ s/.*\///;

my $pictures = "pictures";

defined $ENV{ULTRACAM} or die "Environment variable ULTRACAM must point the location of the executables\n";

my $ULTRACAM = $ENV{ULTRACAM};

# First read xml file to see if this is drift mode, in which case we have to lose a few frames at the start

open(XML, "$run.xml") or die "Could not open $run.xml\n";

my $drift   = 0;
my $y1_size;
my $powon = 0;
while(<XML>){
    if(/DRIFTSCAN\.xml/){
	$drift   = 1;
    }elsif(/Y1_SIZE.*value=\"(\d+)/){
	$y1_size = $1;
    }elsif(/powon/){
	$powon = 1;
	last;
    }
}
close(XML);

if($powon){
    print "Run = $run is a power on and will be skipped\n";
    exit;
}

(!$drift || defined $y1_size) or die "Drift mode but Y1_SIZE cannot be found\n";

my $nfirst = $drift ? int((1033./$y1_size+1.)/2.) + 1 : 1;

# Now run 'gettimes' on the file to determine the number of the last OK frame

my $nlast = `$ULTRACAM/gettime $run | grep 'Number of frames' | sed -e 's/Number.*= //'`;

# Safety check, and chop the last frame if possible
if($nlast < $nfirst){
    print "Run = $run has no valid frames; will skip\n";
    exit;
}elsif($nlast >= $nfirst+$nsec){
    $nlast--;
}

print "Frame range: $nfirst to $nlast\n";

# temporary directory to write intermediate files to
    
my $temp_dir = ".binpic";
-e $temp_dir or mkdir $temp_dir or die "$!";
-d $temp_dir or die "$temp_dir is not a directory\n";

-e $pictures or mkdir $pictures or die "$!";
-d $pictures or die "pictures is not a directory\n";

my $nmedian = int(($nlast-$nfirst+1)/$nsec);
$nmedian = $nmedian > 0 ? $nmedian : 1; 
$nmedian = $nmedian > $nmax ? $nmax : $nmedian;

# Check whether png files exist, if yes, we skip to the end and just re-build the html

my $nact = ($nlast-$nfirst+1) / $nmedian;
$nact = $nact < $nsec ? $nact : $nsec;

my $all_png_present = 1;
for(my $n=1; $n<=$nact; $n++){
    if(!(-e "$pictures/${run_end}_sec${n}.png")){
	$all_png_present = 0;
	last;
    }
}

if($all_png_present){
    print "png files found for ${run} and they will not be re-made\n";
}else{
    print "Not all png files found for ${run} so they will be re-made\n";
}

my $ndiv = $nact > 1 ? $nact - 1 : 1;
 
if(!$all_png_present){
       
    for(my $n=0; $n<$nact; $n++){

	my $ns = $n+1;	
	my $first = int($nfirst + ($nlast-$nfirst-$nmedian+1)*$n/$ndiv + 0.5);
	my $last  = $first  + $nmedian - 1;

	print "Section $ns, frame range: $first to $last\n";
 
# Delete any runs from the directory
	
	opendir(DIR, $temp_dir) or die "$!";
	my @runs = grep /^run\d\d\d_\d+\.ucm/, readdir(DIR);
	closedir(DIR);
	
	my $file;
	foreach $file (@runs){
	    unlink "$temp_dir/$file";
	}
	
# grab group of frames
	
	system("cd $temp_dir; $ULTRACAM/grab source=l ../$run 3 $first $last bias=no trim=yes ncol=2 nrow=2 twait=0 tmax=0\n");
	
# read file names
	
	opendir(DIR, $temp_dir) or die "$!";
	@runs = grep /^run\d\d\d_\d+\.ucm/, readdir(DIR);
	closedir(DIR);
	
# write out to a list
	
	open(LIST, ">$temp_dir/ucm.lis") or die "$!";
	foreach $file (@runs){
	    print LIST "$temp_dir/$file\n";
	}
	close(LIST);
	
# median combine them
	
	system("$ULTRACAM/combine $temp_dir/ucm.lis method=m adjust=i output=$temp_dir/${run_end}_sec$ns\n");
	
# background subtract them
	
	system("$ULTRACAM/backsub $temp_dir/${run_end}_sec$ns 40 $temp_dir/${run_end}_sec$ns\n");
	
# plot them
	
	system("$ULTRACAM/plot device=$pictures/${run_end}_sec${ns}.png/png stack=x width=10 aspect=0.34 reverse=true $temp_dir/${run_end}_sec$ns nccd=0 xleft=min xright=max ylow=min yhigh=max iset=p plow=10 phigh=99 applot=no\n");
	
# clean up the directory
	
	opendir(DIR, $temp_dir) or die "$!";
	@runs = grep /^run\d\d\d_.*\.ucm/, readdir(DIR);
	closedir(DIR);
	
	foreach $file (@runs){
	    unlink "$temp_dir/$file";
	}
    }
}

# Finally write the html file

open(HTML, ">$pictures/${run_end}.html") or die "Could not open $pictures/${run_end}.html for writing.\n";

print HTML "<html>\n<body>\n<h1>Images of $run\n</h1>\n\n";
print HTML "<p>This page contains links to $nsec PNG images of CCD 2 of $run spread evenly\n";
print HTML "through the run.\n";
print HTML "<p>\n";

for(my $n=1; $n<=$nact; $n++){
    my $first = int($nfirst + ($nlast-$nfirst-$nmedian+1)*$n/$ndiv + 0.5);
    my $last  = $first  + $nmedian - 1;
    print HTML "<img src=\"${run_end}_sec${n}.png\"/>Median of frames $first to $last <br clear=left>\n";
}
 
print HTML "</body>\n</html>\n";

exit;
