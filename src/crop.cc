/*

!!begin

!!title   Cropping of Ultracam frames
!!author  T.R. Marsh
!!created 28 February 2002
!!revised 10 November 2003
!!descr   Crops Ultracam frames
!!css   style.css
!!root    crop
!!index   crop
!!class   Programs
!!class   Arithematic
!!class   Testing
!!class   Manipulation
!!head1   crop - crops Ultracam frames

!!emph{crop} crops an Ultracam frames to match a
set of windows either from an ASCII file or from another Ultracam frame. 
The windows in the frame to be cropped must enclose the new windows. Their 
binning factors must divide into those of the new windows and the pixels must be in step.

!!head2 Invocation

crop input window output!!break

!!head2 Arguments

!!table
!!arg{input}{Input frame}
!!arg{window}{Multi-window file (as produced by !!ref{setwin.html}{setwin} for example) or ULTRACAM file 
to crop down to. The program first looks for a window file, with extension ".win". Failing this it looks for 
a file with extension ".ucm" and uses the window from this instead.}
!!arg{output}{Output, cropped frame}
!!table

See also !!ref{bcrop.html}{bcrop} and !!ref{window.html}{window}

!!end

*/

#include <cstdlib>
#include <string>
#include <map>
#include "trm_subs.h"
#include "trm_input.h"
#include "trm_frame.h"
#include "trm_mccd.h"
#include "trm_ultracam.h"

int main(int argc, char* argv[]){

  using Ultracam::Ultracam_Error;

  try{

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // sign-in input variables
    input.sign_in("input",  Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("window", Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("output", Subs::Input::LOCAL, Subs::Input::PROMPT);

    // Get inputs
    std::string sinput;
    input.get_value("input", sinput, "input", "file to crop");
    Ultracam::Frame indata(sinput);

    std::string swindow;
    input.get_value("window", swindow, "window", "the window or frame to chop down to");

    std::string output;
    input.get_value("output", output, "output", "file to dump result to");

    Ultracam::Mwindow window;
    try{
      window.rasc(swindow);

      // Crop
      indata.crop(window);    
    }

    // If we fail to find a window file, look for a Frame instead
    catch(const Ultracam::File_Open_Error&){
      Ultracam::Frame temp(swindow);

      // Crop
      indata.crop(temp);    
    }

    indata.write(output);

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



