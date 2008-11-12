/*

!!begin
!!title  dsub to subtract a dark frame from multiple Ultracam frames
!!author T.R. Marsh
!!created 06 Jan 2006
!!revised 13 Nov 2007
!!descr  Subtracts a drak frame from multiple Ultracam frames
!!css    style.css
!!root   dsub
!!index  dsub
!!class  Programs
!!class  Arithematic
!!head1  dsub - subtracts a dark frame from multiple Ultracam frames

!!emph{dsub} subtracts a dark frame from many others, overwriting the
frames. It scales the dark frame according to the ratio of exposure times,
if they can be found. It aborts if the exposure times cannot be found.
The dark frame must be bias subtracted and created from a series of
frames with identical exposure times. 

There are some subtleties with dark subtraction. If the data are bias subtracted and the bias has 
a significant exposure time, then rather less dark needs subtracting. Thr reverse applies if the dark
itself has had a finite length bias subtracted.

!!head2 Invocation

dsub flist dark [coerce nccd nwin]

!!head2 Arguments

!!table
!!arg{flist}{Frame list}
!!arg{dark}{Dark frame to subtract}
!!arg{coerce}{Coerce the frame to be subtracted the input file formats or not. Not always possible: needs
binning factors to be correctly related and window start positions to match up correctly.}
!!arg{nccd}{CCD number if only one to be subtracted, except that 0 means all but 
then allows a particular window of each CCD to be handled.}
!!arg{nwin}{Window number if only one to be subtracted.}
!!table

See also !!ref{add.html}{add}, !!ref{mul.html}{mul}, !!ref{sub.html}{sub},
!!ref{div.html}{div}, !!ref{uset.html}{uset},
!!ref{cadd.html}{cadd}, !!ref{csub.html}{csub}, 
!!ref{cmul.html}{cmul}, !!ref{cdiv.html}{cdiv}, !!ref{mdiv.html}{mdiv} and
!!ref{msub.html}{msub}.

!!end

*/

#include <cstdlib>
#include <string>
#include <map>
#include "trm_subs.h"
#include "trm_input.h"
#include "trm_frame.h"
#include "trm_ultracam.h"

int main(int argc, char* argv[]){

    try{

	// Construct Input object
	Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

	// sign-in input variables
	input.sign_in("flist",  Subs::Input::LOCAL, Subs::Input::PROMPT);
	input.sign_in("dark",   Subs::Input::LOCAL, Subs::Input::PROMPT);
	input.sign_in("coerce", Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("nccd",   Subs::Input::LOCAL, Subs::Input::NOPROMPT);
	input.sign_in("nwin",   Subs::Input::LOCAL, Subs::Input::NOPROMPT);

	// Get inputs
	std::string sflist;
	input.get_value("flist", sflist, "flist", "list of ultracam frames");

	// Read file or list
	std::string name;
	std::vector<std::string> flist;
	std::ifstream istr(sflist.c_str());
	while(istr >> name){
	    flist.push_back(name);
	}
	istr.close();
	if(flist.size() == 0) throw Ultracam::Input_Error("No file names loaded");

	std::string sframe;
	input.get_value("dark", sframe, "dark", "dark frame to subtract");
	Ultracam::Frame frame(sframe);

	float dark_expose;
	frame["Exposure"]->get_value(dark_expose);
	if(dark_expose <= 0.f) 
	    throw Ultracam::Input_Error("Exposure time in dark frame must be > 0.");

	float dark_bias_expose;
	frame["Bias_exposure"]->get_value(dark_bias_expose);
	if(dark_bias_expose > dark_expose) 
	    throw Ultracam::Input_Error("Bias used for dark has exposure time > dark itself");

	bool coerce;
	input.get_value("coerce", coerce, true, "coerce second input file to match the first?");
    
	Ultracam::Frame save;
	if(coerce) save = frame;

	size_t nccd;
	input.get_value("nccd", nccd, size_t(0), size_t(0), frame.size(), "CCD number");
	if(nccd > 0 && frame.size() > 1) std::cout << "Operation will be carried out on CCD " << nccd << " only." << std::endl;

	size_t nwin;
	if(nccd){
	    input.get_value("nwin", nwin, size_t(0), size_t(0), frame[nccd-1].size(), "window number");
	}else{
	    size_t wmax = frame[0].size();
	    for(size_t nc=1; nc<frame.size(); nc++) wmax = std::max(wmax, frame[nc].size());
	    input.get_value("nwin", nwin, size_t(0), size_t(0), frame[nccd-1].size(), "window number");
	}
	if(nwin > 0) std::cout << "Operation will be carried out on window " << nwin << " only." << std::endl;

	// OK carry out op
	Ultracam::Frame work;
	for(size_t nf=0; nf<flist.size(); nf++){

	    work.read(flist[nf]);

	    float work_expose;
	    work["Exposure"]->get_value(work_expose);
	    if(work_expose <= 0.f) 
		throw Ultracam::Ultracam_Error("Exposure time in frame = " + flist[nf] + " frame must be > 0.");

	    // Also need exposure time of bias frame used on data (if any) for proper dark subtraction
	    float bias_expose = 0.f;
	    Subs::Header::Hnode *hnode = work.find("Bias_exposure");
	    if(hnode->has_data())
		hnode->value->get_value(bias_expose);
	    else
		std::cerr << "No bias exposure found in data file; assume not bias subtracted." << std::endl;
	    if(work_expose <= bias_expose) 
		throw Ultracam::Ultracam_Error("Exposure time in frame = " + flist[nf] + " is less than the bias used on the frame");
      
	    // Restore frame to be subtracted
	    if(coerce && work != frame){
		frame = save;
		frame.crop(work);
	    }else if(work != frame) {
		throw Ultracam::Input_Error("Format of input frame = " + flist[nf] + " does not match frame = " + sframe);
	    }

	    if(nccd){
		nccd--;
		if(nwin){
		    nwin--;
		    work[nccd][nwin] -= (work_expose-bias_expose)/(dark_expose-dark_bias_expose)*frame[nccd][nwin];
		}else{
		    for(size_t iw=0; iw<work[nccd].size(); iw++)
			work[nccd][iw] -= (work_expose-bias_expose)/(dark_expose-dark_bias_expose)*frame[nccd][iw];
		}
	    }else if(nwin){
		nwin--;
		for(size_t ic=0; ic<work.size(); ic++)
		    if(work[ic].size() > nwin)
			work[ic][nwin] -= (work_expose-bias_expose)/(dark_expose-dark_bias_expose)*frame[ic][nwin];
	    }else{
		work -= (work_expose/dark_expose)*frame;
	    }

	    std::cout << "Subtracted " << sframe << " scaled by " << (work_expose-bias_expose)/(dark_expose-dark_bias_expose) << " from " << flist[nf] << std::endl;

	    // Output
	    work.write(flist[nf]);

	}

    }
    catch(const Ultracam::Input_Error& err){
	std::cerr << "Ultracam::Input_Error exception:" << std::endl;
	std::cerr << err << std::endl;
	exit(EXIT_FAILURE);
    }
    catch(const Ultracam::Ultracam_Error& err){
	std::cerr << "Ultracam::Ultracam_Error exception:" << std::endl;
	std::cerr << err << std::endl;
	exit(EXIT_FAILURE);
    }
    catch(const Subs::Subs_Error& err){
	std::cerr << "Subs::Subs_Error exception:" << std::endl;
	std::cerr << err << std::endl;
	exit(EXIT_FAILURE);
    }
    catch(const std::string& err){
	std::cerr << err << std::endl;
	exit(EXIT_FAILURE);
    }
}


