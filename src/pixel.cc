/*

!!begin
!!title    Prints out information on a pixel in an Ultracam frame
!!author   T.R. Marsh
!!created  02 May 2002
!!root     pixel
!!index    pixel
!!descr    prints out information on a pixel in an Ultracam frame
!!css   style.css
!!class    Programs
!!class    Information
!!head1    pixel - prints out information on a pixel in an Ultracam frame

!!emph{pixel} prints out the information about a single pixel in an ultracam file. It will tell if
the pixel is not valid. The pixels must be specified in binned coordinates.

!!head2 Invocation

pixel image nccd nwin ix iy

!!head2 Command line arguments

!!table
!!arg{data}{Ultracam data file}
!!arg{nccd}{CCD number starting at 1}
!!arg{nwin}{Window number starting at 1}
!!arg{ix, iy}{Pixels coordinates, starting at 1, 1 in the lower left corner of the window.}
!!table

!!end

*/

#include <cstdlib>
#include <string>
#include <sstream>
#include <map>
#include "trm/subs.h"
#include "trm/input.h"
#include "trm/frame.h"
#include "trm/ultracam.h"

int main(int argc, char* argv[]){

  using Ultracam::Ultracam_Error;
  using Ultracam::Input_Error;

  try{

    // Construct Input object

    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // sign-in input variables

    input.sign_in("data",    Subs::Input::GLOBAL,  Subs::Input::PROMPT);
    input.sign_in("nccd",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("nwin",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("ix",      Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("iy",      Subs::Input::LOCAL,  Subs::Input::PROMPT);

    // Get inputs

    std::string sdata;
    input.get_value("data", sdata, "run001", "file name");
    Ultracam::Frame data(sdata);

    int nccd;
    input.get_value("nccd", nccd, int(1), int(1), int(data.size()), "CCD number");
    nccd--;

    int nwin;
    input.get_value("nwin", nwin, int(1), int(1), int(data[nccd].size()), "window number");
    nwin--;

    int ix, iy;
    input.get_value("ix", ix, int(1), int(1), int(data[nccd][nwin].nx()), "X pixel number");
    input.get_value("iy", iy, int(1), int(1), int(data[nccd][nwin].ny()), "Y pixel number");

    std::cout
      << "Pixel " << ix << ", " << iy << " of "
      << "CCD " << nccd+1
      << ", window " << nwin+1 << " covers unbinned range of "
      << " X: "  << data[nccd][nwin].xcomp(ix-0.5f) << " to " << data[nccd][nwin].xcomp(ix+0.5f)
      << ", Y: " << data[nccd][nwin].ycomp(iy-0.5f) << " to " << data[nccd][nwin].ycomp(iy+0.5f)
      << " and has value = " << data[nccd][nwin][iy-1][ix-1] << std::endl;
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

