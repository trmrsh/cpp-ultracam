/**
 * Plots apertures, for use after a call to plot_images.
 * \param apers Multiple aperture file
 * \param x1 left plot limit
 * \param x2 right plot limit
 * \param y1 lower plot limit
 * \param y2 upper plot limit
 * \param all true to plot all CCDs
 * \param stackdir  stacking direction for multi-ccd plots: either 'X' or 'Y'
 * \param nccd the CCD number to plot if not all.
 */

#include <string>
#include "cpgplot.h"
#include "trm/subs.h"
#include "trm/mccd.h"
#include "trm/aperture.h"
#include "trm/ultracam.h"

void Ultracam::plot_apers(const Maperture& apers, float x1, float x2, float y1, float y2, 
			  bool all, char stackdirn, int nccd){
  if(all){
    if(stackdirn == 'X'){
      cpgsubp(apers.size(),1);
    }else if(stackdirn == 'Y'){
      cpgsubp(1,apers.size());  
    }else{
      throw Ultracam_Error(std::string("void Ultracam::plot_apers(const Maperture&, float, float,"
				  " float, float, bool, char, int nccd): "
				  " invalid stacking option = ") + stackdirn);
    }
    for(size_t ic=0; ic<apers.size(); ic++){
      if(stackdirn == 'X'){
	cpgpanl(ic+1,1);
      }else{
	cpgpanl(1,ic+1);
      }
      cpgwnad(x1,x2,y1,y2);
      cpgsci(Subs::WHITE);
      pgline(apers[ic]);
    }
  }else{
    cpgsci(Subs::WHITE);
    pgline(apers[nccd]);
  }
}

