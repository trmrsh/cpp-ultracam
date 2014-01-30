#include "trm/ultracam.h"
#include "trm/ccd.h"
#include "trm/windata.h"

/** profit_init carries out some tedious stuff to get ready for fits to profiles
 * \param data           the CCD under analysis
 * \param dvar           the variance of the CCD under analysis
 * \param x              the initial x position. These are refined and returned.
 * \param y              the initial y position. These are refined and returned.
 * \param initial_search true/false for initial 1D search or not.
 * \param fwhm1d         FWHM for 1D search
 * \param hwidth1d       half width of 1D search window
 * \param hwidth         half width of fit window
 * \param sky            initial estimate of sky background
 * \param peak           initial estimate of peak height
 * \param skip_sky       true to skip the sky estimate (if the current value is near the mark)
 * \exception Throws Ultracam::Ultracam_Error exceptions if things go wrong.
 */

void Ultracam::profit_init(const Image& data, const Image& dvar, double& x, double& y, bool initial_search,
               float fwhm1d, int hwidth1d, int hwidth, float& sky, float& peak, bool skip_sky){

   const Windata &dwin = data.enclose(x,y);
   const Windata &vwin = dvar.enclose(x,y);

  // Start by trying to refine the initial position with a 1D collapse
  if(initial_search){
    float xstart   = dwin.xcomp(x);
    float ystart   = dwin.ycomp(y);
    float fwhm_x   = fwhm1d/dwin.xbin();
    fwhm_x = fwhm_x > 1.f ? fwhm_x : 1.f;
    float fwhm_y   = fwhm1d/dwin.ybin();
    fwhm_y = fwhm_y > 1.f ? fwhm_y : 1.f;
    int   hwidth_x = hwidth1d/dwin.xbin();
    hwidth_x = hwidth_x > int(fwhm_x+1.) ? hwidth_x : int(fwhm_x+1.);
    int   hwidth_y = hwidth1d/dwin.ybin();
    hwidth_y = hwidth_y > int(fwhm_y+1.) ? hwidth_y : int(fwhm_y+1.);
    double xpos, ypos;
    float xe, ye;

    Ultracam::findpos(dwin, vwin, dwin.nx(), dwin.ny(), fwhm_x, fwhm_y, hwidth_x, hwidth_y, xstart, ystart, true, xpos, ypos, xe, ye);

    // translate to CCD coordinates
    x = dwin.xccd(xpos);
    y = dwin.yccd(ypos);

    int ix;
    ix = xpos + 0.5 > 0. ? int(xpos+0.5) : 0;
    int iy;
    iy = ypos + 0.5 > 0. ? int(ypos+0.5) : 0;
    peak = dwin[iy][ix];
  }

  // Define fit region
  int hx = hwidth/dwin.xbin();
  hx = hx > 2 ? hx : 2;
  int hy = hwidth/dwin.ybin();
  hy = hy > 2 ? hy : 2;

  int xlo = int(dwin.xcomp(x) + 0.5) - hx;
  xlo = xlo < 0 ? 0 : xlo;
  int xhi = int(dwin.xcomp(x) + 0.5) + hx;
  xhi = xhi < dwin.nx() ? xhi : dwin.nx()-1;
  int ylo = int(dwin.ycomp(y) + 0.5) - hy;
  ylo = ylo < 0 ? 0 : ylo;
  int yhi = int(dwin.ycomp(y) + 0.5) + hy;
  yhi = yhi < dwin.ny() ? yhi : dwin.ny()-1;

  if(!skip_sky){
    // Initial estimate of sky background for 20%-ile
    float buff[(yhi-ylo+1)*(xhi-xlo+1)];
    unsigned long np = 0;
    for(int iy=ylo; iy<=yhi; iy++)
      for(int ix=xlo; ix<=xhi; ix++)
    buff[np++] = dwin[iy][ix];
    sky = Subs::select(buff, np, (unsigned long)(np*0.2));
  }

  if(!initial_search){
    peak = dwin[ylo][xlo];
    for(int iy=ylo; iy<=yhi; iy++)
      for(int ix=xlo; ix<=xhi; ix++)
    if(dwin[iy][ix] > peak) peak = dwin[iy][ix];
  }

  peak -= sky;

}
