/*

!!begin
!!title   medians an Ultracam frame in boxes
!!author  T.R. Marsh
!!created 19 September 2005
!!descr   medians an Ultracam frame in boxes
!!css     style.css
!!root    boxmed
!!index   boxmed
!!class   Programs
!!class   Arithematic
!!class   Manipulation
!!head1   boxmed - medians an Ultracam frame in boxes

!!emph{boxmed} generates a frame where each new pixel is the median of the box
of pixels centred on the old. Each window is treated independently. Edge pixels
just use whichever pixels in the box actually exist. 

!!head2 Invocation

boxmed input xhwidth yhwidth output 

!!head2 Arguments

!!table
!!arg{input}{Input frame}
!!arg{xhwidth}{half-width of the box in X. The actual box size in X will be 2*xhwidth+1}
!!arg{yhwidth}{half-width of the box in Y. The actual box size in Y will be 2*yhwidth+1}
!!arg{output}{Output frame}
!!table

See also !!ref{boxavg.html}{boxavg}

!!end

*/

#include <cstdlib>
#include <cfloat>
#include <climits>
#include <string>
#include <map>
#include "trm/subs.h"
#include "trm/array1d.h"
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
    input.sign_in("input",     Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("xhwidth",   Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("yhwidth",   Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("output",    Subs::Input::LOCAL, Subs::Input::PROMPT);

    // Get inputs
    std::string infile;
    input.get_value("input", infile, "input", "input file");
    Ultracam::Frame frame(infile);

    int xhwidth;
    input.get_value("xhwidth", xhwidth, 1, 0, 500, "half-width of box in X");

    int yhwidth;
    input.get_value("yhwidth", yhwidth, 1, 0, 500, "half-width of box in Y");

    std::string output;
    input.get_value("output", output, "output", "output file");
    Ultracam::Frame out = frame;

    // Pre-allocate buffer of sufficient size for efficiency
    Subs::Array1D<float> buffer((2*xhwidth+1)*(2*yhwidth+1));

    for(size_t ic=0; ic<frame.size(); ic++){
      for(size_t iw=0; iw<frame[ic].size(); iw++){
	const Ultracam::Windata &dwin = frame[ic][iw];
	for(int iyn=0; iyn<dwin.ny(); iyn++){
	  for(int ixn=0; ixn<dwin.nx(); ixn++){

	    // Load up buffer
	    buffer.clear();
	    for(int iyo=std::max(iyn-yhwidth,0); iyo<std::min(iyn+yhwidth+1,dwin.ny()); iyo++){
	      for(int ixo=std::max(ixn-xhwidth,0); ixo<std::min(ixn+xhwidth+1,dwin.nx()); ixo++){
		buffer.push_back(dwin[iyo][ixo]);
	      }
	    }
	    out[ic][iw][iyn][ixn] = buffer.median();
	  }
	}
      }
    }

    // Output
    out.write(output);

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



