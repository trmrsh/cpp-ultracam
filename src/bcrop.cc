/*

!!begin

!!title   Crops Ultracam frames accounting for binning problem
!!author  T.R. Marsh
!!created 06 October 2005
!!revised 12 October 2005
!!descr   crops Ultracam frames accounting for binning problem
!!css     style.css
!!root    bcrop
!!index   bcrop
!!class   Programs
!!class   Manipulation
!!head1   bcrop -- crops Ultracam frames accounting for binning problem

Up to and including the August 2004 run, there was an unsuspected problem with
the binning of the ultracam chips.  What happened was that if, say, you binned
by 3 in X, then the 2 closest pixels in X to the readout were lost from every
pixel. If you binned by N in X then the N-1 pixesl closest to the readouts were
lost. This obviously loses flux (and so the problem has since been fixed), but
also means that using the routine !!ref{crop.html}{crop} to bin up an unbinned
frame, e.g. a flat field, is not accurate.  The purpose of this routine is to
modify a frame so that when !!ref{crop.html}{crop} is applied, the end result is
OK. It does this by setting the 'lost' pixels to zero and making sure that there
appear windows that when binned will match the supplied template binned frame.

!!emph{NB} This routine !!emph{MUST} only be applied to a frame !!emph{after} it has been bias-subtracted
(the same applies to !!ref{crop.html}{crop} for that matter).

!!head2 Invocation

bcrop input template output

!!head2 Arguments

!!table
!!arg{input}{Input frame}
!!arg{template}{Binned frame that acts as a template}
!!arg{output}{Output frame which will be cropped & binned allowing for the problem.}
!!table

!!head2 Notes

It only makes sense to use this routine on unbinned frames and you will not be able to use it on any other type.

See also !!ref{crop.html}{crop} and !!ref{window.html}{window}

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
    input.sign_in("input",    Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("template", Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("output",   Subs::Input::LOCAL, Subs::Input::PROMPT);

    // Get inputs
    std::string sinput;
    input.get_value("input", sinput, "input", "unbinned frame to zero");
    Ultracam::Frame indata(sinput);
    for(size_t nccd=0; nccd<indata.size(); nccd++){
      for(size_t nwin=0; nwin<indata[nccd].size(); nccd++){
	if(indata[nccd][nwin].xbin() != 1 || indata[nccd][nwin].ybin() != 1)
	  throw Ultracam_Error("Input frame = " + sinput + " is binned.");
      }
    }

    std::string stemplate;
    input.get_value("template", stemplate, "template", "the binned frame that you wish to match");
    Ultracam::Frame temp(stemplate);

    if(indata.size() != temp.size())
      throw Ultracam::Ultracam_Error("Conflicting numbers of CCDs in the input frames");

    std::string output;
    input.get_value("output", output, "output", "file to dump result to");

    // Generate the format for the output frame but with unbinned pixels.
    Ultracam::Mwindow mwin(temp.size()), owin(temp.size());
    for(size_t nccd=0; nccd<temp.size(); nccd++){
      for(size_t nwin=0; nwin<temp[nccd].size(); nwin++){
	const Ultracam::Windata& wind = temp[nccd][nwin];
	mwin[nccd].push_back(Ultracam::Window(wind.llx(), wind.lly(), wind.xbin()*wind.nx(), wind.ybin()*wind.ny(), 1, 1, wind.nxtot(), wind.nytot()));
	owin[nccd].push_back(Ultracam::Window(wind.llx(), wind.lly(), wind.nx(), wind.ny(), wind.xbin(), wind.ybin(), wind.nxtot(), wind.nytot()));
      }
    }

    // Now try to crop the input frame to this
    try {
      indata.crop(mwin);
    }
    catch(const Ultracam::Modify_Error& msg){
      throw Ultracam::Ultracam_Error("zmask failed to crop input files to unbinned version of template windows with the following message\n" + msg);
    }

    // OK, now set pixels closest to the readouts in X to zero
    for(size_t nccd=0; nccd<indata.size(); nccd++){
      for(size_t nwin=0; nwin<indata[nccd].size(); nwin++){
	Ultracam::Windata& wind = indata[nccd][nwin];
	int xbin = owin[nccd][nwin].xbin();
	for(int ny=0; ny<wind.ny(); ny++){
	  for(int nx=0; nx<wind.nx(); nx++){
	    if(nwin % 2 == 0 && nx % xbin < xbin-1){
	      wind[ny][nx] = 0;
	    }else if(nwin % 2 == 1 && nx % xbin > 0){
	      wind[ny][nx] = 0;
	    }
	  }
	}
      }	
    }

    indata.crop(owin);

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



