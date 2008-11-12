#!/usr/bin/perl
#
# !!begin
# !!title Updates old reduce files to bring them up to the current version.
# !!author T.R.Marsh
# !!created 09 May 2005
# !!revised 03 Jan 2006
# !!root newreduce
# !!index newreduce
# !!descr updates old version reduce files
# !!class Scripts
# !!css ultracam.css
# !!head1 newreduce -- updates old version reduce files
# 
# When the pipeline software goes through a major version change, e.g. 5 to 6,
# the reduce files are often changed. This script will convert previous versions
# to the new one. It will work on versions dated 29/06/2004 (5.x.x), 07/05/2005
# (6.x.x) and 13/05/2005 (7.x.x) and convert them to the current version. 
#
# This version is not tested and may only do a half-way job. I am developing a GUI
# to generate reduce files which may in the end be a better bet.
#
# !!head2 Arguments:
#
# !!table
# !!arg{input}{input old-style aperture file.} 
# !!arg{output}{output new-style aperture file, can overwrite the input if need be.} 
# !!table
#
# !!end

(@ARGV == 2) or die "usage: input ouput\n";

$input  = shift;
$output = shift;


open(INPUT, "$input")    or die "Failed to open $input for input\n";
while(<INPUT>){
    $old[$n++] = $_;
    if(/^version\s*=\s*(\S*)/){
	$version = $1;
    }
}
close(INPUT);

defined $version or die "Could not determine version date\n";

$version5 = "29/06/2004";
$version6 = "07/05/2005";
$version7 = "13/05/2005";
$current  = "19/12/2005";

$version ne $current or die "File = $input is already up-to-date\n";

$version eq $version5 or $version eq $version6 or $version eq $version7 or die "Version = $version not recognised\n";

$lcurve_first = 1;

open(OUTPUT, ">$output") or die "Failed to open $output for output\n";
foreach $line (@old){

    $line =~ s%^(version\s*=\s*)\S*%${1}$current%;
    $line =~ s%^(lcurve_light_frac\s*=\s*)\S*%${1}3%;
    $line =~ s%^aperture(\s*=.*)%aperture_file${1}%;
    $line =~ s%aperture_star_radii%star_aperture_radii%;
    $line =~ s%profile_fit_sigrej%profile_fit_sigma %;
    $line =~ s%sky_clip%sky_thresh%;
    $line =~ s%^readout(\s*=.*)%calibration_readout$1%;
    $line =~ s%^gain(\s*=.*)%calibration_gain$1%;
    $line =~ s%^bias(\s*=.*)%calibration_bias$1%;
    $line =~ s%^flat(\s*=.*)%calibration_flat$1%;
    $line =~ s%^dark(\s*=.*)%calibration_dark$1%;
    $line =~ s%^bad_pixel(\s*=.*)%calibration_bad$1%;
    $line =~ s%^coerce(\s*=.*)%calibration_coerce$1%;

    $line =~ s%^lcurve_plot_pos(\s*=.*)%position_plot$1%;
    $line =~ s%^lcurve_pos(\s*=.*)%position_targ$1%;
    $line =~ s%^\#lcurve_pos(\s*=.*)%\#position_targ$1%;
    $line =~ s%^lcurve_pos%position%;
    $line =~ s%x_yrange%x_yrange_fixed%;
    $line =~ s%y_yrange%y_yrange_fixed%;

    $line =~ s%^lcurve_plot_trans(\s*=.*)%transmission_plot$1%;
    $line =~ s%^lcurve_trans(\s*=.*)%transmission_targ$1%;
    $line =~ s%^\#lcurve_trans(\s*=.*)%\#transmission_targ$1%;
    $line =~ s%^lcurve_trans%transmission%;

    $line =~ s%^lcurve_plot_fwhm(\s*=.*)%seeing_plot$1%;
    $line =~ s%^lcurve_fwhm(\s*=.*)%seeing_targ$1%;
    $line =~ s%^\#lcurve_fwhm(\s*=.*)%\#seeing_targ$1%;
    $line =~ s%^lcurve_fwhm%seeing%;
    $line =~ s%^seeing_yscale%seeing_extend_yrange%;

    if($version eq $version5 && $lcurve_first && $line =~ /^position_targ/){
	print OUTPUT "\n# Positions\n\n";
	print OUTPUT "position_plot             = yes # yes/no to plot the position (AUTO)\n";
	print OUTPUT "position_frac             = 1   # yes/no to plot the position (AUTO)\n";
        $lcurve_first = 0;
    }

    if($version eq $version7 && $line =~ /^seeing_targ\s*=\s*(\d+)\s*(\S*)/){
	$line = "seeing_targ                 = $1 1 $2  # CCD, aperture, colour [multiple] (AUTO)\n";
    }

    print OUTPUT $line;

    if($version eq $version5 && $line =~ /^position_extend_yrange/){
	print OUTPUT "\n# Transmission plot (AUTO)\n\n";
	print OUTPUT "transmission_plot          = no       # yes/no to plot the transmission (AUTO)\n";
	print OUTPUT "transmission_frac          = 1        # Relative fraction of vertical height of plot devoted to position plot (AUTO)\n";
	print OUTPUT "transmission_ymax          = 100      # Max transmission to plot, plot will be re-drawn  if any point exceeds this value (AUTO)\n";
	print OUTPUT "transmission_targ          = 1 2 red  # CCD, aperture, colour for transmission (AUTO)\n";
	print OUTPUT "\n# Seeing\n\n";
	print OUTPUT "seeing_plot                = no      # yes/no to plot the seeing (AUTO)\n";
        print OUTPUT "seeing_frac                = 1       # Relative fraction of vertical height of plot devoted to position plot (AUTO)\n";
	print OUTPUT "seeing_scale               = 0.1557  # Arcsec/pixel (AUTO)\n";
	print OUTPUT "seeing_ymax                = 1.999   # Initial maximum seeing (AUTO)\n";
	print OUTPUT "seeing_yscale              = 1.5     # Amount to re-scale plot by if seeing > maximum (AUTO)\n";	
	print OUTPUT "seeing_targ                = 1 1 red   # CCD, aperture, colour [multiple] (AUTO)\n";
    }

    if(($version eq $version5 || $version eq $version6) && $line =~ /^calibration_gain/){
	print OUTPUT "\n# Saturation  and peppering (AUTO)\n\n"; 
	print OUTPUT "pepper                     = 33000 24000 24000  #  level at which to set error flag to indicate peppering  (AUTO)\n";
	print OUTPUT "saturation                 = 61000 61000 61000  #  level at which to set error flag to indicate saturation (AUTO)\n";
    }

}
close(OUTPUT);

exit;
