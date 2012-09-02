/**
 * Plots defects, for use after a call to plot_images. 
 * \param defect Multiple defect file
 * \param x1 left plot limit
 * \param x2 right plot limit
 * \param y1 lower plot limit
 * \param y2 upper plot limit
 * \param all true to plot all CCDs
 * \param stackdirn  stacking direction for multi-ccd plots: either 'X' or 'Y'
 * \param nccd the CCD number to plot the defects of if not all. 0 will show all, in which
 * case their relative coordinates should have been transformed, see rtplot
 */

#include <string>
#include "cpgplot.h"
#include "trm_subs.h"
#include "trm_mccd.h"
#include "trm_defect.h"
#include "trm_ultracam.h"

void Ultracam::plot_defects(const Mdefect& defect, float x1, float x2, float y1, float y2, 
			    bool all, char stackdirn, int nccd){
  static int colour[3] = {2, 3, 5};
  if(all){
    if(stackdirn == 'X'){
      cpgsubp(defect.size(),1);
    }else if(stackdirn == 'Y'){
      cpgsubp(1,defect.size());
    }else{
      throw Ultracam_Error(std::string("void Ultracam::plot_defects(const Mdefect&, float, float,"
				  " float, float, bool, char, int nccd): "
				  " invalid stacking option = ") + stackdirn);
    }
    for(size_t ic=0; ic<defect.size(); ic++){
      cpgsci(colour[ic]);
      if(stackdirn == 'X'){
	cpgpanl(ic+1,1);
      }else{
	cpgpanl(1,ic+1);
      }
      cpgwnad(x1,x2,y1,y2);
      pgline(defect[ic]);
    }

  }else{

      // We plot all defects which should have been transformed onto the correct coordinates
      if(nccd == 0){
	  for(size_t ic=0; ic<defect.size(); ic++){
	      cpgsci(colour[ic]);
	      pgline(defect[ic]);
	  }
      }else{
	  cpgsci(colour[nccd-1]);
	  pgline(defect[nccd-1]);
      }
  }
}

