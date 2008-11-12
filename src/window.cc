/*

!!begin

!!title   Windowing of Ultracam frames
!!author  T.R. Marsh
!!created 11 June 2004
!!descr   Windows Ultracam frames
!!css   style.css
!!root    window
!!index   window
!!class   Programs
!!class   Arithematic
!!class   Testing
!!class   Manipulation
!!head1   window - windows Ultracam frames

!!emph{window} windows an Ultracam frame in the sense that the result is
as if one only kept the region of the frame visible though the window.
See also !ref{crop.html}{crop}. The binning factors of any overlapping
windows must match.

!!head2 Invocation

window input window output!!break

!!head2 Arguments

!!table
!!arg{input}{Input frame}
!!arg{window}{Multi-window file (as produced by !!ref{setwin.html}{setwin} for example).}
!!arg{output}{Output, cropped frame}
!!table

See also: !!ref{crop.html}{crop} and !!ref{bcrop.html}{bcrop}

!!end

*/

#include <cstdlib>
#include <string>
#include <map>
#include "trm_subs.h"
#include "trm_input.h"
#include "trm_frame.h"
#include "trm_mccd.h"
#include "trm_window.h"
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
    input.get_value("input", sinput, "input", "file to window");
    Ultracam::Frame indata(sinput);

    std::string swindow;
    input.get_value("window", swindow, "window", "the window to apply");
    Ultracam::Mwindow window(swindow);

    std::string output;
    input.get_value("output", output, "output", "file to dump result to");

    // Window
    indata.window(window);    

    // dump to disk
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



