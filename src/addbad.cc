/*

!!begin
!!title   Adds new bad pixels to a bad pixel frame
!!author  T.R. Marsh
!!created 21 December 2005
!!descr   Adds new bad pixels to a bad pixel frame
!!css     style.css
!!root    addbad
!!index   addbad
!!class   Programs
!!class   Arithematic
!!class   Manipulation
!!head1   addbad - adds new bad pisles to a bad pixel frame

!!emph{addbad} reads in a bad pixel frame and a defect file and uses the latter to
set new bad pixels (values > 0) in the bad pixel file. Nothing is done if the pixels
are already non-zero, so this routine is really just to give an extra bit of control
over locating bad pixels.

!!head2 Invocation

addbad input defect low high output

!!head2 Arguments

!!table
!!arg{input}{Input frame}
!!arg{defect}{The defect file}
!!arg{low}{The level to set the not-so-bad defects to}
!!arg{high}{The level to set the terrible defects to}
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
#include "trm/mccd.h"
#include "trm/defect.h"
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
    input.sign_in("defect",   Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("low",      Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("high",     Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("output",   Subs::Input::LOCAL, Subs::Input::PROMPT);

    // Get inputs
    std::string infile;
    input.get_value("input", infile, "input", "input file");
    Ultracam::Frame frame(infile);

    std::string dfile;
    input.get_value("defect", dfile, "defect", "defect file");
    Ultracam::Mdefect defect(dfile);

    float low;
    input.get_value("low", low, 10.f, 0.f, FLT_MAX, "level for not-so-bad defects");

    float high;
    input.get_value("high", high, std::max(100.f, low), low, FLT_MAX, "level for really bad defects");

    std::string outfile;
    input.get_value("output", outfile, "output", "the output file");

    // Set the values
    for(size_t ic=0; ic<frame.size(); ic++){
      for(size_t iw=0; iw<frame[ic].size(); iw++){
    Ultracam::Windata &dwin = frame[ic][iw];
    for(int iy=0; iy<dwin.ny(); iy++){
      for(int ix=0; ix<dwin.nx(); ix++){
        if(dwin[iy][ix] < 0.5){

          float sum = 0.f;

          // A binned pixel gets the worst value of any of its components
          int icylo = dwin.lly() + dwin.ybin()*iy;
          int icyhi = icylo      + dwin.ybin();
          int icxlo = dwin.llx() + dwin.xbin()*ix;
          int icxhi = icxlo      + dwin.xbin();
          for(int icy=icylo; icy < icyhi; icy++){
        for(int icx=icxlo; icx < icxhi; icx++){
          float worst = 0.f;
          for(size_t id=0; id<defect[ic].size(); id++){
            float bval = defect[ic][id].bad_value(icx, icy, low, high);
            worst = worst <  bval ?  bval : worst;
          }
          sum += worst;
        }
          }

          dwin[iy][ix] = sum;
        }
      }
    }
      }
    }

    frame.write(outfile);

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



