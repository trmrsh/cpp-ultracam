#include <cfloat>
#include <cmath>
#include <iostream>
#include "trm/subs.h"
#include "trm/array1d.h"
#include "trm/ultracam.h"

/** Subroutine to generate probability distributions of L3 CCDs for use in simulating
 * their noise characteristics.
 * 
 * \param nstage the number of multiplication steps in the avalanche serial register, e.g. 591
 * \param p      the multiplication probability per stage, e.g. 0.015
 * \param pcic   the probability of a CIC per stage
 * \param cdf    a set of CDFs. On output cdf[n], n=0 to NIMAX-1 will contain the CDF for n input electrons, i.e. cdf[n][m]
 * is probability that given n input electrons, the output will have m or fewer electrons. For n=0 this will
 * be pure-CIC while for n>0 no CICs are included. This allows one to build up the distribution for number of electrons
 * greater than the maximum number of electrons NIMAX-1 that are computed. The maximum size of the CDFs sets the limit for the
 * internal computations. Experiment to see what is acceptable in terms of speed. note that there is essentially nothing to be
 * gained by making some CDFs shorter than others since the main computations are carried out with buffers determined by the
 * maximum dimension.
 */

void Ultracam::lllccd(int nstage, double p, double pcic, Subs::Buffer1D<Subs::Array1D<double> >& cdf){ 

  std::cout << "Now computing CDFs. This can take a while." << std::endl;
  const int NIMAX = cdf.size();
  if(NIMAX < 2) throw Ultracam_Error("lllccd error: NIMAX < 2");

  // work out maximum CDF length and remember which one it is
  // for later
  int nmax = cdf[0].size();
  int nstore = 0;
  for(int n=1; n<NIMAX; n++){
    if(nmax < cdf[n].size()){
      nmax = cdf[n].size();
      nstore = n;
    }
  }
  const int NMAX   = nmax;
  if(NMAX < 1) throw Ultracam_Error("lllccd error: NMAX < 0");
  const int NSTORE = nstore;

  // Number of points for the FFTs.
  const int NFFT = int(pow(2.,int(log(2.*NMAX)/log(2.))+1));

  // Grab memory. 
  Subs::Array1D<double> prob(NFFT), fft(NFFT), cfft(NFFT);
  
  // Initialise the single electron probability array which is 1 for n = 1,
  // but zero otherwise. Note that for a weird reason I do not understand, 
  // g++ seems to run faster if the numbers are not set to precisely zero
  for(int i=0; i<NMAX; i++)
    prob[i] = DBL_MIN;
  prob[1] = 1.;

  // Initialise the cumulative DFT for the CIC probabilities with the DFT of 
  // a delta function at zero representing the initial 0 electrons
  fft[0] = fft[1] = 1;
  for(int i=2; i<NFFT; i+=2){
    fft[i]   = 1.;
    fft[i+1] = DBL_MIN;
  }

  // Apply recurrence
  const double COMP    = 1.-p;
  const double CICCOMP = 1.-pcic;

  for(int ns=0; ns<nstage; ns++){

    // Give the user some hope ...
    if((ns+1) % 10 == 0) std::cerr << "L3 probability step = " << ns+1 << " out of " << nstage << std::endl;

    // Copy the probabilities into the maximum length CDF
    for(int i=0; i<NMAX; i++)
      cdf[NSTORE][i] = prob[i];
    
    // Set probability to (almost) zero beyond NMAX to avoid wrap round errors
    for(int i=NMAX; i<NFFT; i++)
      prob[i] = DBL_MIN;
    
    // FFT the probability array
    Subs::fftr(prob, NFFT, 1);
    
    // Multiply this in accounting for probability of form (1-p_c)\delta_{0n} + p_c P_r(n)
    // This is the convolution needed for the CIC probabilities
    fft[0] *= CICCOMP+pcic*prob[0];
    fft[1] *= CICCOMP+pcic*prob[1];
    for(int i=2; i<NFFT; i+=2){
      double real = fft[i]*(CICCOMP+pcic*prob[i])   - fft[i+1]*pcic*prob[i+1];
      double imag = fft[i+1]*(CICCOMP+pcic*prob[i]) + fft[i]*pcic*prob[i+1];
      fft[i]      = real;
      fft[i+1]    = imag;
    }

    // Square the result in order to derive the distribution needed for the Matsuo et al recurrence
    prob[0] *= prob[0];
    prob[1] *= prob[1];
    for(int i=2; i<NFFT; i += 2){
      double real = prob[i]*prob[i] - prob[i+1]*prob[i+1];
      double imag = 2*prob[i]*prob[i+1];
      prob[i]     = real;
      prob[i+1]   = imag;
    }
    
    // Now inverse FFT
    Subs::fftr(prob, NFFT, -1);
    
    // Finally apply the recurrence relation (the 2/NFFT factor is to normalise the FFT/inverse FFT pair) 
    for(int n=0; n<NMAX; n++)
      prob[n] = COMP*cdf[NSTORE][n] + 2*p*prob[n]/NFFT;

  }

  // By this stage 'prob' contains the PDF for 1 electron input with no CICs, while 'fft'
  // contains the FFT of the CIC distribution.

  // First inverse FFT for the final CIC distribution.
  Subs::fftr(fft, NFFT, -1);

  // Convert to the CDF and store with the 2/NFFT normalisation factor
  // ensure CDF is monotonic
  double sum = 0.;
  for(int i=0; i<cdf[0].size(); i++){
    sum += std::max(0.,2*fft[i]/NFFT);
    cdf[0][i] = sum;
  }

  // Store single electron case
  sum = 0.;
  for(int i=0; i<cdf[1].size(); i++){
    sum += std::max(0.,prob[i]);
    cdf[1][i] = sum;
  }

  // Now we need to generate the output CDFs equivalent to electron inputs from 1 to NIMAX-1

  if(NIMAX > 2){

    // Set probability to (almost) zero beyond NMAX to avoid wrap round errors
    for(int i=NMAX; i<NFFT; i++)
      prob[i] = DBL_MIN;
    
    // FFT the final single electron probability array in order to be able to convolve it
    Subs::fftr(prob, NFFT, 1);
    
    // Store it (fft no longer needed)
    fft = prob;

    // Convolve in single electron/no CIC with zero electron/CIC cases
    for(int n=2; n<NIMAX; n++){
      fft[0] *= prob[0];
      fft[1] *= prob[1];
      for(int i=2; i<NFFT; i += 2){
	double real = fft[i]*prob[i]   - fft[i+1]*prob[i+1];
	double imag = fft[i]*prob[i+1] + fft[i+1]*prob[i];
	fft[i]     = real;
	fft[i+1]   = imag;
      }

      // Copy to buffer which can be inverse FFT-ed without destroying the fft
      cfft = fft;
      
      // Now inverse the buffer
      Subs::fftr(cfft, NFFT, -1);

      // Store the result as a CDF
      sum = 0.;
      for(int i=0; i<cdf[n].size(); i++){
	sum += std::max(0.,2*cfft[i]/NFFT);
	cdf[n][i] = sum;
      }
    }
  }
}
