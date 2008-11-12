#
# Data file written by the java program 'Reduce'
# The file is supposed to be used to drive the ultracam pipeline program 'reduce'
#
# See the on-line help on 'reduce' for a full description of the format and parameters
#

# General stuff

version                    = 19/12/2005               # Date of reduce version (must match reduce itself)
cr_to_start                = no                       # yes/no. Carriage return to start or not
abort_behaviour            = relaxed                  # When to give up on the reduction: 'fussy' or 'relaxed'
terminal_output            = little                   # Amount of terminal output: 'none', 'little', 'medium', 'full'
clobber                    = yes                      # Let the log file over-write pre-existing files or not

# Saturation parameters

pepper                     = 53000 30000 27000        # level at which to set error flag to indicate peppering
saturation                 = 65530 65530 65530        # level at which to set error flag to indicate saturation

# Cosmic ray cleaning (not currently enabled)

cosmic_clean               = no                       # Cosmic ray cleaning: 'yes' or 'no'
cosmic_height              = 50                       # Minimum height above average of nearest pixels to count as a cosmic ray
cosmic_ratio               = 1.2                      # Minimum ratio relative to average of nearest pixels to count as a cosmic ray

# Aperture parameters

aperture_file              = gwlib                    # file of software apertures for each CCD
aperture_reposition_mode   = reference_plus_tweak     # relocation method: static, individual, individual_plus_tweak, reference_plus_tweak
aperture_positions_stable  = yes                      # whether to weight search towards last position or not
aperture_search_half_width = 35                       # half width of box for initial search around last position, unbinned pixels
aperture_search_fwhm       = 14.0                     # FWHM for gaussian used to locate objects, unbinned pixels
aperture_search_max_shift  = 24.0                     # maximum allowed shift in object positions, frame to frame, unbinned pixels
aperture_tweak_half_width  = 20                       # half width of box for tweak after a search, unbinned pixels
aperture_tweak_fwhm        = 8.0                      # FWHM for gaussian used in tweaking object position, unbinned pixels
aperture_tweak_max_shift   = 4.0                      # maximum allowed shift when tweaking object positions, unbinned pixels.
aperture_twopass           = no                       # twopasses to fit relative position drift or not
aperture_twopass_counts    = 20.0                     # minimum number of counts for a position to be included in the fits
aperture_twopass_npoly     = 3                        # number of polynomial coefficients for the fits
aperture_twopass_sigma     = 3.0                      # mrejection threshold, multiple of RMS, for fits

# Extraction control parameters. One per line with the format
#
# nccd aperture_type extraction_method star_scale star_min star_max inner_sky_scale inner_sky_min inner_sky_max outer_sky_scale outer_sky_min outer_sky_max
#
# aperture_type can be 'fixed' or 'variable' (i.e. fixed or variable radii); extraction_method can be 'normal' or 'optimal'.
# The aperture radius scale factors are multiples of the FWHM so if either of 'variable' or 'optimal' are set, profile fitting will
# be carried out. The minimum and maximum ranges allow you to control the sky aperture radii, for instance to avoid a nearby bright star.

extraction_control         = 1 variable normal 1.7 6.0 30.0 2.5 17.0 35.0 3.0 20.0 40.0
extraction_control         = 2 variable normal 1.7 6.0 30.0 2.5 17.0 35.0 3.0 20.0 40.0
extraction_control         = 3 variable normal 1.6 6.0 30.0 2.5 17.0 35.0 3.0 20.0 40.0

# This line is optional but allow one to force gaussian weights even when using
# moffat fits

extraction_weights         = gaussian # values 'gaussian' or 'moffat'

# Multiple-radii extractions if next line is not commented

#star_aperture_radii        = 1.1 1.2 1.3 1.4 1.5 1.6 1.7 1.8 1.9 2.0 2.1 2.2 2.3 2.4 2.5

# Profile fitting parameters

profile_fit_method         = moffat                   # method of fitting, 'gaussian' or 'moffat'
profile_fit_fwhm           = 8                        # FWHM, unbinned pixels
profile_fit_hwidth         = 35                       # Half-width of box over which fit will be made, unbinned pixels
profile_fit_symm           = yes                      # symmetrical profile or not (else elliptical)
profile_fit_beta           = 3                        # Moffat exponent
profile_fit_sigma          = 3.6                      # Sigma rejection threshold for fits

# Sky background estimation parameters

sky_method                 = clipped_mean             # method of estimating sky background, 'clipped_mean' or 'median'
sky_error                  = variance                 # method of estimating uncertainty in sky background, 'variance' or 'photon'
sky_thresh                 = 3                        # threshold (multiple of RMS) for rejection if clipped_mean in use

# Calibration section

calibration_bias           = bias                     # bias frame, blank to ignore
calibration_flat           = flat                     # flat field, blank to ignore
calibration_dark           =                          # dark frame, blank to ignore
calibration_bad            = bad                      # bad pixel file, blank to ignore
calibration_gain           = 1.1                      # gain, electrons/ADU, value or file
calibration_readout        = 4                        # readout noise (RMS ADU), value or file
calibration_coerce         = yes                      # coerce calibration frames to match data format or not

# Image plot device (most image display parameters set on the fly)

image_device               = 1/xs                     # image display device

# Lightcurve plot

lightcurve_frac            = 3                        # relative fraction of vertical height of lightcurve plot
lightcurve_device          = 2/xs                     # display device for lightcurves, seeing etc
lightcurve_xunits          = minutes                  # units for X-axis, 'seconds', 'minutes', 'hours', 'days'
lightcurve_max_xrange      = 0                        # maximum range in x to plot (<= 0 for everything)
lightcurve_extend_xrange   = 10.0                     # amount to extend X range when buffer fills
lightcurve_linear_or_log   = log                      # Y axis: 'linear' or 'log' (magnitudes)
lightcurve_yrange_fixed    = no                       # 'yes' or 'no' to fix Y axis limits or not
lightcurve_invert          = no                       # 'yes' or 'no' to invert Y limits or not
lightcurve_y1              = 0                        # lower Y limit
lightcurve_y2              = 0.2                      # upper Y limit
lightcurve_extend_yrange   = 1.2                      # Extension factor for Y range if not fixed
lightcurve_targ            = 1 1 2 0.0 red red        # CCD, target, offset, colour, error colour
lightcurve_targ            = 2 1 2 0.0 green red      # CCD, target, offset, colour, error colour
lightcurve_targ            = 3 1 2 0.0 blue red       # CCD, target, offset, colour, error colour

# Position plot

position_plot              = yes                      # yes/no to plot the position
position_frac              = 1                        # relative fraction of vertical height of position plot
position_x_yrange_fixed    = no                       # Fix limits on X position plot or not
position_x_y1              = -3                       # lower Y limit on X position plot
position_x_y2              = 3                        # upper Y limit on X position plot
position_y_yrange_fixed    = no                       # Fix limits on Y position plot or not
position_y_y1              = -3                       # lower Y limit on Y position plot
position_y_y2              = 3                        # upper Y limit on Y position plot
position_extend_yrange     = 1.2                      # Extension factor for Y ranges if not fixed
position_targ              = 2 2 0.0 green red        # CCD, target, offset, colour, error colour

# Transmission plot

transmission_plot          = yes                      # yes/no to plot the transmission
transmission_frac          = 1                        # relative fraction of vertical height of transmission plot
transmission_ymax          = 100                      # Initial maximum transmission
transmission_targ          = 2 2 green                # CCD, target, colour

# Seeing plot

seeing_plot                = yes                      # yes/no to plot the seeing
seeing_frac                = 1                        # relative fraction of vertical height of seeing plot
seeing_scale               = 0.15                     # Arcsec/pixel
seeing_ymax                = 1.999                    # Initial maximum seeing
seeing_extend_yrange       = 1.2                      # Y range rescaling factor if seeing > maximum of plot
seeing_targ                = 2 2 green                # CCD, target, colour
