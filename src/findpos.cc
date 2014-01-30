#include <cstdlib>
#include <string>
#include "trm/subs.h"
#include "trm/constants.h"
#include "trm/ultracam.h"

void sub_back(float* y, int i1, int i2);

/*! \file
  \brief Defines the findpos function
*/

/**
 * findpos is a workhorse routine for measuring target positions
 * as required when defining aperture positions.
 *
 * findpos works as follows: given an initial starting position
 * and search half-width, it collapses a box around the initial position
 * and then measures the centroid in x and y by cross-correlating with
 * a gaussian profile. It then refines this by performing a weighted collapse
 * with weights given by the gaussian width and centred upon the position
 * after the first part. The idea is that the first pass is relatively insensitive
 * to where the target is within the box, but can be affected by the presence of
 * other nearby stars, whereas the second pass needs a good initial position, but
 * is not much affected by other stars.
 *
 * If the routine gets stuck it will throw an Ultracam_Error.
 *
 * If you compile with -DPLOT then extra plotting section are enabled which can be useful
 * for checking on troublesome cases.
 *
 * \param dat 2D array such that dat[iy][ix] gives the value of (ix,iy).
 * \param var 2D array such that var[iy][ix] gives the variance of (ix,iy).
 * \param nx X dimension of array
 * \param ny Y dimension of array
 * \param fwhm_x FWHM to use in X
 * \param fwhm_y FWHM to use in Y
 * \param hwidth_x Half-width to use in X. Collapse is over a region
 * +/- hwidth_x around the pixel nearest to the start position.
 * \param hwidth_y Half-width to use in Y.
 * \param xstart Start position in X. Used to define the initial search region.
 * \param ystart Start position in Y.Used to define the initial search region.
 * \param bias if true, the search will be made starting from the initial position defined by
 * xstart & ystart. If false, the initial search will start from the maxima of the 1D correlations.
 * bias=true is less likely to get confused by cosmic rays and other targets, bias=false is better
 * at coping with large shifts and bad guiding.
 * \param xpos Final X position. The centre of pixel 0,0 has position 0,0.
 * \param ypos Final Y position.
 * \param ex   uncertainty in final X position. For this to be right, the data must be bias-subtracted.
 * \param ey   uncertainty in final Y position. For this to be right, the data must be bias-subtracted.
 */

void Ultracam::findpos(internal_data **dat, internal_data **var, int nx, int ny, float fwhm_x, float fwhm_y,
               int hwidth_x, int hwidth_y, float xstart, float ystart, bool bias,
               double& xpos, double &ypos, float& ex, float& ey){

  try {

    // Check start position
    if(xstart <= -0.5 || xstart >= nx-0.5 || ystart <= -0.5 || ystart >= ny-0.5)
      throw Ultracam_Error("findpos: initial posiion outside array boundary");

    // Define region to examine
    int xlo = int(xstart+0.5) - hwidth_x;
    int xhi = int(xstart+0.5) + hwidth_x;
    int ylo = int(ystart+0.5) - hwidth_y;
    int yhi = int(ystart+0.5) + hwidth_y;

    // Make sure its in range
    xlo = xlo < 0  ? 0   : xlo;
    xhi = xhi < nx ? xhi : nx-1;
    ylo = ylo < 0  ? 0   : ylo;
    yhi = yhi < ny ? yhi : ny-1;

    // Collapse in X
    float xprof[nx], vxprof[nx];
    for(int ix=xlo; ix<=xhi; ix++)
      xprof[ix]  = vxprof[ix] = 0.;

    for(int iy=ylo; iy<=yhi; iy++){
      for(int ix=xlo; ix<=xhi; ix++){
    xprof[ix]  += dat[iy][ix];
    vxprof[ix] += var[iy][ix];
      }
    }

    // Search for maximum
    int   xmax = xlo+1;
    float fmax = xprof[xmax];
    if(!bias){
      for(int ix=xlo+1; ix<=xhi-1; ix++){
    if(xprof[ix] > fmax){
      fmax = xprof[ix];
      xmax = ix;
    }
      }
      xstart = float(xmax);
    }

    // Measure first X position
    sub_back(xprof, xlo, xhi);
    Subs::centroid(xprof,vxprof,xlo,xhi,fwhm_x,xstart,true,xpos,ex);

    // Collapse in Y
    float yprof[ny], vyprof[ny];

    for(int iy=ylo; iy<=yhi; iy++)
      vyprof[iy] = yprof[iy]  = 0.;

    for(int iy=ylo; iy<=yhi; iy++){
      for(int ix=xlo; ix<=xhi; ix++){
    yprof[iy]  += dat[iy][ix];
    vyprof[iy] += var[iy][ix];
      }
    }

    // Search for maximum
    int   ymax = ylo+1;
    fmax = yprof[ymax];
    if(!bias){
      for(int iy=ylo+1; iy<=yhi-1; iy++){
    if(yprof[iy] > fmax){
      fmax = yprof[iy];
      ymax = iy;
    }
      }
      ystart = float(ymax);
    }

    // Measure first Y position
    sub_back(yprof, ylo, yhi);
    Subs::centroid(yprof,vyprof,ylo,yhi,fwhm_y,ystart,true,ypos,ey);

    // Redefine region to examine
    xlo = int(xpos+0.5) - hwidth_x;
    xhi = int(xpos+0.5) + hwidth_x;
    ylo = int(ypos+0.5) - hwidth_y;
    yhi = int(ypos+0.5) + hwidth_y;

    // Make sure its in range
    xlo = xlo < 0  ? 0   : xlo;
    xhi = xhi < nx ? xhi : nx-1;
    ylo = ylo < 0  ? 0   : ylo;
    yhi = yhi < ny ? yhi : ny-1;

    // Weighted collapse in X
    float wgt;
    float norm = 0.;
    for(int ix=xlo; ix<=xhi; ix++){
      xprof[ix]  = 0.;
      vxprof[ix] = 0.;
    }
    for(int iy=ylo; iy<=yhi; iy++){
      wgt = exp(-Subs::sqr((float(iy)-ypos)/(fwhm_y/Constants::EFAC))/2.);
      norm += wgt;
      for(int ix=xlo; ix<=xhi; ix++){
    xprof[ix]  += wgt*dat[iy][ix];
    vxprof[ix] += wgt*wgt*var[iy][ix];
      }
    }

    xstart = xpos;

    // Measure weighted X position
    sub_back(xprof, xlo, xhi);
    Subs::centroid(xprof,vxprof,xlo,xhi,fwhm_x,xstart,true,xpos,ex);

    // Weighted collapse in Y
    for(int iy=ylo; iy<=yhi; iy++){
      yprof[iy]  = 0.;
      vyprof[iy] = 0.;
    }
    norm = 0.;
    for(int ix=xlo; ix<=xhi; ix++){
      wgt = exp(-Subs::sqr((float(ix)-xpos)/(fwhm_x/Constants::EFAC))/2.);
      norm += wgt;
      for(int iy=ylo; iy<=yhi; iy++){
    yprof[iy]  += wgt*dat[iy][ix];
    vyprof[iy] += wgt*wgt*var[iy][ix];
      }
    }

    // Measure weighted Y position
    ystart = ypos;
    sub_back(yprof, ylo, yhi);
    Subs::centroid(yprof,vyprof,ylo,yhi,fwhm_y,ystart,true,ypos,ey);
  }
  catch(const Subs::Subs_Error& err){
    throw Ultracam_Error("Ultracam::findpos: failed to measure position. Re-thrown this error\n" + err);
  }
}

// Subtracts median as an estimate of the background
// y is the array, i1 and i2 are the first and last elements
// to consider. This is to help the centroiding which may otherwise
// be affected by edge effects.
void sub_back(float* y, int i1, int i2){

  // make a temporary copy of the input data
  float temp[i2-i1+1];
  for(int i=i1,j=0; i<=i2; i++,j++) temp[j] = y[i];

  // determine the median
  float back = Subs::select(temp,i2-i1+1,(i2-i1+1)/2);

  // subtract
  for(int i=i1; i<=i2; i++) y[i] -= back;
}
