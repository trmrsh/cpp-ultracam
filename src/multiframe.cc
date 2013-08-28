/*

!!begin
!!title  msub to subtract a frame from multiple Ultracam frames
!!author T.R. Marsh
!!created 02 Feb 2004
!!descr  Subtracts a frame from multiple Ultracam frames
!!css   style.css
!!root   msub
!!index  msub
!!class  Programs
!!class  Arithematic
!!head1  msub - subtracts a frame from multiple Ultracam frames

!!emph{msub} subtracts a constant frame from many others, overwriting the
frames. Note that for the common operation of bias subtraction you should 
probably use !!ref{bsub.html}{bsub} in preference to this program because it stores
the exposure time of the bias in the output file.

!!head2 Invocation

msub flist frame [coerce nccd nwin]!!break

!!head2 Arguments

!!table
!!arg{flist}{Frame list}
!!arg{frame}{Frame to subtract. The windows of this frame must at least match or enclose those
of the frame list}
!!arg{coerce}{Coerce the frame to be subtracted the input file formats or not. Not always possible: needs
binning factors to be correctly related and window start positions to match up correctly.}
!!arg{nccd}{CCD number if only one to be subtracted, except that 0 means all but 
then allows a particular window of each CCD to be handled.}
!!arg{nwin}{Window number if only one to be subtracted.}
!!table

See also !!ref{add.html}{add}, !!ref{mul.html}{mul}, !!ref{sub.html}{sub},
!!ref{div.html}{div}, !!ref{uset.html}{uset},
!!ref{cadd.html}{cadd}, !!ref{csub.html}{csub},
!!ref{cmul.html}{cmul}, !!ref{cdiv.html}{cdiv}, !!ref{mdiv.html}{mdiv}
!!ref{dsub.html}{dsub}, !!ref{bsub.html}{bsub}.

!!end

!!begin
!!title  bsub to subtract a frame from multiple Ultracam frames
!!author T.R. Marsh
!!created 13 Nov 2007
!!descr  Subtracts a frame from multiple Ultracam frames
!!css   style.css
!!root   bsub
!!index  bsub
!!class  Programs
!!class  Arithematic
!!head1  bsub - subtracts a frame from multiple Ultracam frames

!!emph{bsub} subtracts a bias frame from many others, overwriting the
frames. !!emph{bsub} stores the exposure time of the bias frame used
in the output file, as required for accurate dark subtraction with e.g.
!!ref{dsub.html}{dsub} or during operation of !!ref{reduce.html}{reduce}.

!!head2 Invocation

bsub flist bias [coerce nccd nwin]!!break

!!head2 Arguments

!!table
!!arg{flist}{Frame list}
!!arg{bias}{bias frame to subtract. The windows of this frame must at least match or enclose those
of the frame list}
!!arg{coerce}{Coerce the frame to be subtracted the input file formats or not. Not always possible: needs
binning factors to be correctly related and window start positions to match up correctly.}
!!arg{nccd}{CCD number if only one to be subtracted, except that 0 means all but 
then allows a particular window of each CCD to be handled.}
!!arg{nwin}{Window number if only one to be subtracted.}
!!table

See also !!ref{add.html}{add}, !!ref{mul.html}{mul}, !!ref{sub.html}{sub},
!!ref{div.html}{div}, !!ref{uset.html}{uset},
!!ref{cadd.html}{cadd}, !!ref{csub.html}{csub}, 
!!ref{cmul.html}{cmul}, !!ref{cdiv.html}{cdiv}, !!ref{mdiv.html}{mdiv},
!!ref{dsub.html}{dsub}, !!ref{msub.html}{msub}

!!end

!!begin
!!title  mdiv to divide a frame into multiple Ultracam frames
!!author T.R. Marsh
!!created 11 June 2004
!!descr  Divides a frame into multiple Ultracam frames
!!css   style.css
!!root   mdiv
!!index  mdiv
!!class  Programs
!!class  Arithematic
!!head1  mdiv - divides a frame from multiple Ultracam frames

!!emph{mdiv} divides a constant frame into many others, overwriting the
input frames. 

!!head2 Invocation

mdiv flist frame [coerce nccd nwin]!!break

!!head2 Arguments

!!table
!!arg{flist}{Frame list}
!!arg{frame}{Frame to subtract. The windows of this frame must at least match or enclose those
of the frame list}
!!arg{coerce}{Coerce the frame to be used to divide the input file formats or not. Not always possible: needs
binning factors to be correctly related and window start positions to match up correctly.}
!!arg{nccd}{CCD number if only one to be subtracted, except that 0 means all but 
then allows a particular window of each CCD to be handled.}
!!arg{nwin}{Window number if only one to be subtracted.}
!!table

See also !!ref{add.html}{add}, !!ref{mul.html}{mul}, !!ref{sub.html}{sub},
!!ref{div.html}{div}, !!ref{uset.html}{uset},
!!ref{cadd.html}{cadd}, !!ref{csub.html}{csub}, 
!!ref{cmul.html}{cmul}, !!ref{cdiv.html}{cdiv}, !!ref{msub.html}{msub}
and !!ref{dsub.html}{dsub}.

!!end

*/

#include <cstdlib>
#include <string>
#include <map>
#include "trm/subs.h"
#include "trm/input.h"
#include "trm/frame.h"
#include "trm/ultracam.h"

int main(int argc, char* argv[]){

    try{

	// Set comm to name following last slash,if any.
	std::string comm  = argv[0];
	size_t slash = comm.find_last_of('/');
	if(slash != std::string::npos) comm.erase(0,slash+1);

	const int NCOM = 3; 
	std::string command[NCOM] = {"msub", "mdiv", "bsub"};

	bool recog = false;
	for(int i=0; i<NCOM && !recog; i++)
	    recog = (comm == command[i]);

	if(!recog) throw Ultracam::Input_Error(std::string("Could not recognise command = ") + comm);

	// Construct Input object
	Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

	// sign-in input variables
	input.sign_in("flist",  Subs::Input::LOCAL, Subs::Input::PROMPT);
	input.sign_in("frame",  Subs::Input::LOCAL, Subs::Input::PROMPT);
	input.sign_in("bias",   Subs::Input::LOCAL, Subs::Input::PROMPT);
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
	if(comm == "msub"){
	    input.get_value("frame", sframe, "frame", "frame to subtract");
	}else if(comm == "bsub"){
	    input.get_value("bias", sframe, "frame", "bias frame to subtract");
	}else if(comm == "mdiv"){
	    input.get_value("frame", sframe, "frame", "frame to divide by");
	}
	Ultracam::Frame frame(sframe);

	float bias_expose = 0;
	if(comm == "bsub")
	    frame["Exposure"]->get_value(bias_expose);

	bool coerce;
	input.get_value("coerce", coerce, true, "coerce second input file to match the first?");
    
	Ultracam::Frame save;
	if(coerce) save = frame;

	int nccd;
	input.get_value("nccd", nccd, int(0), int(0), int(frame.size()), "CCD number");
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

	    // Restore frame to be subtracted
	    if(coerce && work != frame){
		frame = save;
		frame.crop(work);
	    }else if(work != frame) {
		throw Ultracam::Input_Error("Format of input frame = " + flist[nf] + " does not match frame = " + sframe);
	    }

	    if(comm == "msub" || comm == "bsub"){
		if(nccd){
		    nccd--;
		    if(nwin){
			nwin--;
			work[nccd][nwin] -= frame[nccd][nwin];
		    }else{
			for(size_t iw=0; iw<work[nccd].size(); iw++)
			    work[nccd][iw] -= frame[nccd][iw];
		    }
		}else if(nwin){
		    nwin--;
		    for(size_t ic=0; ic<work.size(); ic++)
			if(work[ic].size() > nwin)
			    work[ic][nwin] -= frame[ic][nwin];
		}else{
		    work -= frame;
		}
		std::cout << "Subtracted " << sframe << " from " << flist[nf] << std::endl;
	
		if(comm == "bsub")
		    work.set("Bias_exposure", new Subs::Hfloat(bias_expose, "Exposure time of bias subtracted from this frame"));

	    }else if(comm == "mdiv"){
		if(nccd){
		    nccd--;
		    if(nwin){
			nwin--;
			work[nccd][nwin] /= frame[nccd][nwin];
		    }else{
			for(size_t iw=0; iw<work[nccd].size(); iw++)
			    work[nccd][iw] /= frame[nccd][iw];
		    }
		}else if(nwin){
		    nwin--;
		    for(size_t ic=0; ic<work.size(); ic++)
			if(work[ic].size() > nwin)
			    work[ic][nwin] /= frame[ic][nwin];
		}else{
		    work /= frame;
		}
		std::cout << "Divided " << flist[nf] << " by " << sframe << std::endl;

	    }else{
		throw Ultracam::Ultracam_Error(std::string("Can't process command = ") + comm);
	    }

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


