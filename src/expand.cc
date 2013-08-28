/*

!!begin

!!title   Expands Ultracam frames
!!author  T.R. Marsh
!!created 08 November 2006
!!revised 19 November 2006
!!descr   Expands Ultracam frames
!!css     style.css
!!root    expand
!!index   expand
!!class   Programs
!!class   Arithematic
!!class   Spectra
!!class   Manipulation
!!head1   expand - expands Ultracam frames

!!emph{expand} approximately reverses the effect of !!ref{collapse.html}{collapse}. i.e.
it takes an Ultracam frame consisting of null or 1D windows and expands them by repeating the
data across the frame to make it match a reference image. The resulting frame can then be used to 
divide into, subtract from etc the reference image. A complication is that one or more of the windows
that result from !!ref{collapse.html}{collapse} may have zero dimension. They are not eliminated in order
to keep the number and order of windows the same. However, they will be grown back as well. In this case
you must think what value you want them to have. If you intend to add or subtract the expanded frame, you probably
want to grow the data back with zeroes, while for multiplication or division you probably want to use 1. You
will be prompted for the value to use.

!!head2 Invocation

expand input template value output!!break

!!head2 Arguments

!!table
!!arg{input}{Input frame to be expanded}
!!arg{template}{Frame defining the sizes of the expanded windows. The windows will have to match in their lower-left corners
and non-collapsed dimensions.}
!!arg{value}{The value to use when expanding null windows; see above for a discussion of why this is needed.}
!!arg{output}{Output, a frame in which one of the dimensions of the windows is collapsed to 1.}
!!table

!!head2 See also

!!ref{collapse.html}{collapse}

!!end

*/

#include <cstdlib>
#include <cfloat>
#include <string>
#include <map>
#include "trm/subs.h"
#include "trm/input.h"
#include "trm/array1d.h"
#include "trm/frame.h"
#include "trm/mccd.h"
#include "trm/ultracam.h"

int main(int argc, char* argv[]){
    
    using Ultracam::Ultracam_Error;
    
    try{
	
	// Construct Input object
	Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

	// Sign-in input variables
	input.sign_in("input",    Subs::Input::LOCAL, Subs::Input::PROMPT);
	input.sign_in("template", Subs::Input::LOCAL, Subs::Input::PROMPT);
	input.sign_in("value",    Subs::Input::LOCAL, Subs::Input::PROMPT);
	input.sign_in("output",   Subs::Input::LOCAL, Subs::Input::PROMPT);

	// Get inputs
	std::string sinput;
	input.get_value("input", sinput, "input", "file to expand");
	Ultracam::Frame indata(sinput);

	std::string stemp;
	input.get_value("template", stemp, "template", "template file to define expanded windows");
	Ultracam::Frame temp(stemp);

	float value;
	input.get_value("value", value, 0.f, -FLT_MAX, FLT_MAX, "value to use for null window expansion");

	std::string output;
	input.get_value("output", output, "output", "file to dump result to");


	Subs::Array1D<float> profile;
	for(size_t nccd=0; nccd<indata.size(); nccd++){
	    for(size_t nwin=0; nwin<indata[nccd].size(); nwin++){
		Ultracam::Windata& win = indata[nccd][nwin];
		const Ultracam::Windata& twin = temp[nccd][nwin];
		
		bool dir_is_x;
		if(win.nx() == twin.nx() && (win.ny() == 0 || win.ny() == 1)){
		    dir_is_x = false;
		}else if(win.ny() == twin.ny() && (win.nx() == 0 || win.nx() == 1)){
		    dir_is_x = true;
		}else{
		    throw Ultracam::Ultracam_Error("NCCD = " + Subs::str(nccd+1) + " window " + Subs::str(nwin+1) + 
						   " has incompatible dimensions in template versus input file.");
		}

		if(dir_is_x){

		    // Save data
		    profile.resize(win.ny());
		    if(win.nx()){
			for(int iy=0; iy<win.ny(); iy++)
			    profile[iy] = win[iy][0];
		    }else{
			profile = value;
		    }
		    
		    // Resize and copy back
		    win.resize(win.ny(), twin.nx());
		    
		    for(int iy=0; iy<win.ny(); iy++)
			for(int ix=0; ix<win.nx(); ix++)
			    win[iy][ix] = profile[iy];

		}else{

		    // Save data
		    profile.resize(win.nx());
		    if(win.ny()){
			for(int ix=0; ix<win.nx(); ix++)
			    profile[ix] = win[0][ix];
		    }else{
			profile = value;
		    }
		    
		    // Resize and copy back
		    win.resize(twin.ny(), win.nx());
		    
		    for(int iy=0; iy<win.ny(); iy++)
			for(int ix=0; ix<win.nx(); ix++)
			    win[iy][ix] = profile[ix];
		}
	    }
	}
    
	// Write out the result	
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



