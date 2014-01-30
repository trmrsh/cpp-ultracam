/*

!!begin
!!title   lists pixels of an Ultracam frame
!!author  T.R. Marsh
!!created 29 June 2004
!!descr   Lists pixels with a specified range of values
!!css   style.css
!!root    list
!!index   list
!!class   Programs
!!class   Arithematic
!!head1   list - lists pixels with a specified range of values

!!emph{list} lists all pixels within a specific range of values of a given CCD from
an ULTRACAM frame. This is meant to be an aid in finding bad pixels. Output from this can be
edited into defect files. The pixel values are listed along with their position on the CCD
and their position within the window.

!!head2 Invocation

list input nccd min max

!!head2 Arguments

!!table
!!arg{input}{Input frame}
!!arg{nccd}{The CCD to list}
!!arg{min}{Minimum value to consider}
!!arg{max}{Maximum value to consider}
!!table

!!end

*/

#include <cstdlib>
#include <cfloat>
#include <climits>
#include <string>
#include <map>
#include "trm/subs.h"
#include "trm/input.h"
#include "trm/frame.h"
#include "trm/ultracam.h"

int main(int argc, char* argv[]){

  using Ultracam::Input_Error;
  using Ultracam::Ultracam_Error;

  try{

    // Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Sign-in input variables
    input.sign_in("input",    Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("nccd",     Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("min",      Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("max",      Subs::Input::LOCAL, Subs::Input::PROMPT);

    // Get inputs
    std::string infile;
    input.get_value("input", infile, "input", "input file");
    Ultracam::Frame frame(infile);

    size_t nccd;
    input.get_value("nccd", nccd, size_t(1), size_t(1), frame.size(), "CCD number to examine");
    nccd--;

    float vmin;
    input.get_value("min", vmin, 0.f, -FLT_MAX, FLT_MAX, "minimum pixel value to consider");

    float vmax;
    input.get_value("max", vmax, std::max(1.f, vmin), vmin, FLT_MAX, "maximum pixel value to consider");

    for(size_t iw=0; iw<frame[nccd].size(); iw++){
      const Ultracam::Windata &dwin = frame[nccd][iw];
      for(int iy=0; iy<dwin.ny(); iy++){
    for(int ix=0; ix<dwin.nx(); ix++){
      if(dwin[iy][ix] >= vmin && dwin[iy][ix] <= vmax)
        std::cout << "Window " << iw+1 << ", pixel (" << ix << "," << iy
         << "), value = " << dwin[iy][ix] << ", position on CCD = "
         << dwin.xccd(ix) << ", " << dwin.yccd(iy) << std::endl;
    }
      }
    }
  }

  catch(Ultracam::Input_Error err){
    std::cerr << "Ultracam::Input_Error exception:" << std::endl;
    std::cerr << err << std::endl;
  }
  catch(Ultracam::Ultracam_Error err){
    std::cerr << "Ultracam::Ultracam_Error exception:" << std::endl;
    std::cerr << err << std::endl;
  }
  catch(Subs::Subs_Error err){
    std::cerr << "Subs::Subs_Error exception:" << std::endl;
    std::cerr << err << std::endl;
  }
  catch(std::string err){
    std::cerr << err << std::endl;
  }
}



