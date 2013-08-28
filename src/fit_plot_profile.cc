#include "trm/subs.h"
#include "trm/ccd.h"
#include "trm/ultracam.h"

/** fit_plot_profile is a high-level routine that carries out initialisation,
 * position tweaking, fitting, rejection and plotting of gaussian or moffat profiles.
 * It is designed to be usable from different routines.
 *
 * \param data           the CCD frame to fit
 * \param dvar           equivalent variance frame
 * \param profile        the profile parameters
 * \param initial_search true/false to carry out a 1D position tweak at the start
 * \param initialise     if initialise=false, whatever position and sky value are set inside 
 * profile are used to start with. If initialise=true, the next two parameters, \c xinit and \c yinit
 * are used for the position, and the sky will be estimated separately. You must in either case have
 * initialised the shape parameters in profile before entering the routine.
 * \param xinit          if initialise=true, xinit is the start position in X
 * \param yinit          if initialise=true, yinit is the start position in Y
 * \param skymask        a vector of masks to reduce the effect that nearby stars might have on the fits. This will be
 * applied after the first fit has got the position roughly right. It will be applied in two
 * steps first with 0.7 of the full radius (i.e. about 50% of the area) and then at the full radius of each mask.
 * This is to reduce the problem that shifts in fitted position will increase the number of pixels to be masked.
 * \param fwhm1d         the FWHM of the profile used if there is a 1D position tweak
 * \param hwidth1d       the half-width of the box used for the 1D position tweak
 * \param hwidth         the half-width of the box used for the profile fit. 
 * \param fplot          if open, then plots of the fits will be made.
 * \param sigrej         the number of RMS ro reject at
 * \param iprofile       structure with lots of returned information that could be useful for printing.
 * \param print          print out information on fit or not
 * \exception This routine can throw Ultracam_Error exceptions for a number of different reasons.
 */

void Ultracam::fit_plot_profile(const Image& data, const Image& dvar, Ppars& profile, bool initial_search, bool initialise,
				float xinit, float yinit, const std::vector<sky_mask>& skymask, float fwhm1d, int hwidth1d, 
				int hwidth, const Subs::Plot& fplot, float sigrej, Iprofile& iprofile, bool print){

  // override whatever is currently set inside profile
  if(initialise){
    profile.x   = xinit;
    profile.y   = yinit;
    profile.sky = 0.;
  }

  // work out (if there is) a reference to an appropriate window
  const Windata& win = data.enclose(profile.x, profile.y);
  const Windata& var = dvar.enclose(profile.x, profile.y);
  	      
  // Adjust initial position
  if(initial_search){
    
    double xpos, ypos;
    Ultracam::pos_tweak(win, var, fwhm1d, hwidth1d, profile.x, profile.y, xpos, ypos);
    profile.x = xpos;
    profile.y = ypos;

    xpos = win.xcomp(xpos);
    ypos = win.ycomp(ypos);

    int ix;
    ix = xpos + 0.5 > 0. ? int(xpos+0.5) : 0;
    int iy;
    iy = ypos + 0.5 > 0. ? int(ypos+0.5) : 0;
    profile.height = win[iy][ix] - profile.sky;
  }

  // Define fit region
  int hx = hwidth/win.xbin();
  hx = hx > 2 ? hx : 2;
  int hy = hwidth/win.ybin();
  hy = hy > 2 ? hy : 2;
  
  int xlo = int(win.xcomp(profile.x) + 0.5) - hx;
  xlo = xlo < 0 ? 0 : xlo;
  int xhi = int(win.xcomp(profile.x) + 0.5) + hx;
  xhi = xhi < win.nx() ? xhi : win.nx()-1;
  int ylo = int(win.ycomp(profile.y) + 0.5) - hy;
  ylo = ylo < 0 ? 0 : ylo;
  int yhi = int(win.ycomp(profile.y) + 0.5) + hy;
  yhi = yhi < win.ny() ? yhi : win.ny()-1;

  if(initialise){
    // Estimate sky background from 20%-ile
    float buff[(yhi-ylo+1)*(xhi-xlo+1)];
    unsigned long np = 0;
    for(int iy=ylo; iy<=yhi; iy++)
      for(int ix=xlo; ix<=xhi; ix++)
	buff[np++] = win[iy][ix];
    profile.sky = Subs::select(buff, np, (unsigned long)(np*0.2));
    
    if(!initial_search){
      profile.height = win[ylo][xlo];
      for(int iy=ylo; iy<=yhi; iy++)
	for(int ix=xlo; ix<=xhi; ix++)
	  if(win[iy][ix] > profile.height) profile.height = win[iy][ix];
    }
    profile.height -= profile.sky;
  }

  iprofile.xlo = xlo;
  iprofile.xhi = xhi;
  iprofile.ylo = ylo;
  iprofile.yhi = yhi;
	      
  // Create a window of 1-sigma RMS values, avoid copying the whole array to save time
  Windata sigwin((Window&)win);
  Ultracam::internal_data rmax = win[ylo][xlo];
  for(int iy=ylo; iy<=yhi; iy++){
    for(int ix=xlo; ix<=xhi; ix++){
      sigwin[iy][ix] = sqrt(var[iy][ix]);
      rmax = rmax > win[iy][ix] ? rmax : win[iy][ix];
    }
  }
  iprofile.rmax = rmax;
	      
  // Fit
  double alambda, alambdaold;
  double oldchisq, chisq, sfac;
  int nits = 0, nrej = 0, nrejected = 1, ncycle = 0, ndof = (yhi-ylo)*(xhi-xlo) - profile.npar();
  
  if(ndof < 5) throw Ultracam::Ultracam_Error("Oops! Too few points for profile fit");
  
  while(nits < 4 && nrejected > 0){
    alambda  = -1.;
    alambdaold  = -2.;
    int ncount   = 0;
    oldchisq = 1.;
    chisq    = 0.;
    while((oldchisq - chisq > 0.001 || alambda > alambdaold || alambda > 0.001) && ncount < 100){
      alambdaold = alambda;
      oldchisq   = chisq;

      if(profile.ptype == Ppars::GAUSSIAN){
	Ultracam::fitgaussian(win, sigwin, xlo, xhi, ylo, yhi, profile, chisq, alambda, iprofile.covar);
      }else if(profile.ptype == Ppars::MOFFAT){
	Ultracam::fitmoffat(win, sigwin, xlo, xhi, ylo, yhi, profile, chisq, alambda, iprofile.covar);
      }
      ncount++;
    }
    nits += ncount;

    // Rejection loop
    if(nits < 4){
      sfac = 1. + 1./pow(2.,nits);
    }else{
      sfac = 1.;
    }
    float scale = sfac*sigrej*sqrt(chisq/ndof);
    if(profile.ptype == Ppars::GAUSSIAN){
      Ultracam::gauss_reject(win, sigwin, xlo, xhi, ylo, yhi, profile, scale, nrejected);
    }else if(profile.ptype == Ppars::MOFFAT){
      Ultracam::moffat_reject(win, sigwin, xlo, xhi, ylo, yhi, profile, scale, nrejected);
    }

    // Sky mask
    if(skymask.size() > 0 && ncycle < 2){

      // Apply the sky mask in two steps, the central 50% of pixels
      // first and then the rest. This is to allow for small shifts.
      // The application only occurs in the first 2 cycles to prevent an 
      // endless series of maskings taking place

      float fac;
      if(ncycle == 0)
	fac = 0.7f;
      else
	fac = 1.f;
      for(int iy=ylo; iy<=yhi; iy++){
	float yoff = win.yccd(iy)-profile.y;
	for(int ix=xlo; ix<=xhi; ix++){
	  if(sigwin[iy][ix] > 0.){
	    float xoff  = win.xccd(ix)-profile.x;
	    bool masked = false;
	    for(size_t nm=0; nm<skymask.size(); nm++){
	      if(Subs::sqr(xoff - skymask[nm].x) + Subs::sqr(yoff - skymask[nm].y) < Subs::sqr(fac*skymask[nm].z)){
		masked = true;
		break;
	      }
	    }
	    if(masked){
	      sigwin[iy][ix] = - sigwin[iy][ix];
	      nrejected++;
	    }
	  }
	}
      }
    }

    nrej += nrejected;
    ndof -= nrejected;
    ncycle++;
    if(ndof < 5) throw Ultracam::Ultracam_Error("fit_profile_fit: too few points std::left for profile fit");
  }

  // Avoid proceeding if values are silly
  if(profile.a < 1.e-4 || (profile.ptype == Ppars::MOFFAT && profile.beta < 0.5))
    throw Ultracam::Ultracam_Error("fit_plot_profile: the fit has failed");

  
  // Set alambda = 0. to get the covariances right
  alambda = 0.;
  if(profile.ptype == Ppars::GAUSSIAN){
    Ultracam::fitgaussian(win, sigwin, xlo, xhi, ylo, yhi, profile, chisq, alambda, iprofile.covar);
  }else if(profile.ptype == Ppars::MOFFAT){
    Ultracam::fitmoffat(win, sigwin, xlo, xhi, ylo, yhi, profile, chisq, alambda, iprofile.covar);
  }

  iprofile.chisq  = chisq;
  iprofile.ndof   = ndof;
  iprofile.nrej   = nrej;
  iprofile.nits   = nits;
  iprofile.ncycle = ncycle;

  if(profile.symm){

    // symmetrical profile
    if(profile.ptype == Ppars::GAUSSIAN){
      iprofile.fwhm   = Constants::EFAC/sqrt(2.*profile.a); 
      iprofile.efwhm  = Constants::EFAC/sqrt(2.)*sqrt(iprofile.covar[profile.a_index()][profile.a_index()])/Subs::sqr(profile.a);

    }else if(profile.ptype == Ppars::MOFFAT){

      double fac1   = pow(2.,1./profile.beta);
      iprofile.fwhm = 2.*sqrt((fac1-1.)/profile.a);
      double fac2   = log(2.)*fac1/(fac1-1.)/Subs::sqr(profile.beta);
      double fac3   = 1./profile.a;

      iprofile.efwhm  = 0.5*sqrt(fac2*fac2*iprofile.covar[profile.beta_index()][profile.beta_index()] +
				 2.*fac2*fac3*iprofile.covar[profile.beta_index()][profile.a_index()] +
				 fac3*fac3*iprofile.covar[profile.a_index()][profile.a_index()]);
    }

    // Plot fit if wanted
    if(fplot.is_open()){
      fplot.focus();
      float r, y = 0;
      cpgsci(4);
      cpgask(0);
      cpgenv(0., 3.*iprofile.fwhm, 0., 1.5*rmax, 0, 0);
      cpgsci(Subs::RED);
      cpglab("Pixels from centre of profile", "Counts", "Fit");
      cpgmove(0., profile.sky + profile.height);
      const int NR=200;	      
      for(int nr=1; nr<NR; nr++){
	r = 3.*iprofile.fwhm*nr/(NR-1);
	if(profile.ptype == Ppars::GAUSSIAN){
	  y = profile.sky + profile.height*exp(-profile.a*r*r);  
	}else if(profile.ptype == Ppars::MOFFAT){
	  y = profile.sky + profile.height/pow(1.+profile.a*r*r,profile.beta);
	}
	cpgdraw(r, y);
      }
      cpgsci(1);
      for(int iy=ylo; iy<=yhi; iy++){
	for(int ix=xlo; ix<=xhi; ix++){
	  r = sqrt(pow(win.xccd(ix)-profile.x,2) + pow(win.yccd(iy)-profile.y,2));
	  if(sigwin[iy][ix] > 0.)
	    cpgsci(Subs::WHITE);
	  else
	    cpgsci(Subs::RED);
	  cpgpt1(r, win[iy][ix], 1);
	}
      }
    }
    
  }else{
    
    // elliptical profile
    float lin        = profile.a + profile.c;
    float csqrt      = sqrt(Subs::sqr(profile.a -profile.c) + 4.*Subs::sqr(profile.b));
    float lambda_min = (lin - csqrt)/2.;
    float lambda_max = (lin + csqrt)/2.;
    if(profile.ptype == Ppars::GAUSSIAN){
      iprofile.fwhm_min  = Constants::EFAC/sqrt(2.*lambda_max); 
      iprofile.fwhm_max  = Constants::EFAC/sqrt(2.*lambda_min);
    }else if(profile.ptype == Ppars::MOFFAT){
      double fac1       = 2.*sqrt(pow(2.,1./profile.beta)-1.);
      iprofile.fwhm_min = fac1/sqrt(lambda_max);
      iprofile.fwhm_max = fac1/sqrt(lambda_min);
    }
    iprofile.fwhm = (iprofile.fwhm_min+iprofile.fwhm_max)/2.;

    float angle      = 360.*atan2(lambda_min-profile.a, profile.b)/Constants::TWOPI;
    angle = angle < 0.   ? angle + 360. : angle;
    angle = angle > 180. ? angle - 180. : angle;
    iprofile.angle = angle;

    // Plot fit if wanted
    if(fplot.is_open()){
      fplot.focus();
      float r, y = 0;
      cpgsci(4);
      cpgask(0);
      cpgenv(0., 3.*iprofile.fwhm_max, 0., 1.5*rmax, 0, 0);
      cpgsci(Subs::RED);
      cpglab("Pixels from centre of profile", "Counts", "Fit");

      // Plot profiles for minimum and maximum FWHM
      const int NR=200;	      
      cpgmove(0., profile.sky + profile.height);
      for(int nr=1; nr<NR; nr++){
	r = 3.*iprofile.fwhm_max*nr/(NR-1);
	if(profile.ptype == Ppars::GAUSSIAN){
	  y = profile.sky + profile.height*exp(-lambda_max*r*r);  
	}else if(profile.ptype == Ppars::MOFFAT){
	  y = profile.sky + profile.height/pow(1.+lambda_max*r*r,profile.beta);
	}
	cpgdraw(r, y);
      }
      cpgmove(0., profile.sky + profile.height);
      for(int nr=1; nr<NR; nr++){
	r = 3.*iprofile.fwhm_max*nr/(NR-1);
	if(profile.ptype == Ppars::GAUSSIAN){
	  y = profile.sky + profile.height*exp(-lambda_min*r*r);  
	}else if(profile.ptype == Ppars::MOFFAT){
	  y = profile.sky + profile.height/pow(1.+lambda_min*r*r,profile.beta);
	}
	cpgdraw(r, y);
      }

      // Plot points
      cpgsci(1);
      for(int iy=ylo; iy<=yhi; iy++){
	for(int ix=xlo; ix<=xhi; ix++){
	  r = sqrt(pow(win.xccd(ix)-profile.x,2) + pow(win.yccd(iy)-profile.y,2));
	  if(sigwin[iy][ix] > 0.)
	    cpgsci(Subs::WHITE);
	  else
	    cpgsci(Subs::RED);
	  cpgpt1(r, win[iy][ix], 1);
	}
      }
    }
  }

  iprofile.esky  = sqrt(iprofile.covar[profile.sky_index()][profile.sky_index()]);
  iprofile.epeak = sqrt(iprofile.covar[profile.height_index()][profile.height_index()]);
  iprofile.ex    = sqrt(iprofile.covar[profile.x_index()][profile.x_index()]);
  iprofile.ey    = sqrt(iprofile.covar[profile.y_index()][profile.y_index()]);

  if(print){
    // Report some information
    std::cout << "Fit region: x: " << iprofile.xlo << " to " << iprofile.xhi << ", y: " 
	 << iprofile.ylo << " to " << iprofile.yhi << std::endl;
    std::cout << "Maximum value in fit region = " << std::setprecision(5) << iprofile.rmax << std::endl;
    std::cout << "Chi**2 = " << std::setprecision(5) << iprofile.chisq << " with " << iprofile.ndof << " degrees of freedom, after " 
	 << iprofile.nits << " iterations, " << iprofile.ncycle << " reject cycles and " << iprofile.nrej 
	 << " points rejected at " << sigrej << " sigma." << std::endl;
    
    if(profile.symm){
      
      std::cout << "\nFWHM= " << std::setprecision(4) << iprofile.fwhm << "+/-" << std::setprecision(2) << iprofile.efwhm 
	   << " (unbin), sky= " << std::setprecision(4) << profile.sky  << "+/-" << std::setprecision(2) << iprofile.esky;

    }else{
      
      std::cout << "\nMinor & major axis FWHM and major axis angle (anti-clockwise from X axis) = " 
	   << std::setprecision(4) << iprofile.fwhm_min << ", " << iprofile.fwhm_max << " unbinned pixels, " 
	   << iprofile.angle << " degrees\nSky= " << std::setprecision(4) << profile.sky  << "+/-" 
	   << std::setprecision(2) << iprofile.esky;
      
    }

    if(profile.ptype == Ppars::MOFFAT){
      iprofile.ebeta = sqrt(iprofile.covar[profile.beta_index()][profile.beta_index()]);
      std::cout << ", beta= " << std::setprecision(4) << profile.beta << "+/-" << std::setprecision(2) << iprofile.ebeta;
    }

    std::cout << ", peak= "     << std::setprecision(4) << profile.height << "+/-" << std::setprecision(2) << iprofile.epeak 
	 << ", x= "               << std::setprecision(5) << profile.x      << "+/-" << std::setprecision(2) << iprofile.ex
	 << ", y= "               << std::setprecision(5) << profile.y      << "+/-" << std::setprecision(2) << iprofile.ey;

    if(profile.symm){
      if(profile.ptype == Ppars::GAUSSIAN){
	std::cout << ", area= " << std::setprecision(4) << Constants::PI*profile.height/profile.a;
      }else if(profile.ptype == Ppars::MOFFAT){
	std::cout << ", area= " << std::setprecision(4) << Constants::PI*profile.height/profile.a/(profile.beta-1);
      }
    }
    std::cout << std::setprecision(0) << "\n" << std::endl;
  }
}
