#include "trm_subs.h"
#include "trm_windata.h"
#include "trm_ultracam.h"

/** pos_tweak refines a position of a star in an image by collapse along X and Y followed
 * by gaussian cross-correlation of each 1D profile. It is useful as
 * a fairly robust routine to initialise for a more sophisticated fit. Its main weakness
 * is if there is another star in the region that is collapsed.
 * \param win    the window of interest 
 * \param var    the variance over the window of interest 
 * \param fwhm   the FWHM of the 1D gaussian cross-correlator, unbinned pixels
 * \param hwidth the half-width of the search window, unbinned pixels.
 * \param xinit  the initial X position within CCD (unbinned pixels, lower-left pixel = 1,1). 
 * \param yinit  the initial Y position within CCD (unbinned pixels, lower-left pixel = 1,1). 
 * \param xnew   the modified X position within CCD (unbinned pixels, lower-left pixel = 1,1). 
 * \param ynew   the modified Y position within CCD (unbinned pixels, lower-left pixel = 1,1). 
 */
void Ultracam::pos_tweak(const Windata& win, const Windata& var, float fwhm, int hwidth, float xinit, float yinit, double& xnew, double& ynew){

  // positions and FWHMs in terms of 'computer' coords
  float xstart   = win.xcomp(xinit);
  float ystart   = win.ycomp(yinit);
  float fwhm_x   = fwhm/win.xbin();
  fwhm_x = fwhm_x > 1.f ? fwhm_x : 1.f;
  float fwhm_y   = fwhm/win.ybin();
  fwhm_y = fwhm_y > 1.f ? fwhm_y : 1.f;

  // width of window to collapse
  int   hwidth_x = hwidth/win.xbin();
  hwidth_x = hwidth_x > int(fwhm_x+1.) ? hwidth_x : int(fwhm_x+1.);
  int   hwidth_y = hwidth/win.ybin();
  hwidth_y = hwidth_y > int(fwhm_y+1.) ? hwidth_y : int(fwhm_y+1.);

  // OK, now refine position
  float xe, ye;
  Ultracam::findpos(win, var, win.nx(), win.ny(), fwhm_x, fwhm_y, hwidth_x, hwidth_y, xstart, ystart, 
		    true, xnew, ynew, xe, ye);

  // translate back to CCD coords
  xnew = win.xccd(xnew);
  ynew = win.yccd(ynew);
}
