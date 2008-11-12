/*

!!begin
!!title   Plots an ultracam file
!!author  T.R. Marsh
!!created 20 April 2001
!!revised 06 July 2007
!!root    plot
!!index   plot
!!descr   plots an Ultracam data frame
!!css     style.css
!!class   Programs
!!class   Display
!!head1   plot - displays an ultracam image

!!emph{plot} provides some basic display capability to look at Ultracam frames. If you
want something better, it would be best to convert to FITS and use 'gaia' or 'ds9'.
See also !!ref{cplot.html}{cplot} for a version of 'plot' which allows interaction with
a cursor.

!!head2 Invocation

plot data [device] nccd ([stack]) xleft xright ylow yhigh iset (ilow ihigh)/(plow phigh) [width aspect reverse cheight font lwidth]
applot (aperture) [fwhm hwidth readout gain symm beta sigrej onedsrch (fwhm1d hwidth1d) fdevice]!!break

!!head2 Command line arguments

!!table

!!arg{data}{Ultracam data file or a list of Ultracam files. If the program fails to open
it as an ultracam file, it will assume that it is a list.}

!!arg{device}{Display device}

!!arg{nccd}{The particular CCD to display, 0 for the whole lot}

!!arg{stack}{Stacking direcion when plotting more than on CCD. Either in 'X' or 'Y'}

!!arg{xleft xright}{X range to plot}

!!arg{ylow yhigh}{Y range to plot}

!!arg{iset}{'A', 'D' or 'P' according to whether you want to set the intensity limits
automatically (= min to max), directly or with percentiles.}

!!arg{ilow ihigh}{If iset='d', ilow and ihigh specify the intensity range to plot}

!!arg{plow phigh}{If iset='p', plow and phigh are percentiles to set the intensity range,
e.g. 10, 99} 

!!arg{width}{Width of plot in inches. 0 for the default width.}

!!arg{aspect}{aspectd ratio  (y/x) of the plot panel}

!!arg{reverse}{true/false to reverse the measuring of black and white.}

!!arg{cheight}{Character height, as a multiple of the default}

!!arg{font}{Character font, 1-4 PGPLOT fonts}

!!arg{lwidth}{Line width, integer multiple of default}

!!arg{applot}{true to plot an aperture file.}

!!arg{aperture}{If applot, this is the aperture file to plot.}

!!end

!!begin
!!title   Plots an ultracam file and puts up a cursor
!!author  T.R. Marsh
!!created 06 January 2005
!!revised 09 March 2006
!!root    cplot
!!index   cplot
!!descr   plots an Ultracam data frame and allows examination with a cursor
!!css   style.css
!!class   Programs
!!class   Display
!!head1   cplot - displays an ultracam image

!!emph{cplot} provides some basic display capability to look and examine at Ultracam frames. 
The profile fitting requires the data to be bias-subtracted to work completely correctly.
This command also provides a way of examining statistics of an image.

!!head2 Invocation

cplot data [device] nccd (cursor [stack]) xleft xright ylow yhigh iset (ilow ihigh)/(plow phigh) [width aspect reverse cheight font]
applot (aperture) [fwhm hwidth readout gain symm beta sigrej onedsrch (fwhm1d hwidth1d) fdevice]!!break

!!head2 Command line arguments

!!table

!!arg{data}{Ultracam data file or a list of Ultracam files. If the program fails to open
it as an ultracam file, it will assume that it is a list.}

!!arg{device}{Display device}

!!arg{nccd}{The particular CCD to display, 0 for the whole lot}

!!arg{cursor}{Enable the cursor, but only for single CCD plots}

!!arg{stack}{Stacking direcion when plotting more than on CCD. Either in 'X' or 'Y'}

!!arg{xleft xright}{X range to plot}

!!arg{ylow yhigh}{Y range to plot}

!!arg{iset}{'A', 'D' or 'P' according to whether you want to set the intensity limits
automatically (= min to max), directly or with percentiles.}

!!arg{ilow ihigh}{If iset='d', ilow and ihigh specify the intensity range to plot}

!!arg{plow phigh}{If iset='p', plow and phigh are percentiles to set the intensity range,
e.g. 10, 99} 

!!arg{width}{Width of plot in inches. 0 for the default width.}

!!arg{aspect}{aspectd ratio  (y/x) of the plot panel}

!!arg{reverse}{true/false to reverse the measuring of black and white.}

!!arg{cheight}{Character height, as a multiple of the default}

!!arg{font}{Character font, 1-4 PGPLOT fonts}

!!arg{lwidth}{Line width, integer multiple of default}

!!arg{applot}{true to plot an aperture file.}

!!arg{aperture}{If applot, this is the aperture file to plot.}

!!arg{fwhm}{This is the first of several parameters associated with profile fits (gaussian
or moffat profiles). fwhm is the initial FWHM to use in either case.}

!!arg{hwidth}{The half-width of the region to be used when fitting a target. Should be larger
than the fwhm, but not so large as to include multiple targets if possible.}

!!arg{readout}{Readout noise, RMS ADU in order for the program to come back with an uncertainty.}

!!arg{gain}{Gain, electrons/ADU, again for uncertainty estimates}

!!arg{symm}{Yes/no for symmetric versus ellliptical profile fits}

!!arg{beta}{The beta parameter of the moffat fits.}

!!arg{sigrej}{The fits can include rejection of poor pixels. This is the threshold, meaured in sigma. Should not
be too small.}

!!arg{onedsrch}{Yes if you want an initial 1D search to be made. This is tolerant of poor positioning of the start point, 
but potentially vulnerable to problems with multiple targets. What happens is that a box around the
cursor position is collpased in X and Y and then the peak in each direction is located using cross-correlation
with a gaussian of FWHM=fwhm1D. This new position is then used to define the fitting region and initial position for
the 2D gaussian fit.}

!!arg{fwhm1d}{This is the FWHM used in the 1D search. It does not have to match the FWHM of the target necessarily.
In particular a somewhat larger value is less sensitive to initial position errors.}

!!arg{hwidth1d}{The half-width of the region to be used for searching for a target. The wider
this is, the more chance of finding a target from a sloppy start position, but also the more chance of
peaking up on a spurious target.}

!!arg{rstar}{If you carry out fits, the program also extracts a flux. It scales the aperture radii by the seeing.
'rstar' gives the multiple of the seeing to use for the target.}

!!arg{rsky1}{If you carry out fits, the program also extracts a flux. It scales the aperture radii by the seeing.
'rsky1' gives the multiple of the seeing to use for the inner radius of the sky annulus.}

!!arg{rsky2}{If you carry out fits, the program also extracts a flux. It scales the aperture radii by the seeing.
'rsky2' gives the multiple of the seeing to use for the outer radius of the sky annulus.}

!!arg{fdevice}{Plot device for showing Moffat & symmetrical gaussian fits.
Should be different from the image plot device. e.g. "/xs" or "2/xs" if 
image plot device = "1/xs" otherwise the program will go belly up for reasons that
I cannot quite track down. 'null' to ignore.}

!!arg{xbox}{The 'S' show command gives a few simple stats for the region the cursor is 
centered on such as the mean and median. This parameter specifies the half-size in X that will be used
in terms of binned pixels. If the nearest pixel is at ix, the a range ix-xbox to ix+xbox will
be used, but truncated at the edge of the window.}

!!arg{xbox}{The 'S' show command gives a few simple stats for the region the cursor is 
centered on such as the mean and median. This parameter specifies the half-size in Y that will be used
in terms of binned pixels. If the nearest pixel is at iy, the a range iy-ybox to iy+ybox will
be used, but truncated at the edge of the window.}

!!table

!!head2 Cursor commands

The cursor options are as follows
!!table
!!arg{I}{For 'In'. Zooms in around the current cursor position by a factor of 2}
!!arg{O}{For 'Out'. Zooms out around the current cursor position by a factor of 2}
!!arg{G}{For Gaussian. Performs a 2D gaussian fit. This proceeds
as follows: first, a search is made over a box centred on the cursor position by collapsing in X and Y
and performing a 1D gaussian cross-correlation. Then using this updated position, a 2D gaussian fit is made.
This can either be symmetric or elliptical in shape. Note that the uncertainties reported rely on accurate
readout and gain parameters having been supplied. There is one subtle feature of the gaussian fits: it
fits to a blurred model profile as a way of preventing fits ot cosmic rays. Many other
parameters are hidden command-line parameters that can be set if you specify 'prompt' on the command-line.}  
!!arg{M}{For Moffat. Performs a 2D Moffat profile fit, in much the same way as the Gaussian fit, but
only symmetric Moffat profiles are allowed for. Starting parameters are hidden; specify prompt if you
want to alter them.}
!!arg{L}{For Levels. Change the plot levels.}
!!arg{W}{For Whole. Display whole CCD.}
!!arg{S}{Shows value of pixel corresponding to cursor position (no graphics scaling -- it gives you 
the true value), along with a few simple stats on the surrounding region specified using the hidden xbox and
ybox parameters. These are the mean, rms, median, minimum and maximum values. The box is truncated at the edges.}
!!arg{Q}{Quit}
!!table

!!end

*/

#include <climits>
#include <cstdlib>
#include <cfloat>
#include <string>
#include <map>
#include <vector>
#include "cpgplot.h"
#include "trm_constants.h"
#include "trm_subs.h"
#include "trm_format.h"
#include "trm_input.h"
#include "trm_plot.h"
#include "trm_frame.h"
#include "trm_aperture.h"
#include "trm_mccd.h"
#include "trm_reduce.h"
#include "trm_ultracam.h"

int main(int argc, char* argv[]){

    using Ultracam::Input_Error;
    using Ultracam::Ultracam_Error;

    try{

	// Construct Input object

	Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

	std::string command  = argv[0];
	size_t slash = command.find_last_of('/');
	if(slash != std::string::npos) command.erase(0,slash+1);

	// sign-in input variables
	input.sign_in("data",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("device",  Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
	input.sign_in("nccd",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
	if(command == "cplot")
	    input.sign_in("cursor",  Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("stack",   Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("xleft",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("xright",  Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("ylow",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("yhigh",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("iset",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("ilow",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("ihigh",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("plow",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("phigh",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("width",   Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
	input.sign_in("aspect",  Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
	input.sign_in("reverse", Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
	input.sign_in("cheight", Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
	input.sign_in("font",    Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
	input.sign_in("lwidth",  Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
	input.sign_in("applot",  Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("aperture",Subs::Input::GLOBAL, Subs::Input::PROMPT);

	// Interactive settings
	if(command == "cplot"){
	    input.sign_in("fwhm",    Subs::Input::GLOBAL,  Subs::Input::NOPROMPT);
	    input.sign_in("hwidth",  Subs::Input::GLOBAL,  Subs::Input::NOPROMPT);
	    input.sign_in("readout", Subs::Input::GLOBAL,  Subs::Input::NOPROMPT);
	    input.sign_in("gain",    Subs::Input::GLOBAL,  Subs::Input::NOPROMPT);
	    input.sign_in("symm",    Subs::Input::GLOBAL,  Subs::Input::NOPROMPT);
	    input.sign_in("beta",    Subs::Input::GLOBAL,  Subs::Input::NOPROMPT);
	    input.sign_in("sigrej",  Subs::Input::GLOBAL,  Subs::Input::NOPROMPT);
	    input.sign_in("onedsrch",Subs::Input::GLOBAL,  Subs::Input::NOPROMPT);
	    input.sign_in("fwhm1d",  Subs::Input::GLOBAL,  Subs::Input::NOPROMPT);
	    input.sign_in("hwidth1d",Subs::Input::GLOBAL,  Subs::Input::NOPROMPT);
	    input.sign_in("rstar",   Subs::Input::LOCAL,   Subs::Input::NOPROMPT);
	    input.sign_in("rsky1",   Subs::Input::LOCAL,   Subs::Input::NOPROMPT);
	    input.sign_in("rsky2",   Subs::Input::LOCAL,   Subs::Input::NOPROMPT);
	    input.sign_in("fdevice", Subs::Input::LOCAL,   Subs::Input::NOPROMPT);
	    input.sign_in("xbox",    Subs::Input::LOCAL,   Subs::Input::NOPROMPT);
	    input.sign_in("ybox",    Subs::Input::LOCAL,   Subs::Input::NOPROMPT);      
	}

	// Get inputs
	std::string name;
	input.get_value("data", name, "run001", "file or file list to plot");
	std::string device;
	input.get_value("device", device, "/xs", "plot device");

	// Read file or list

	std::vector<std::string> flist;
	if(Ultracam::Frame::is_ultracam(name)){
	    flist.push_back(name);
	}else{      
	    std::ifstream istr(name.c_str());
	    while(istr >> name){
		flist.push_back(name);
	    }
	    istr.close();
	    if(flist.size() == 0) throw Input_Error("No file names loaded");
	}

	// Read first file for establishing defaults
	Ultracam::Frame frame(flist[0]);

	size_t nccd = 1;
	if(frame.size() > 1){
	    if(command == "cplot"){
		input.get_value("nccd", nccd, size_t(1), size_t(1), frame.size(), "CCD number to plot");
	    }else{
		input.get_value("nccd", nccd, size_t(0), size_t(0), frame.size(), "CCD number to plot (0 for all)");
	    }
	}

	char stackdirn;
	if(nccd == 0){
	    input.get_value("stack", stackdirn, 'X', "xXyY", "stacking direction for image display (X or Y)");
	    stackdirn = toupper(stackdirn);
	}
	float x1, x2, y1, y2;
	if(nccd){
	    x2 = frame[nccd-1].nxtot()+0.5;
	    y2 = frame[nccd-1].nytot()+0.5;
	}else{
	    x2 = frame.nxtot()+0.5;
	    y2 = frame.nytot()+0.5;
	}
	input.get_value("xleft",  x1, 0.5f, 0.5f, x2, "left X limit of plot");
	input.get_value("xright", x2, x2,   0.5f, x2, "right X limit of plot");
	input.get_value("ylow",   y1, 0.5f, 0.5f, y2, "lower Y limit of plot");
	input.get_value("yhigh",  y2, 0.5f, 0.5f, y2, "upper Y limit of plot");

	char iset;
	input.get_value("iset", iset, 'a', "aAdDpP", 
			"set intensity a(utomatically), d(irectly) or with p(ercentiles)?");
	iset = toupper(iset);
	float ilow, ihigh, plow, phigh;
	if(iset == 'D'){
	    input.get_value("ilow",   ilow,  0.f, -FLT_MAX, FLT_MAX, "lower intensity limit");
	    input.get_value("ihigh",  ihigh, 1000.f, -FLT_MAX, FLT_MAX, "upper intensity limit");
	}else if(iset == 'P'){
	    input.get_value("plow",   plow,  1.f, 0.f, 100.f,  "lower intensity limit percentile");
	    input.get_value("phigh",  phigh, 99.f, 0.f, 100.f, "upper intensity limit percentile");
	    plow  /= 100.;
	    phigh /= 100.;
	}

	float width;
	input.get_value("width",  width, 0.f, 0.f, 100.f, "width of plot in inches (0 for default)");
	float aspect;
	if(width == 0.f)
	    input.get_value("aspect", aspect, 0.6f, 0.f, 100.f, "aspect ratio of plot (0 for default)");
	else
	    input.get_value("aspect", aspect, 0.6f, 1.e-2f, 100.f, "aspect ratio of plot");
	bool reverse;
	input.get_value("reverse", reverse, false, "do you want to reverse black and white?");
	float cheight;
	input.get_value("cheight", cheight, 1.f, 0.f, 100.f, "character height (multiple of default)");
	int font;
	input.get_value("font", font, 1, 1, 4, "character font (1-4)");
	int lwidth;
	input.get_value("lwidth", lwidth, 1, 1, 40, "line width (multiple of default)");

	bool aflag;
	input.get_value("applot", aflag, false, "do you want to overplot some apertures?");
	std::string aperture;
	Ultracam::Maperture apers;
	if(aflag){
	    input.get_value("aperture", aperture, "aperture", "aperture file to plot");
	    apers = Ultracam::Maperture(aperture);
	}

	bool allccds = (nccd == 0);
	if(nccd) nccd--;

	// Profile fits
	float fwhm, readout, gain, beta, sigrej, fwhm1d;
	float rstar, rsky1, rsky2;
	int   hwidth, hwidth1d, xbox, ybox;
	bool symm, initial_search;
	std::string fdevice;
	if(command == "cplot"){
	    input.get_value("fwhm",   fwhm,  10.f, 2.f, 1000.f, "initial FWHM for gaussian & moffat profile fits");
	    input.get_value("hwidth", hwidth, int(fwhm)+1, 2, INT_MAX, "half-width of region for profile fits (unbinned pixels)");
	    input.get_value("readout",   readout,  4.f, 0.f, FLT_MAX, "readout noise for profile fits (RMS ADU)");
	    input.get_value("gain",   gain,  1.f, 0.01f, 100.f, "electrons/ADU for profile fits");
	    input.get_value("symm",  symm, true, "force symmetric profile fits?");
	    input.get_value("beta",  beta, 3.f, 1.f, 1000.f, "default beta exponent for moffat fits");
	    input.get_value("sigrej",  sigrej, 5.f, 0.f, FLT_MAX, "threshold for masking pixels (in sigma)");
	    input.get_value("onedsrch",  initial_search, true, "carry out an initial 1D position tweak?");
	    if(initial_search){
		input.get_value("fwhm1d",   fwhm1d,  fwhm, 2.f, 1000.f, "FWHM for 1D search");
		input.get_value("hwidth1d", hwidth1d, hwidth, int(fwhm1d)+1, INT_MAX, "half-width of 1D search region");
	    }
 
	    input.get_value("rstar",   rstar,  1.5f, 0.f,   1000.f, "target aperture scale factor");
	    input.get_value("rsky1",   rsky1,  2.5f, rstar, 1000.f, "inner sky scale factor");
	    input.get_value("rsky2",   rsky2,  3.5f, rsky1, 1000.f, "outer sky scale factor");
	    input.get_value("fdevice", fdevice, "2/xs", "plot device for profile fits ('null' to ignore)");
	    input.get_value("xbox",  xbox, 2, 0, 10000, "half-size of stats region in X");
	    input.get_value("ybox",  ybox, 2, 0, 10000, "half-size of stats region in Y");

	}
	// Save defaults now because one often wants to terminate this program early
	input.save();

	std::vector<Ultracam::sky_mask> skymask;

	// Open plot
	Subs::Plot plot(device);
	if(aspect > 0) cpgpap(width, aspect);
	if(reverse){
	    cpgscr( 0, 1., 1., 1.);
	    cpgscr( 1, 0., 0., 0.);
	} 
	cpgsch(cheight);
	cpgslw(lwidth);
	cpgscf(font);

	Subs::Plot fplot;
	Ultracam::Frame dvar, bad, gain_frame;
	std::vector<std::pair<int,int> > zapped;
	Reduce::Meanshape shape;
	Subs::Format cform(8);

	for(size_t ifile=0; ifile<flist.size(); ifile++){
	
	    // Read data
	    Ultracam::Frame data(flist[ifile]);

	    // Override the checking of junk blue if only one file being plotted
	    if(flist.size() == 1){
		Subs::Header::Hnode *hnode = data.find("Frame.bad_blue");
		if((hnode->has_data() ? hnode->value->get_bool() : false)){
		    std::cerr << "The blue data are junk (u-band coadd mode) but will be plotted anyway" << std::endl;
		    hnode->value->set_value(false);
		}
	    }

	    if(!allccds && nccd >= data.size())
		throw Input_Error("File = " + flist[ifile] + "CCD number = " + Subs::str(nccd+1) + " too large cf " + Subs::str(data.size()) );
    
	    if(aflag && data.size() != apers.size()) 
		throw Input_Error("File = " + flist[ifile] + "Data and aperture files have different numbers of CCDs!");

	    // All set, lets plot.
	    Ultracam::plot_images(data, x1, x2, y1, y2, allccds, stackdirn, iset, ilow, ihigh, plow, phigh, true, flist[ifile], nccd, true);

	    if(aflag) Ultracam::plot_apers(apers, x1, x2, y1, y2, allccds, stackdirn, nccd);

	    // interaction
     
	    if(command == "cplot"){

		// Create variance frame
		dvar = data;
		dvar.max(0);
		dvar /= gain;
		dvar += readout*readout;

		// Create 'bad pixel' frame, all zero
		bad  = data;
		bad  = 0.;

		gain_frame = data;
		gain_frame = gain;

		std::cout << "Position the cursor and hit the appropriate letter to zoom in/out\n"
			  << "or measure the FWHM of a star\n" << std::endl; 

		// Now aperture addition loop

		char ret;
		float x, y;
		while(ret != 'Q'){
		    ret = 'X';

		    // Next lines define the prompt:
		    std::cout << "\nI(n), O(ut), G(aussian), M(offat),  L(evels), W(hole), S(how), Q(uit)" << std::endl;

		    // Now get cursor input.
		    if(!cpgcurs(&x,&y,&ret)) throw Ultracam_Error("Cursor error");

		    ret = toupper(ret);

		    if(ret == 'I'){

			// Zoom in

			float xr = (x2-x1)/2., yr = (y2-y1)/2.; 
	
			x1 = x - xr/2.;
			x2 = x + xr/2.;
			y1 = y - yr/2.;
			y2 = y + yr/2.;
	    
			cpgeras();
			Ultracam::plot_images(data, x1, x2, y1, y2, allccds, stackdirn, iset, ilow, ihigh, 
					      plow, phigh, true, flist[ifile], nccd, true);
			if(aflag) Ultracam::plot_apers(apers, x1, x2, y1, y2, allccds, stackdirn, nccd);

		    }else if(ret == 'O'){
	    
			// Zoom out
	    
			float xr = (x2-x1)/2., yr = (y2-y1)/2.; 
	    
			x1 = x - 2.*xr;
			x2 = x + 2.*xr;
			y1 = y - 2.*yr;
			y2 = y + 2.*yr;
	    
			cpgeras();
			Ultracam::plot_images(data, x1, x2, y1, y2, allccds, stackdirn, iset, ilow, ihigh, plow, phigh,
					      true, flist[ifile], nccd, true);
			if(aflag) Ultracam::plot_apers(apers, x1, x2, y1, y2, allccds, stackdirn, nccd);

		    }else if(ret == 'W'){
	    
			// Reset frame
			x1 = 0.5;
			x2 = data[nccd].nxtot()+0.5;	    
			y1 = 0.5;
			y2 = data[nccd].nytot()+0.5;	    
	    
			cpgeras();
			Ultracam::plot_images(data, x1, x2, y1, y2, allccds, stackdirn, iset, ilow, ihigh, plow, phigh,
					      true, flist[ifile], nccd, true);
			if(aflag) Ultracam::plot_apers(apers, x1, x2, y1, y2, allccds, stackdirn, nccd);

		    }else if(ret == 'L'){
	    
			std::string entry;
			float i1n, i2n;
			std::cout << "Enter upper and lower levels [" << ilow << "," << ihigh << "]: ";
			getline(std::cin,entry);
			if(entry != ""){
			    std::istringstream ist(entry);
			    ist >> i1n >> i2n;
			    if(ist){
				ilow  = i1n;
				ihigh = i2n;
				iset = 'D';
			    }else{
				std::cerr << "Invalid entry. No change made." << std::endl;
			    }
			}

			// Re-plot
	    
			cpgeras();
			Ultracam::plot_images(data, x1, x2, y1, y2, allccds, stackdirn, iset, ilow, ihigh, plow, phigh,
					      true, flist[ifile], nccd, true);
			if(aflag) Ultracam::plot_apers(apers, x1, x2, y1, y2, allccds, stackdirn, nccd);

		    }else if(ret == 'G' || ret == 'M'){

			// Profile fit section.

			try{

			    if(fdevice != "null" && !fplot.is_open()) fplot.open(fdevice);

			    // obtain initial value of 'a'
			    double a = 1./2./Subs::sqr(fwhm/Constants::EFAC);

			    Ultracam::Ppars profile;
			    if(ret == 'G'){	    
				// Gaussian fit section.
				std::cout << "\nFitting 2D gaussian ...\n" << std::endl;
				profile.set(0., x, y, 0., a, 0., a, symm);

			    }else if(ret == 'M'){	    
				// Moffat fit section.
				std::cout << "\nFitting moffat profile ...\n" << std::endl;
				profile.set(0., x, y, 0., a, 0., a, beta, symm);
		
			    }

			    Ultracam::Iprofile iprofile;
			    Ultracam::fit_plot_profile(data[nccd], dvar[nccd], profile, initial_search, true, x, y, skymask, 
						       fwhm1d, hwidth1d, hwidth, fplot, sigrej, iprofile, true);
	    
			    // adjust defaults for next time
			    x    = profile.x;
			    y    = profile.y;
			    fwhm = iprofile.fwhm;
			    if(ret == 'M') beta = profile.beta;

			    // Create an aperture
			    Ultracam::Aperture aper(profile.x, profile.y, 0.f, 0.f, rstar*fwhm, rsky1*fwhm, rsky2*fwhm);
	      
			    // Set the shape parameters
			    if(ret == 'G'){	    
				shape.profile_fit_method = Reduce::GAUSSIAN;
				shape.extraction_weights = Reduce::GAUSSIAN;

			    }else if(ret == 'M'){	    
				shape.profile_fit_method = Reduce::MOFFAT;
				shape.extraction_weights = Reduce::MOFFAT;
		
			    }
			    shape.fwhm = fwhm;
			    shape.a    = profile.a;
			    shape.b    = profile.b;
			    shape.c    = profile.c;
			    shape.beta = profile.beta;
	      
			    float counts, sigma, sky;
			    int nsky, nrej, worst;
			    Reduce::ERROR_CODES ecode;

			    // Extract the flux
			    Ultracam::extract_flux(data[nccd], dvar[nccd], bad[nccd], gain_frame[nccd], 
						   bad[nccd], aper, Reduce::CLIPPED_MEAN, 2.8, Reduce::VARIANCE, 
						   Reduce::NORMAL, zapped, shape, 1e5, 1e5, counts, sigma, 
						   sky, nsky, nrej, ecode, worst);

			    if(sigma == -1.){
				std::cout << "Aperture photometry failed with error code =  " << ecode << std::endl;
			    }else{
				std::cout << "Aperture photometry: " << cform(counts) << " +/- " << cform(sigma) << " counts above sky in radius " << cform(rstar*fwhm) << " pixels\n" << std::endl;
			    }

			    // Return focus to image plot
			    plot.focus();
			    cpgsfs(2);

			    if(symm){
		
				// Overplot circle of radius FWHM
				cpgsci(Subs::GREEN);
				cpgcirc(profile.x, profile.y, fwhm);
				cpgpt1(profile.x, profile.y, 1);
				cpgsci(Subs::WHITE);
		
			    }else{

				// Overplot ellipse
				cpgsci(Subs::GREEN);
				float cosa = cos(Constants::TWOPI*iprofile.angle/360.);
				float sina = sin(Constants::TWOPI*iprofile.angle/360.);
				float xi, yi, x, y;
				xi = iprofile.fwhm_max;
				yi = 0.;
				x  = profile.x + cosa*xi - sina*yi;
				y  = profile.y + sina*xi + cosa*yi;
				cpgmove(x,y);
				const int NPLOT=200;
				for(int np=0; np<NPLOT; np++){
				    xi = iprofile.fwhm_max*cos(Constants::TWOPI*(np+1)/NPLOT);
				    yi = iprofile.fwhm_min*sin(Constants::TWOPI*(np+1)/NPLOT);
				    x  = profile.x + cosa*xi - sina*yi;
				    y  = profile.y + sina*xi + cosa*yi;
				    cpgdraw(x, y);
				}
				cpgpt1(profile.x, profile.y, 1);
				cpgsci(Subs::WHITE);

			    }

			}
			catch(const Ultracam_Error& err){
			    std::cerr << err << std::endl;
			}
			catch(const Subs::Subs_Error& err){
			    std::cerr << err << std::endl;
			}

			// return focus to image plot
			plot.focus();

		    }else if(ret == 'S'){

			try{
			    int wfind;
			    const Ultracam::Windata& win = data[nccd].enclose(x,y,wfind);

			    int ix     = int(win.xcomp(x) + 0.5);
			    int iy     = int(win.ycomp(y) + 0.5);
	      
			    Subs::Format form(6);
			    std::cout << "\nAbsolute position = (" << x << "," << y << ")" << std::endl;
			    std::cout << "Window " << wfind+1 << ", relative pixel (" << ix << "," << iy << "), value = " << form(win[iy][ix]) << std::endl;

			    // Generate stats window
			    int llx  = std::max(0, ix - xbox);
			    int lly  = std::max(0, iy - ybox);
			    int nx   = std::min(win.nx(), ix + xbox - llx + 1);
			    int ny   = std::min(win.ny(), iy + ybox - lly + 1);

			    // Report in terms of window pixels
			    std::cout << 2*xbox+1 << "x" << 2*ybox+1 << " box centred on " << ix << "," << iy << " covers X: " << llx << " to " << llx+nx-1 << ", Y: " << lly << " to " << lly+ny-1;
			    if(2*xbox+1 != nx && 2*ybox+1 != ny){
				std::cout << ", relative window coordinates (truncated in X & Y)" << std::endl;
			    }else if(2*xbox+1 != nx){
				std::cout << " relative window coordinates (truncated in X)" << std::endl;
			    }else if(2*ybox+1 != ny){
				std::cout << " relative window coordinates (truncated in Y)" << std::endl;
			    }else{
				std::cout << " relative window coordinates" << std::endl;
			    }
			    llx      = win.llx() + llx*win.xbin();
			    lly      = win.lly() + lly*win.ybin();
			    Ultracam::Window stats(llx, lly, nx, ny, win.xbin(), win.ybin(), win.nxtot(), win.nytot());
			    std::cout << "Absolute region covered  X: " << stats.xccd(llx) << " to " << stats.xccd(llx+nx-1) << ", Y: " << stats.yccd(lly) << " to " << stats.yccd(lly+ny-1) << std::endl;
	      
			    // Copy over data
			    Ultracam::Windata twin = win.window(stats);

			    float medval = twin.median();	      
			    float mean   = twin.mean();
			    float rms    = twin.rms();

			    std::cout << "npix = " << nx*ny << ", mean = " << form(mean) << ", rms = " << form(rms) << ", median = " << form(medval) << ", min = " << twin.min() << ", max = " << twin.max() << std::endl;
			}
			catch(const Ultracam::Ultracam_Error& err){
			    std::cerr << err << std::endl;
			}
		    }
		}
	    }
	}
    }

    catch(const Ultracam::Input_Error& err){
	std::cerr << "Ultracam::Input_Error exception:" << std::endl;
	std::cerr << err << std::endl;
    }
    catch(const Ultracam::Ultracam_Error& err){
	std::cerr << "Ultracam::Ultracam_Error exception:" << std::endl;
	std::cerr << err << std::endl;
    }
    catch(const Subs::Subs_Error& err){
	std::cerr << "Subs::Subs_Error exception:" << std::endl;
	std::cerr << err << std::endl;
    }
    catch(const std::string& err){
	std::cerr << err << std::endl;
    }
}


