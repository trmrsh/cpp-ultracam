#include "trm/subs.h"
#include "trm/array1d.h"
#include "trm/ultracam.h"
#include "trm/ccd.h"
#include "trm/mccd.h"
#include "trm/frame.h"
#include "trm/specap.h"

/**
 * \param data   the data frame
 * \param dvar   variances of the data frame
 * \param region the region file returned, and also the most up-to-date one if in the midst of sequence of images. Valid regions will
 * be used, else the equivalent one from 'master' will be adopted.
 * \param npoly  the number of poly coefficients used during the sky fits (needed for a proper uncertainty computation)
 * \param sky    the fitted sky values 
 * \param sdata  the spectrum data
 * \param serror the spectrum errors 
 */
void Ultracam::ext_nor(const Frame& data, const Frame& dvar, const Mspecap& region, int npoly, const Frame& sky,
		       std::vector<std::vector<Subs::Array1D<float> > >& sdata, std::vector<std::vector<Subs::Array1D<float> > >& serror){

  // Clear the storage buffers
  sdata.resize(data.size());
  serror.resize(data.size());

  // Wind through the CCDs
  for(size_t nccd=0; nccd<data.size(); nccd++){
    
    // Through each region of each CCD
    sdata[nccd].resize(region[nccd].size());
    serror[nccd].resize(region[nccd].size());

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
      const Windata& swin = sky[nccd][nwin];
      
      // Define extraction range in dispersion direction
      int xlo = std::max(0, std::min(dwin.nx(),int(dwin.xcomp(reg.get_xleft())+0.5)));
      int xhi = std::max(0, std::min(dwin.nx(),int(dwin.xcomp(reg.get_xright())+1.5)));
      
      sdata[nccd][nreg].resize(xhi-xlo);
      serror[nccd][nreg].resize(xhi-xlo);

      Subs::Array1D<float>& spec_dat = sdata[nccd][nreg];
      Subs::Array1D<float>& spec_err = serror[nccd][nreg];

      for(int ix=xlo; ix<xhi; ix++){

	// Compute region to extract object flux
	int ylo = std::max(0, std::min(dwin.ny(), int(dwin.ycomp(reg.get_ylow()) + 0.5)));
	int yhi = std::max(0, std::min(dwin.ny(), int(dwin.ycomp(reg.get_yhigh())+ 1.5)));

	// Extract flux
	double sumd = 0., sumv = 0.;
	for(int iy=ylo; iy<yhi; iy++){
	  sumd += (dwin[iy][ix] - swin[iy][ix]);
	  sumv += vwin[iy][ix];
	}
	spec_dat[ix-xlo] = sumd;
	spec_err[ix-xlo] = sqrt(sumv);
      }
    }
  }
}


