#include "trm_windata.h"
#include "trm_ultracam.h"

/**
 * Carries out rejection stage of gaussian fitting.
 * \param data    the data
 * \param sigwin  1sigma uncertainties. -ve to mask. Will be modified on exit
 * \param xlo lower X limit of region to fit
 * \param xhi the upper  X limit of region to fit
 * \param ylo lower Y limit of region to fit
 * \param yhi the upper Y limit of region to fit
 * \param params the gaussian fit parameters
 * \param thresh the threshold multiple of sigma to reject at
 * \param nrej the number of pixels rejected (returned)
 */

void Ultracam::gauss_reject(const Windata& data, Windata& sigwin, int xlo, int xhi, 
			    int ylo, int yhi, const Ultracam::Ppars& params, float thresh, int& nrej){
  
  double xoff, yoff, efac, model, yefac;
  nrej = 0;
  for(int iy=ylo; iy<=yhi; iy++){
    yoff  = data.yccd(iy)-params.y;
    if(params.symm){
      yefac = params.a*yoff*yoff;
    }else{
      yefac = params.c*yoff*yoff;
    }
    for(int ix=xlo; ix<=xhi; ix++){
      if(sigwin[iy][ix] > 0.){
	xoff  = data.xccd(ix)-params.x;
	if(params.symm){
	  efac  = params.a*xoff*xoff + yefac;
	}else{
	  efac  = xoff*(params.a*xoff+2.*params.b*yoff) + yefac;
	}
	if(efac < Ultracam::Ppars::thresh()){
	  model = params.sky + params.height*exp(-efac);
	}else{
	  model = params.sky;
	}
	if(fabs(data[iy][ix]-model) > thresh*sigwin[iy][ix]){
	  sigwin[iy][ix] = - sigwin[iy][ix];
	  nrej++;
	}
      }
    }
  }
}

