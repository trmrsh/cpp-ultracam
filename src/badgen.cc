/*

!!begin
!!title   generates bad pixel frames
!!author  T.R. Marsh
!!created 29 June 2004
!!descr   Generates bad pixel frames
!!css     style.css
!!root    badgen
!!index   badgen
!!class   Programs
!!class   Arithematic
!!class   Manipulation
!!head1   badgen - generates bad pixel frames

!!emph{badgen} generates a frame in which all pixels within an input frame that lie within
a certain range of values are set to 1 while all the others are set to zero. This can be used
to generate bad pixel masks. A bad pixel mask is an ordinary ultracam frame in which good
pixels have been set = 0, while bad pixels are set to integral positive values. For instance, 
take a flat field, smooth it with !!ref{smooth.html}{smooth} and divide by its smoothed version. Then you can 
identify all pixels with value below 0.8 (or something) as bad using this routine. You can define several 
levels of badness with multiple runs of this routine, and add the resulting frames together after multiplication 
of the individual frames by an integer denoting the level of badness. Avoid the value 10 which I
reserve for possible cosmic ray IDs in the future.
!!ref{reduce.html}{reduce} then stores the worst bad pixel encountered in each aperture in the log file.

!!head2 Invocation

badgen input min max output

!!head2 Arguments

!!table
!!arg{input}{Input frame}
!!arg{min}{Minimum value to consider}
!!arg{max}{Maximum value to consider}
!!arg{output}{The output  frame set to 1 whenever pixel values lie within range}
!!table

!!end

*/

#include <cstdlib>
#include <cfloat>
#include <climits>
#include <string>
#include <map>
#include "trm_subs.h"
#include "trm_input.h"
#include "trm_frame.h"
#include "trm_ultracam.h"

int main(int argc, char* argv[]){

  using Ultracam::Input_Error;
  using Ultracam::Ultracam_Error;

  try{

    // Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Sign-in input variables
    input.sign_in("input",    Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("min",      Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("max",      Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("output",   Subs::Input::LOCAL, Subs::Input::PROMPT);

    // Get inputs
    std::string infile;
    input.get_value("input", infile, "input", "input file");
    Ultracam::Frame frame(infile);

    float vmin;
    input.get_value("min", vmin, 0.f, -FLT_MAX, FLT_MAX, "minimum pixel value to consider");

    float vmax;
    input.get_value("max", vmax, std::max(1.f, vmin), vmin, FLT_MAX, "maximum pixel value to consider");

    std::string outfile;
    input.get_value("output", outfile, "output", "the output file");

    for(size_t ic=0; ic<frame.size(); ic++){
      for(size_t iw=0; iw<frame[ic].size(); iw++){
	Ultracam::Windata &dwin = frame[ic][iw];
	for(int iy=0; iy<frame[ic][iw].ny(); iy++){
	  for(int ix=0; ix<frame[ic][iw].nx(); ix++){
	    if(dwin[iy][ix] >= vmin && dwin[iy][ix] <= vmax)
	      dwin[iy][ix] = 1;
	    else
	      dwin[iy][ix] = 0;
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



