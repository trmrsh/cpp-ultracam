/*

!!begin
!!title   Sky line generation program
!!author  T.R. Marsh
!!created 18 October 2005
!!revised 02 December 2006
!!root    addsky
!!index   addsky
!!descr   adds sky lines to a frame
!!css     style.css
!!class   Programs
!!class   Spectra
!!class   Testing
!!head1   addsky for adding sky lines to a series of spectra

!!emph{addsky} loads a file defining skylines and uses this to add fakes sky lines
to a frame or a series of frames.

!!head2 Invocation

addspec skylines data scale xover yover [seed xd yd yrms]!!break

!!head2 Command line arguments

!!table

!!arg{skylines}{Name of the skyline definition file}

!!arg{data}{Name of data file or list of data files to add sky lines to. These will be overwritten on output.}

!!arg{scale}{Scale factor to multiply the input strengths by to give an easy way to change the effective
brightness, telescope size etc. 1 implies no change.}

!!arg{xover}{Oversampling factor (in terms of unbinned pixels) in the X direction, i.e. spectral direction.}

!!arg{seed}{Random number seed. This and succedding arguments is only prompted for
if there is more than one data file}

!!arg{xdrift, ydrift}{Drift per image in x and y. Note that X is the dispersion direction and therefore represents flexure while 
Y includes guiding as well.}

!!arg{yrms}{RMS scatter in Y.}

!!arg{seeing1}{Starting seeing, in pixels. It is assumed that the spectrum profile is gaussian. Then a linear
ramp of seeing will be added to them with image number. The seeing is added in quadrature to the specified width.}

!!arg{seeing2}{Ending seeing, in pixels. It is assumed that the profiles given are without any seeing added.}

!!arg{nreset}{Reset drift every nreset images. Simulates a position
correction at the telescope.}

!!table

!!end

*/

#include <cstdlib>
#include <cfloat>
#include <climits>
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include "trm/subs.h"
#include "trm/input.h"
#include "trm/frame.h"
#include "trm/skyline.h"
#include "trm/mccd.h"
#include "trm/ultracam.h"

int main(int argc, char* argv[]){

  using Ultracam::Input_Error;
  using Ultracam::Ultracam_Error;

  try{

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Define inputs
    input.sign_in("skylines", Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("data",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("xover",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("scale",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("seed",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("xdrift",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ydrift",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("yrms",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("seeing1",  Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("seeing2",  Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("nreset",   Subs::Input::GLOBAL, Subs::Input::PROMPT);

    // Get inputs

    std::string sskylines;
    input.get_value("skylines", sskylines, "skylines", "skyline definition file");
    Ultracam::Mskyline mskyline(sskylines);

    std::string name;
    input.get_value("data", name, "blank", "file or file list to add sky lines to");

    // Read file or list
    std::vector<std::string> flist;
    if(Ultracam::Frame::is_ultracam(name)){
      flist.push_back(name);
    }else{      
      std::ifstream istr(name.c_str());
      while(istr >> name){
	flist.push_back(name);
      }
      istr.close();
      if(flist.size() == 0) throw Input_Error("No file names loaded");
    }

    // Read first file for establishing defaults
    Ultracam::Frame frame(flist[0]);

    if(mskyline.size() != frame.size())
      throw Ultracam::Input_Error("Conflicting numbers of CCDs in skyline file and first data file");

    double scale;
    input.get_value("scale", scale, 1., DBL_MIN, DBL_MAX, "intensity scaling factor");

    int xover;
    input.get_value("xover", xover, 1, 1, 100, "oversampling factor in X (subdivisions/unbinned pixel)");

    float xdrift, ydrift;
    Subs::INT4 seed;
    int nreset = 1;
    if(flist.size() > 1){
      input.get_value("seed",    seed,    657687, INT_MIN, INT_MAX, "seed integer for random number generator");
      input.get_value("xdrift",  xdrift,  0.f, -100.f, 100.f, "drift in X per image");
      input.get_value("ydrift",  ydrift,  0.f, -100.f, 100.f, "drift in Y per image");
      input.get_value("nreset",  nreset,  1, 1, 1000000, "number of images before resetting drift");
    }

    float xoff, yoff;

    // Loop over frames
    for(size_t im=0; im<flist.size(); im++){

      // Read data
      Ultracam::Frame data(flist[im]);

      // Compute the amount of drift
      xoff = xdrift*(im % nreset);
      yoff = ydrift*(im % nreset);

      // Now add sky lines to each window of each CCD
      for(size_t nccd=0; nccd<data.size(); nccd++){ // CCDs

	for(size_t nwin=0; nwin<data[nccd].size(); nwin++){ // windows

	  Ultracam::Windata& win = data[nccd][nwin];
	  int nxs = win.xbin()*xover;

	  for(size_t nline=0; nline<mskyline[nccd].size(); nline++){ // lines

	    Ultracam::Skyline& line = mskyline[nccd][nline];

	    // Now add in the sky line to every pixel
	    for(int iy=0; iy<win.ny(); iy++){

	      // X, Y position of the centre of the pixel relative to the spectrum
	      double y  = win.xccd(iy) + yoff;
	      double x  = line.get_position(y) + xoff;

	      // Compute total counts for this column and width
	      double width = line.get_fwhm(y);
	      double total = line.get_strength()/sqrt(Constants::TWOPI*width/Constants::EFAC);

	      for(int ix=0; ix<win.nx(); ix++){
		double dx  = win.xccd(ix) - x;
		double sum = 0.;
		for(int ixs=0; ixs<nxs; ixs++){
		  double diff = Subs::sqr((dx + (ixs+0.5)/nxs - 0.5)/(width/Constants::EFAC))/2;
		  if(diff < 80.) sum += total*exp(-diff);
		}
		win[iy][ix] += sum/nxs;
	      }
	    }
	  }
	}
      }

      data.write(flist[im]);
      std::cout << "Written " << flist[im] << " to disk" << std::endl;

    }
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

