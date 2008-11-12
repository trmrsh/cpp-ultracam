#include "trm_subs.h"
#include "trm_buffer2d.h"
#include "trm_ultracam.h"

/** covsrt sorts the covariances produced during profile fits into correct order, 
 * setting the covariances of fixed parameters to zero and getting the indices correct
 * \param covar the covariance array
 * \param params the profile parameters
 * \param nvar the number of variable parameters
 */

void Ultracam::covsrt(Subs::Buffer2D<double>& covar, const Ultracam::Ppars& params, int nvar){
  int npar = params.npar();
  for(int i=nvar; i<npar; i++)
    for(int j=0; j<=i; j++) covar[i][j] = covar[j][i] = 0.;
  int k = nvar-1;
  for(int j=npar-1; j>=0; j--){
    if(params.get_param_state(j)){
      for(int i=0; i<npar; i++) std::swap(covar[i][k], covar[i][j]);
      for(int i=0; i<npar; i++) std::swap(covar[k][i], covar[j][i]);
      k--;
    }
  }
}
