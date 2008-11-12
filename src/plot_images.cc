/**
 * Plots images inside plot and rtplot and various other programs. This programs
 * works out the intensity limits if requested. It does so only over the region
 * displayed.
 *
 * \param data   ULTRACAM file name
 * \param x1     Left plot limit
 * \param y1     Right plot limit
 * \param x2     Lower plot limit
 * \param y2     Upper plot limit
 * \param all    Plot all CCDs or not
 * \param stackdirn  stacking direction for multi-ccd plots: either 'X' or 'Y'
 * \param iset   Type of intensity setting
 * \param ilow   Lower intensity limit if iset='d', also returned as the intensity actually used.
 * \param ihigh  Upper intensity limit if iset='d', also returned as the intensity actually used.
 * \param plow   Lower percentile if iset='p'
 * \param phigh  Upper percentile if iset='p'
 * \param first  Is this the first frame of a sequence
 * \param fname  Name for terminal I/O
 * \param nccd   CCD number to plot if not all (starts at 0)
 * \param termio true/false for terminal I/O or not
 */

#include <string>
#include "cpgplot.h"
#include "trm_subs.h"
#include "trm_frame.h"
#include "trm_format.h"
#include "trm_ultracam.h"

void Ultracam::plot_images(const Frame& data, float x1, float x2, float y1, float y2, 
			   bool all, char stackdirn, char iset, float& ilow, float& ihigh, 
			   float plow, float phigh, bool first, const std::string& fname, 
			   int nccd, bool termio){
    iset = toupper(iset);

    // Is the blue frame bad or not?
    Subs::Header::Hnode *hnode = data.find("Frame.bad_blue");
    bool blue_is_bad = hnode->has_data() ? hnode->value->get_bool() : false;

    // Turn region into a CCD of windows (with just 1 window)
    CCD<Window> window;
    int llx = std::max(int(1), std::min(data[0][0].nxtot(), int(std::min(x1,x2)+0.5)));
    int lly = std::max(int(1), std::min(data[0][0].nytot(), int(std::min(y1,y2)+0.5)));
    int nx  = std::min(data[0][0].nxtot()-llx+1, int(fabs(x2-x1)+0.5));
    int ny  = std::min(data[0][0].nytot()-lly+1, int(fabs(y2-y1)+0.5));
    window.push_back(Window(llx,lly,nx,ny,1,1,data[0][0].nxtot(),data[0][0].nytot()));

    Subs::Format plform(6);

    if(all){

	if(stackdirn == 'X'){
	    cpgsubp(data.size(),1);
	}else if(stackdirn == 'Y'){
	    cpgsubp(1,data.size());
	}else{
	    throw Ultracam_Error(std::string("void Ultracam::plot_images(const Frame&, float, float, float, float," 
					     "bool, ch+ar, char, float&, float&, float, float, bool, const std::string&, "
					     "int, bool): invalid stacking option = ") + stackdirn);
	}
	for(size_t ic=0; ic<data.size(); ic++){
	    //! Specific to ultracam, blue frames can be bad
	    if(!blue_is_bad || ic != 2){
		if(stackdirn == 'X'){
		    cpgpanl(ic+1,1);
		}else{
		    cpgpanl(1,ic+1);
		}
		cpgvstd();
		if(first){
		    cpgsci(Subs::BLUE);
		    cpgwnad(x1,x2,y1,y2);
		    if(iset == 'P'){
			data[ic].centile(plow,phigh,ilow,ihigh,window);
		    }else if(iset == 'A'){
			ilow  = min(data[ic],window);
			ihigh = max(data[ic],window);
		    }
		}
		cpgsci(Subs::WHITE);
		pggray(data[ic],ihigh,ilow);
		if(first){
		    cpgsci(Subs::BLUE);
		    cpgbox("BCNST",0.,0,"BCNST",0.,0);
		}
		cpgsci(Subs::WHITE);
		pgline(data[ic]);
		pgptxt(data[ic]);
		if(first){
		    cpgsci(Subs::RED);
		    std::string title = std::string("CCD ") + Subs::str(ic+1);
		    cpglab("X pixels", "Y pixels", title.c_str());
		}
		if(termio)
		    std::cout << fname << ", CCD " << ic+1 << ", plot range = " << plform(ilow) << " to " << plform(ihigh) << std::endl;
	    }else if(blue_is_bad && termio){
		std::cout << fname << ", CCD " << ic+1 << " skipped as rubbish data" << std::endl;
	    }
	}

    }else if(!blue_is_bad || nccd != 2){

	if(first){
	    cpgwnad(x1,x2,y1,y2);
	    if(iset == 'P'){
		data[nccd].centile(plow,phigh,ilow,ihigh,window);
	    }else if(iset == 'A'){
		ilow  = min(data[nccd],window);
		ihigh = max(data[nccd],window);
	    }
	}
	cpgsci(Subs::WHITE);
	pggray(data[nccd],ihigh,ilow);
	if(first){
	    cpgsci(Subs::BLUE);
	    cpgbox("BCNST",0.,0,"BCNST",0.,0);
	}
	cpgsci(Subs::WHITE);
	pgline(data[nccd]);
	pgptxt(data[nccd]);
	if(first){
	    cpgsci(Subs::RED);
	    std::string title = std::string("CCD ") + Subs::str(nccd+1);
	    cpglab("X pixels", "Y pixels", title.c_str());
	}
	if(termio)
	    std::cout << fname << ", CCD " << nccd+1 << ", plot range = " << plform(ilow) << " to " << plform(ihigh) << std::endl;
    }

}

