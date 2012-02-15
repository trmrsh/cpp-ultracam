#include <cstdlib>
#include "trm_subs.h"
#include "trm_buffer2d.h"
#include "trm_ultracam.h"
#include "trm_windata.h"

// function defined at the end
void fitmoffat_cof(const Ultracam::Windata& data, Ultracam::Windata& sigma, const Ultracam::Ppars& params, 
		   int xlo, int xhi, int ylo, int yhi, Subs::Buffer2D<double>& alpha, 
		   Subs::Buffer1D<double>& beta, double& chisq, int nvar);

/**
 * Uses the Levenburg-Marquardt method to fit a Moffat profile to a single Windata. 
 * Based upon mrqmin from Numerical Recipes
 * \param data the data to fit
 * \param sigma the 1-sigma uncertainties on each point, -ve to mask
 * \param xlo the lower X index limit for the sub-region of the window to compute over
 * \param xhi the upper X index limit for the sub-region of the window to compute over
 * \param ylo the lower Y index limit for the sub-region of the window to compute over
 * \param yhi the upper Y index limit for the sub-region of the window to compute over
 * \param params the initial parameters (given and returned). This structure defines everyhing about the
 * model to be fitted, including which parameters are to be varied.
 * \param chisq the Chi**2, returned.
 * \param alambda parameter which controls the matrix used to iterate towards a better solution. When small,
 * the matrix is essentially the usual curvature matrix. When large, it is forced towards becoming more diagonal. As
 * iterations proceed, alambda should decrease. If it increases, the last attempt to improve the fit failed, and
 * you should carry on iterating. If it decreases, then look at the change in Chi**2 to see if it worth further
 * iterations. \c alambda should be set -ve on the first call, and should not be altered between subsequent calls.
 * Set -ve again every time you want to re-fit from scratch. Once convergence has occurred, set \c alambda = 0 
 * to get the covariances sorted properly.
 * \param covar 2D array of covariances. Like \ alambda, these should be left unchanged during a sequence of calls. At the
 * end, after a call with alambda set = 0, they give the covariances on the fitted parameters. The size is automatically
 * increased if necessary by the program to cope with all the parameters.
 */

void Ultracam::fitmoffat(const Ultracam::Windata& data, Ultracam::Windata& sigma, int xlo, int xhi, int ylo, int yhi, 
			 Ultracam::Ppars& params, double& chisq, double& alambda, Subs::Buffer2D<double>& covar){

  // Static parameters whose value must be preserved between calls. In the case of the arrays,
  // they must be allocated to the maximum number of parameters to prevent segmentation faults
  // if the program is called multiple times with varying numbers of parameters. 

  static Ppars atry;
  static int nvar;
  static double ochisq;
  int    nmax = params.nmax();
  static Subs::Buffer1D<double> beta(nmax), da(nmax);
  static Subs::Buffer2D<double> oneda(nmax,1), alpha(nmax,nmax);

 // Number of parameters (not all of which are necessarily variable).
  int npar = params.npar();

  // Ensure that 'covar' is big enough, but not in the middle of a sequence
  if(alambda < 0. && (npar > int(covar.nrow()) || npar > int(covar.ncol()))){
    covar.resize(npar,npar);
  }else if(npar > int(covar.nrow()) || npar > int(covar.ncol())){
    throw Ultracam_Error("void Ultracam::fitmoffat(const Windata&, Windata&, int, int, "
			 "int, int, Ultracam::Ppars&, double&, double&, Subs::Buffer2D<double>&): "
			 "covariance matrix too small in midst of a sequence -- should not have happened");
  }

  // Initialise a few things if alambda is set negative indicating a restart
  if (alambda < 0.0) {
    
    // Number of variable parameters
    nvar = 0;
    for (int j=0; j<npar; j++)
      if (params.get_param_state(j)) nvar++;

    alambda=0.001;
    fitmoffat_cof(data, sigma, params, xlo, xhi, ylo, yhi, alpha, beta, chisq, nvar);
    ochisq = chisq;
    atry   = params;
  }

  // Alter linearised fitting matrix by augmenting diagonal elements
  for (int j=0; j<nvar; j++){
    for (int k=0; k<nvar; k++) covar[j][k]=alpha[j][k];
    covar[j][j]=alpha[j][j]*(1.0+alambda);
    oneda[j][0]=beta[j];
  }

  // Matrix solution to find a (hopefully) better solution

  Subs::gaussj(nvar,covar,oneda);

  for (int j=0; j<nvar; j++) da[j] = oneda[j][0];

  // If alambda set = 0, this indicates convergence and we therefore sort the covariances
  if(alambda == 0.){
    covsrt(covar,params,nvar);
    return;
  }

  // Lets see if the better solution really is better ...
  for (int j=0,l=0; l<npar; l++)
    if(params.get_param_state(l)) atry.set_param(l, params.get_param(l) + da[j++]);

  fitmoffat_cof(data, sigma, atry, xlo, xhi, ylo, yhi, covar, da, chisq, nvar);

  // Get to a lower Chi**2 ==> success, else increase alambda
  if (chisq < ochisq) {
    alambda *= 0.1;
    ochisq = chisq;
    for (int j=0; j<nvar; j++) {
      for (int k=0; k<nvar; k++) alpha[j][k]=covar[j][k];
      beta[j]=da[j];
    }
    params = atry;
  } else {
    alambda *= 10.0;
    chisq = ochisq;
  }
}

void fitmoffat_cof(const Ultracam::Windata& data, Ultracam::Windata& sigma, const Ultracam::Ppars& params, 
		   int xlo, int xhi, int ylo, int yhi, 
		   Subs::Buffer2D<double>& alpha, Subs::Buffer1D<double>& beta, double& chisq, int nvar){
  
  int npar = params.npar();
  
  // Initialise alpha and beta
  for(int j=0; j<nvar; j++){
    for(int k=0; k<=j; k++) alpha[j][k] = 0.;
    beta[j] = 0.;
  }

  // Now the real work.
  double *dyda = new double[npar];

  // start with an effective time saver.
  bool *tsave = new bool[npar];
  for(int i=0; i<npar; i++) tsave[i] = params.get_param_state(i);

  float sig;
  double xoff, yoff, yfac, fac, val1, val2, wgt, diff, wt, dfac;
  chisq = 0.;
  for(int iy=ylo; iy<=yhi; iy++){
    yoff  = data.yccd(iy)-params.y;
    if(params.symm)
      yfac  = params.a*yoff*yoff;
    else
      yfac  = params.c*yoff*yoff;

    for(int ix=xlo; ix<=xhi; ix++){
      if((sig = sigma[iy][ix]) > 0.){
	wgt   = 1./Subs::sqr(sig);
	xoff  = data.xccd(ix)-params.x;
	if(params.symm){
	  fac   = 1. + params.a*xoff*xoff + yfac;
	}else{
	  fac   = 1. + xoff*(params.a*xoff+2.*params.b*yoff) + yfac;
	}

	val1  = 1./pow(fac,params.beta);
	val2  = params.height*val1;
	diff  = data[iy][ix] - val2 - params.sky;
	dfac  = -params.beta*val2/fac;
	
	// Derivative vector
	dyda[params.sky_index()]    =   1.;
	dyda[params.height_index()] =   val1;
	
	if(params.symm){
	  dyda[params.x_index()]    =  -2.*dfac*params.a*xoff;
	  dyda[params.y_index()]    =  -2.*dfac*params.a*yoff;
	  dyda[params.a_index()]    =   dfac*(xoff*xoff + yoff*yoff);
	}else{
	  dyda[params.x_index()]    =  -2.*dfac*(params.a*xoff + params.b*yoff);
	  dyda[params.y_index()]    =  -2.*dfac*(params.b*xoff + params.c*yoff);
	  dyda[params.a_index()]    =   dfac*xoff*xoff;
	  dyda[params.b_index()]    =   2.*dfac*xoff*yoff;
	  dyda[params.c_index()]    =   dfac*yoff*yoff;
	}	
	dyda[params.beta_index()]   =  -val2*log(fac);

	for(int j=0, l=0; l<npar; l++){
	  if(tsave[l]){
	    wt = wgt*dyda[l];
	    for(int k=0, m=0; m<=l; m++)
	      if(tsave[m]) alpha[j][k++] += wt*dyda[m];
	    beta[j++] += wt*diff;
	  }
	}
	chisq += wgt*Subs::sqr(diff);
      }
    }
  }

  for(int j=1; j<nvar; j++)
    for(int k=0; k<j; k++) alpha[k][j] = alpha[j][k];

  delete[] tsave;
  delete[] dyda;
}

