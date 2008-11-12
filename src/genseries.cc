/*

!!begin
!!title   Generates a series of frames
!!author  T.R. Marsh
!!created 18 October 2005
!!root    genseries
!!index   genseries
!!descr   generates a series of frames
!!css     style.css
!!class   Programs
!!class   Testing
!!head1   genseries generates a series of frames

!!emph{genseries} generates a series of frames. It does so by copying
one frames many times and modifying the times. 

!!head2 Invocation

genseries data time1 time2 ntime root

!!head2 Command line arguments

!!table

!!arg{data}{The template data file}

!!arg{time1}{The first time, in MJD of the oputput times}

!!arg{time2}{The last time, in MJD of the oputput times}

!!arg{nframe}{The number of frames}

!!arg{root}{The root file name. The output names will be of the form root01, root02 etc}

!!table

!!end

*/

#include <cfloat>
#include <cstdlib>
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include "trm_subs.h"
#include "trm_input.h"
#include "trm_frame.h"
#include "trm_mccd.h"
#include "trm_target.h"
#include "trm_ultracam.h"


std::string fname(const std::string& root, int nim, int ndig);

int main(int argc, char* argv[]){

  using Ultracam::Input_Error;
  using Ultracam::Ultracam_Error;

  try{

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Define inputs
    input.sign_in("data",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("time1",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("time2",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("nframe",   Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("root",     Subs::Input::GLOBAL, Subs::Input::PROMPT);

    // Get inputs
    std::string sdata;
    input.get_value("data", sdata, "blank", "name of template data file");
    Ultracam::Frame data(sdata);

    double time1;
    input.get_value("time1", time1, 50000., -DBL_MAX, DBL_MAX,  "first time of sequence");

    double time2;
    input.get_value("time2", time2, 55000., -DBL_MAX, DBL_MAX,  "last time of sequence");

    int nframe;
    input.get_value("nframe", nframe, 10, 1, 1000000, "number of images in sequence");

    std::string root;
    input.get_value("root", root, "blank", "name of template data file");

    int ndigit = int(log10(nframe+0.5))+1;

    for(int im=0; im<nframe; im++){

      // Set the time
      double time = time1 + (time2-time1)*im/std::max(1, nframe-1);
      data.set("UT_date",  new Subs::Htime(Subs::Time(time), "UT date and time at the centre of the exposure"));

      // Create output name
      std::string output = root + Subs::str(im+1, ndigit);

      // Write out
      data.write(output);
      std::cout << "Written " << output << " to disk" << std::endl;

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





