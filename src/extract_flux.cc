#include "trm_subs.h"
#include "trm_frame.h"
#include "trm_aperture.h"
#include "trm_ultracam.h"
#include "trm_reduce.h"

// Globals read by read_reduce_file

/** Routine to carry out the determination of the flux given an aperture and
 * a CCD
 * \param data data frame of interest, bias subtracted
 * \param dvar equivalent variance frame
 * \param bad  bad pixel frame. 0 if OK, then 10, 20, 30 etc for successively worse pixels. Values below
 * 10 are reserved for future automated cosmic ray detection.
 * \param gain equivalent gain frame
 * \param bias bias frame
 * \param aperture the aperture
 * \param sky_method method of estimating the sky
 * \param sky_thresh   threshold number of RMS to reject at.
 * \param sky_error  method of estimating the error on the sky
 * \param extraction_method type of extraction
 * \param zapped list of pixel positions rejected by cosmic ray cleaning
 * \param shape shape parameters from any fits made for this CCD
 * \param pepper  level at which peppering occurs
 * \param saturate level at which saturation occurs
 * \param counts  returned, the flux in counts
 * \param sigma   returned, estimated error on the flux in counts.
 * \param sky     returned, sky level, counts/pixel
 * \param nsky    returned, number of sky pixels
 * \param nrej    returned, number of sky pixels rejected
 * \param ecode   returned, error code
 * \param worst   value of worst bad pixel in aperture (0 = OK)
 */

void Ultracam::extract_flux(const Image& data, const Image& dvar, const Image& bad, 
			    const Image& gain, const Image& bias, const Aperture& aperture, Reduce::SKY_METHOD sky_method, 
			    float sky_thresh, Reduce::SKY_ERROR sky_error, Reduce::EXTRACTION_METHOD extraction_method,
			    const std::vector<std::pair<int,int> >& zapped, const Reduce::Meanshape& shape, float pepper, float saturate,
			    float& counts, float& sigma, float& sky, int& nsky, int& nrej,
			    Reduce::ERROR_CODES& ecode, int& worst){

    // flag to skip extra aperture section
    bool skip = (extraction_method == Reduce::OPTIMAL || aperture.nextra() == 0);

    worst = 0;

    if(aperture.valid()){

	try{

	    const Windata &dwin  = data.enclose(aperture.xpos(), aperture.ypos());	    
	    const Windata &vwin  = dvar.enclose(aperture.xpos(), aperture.ypos());	    
	    const Windata &bwin  = bad.enclose(aperture.xpos(), aperture.ypos());	    
	    const Windata &gwin  = gain.enclose(aperture.xpos(), aperture.ypos());	    
	    const Windata &bswin = bias.enclose(aperture.xpos(), aperture.ypos());	    

	    // window found for this aperture, but need to check that
	    // star aperture and any extra apertures are fully enclosed by it
	    bool enclosed = (
		dwin.left()   < aperture.xpos()-aperture.rstar() &&
		dwin.bottom() < aperture.ypos()-aperture.rstar() &&
		dwin.right()  > aperture.xpos()+aperture.rstar() &&
		dwin.top()    > aperture.ypos()+aperture.rstar());

	    if(!skip){
		for(int i=0; i<aperture.nextra(); i++){
		    enclosed = enclosed && 
			(dwin.left()   < aperture.xpos()+aperture.extra(i).x-aperture.rstar() &&
			 dwin.bottom() < aperture.ypos()+aperture.extra(i).y-aperture.rstar() &&
			 dwin.right()  > aperture.xpos()+aperture.extra(i).x+aperture.rstar() &&
			 dwin.top()    > aperture.ypos()+aperture.extra(i).y+aperture.rstar());
		}
	    }

	    if(enclosed){

		// Estimate sky background value
		float sky_sigma;
		bool overlap;
		double rms;
		sky_estimate(aperture, dwin, vwin, bwin, sky_method, sky_thresh, sky_error, sky, sky_sigma, rms, nsky, nrej, overlap);

		// Define the region for extraction of counts
		int xlo = int(Subs::nint(dwin.xcomp(aperture.xpos()-aperture.rstar())));
		int ylo = int(Subs::nint(dwin.ycomp(aperture.ypos()-aperture.rstar())));
		int xhi = int(Subs::nint(dwin.xcomp(aperture.xpos()+aperture.rstar())));
		int yhi = int(Subs::nint(dwin.ycomp(aperture.ypos()+aperture.rstar())));
	
		if(!skip){
		    for(int i=0; i<aperture.nextra(); i++){
			xlo = std::min(xlo, int(Subs::nint(dwin.xcomp(aperture.xpos()+aperture.extra(i).x-aperture.rstar()))));
			ylo = std::min(ylo, int(Subs::nint(dwin.ycomp(aperture.ypos()+aperture.extra(i).y-aperture.rstar()))));
			xhi = std::max(xhi, int(Subs::nint(dwin.xcomp(aperture.xpos()+aperture.extra(i).x+aperture.rstar()))));
			yhi = std::max(yhi, int(Subs::nint(dwin.ycomp(aperture.ypos()+aperture.extra(i).y+aperture.rstar()))));
		    }
		}

		xlo = std::max(0, xlo);
		ylo = std::max(0, ylo);
		xhi = std::min(dwin.nx()-1, xhi);
		yhi = std::min(dwin.ny()-1, yhi);

		// Approximate pixellation correction. Pixels fade out over 
		// length of 2.*rpix, where rpix is the "radius" of a pixel 
		// which depends upon the binning factors (=0.5 for xbin=ybin=1).
		float r, rpix=0.f, mweight, weight = 0, targ = 0;
		float fvar = 0.f, tpix = 0.f, fac, norm=0.f;
		bool  same = (dwin.xbin() == dwin.ybin());
		if(same) rpix = dwin.xbin()/2.;
		counts = 0.;

		int naper;
		if(skip)
		    naper = 1;
		else
		    naper = 1 +  aperture.nextra();

		float sdx[naper], dx[naper], sdy[naper], dy[naper];
		float maxval = 0.;

		// Start the loop over all possible pixels. Aim is to work out a weight for each one.
		// In the normal case this is something from 0 to 1 depending upon whether a pixel
		// is included in an aperture or not. Linear tapering of the weight is used to reduce
		// pixellation noise.
		for(int iy=ylo; iy<=yhi; iy++){
		    sdy[0] = Subs::sqr(dy[0] = dwin.yccd(iy)-aperture.ypos());
		    if(!skip){
			for(int i=0; i<aperture.nextra(); i++)
			    sdy[i+1] = Subs::sqr(dy[i+1] = dwin.yccd(iy)-aperture.ypos()-aperture.extra(i).y);
		    }

		    for(int ix=xlo; ix<=xhi; ix++){
			sdx[0] = Subs::sqr(dx[0] = dwin.xccd(ix)-aperture.xpos());
			if(!skip){
			    for(int i=0; i<aperture.nextra(); i++)
				sdx[i+1] = Subs::sqr(dx[i+1] = dwin.xccd(ix)-aperture.xpos()-aperture.extra(i).x);
			}

			// Now wind through all star apertures (main plus extras) to compute the weight for this pixel
			mweight = 0.;
			for(int i=0; i<naper; i++){
			    r = sqrt(sdx[i] + sdy[i]);
			    if(!same){
				if(r == 0.f) 
				    rpix = aperture.rstar()/2.;
				else
				    rpix = sqrt(Subs::sqr(dwin.xbin())*sdx[i] + Subs::sqr(dwin.ybin())*sdy[i])/r/2.;
			    }
	      
			    // only consider at all if within rpix of the outer radius.
			    if(r < aperture.rstar() + rpix){

				// Keep up with bad pixels
				if(bwin[iy][ix] > 0.5)
				    worst = std::max(worst, int(floor(bwin[iy][ix]+0.5f)));

				// Now compute extraction weights.
				if(extraction_method == Reduce::OPTIMAL){

				    if(shape.profile_fit_symm)
					fac = shape.a*(sdx[i] + sdy[i]);
				    else
					fac = shape.a*sdx[i] + 2.*shape.b*dx[i]*dy[i] + shape.c*sdy[i];
		  
				    if(shape.profile_fit_method == Reduce::GAUSSIAN){
					weight = exp(-fac);
				    }else if(shape.profile_fit_method == Reduce::MOFFAT){
					// In the case of moffat fit but gaussian extraction (the reverse is
					// not allowed), derive a scaling factor to ensure the same FWHM
					if(shape.extraction_weights == Reduce::GAUSSIAN)
					    weight = exp(-log(2.)/(pow(2.,1./shape.beta)-1)*fac);
					else
					    weight = 1./pow(1.+fac, shape.beta);
				    }
				}else{
				    weight = 1.;
				}
		
				// Apply linear taper at edge
				if(r > aperture.rstar() - rpix) weight *= (aperture.rstar()+rpix-r)/(2.*rpix);

				// The final weight used is the maximum ever encountered. 
				mweight = std::max(mweight, weight);

			    }
			}
	    
			// Finally form the weighted sums
			if(mweight > 0.){

			    // Sky subtracted flux
			    targ    = dwin[iy][ix] - sky;
			    counts += mweight*targ;
			    tpix   += mweight;
			    norm   += mweight*mweight;
			    maxval  = std::max(maxval, bswin[iy][ix]+dwin[iy][ix]);

			    // In the case of VARIANCE, our background variance estimate
			    // includes readout and sky photon noise thus we just add the additional
			    // amount from the object. In the photon case we compute ab initio.
			    // Note that the VARIANCE case fails to account for the dark subtraction
			    // and could thus under-estimate the value. ??
			    switch (sky_error) {
				case Reduce::VARIANCE:
				    fvar += Subs::sqr(mweight)*(Subs::sqr(rms) + std::max(0.f,targ)/gwin[iy][ix]);
				    break;
				case Reduce::PHOTON:
				    fvar += Subs::sqr(mweight)*vwin[iy][ix];
			    }
			}
		    }
		}

		// Add in the contribution to the variuance from the uncertainty in the sky estimate
		fvar += Subs::sqr(tpix*sky_sigma);

		// Check whether a cosmic ray was deleted from this aperture
		bool cosmic_detected = false;
		for(size_t ncos=0; ncos<zapped.size(); ncos++){
		    float sdx = Subs::sqr(dwin.xccd(zapped[ncos].first)-aperture.xpos());
		    float sdy = Subs::sqr(dwin.yccd(zapped[ncos].second)-aperture.ypos());
	  
		    r   = sqrt(sdx + sdy);
		    if(!same){
			if(r == 0.f) 
			    rpix = aperture.rstar()/2.;
			else
			    rpix = sqrt(Subs::sqr(dwin.xbin())*sdx + Subs::sqr(dwin.ybin())*sdy)/r;
		    }
	    
		    if(r < aperture.rstar() + rpix){
			cosmic_detected = true;
			break;
		    }

		    if(!skip){
			for(int i=0; i<aperture.nextra(); i++){
			    sdx = Subs::sqr(dwin.xccd(zapped[ncos].first)-aperture.xpos()-aperture.extra(i).x);
			    sdy = Subs::sqr(dwin.yccd(zapped[ncos].second)-aperture.ypos()-aperture.extra(i).y);
	      
			    r   = sqrt(sdx + sdy);
			    if(!same){
				if(r == 0.f) 
				    rpix = aperture.rstar()/2.;
				else
				    rpix = sqrt(Subs::sqr(dwin.xbin())*sdx + Subs::sqr(dwin.ybin())*sdy)/r;
			    }
	      
			    if(r < aperture.rstar() + rpix){
				cosmic_detected = true;
				break;
			    }
			}
		    }
		}

		// Error code
		if(maxval >  saturate){
		    ecode = Reduce::SATURATION;
		}else if(nsky == 0){
		    ecode = Reduce::NO_SKY;
		}else if(maxval > dwin.xbin()*dwin.ybin()*pepper){
		    ecode = Reduce::PEPPERED;
		}else if(overlap && cosmic_detected){
		    ecode = Reduce::SKY_OVERLAPS_AND_COSMIC_RAY_DETECTED;
		}else if(overlap){
		    ecode = Reduce::SKY_OVERLAPS_EDGE_OF_WINDOW;
		}else if(cosmic_detected){
		    ecode = Reduce::COSMIC_RAY_DETECTED_IN_TARGET_APERTURE;
		}else if(sky < -5.){
		    ecode = Reduce::SKY_NEGATIVE;
		}else if(extraction_method == Reduce::OPTIMAL && aperture.nextra()){
		    ecode = Reduce::EXTRA_APERTURES_IGNORED;
		}else{
		    ecode = Reduce::OK;
		}
	
		sigma = sqrt(fvar);

		// Try to get counts in the optimal case in rough agreement
		// with expected values. Need to check that this does not screw
		// things up.
		if(extraction_method == Reduce::OPTIMAL){
		    counts *= (tpix/norm);
		    sigma  *= (tpix/norm);
		}

	    }else{
		counts = 0.;
		sigma  = -1.;
		ecode  = Reduce::TARGET_APERTURE_AT_EDGE_OF_WINDOW;
	    } 
	}
	catch(const Ultracam::Ultracam_Error& err){ 
	    counts = 0.;
	    sigma  = -1.;
	    ecode  = Reduce::APERTURE_OUTSIDE_WINDOW;
	}

    }else{  

	ecode  = Reduce::APERTURE_INVALID;
	counts = 0.;
	sigma  = -1.;
    }

}

