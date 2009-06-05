## Process this file with automake to generate Makefile.in

## This to avoid . being in the path of includes which causes problems for
## functions looking for the 'list' C++ header

AUTOMAKE_OPTIONS = nostdinc

## Programs

progdir = @bindir@/@PACKAGE@

prog_PROGRAMS  = addfield arith carith crop folder grab hedit hedlis hist combine ncal noise pixel plot reduce \
rtplot setaper setdefect setfield setwin stats uinfo uinit ucm2fits grab2fits fits2ucm oneline movie wjoin makeflat \
vshow multiframe window boxavg list badgen shifter times gettime boxmed bcrop addspec gentemp genseries addsky \
addbad dsub backsub setreg sreduce collapse expand lplot ppos

addfield_SOURCES   = addfield.cc 
addsky_SOURCES     = addsky.cc 
addbad_SOURCES     = addbad.cc 
addspec_SOURCES    = addspec.cc
arith_SOURCES      = arith.cc 
backsub_SOURCES    = backsub.cc 
badgen_SOURCES     = badgen.cc 
bcrop_SOURCES      = bcrop.cc
boxavg_SOURCES     = boxavg.cc
boxmed_SOURCES     = boxmed.cc
carith_SOURCES     = carith.cc
collapse_SOURCES   = collapse.cc
combine_SOURCES    = combine.cc
crop_SOURCES       = crop.cc
dsub_SOURCES       = dsub.cc
expand_SOURCES     = expand.cc
fits2ucm_SOURCES   = fits2ucm.cc
folder_SOURCES     = folder.cc 
genseries_SOURCES  = genseries.cc
gentemp_SOURCES    = gentemp.cc

## special case for this routine which does not need other libraries
gettime_SOURCES   = gettime.cc
gettime: gettime.cc
	g++ -o gettime gettime.cc

grab_SOURCES       = grab.cc 
grab2fits_SOURCES  = grab2fits.cc
hedit_SOURCES      = hedit.cc
hedlis_SOURCES     = hedlis.cc 
hist_SOURCES       = hist.cc
list_SOURCES       = list.cc
lplot_SOURCES      = lplot.cc
makeflat_SOURCES   = makeflat.cc 
movie_SOURCES      = movie.cc
multiframe_SOURCES = multiframe.cc
ncal_SOURCES       = ncal.cc
noise_SOURCES      = noise.cc 
oneline_SOURCES    = oneline.cc
pixel_SOURCES      = pixel.cc
plot_SOURCES       = plot.cc  
ppos_SOURCES       = ppos.cc
reduce_SOURCES     = reduce.cc read_reduce_file.cc rejig_apertures.cc light_plot.cc def_col.cc
rtplot_SOURCES     = rtplot.cc
setaper_SOURCES    = setaper.cc
setdefect_SOURCES  = setdefect.cc
setfield_SOURCES   = setfield.cc
setreg_SOURCES     = setreg.cc
setwin_SOURCES     = setwin.cc
shifter_SOURCES    = shifter.cc
sreduce_SOURCES    = sreduce.cc read_sreduce_file.cc
stats_SOURCES      = stats.cc
times_SOURCES      = times.cc
uinfo_SOURCES      = uinfo.cc
ucm2fits_SOURCES   = ucm2fits.cc
uinit_SOURCES      = uinit.cc
vshow_SOURCES      = vshow.cc
window_SOURCES     = window.cc
wjoin_SOURCES      = wjoin.cc

DATE = $(shell date)

install-data-hook: $(progdir)/add $(progdir)/mul $(progdir)/div $(progdir)/sub $(progdir)/cadd $(progdir)/cmul $(progdir)/cdiv \
$(progdir)/csub $(progdir)/cset $(progdir)/uset $(progdir)/msub $(progdir)/mdiv $(progdir)/cplot
	echo "This is $(PACKAGE)-$(VERSION), built on $(DATE)" > $(progdir)/VERSION
	@echo "Installed link targets"

## Link targets

$(progdir)/add: $(progdir)/arith
	cd $(progdir); ln -sf arith $@

$(progdir)/sub: $(progdir)/arith
	cd $(progdir); ln -sf arith $@

$(progdir)/mul: $(progdir)/arith
	cd $(progdir); ln -sf arith $@

$(progdir)/div: $(progdir)/arith
	cd $(progdir); ln -sf arith $@

$(progdir)/uset: $(progdir)/arith
	cd $(progdir); ln -sf arith $@

$(progdir)/cadd: $(progdir)/carith
	cd $(progdir); ln -sf carith $@

$(progdir)/csub: $(progdir)/carith
	cd $(progdir); ln -sf carith $@

$(progdir)/cmul: $(progdir)/carith
	cd $(progdir); ln -sf carith $@

$(progdir)/cdiv: $(progdir)/carith
	cd $(progdir); ln -sf carith $@

$(progdir)/cset: $(progdir)/carith
	cd $(progdir); ln -sf carith $@

$(progdir)/msub: $(progdir)/multiframe
	cd $(progdir); ln -sf multiframe $@

$(progdir)/mdiv: $(progdir)/multiframe
	cd $(progdir); ln -sf multiframe $@

$(progdir)/cplot: $(progdir)/plot
	cd $(progdir); ln -sf plot $@

INCLUDES = -I../include -I../.

# force re-build of reduce if configure.ac changes to allow for
# version number changes
reduce_DEPENDENCIES = ../configure.ac

LDADD = libultracam.la

## Library

lib_LTLIBRARIES = libultracam.la 

libultracam_la_SOURCES = trm_window.cc trm_windata.cc trm_ccd.cc trm_frame.cc trm_target.cc trm_mccd.cc trm_skyline.cc \
trm_defect.cc shift_and_add.cc WriteMemoryCallback.cc parseXML.cc de_multiplex.cc read_header.cc trm_aperture.cc \
get_server_frame.cc loadXML.cc trm_fdisk.cc lllccd.cc plot_images.cc findpos.cc \
plot_apers.cc fitgaussian.cc trm_ultracam.cc gauss_reject.cc profit_init.cc moffat_reject.cc fitmoffat.cc pos_tweak.cc \
fit_plot_profile.cc covsrt.cc extract_flux.cc sky_estimate.cc badInput.cc plot_defects.cc plot_setupwins.cc trm_spectrum.cc \
make_profile.cc trm_specap.cc sky_move.cc sky_fit.cc ext_nor.cc plot_trail.cc plot_spectrum.cc
