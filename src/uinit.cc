/*

!!begin
!!title    Ultracam initialiser
!!author   T.R. Marsh
!!created  17 April 2001
!!revised  15 June 2001
!!descr    Creates a blank Ultracam frame
!!css   style.css
!!root     uinit
!!index    uinit
!!class    Programs
!!class    Testing
!!head1    Uinit - creates a blank ultracam frame

!!emph{uinit} takes a window defined by, for example,
!!ref{setwin.html}{setwin} and produces an equivalent data frame, set
to zero. It is the first step towards creating more complex frames.
In addition to defining the windows and generating equivalent data
arrays, !!emph{uinit} sets up the readout noise and gain for each
window.

!!head2 Invocation

uinit window out!!break

!!head2 Arguments

!!table
!!arg{window}{A window file (see !!ref{setwin.html}{setwin}.}
!!arg{data}{Output ultracam data frame.}
!!table

!!head2 Test & Development status

!!emph{unit} works, and is complete.

!!end

*/

#include <cstdlib>
#include <string>
#include <map>
#include <vector>
#include "trm/input.h"
#include "trm/frame.h"
#include "trm/mccd.h"
#include "trm/window.h"
#include "trm/ultracam.h"

int main(int argc, char* argv[]){

  try{

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Define inputs
    input.sign_in("window", Subs::Input::GLOBAL,  Subs::Input::PROMPT);
    input.sign_in("data",   Subs::Input::GLOBAL, Subs::Input::PROMPT);

    std::string window;
    input.get_value("window", window, "window", "window file");
    Ultracam::Mwindow  win(window);
    std::string sdata;
    input.get_value("data", sdata, "run001", "data file for output");

    Ultracam::Frame data(win);

    // Set to 0
    data = 0.f;

    // Set up a fake header

    data.set("Object", new Subs::Hstring("FAKE DATA", "Object name"));
    data.set("Exposure", new Subs::Hfloat(1.2345f, "Exposure time, seconds"));
    data.set("UT_date", new Subs::Htime(Subs::Time(17, Subs::Date::Nov, 2041, 12, 34, 56.789f), "Date and time, UT"));
    data.set("Site", new Subs::Hdirectory("Observing site information"));
    data.set("Site.Observatory", new Subs::Hstring("An excellent site", "Name of the observing site"));
    data.set("Site.Telescope",   new Subs::Hstring("The Enormous Telescope", "Name of the telescope"));
    data.set("Instrument", new Subs::Hdirectory("Instrument information"));
    data.set("Instrument.Gain_Speed", new Subs::Hstring("Just Right", "Gain speed setting"));

    // Output
    data.write(sdata);

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



