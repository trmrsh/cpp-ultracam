#include "trm_buffer2d.h"
#include "trm_ultracam.h"
#include "trm_ccd.h"
#include "trm_mccd.h"
#include "trm_frame.h"
#include "trm_specap.h"

// Function for polynomial fits needed by llsqr

class Poly : public Subs::Llfunc {
public:

  // Constructor.
  Poly(int npoly, double xstart, double xend) : npoly(npoly), xmid((xstart+xend)/2.), hrange((xend-xstart)/2.) {}

  ~Poly(){}

  int get_nfunc() const {return npoly;}

  void eval(double x, double* v) const {
    v[0] = 1.;
    if(get_nfunc() > 1){
      x = (x-xmid)/hrange;
      double val = 1.;
      for(int i=1; i<get_nfunc(); i++){
	val *= x;
	v[i] = val;
      }
    }
  }

  // Evaluates value at x given coefficients
  double operator()(double x, const Subs::Buffer1D<double>& coeff) const {
    double total = coeff[0];
    if(get_nfunc() > 1){
      x = (x-xmid)/hrange;
      double val = 1.;
      for(int i=1; i<get_nfunc(); i++){
	val   *= x;
	total += coeff[i]*val;
      }
    }
    return total;
  }

private:
  int npoly;
  double xmid, hrange;
};

/** Carries out polynomial fits to the sky in the y direction
 * \param data   the data frame
 * \param dvar   variances of the data frame
 * \param region the region file returned, and also the most up-to-date one if in the midst of sequence of images. Valid regions will
 * be used, else the equivalent one from 'master' will be adopted.
 * \param npoly  the number of poly coefficients to use when fitting the sky
 * \param reject the rejection threshold for sky fits
 * \param sky    the fitted sky values (assumes object regions are mutually exclusive). Must be set up with same format as data.
 */
void Ultracam::sky_fit(const Frame& data, const Frame& dvar, const Mspecap& region, int npoly, float reject, Frame& sky){

    // Constant to limit the variation in the variances
    const double MINVAR = 0.2;

    // Zero the sky
    sky = 0;
    
    // Buffers for the profile
    Subs::Buffer1D<double> x, y, v;
    Subs::Buffer1D<float> e, u;
    Subs::Buffer1D<double> coeff(npoly);
    Subs::Buffer2D<double> covar(npoly, npoly);
    
    const double THRESH = reject;
    
    int nrejtot = 0, nfit = 0, nptot = 0;
    
    // Wind through the CCDs
    for(size_t nccd=0; nccd<data.size(); nccd++){
	
	// Through each region of each CCD
	for(size_t nreg=0; nreg<region[nccd].size(); nreg++){
	    
	    const Specap& reg = region[nccd][nreg];
	    
	    // Look for a unique overlap ...
	    int nwin = reg.unique_window(data[nccd]);
	    if(nwin == -1)
		throw Ultracam_Error("sky_fit: region " + Subs::str(nreg+1) + ", CCD " + Subs::str(nccd+1) + " does not overlap with any window"); 
	    if(nwin == int(data[nccd].size()))
		throw Ultracam_Error("sky_fit: region " + Subs::str(nreg+1) + ", CCD " + Subs::str(nccd+1) + " overlaps with more than one window"); 
	    
	    const Windata& dwin = data[nccd][nwin];
	    const Windata& vwin = dvar[nccd][nwin];
	    Windata&       swin = sky[nccd][nwin];
	    
	    x.resize(dwin.ny());
	    y.resize(dwin.ny());
	    e.resize(dwin.ny());
	    u.resize(dwin.ny());
	    v.resize(dwin.ny());
	    
	    int xlo = std::max(0, std::min(dwin.nx(), int(dwin.xcomp(reg.get_xleft())  + 0.5)));
	    int xhi = std::max(0, std::min(dwin.nx(), int(dwin.xcomp(reg.get_xright()) + 1.5)));
	    
	    for(int ix=xlo; ix<xhi; ix++){
		
		// Load up a column
		double yccd;
		int ylo = -1, yhi = -1;
		for(int iy=0; iy<dwin.ny(); iy++){
		    x[iy] = yccd = dwin.yccd(iy);
		    y[iy] = dwin[iy][ix];
		    
		    // Test whether the current pixel is part of the sky. Must go through
		    // each sky region here since later regions can cancel out earlier ones
		    bool in = false;
		    for(int is=0; is<reg.nsky(); is++)
			if(reg.sky(is).ylow < yccd && reg.sky(is).yhigh > yccd)
			    in = reg.sky(is).good;
		    
		    if(in){
			nptot++;
			v[iy] = vwin[iy][ix];
			u[iy] = 1.;
			if(ylo < 0) ylo = iy;
			yhi = iy;
		    }else{
			u[iy] = -1.;
			v[iy] = 0.;
		    }
		}
		
		// Define polynomial function
		Poly poly(npoly, dwin.yccd(ylo), dwin.yccd(yhi));
		
		// Start the fitting
		int nrej = 1;
		while(nrej){
		    
		    // We first fit the variances themselves to avoid excessively different weights which
		    // is a particular risk in the case of photon counting. This may overweight cosmic
		    // but since they will be rejected, I think this is OK. This fit has to be re-done 
		    // after each rejection, but is carried out without any rejection of its own and with 
		    // uniform weights
		    Subs::llsqr(yhi-ylo+1, x.ptr()+ylo, v.ptr()+ylo, u.ptr()+ylo, poly, coeff, covar);
		    nfit++;
		    
		    // Evaluate fit over the sky regions only
		    bool first = true;
		    float vmax = 0.;
		    for(int iy=ylo; iy<=yhi; iy++){
			if(u[iy] > 0.){
			    v[iy] =  poly(x[iy], coeff);
			    if(first){
				vmax = v[iy];
				first = false;
			    }else{
				vmax = v[iy] > vmax ? v[iy] : vmax;
			    }
			}
		    }
		    
		    if(!first && vmax <= 0)
			throw Ultracam::Ultracam_Error("Ultracam::sky_fit: maximum variance <= 0; should not be possible");
		    
		    const double TVAR = MINVAR*vmax;

		    // Limit the minimum variance to be at least MINVAR of the maximum, set the uncertainties.
		    for(int iy=ylo; iy<=yhi; iy++){
			if(u[iy] > 0.){
			    v[iy] = v[iy] < TVAR ? TVAR : v[iy];
			    e[iy] = sqrt(v[iy]);
			}else{
			    e[iy] = -1.;
			}
		    }
		    
		    // Now fit the data themselves
		    Subs::llsqr(yhi-ylo+1, x.ptr()+ylo, y.ptr()+ylo, e.ptr()+ylo, poly, coeff, covar);
		    nfit++;
	    
		    // reject bad points
		    nrej = Subs::llsqr_reject(yhi-ylo+1, x.ptr()+ylo, y.ptr()+ylo, e.ptr()+ylo, poly, coeff, THRESH, true);
	    
		    // Reject the same points from the variances
		    for(int iy=ylo; iy<=yhi; iy++)
			if(e[iy] < 0.) u[iy] = -1.;

		    nrejtot += nrej;
		}
	
		// Store fit, making sure we do not overwrite others. yhi must be one more than the last valid pixel
		ylo = std::max(0, std::min(int(dwin.ycomp(reg.get_ylow()) +0.5), dwin.ny()));
		yhi = std::max(0, std::min(int(dwin.ycomp(reg.get_yhigh())+1.5), dwin.ny()));
		for(int iy=ylo; iy<yhi; iy++){
		    yccd = dwin.yccd(iy);
		    swin[iy][ix] = poly(dwin.yccd(iy), coeff);
		}
	    }
	}
    }
    std::cout << nfit << " fits were made with " << nrejtot << " rejected pixels = " << 100.*nrejtot/nptot << "% of the total." << std::endl;
}

