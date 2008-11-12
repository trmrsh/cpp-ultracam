#include "trm_subs.h"
#include "trm_windata.h"
#include "trm_aperture.h"
#include "trm_ultracam.h"
#include "trm_reduce.h"

/** Routine to carry out the determination of the sky of a given aperture 
 * and appropriate windows. This routine serves to encapsulate this routine
 * so that it can be used in different places with confidence that the same
 * methods are being applied.
 * \param aperture the aperture. It is assumed to be valid.
 * \param dwin       data window of interest, which is assumed to contain the aperture
 * \param vwin       variance window equivalent to dwin
 * \param bwin       bad pixel file. 0 is ok, anything > 0 is not.
 * \param sky_method method of estimating the sky, CLIPPED_MEAN or MEDIAN. I generally prefer CLIPPED_MEAN
 * since the median is digitized and the +/- 0.5 count digitisation noise can be significant compared to
 * the true statistical uncertainty on the sky estimate, 'sky_sigma'. This can show up as nasty jumps in
 * time series data.
 * \param sky_thresh   threshold number of RMS to reject at.
 * \param sky_error  method of estimating the error on the sky.  PHOTON or VARIANCE. In the case of PHOTON,
 * the sky_sigma returned is based upon the standard variance estimate computed from readout noise
 * and the number of counts, while in the case of VARIANCE it comes from the measured RMS scatter in
 * the sky (i.e. it is directly related to the value 'rms' below). VARIANCE is robust to poor or no bias
 * subtraction and poor readout noise estimates, but could be adversely  affected by stars within the sky
 * aperture.
 * \param sky        returned, sky level, counts/pixel
 * \param sky_sigma  returned, estimated uncertainty on sky level, counts/pixel
 * \param rms        returned, RMS scatter on sky to be used as a background noise value
 * \param nsky       returned, total number of sky pixels
 * \param nrej       returned, number of sky pixels rejected (so actual number of sky pixels used = nsky-nrej)
 * \param overlap    returned, true if sky annulus overlaps edge of data window
 */

void Ultracam::sky_estimate(const Aperture& aperture, const Windata& dwin, const Windata& vwin, const Windata& bwin,
			    Reduce::SKY_METHOD sky_method, float sky_thresh, Reduce::SKY_ERROR sky_error,
			    float& sky, float& sky_sigma, double& rms, int& nsky, int& nrej, bool& overlap){
  
    // Static buffers to reduce allocation overheads
    static Subs::Buffer1D<float> sky_back(2000), sky_back_var(2000);
  
    // Initialise
    sky       = 0.f;
    sky_sigma = 0.f;
    nrej      = 0;
    nsky      = 0;
    sky_back.clear();
    sky_back_var.clear();
  
    // Everything OK, define region of frame containing outer sky radius, test for overlap
    // with edge of window
    overlap = false;
    int xlo = int(ceil(dwin.xcomp(aperture.xpos()-aperture.rsky2()) - 0.5));
    if(xlo < 0){
	overlap = true;
	xlo     = 0;
    }
    int xhi = int(floor(dwin.xcomp(aperture.xpos()+aperture.rsky2()) + 0.5));
    if(xhi > dwin.nx()-1){
	overlap = true;
	xhi     = dwin.nx()-1;
    }
    int ylo = int(ceil(dwin.ycomp(aperture.ypos()-aperture.rsky2()) - 0.5));
    if(ylo < 0){
	overlap = true;
	ylo     = 0;
    }
    int yhi = int(floor(dwin.ycomp(aperture.ypos()+aperture.rsky2()) + 0.5));
    if(yhi > dwin.ny()-1){
	overlap = true;
	yhi     = dwin.ny()-1;
    }

    // Load sky pixels into buffers
    float sd, sdy, sr1 = Subs::sqr(aperture.rsky1()), sr2 = Subs::sqr(aperture.rsky2());
    float dx, dy;
    for(int iy=ylo; iy<=yhi; iy++){
	dy  = dwin.yccd(iy) - aperture.ypos();
	sdy = Subs::sqr(dy);
	for(int ix=xlo; ix<=xhi; ix++){

	    // Only consider good pixels
	    if(bwin[iy][ix] < 0.5f){
		dx = dwin.xccd(ix) - aperture.xpos();
		sd = sdy + Subs::sqr(dx);
		if(sd > sr1 && sd < sr2){
	  
		    // OK have a pixel within the annulus; now check that it is not masked
		    bool masked = false;
		    for(int nm=0; nm<aperture.nmask(); nm++){
			if(Subs::sqr(dx - aperture.mask(nm).x) + Subs::sqr(dy - aperture.mask(nm).y) < Subs::sqr(aperture.mask(nm).z)){
			    masked = true;
			    break;
			}
		    }

		    if(!masked){
			if(sky_error == Reduce::PHOTON) sky_back_var.push_back(vwin[iy][ix]);
			sky_back.push_back(dwin[iy][ix]);
			nsky++;
		    }
		}
	    }
	}
    }
  
    // Clipped mean to guard against cosmic rays
    double rawmean=0., rawrms=0., mean=0.;
    rms = 0.;

    if(nsky){

	Subs::sigma_reject(sky_back.ptr(), nsky, sky_thresh, true, rawmean, rawrms, mean, rms, nrej);
    
	if(nrej < nsky){
	    
	    // Estimate sky variance. Two parts: individual part per pixel 
	    // and an overall uncertainty due to finite number of sky pixels.
	    // individual part already done at this point (estimated later
	    // for PHOTON case)
	    if(sky_error == Reduce::VARIANCE){
		sky_sigma = Subs::sqr(rms)/(nsky-nrej);
	    }else if(sky_error == Reduce::PHOTON){
		// Compute the fianl threshold used in order to work out which
		// pixels were rejected.
 		float thresh = sky_thresh*rms;
		sky_sigma = 0.f;
		int nok = 0;
		for(int isk=0; isk<nsky; isk++){
		    if(fabs(sky_back[isk]-mean) < thresh){
			nok++;
			sky_sigma += sky_back_var[isk];
		    }
		}
		if(nok)
		    sky_sigma /= Subs::sqr(nok);
		else
		    sky_sigma = 0.f;
	    }

	    // This is the statistical uncertainty in the final sky estimate,
	    // not the variance on a single pixel.
	    sky_sigma = sqrt(sky_sigma);
	    
	    // Estimate sky background 
	    if(sky_method == Reduce::CLIPPED_MEAN){
		sky = mean;
	    }else if(sky_method == Reduce::MEDIAN){
		if(nsky % 2 == 0)
		    sky = Subs::select(sky_back.ptr(), nsky-1, nsky/2);
		else
		    sky = Subs::select(sky_back.ptr(), nsky, nsky/2+1);
	    }
	}else{
	    sky = 0.;
	}
    }else{
	sky = 0.;
    }
}
