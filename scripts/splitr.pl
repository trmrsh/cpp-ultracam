#!/usr/bin/perl
#
# !!begin
# !!title ULTRACAM pipeline: splitr
# !!author T.R.Marsh
# !!created 08 Nov 2005
# !!root  splitr
# !!index splitr
# !!descr splits multi-aperture reduce files
# !!class Scripts
# !!css ultracam.css
# !!head1 splitr splits up multi-apertue reduce files
# 
# The optimum aperture scale factor is not obvious a priori. For 
# faint objects 1.5 often seems about right, but for bright objects 
# this becomes larger. Thus !!ref{reduce.html}{reduce} has a multi-aperture
# option which can extract several apertures relatively efficiently compared
# to re-running it several times over. The resulting log files contain all the apertures
# and need splitting to reduce them to normal format. This command does this task.
#
# !!head2 Arguments:
#
# !!table
# !!arg{input}{input reduce log file. The output files will be generated automatically from this. e.g. if
# the input is run003.log, then output of the form run003_r1.5.log run003_r1.6.log will be produced.} 
# !!table
#
# !!end

(@ARGV == 1) or die "usage: input\n";

$root = $input = shift;
$root =~ s/\.log$//;

$in_header = 1;
open(INPUT, "$input")    or die "Failed to open $input for input\n";
while(<INPUT>){

    if($in_header && (/^\#/ || /^\s*$/)){

	if(/^\# Aperture radii\s*=\s*(.*)/){
	    $radii = $1;
	}else{
	    push @header, $_;
	}

    }elsif($in_header){

# Write the header out for each individual file.

	$in_header = 0;
	defined $radii or die "No multiple aperture radii found\n";
	@radii = split(' ',$radii);
	for($n=0; $n<@radii; $n++){
	    $file = $file[$n] = "FH$n";
	    open($file, ">${root}_r$radii[$n].log") or die "Failed to open ${root}_r$radii[$n].log\n";
	    print $file "#\n# This file was produced by running 'split' on $input\n";
	    foreach $header (@header){
		if($header =~ /^\# Extraction control\s*= (\d+ \S+ \S+) [\d\.]+ (.*)/){
		    if($n == 0){
			$nccd++;
		    }
		    print $file "# Extraction control                                 = $1 ",$radii[$n]," $2\n";
		}else{
		    print $file "$header";
		}
	    }
	}
	
	print "NCCD = $nccd\n";

	$file = $file[$nfile];
	select $file;
	print;
	$nout = 1;
	if($nout == $nccd){
	    $nout = 0;
	    $nfile++;
	    $nfile = $nfile % scalar(@radii);
	    $file = $file[$nfile];
	    select $file;
	}

    }else{

	print;
	$nout++;
	if($nout == $nccd){
	    $nout = 0;
	    $nfile++;
	    $nfile = $nfile % scalar(@radii);
	    $file = $file[$nfile];
	    select $file;
	}

    }
}
close(INPUT);

select STDOUT;

for($n=0; $n<@radii; $n++){
    $file = $file[$n];
    close($file);
    print "Written ${root}_r$radii[$n].log\n";
}

exit;
