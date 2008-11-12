/*

!!begin
!!title   backsub to subtract background off an Ultracam frame
!!author  T.R. Marsh
!!created 07 Mar 2006
!!descr   Subtracts the background off an Ultracam frame
!!css     style.css
!!root    backsub
!!index   backsub
!!class   Programs
!!class   Arithematic
!!head1   backsub - subtracts the background off an Ultracam frame

!!emph{backsub} subtracts the background off each window of an Ultracam frame.
This is a routine used to get rid of the background without having to identify
an appropriate bias frame.

!!head2 Invocation

backsub input centile output

!!head2 Arguments

!!table
!!arg{input}{Input frame}
!!arg{centile}{Centile to use as the background (0 to 100)}
!!arg{output}{Output frame}

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

    // Construct Input object

    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // sign-in input variables

    input.sign_in("input",    Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("centile",  Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("output",   Subs::Input::LOCAL, Subs::Input::PROMPT);

    // Get inputs

    std::string infile;
    input.get_value("input", infile, "input", "input file");
    Ultracam::Frame frame(infile);

    float centile;
    input.get_value("centile", centile, 50.f, 0.f, 100.f, "centile to compute the background of each window");

    std::string output;
    input.get_value("output", output, "output", "output file");

    for(size_t nccd=0; nccd<frame.size(); nccd++){
      for(size_t nwin=0; nwin<frame[nccd].size(); nwin++){
	float back;
	frame[nccd][nwin].centile(centile/100.f, back); 
	frame[nccd][nwin] -= back;
      }
    }

    // Output
    frame.write(output);

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



