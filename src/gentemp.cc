/*

!!begin
!!title   ULTRACAM pipeline: gentemp
!!author  T.R. Marsh
!!created 18 October 2005
!!root    gentemp
!!index   gentemp
!!descr   makes template files
!!css     style.css
!!class   Programs
!!class   Spectra
!!class   Testing
!!head1   gentemp generates template ASCII files

There are several instances in the software of ASCII input files, e.g. to define windows,
spectra or sky lines. The format of these files is not very memorable, and fixed example
templates carry the risk of becoming out of date. This program uses the current version of
the software to generate example of these files and should therefore always give a valid
output, assuming that there are no bugs.

For each template type the program prints out some information about what it is doing to help you interpret
the ASCII file.

!!head2 Invocation

gentemp nccd type name

!!head2 Command line arguments

!!table

!!arg{nccd}{The number of CCDs to use}
!!arg{type}{The type of template to generate. Current choices are 'skylines', 'spectra' and 'windows', 
all case insensitive. You should of course be aware that in some cases interactive programs exist such
as !!ref{setwin.html}{setwin}}
!!arg{name}{This is the root name of the output file which will have a standard extension such as '.win' for
window files}

!!table

See also !!ref{addfield.html}{addfield}, !!ref{addsky.html}{addsky}, !!ref{addspec.html}{addspec}, !!ref{setaper.html}{setaper}, 
!!ref{setdefect.html}{setdefect}, !!ref{setfield.html}{setfield}, !!ref{setwin.html}{setwin}.
 
!!end

*/

#include <cstdlib>
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include "trm_subs.h"
#include "trm_input.h"
#include "trm_ultracam.h"
#include "trm_mccd.h"
#include "trm_window.h"
#include "trm_spectrum.h"
#include "trm_skyline.h"

int main(int argc, char* argv[]){

  using Ultracam::Input_Error;
  using Ultracam::Ultracam_Error;

  try{

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Define inputs
    input.sign_in("nccd",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("type",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("name",     Subs::Input::LOCAL,  Subs::Input::PROMPT);

    // Get inputs
    int nccd;
    input.get_value("nccd", nccd, 1, 1, 10, "number of CCDs");
    std::string type;
    input.get_value("type", type, "windows", "type of template to generate");
    type == Subs::tolower(type);
    if(type != "windows" && type != "spectra" && type != "skylines"){
      std::cerr << "Unrecognised type = " << type << "\n" << std::endl;
      std::cerr << "Recognised types are:\n" << std::endl;
      std::cerr << "skylines  -- makes a set of sky lines for use by addsky" << std::endl;
      std::cerr << "spectra   -- makes a set of spectra for use by addspec" << std::endl;
      std::cerr << "windows   -- makes a file of windows" << std::endl;
      throw Ultracam_Error("Invalid type input");
    }

    std::string name;
    input.get_value("name", name, "window", "name of output file");

    if(type == "spectra"){

      Ultracam::Mspectrum spectra(nccd);
      Ultracam::Spectrum spectrum1, spectrum2;

      spectrum1.add_line(Ultracam::Spectrum::Line(300.,10.,20.,50000,1.,0.));
      std::cout << "Spectrum 1, emission line added at x=300, height=10, FWHM=20, T0=50000, Period=1, semi-amp=0" << std::endl;
      spectrum1.add_line(Ultracam::Spectrum::Line(550.,3.,15.,50000,1.,0.));
      std::cout << "Spectrum 1, emission line added at x=550, height=3, FWHM=15, T0=50000, Period=1, semi-amp=0" << std::endl;

      Subs::Array1D<double> linear(2);
      linear[0] = 450;
      linear[1] = 2;
      Subs::Poly position(true, 1, 1000, linear);
      spectrum1.set_position(position);
      std::cout << "Spectrum 1, Y position set to have a linear gradient of 2 pixels from end-to-end, mean = 450" << std::endl;

      linear[0] = 1;
      linear[1] = 0.2;
      Subs::Poly continuum(true, 1, 1000, linear);
      spectrum1.set_continuum(continuum);
      std::cout << "Spectrum 1, continuum set to have a linear gradient of 0.2 end-to-end, mean = 1" << std::endl;


      Subs::Array1D<double> quad(3);
      quad[0] = 3;
      quad[1] = 0.;
      quad[2] = 0.5;
      Subs::Poly fwhm(true, 1, 1000, quad);
      spectrum1.set_fwhm(fwhm);
      std::cout << "Spectrum 1, FWHM set to be a quadratic with value 3 in the centre, rising to 3.5 at the ends of the spectrum" << std::endl;

      spectrum2.add_line(Ultracam::Spectrum::Line(500.,-0.5, 3.));
      std::cout << "Spectrum 2, absorption line added at x=500, depth=0.5, FWHM=3" << std::endl;
      spectrum2.add_line(Ultracam::Spectrum::Line(610.,0.2,5.));
      std::cout << "Spectrum 2, absorption line added at x=610, depth=0.2, FWHM=3" << std::endl;
      position[0] = 550;
      spectrum2.set_position(position);
      spectrum2.set_continuum(continuum);
      spectrum2.set_fwhm(fwhm);
      std::cout << "Spectrum 2, parallel to spectrum 1, moved to 550; continuum and FWHM the same" << std::endl;
		       
      for(int nc=0; nc<nccd; nc++){
	spectra[nc].push_back(spectrum1);
	spectra[nc].push_back(spectrum2);
      }

      if(nccd > 1)
	std::cout << "Same spectra set in all CCDs" << std::endl;

      // Write it out
      spectra.wasc(name);

      std::cout << "Spectra written to " << name << std::endl;

    }else if(type == "windows"){

      Ultracam::Mwindow windows(nccd);
      Ultracam::Window window(10,400,1000,100,1,2,1024,1000);
      std::cout << "Set 1 window with lower-left corner at x=10, y=400, 1000 binned pixels in X, 100 in Y," << std::endl;
      std::cout << "xbin=1, ybin=2, total CCD readout dimensions (unbinned) 1024 by 1000" << std::endl;
      
      for(int nc=0; nc<nccd; nc++)
	windows[nc].push_back(window);

      // Write it out
      windows.wasc(name);

      std::cout << "Windows written to " << name << std::endl;


    }else if(type == "skylines"){

      Ultracam::Mskyline skylines(nccd);
      Ultracam::Skyline skyline1, skyline2;

      Subs::Array1D<double> linear(2), quadratic(3);

      linear[0] = 450;
      linear[1] = 2;
      skyline1.set_position(Subs::Poly(true, 1, 400, linear));

      linear[0] = 750;
      skyline2.set_position(Subs::Poly(true, 1, 400, linear));

      quadratic[0] = 3;
      quadratic[1] = 0.5;
      quadratic[2] = 1.5;
      skyline1.set_fwhm(Subs::Poly(true, 1, 400, quadratic));
      skyline2.set_fwhm(Subs::Poly(true, 1, 400, quadratic));
			
      skyline1.set_strength(100);
      skyline2.set_strength(500);

			
      for(int nc=0; nc<nccd; nc++){
	skylines[nc].push_back(skyline1);
	skylines[nc].push_back(skyline2);
      }

      // Write it out
      skylines.wasc(name);

      std::cout << "Skylines written to " << name << std::endl;

    }else{
      std::cerr << "Sorry this option is not yet implemented" << std::endl;
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
