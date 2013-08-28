/*

!!begin
!!title   setaper
!!author  T.R. Marsh
!!created 17 May 2001
!!revised 09 June 2004
!!root    setaper
!!index   setaper
!!descr   sets up photometry apertures
!!css   style.css
!!class   Programs
!!class   Setup
!!head1   setaper for setting up photometric apertures

!!emph{setaper} is an interactive program for defining photometric
apertures. It requires a data frame to exist. It uses 3 circles,
one for the target and two to define an annulus for the sky centred
on it. It dumps an ASCII file, which can be edited directly if
desired and you know what you are doing.

When creating a file you will be asked to define the radii. These
will be kept fixed unless you specifically request a change. For
an old file, the radii will be taken from the first aperture. 

The general idea is to place the cursor at a position or aperture that
you want to do something at/to, and then to hit the appropriate letter.

!!emph{setaper} supports the addition of sky masks attached to the individual
apertures. These are so that if there are stars contaminating the sky annulus you
can remove them rather than relying upon the sky estimator to do it for you.
Here is <a href="aper.gif">an example</a> of what this can look like. See at the
end of this help for an even more complex case which shows what one can do.

!!emph{setaper} also supports the addition of extra apertures to attach to a given
aperture. These are region that you want to include along with your main target. They 
provide a mechanism to include nearby stars when not including them could affect
photometry.

Creation of an aperture file is needed for the main reduction program !!ref{reduce.html}{reduce}.
It might be  worth your reviewing the options supported in !!ref{reduce.html}{reduce} to work out
how you set up your aperture file. Some options for instance require that you define reference
stars.

Various options move or delete apertures. There is no good way to do this graphically
and so I just overplot in red. This can end up looking confusing in which case a replot
can be a good idea at times (e.g. with 'W').

!!head2 Invocation

setaper [device] data newfile aper (rstar rsky1 rsky2) nccd xleft xright ylow yhigh iset (ilow ihigh)/(plow phigh) 
refine [fwhm hwidth readout gain symm (beta) sigrej onedsrch (fwhm1d hwidth1d) fdevice]!!break

!!head2 Command line arguments

!!table

!!arg{device}{The image display device.}

!!arg{data}{Ultracam data file.}

!!arg{newfile}{flag to indicate that the aperture file is new.}

!!arg{aperture}{Aperture file (new or old).}

!!arg{rstar}{If the aperture file is new, you will be prompted to specify the radius of the star aperture. There is also
an interactive option to change this radius during the routine.}

!!arg{rsky1}{If the aperture file is new, you will be prompted to specify the inner radius of the sky annulus}

!!arg{rsky2}{If the aperture file is new, you will be prompted to specify the outer radius of the sky annulus}

!!arg{nccd}{The number of the CCD to set apertures over.}

!!arg{xleft xright}{X range to plot}

!!arg{ylow yhigh}{Y range to plot}

!!arg{iset}{How to set intensity: 'a' for automatic, min to max,
'd' for direct input of range, 'p' for percentiles, probably the most useful option.}

!!arg{ilow ihigh}{the intensity range is i1 to i2 if iset='d'}

!!arg{plow phigh}{p1 to p2 are percentiles to set the intensity if iset = 'p'}

!!arg{refine}{One selects positions byt cursor, but these can be refined automatically by
C(entroiding) which uses a simple cross-correlation with a gaussian of the projections around
the star in /x and Y, G(aussian) which fits 2D gaussian profiles, M(offat) which fits 2D Moffat
profiles or N(one) which just takes the raw positions. For both gaussian and moffat fits there
is also the option of an initial centroid. The many other arguments associated with refining
are hidden by default.}

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

!!arg{fdevice}{Plot device for showing Moffat & symmetrical gaussian fits.
Should be different from the image plot device. e.g. "/xs" or "2/xs" if 
image plot device = "1/xs" otherwise the program will go belly up for reasons that
I cannot quite track down. 'null' to ignore.}

!!table

!!head1 Interactive options

There are several interactive options, all single letters, which are as follows:

!!table

!!arg{A(dd)}{Adds a new aperture at the cursor position.}

!!arg{B(reak)}{Break the link on an aperture. i.e. restore it to its usual state without deleteing it.}

!!arg{C(entre)}{Centre an aperture using whatever centering option is current. Aperture selected with 'A' will
be centred, but this option allows an aperture to be re-centred, for instance after some sky masks have been added
to it, or for a small shift in the image position.  NB the current aperture position is used as the start position, 
not the cursor position which is used only to select the aperture to center.}

!!arg{E(xtra)}{Adds or removes extra star apertures associated with a given aperture. The purpose is to help when there is
a close by styar that for best photometry you might as well include.}

!!arg{F(ull)}{Display the full frame.}

!!arg{I(n)}{Zooms in by a factor 2 around the cursor position.}

!!arg{L(ink)}{Links two apertures. When determining the position of a target, it will
often make sense to do so using a nearby brighter star which is less likely to be
upset by poor signal, cosmic rays etc. To do this place the cursor near to the 
potentially feeble star and hit 'L'. You will then be prompted to specify the
'master target' that you want to link to. NB Really you are linking the 'slave'
to the position of target aperture, not anything else. Thus it is quite permissible to
delete the master aperture, if for instance you only wanted to use it for positioning
and not photometry. Similarly, there is nothing to prevent deletion of the 'slave'.}

!!arg{M(ask)}{Mask a star from the sky annulus of an aperture. You need to click near the 
relevant aperture, then at the position of the star to be masked (it will not be 
centroided), and then at the radius wanted for the mask. This should be done at reasonably
high zoom for best results. The masks can be removed with 'U'.}

!!arg{N(ew)}{New aperture radii.}

!!arg{O(ut)}{Zooms out by a factor 2 around the cursor position.}

!!arg{Q(uit)}{Quit the program.}

!!arg{R(emove)}{Remove an aperture. Place the cursor near to the aperture you
want to remove and then hit 'R'.}

!!arg{S(et)}{Set/unset an aperture as a !!emph{reference star}. This is in order to support the 
aperture moving options of !!ref{reduce.html}{reduce} and for the profile fitting
used in optimal and variable aperture extraction.}

!!arg{U(nmask)}{Removes a mask from an aperture.}

!!arg{W(indow)}{Define a window with the cursor which will then be displayed}

!!table

!!head2 Complex example of an aperture file

This <a href="rxj2130.gif">complex example</a> shows a case with dummy apertures (5, 7, 8) to calibrate
the scattered light from a bright star that will affect the main target (aperture 1). Each of these must 
be linked because, since they have no star, no position can be determined. Each of them also has a mask 
to exclude some of the scattered light. Other apertures are set on comparison stars. Aperture 2 is placed upon
the brightest available comparison which will be uased as a reference. There are some useful scripts for developing
such apertures: !!ref{dummy.html}{dummy} and !!ref{copymask.html}{copymask}.
 
!!end

*/

#include <cstdlib>
#include <cfloat>
#include <climits>
#include <string>
#include <sstream>
#include <map>
#include <fstream>
#include "cpgplot.h"
#include "trm/subs.h"
#include "trm/plot.h"
#include "trm/input.h"
#include "trm/frame.h"
#include "trm/mccd.h"
#include "trm/aperture.h"
#include "trm/ultracam.h"

int main(int argc, char* argv[]){

  using Subs::sqr;
  using Ultracam::Ultracam_Error;
  using Ultracam::Input_Error;

  try{

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Sign-in input variables
    input.sign_in("device",  Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("data",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("newfile", Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("aperture",Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("rstar",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("rsky1",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("rsky2",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("nccd",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("xleft",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("xright",  Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ylow",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("yhigh",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("iset",    Subs::Input::GLOBAL,  Subs::Input::PROMPT);
    input.sign_in("ilow",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ihigh",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("plow",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("phigh",   Subs::Input::GLOBAL, Subs::Input::PROMPT);

    // Profile fit settings
    input.sign_in("refine",  Subs::Input::GLOBAL,  Subs::Input::PROMPT);
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
    input.sign_in("fdevice", Subs::Input::GLOBAL,  Subs::Input::NOPROMPT);

    // Get inputs
    std::string device;
    input.get_value("device", device, "/xs", "plot device");
    std::string name;
    input.get_value("data", name, "run001", "data file to plot");
    Ultracam::Frame data(name), dvar;
    bool newfile;
    input.get_value("newfile", newfile, true, "do you want to open a new aperture file?");
    std::string apname;
    input.get_value("aperture", apname, "aperture", "aperture file name");

    // Create or open an aperture file
    Ultracam::Maperture aper;
    float rstar, rsky1, rsky2;
    std::string entry;
    if(newfile){
      aper = Ultracam::Maperture(data.size());

      std::cout << "For a new file you need to define the aperture radii." << std::endl;

      input.get_value("rstar", rstar, 5.f, 0.f, 1000.f, "radius of star aperture");
      input.get_value("rsky1", rsky1, 10.f, rstar, 1000.f, "inner radius of sky annulus");
      input.get_value("rsky2", rsky2, std::max(20.f, rsky1), rsky1, 1000.f, "outer radius of sky annulus");

    }else{
      aper.rasc(apname);

      if(aper.size() != data.size())
	throw Ultracam_Error("Data frame and aperture file have conflicting CCD numbers");    

      bool found = false;
      for(size_t i=0; i<aper.size(); i++){
	if(aper[i].size()){
	  found = true;
	  rstar = aper[i][0].rstar();
	  rsky1 = aper[i][0].rsky1();
	  rsky2 = aper[i][0].rsky2();
	  break;
	}
      }
      if(!found){
	std::cout << "The file is empty, so you need to define the aperture radii." << std::endl;

	input.get_value("rstar", rstar, 5.f, 0.f, 1000.f, "radius of star aperture");
	input.get_value("rsky1", rsky1, 10.f, rstar, 1000.f, "inner radius of sky annulus");
	input.get_value("rsky2", rsky2, std::max(20.f, rsky1), rsky1, 1000.f, "outer radius of sky annulus");
      }
    }

    int nccd;
    input.get_value("nccd", nccd, int(1), int(1), int(data.size()), "CCD number to set apertures for");
    nccd--;

    float x1, x2, y1, y2;
    x2 = data[nccd].nxtot()+0.5;
    y2 = data[nccd].nytot()+0.5;
    input.get_value("xleft",  x1, 0.5f, 0.5f, x2, "left X limit of plot");
    input.get_value("xright", x2, x2,   0.5f, x2, "right X limit of plot");
    input.get_value("ylow",   y1, 0.5f, 0.5f, y2, "lower Y limit of plot");
    input.get_value("yhigh",  y2, 0.5f, 0.5f, y2, "upper Y limit of plot");
    char iset;
    input.get_value("iset", iset, 'a', "aAdDpP", "set intensity a(utomatically), d(irectly) or with p(ercentiles)?");
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

    // Profile fits
    char refine;
    input.get_value("refine",  refine, 'm', "cCgGmMnN", "refine positions with C(entroid), G(aussian), M(offat) or N(o) fits?");
    refine = std::toupper(refine);
    float fwhm, readout, gain, beta, sigrej, fwhm1d;
    int   hwidth, hwidth1d;
    bool symm, initial_search;
    std::string fdevice;

    if(refine == 'G' || refine == 'M'){
      input.get_value("fwhm",   fwhm,  10.f, 2.f, 1000.f, "initial FWHM for gaussian & moffat profile fits");
      input.get_value("hwidth", hwidth, int(fwhm)+1, 2, INT_MAX, "half-width of region for profile fits (unbinned pixels)");
      input.get_value("readout",   readout,  4.f, 0.f, FLT_MAX, "readout noise for profile fits (RMS ADU)");
      input.get_value("gain",   gain,  1.f, 0.01f, 100.f, "electrons/ADU for profile fits");
      input.get_value("symm",  symm, true, "force symmetric profiles?");
      if(refine == 'M')
	input.get_value("beta",  beta, 3.f, 1.f, 1000.f, "default beta exponent for moffat fits");
      input.get_value("sigrej",  sigrej, 5.f, 0.f, FLT_MAX, "threshold for masking pixels (in sigma)");
      input.get_value("onedsrch",  initial_search, true, "carry out an initial 1D position tweak for moffat or gaussian fits?");
      if(initial_search){
	input.get_value("fwhm1d",   fwhm1d,  fwhm, 2.f, 1000.f, "FWHM for 1D search");
	input.get_value("hwidth1d", hwidth1d, hwidth, int(fwhm1d)+1, INT_MAX, "half-width of 1D search region");
      }
      input.get_value("fdevice", fdevice, "2/xs", "plot device for profile fits ('null' to ignore)");
    }else if(refine == 'C'){
      input.get_value("fwhm1d",   fwhm1d,  fwhm, 2.f, 1000.f, "FWHM for 1D search");
      input.get_value("hwidth1d", hwidth1d, hwidth, int(fwhm1d)+1, INT_MAX, "half-width of 1D search region");
      fdevice = "null";
    }

    // Save defaults now because one often wants to terminate this program early
    input.save();

    dvar = data;
    dvar.max(0);
    dvar /= gain;
    dvar += readout*readout;

    // Empty skymask for the routine fit_plot_profile
    std::vector<Ultracam::sky_mask> skymask;

    Subs::Plot plot(device), fplot;
    if(fdevice != "null") fplot.open(fdevice);

    plot.focus();
    cpgsch(1.5);
    cpgscf(2);
    Ultracam::plot_images(data, x1, x2, y1, y2, false, 'X', iset, ilow, ihigh, plow, phigh, 
			  true, name, nccd, false);
    Ultracam::plot_apers(aper, x1, x2, y1, y2, false, 'X', nccd);
    
    cpgsci(Subs::WHITE);
    float x = (x1+x2)/2., y = (y1+y2)/2.;
    char ret = 'X';
    std::string lab;

    std::cout << "Position the cursor to add/delete/etc apertures and\n"
	 << "hit the appropriate letter.\n" << std::endl;

    // Now aperture addition loop

    while(ret != 'Q'){
      ret = 'X';

      // Next lines define the prompt:
      std::cout << "A(dd), C(entre), N(ew), I(n), O(ut), F(ull), W(indow), ";

      if(aper[nccd].size() > 1) std::cout << "L(ink), B(reak link), ";
      if(aper[nccd].size() > 0) std::cout << "E(xtra), R(emove), S(et), M(ask), U(nmask), ";
      std::cout << "Q(uit)" << std::endl; 

      // Now get cursor input.

      if(!cpgcurs(&x,&y,&ret)) throw Ultracam_Error("Cursor error");

      ret = toupper(ret);

      if(ret == 'A'){
	
	try{
	  std::cout << "\n                      Initial cursor position = " << x << ", " << y << std::endl;

	  // Refine object position (this does the centroiding)
	  float sky=0., peak;
	  double xm = x, ym = y;
	  if(refine == 'G' || refine == 'M' || refine == 'C'){
	    Ultracam::profit_init(data[nccd], dvar[nccd], xm, ym, refine == 'C' || initial_search, fwhm1d, hwidth1d, hwidth, sky, peak, false);
	    std::cout << "Refined by 1D collapse and cross-correlation to " << xm << ", " << ym << std::endl;
	  }	  
	  
	  if(refine == 'G' || refine == 'M'){

	    // obtain initial value of 'a'
	    double a = 1./2./Subs::sqr(fwhm/Constants::EFAC);
	    
	    Ultracam::Ppars profile;
	    if(refine == 'G'){	    
	      // Gaussian fit section.
	      std::cout << "\nFitting 2D gaussian ...\n" << std::endl;
	      profile.set(sky, xm, ym, peak, a, 0., a, symm);
	      
	    }else if(refine == 'M'){	    
	      // Moffat fit section.
	      std::cout << "\nFitting moffat profile ...\n" << std::endl;
	      profile.set(sky, xm, ym, peak, a, 0., a, beta, symm);
	      
	    }
	    
	    Ultracam::Iprofile iprofile;
	    Ultracam::fit_plot_profile(data[nccd], dvar[nccd], profile, false, false, 0., 0., skymask, fwhm1d, 
				       hwidth1d, hwidth, fplot, sigrej, iprofile, true);
	    
	    // adjust defaults for next time
	    x    = profile.x;
	    y    = profile.y;
	    fwhm = iprofile.fwhm;
	    if(refine == 'M') beta = profile.beta;
	  
	    // return focus to image plot
	    plot.focus();
	  }
	  
	  // Add in new aperture
	  aper[nccd].push_back(Ultracam::Aperture(x,y,0,0,rstar,rsky1,rsky2));
	  pgline(aper[nccd][aper[nccd].size()-1]);
	  lab  = "";
	  lab += aper[nccd].size();
	  pgptxt(aper[nccd][aper[nccd].size()-1],lab);
	  
	}
	catch(const Ultracam_Error& err){
	  std::cerr << err << std::endl;
	}
	catch(const Subs::Subs_Error& err){
	  std::cerr << err << std::endl;
	}
	catch(const std::string& str){
	  std::cerr << str << std::endl;
	}
	
	// return focus to image plot
	plot.focus();
	
      }else if(aper[nccd].size() && ret == 'R' ){
	
	// Remove an aperture	
	Ultracam::Aperture a;
	if(aper[nccd].del_obj(x,y,a)){
	  cpgsci(Subs::RED);
	  pgline(a);
	  cpgsci(Subs::WHITE);
	}

      }else if(aper[nccd].size() && ret == 'C'){
	
	Ultracam::CCD<Ultracam::Aperture>::optr app;
	if(aper[nccd].selected(x,y,app)){

	  x = app->xpos();
	  y = app->ypos();

	  // Refine object position (this does the centroiding)
	  float sky=0., peak;
	  double xm=x, ym=y;
	  if(refine == 'G' || refine == 'M' || refine == 'C'){
	    Ultracam::profit_init(data[nccd], dvar[nccd], xm, ym, refine == 'C' || initial_search, fwhm1d, hwidth1d, hwidth, sky, peak, false);
	    std::cout << "Refined by 1D collapse and cross-correlation to " << xm << ", " << ym << std::endl;
	  }	  
	  
	  if(refine == 'G' || refine == 'M'){
	    
	    // obtain initial value of 'a'
	    double a = 1./2./Subs::sqr(fwhm/Constants::EFAC);
	    
	    Ultracam::Ppars profile;
	    if(refine == 'G'){	    
	      // Gaussian fit section.
	      std::cout << "\nFitting 2D gaussian ...\n" << std::endl;
	      profile.set(sky, xm, ym, peak, a, 0., a, symm);
	      
	    }else if(refine == 'M'){	    
	      // Moffat fit section.
	      std::cout << "\nFitting moffat profile ...\n" << std::endl;
	      profile.set(sky, xm, ym, peak, a, 0., a, beta, symm);
	      
	    }
	    
	    Ultracam::Iprofile iprofile;
	    Ultracam::fit_plot_profile(data[nccd], dvar[nccd], profile, false, false, 0., 0., app->mask(), fwhm1d, 
				       hwidth1d, hwidth, fplot, sigrej, iprofile, true);
	    
	    // adjust defaults for next time
	    x    = profile.x;
	    y    = profile.y;
	    fwhm = iprofile.fwhm;
	    if(refine == 'M') beta = profile.beta;
	    
	    // return focus to image plot
	    plot.focus();
	  }
	  
	  cpgsci(Subs::RED);
	  pgline(*app);
	  cpgsci(Subs::WHITE);

	  // Update aperture
	  if(app->linked()){
	    app->set_xoff(x-app->xref());
	    app->set_yoff(y-app->yref());
	  }else{
	    app->set_xref(x);
	    app->set_yref(y);
	  }

	  pgline(*app);
	}
	
	// return focus to image plot
	plot.focus();

      }else if(aper[nccd].size() && ret == 'M' ){

	Ultracam::CCD<Ultracam::Aperture>::optr app;
	if(aper[nccd].selected(x,y,app)){

	  float xref = app->xpos();
	  float yref = app->ypos();
	  char reply = 'z';
	  while(reply != 'M' && reply != 'Q'){
	    std::cout << "Now position on star you want to mask from the sky annulus, hit 'M' to mask, 'Q' to quit" << std::endl;
	    float xm=xref, ym=yref;
	    if(cpgband(1,1,xref,yref,&xm,&ym,&reply)){
	      reply = std::toupper(reply);
	      if(reply == 'M'){
		while(reply != 'S' && reply != 'Q'){
		  std::cout << "Finally position at edge of masking circle, hit 'S' to set the radius, 'Q' to quit" << std::endl;
		  float xr=xm, yr=ym;
		  if(cpgband(1,1,xm,ym,&xr,&yr,&reply)){
		    reply = std::toupper(reply);
		    if(reply == 'S'){
		      float rmask = sqrt(Subs::sqr(xr-xm)+Subs::sqr(yr-ym));
		      app->push_back(Ultracam::sky_mask(xm-xref,ym-yref,rmask));
		      pgline(*app);
		    }
		  }else{
		    std::cerr << "Cursor error" << std::endl;
		    break;
		  }
		}
		reply = 'Q';
	      }
	    }else{
	      std::cerr << "Cursor error" << std::endl;
	      break;
	    }
	  }
	}else{
	  std::cerr << "Failed to set cursor near enough to any aperture" << std::endl;
	}

      }else if(aper[nccd].size() && ret == 'U' ){

	// Search through all apertures to see what is nearest mask, if any.
	float rmin = 1e30;
	int naper_min = -1, nmask_min = 0;
	for(size_t na=0; na<aper[nccd].size(); na++){
	  for(int nm=0; nm<aper[nccd][na].nmask(); nm++){
	    float r = sqrt(Subs::sqr(x-aper[nccd][na].xpos()-aper[nccd][na].mask(nm).x) + 
			   Subs::sqr(y-aper[nccd][na].ypos()-aper[nccd][na].mask(nm).y));
	    if(r < rmin){
	      rmin = r;
	      naper_min = na;
	      nmask_min = nm;
	    }
	  }
	}
	if(naper_min >= 0 && rmin < aper[nccd][naper_min].mask(nmask_min).z + 10){

	  cpgsci(Subs::RED);
	  pgline(aper[nccd][naper_min]);
	  aper[nccd][naper_min].del_mask(nmask_min);
	  cpgsci(Subs::WHITE);
	  pgline(aper[nccd][naper_min]);

	}else{
	  std::cerr << "No mask exists or cursor not set close enough to any mask" << std::endl;
	}
	
      }else if(aper[nccd].size() && ret == 'E' ){

	Ultracam::CCD<Ultracam::Aperture>::optr app;
	if(aper[nccd].selected(x,y,app)){

	  float xref = app->xpos();
	  float yref = app->ypos();
	  char reply = 'z';
	  while(reply != 'A' && reply != 'Q' && reply != 'R'){
	    std::cout << "Position on the star you want to add/remove as an extra star aperture, hit 'A' to add, 'R' to remove, 'Q' to quit" << std::endl;
	    float xm=xref, ym=yref;
	    if(cpgband(1,1,xref,yref,&xm,&ym,&reply)){
	      reply = std::toupper(reply);
	      if(reply == 'A'){
		app->push_back(Ultracam::extra_star(xm-xref,ym-yref));
	      }else if(reply == 'R'){
		if(app->nextra() == 0){
		  std::cerr << "No extra star apertures to remove" << std::endl;
		}else{
 		  float r = 0, rmin = 1.e10;
		  int imin = -1;
		  for(int i=0; i<app->nextra(); i++){
		    r = sqrt(Subs::sqr(xref+app->extra(i).x-xm) + Subs::sqr(yref+app->extra(i).y-ym));
		    if(r < rmin){
		      rmin = r;
		      imin = i;
		    }
		  }
		  if(r < 2.*app->rstar()){
		    cpgsci(Subs::RED);
		    pgline(*app);
		    app->del_extra(imin);
		    cpgsci(Subs::WHITE);
		    pgline(*app);

		  }else{
		    std::cerr << "Cursor not near enough to any extra aperture for deletion to go ahead" << std::endl;
		  }
		}
	      }
	    }else{
	      std::cerr << "Cursor error" << std::endl;
	      break;
	    }
	  }
	}else{
	  std::cerr << "Failed to set cursor near enough to any aperture" << std::endl;
	}

      }else if(ret == 'N'){
	
	// Set new aperture radii
	
	input.get_value("rstar", rstar, 5.f, 0.f, 1000.f, "radius of star aperture");
	input.get_value("rsky1", rsky1, 10.f, rstar, 1000.f, "inner radius of sky annulus");
	input.get_value("rsky2", rsky2, std::max(20.f, rsky1), rsky1, 1000.f, "outer radius of sky annulus");
	
      }else if(ret == 'F'){
	
	// Re-plot full frame

	x1 = 0.5;
	x2 = data[nccd].nxtot()+0.5;
	y1 = 0.5;
	y2 = data[nccd].nytot()+0.5;
	cpgeras();
	Ultracam::plot_images(data, x1, x2, y1, y2, false, 'X', iset, ilow, ihigh, plow, phigh,
			      true, name, nccd, false);
	Ultracam::plot_apers(aper, x1, x2, y1, y2, false, 'X', nccd);

      }else if(ret == 'W'){

	// Select a region to window

	std::cout << "Pick first corner of window" << std::endl;
	char reply;
	float xc1 = x, yc1 = y;
	if(cpgcurs(&xc1,&yc1,&reply)){
	  std::cout << "Set other corner (Q to quit)" << std::endl;
	  float xc2, yc2;
	  xc2 = xc1;
	  yc2 = yc1;
	  if(cpgband(2,1,xc1,yc1,&xc2,&yc2,&reply)){
	    reply = toupper(reply);
	    if(reply != 'Q'){
	      x1 = std::min(xc1, xc2);
	      x2 = std::max(xc1, xc2);
	      y1 = std::min(yc1, yc2);
	      y2 = std::max(yc1, yc2);
	      cpgeras();
	      Ultracam::plot_images(data, x1, x2, y1, y2, false, 'X', iset, ilow, ihigh, plow, phigh, true, name, nccd, false);
	      Ultracam::plot_apers(aper, x1, x2, y1, y2, false, 'X', nccd);
	      
	      x = (x1+x2)/2., y = (y1+y2)/2.;
	    }
	  }else{
	    std::cerr << "Cursor error" << std::endl;
	  }
	}else{
	  std::cerr << "Cursor error" << std::endl;
	}

      }else if(ret == 'I'){
	
	// Zoom in
	
	float xr = (x2-x1)/2., yr = (y2-y1)/2.; 
	
	x1 = x - xr/2.;
	x2 = x + xr/2.;
	y1 = y - yr/2.;
	y2 = y + yr/2.;

	cpgeras();
	Ultracam::plot_images(data, x1, x2, y1, y2, false, 'X', iset, ilow, ihigh, plow, phigh,
			      true, name, nccd, false);
	Ultracam::plot_apers(aper, x1, x2, y1, y2, false, 'X', nccd);

      }else if(ret == 'O'){

	// Zoom out

	float xr = (x2-x1)/2., yr = (y2-y1)/2.; 
	
	x1 = x - 2.*xr;
	x2 = x + 2.*xr;
	y1 = y - 2.*yr;
	y2 = y + 2.*yr;
	
	cpgeras();
	Ultracam::plot_images(data, x1, x2, y1, y2, false, 'X', iset, ilow, ihigh, plow, phigh,
			      true, name, nccd, false);
	Ultracam::plot_apers(aper, x1, x2, y1, y2, false, 'X', nccd);

      }else if(aper[nccd].size() && ret == 'S'){

	// Set or unset an aperture as a reference one
	Ultracam::CCD<Ultracam::Aperture>::optr sel;
	if(aper[nccd].selected(x,y,sel)){
	  if(sel->xoff() == 0. && sel->yoff() == 0.){
	    sel->set_ref(!sel->ref());
	    pgline(*sel);
	    if(sel->ref()){
	      std::cout << "Aperture selected as a reference source" << std::endl;
	    }else{
	      std::cout << "Aperture deselected as a reference source" << std::endl;
	    }
	  }else{
	    std::cerr << "Cannot selected a linked aperture as a reference source" << std::endl;
	  }
	}else{
	  std::cerr << "Not close to enough to any aperture to marks it for referencing." << std::endl;
	}
	
      }else if(aper[nccd].size() > 1 && ret == 'L'){

	Ultracam::CCD<Ultracam::Aperture>::optr slave;
	if(aper[nccd].selected(x,y,slave)){
	  if(slave->ref()){
	    std::cerr << "Cannot link a reference aperture!" << std::endl;
	  }else{
	    float xref = slave->xpos();
	    float yref = slave->ypos();
	    char reply = 'z';
	    while(reply != 'L' && reply != 'Q'){
	      std::cout << "Position near the aperture of the master target you wish to link to," << std::endl;
	      std::cout << "then enter 'L' to link, or 'Q' to quit." << std::endl;
	      if(cpgband(1,1,xref,yref,&x,&y,&reply)){
		reply = toupper(reply);
		if(reply == 'L'){
		  Ultracam::CCD<Ultracam::Aperture>::optr master;
		  if(aper[nccd].selected(x,y,master)){
		    float xp = slave->xpos();
		    float yp = slave->ypos();
		    slave->set_xref(master->xpos());
		    slave->set_yref(master->ypos());
		    slave->set_xoff(xp - master->xpos());
		    slave->set_yoff(yp - master->ypos());
		    Ultracam::plot_apers(aper, x1, x2, y1, y2, false, 'X', nccd);
		  }else{
		    std::cerr << "Failed to set cursor near enough to any other aperture" << std::endl;
		  }
		}
	      }else{
		std::cerr << "Cursor error" << std::endl;
		break;
	      }
	    }
	  }
	  
	}else{
	  std::cerr << "Failed to set cursor near enough to any aperture to allow linkage to proceed" << std::endl;
	}

      }else if(aper[nccd].size() > 1 && ret == 'B'){

	Ultracam::CCD<Ultracam::Aperture>::optr slave;
	if(aper[nccd].selected(x,y,slave)){
	  if(slave->xoff() == 0 && slave->yoff() == 0){
	    std::cerr << "Aperture not linked." << std::endl;
	  }else{
	    float xref = slave->xpos();
	    float yref = slave->ypos();
	    slave->set_xref(xref);
	    slave->set_yref(yref);
	    slave->set_xoff(0.);
	    slave->set_yoff(0.);
	  }
	}else{
	  std::cerr << "Failed to set cursor near enough to any aperture to remove any link" << std::endl;
	}

      }else if(ret != 'Q'){
	std::cerr << "Input = " << ret << " not recognised." << std::endl;
      }
    }
    
    // Dump the result
    aper.wasc(apname);
    
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



