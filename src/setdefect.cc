/*

!!begin
!!title   Defect map setup program
!!author  T.R. Marsh
!!created 02 August 2002
!!root    setdefect
!!index   setdefect
!!descr   sets up defect map
!!css   style.css
!!class   Programs
!!head1   setdefect is for setting up maps of CCD defects

!!emph{setdefect} is an interactive program for defining the position
of CCD defects, useful for positioning objects in !!ref{rtplot.html}{rtplot}.
Two types of defects are supported: pixel and line defects. You locate these
by clicking with a cursor. You will be asked to specify whether a defect is 
'moderate' or 'disastrous' -- what you decide here is entirely up to you, but
I assume that a 'moderate' defect is one you would prefer to avoid if possible,
while a 'disastrous' defect is one to be avoided at all costs.

Note that the defects of all CCDs will be plotted (with different colours, CCD 1 = red,
CCD 2 = green, CCD 3 = light blue), but that only the position of the CCD in question
can be fully trusted because of the differing orientations of the CCD. These are
accounted for in 'rtplot'.

!!head2 Invocation

setdefect [device] data newfile defect nccd xleft xright ylow yhigh iset (ilow ihigh)/(plow phigh) 

!!head2 Command line arguments

!!table
!!arg{device}{The image display device.}
!!arg{data}{Ultracam data file.}
!!arg{newfile}{flag to indicate that the aperture file is new.}
!!arg{defect}{Defect file (new or old).}
!!arg{nccd}{The number of the CCD to set apertures over.}
!!arg{xleft xright}{X range to plot}
!!arg{ylow yhigh}{Y range to plot}
!!arg{iset}{How to set intensity: 'a' for automatic, min to max,
'd' for direct input of range, 'p' for percentiles, probably the most useful option.}
!!arg{ilow ihigh}{the intensity range is i1 to i2 if iset='d'}
!!arg{plow phigh}{p1 to p2 are percentiles to set the intensity if iset='p'}
!!table

!!head1 Interactive options

There are several interactive options, which are as follows:

!!table

!!arg{D}{Delete a defect. Place the cursor near to the defect you
want to remove and then hit 'D'.}

!!arg{L}{Adds a new line defect starting at the cursor position. You will be prompted to indicate
the other end of the line too.}

!!arg{F}{Display the full frame.}

!!arg{I}{Zooms in by a factor 2 around the cursor position.}

!!arg{O}{Zooms out by a factor 2 around the cursor position.}

!!arg{P}{Adds a new pixel defect at the cursor position.}

!!arg{Q}{Quit the program.}

!!arg{S}{Shows the value of pixel corresponding to the cursor position (no graphics scaling -- it gives you 
the true value)}

!!arg{W}{Define a window with the cursor which will then be displayed}

!!table

!!end

*/

#include <cstdlib>
#include <cfloat>
#include <string>
#include <sstream>
#include <map>
#include <fstream>
#include "cpgplot.h"
#include "trm_subs.h"
#include "trm_plot.h"
#include "trm_input.h"
#include "trm_frame.h"
#include "trm_mccd.h"
#include "trm_defect.h"
#include "trm_ultracam.h"

int main(int argc, char* argv[]){

  try{

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // sign-in input variables
    input.sign_in("device",  Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("data",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("newfile", Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("defect",  Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("nccd",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("xleft",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("xright",  Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ylow",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("yhigh",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("iset",    Subs::Input::GLOBAL,  Subs::Input::PROMPT);
    input.sign_in("ilow",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ihigh",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("plow",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("phigh",   Subs::Input::GLOBAL, Subs::Input::PROMPT);

    // Get inputs

    std::string device;
    input.get_value("device", device, "/xs", "plot device");
    std::string name;
    input.get_value("data", name, "run001", "file to plot");
    Ultracam::Frame data(name);
    bool newfile;
    input.get_value("newfile", newfile, true, "do you want to open a new defect file?");
    std::string defname;
    input.get_value("defect", defname, "defect", "defect std::map file name");

    // Create or open a defect file
    Ultracam::Mdefect defect;
    std::string entry;
    if(newfile){
      defect = Ultracam::Mdefect(data.size());
    }else{
      defect.rasc(defname);
      if(defect.size() != data.size())
	throw Ultracam::Ultracam_Error("Data frame and defect file have conflicting CCD numbers");    
    }
    int nccd;
    input.get_value("nccd", nccd, int(1), int(1), int(data.size()), 
		    "CCD number to set defects for");
    nccd--;

    float x1, x2, y1, y2;
    x2 = data[nccd].nxtot()+0.5;
    y2 = data[nccd].nytot()+0.5;
    input.get_value("xleft",  x1, 0.5f, 0.5f, x2, "left X limit of plot");
    input.get_value("xright", x2, x2,   0.5f, x2, "right X limit of plot");
    input.get_value("ylow",   y1, 0.5f, 0.5f, y2, "lower Y limit of plot");
    input.get_value("yhigh",  y2, 0.5f, 0.5f, y2, "upper Y limit of plot");
    char iset;
    input.get_value("iset", iset, 'a', "aAdDpP", 
		    "set intensity a(utomatically), d(irectly) or with p(ercentiles)?");
    iset = toupper(iset);
    float ilow, ihigh, plow, phigh;
    if(iset == 'D'){
      input.get_value("ilow",   ilow,  0.f, -FLT_MAX, FLT_MAX, "lower intensity limit");
      input.get_value("ihigh",  ihigh, 1000.f, -FLT_MAX, FLT_MAX, "upper intensity limit");
    }else if(iset == 'P'){
      input.get_value("plow",   plow,  1.f, 0.f, 100.f,  "lower intensity limit percentile");
      input.get_value("phigh",  phigh, 99.f, 0.f, 100.f, "upper intensity limit percentile");
      plow  /= 100.;
      phigh /= 100.;
    }

    Subs::Plot plot(device);
    cpgsch(1.5);
    cpgscf(2);

    Ultracam::plot_images(data,x1,x2,y1,y2,false,'X',iset,ilow,ihigh,plow,phigh,true,name,nccd,false);
    Ultracam::plot_defects(defect, x1, x2, y1, y2, false, 'X', nccd);

    cpgsci(Subs::WHITE);
    float x = (x1+x2)/2., y = (y1+y2)/2.;
    char ret = 'X';
    std::string lab;

    std::cout << "Position the cursor to add/delete etc defects and\n"
	 << "hit the appropriate letter.\n" << std::endl;

    // Now defect addition loop

    while(ret != 'Q'){
      ret = 'X';

      // Next lines define the prompt:

      std::cout << "P(ixel), L(ine), ";
      if(defect[nccd].size()) std::cout << "D(elete), ";
      std::cout << "I(n), O(ut), F(ull), S(how), W(indow), Q(uit)" << std::endl; 

      // Now get cursor input.

      if(!cpgcurs(&x,&y,&ret)) throw Ultracam::Ultracam_Error("Cursor error");
      ret = toupper(ret);

      if(ret == 'P'){

	std::cout << "Is this defect M(oderate) or D(isastrous)?" << std::endl;
	float xd=x, yd=y;
	if(!cpgcurs(&xd,&yd,&ret)) throw Ultracam::Ultracam_Error("Cursor error");
	ret = toupper(ret);

	if(ret == 'M'){
	  defect[nccd].push_back(Ultracam::Defect(x,y,Ultracam::Defect::MODERATE));
	  pgline(defect[nccd][defect[nccd].size()-1]);
	}else if(ret == 'D'){
	  defect[nccd].push_back(Ultracam::Defect(x,y,Ultracam::Defect::DISASTER));
	  pgline(defect[nccd][defect[nccd].size()-1]);
	}else{
	  std::cerr << "Only options are 'm' or 'd'; no defect added." << std::endl;
	}

      }else if(ret == 'L'){

	float xd=x, yd=y;
	char reply = 'z';
	std::cout << "Position at the other end of the line defect then hit 'L' again" << std::endl;
	if(cpgband(1,1,x,y,&xd,&yd,&reply)){
	  reply = toupper(reply);
	  if(reply == 'L'){
	    std::cout << "Is this defect M(oderate) or D(isastrous)?" << std::endl;
	    float xdd=xd, ydd=yd;
	    if(!cpgband(1,1,x,y,&xdd,&ydd,&ret)) 
	      throw Ultracam::Ultracam_Error("Cursor error");
	    ret = toupper(ret);

	    if(ret == 'M'){
	      defect[nccd].push_back(Ultracam::Defect(x,y,xd,yd,Ultracam::Defect::MODERATE));
	      pgline(defect[nccd][defect[nccd].size()-1]);
	    }else if(ret == 'D'){
	      defect[nccd].push_back(Ultracam::Defect(x,y,xd,yd,Ultracam::Defect::DISASTER));
	      pgline(defect[nccd][defect[nccd].size()-1]);
	    }else{
	      std::cerr << "Only options are 'm' or 'd'; no defect added." << std::endl;
	    }
	  }else{
	    std::cerr << "Only options is 'L'; no defect added." << std::endl;
	  }
	}else{
	  throw Ultracam::Ultracam_Error("Cursor error");
	}

      }else if(defect[nccd].size() && ret == 'D' ){

	// Delete a defect

	Ultracam::Defect a;
	if(defect[nccd].del_obj(x,y,a)){
	  cpgsci(Subs::RED);
	  pgline(a);
	  cpgsci(Subs::WHITE);
	}

      }else if(ret == 'F'){

	// Re-plot full frame

	x1 = 0.5;
	x2 = data[nccd].nxtot()+0.5;
	y1 = 0.5;
	y2 = data[nccd].nytot()+0.5;
	cpgeras();
	Ultracam::plot_images(data,x1,x2,y1,y2,false,'X',iset,ilow,ihigh,plow,phigh,true,
			      name,nccd,false);
	Ultracam::plot_defects(defect, x1, x2, y1, y2, false, 'X', nccd);

      }else if(ret == 'W'){

	// Select a region to window

	std::cout << "Pick first corner of window" << std::endl;
	char reply;
	float xc1 = x, yc1 = y;
	if(cpgcurs(&xc1,&yc1,&reply)){
	  std::cout << "Set other corner (Q to quit)" << std::endl;
	  float xc2, yc2;
	  xc2 = xc1;
	  yc2 = yc1;
	  if(cpgband(2,1,xc1,yc1,&xc2,&yc2,&reply)){
	    reply = toupper(reply);
	    if(reply != 'Q'){
	      x1 = std::min(xc1, xc2);
	      x2 = std::max(xc1, xc2);
	      y1 = std::min(yc1, yc2);
	      y2 = std::max(yc1, yc2);
	      cpgeras();
	      Ultracam::plot_images(data,x1,x2,y1,y2,false,'X',iset,ilow,ihigh,plow,phigh,true,
				    name,nccd,false);
	      Ultracam::plot_defects(defect, x1, x2, y1, y2, false, 'X', nccd);
	      x = (x1+x2)/2., y = (y1+y2)/2.;
	    }
	  }else{
	    std::cerr << "Cursor error" << std::endl;
	  }
	}else{
	  std::cerr << "Cursor error" << std::endl;
	}

      }else if(ret == 'I'){

	// Zoom in

	float xr = (x2-x1)/2., yr = (y2-y1)/2.; 
	
	x1 = x - xr/2.;
	x2 = x + xr/2.;
	y1 = y - yr/2.;
	y2 = y + yr/2.;

	cpgeras();
	Ultracam::plot_images(data,x1,x2,y1,y2,false,'X',iset,ilow,ihigh,plow,phigh,true,
			      name,nccd,false);
	Ultracam::plot_defects(defect, x1, x2, y1, y2, false, 'X', nccd);

      }else if(ret == 'O'){

	// Zoom out

	float xr = (x2-x1)/2., yr = (y2-y1)/2.; 
	
	x1 = x - 2.*xr;
	x2 = x + 2.*xr;
	y1 = y - 2.*yr;
	y2 = y + 2.*yr;

	cpgeras();
	Ultracam::plot_images(data,x1,x2,y1,y2,false,'X',iset,ilow,ihigh,plow,phigh,true,
			      name,nccd,false);
	Ultracam::plot_defects(defect, x1, x2, y1, y2, false, 'X',nccd);

      }else if(ret == 'S'){
	
	try{
	  int wfind;
	  const Ultracam::Windata& win = data[nccd].enclose(x,y,wfind);
	  
	  int ix  = int(win.xcomp(x) + 0.5);
	  int iy  = int(win.ycomp(y) + 0.5);
	  
	  std::cout << "\nPosition = (" << x << "," << y << ")" << std::endl;
	  std::cout << "Window " << wfind+1 << ", pixel = (" << ix << "," 
	       << iy << "), value = " << win[iy][ix] << std::endl;
	}
	catch(const Ultracam::Ultracam_Error& err){
	  std::cerr << err << std::endl;
	}
	
      }else if(ret != 'Q'){
	std::cerr << "Input = " << ret << " not recognised." << std::endl;
      }
    }
    
    // Dump the result
    defect.wasc(defname);

  }

  catch(const Ultracam::Input_Error& err){
    std::cerr << "Ultracam::Input_Error exception:" << std::endl;
    std::cerr << err << std::endl;
  }
  catch(const Ultracam::Ultracam_Error& err){
    std::cerr << "Ultracam::Ultracam_Error exception:" << std::endl;
    std::cerr << err << std::endl;
  }
  catch(const Subs::Subs_Error& err){
    std::cerr << "Subs::Subs_Error exception:" << std::endl;
    std::cerr << err << std::endl;
  }
  catch(const std::string& err){
    std::cerr << err << std::endl;
  }

}








