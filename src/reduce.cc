/*

!!begin
!!title   reduce
!!author  T.R. Marsh
!!created 24 Feb 2002
!!root    reduce
!!index   reduce
!!descr   reduces time-series CCD photometry
!!class   Reduction
!!class   Observing
!!css     style.css
!!head1   reduce for reducing time-series photometry

!!emph{reduce} is the main and most complex ultracam reduction program. According to
the command line inputs it can read ultracam raw data files from a server or local disk
or as a series of 'ucm' files. Most elements of the reduction are defined by an
!!ref{reduce.red}{ASCII file} that is loaded at the start.

Most of the customisation of !!emph{reduce} is done through an ASCII data file read in at the start,
although other options that one often wants to vary (and which make no difference to the final
reduced data) are entered through the command input section.  Look at !!ref{reduce.red}{reduce.red}
for an example file. See the descriptions below for what each parameter does.

!!emph{NB} It may not be possible to get reliable times or data for the first few frames.
Bad data are simply skipped, data with unreliable times are reduced
but flagged in the log file. This applies chiefly to drift mode.

!!emph{reduce} now has a variable aperture option in which the radius of the apertures is scaled
according to the FWHM. This has worked with real data for many thousands of frames on the trot,
but I know that occasionally the fit can fail. I have tried to make it robust enough to survive
a few such failures. You will need to employ the 'relaxed' approach to failures to enable this.
Basically with this enabled, if it encounters a bad fit, that aperture is lost, but it carries
on to try the next one. I hope to gradually work out how to improve the robustness of the fits,
however drop-outs are already pretty rare.

!!emph{reduce} also supports Tim Naylor's 'optimal' extraction which employs a weighted extraction
with weights proportional to the fitted profile. This is 'optimal' in terms of signal-to-noise for
background limited noise. It leads to a noticeable improvement for very weak signals. It requires
profile fitting to implement. It therefore has the same problems with poor fits, which do happen
on occasion. Failed fits usually result in a 'singular matrix' error being reported by a routine
called 'gaussj'. Optimal photometry is often not as good as straight-forward photometry for
high signal-to-noise data, so it is possible to employ different schemes for each CCD.

The fitting option works as follows: first the apertures are moved using
gaussian correlation if requested. Then the 'reference' apertures are fitted in
full generality, i.e. their positionsand shapes are fitted. Finally
non-reference apertures are fitted in position only. The shape parameters of the
reference apertures are saved for later use in any optimal extraction. The
position-only fit is fairly robust which allows this procedure to work on quite
weak signals, although there is a limit. Linked apertures are not fit at all,
which is useful in the lowest signal-to-noise cases of all. 'Reference'
apertures are set in !!ref{setaper.html}{setaper} -- one has to mark an aperture
specifically. If you attempt to run reduce without having done this the program
will abort.

<strong>Reducing data without times:</strong> the light curve plotting section in reduce at the moment requires there
to be times present. If you switch it off, the program will complain about no times but carry on reducing.

<strong>Two-pass mode:</strong> at the telescope one wants to reduce data on the fly. However there is a potential
advantage to working in two passes, the first to measure positions, the second to measure fluxes. This is useful because
it allows you to fit smooth functions to the differences in positions of your target and reference stars, which is
potentially better than determining them individually for each frame or assuming that they are constant as the aperture
link option effectively imposes. It should be realised however that it is not always good: very fast exposures
have genuine differential motion due to the atmosphere and cannot be assumed to have offset which vary smoothly
with time.

Finally, you may see warnings. Please consider these seriously. They are only warnings to allow something to be obtained but
see if you can't fix them.

!!head2 Invocation

reduce [source] rfile logfile ((url)/(file) first trim [(ncol nrow) twait tmax])/(flist)
lplot implot [skip] ((nccd) [stack] xleft xright yleft yright iset  (i1 i2)/(p1 p2))!!break

!!head2 Command line arguments

!!table

!!arg{source}{Data source, either 'l' for local, 's' for server or 'u' for ucm files. 'Local' means the
usual .xml and .dat files accessed directly. Do not add either .xml or .dat to the file name; these are assumed.
'u' means you will need to specify a list of files which should all be .ucm files (either with or without
the extension)}

!!arg{rfile}{ASCII File defining the reduction. The extension ".red" is added by default.}

!!arg{logfile}{File to record the results of the reduction. This will be an ASCII file with one line per CCD per
exposure. The format is explained inside the file. The one line lists all the results of all the apertures.
The extension ".log" is added by default.}

!!arg{url/file}{If source = 'S', this argument specifies the name of the file of
interest (it can be a full URL or just the file name in which case the program will attempt to fill in
the rest using either the environment variable ULTRACAM_DEFAULT_URL or a URL appropriate to a server
running on the local host). If source = 'L', this should be a plain file name without .xml or .dat}

!!arg{first}{If source = 'S' or 'L', this argument specifies the first frame to reduce starting from 1}

!!arg{trim}{If source = 'S' or 'L', set trim=true to enable trimming of potential junk rows and
columns of each window}

!!arg{ncol}{If trim, then this specifies the number of columns nearest the readouts of each window to be snipped
off as these can be corrupted.}

!!arg{nrow}{If trim, then this specifies the number of rows to snip off the bottom of each window as these
can be corrupted.}

!!arg{twait}{If source = 'S' or 'L', time to wait between attempts to find a new exposure (seconds).}

!!arg{tmax}{If source = 'S' or 'L', maximum time to wait before giving up (seconds). Set = 0 to quit
as soon as a frame is not found.}

!!arg{flist}{If source = 'U', this is the name of a list of ULTRACAM files to reduce. These should be arranged in
temporal order to help the reduction move from one exposure to the next successfully.}

!!arg{lplot}{true/false according to whether you want to plot light curves. There is one other implication of
setting this which is that it will require extra memory because then light curve information is stored for plotting.
The total amount needed is N*(4+60*M) where N is the number of points and M the number of apertures/CCD. This should
not be a problem unless you have a very small number of pixels (and therefore very large N) indeed.}

!!arg{hcopy}{Set this to make a hard copy plot at the end of the reduction}

!!arg{implot}{true/false according to whether you want to plot images}

!!arg{skip}{Number of frames to skip between image plots (to make up for the fact that image plotting can slow
things down a fair bit). 0 = no skipping at all. With no image plotting it acts to suppress the amount of output
to the terminal so that skip frames are skipped between each output.}

!!arg{nccd}{If image plotting enabled, CCD number to plot, 0 for all}

!!arg{stack}{Stacking direction when plotting more than on CCD. Either in 'X' or 'Y'}

!!arg{xleft xright}{If image plotting enabled, X range to plot}

!!arg{yleft yright}{If image plotting enabled, Y range to plot}

!!arg{iset}{'A', 'D' or 'P' according to whether you want to set the intensity limits
automatically (= min to max), directly or with percentiles.}

!!arg{ilow ihigh}{If iset='d', ilow and ihigh specify the intensity range to plot}

!!arg{plow phigh}{If iset='p', plow and phigh are percentiles to set the intensity range,
e.g. 10, 99}

!!table

!!head2 Reduction file

As well as the command line parameters, there are many others required to define the reduction, so many in fact that
file input is preferable, as many of them do not change much and for later checking on what was done. The !!ref{reduce.red}{file}
is in ASCII with a series of lines of the form  "option = value". Both "option" and "value" must contain no blanks. Lines
will only be read if they start with a non-blank character, with the exception of '#' which acts as a comment flag. Some
inputs are required, others are optional. These are indicated below. The option is read as the
first string before the '=' sign. The value is read as everything after
the '=' sign up to the first '#' (not escaped) or the end of the string.

In more-or-less alphabetical order (some grouping by purpose too), the options are as follows:

!!table

!!arg{abort_behaviour}{How to deal with problems. 'fussy' = give up at the first hint of
trouble, such as an aperture going awol. 'relaxed' = continue regardless.!!emph{Required}.}

!!arg{aperture_file}{Name of aperture file. This is a file of photometry apertures setup with
!!ref{setaper.html}{setaper}. !!emph{Required}.}

!!arg{aperture_reposition_mode}{The method to be used to reposition the apertures
from frame to frame. Options: !!emph{fixed} = no change. !!emph{individual} = move
each aperture separately, apertures linked to others are held at fixed offsets
relative to them. !!emph{individual_plus_tweak} =  move each aperture separately and then
tweak the offsets of any linked apertures. The idea here is that one can determine the
shift from a more reliable star while still allowing freedom of movement. !!emph{reference_plus_tweak}
is the same as individual, but before trying to reposition apertures, an initial estimate of
the shift is made from reference stars (which must be defined with !!ref{setaper.html}{setaper}).
This allows one to determine the majority of the shift from a safe target, while refining
with tighter criteria. This option is probably the most robust. In cases where the aperture is moved,
if a failure occurs the aperture is marked invalid and then its most recent correct
state is preserved for use on the next frame. !!emph{Required}.}

!!arg{aperture_positions_stable}{The way in which the apertures are repositioned can be varied slightly.
If guiding is good and the targets move only by small amounts, set this parameter to 'yes'. On the other
hand if the targets are jumping around by considerably more than the seeing width from exposure to exposure
then you may want to set it to 'no'. This allows a slightly more cavalier search for the target, which brings
with it dangers of locking onto other sources, but copes better with large shifts. The search carried out is
constrained by the following series of parameters. The old versions of 'reduce' effectively assumed that the
aperture positions were stable, so you may want to set it to 'yes' to start with at least.}

!!arg{aperture_search_half_width}{When apertures are adjusted, the program first collapses a
box centred on the first position in both X and Y directions. Its half-width is specified by this parameter, measured
in !!emph{unbinned} pixels. It should be wide enough to capture any likely movement but not
so wide that it will include lots of other stars and overwhelming extra noise. The search starts
from the last position if floatition_stable=yes, or from the maxima of the 1D collapses if
not. !!emph{Required if aperture_reposition_mode = 'individual' or 'individual_plus_tweak' or
'reference_plus_tweak' or 'cosmic_clean'=yes}.}

!!arg{aperture_search_fwhm}{When apertures are adjusted, the new position is located by
cross-correlating with a gaussian of specified FWHM measured in unbinned pixels.
!!emph{Required if aperture_reposition_mode = 'individual' or 'individual_plus_tweak' or
'reference_plus_tweak'}.}

!!arg{aperture_search_max_shift}{When apertures are adjusted, this parameter imposes a maximum
upon the shift of the aperture between its first and last position. It is measured in unbinned
pixels. !!emph{Required if aperture_reposition_mode = 'individual' or 'individual_plus_tweak' or
'reference_plus_tweak'}.}

!!arg{aperture_tweak_half_width}{If an aperture is offset from another target, then the offset
is kept fixed for the first re-location, but the individual_plus_tweak option allows the offset to be adjusted as
may be necessary to keep up with the effects of atmospheric refraction for instance. However,
the tweak parameters are defined separately in order to allow them to be more strictly defined
to reduce the chances of mis-acquiring the star. The meaning is otherwise identical to
aperture_search_half_width. It should normally be smaller than the search half width, although this
will not be checked. In the case of reference_plus_tweak this is the parameter used when making the position
measurements of non-linked, non-reference stars. i.e. it is assumed that the position shift obtained from the
reference stars is good enough that a tight search can be used for the non-reference stars.!!emph{Required if aperture_reposition_mode
= 'individual_plus_tweak' or 'reference_plus_tweak'}}

!!arg{aperture_tweak_fwhm}{See the description for aperture_tweak_half_width.
!!emph{Required if aperture_reposition_mode = 'individual_plus_tweak' or 'reference_plus_tweak'}.}

!!arg{aperture_tweak_max_shift}{See the description for aperture_tweak_half_width. This parameter
should be smaller than the equivalent search one, although this is not checked. This shift is also
applied to check the position location of the profile fits. They will be invalidated if they shift
by more than this amount. !!emph{Required if aperture_reposition_mode = 'individual_plus_tweak' or
'reference_plus_tweak' or if profile fitting is used.}.}

!!arg{aperture_twopass}{As discussed above, the normal operation during observing is to determine the aperture
positions 'on the fly'. However, once a run is complete, there is a potential gain in determining all the
positions and then fitting smooth functions to correct them. This option if set to yes enables this type of
reduction. The fitting is done to the difference between individual aperture positions and a reference position
determined from one or more bright stars because there can always be random shifts from frame to frame.
However the fitting allows for slow drifts in the relative positions of targets which may result from
refraction and colour effects. This option can only be used if the aperture repositioning is set to
'reference_plus_tweak' (and therefore you must have set reference stars in !!ref{setaper.html}{setaper}.
This option also has a potential impact in terms of memory. For each exposure a
total of 21 bytes per aperture plus 55 bytes per CCD plus 12 bytes must be stored. For a 2 aperture per CCD for 3
CCDs this amounts to 300 bytes/exposure. This may make very large frame numbers difficult to accomodate.
However, this would normally only apply to high frame rates for which differentiaal image motion due
to the atmosphere probably negates the advantage of the fitting process so I don't expect it ever to
cause problems. !!emph{Required}}

!!arg{aperture_twopass_counts}{The position fits are guarded by the shift parameters against wandering
too far off target. However, as a final check upon the positions used for the twopass case, you
can define a lower limit to the number of counts within an aperture. Set this < 0 to ignore, which
will speed things up a bit, at the risk of finding dodgy positions (which may in any case be flagged
as bad, or get kicked out during the polynomial fitting stage). This threshold is applied to the
positions measured during the first pass over the data. !!emph{Required if aperture_twopass = yes}.}

!!arg{aperture_twopass_npoly}{In two pass mode, at the end of the first pass, polynomial
fits are made to the position offsets. This is the number of coefficients used. There should
be enough to allow for trends in the data. There is no facility within reduce for examining the
fits, although this is possible post facto using the log file. In the log file the measured and
fitted positions are stored. The 'fitted positions' in this case measn the poly fit to the offset
<i>added to the reference positions</i>. Therefore to examine the fits one should read in the
fitted positions and the measured positions for an aperture, and then read in the reference positions and subtract
them from both the fitted and measured position before plotting. Remember that using npoly=1 (a constant)
is not that different to linking the apertures, except that it determines the best offset given all
of the data. !!emph{Required if aperture_twopass = yes}.}

!!arg{aperture_twopass_sigma}{In two pass mode, at the end of the first pass, polynomial
fits are made to the position offsets. This is the threshold used for rejection of bad data
during this process. This is the final check after the maximum shift tests and the minimum number
of counts for dodgy positions. !!emph{Required if aperture_twopass = yes}.}

!!arg{extraction_control}{This is the most complicated and important option. You need one 'extraction_control' line per CCD.
The idea is to give you independent control over the reduction used for each CCD. What you choose here has the
most important effect over the end results. You will not necessarily want the same for
each CCD. For instance Tim Naylor's (1998) optimal extraction is designed for cases where the variances are sky
or readout-noise limited, and is not the best for high signal-to-noise cases. You may therefore want
to employ it for the u' band only, depending upon your particular case. Each extraction control
lines line has the following parameters:
<p>
<strong>nccd aperture_type extraction_method star_scale star_min star_max inner_sky_scale inner_sky_min inner_sky_max outer_sky_scale outer_sky_min outer_sky_max
</strong>
<p>
with these meanings:
<table>

<tr valign="top">
<td><i>nccd</i><td>
<td>The CCD number, starting from 1</td>
</tr>

<tr valign="top">
<td><i>aperture_type</i><td>
<td>The aperture type, either 'fixed' or 'variable', referring to whether the aperture radii will be adjusted by profile fitting to match the
seeing or not.</td>
</tr>

<tr valign="top">
<td><i>extraction_method</i><td>
<td>The way in which the fluxes will be extracted, either 'normal' for a straight sum, or 'optimal' for
Tim Naylor's (1998) 'optimal' extarction method. In either case a rough attempt will be made to reduce pixellation
errors by making the contribution of pixels decline linearly at the edge of the aperture.
</td></tr>

<tr valign="top">
<td><i>star_scale</i><td>
<td>If aperture_type=variable, then the star aperture radius will be set to <i>star_scale</i> times the seeing. What value to choose here is
rather a difficult question, but it can be an important one. For faint stars, a good choice seems
to be about 1.5; see e.g. Naylor (1998). however, I have found that this can be quite poor for bright stars. The signal-to-noise versus this scale factor peaks at some value, which is what you want of course, and tends to drop off more steeply at low values than high ones. In the end, the only certain way to determine this number is to extract at a multiplicity of scale factors and see which is best. You should look at the parameter star_aperture_radii to see how this can be done efficiently.
</td>
</tr>

<tr valign="top">
<td><i>star_min</i><td>
<td>In the case of variable apertures, this allows you to limit the range of the star aperture's radius. e.g. you might not want it to become too small because
of pixellation problems. <i>star_min</i> is the lower limit to the star aperture's radius in unbinned pixels.</td>
</tr>

<tr valign="top">
<td><i>star_max</i><td>
<td>The maximum star aperture radius. This could be useful in cases of poor seeing where you would prefer your aperture not to expand so much
that it includes nearby stars. It has to be said of course that in such cases your photometry is never going to be that good, but used carefully
this may reduce the size of the problems.</td>
</tr>

<tr valign="top">
<td><i>inner_sky_scale</i><td>
<td>If aperture_type=variable, then the inner sky aperture radius will be set to <i>inner_sky_scale</i> times the seeing.</td>
</tr>

<tr valign="top">
<td><i>inner_sky_min</i><td>
<td>In the case of variable apertures, this allows you to limit the range of the inner_sky aperture's radius.
<i>inner_sky_min</i> is the lower limit to the inner_sky aperture's radius in unbinned pixels.</td>
</tr>

<tr valign="top">
<td><i>inner_sky_max</i><td>
<td>The maximum inner sky aperture radius. This could be useful in cases of poor seeing where you would prefer your aperture not to expand so much
that it includes nearby stars. It has to be said of course that in such cases your photometry is never going to be that good, but used carefully
this may reduce the size of any problems.</td>
</tr>

<tr valign="top">
<td><i>outer_sky_scale</i><td>
<td>If aperture_type=variable, then the outer sky aperture radius will be set to <i>outer_sky_scale</i> times the seeing. This should be more than
<i>inner_sky_scale</i>.</td>
</tr>

<tr valign="top">
<td><i>outer_sky_min</i><td>
<td>In the case of variable apertures, this allows you to limit the range of the outer_sky aperture's radius.
<i>outer_sky_min</i> is the lower limit to the outer_sky aperture's radius in unbinned pixels.</td>
</tr>

<tr valign="top">
<td><i>outer_sky_max</i><td>
<td>The maximum outer sky aperture radius. This could be useful in cases of poor seeing where you would prefer your aperture not to expand so much
that it includes nearby stars. It has to be said of course that in such cases your photometry is never going to be that good, but used carefully
this may reduce the size of the problems.</td>
</tr>
</table>
}

!!arg{star_aperture_radii}{As a time saver, if you want to carry out reductions with a variety of star radii
enter the star aperture radius or scale factor as 0, you will then need to list on another line, corresponding to
this parameter a series of radii. These will be used during reduction to extract the flux for each radius. If this option
is used, ALL CCDs will be extracted in this way and no plotting will occur. If the aperture radius type is fixed,
the numbers you give will be interpreted directly as radii in pixels. If variable they will be taken to be
scaling factors times the seeing. Use the script !!ref{splitr.html}{splitr} to split up the multiplexed log file
that results from this parameter.}

!!arg{profile_fit_method}{'gaussian' or 'moffat'. You can try these out with 'rtplot' and 'plot' to see
which is to be preferred. 'moffat'normally seems a fair bit better. !!emph{Required} if variable apertures
and/or optimal extraction are set for any CCD. 'gaussian' has the advantage in some cases of less extended
wings which may mean that it is less affected by neaarby stars. However, as I say it does not normally
give good fits and therefore one should probably use a high rejection threshold.}

!!arg{profile_fit_fwhm}{This is the first of several parameters associated with profile fits (gaussian
or moffat profiles). profile_fit_fwhm is the initial FWHM to use in either case.
!!emph{Required} if variable apertures and/or optimal extraction are set for any CCD.}

!!arg{profile_fit_hwidth}{The half-width of the region to be used when fitting a target. Should be larger
than the profile_fit_fwhm, but not so large as to include multiple targets if possible.
!!emph{Required} if variable apertures and/or optimal extraction are set for any CCD.}

!!arg{profile_fit_symm}{Yes/no for symmetric versus ellliptical profiles.
!!emph{Required} if variable apertures and/or optimal extraction are set for any CCD.}

!!arg{profile_fit_beta}{The beta parameter of the moffat fits.
!!emph{Required} if variable apertures and/or optimal extraction are set for any CCD.}

!!arg{profile_fit_sigma}{The fits can include rejection of poor pixels. This is the threshold, measured in sigma. Should not
be too small especially in the gaussian fit case. For gaussian fits for example I have noticed that if this
is set too low, the entire core of the profile can end up being rejected in favour of fitting the wings.
This is because gaussians are usually a poor approximation of stellar profiles. See comments about
the fitting method however for why one might still want to use gaussians.
!!emph{Required} if variable apertures and/or optimal extraction are set for any CCD.}

!!arg{bias}{Name of bias frame.  If not specified, no bias subtraction is carried out.}

!!arg{clobber}{[yes/no] Can the log file overwrite any previously existing file of the same name or not?
For safety say no, and the program will fall over if it finds the same name. Say yes if this irritates you.
!!emph{Required}.}

!!arg{coerce}{[yes/no] Should calibration frames be cropped to match data frames?
!!emph{Required} if any calibration has been enabled. Normally yes. You should never
coerce biases when it involves a change of binning factors. When coercing flat fields,
gain frames and readout noise frames, the programs scales to give an average rather than
a sum. It warns when it does this in case it is not what you want.}

!!arg{cosmic_clean}{Whether or not to carry out cosmic ray cleaning or not. The cosmic ray cleaning is carried out
over a box centred on the position of the aperture prior to its being tweaked over a size defined by
!!emph{aperture_search_half_width}. It proceeds by locating maxima and setting them equal to the average of the
neighbouring pixels if they are greater than cosmic_height above the average and also greater than
cosmic_ratio times the average.}

!!arg{cosmic_height}{Height relative to average of near pixels to count as a cosmic ray.}

!!arg{cosmic_ratio}{Ratio relative to average of near pixels to count as a cosmic ray.}

!!arg{cr_to_start}{yes/no. Require a carriage return to start the reduction
or dive straight in. Useful if you want to move/resize windows before everything
gets going.}

!!arg{dark}{Name of dark frame. If not specified, no dark subtraction
is carried out. The frame, if specified, should be bias subtracted. Typically this should
be the average of a large number of dark frames to reduce noise. Note that to do dark subtraction correctly,
one needs the exposure time of the bias freame used to bias subtract the dark itself. This will be stored
by !!ref{grab.html}{grab}.}

!!arg{flat}{Name of flat field frame (divide into data to correct). If
not specified, not flat fielding is carried out. The frame, if specified, should
be bias-subtracted.}

!!arg{bad_pixel}{Name of a bad pixel mask file. Ignored if not specified. This is an ordinary
ULTRACAM file set to 0 everywhere except for 'bad' pixels. These should have positive integer values which rise
according to how 'bad' they are. It is up to you, the user, to define your own levels of badness.
The program will report the highest value of any bad pixel located within each star aperture. This
can later be used to mask data affected by bad pixels. Programs !!ref{smooth.html}{smooth} and
!!ref{badgen.html}{badgen} are supplied to help with the development of bad pixel masks. In
the future I may implement some sort of cosmic ray identification in which case I will assign
pixels affected by them a value of 10. Assign your values higher or lower than this according to whether
you think they are better or worse than cosmic rays.}

!!arg{gain}{Gain, electrons/count, or if preceded by '@', the name of a gain frame
frame giving gain for every pixel. !!emph{Required}.}

!!arg{image}{yes/no for plotting of images. Clearly this will slow things
down, but often exposure times may be long enough to allow this with no
serious effect upon the reducion process. !!emph{Required}.}

!!arg{image_device}{Device to be used for plotting images. !!emph{Required.}}

!!arg{lightcurve_frac}{The weight to be given when dividing up the vertical height of the plot
between the light curves and other bits of information.}

!!arg{lightcurve_device}{Device to plot to.}

!!arg{lightcurve_xunits}{Units of X axis, 'seconds', 'minutes', 'hours' or 'days' from start of run.}

!!arg{lightcurve_max_xrange}{Maximum range in X to plot (<= 0 for everything). If positive, early points will be lost
as the run progresses.}

!!arg{lightcurve_xbuffer}{Buffer to have to right of plotted data to avoid continually changing axes. When the axes are changed,
the last point is lightcurve_buffer from the right-edge to allow room for a few points before re-plotting the axes. The whole
range is lightcurve_max_xrange + lightcurve_buffer if lightcurve_max_xrange > 0.}

!!arg{lightcurve_extend_xrange}{Factor by which to extend the X range when buffer filled (when lightcurve_max_xrange <= 0.)}

!!arg{lightcurve_linear_or_log}{Y plot linear or log (=magnitudes) for light curve}

!!arg{lightcurve_yrange_fixed}{Fix y limits on light curve or not. If not then the scale range is computed automatically to
keep all points in view.}

!!arg{lightcurve_invert}{Invert y scale or not (e.g. for magnitudes) when automatic range generation is enabled.}

!!arg{lightcurve_y1}{Lower y limit for light curve.}

!!arg{lightcurve_y2}{Upper y limit for light curve.}

!!arg{lightcurve_extend_yrange}{Extension factor for y range if lightcurve_yrange = 'no'. The amount by which the
yrange is expanded if a point exceeds displayed range and axes have to be regenerated.}

!!arg{lightcurve_targ}{A line specifying what to plot in the light curve, consisting of
the CCD number, target aperture number, comparison aperture number, offset and colour to plot the point
and error bar. e.g. '1 1 2 5 blue red'. Any number of such entries are permitted. CCD,
target and comparison aperture numbers all start from 1, except that if you specify comparison < 1, no
comparison will be used. The offset is applied after any other calulations such as division by the comparison
star and conversion to magnitudes i.e. it is in terms of what you see plotted. A number < 0 for the error bar
colour will switch the error bar plot off.}

!!arg{position_plot}{Yes/no depending whether you want to plot positional data}

!!arg{position_targ}{A line specifying what to plot in the X, Y position panels, consisting of
the CCD number, target aperture number and offset to add to distinguish different entries, a colour to plot
the point and one for any error bars. e.g. '1 1 -5. blue red'. Any number of such entries are permitted.
The positions are plotted relative to the first one and thus are naturally all close to 0. At the moment the
error bar entry is not used, but may be in the future.}

!!arg{position_frac}{The weight to be given to the position panel when assigning vertical height.}

!!arg{position_x_yrange}{Fix y limits on x position plot, 'yes' or 'no'}

!!arg{position_x_y1}{Lower limit on x position plot}

!!arg{position_x_y2}{Upper limit on x position plot}

!!arg{position_y_yrange}{Fix y limits on y position plot, 'yes' or 'no'}

!!arg{position_y_y1}{Lower limit on y position plot}

!!arg{position_y_y2}{Upper limit on y position plot}

!!arg{position_extend_yrange}{Extension factor for y range if position_x_yrange = 'no'}

!!arg{transmission_plot}{Yes/no to plot transmission data. This is computed from a constant star by scaling to its
maximum observed value and is therefore not necessarily exactly correct, but it should at least give an idea of
the variability present.}

!!arg{transmission_frac}{The weight to be given to the transmission panel when assigning vertical height.}

!!arg{transmission_ymax}{The maximum percentage 'transmission' which must be >= 100. Whenever a value is encountered
whuch exceeds this limit, the data will be replotted and scaled. Specifying a value >100 will reduce the number of
re-plots}

!!arg{transmission_targ}{The information to specify the transmission data. Can be specified multiple times.
Each line specifies the CCD and aperture to use to measure the transmission. Clearly this should be a non-variable object.
It finishes with the plotting colour to be used.
}

!!arg{seeing_plot}{Yes/no to plot seeing data. This is only possible if profile fits are made, as with
variable and optimal photometry}

!!arg{seeing_frac}{The weight to be given to the seeingn panel when assigning vertical height.}

!!arg{seeing_ymax}{The initial maximum seeing value to plot (the plots will start from 0).}

!!arg{seeing_scale}{The plate scale in arcseconds/pixel}

!!arg{seeing_yscale}{When a seeing value is encountered which exceeds the maximum of the plot, the plot will be re-scaled
by this factor which must be > 1}

!!arg{seeing_targ}{The information lines for the seeing plots which consist of a CCD number, an aperture number and colour.
The seeing is not necessarily assoiated with a single target because it is the average of the seeing measured for whatever stars
are said to be 'references', however an aperture number is used to determine the plot symbol, and typically should be set equal to
the aperture number of your brightest reference to give an accurate warning of saturation for example.}

!!arg{pepper}{For each aperture it is possible to set an error flag if any pixel exceeds the level at which peppering starts. This is
only a rough guide, but could be useful. This line consists of the estimated peppering level for each CCD. Note that this applies to
unbinned pixels. Binned pixels have correspondingly higher levels which the program computes.}

!!arg{readout}{Readout noise, or if preceded by '@', the name of readout noise
frame giving the readout noise for every pixel !!emph{in terms of variance} (counts**2).
!!emph{Required}.}

!!arg{saturation}{For each aperture it is possible to set an error flag if any pixel exceeds a level above which saturation is likely in the
sene of exceeding 65535. This line consists of the estimated saturation level of each CCD. See the 'pepper' line also.}

!!arg{sky_thresh}{RMS rejection threshold for cleaning sky. Always applied
regardless of sky estimation method as it is needed in deriving uncertainty
estimates. !!emph{Required}.}

!!arg{sky_error}{Method to use for estimating error in sky background estimate.
Options: 'variance', 'photon'. !!emph{Required}.}

!!arg{sky_method}{Method to use for sky estimation. Options: 'clipped_mean',
'median', 'mode'. !!emph{Required}.}

!!arg{terminal_output}{Mode of terminal output. Options: "none", "little", "medium", "full".
!!emph{Required}.}

!!arg{version}{Date of version of reduce which has to match the date in this
program for anything to work at all. !!emph{Required}.}

!!table

!!head2 Format of output

I have now made the format of the output uniform in the sense that each line should be the same length.
This requires estimating likely maximum and minimum limits for the various numbers and to what precision
they are needed. Unlike FORTRAN even if these are exceeded, the number is printed but it will make the
line a different length. I think I have selected these to cover all likely cases, but if the format provided
is inadequate for you (e.g. in terms of precision), or you would like more information, please let me know.
The output files are ASCII and self-documenting.

!!end

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cfloat>
#include <string>
#include <fstream>
#include <map>
#include <deque>
#include <time.h>
#include "trm/subs.h"
#include "trm/constants.h"
#include "trm/buffer2d.h"
#include "trm/input.h"
#include "trm/header.h"
#include "trm/plot.h"
#include "trm/aperture.h"
#include "trm/mccd.h"
#include "trm/frame.h"
#include "trm/ultracam.h"
#include "trm/reduce.h"

// Variables that are set by reading from the input file with read_reduce_file.
// Enclosed in a namespace for safety.

namespace Reduce {

    // Miscellaneous parameters
    bool cr_to_start;                                  // Carriage return to initiate reduction or not.
    Ultracam::Logger logger;                           // Logger object for logging to file and standard out
    ABORT_BEHAVIOUR abort_behaviour;                   // What to do when faced with a problem.
    bool  cosmic_clean;                                // cosmic ray cleaning or not
    float cosmic_height;                               // height relative to average of near pixels to count as a rat
    float cosmic_ratio;                                // ratio relative to average of near pixels to count as a rat
    std::vector<float> pepper;                              // Warning of possible peppering
    std::vector<float> saturation;                          // Warning of saturation
    bool coerce;                                       // Coerce calibration frames to match data or not
    bool gain_const;                                   // Constant gain or a gain frame
    float gain;                                        // The gain if gain_const
    Ultracam::Frame gain_frame;                        // The gain frame if !gain_const
    bool readout_const;                                // Constant readout or not
    float readout;                                     // The readout if readout_const
    Ultracam::Frame readout_frame;                     // The readout frame if !readout_const
    TERM_OUT terminal_output;                          // Terminal output mode

    // Aperture parameters
    Ultracam::Maperture aperture_master;               // Initial aperture file
    APERTURE_REPOSITION_MODE aperture_reposition_mode; // Method for updating apertures
    bool  aperture_positions_stable;                   // Is guiding good or not?
    int   aperture_search_half_width;                  // Half-width of box when re-centroiding apertures
    float aperture_search_fwhm;                        // FWHM of gaussian for re-centroiding apertures
    float aperture_search_max_shift;                   // Maximum shift when re-centroiding apertures
    int   aperture_tweak_half_width;                   // Half-width of box when tweaking offset apertures
    float aperture_tweak_fwhm;                         // FWHM of gaussian for tweaking offset apertures
    float aperture_tweak_max_shift;                    // Maximum shift when tweaking offset apertures
    bool  aperture_twopass;                            // Positions fixed with two passes or not
    float aperture_twopass_counts;                     // Minimum number of counts to be a valid aperture, two pass mode
    int   aperture_twopass_npoly;                      // Number of poly terms, two pass mode
    float aperture_twopass_sigma;                      // Rejection threshold for poly fits, two pass mode

    // Extraction and profile fitting
    std::map<int,Reduce::Extraction> extraction_control;    // Extraction control parameters for each CCD
    std::vector<float> star_radius;                         // Radii to use when extracting multiple times.
    PROFILE_FIT_METHOD profile_fit_method;             // Type of profile fitting to use
    PROFILE_FIT_METHOD extraction_weights;             // Weighting to use when extracting
    float profile_fit_fwhm;                            // Initial FWHM to use for profile fitting
    int   profile_fit_hwidth;                          // half-width of box to use when profile fitting
    bool  profile_fit_symm;                            // symmetric gaussian or not
    float profile_fit_beta;                            // initial value of Moffat exponent beta
    float profile_fit_sigma;                           // sigma rejection threshold for profile fits
    SKY_METHOD sky_method;                             // Sky estimation method
    SKY_ERROR  sky_error;                              // Sky uncertainty estimation method
    float sky_thresh;                                    // RMS clip limit for rejection of sky data

    // Calibration parameters
    bool bias;                                         // Whether there is a bias frame or not
    Ultracam::Frame bias_frame;                        // The bias frame if bias
    bool dark;                                         // Dark frame or not
    Ultracam::Frame dark_frame;                        // The dark frame if dark
    bool flat;                                         // Flat field or not
    Ultracam::Frame flat_frame;                        // The flat field if flat
    bool bad_pixel;                                    // Bad pixel frame or not
    Ultracam::Frame bad_pixel_frame;                   // Bad pixel frame if bad_pixel

    // Lightcurve plotting parameters
    std::string image_device;                               // The image display device
    float  lightcurve_frac;                            // Fraction of plot devoted to lightcurve
    std::string lightcurve_device;                          // Device for the lightcurve etc plots
    float  lightcurve_max_xrange;                      // Maximum X range (if > 0)
    X_UNITS lightcurve_xunits;                         // Units of x axis
    float  lightcurve_extend_xrange;                   // Factor by which to extend automatic X range
    bool   lightcurve_linear;                          // Linear Y scale, elese log
    bool   lightcurve_yrange_fixed;                    // Whether Y scale is fixed or not
    bool   lightcurve_invert;                          // Whether Y scale is inverted or not
    float  lightcurve_y1;                              // Lower Y limit
    float  lightcurve_y2;                              // Upper Y limit
    float  lightcurve_extend_yrange;                   // Factor by which to extend Y range if it is not fixed
    std::vector<Laps> lightcurve_targ;                      // Lightcurve target info

    // Positions
    bool  position_plot;                               // Whether to plot positions
    float position_frac;                               // Fraction of plot to devote to the positions
    std::vector<Paps> position_targ;                        // Position target info
    bool  position_x_yrange_fixed;                     // Y range on X positions fixed or not
    float position_x_y1;                               // Lower Y limit on X positions
    float position_x_y2;                               // Upper Y limit on X positions
    bool  position_y_yrange_fixed;                     // Y range on Y positions fixed or not
    float position_y_y1;                               // Lower Y limit on Y positions
    float position_y_y2;                               // Upper Y limit on Y positions
    float position_extend_yrange;                      // Factor by which to extend plot Y ranges if not fixed

    // Transmission
    bool  transmission_plot;                           // Whether to plot transmission
    float transmission_frac;                           // Fraction of vertical height to devote to the transmission
    float transmission_ymax;                           // Maximum transmission to plot
    std::vector<Taps> transmission_targ;                    // Transmission target info

    // Seeing
    bool seeing_plot;                                  // Whether to plot seeing
    float seeing_frac;                                 // Fraction of vertical height to devote to the seeing
    std::vector<Faps> seeing_targ;                          // Seeing target info
    float seeing_scale;                                // Scale in arcsec/pixel
    float seeing_ymax;                                 // Initial maximum Y value
    float seeing_extend_yrange;                        // Factor by which to extend plot Y range
};

// Function for polynomial fits needed by llsqr used for two pass mode

class Poly : public Subs::Llfunc {

public:

    // Default constructor.
    Poly() : npoly(0), xstart(0.), range(1.) {}
    ~Poly() {}

    // Sets up object
    void setup(int npoly, double xstart, double xend){
        this->npoly  = npoly;
        this->xstart = xstart;
        this->range = xend-xstart;
    }

    int get_nfunc() const {return npoly;}

    void eval(double x, double* v) const {
        v[0] = 1.;
        if(get_nfunc() > 1){
            x = -1. + 2.*(x-xstart)/range;
            double val = 1.;
            for(int i=1; i<get_nfunc(); i++){
                val *= x;
                v[i] = val;
            }
        }
    }

    // Evaluates value at x given coefficients
    double operator()(double x, const std::vector<double>& coeff) const {
        double total = coeff[0];
        if(get_nfunc() > 1){
            x = -1. + 2.*(x-xstart)/range;
            double val = 1.;
            for(int i=1; i<get_nfunc(); i++){
                val *= x;
                total += coeff[i]*val;
            }
        }
        return total;
    }

private:
    int npoly;
    double xstart, range;
};

// Structure used to store polynomial fits for each aperture in two pass mode
struct Polyfit {
    bool ok;
    std::vector<double> x;
    std::vector<double> y;
};

// ************************************************************
//
// Main program starts here!
//
// ************************************************************

int main(int argc, char* argv[]){

    using Subs::sqr;
    using Ultracam::File_Open_Error;
    using Ultracam::Ultracam_Error;
    using Ultracam::Input_Error;

    const std::string nothing = "";
    const std::string blank   = " ";
    const std::string hashb   = "# ";
    const std::string newl    = "\n";

    // For listing meanings of error codes in the output log.
    std::map<Reduce::ERROR_CODES, std::string> error_names;
    error_names[Reduce::OK] = "All OK";
    error_names[Reduce::COSMIC_RAY_DETECTED_IN_TARGET_APERTURE] = "At least one attempt was made to remove a cosmic ray from the star aperture (non-fatal)";
    error_names[Reduce::SKY_OVERLAPS_EDGE_OF_WINDOW] = "Sky annulus overlaps edge of data window (non-fatal)";
    error_names[Reduce::SKY_OVERLAPS_AND_COSMIC_RAY_DETECTED] = "Sky annulus overlaps edge of data window and at least one cosmic ray was zapped (non-fatal)";
    error_names[Reduce::SKY_NEGATIVE] = "Sky < -5: may indicate a bad bias frame which leads to underestimated errors in the photon case (non-fatal).";
    error_names[Reduce::PEPPERED] = "Counts in at least one pixel of the aperture exceeds peppering level for the CCD (non-fatal)";
    error_names[Reduce::NO_SKY] = "No sky pixels (non-fatal, although could be bad!)";
    error_names[Reduce::EXTRA_APERTURES_IGNORED] = "Aperture has extra apertures but these are ignored for optimal extraction (non-fatal)";
    error_names[Reduce::SATURATION] = "Counts in at least one pixel of the aperture exceeds saturation level for the CCD (non-fatal, but bad!)";
    error_names[Reduce::EXTRA_APERTURES_IGNORED] = "Aperture has extra apertures but these are ignored for optimal extraction (non-fatal)";
    error_names[Reduce::APERTURE_OUTSIDE_WINDOW] = "Aperture lies outside all data windows (fatal)";
    error_names[Reduce::TARGET_APERTURE_AT_EDGE_OF_WINDOW] = "Target aperture overlaps edge of data window (fatal)";
    error_names[Reduce::APERTURE_INVALID] = "Aperture has been invalidated (fatal)";
    error_names[Reduce::BLUE_IS_JUNK] = "Blue frame is junk (fatal)";

    // Buffer for formatting output with 'sprintf'
    const int NSPRINTF=1024;
    char sprint_out[NSPRINTF];

    try{

        // Construct Input object
        Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

        // Sign-in the input variables
        input.sign_in("source",   Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
        input.sign_in("rfile",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
        input.sign_in("logfile",  Subs::Input::LOCAL,  Subs::Input::PROMPT);
        input.sign_in("url",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("file",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("first",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
        input.sign_in("trim",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("ncol",     Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
        input.sign_in("nrow",     Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
        input.sign_in("twait",    Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
        input.sign_in("tmax",     Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
        input.sign_in("flist",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("lplot",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
        input.sign_in("hcopy",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
        input.sign_in("implot",   Subs::Input::LOCAL,  Subs::Input::PROMPT);
        input.sign_in("skip",     Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
        input.sign_in("nccd",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
        input.sign_in("stack",    Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
        input.sign_in("xleft",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("xright",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("ylow",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("yhigh",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("iset",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("ilow",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("ihigh",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("plow",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("phigh",    Subs::Input::GLOBAL, Subs::Input::PROMPT);

        // title for any light curve plots
        std::string light_curve_title = "Data: ";

        // Get input parameters
        char source;
        input.get_value("source", source, 'S', "sSlLuU", "data source: L(ocal), S(erver) or U(cm)?");
        source = toupper(source);

        std::string rfile;
        input.get_value("rfile", rfile, "reduce", "name of reduction file");
        rfile = Subs::filnam(rfile, ".red");

        std::string logfile;
        input.get_value("logfile", logfile, "reduce", "name of log file");
        std::string save_log = logfile;
        logfile = Subs::filnam(logfile, ".log");

        // Read reduction file
        Ultracam::read_reduce_file(rfile, logfile);

        std::string url;
        if(source == 'S'){
            input.get_value("url", url, "url", "url of file");
            light_curve_title += url;
        }else if(source == 'L'){
            input.get_value("file", url, "file", "name of local file");
            light_curve_title += url;
        }

        size_t first, nfile;
        bool trim;
        std::vector<std::string> file;
        Ultracam::Mwindow mwindow;
        Subs::Header header;
        Ultracam::ServerData serverdata;
        Ultracam::Frame data, dvar, bad, tframe;
        double twait, tmax;
        int ncol, nrow;
        if(source == 'S' || source == 'L'){
            input.get_value("first", first, size_t(1), size_t(1), size_t(9999999), "first frame to access (starting from 1)");
            input.get_value("trim", trim, true, "trim junk lower rows from windows?");
            if(trim){
                input.get_value("ncol", ncol, 0, 0, 100, "number of columns to trim from each window");
                input.get_value("nrow", nrow, 0, 0, 100, "number of rows to trim from each window");
            }
            input.get_value("twait", twait, 1., 0., 1000., "time to wait between attempts to find a frame (seconds)");
            input.get_value("tmax", tmax, 2., 0., 100000., "maximum time to wait before giving up trying to find a frame (seconds)");

            // Add extra stuff to URL if need be.
            if(url.find("http://") == std::string::npos && source == 'S'){
                char *DEFAULT_URL = getenv(Ultracam::ULTRACAM_DEFAULT_URL);
                if(DEFAULT_URL != NULL){
                    url = DEFAULT_URL + url;
                }else{
                    url = Ultracam::ULTRACAM_LOCAL_URL + url;
                }
            }else if(url.find("http://") == 0 && source == 'L'){
                throw Ultracam::Input_Error("Should not specify local file as a URL");
            }

            // Finally, read the XML file.
            parseXML(source, url, mwindow, header, serverdata, trim, ncol, nrow, twait, tmax);

            if(source == 'S'){
                Reduce::logger.logit("Server file name", url);
            }else{
                Reduce::logger.logit("Data file name", url);
            }
            Reduce::logger.logit("Starting from frame number", first);
            if(trim){
                Reduce::logger.logit("Junk data trimmed.");
            }else{
                Reduce::logger.logit("Junk data not trimmed.");
            }
            Reduce::logger.logit("");
            // Version passed as a compiler option
            Reduce::logger.logit(std::string(" ULTRACAM pipeline software version ") + VERSION);
            Reduce::logger.logit("");
            Reduce::logger.ofstr() << hashb << std::string("Information extracted from the XML headers follows:") << newl;
            Reduce::logger.logit("");
            Subs::Header::start_string = hashb;
            Reduce::logger.ofstr()<< header;
            Reduce::logger.logit("");

            data.format(mwindow, header);

        }else{

            std::string flist;
            input.get_value("flist", flist, "files.lis", "name of local file list");
            light_curve_title += flist;

            Reduce::logger.logit("Name of file list", flist);

            // Read file list
            std::string name;
            std::ifstream istr(flist.c_str());
            while(istr >> name){
                file.push_back(name);
            }
            istr.close();
            if(file.size() == 0)
                throw Input_Error("No file names loaded");

            data.read(file[0]);

            first = 0;

        }

        light_curve_title += ", log: " + save_log;

        // Carry on getting inputs
        bool lplot;
        bool implot;
        int image_skip;
        char stackdirn;
        float image_x1, image_x2, image_y1, image_y2;
        char iset;
        float ilow, ihigh, plow, phigh;
        int image_ccd;
        std::string hcopy;

        if(Reduce::star_radius.size() == 0){
            input.get_value("lplot", lplot, true, "do you want to plot light curves?");

            input.get_value("hcopy", hcopy, "null", "name of hard copy device to save final picture of light curves (null to ignore)");

            input.get_value("implot", implot, true, "do you want to plot images?");
            input.get_value("skip", image_skip, 0, 0, 10000, "number of images to skip between plots or output lines");

            if(implot){
                if(data.size() > 1){
                    input.get_value("nccd", image_ccd, int(0), int(0), int(data.size()),
                                    "CCD number to plot (0 for all)");
                }else{
                    image_ccd = 1;
                }
                if(image_ccd == 0){
                    input.get_value("stack", stackdirn, 'X', "xXyY", "stacking direction for image display (X or Y)");
                    stackdirn = toupper(stackdirn);
                }
                if(image_ccd == 0){
                    image_x2 = data.nxtot()+0.5;
                    image_y2 = data.nytot()+0.5;
                }else{
                    image_x2 = data[image_ccd-1].nxtot()+0.5;
                    image_y2 = data[image_ccd-1].nytot()+0.5;
                }
                input.get_value("xleft",  image_x1, 0.5f, 0.5f, image_x2, "left X limit of plot");
                input.get_value("xright", image_x2, image_x2,   0.5f, image_x2, "right X limit of plot");
                input.get_value("ylow",   image_y1, 0.5f, 0.5f, image_y2, "lower Y limit of plot");
                input.get_value("yhigh",  image_y2, image_y2, 0.5f, image_y2, "upper Y limit of plot");

                input.get_value("iset", iset, 'a', "aAdDpP", "set intensity a(utomatically), d(irectly) or with p(ercentiles)?");
                iset = toupper(iset);
                if(iset == 'D'){
                    input.get_value("ilow",   ilow,  0.f, -FLT_MAX, FLT_MAX, "lower intensity limit");
                    input.get_value("ihigh",  ihigh, 1000.f, -FLT_MAX, FLT_MAX, "upper intensity limit");
                }else if(iset == 'P'){
                    input.get_value("plow",   plow,  1.f, 0.f, 100.f,  "lower intensity limit percentile");
                    input.get_value("phigh",  phigh, 99.f, 0.f, 100.f, "upper intensity limit percentile");
                    plow  /= 100.;
                    phigh /= 100.;
                }
            }
        }else{
            image_skip = 0;
            lplot      = false;
            implot     = false;
            hcopy      = "null";
        }

        // All inputs read, save them as defaults
        input.save();

        Ultracam::Maperture aperture;

        Subs::Header::Hnode *hnode; // header pointer

        // Need exposure time from dark frame in order to scale it.
        float dark_expose, dark_bias_expose = 0;
        if(Reduce::dark){
            Reduce::dark_frame["Exposure"]->get_value(dark_expose);
            if(dark_expose <= 0.f)
                throw Input_Error("Exposure time in dark frame must be > 0.");

            hnode = Reduce::dark_frame.find("Bias_exposure");
            if(hnode->has_data())
                hnode->value->get_value(dark_bias_expose);
            else
                std::cerr << "WARNING: cannot find exposure time of bias used for dark frame; will assume = 0" << std::endl;
            if(dark_bias_expose > dark_expose)
                throw Input_Error("Bias used for dark has exposure time > dark itself");
        }

        // Also need exposure time of bias frame for proper dark subtraction
        float bias_expose = 0;
        if(Reduce::bias)
            Reduce::bias_frame["Exposure"]->get_value(bias_expose);

        // Write header info into log file
        Reduce::logger.ofstr() << hashb << newl;
        Reduce::logger.ofstr() << hashb << std::string("For each CCD of each frame reduced, the following information is printed:") << newl;
        Reduce::logger.ofstr() << hashb << newl;
        Reduce::logger.ofstr() << hashb << std::string("name/number mjd flag expose ccd fwhm beta [naper x y xm ym exm eym counts sigma sky nsky nrej worst error_flag]*num_aper") << newl;
        Reduce::logger.ofstr() << hashb << newl;
        Reduce::logger.ofstr() << hashb << std::string("where 'name/number' is either the file name for ucm file list data or the frame number for data from the .dat files,") << newl;
        Reduce::logger.ofstr() << hashb << std::string("'mjd' is the Modified Julian Date (UTC) at the centre of the exposure. MJD = JD-2400000.5, no correction for light travel") << newl;
        Reduce::logger.ofstr() << hashb << std::string("etc is made on the basis that the key thing is have a well-understood & correct time. 'flag' is an indication of whether") << newl;
        Reduce::logger.ofstr() << hashb << std::string("the time is thought to be reliable or not (1=OK,0=NOK). 'expose' is the exposure time in seconds. 'ccd' is the ccd number") << newl;
        Reduce::logger.ofstr() << hashb << std::string("(1=red,2=green,3=uv). 'fwhm' is the fitted FWHM, =0 if no fit made. 'beta' is the fitted Moffat exponent, 0 if no Moffat fit is made.") << newl;
        Reduce::logger.ofstr() << hashb << std::string("'naper' = aperture number, ('x','y') = aperture position actually used, ('xm', 'ym') = the measured aperture position (0,0) for") << newl;
        Reduce::logger.ofstr() << hashb << std::string("invalid and/or linked apertures, ('exm', 'eym') = the 1-sigma uncertainty in the measured aperture position (-1,-1) for invalid") << newl;
        Reduce::logger.ofstr() << hashb << std::string("and/or linked apertures. 'counts' = estimated target counts, 'sigma' = 1-sigma uncertainty. 'sky' = sky background in counts,") << newl;
        Reduce::logger.ofstr() << hashb << std::string("'nsky' = the number of sky pixels available, 'nrej' = number of sky pixels rejected, 'worst' is the value of the worst bad pixel") << newl;
        Reduce::logger.ofstr() << hashb << std::string("within the star aperture (0 is OK) and 'error_flag' is one of a number of codes with the following possibilities:") << newl;
        Reduce::logger.ofstr() << hashb << newl;

        for(std::map<Reduce::ERROR_CODES,std::string>::const_iterator mi=error_names.begin(); mi!=error_names.end(); mi++)
            Reduce::logger.ofstr() << hashb << std::string("Error code = ") << mi->first << std::string(", meaning: ") << mi->second << newl;

        Reduce::logger.ofstr() << hashb << newl;
        Reduce::logger.ofstr() << hashb << std::string("For the fatal codes, 0. -1 0. 0 will be printed in place"
                                                       " of \"counts sigma sky nrej\"") << newl;
        Reduce::logger.ofstr() << hashb << std::string("The square bracketed section is repeated for each aperture.") << newl;
        Reduce::logger.ofstr() << hashb << newl;

        // Now get started. Open plot devices and optionally stop so that user can
        // adjust their position.
        Subs::Plot image_plot;
        if(implot) image_plot.open(Reduce::image_device);

        Subs::Plot lcurve_plot, profile_fit_plot;
        if(lplot) lcurve_plot.open(Reduce::lightcurve_device);

        if(Reduce::cr_to_start){
            std::cout << "Hit <CR> to start reduction: " << std::flush;
            std::cin.ignore(1,'\n');
        }

        // Declare the objects required for the reduction
        bool reliable = false; // Is the time reliable?
        bool reliable_blue = false; // Is the u-band time reliable?
        bool blue_is_bad = false; // is the blue frame junk?
        Subs::Time ut_date, ut_date_blue, ttime(1,Subs::Date::Jan,1999); // the time and a check time
        float expose; // exposure time
        float expose_blue; // blue exposure time
        bool first_file, has_a_time; // are we on the first data file? do we have a time?
        std::vector<Reduce::Meanshape> shape; // Vector of shape parameters for each CCD.
        std::vector<std::vector<Ultracam::Fxy> > errors; // Vectors of position uncertainties for all apertures
        std::vector<Reduce::Twopass>  twopass; // Structure for storage of position offset information in two-pass case
        Reduce::Twopass twop; // Temporary element for storage of position offset information in two-pass case
        // Vectors of polynomial coefficients for two-pass case. Consists of polynomials in x and y for every
        // aperture of every CCD (potentially)
        std::vector<std::vector<Polyfit> > vpoly;
        Poly poly; // polynomial function object

        int maxpass = 1; // Maximum number of passes through data, default value
        // Define the twopass structure, if needed.
        if(Reduce::aperture_twopass){
            twop.shape.resize(Reduce::aperture_master.size());
            twop.ref_pos.resize(Reduce::aperture_master.size());
            twop.ref_valid.resize(Reduce::aperture_master.size());
            twop.offset.resize(Reduce::aperture_master.size());
            vpoly.resize(Reduce::aperture_master.size());
            for(size_t nccd=0; nccd<Reduce::aperture_master.size(); nccd++){
                twop.offset[nccd].resize(Reduce::aperture_master[nccd].size());
                vpoly[nccd].resize(Reduce::aperture_master[nccd].size());
            }
            maxpass = 2;
        }

        // Buffers for storage of zapped pixels
        std::vector<std::vector<std::vector<std::pair<int,int> > > > zapped;
        for(size_t nccd=0; nccd<Reduce::aperture_master.size(); nccd++)
            zapped.push_back(std::vector<std::vector<std::pair<int,int> > >(Reduce::aperture_master[nccd].size()));

        Reduce::ERROR_CODES ecode;
        float counts, sigma, sky;
        int nrej, nsky, worst;

        // buffer for storing results for one frame
        std::vector<std::vector<Reduce::Point> > all_ccds;

        // Finally get going
        // maxpass passes through the data will occur.
        for(int npass=1; npass<=maxpass; npass++){

            nfile      = first;
            first_file = true;
            if(Reduce::aperture_twopass){
                if(npass == 1)
                    std::cout << "Carrying out first pass of two pass mode to determine the positions" << std::endl;
                else
                    std::cout << "Carrying out second pass of two pass mode to extract fluxes" << std::endl;

            }
            int nexp = 0;

            for(;;){

                // Data input section
                if(source == 'S' || source == 'L'){

                    // Carry on reading until data & time are OK
                    bool get_ok, reset = (npass == 2 && nfile == first);
                    for(;;){
                        if(!(get_ok = Ultracam::get_server_frame(source, url, data, serverdata, nfile, twait, tmax, reset))) break;
                        ut_date       = data["UT_date"]->get_time();
                        reliable      = data["Frame.reliable"]->get_bool();
                        ut_date_blue  = serverdata.nblue > 1 ? data["UT_date_blue"]->get_time() : ut_date;
                        reliable_blue = serverdata.nblue > 1 ? data["Frame.reliable_blue"]->get_bool() : reliable;

                        if(serverdata.is_junk(nfile)){
                            std::cerr << "Skipping file " << nfile << " which has junk data" << newl;
                            nfile++;
                        }else if(ut_date < ttime){
                            std::cerr << "Skipping file " << nfile << " which has junk time = " << ut_date << newl;
                            nfile++;
                        }else{
                            break;
                        }
                    }
                    if(!get_ok) break;

                    if((nfile - first) % (image_skip + 1) == 0 &&
                       (Reduce::terminal_output == Reduce::FULL || Reduce::terminal_output == Reduce::MEDIUM || Reduce::terminal_output == Reduce::LITTLE)){
                        if(Reduce::aperture_twopass){
                            if(npass == 1){
                                std::cout << "Computing positions for frame number " << nfile << ", time = " << data["UT_date"]->get_time() << std::endl;
                            }else{
                                std::cout << "Extracting fluxes from frame number " << nfile << ", time = " << data["UT_date"]->get_time() << std::endl;
                            }
                        }else{
                            std::cout << "Processing frame number " << nfile << ", time = " << data["UT_date"]->get_time() << std::endl;
                        }
                    }
                    has_a_time = true;

                }else{

                    if(nfile == file.size()) break;
                    do{
                        data.read(file[nfile]);
                        // Find the time associated with the file
                        hnode = data.find("UT_date");
                        if(hnode->has_data()){
                            hnode->value->get_value(ut_date);
                            has_a_time = true;
                        }else if(!lplot && hcopy == "null"){
                            std::cout << "No header item 'UT_date' found in file " << file[nfile] << ". Will just print time = file number to the log file but continue to reduce" << std::endl;
                            has_a_time = false;
                        }else{
                            throw Ultracam::Ultracam_Error("Failed to find header item 'UT_date' in file " + file[nfile]);
                        }

                        if(has_a_time && ut_date < ttime) nfile++;
                    }while(nfile < file.size() && has_a_time && ut_date < ttime);
                    if(nfile == file.size()) break;

                    // time assumed reliable unless proven otherwise. This ensures that
                    // data read using fits2ucm can be reduced.
                    hnode = data.find("Frame.reliable");
                    reliable = !hnode->has_data() || hnode->value->get_bool();

                    if(has_a_time){

                        if((nfile - first) % (image_skip + 1) == 0 &&
                           (Reduce::terminal_output == Reduce::FULL || Reduce::terminal_output == Reduce::MEDIUM || Reduce::terminal_output == Reduce::LITTLE)){
                            if(Reduce::aperture_twopass){
                                if(npass == 1){
                                    std::cout << "Computing positions for file = " << file[nfile] << ", time = " << data["UT_date"]->get_time() << std::endl;
                                }else{
                                    std::cout << "Extracting fluxes from file = " << file[nfile] << ", time = " << data["UT_date"]->get_time() << std::endl;
                                }
                            }else{
                                std::cout << "Processing file = " << file[nfile] << ", time = " << data["UT_date"]->get_time() << std::endl;
                            }
                        }

                        // Check case of u-band co-adds
                        if((hnode = data.find("Instrument.nblue"))->has_data() && hnode->value->get_int() > 1){
                            hnode = data.find("UT_date_blue");
                            if(hnode->has_data()){
                                hnode->value->get_value(ut_date_blue);
                                hnode = data.find("Frame.reliable_blue");
                                if(hnode->has_data()){
                                    reliable_blue = hnode->value->get_bool();
                                }else{
                                    throw Ultracam_Error("Found UT_date_blue but no corresponding Frame.reliable_blue in file " + file[nfile]);
                                }
                            }else{
                                throw Ultracam_Error("Instrument.nblue > 1 indicated u-band co-add option but no UT_date_blue was found in file " + file[nfile]);
                            }
                        }else{
                            ut_date_blue  = ut_date;
                            reliable_blue = reliable;
                        }

                    }else{
                        if((nfile - first) % (image_skip + 1) == 0 &&
                           (Reduce::terminal_output == Reduce::FULL || Reduce::terminal_output == Reduce::MEDIUM || Reduce::terminal_output == Reduce::LITTLE)){
                            if(Reduce::aperture_twopass){
                                if(npass == 1){
                                    std::cout << "Computing positions for file = " << file[nfile] << std::endl;
                                }else{
                                    std::cout << "Extracting fluxes from file = " << file[nfile]  << std::endl;
                                }
                            }else{
                                std::cout << "Processing file = " << file[nfile] << std::endl;
                            }
                        }
                    }
                }

                // Data now read in, get exposure times
                hnode = data.find("Exposure");
                if(hnode->has_data()){
                    hnode->value->get_value(expose);
                }else{
                    if(Reduce::abort_behaviour == Reduce::FUSSY)
                        throw Ultracam::Ultracam_Error("Fussy mode: failed to find header item 'Exposure' in file " + file[nfile]);
                    std::cerr << "WARNING: failed to find header item 'Exposure' in file " << file[nfile] << ", will set = 0" << std::endl;
                    expose = 0.;
                }

                if((hnode = data.find("Instrument.nblue"))->has_data() && hnode->value->get_int() > 1){
                    hnode = data.find("Exposure_blue");
                    if(hnode->has_data()){
                        hnode->value->get_value(expose_blue);
                    }else{
                        if(Reduce::abort_behaviour == Reduce::FUSSY)
                            throw Ultracam::Ultracam_Error("Fussy mode: failed to find header item 'Exposure_blue' in file " + file[nfile]);
                        std::cerr << "WARNING: failed to find header item 'Exposure_blue' in file " << file[nfile] << ", will set = 0" << std::endl;
                        expose_blue = 0.;
                    }
                }else{
                    expose_blue = expose;
                }
                hnode = data.find("Frame.bad_blue");
                blue_is_bad = (hnode->has_data() && hnode->value->get_bool());

                // Check numbers of CCDs
                if(Reduce::bias && data.size() != Reduce::bias_frame.size())
                    throw Ultracam_Error("Conflicting CCD numbers between data and bias files.");

                if(Reduce::dark && data.size() != Reduce::dark_frame.size())
                    throw Ultracam_Error("Conflicting CCD numbers between data and dark files.");

                if(Reduce::flat && data.size() != Reduce::flat_frame.size())
                    throw Ultracam_Error("Conflicting CCD numbers between data and flat files.");

                if(Reduce::bad_pixel && data.size() != Reduce::bad_pixel_frame.size())
                    throw Ultracam_Error("Conflicting CCD numbers between data and bad pixel files.");

                if(!Reduce::gain_const && data.size() != Reduce::gain_frame.size())
                    throw Ultracam_Error("Conflicting CCD numbers between data and gain files.");

                if(!Reduce::readout_const && data.size() != Reduce::readout_frame.size())
                    throw Ultracam_Error("Conflicting CCD numbers between data and readout files.");

                if(data.size() != Reduce::aperture_master.size())
                    throw Ultracam_Error("Conflicting CCD numbers between data and aperture files.");

                // If first file, crop bias, dark, flat etc if wanted.
                if(first_file){

                    int xbin_data = data[0][0].xbin();
                    int ybin_data = data[0][0].ybin();

                    if(Reduce::coerce){

                        if(Reduce::bias){
                            if(Reduce::bias_frame[0][0].xbin() != xbin_data ||
                               Reduce::bias_frame[0][0].ybin() != ybin_data)
                                throw Ultracam_Error("Binning factors of bias and data fail to match; coercion not allowed in this case.");

                            Reduce::bias_frame.crop(data);
                        }

                        if(Reduce::dark) Reduce::dark_frame.crop(data);

                        if(Reduce::flat){

                            int xbin_flat = Reduce::flat_frame[0][0].xbin();
                            int ybin_flat = Reduce::flat_frame[0][0].ybin();

                            Reduce::flat_frame.crop(data);
                            if(xbin_flat*ybin_flat != xbin_data*ybin_data){
                                std::cerr << "\nWarning: the data and flat-field binning factors do no match and so after it has been re-formatted," << std::endl;
                                std::cerr << "Warning: the flat-field will be scaled by the ratio of pixel areas. If you do not want this, you should" << std::endl;
                                std::cerr << "Warning: prepare a correctly binned version by hand.\n" << std::endl;
                                float scale = float(xbin_data*ybin_data)/(xbin_flat*ybin_flat);
                                Reduce::flat_frame /= scale;
                            }
                        }

                        if(Reduce::bad_pixel) Reduce::bad_pixel_frame.crop(data);

                        if(!Reduce::gain_const){
                            int xbin_gain = Reduce::gain_frame[0][0].xbin();
                            int ybin_gain = Reduce::gain_frame[0][0].ybin();

                            Reduce::gain_frame.crop(data);

                            if(xbin_gain*ybin_gain != xbin_data*ybin_data){
                                std::cerr << "\nWarning: the data and gain frame binning factors do no match and so after it has been re-formatted," << std::endl;
                                std::cerr << "Warning: the gain frame will be scaled by the ratio of pixel areas. If you do not want this, you should" << std::endl;
                                std::cerr << "Warning: prepare a correctly binned version by hand.\n" << std::endl;
                                float scale = float(xbin_data*ybin_data)/(xbin_gain*ybin_gain);
                                Reduce::gain_frame /= scale;
                            }
                        }

                        if(!Reduce::readout_const){
                            int xbin_read = Reduce::readout_frame[0][0].xbin();
                            int ybin_read = Reduce::readout_frame[0][0].ybin();

                            Reduce::readout_frame.crop(data);

                            if(xbin_read*ybin_read != xbin_data*ybin_data){
                                std::cerr << "\nWarning: the data and readout frame binning factors do no match and so after it has been re-formatted," << std::endl;
                                std::cerr << "Warning: the readout noise frame will be scaled by the ratio of pixel areas. If you do not want this, you should" << std::endl;
                                std::cerr << "Warning: prepare a correctly binned version by hand.\n" << std::endl;
                                float scale = float(xbin_data*ybin_data)/(xbin_read*ybin_read);
                                Reduce::readout_frame /= scale;
                            }
                        }
                    }

                    if(Reduce::gain_const){
                        Reduce::gain_frame = data;
                        Reduce::gain_frame = Reduce::gain;
                    }

                    if(Reduce::readout_const){
                        Reduce::readout_frame = data;
                        Reduce::readout_frame = Reduce::readout*Reduce::readout;
                    }

                    // initialise format of bad pixels and bias frame if there is not one
                    bad = data;

                    if(!Reduce::bias){
                        Reduce::bias_frame = data;
                        Reduce::bias_frame = 0;
                    }

                }

                // Check frame formats
                if(Reduce::bias && data != Reduce::bias_frame)
                    throw Ultracam_Error("bias frame does not have same format as data frame");

                if(Reduce::dark && data != Reduce::dark_frame)
                    throw Ultracam_Error("dark frame does not have same format as data frame");

                if(Reduce::flat && data != Reduce::flat_frame)
                    throw Ultracam_Error("flat frame does not have same format as data frame");

                if(Reduce::bad_pixel && data != Reduce::bad_pixel_frame)
                    throw Ultracam_Error("flat frame does not have same format as data frame");

                if(data != Reduce::readout_frame)
                    throw Ultracam_Error("readout frame does not have same format as data frame");

                if(data != Reduce::gain_frame)
                    throw Ultracam_Error("gain frame does not have same format as data frame");

                // Now have data read in and calibration files in correct form, so apply calibration
                if(Reduce::bias) data -= Reduce::bias_frame;

                // Define variance frame after bias subtraction but before dark subtraction
                dvar  = data;
                dvar.max(0);
                dvar /= Reduce::gain_frame;
                dvar += Reduce::readout_frame;

                // Have to care here because of u'-band co-add possibility. Note that we also have to
                // account for the dark counts implicitly removed by bias subtraction.
                if(Reduce::dark){
                    if(expose == expose_blue){
                        data -= (expose-bias_expose)/(dark_expose-dark_bias_expose)*Reduce::dark_frame;
                    }else{
                        tframe = Reduce::dark_frame;
                        for(size_t i=0; i<tframe.size(); i++){
                            if(i != 2)
                                tframe[i] *= (expose-bias_expose)/(dark_expose-dark_bias_expose);
                            else
                                tframe[i] *= (expose_blue-bias_expose)/(dark_expose-dark_bias_expose);
                        }
                        data -= tframe;
                    }
                }

                if(Reduce::flat){
                    data /= Reduce::flat_frame;
                    dvar /= Reduce::flat_frame;
                    dvar /= Reduce::flat_frame;
                }

                // Bad pixels initialised to zero or the input frame if there is one.
                if(Reduce::bad_pixel)
                    bad = Reduce::bad_pixel_frame;
                else
                    bad = 0;

                // Update the apertures
                if(npass == 1){

                    Ultracam::rejig_apertures(data, dvar, profile_fit_plot, blue_is_bad, aperture, shape, errors);

                    // Compute and store the reference positions and offsets
                    if(Reduce::aperture_twopass){

                        if(Reduce::aperture_twopass_counts > 0.f){

                            // Apply minimum count threshold with normal extraction
                            for(size_t nccd=0; nccd<data.size(); nccd++){
                                if((!blue_is_bad || nccd != 2) && Reduce::extraction_control.find(nccd) != Reduce::extraction_control.end()){
                                    for(size_t naper=0; naper<aperture[nccd].size(); naper++){
                                        Ultracam::Aperture &app = aperture[nccd][naper];
                                        if(app.valid() && !app.linked()){
                                            Ultracam::extract_flux(data[nccd], dvar[nccd], bad[nccd], Reduce::gain_frame[nccd], Reduce::bias_frame[nccd],
                                                                   app, Reduce::sky_method, Reduce::sky_thresh, Reduce::sky_error,
                                                                   Reduce::NORMAL, zapped[nccd][naper], shape[nccd], Reduce::pepper[nccd],
                                                                   Reduce::saturation[nccd], counts, sigma, sky, nsky, nrej, ecode, worst);
                                            if(counts < Reduce::aperture_twopass_counts){
                                                app.set_valid(false);
                                                if(Reduce::terminal_output == Reduce::FULL){
                                                    std::cerr << "CCD " << nccd+1 << ", aperture " << naper + 1 << " has " << counts
                                                              << " counts which is less than the limit of " << Reduce::aperture_twopass_counts << std::endl;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Set the time
                        if(has_a_time)
                            twop.time = ut_date.mjd();
                        else
                            twop.time = double(nfile+1);

                        twop.shape = shape;

                        for(size_t nccd=0; nccd<aperture.size(); nccd++){

                            if((!blue_is_bad || nccd != 2) && Reduce::extraction_control.find(nccd) != Reduce::extraction_control.end()){

                                // First compute reference position
                                bool all_found = true;
                                int nref = 0;
                                float xref = 0.f, yref = 0.f;
                                for(size_t naper=0; naper<aperture[nccd].size(); naper++){
                                    Ultracam::Aperture &app = aperture[nccd][naper];
                                    if(app.ref()){
                                        if(app.valid()){
                                            nref++;
                                            xref += app.xref();
                                            yref += app.yref();
                                        }else{
                                            all_found = false;
                                            break;
                                        }
                                    }
                                }

                                if(all_found){
                                    // OK, valid reference position found
                                    xref /= nref;
                                    yref /= nref;
                                    twop.ref_pos[nccd]   = Ultracam::Fxy(xref, yref);
                                    twop.ref_valid[nccd] = true;

                                    // Compute offsets for all valid and unlinked apertures
                                    for(size_t naper=0; naper<aperture[nccd].size(); naper++){
                                        Ultracam::Aperture &app = aperture[nccd][naper];
                                        Reduce::Offset &off = twop.offset[nccd][naper];
                                        if(app.valid() && !app.linked()){
                                            off.ok  = true;
                                            off.x   = app.xref() - xref;
                                            off.y   = app.yref() - yref;
                                            off.xe  = errors[nccd][naper].x;
                                            off.ye  = errors[nccd][naper].y;
                                        }else{
                                            off.ok = false;
                                        }
                                    }
                                }else{
                                    twop.ref_valid[nccd] = false;
                                }
                            }
                        }

                        // Finally store the result
                        twopass.push_back(twop);
                    }

                }else{

                    // Second pass of the two-pass process.
                    // Need to re-define apertures and shape parameters

                    double xtime;
                    if(has_a_time)
                        xtime = ut_date.mjd();
                    else
                        xtime = double(nfile+1);

                    shape = twopass[nexp].shape;

                    for(size_t nccd=0; nccd<aperture.size(); nccd++){

                        if((!blue_is_bad || nccd != 2) && Reduce::extraction_control.find(nccd) != Reduce::extraction_control.end()){

                            // Recompute aperture radii if not fixed
                            float rstar = 0, rsky1 = 0, rsky2 = 0;
                            if(Reduce::extraction_control[nccd].aperture_type == Reduce::VARIABLE){
                                rstar = Subs::clamp(Reduce::extraction_control[nccd].star_min,
                                                    float(Reduce::extraction_control[nccd].star_scale*shape[nccd].fwhm),
                                                    Reduce::extraction_control[nccd].star_max);
                                rsky1 = Subs::clamp(Reduce::extraction_control[nccd].inner_sky_min,
                                                    float(Reduce::extraction_control[nccd].inner_sky_scale*shape[nccd].fwhm),
                                                    Reduce::extraction_control[nccd].inner_sky_max);
                                rsky2 = Subs::clamp(Reduce::extraction_control[nccd].outer_sky_min,
                                                    float(Reduce::extraction_control[nccd].outer_sky_scale*shape[nccd].fwhm),
                                                    Reduce::extraction_control[nccd].outer_sky_max);
                                if(rsky1 >= rsky2)
                                    throw Ultracam_Error("reduce: inner radius of sky annulus >= outer; should not happen. "
                                                         "Probably need to change extraction_control parameters");
                            }

                            if(twopass[nexp].ref_valid[nccd]){
                                for(size_t naper=0; naper<aperture[nccd].size(); naper++){

                                    Ultracam::Aperture &app = aperture[nccd][naper];
                                    app.set_valid(true);

                                    // Adjust aperture radii
                                    if(Reduce::extraction_control[nccd].aperture_type == Reduce::VARIABLE)
                                        app.set_radii(rstar, rsky1, rsky2);

                                    if(!app.linked()){
                                        // Use poly fits to fix the positions of all apertures.
                                        if(vpoly[nccd][naper].ok){
                                            float xref = twopass[nexp].ref_pos[nccd].x + poly(xtime, vpoly[nccd][naper].x);
                                            float yref = twopass[nexp].ref_pos[nccd].y + poly(xtime, vpoly[nccd][naper].y);
                                            float xold = app.xref();
                                            float yold = app.yref();
                                            app.set_xref(xref);
                                            app.set_yref(yref);

                                            // Update positions of any apertures linked to the current one
                                            for(size_t naper1=0; naper1<aperture[nccd].size(); naper1++){
                                                Ultracam::Aperture &app1 = aperture[nccd][naper1];
                                                if(app1.linked() && app1.xref() == xold && app1.yref() == yold){
                                                    app1.set_xref(xref);
                                                    app1.set_yref(yref);
                                                }
                                            }
                                        }
                                    }
                                }
                            }else{
                                for(size_t naper=0; naper<aperture[nccd].size(); naper++)
                                    aperture[nccd][naper].set_valid(false);
                            }
                        }
                    }
                }

                // Cosmic ray cleaning section - needs fixing.

                if(Reduce::cosmic_clean){

                    std::cerr << "Sorry! Cosmic ray cleaning currently disabled as it is not reliable" << std::endl;
                    std::cerr << "If you think cosmic rays are causing you problems, let me know and" << std::endl;
                    std::cerr << "I will have a go at fixing it (t.r.marsh@warwick.ac.uk)" << std::endl;

                    /*
                      for(int nccd=0; nccd<data.size(); nccd++){

                      if((!blue_is_bad || nccd != 2) && Reduce::extraction_control.find(nccd) != Reduce::extraction_control.end()){

                      // Loop over apertures

                      for(int naper=0; naper<aperture[nccd].size(); naper++){

                      app = &aperture[nccd][naper];

                      if(app->valid()){

                      if(data[nccd].closest(app->xpos(), app->ypos(), datp)){

                      // window found for this aperture, but need to check that
                      // star aperture is fully enclosed by it
                      if(
                      datp->left()   < app->xpos()-app->rstar() &&
                      datp->bottom() < app->ypos()-app->rstar() &&
                      datp->right()  > app->xpos()+app->rstar() &&
                      datp->top()    > app->ypos()+app->rstar()){

                      float xstart   = datp->xcomp(app->xpos());
                      float ystart   = datp->ycomp(app->ypos());
                      int   hwidth_x = Reduce::aperture_search_half_width/datp->xbin();
                      int   hwidth_y = Reduce::aperture_search_half_width/datp->ybin();

                      try{

                      // Clean square-ish area around current position of aperture. Note that this
                      // will include the final position of the aperture too. Cleaning should reduce
                      // the chances of mistakes in the aperture refining section.

                      Ultracam::zapcosmic(*datp, datp->nx(), datp->ny(), hwidth_x, hwidth_y, xstart, ystart,
                      Reduce::cosmic_height, Reduce::cosmic_ratio, zapped[nccd][naper]);


                      }
                      catch(Ultracam_Error err){
                      app->set_valid(false);
                      if(Reduce::abort_behaviour == Reduce::FUSSY) throw err;
                      }
                      }
                      }
                      }
                      }
                      }
                      }
                    */
                }


                // Display as an image if wanted
                if(implot && (nfile - first) % (image_skip + 1) == 0){

                    image_plot.focus();

                    //		    Ultracam::def_col();

                    // Plot images
                    Ultracam::plot_images(data, image_x1, image_x2, image_y1, image_y2, image_ccd == 0, stackdirn,
                                          iset, ilow, ihigh, plow, phigh, true, " ", image_ccd - 1, false);

                    // Plot apertures
                    Ultracam::plot_apers(aperture, image_x1, image_x2, image_y1, image_y2, image_ccd == 0,
                                         stackdirn, image_ccd - 1);
                }

                // Extraction stage. Loop over CCDs
                if(npass == maxpass){

                    all_ccds.resize(data.size());

                    // Loop over multiple radii
                    // Results for one time will be written out in order
                    // CCD 1, radius 1
                    // CCD 2, radius 1
                    // CCD 3, radius 1
                    // CCD 1, radius 2
                    // etc
                    size_t nradius = 0;

                    do {

                        for(size_t nccd=0; nccd<data.size(); nccd++){

                            if(Reduce::extraction_control.find(nccd) != Reduce::extraction_control.end()){

                                all_ccds[nccd] = std::vector<Reduce::Point>(aperture[nccd].size());

                                bool time_ok;
                                // I/O -- output standard string that applies to a CCD before the aperture info appears.
                                if(source == 'S' || source == 'L'){

                                    time_ok = reliable;

                                    // Format using 'sprintf' for reliability.
                                    if(nccd != 2){
                                        sprintf(sprint_out, "%8i %16.10f %1i %9.6f %1i %7.3f %7.3f",
                                                int(nfile), ut_date.mjd(), reliable, expose, int(nccd+1), shape[nccd].fwhm, shape[nccd].beta);
                                    }else{
                                        sprintf(sprint_out, "%8i %16.10f %1i %9.6f %1i %7.3f %7.3f",
                                                int(nfile), ut_date_blue.mjd(), reliable, expose_blue, int(nccd+1),
                                                shape[nccd].fwhm, shape[nccd].beta);
                                    }

                                    Reduce::logger.ofstr() << sprint_out;

                                    if(Reduce::terminal_output == Reduce::FULL){
                                        std::cout << nfile << ", CCD " << nccd+1;
                                    }else if(Reduce::terminal_output == Reduce::MEDIUM){
                                        std::cout << nfile << " " << nccd+1;
                                    }

                                }else{

                                    double ptime;
                                    if(has_a_time)
                                        ptime = nccd != 2 ? ut_date.mjd() : ut_date_blue.mjd();
                                    else
                                        ptime = double(nfile+1);

                                    if(nccd != 2){
                                        sprintf(sprint_out, "%15s %16.10f %1i %8.5f %1i %7.3f %7.3f",
                                                file[nfile].c_str(), ptime, reliable, expose, int(nccd+1), shape[nccd].fwhm, shape[nccd].beta);
                                        time_ok = reliable;
                                    }else{
                                        sprintf(sprint_out, "%15s %16.10f %1i %8.5f %1i %7.3f %7.3f",
                                                file[nfile].c_str(), ptime, reliable_blue, expose_blue, int(nccd+1), shape[nccd].fwhm,
                                                shape[nccd].beta);
                                        time_ok = reliable_blue;
                                    }

                                    Reduce::logger.ofstr() << sprint_out;

                                    if(Reduce::terminal_output == Reduce::FULL){
                                        std::cout << file[nfile] << ", CCD " << nccd+1;
                                    }else if(Reduce::terminal_output == Reduce::MEDIUM){
                                        std::cout << file[nfile] << " " << nccd+1;
                                    }
                                }

                                std::cout << std::flush;

                                // Now loop over apertures, adding results in a series on a single line.
                                for(size_t naper=0; naper<aperture[nccd].size(); naper++){

                                    Ultracam::Aperture &app = aperture[nccd][naper];
                                    float xmeas = 0., ymeas = 0., exmeas = -1., eymeas = -1.;

                                    if(!blue_is_bad || nccd != 2){
                                        // Modify the aperture if necessary
                                        if(Reduce::star_radius.size() > 0){
                                            float rstar = 0;
                                            if(Reduce::extraction_control[nccd].aperture_type       == Reduce::VARIABLE){
                                                rstar = Subs::clamp(Reduce::extraction_control[nccd].star_min,
                                                                    float(shape[nccd].fwhm*Reduce::star_radius[nradius]),
                                                                    Reduce::extraction_control[nccd].star_max);
                                            }else if(Reduce::extraction_control[nccd].aperture_type == Reduce::FIXED){
                                                rstar = Subs::clamp(Reduce::extraction_control[nccd].star_min,
                                                                    Reduce::star_radius[nradius],
                                                                    Reduce::extraction_control[nccd].star_max);
                                            }
                                            app.set_rstar(rstar);
                                        }

                                        // I/O. Information to identify aperture and its position as used and the measure position and its uncertainty
                                        if(!aperture[nccd][naper].linked()){
                                            if(Reduce::aperture_twopass){
                                                if(twopass[nexp].offset[nccd][naper].ok){
                                                    xmeas  = twopass[nexp].offset[nccd][naper].x + twopass[nexp].ref_pos[nccd].x;
                                                    ymeas  = twopass[nexp].offset[nccd][naper].y + twopass[nexp].ref_pos[nccd].y;
                                                    exmeas = twopass[nexp].offset[nccd][naper].xe;
                                                    eymeas = twopass[nexp].offset[nccd][naper].ye;
                                                }
                                            }else if(app.valid()){
                                                xmeas  = app.xpos();
                                                ymeas  = app.ypos();
                                                exmeas = errors[nccd][naper].x;
                                                eymeas = errors[nccd][naper].y;
                                            }
                                        }
                                    }else{
                                        xmeas  = ymeas  = 0.;
                                        exmeas = eymeas = -1.;
                                    }

                                    sprintf(sprint_out, " %2i %9.4f %9.4f %9.4f %9.4f %7.4f %7.4f",
                                            int(naper+1), app.xpos(), app.ypos(), xmeas, ymeas, exmeas, eymeas);
                                    Reduce::logger.ofstr()<< sprint_out;

                                    if(Reduce::terminal_output == Reduce::FULL){
                                        std::cout << ", Ap " << naper+1 << ", " << app.xpos() << "  " << app.ypos();
                                    }else if(Reduce::terminal_output == Reduce::MEDIUM){
                                        std::cout << " " << naper+1 << " "  << app.xpos() << " " << app.ypos();
                                    }

                                    if(!blue_is_bad || nccd != 2){
                                        // Extract the flux
                                        Ultracam::extract_flux(data[nccd], dvar[nccd], bad[nccd], Reduce::gain_frame[nccd],
                                                               Reduce::bias_frame[nccd], app, Reduce::sky_method,
                                                               Reduce::sky_thresh, Reduce::sky_error,
                                                               Reduce::extraction_control[nccd].extraction_method, zapped[nccd][naper],
                                                               shape[nccd], Reduce::pepper[nccd], Reduce::saturation[nccd],
                                                               counts, sigma, sky, nsky, nrej, ecode, worst);

                                        if(Reduce::abort_behaviour == Reduce::FUSSY){

                                            switch(ecode){

                                            case Reduce::TARGET_APERTURE_AT_EDGE_OF_WINDOW:
                                                throw Ultracam_Error("Fussy mode: CCD " + Subs::str(nccd+1) + ", aperture " +
                                                                     Subs::str(naper+1) + ": target aperture at edge of window!");

                                            case Reduce::APERTURE_OUTSIDE_WINDOW:
                                                throw Ultracam_Error("Fussy mode: CCD " + Subs::str(nccd+1) + ", aperture " +
                                                                     Subs::str(naper+1) + ": aperture outside window!");

                                            case Reduce::APERTURE_INVALID:
                                                throw Ultracam_Error("Fussy mode: CCD " + Subs::str(nccd+1) + ", aperture " +
                                                                     Subs::str(naper+1) + ": aperture invalid!");
                                            default:
                                                break;
                                            }

                                        }

                                    }else{
                                        ecode  = Reduce::BLUE_IS_JUNK;
                                        counts = 0.;
                                        sigma  = -1.;
                                        sky    = 0.;
                                        nsky   = 0;
                                        nrej   = 0;
                                        worst  = 0;
                                    }

                                    // I/O -- the fluxes
                                    sprintf(sprint_out, " %10.1f %7.1f %9.2f %3i %4i %2i %2i", counts, sigma, sky, nsky, nrej, worst, ecode);
                                    Reduce::logger.ofstr()<< sprint_out;

                                    if(Reduce::terminal_output == Reduce::FULL || Reduce::terminal_output == Reduce::MEDIUM)
                                        std::cout << blank << counts << blank << sigma << blank << sky << blank << nrej << blank
                                                  << worst << blank << ecode;

                                    // Store this point for light curve plot.
                                    if(lplot || hcopy != "null"){
                                        if(nccd != 2)
                                            all_ccds[nccd][naper] = Reduce::Point(counts, sigma, app.xpos(), app.ypos(),
                                                                                  shape[nccd].fwhm, ecode, expose, time_ok);
                                        else
                                            all_ccds[nccd][naper] = Reduce::Point(counts, sigma, app.xpos(), app.ypos(),
                                                                                  shape[nccd].fwhm, ecode, expose_blue, time_ok);
                                    }

                                }

                                // End of apertures for this CCD, insert a newline
                                Reduce::logger.ofstr() << std::string("\n") << std::flush;
                                if(Reduce::terminal_output == Reduce::FULL || Reduce::terminal_output == Reduce::MEDIUM)
                                    std::cout << std::endl;
                            }
                        }
                        nradius++;
                    } while(nradius < Reduce::star_radius.size());

                    // Light curve plot section
                    if(lplot || hcopy != "null")
                        Ultracam::light_plot(lcurve_plot, all_ccds, ut_date, false, hcopy, light_curve_title);

                    nexp++;
                }

                // Update the file number
                nfile++;
                first_file = false;

            }

            if(Reduce::aperture_twopass && npass == 1){

                Reduce::logger.ofstr()<< hashb << std::string("Two pass mode polynomial fitting results.") << newl;

                // Now we have all the positional data stored in the structure 'twopass',
                // we need to carry out polynomial fits.

                // Set up polynomial function object
                poly.setup(Reduce::aperture_twopass_npoly, twopass[0].time, twopass[twopass.size()-1].time);

                // Allocate buffers
                Subs::Buffer1D<double> x(twopass.size()), y(twopass.size());
                Subs::Buffer1D<float>  e(twopass.size());
                Subs::Buffer1D<double> coeff(Reduce::aperture_twopass_npoly);
                Subs::Buffer2D<double> covar(Reduce::aperture_twopass_npoly,Reduce::aperture_twopass_npoly);

                // Go through fitting each aperture
                for(size_t nccd=0; nccd<aperture.size(); nccd++){

                    if((!blue_is_bad || nccd != 2) && (Reduce::extraction_control.find(nccd) != Reduce::extraction_control.end())){
                        for(size_t naper=0; naper<aperture[nccd].size(); naper++){
                            if(!aperture[nccd][naper].linked()){
                                Reduce::logger.ofstr()<< hashb << newl;
                                Reduce::logger.ofstr()<< hashb << std::string(" CCD ") << (nccd+1) <<std::string(" aperture ") << (naper+1) << newl;

                                // First fit X offsets
                                int ndof = - Reduce::aperture_twopass_npoly;
                                for(size_t ntime=0; ntime<twopass.size(); ntime++){
                                    Reduce::Twopass &temp = twopass[ntime];
                                    if(temp.ref_valid[nccd] && temp.offset[nccd][naper].ok){
                                        x[ntime] = temp.time;
                                        y[ntime] = temp.offset[nccd][naper].x;
                                        e[ntime] = temp.offset[nccd][naper].xe;
                                        ndof++;
                                    }else{
                                        x[ntime] = temp.time;
                                        e[ntime] = -1.;
                                    }
                                }

                                int nrej = 1, ncycle = 0, nrejtot=0;
                                double chisq;
                                while(nrej > 0 && ndof >=0){
                                    Subs::llsqr(twopass.size(), x, y, e, poly, coeff, covar);
                                    nrej = Subs::llsqr_reject(twopass.size(), x, y, e, poly, coeff, Reduce::aperture_twopass_sigma, true);
                                    ncycle++;
                                    ndof    -= nrej;
                                    nrejtot += nrej;
                                }

                                Polyfit &tpoly = vpoly[nccd][naper];
                                if(ndof < 0){
                                    tpoly.ok = false;
                                    Reduce::logger.ofstr()<< hashb << std::string("  WARNING: Polynomial fit to X offsets failed from insufficient good data. Will attempt to use directly fitted positions") << newl;
                                    Reduce::logger.ofstr()<< hashb << std::string("  but you should probably investigate further before trusting the results.") << newl;
                                }else{

                                    chisq = Subs::llsqr_chisq(twopass.size(), x, y, e, poly, coeff);

                                    Reduce::logger.ofstr()<< hashb << std::string("  Polynomial fit to X offsets successful.") << newl;
                                    Reduce::logger.ofstr()<< hashb << std::string("  Number of cycles = ") << ncycle << std::string(", number of degrees of freedom = ") << ndof
                                                          << std::string(", number of points rejected = ") << nrejtot << std::string(", Chi**2 = ") << chisq << newl;

                                    // Store results
                                    tpoly.ok = true;
                                    tpoly.x.resize(Reduce::aperture_twopass_npoly);
                                    for(int np=0; np<Reduce::aperture_twopass_npoly; np++)
                                        tpoly.x[np] = coeff[np];

                                    // Now the Y offsets
                                    int ndof = -Reduce::aperture_twopass_npoly;
                                    for(size_t ntime=0; ntime<twopass.size(); ntime++){
                                        Reduce::Twopass &temp = twopass[ntime];
                                        if(temp.ref_valid[nccd] && temp.offset[nccd][naper].ok){
                                            y[ntime] = temp.offset[nccd][naper].y;
                                            e[ntime] = temp.offset[nccd][naper].ye;
                                            ndof++;
                                        }
                                    }

                                    nrej    = 1;
                                    ncycle  = 0;
                                    nrejtot = 0;
                                    while(nrej > 0 && ndof >=0){
                                        Subs::llsqr(twopass.size(), x, y, e, poly, coeff, covar);
                                        nrej = Subs::llsqr_reject(twopass.size(), x, y, e, poly, coeff, Reduce::aperture_twopass_sigma, true);
                                        ncycle++;
                                        ndof    -= nrej;
                                        nrejtot += nrej;
                                    }

                                    if(ndof < 0){
                                        tpoly.ok = false;
                                        Reduce::logger.ofstr()<< hashb << std::string("  WARNING: Polynomial fit to Y offsets failed from insufficient good data. Will attempt to use directly fitted positions") << newl;
                                        Reduce::logger.ofstr()<< hashb << std::string("  but you should probably investigate further before trusting the results. X offset fit will also be ignored.") << newl;
                                    }else{

                                        chisq = Subs::llsqr_chisq(twopass.size(), x, y, e, poly, coeff);

                                        Reduce::logger.ofstr()<< hashb << std::string("  Polynomial fit to Y offsets successful.") << newl;
                                        Reduce::logger.ofstr()<< hashb << std::string("  Number of cycles = ") << ncycle << std::string(", number of degrees of freedom = ") << ndof
                                                              << std::string(", number of points rejected = ") << nrejtot << std::string(", Chi**2 = ") << chisq << newl;

                                        // Store results
                                        tpoly.y.resize(Reduce::aperture_twopass_npoly);
                                        for(int np=0; np<Reduce::aperture_twopass_npoly; np++)
                                            tpoly.y[np] = coeff[np];
                                    }
                                }
                            }else{
                                vpoly[nccd][naper].ok = true;
                            }
                        }
                    }
                }
                Reduce::logger.ofstr()<< hashb << newl;
            }
        }

        // Make a hard copy
        if(hcopy != "null")
            Ultracam::light_plot(lcurve_plot, all_ccds, ut_date, true, hcopy, light_curve_title);

    }

    // Handle errors
    catch(const Subs::Subs_Error& err){
        std::cerr << "\nSubs::Subs_Error:" << std::endl;
        std::cerr << err << std::endl;
    }
    catch(const Ultracam::Input_Error& err){
        std::cerr << "\nUltracam::Input_Error:" << std::endl;
        std::cerr << err << std::endl;
    }
    catch(const Ultracam::File_Open_Error& err){
        std::cerr << "\nUltracam::File_Open_error:" << std::endl;
        std::cerr << err << std::endl;
    }
    catch(const Ultracam::Ultracam_Error& err){
        std::cerr << "\nUltracam::Ultracam_Error:" << std::endl;
        std::cerr << err << std::endl;
    }
    catch(const std::string& err){
        std::cerr << "\n" << err << std::endl;
    }
}

