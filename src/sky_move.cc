#include "trm/array1d.h"
#include "trm/ultracam.h"
#include "trm/ccd.h"
#include "trm/mccd.h"
#include "trm/frame.h"
#include "trm/specap.h"

/**
 * \param data   the data frame
 * \param dvar   variances of the data frame
 * \param master the master extraction region file
 * \param reposition_mode the type of repositioning
 * \param region the region file returned, and also the most up-to-date one if in the midst of sequence of images. Valid regions will
 * be used, else the equivalent one from 'master' will be adopted.
 * \param fwhm   FWHM to use when measuring the object position. The position is measured using cross-correlation with a gaussian starting
 * from the highest pixel in the search region defined for each object in master/region
 * \param hwidth half-width of median filter to use when collapsing profiles 
 * \param max_shift if a position shift of more than this occurs, the region will be invalidated on an object-by-object basis
 * \param error_code error code returned by the routine
 * \param region region file input and returned by the routine
 */
void Ultracam::sky_move(const Frame& data, const Frame& dvar, const Mspecap& master, Sreduce::REGION_REPOSITION_MODE reposition_mode,
			float fwhm, float max_shift, int hwidth, Sreduce::ERROR_CODES& error_code, Mspecap& region){

  static bool first  = true;
  if(first){
    region = master;
    first = false;
  }

  if(reposition_mode == Sreduce::STATIC){
    return;
  }else if(reposition_mode == Sreduce::INDIVIDUAL){

    // Buffers for the profile
    Subs::Array1D<float> prof, pvar;
    Subs::Array1D<int> npix;
    
    // Wind through the CCDs
    for(size_t nccd=0; nccd<data.size(); nccd++){
      
      // Through each region of each CCD
      for(size_t nreg=0; nreg<region[nccd].size(); nreg++){
	
	Specap& reg = region[nccd][nreg];
	if(!reg.is_pos_accurate())
	  std::cerr << "WARNING: position of object not accurate in region " << nreg+1 << " of CCD " << nccd+1 << ", but will still attempt to reposition" << std::endl;

	// Look for a unique overlap ...
	int nwin = reg.unique_window(data[nccd]);
	if(nwin == -1)
	  throw Ultracam_Error("sky_move: region " + Subs::str(nreg+1) + ", CCD " + Subs::str(nccd+1) + " does not overlap with any window"); 
	if(nwin == int(data[nccd].size()))
	  throw Ultracam_Error("sky_move: region " + Subs::str(nreg+1) + ", CCD " + Subs::str(nccd+1) + " overlaps with more than one window"); 
	
	const Windata& dwin = data[nccd][nwin];
	const Windata& vwin = dvar[nccd][nwin];
	
	// Collapse it
	if(make_profile(dwin, vwin, reg.get_xleft(), reg.get_xright(), reg.get_yslow(), reg.get_yshigh(), hwidth, prof, pvar, npix)){
	  
	  // Work out valid range
	  int ilo = prof.size();
	  for(int i=0; i<npix.size(); i++){
	    if(npix[i]){
	      ilo = i;
	      break;
	    }
	  }
	  int ihi = 0;
	  for(int i=npix.size(); i>0; i--){
	    if(npix[i-1]){
	      ihi = i;
	      break;
	    }
	  }
	  
	  // Search for highest peak
	  float pmax = prof[ilo];
	  int imax = ilo;
	  for(int i=ilo+1; i<ihi; i++){
	    if(pmax < prof[i]){
	      pmax = prof[i];
	      imax = i;
	    }
	  }
	  
	  // Measure position
	  float start = imax, epos;
	  double pos;
	  Subs::centroid(prof.ptr(), pvar.ptr(), ilo, ihi-1, fwhm, start, true, pos, epos);
	  std::cerr << "CCD " << nccd+1 << ", object " << nreg+1 << ", position = " << reg.get_ypos() << " ---> " << dwin.yccd(pos) << std::endl;

	  // Update regions
	  reg.add_shift(dwin.yccd(pos) - reg.get_ypos());

	}else{
	  throw Ultracam_Error("sky_move: region " + Subs::str(nreg+1) + ", CCD " + Subs::str(nccd+1) + ", window " + Subs::str(nwin+1) + " gave a null profile");
	}
      }
    }
  }else if(reposition_mode == Sreduce::REFERENCE){
    throw Ultracam_Error("Sorry REFERENCE repositioning not supported yet");
  }
}
