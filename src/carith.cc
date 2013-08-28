/*

!!begin
!!title   cadd to add a constant to an Ultracam frame
!!author  T.R. Marsh
!!created 16 May 2001
!!revised 11 February 2002
!!descr   Adds constant to an Ultracam frame
!!css   style.css
!!root    cadd
!!index   cadd
!!class   Programs
!!class   Arithematic
!!head1   cadd - adds constant to an Ultracam frame

!!emph{cadd} adds a constant to an Ultracam frame and outputs the
answer.

!!head2 Invocation

cadd input constant output [nccd nwin]!!break

!!head2 Arguments

!!table
!!arg{input}{Input frame}
!!arg{constant}{Constant to add}
!!arg{output}{Output frame}
!!arg{nccd}{CCD number if only one to be added, except that 0 means all but 
then allows a particular window of each CCD to be handled.}
!!arg{nwin}{Window number if only one to be processed.}
!!table

See also !!ref{csub.html}{csub}, !!ref{cmul.html}{cmul},
!!ref{cdiv.html}{cdiv}, !!ref{add.html}{add},
!!ref{sub.html}{sub}, !!ref{mul.html}{mul},
!!ref{div.html}{div}, !!ref{uset.html}{uset}.

!!end

!!begin
!!title   csub to subtract a constant from an Ultracam frame
!!author  T.R. Marsh
!!created 16 May 2001
!!revised 11 February 2002
!!descr   Subtracts a constant from an Ultracam frame
!!css   style.css
!!root    csub
!!index   csub
!!class   Programs
!!class   Arithematic
!!head1   csub - subtracts a constant from Ultracam frame

!!emph{csub} subtracts a constant from an Ultracam frame.

!!head2 Invocation

csub input constant output [nccd nwin]!!break

!!head2 Arguments

!!table
!!arg{input}{Input frame}
!!arg{constant}{Constant to subtract}
!!arg{output}{Output frame}
!!arg{nccd}{CCD number if only one to be subtracted, except 
that 0 means all but then allows a particular window of each 
CCD to be handled.}
!!arg{nwin}{Window number if only one to be processed.}
!!table

See also !!ref{cadd.html}{cadd}, !!ref{cmul.html}{cmul},
!!ref{cdiv.html}{cdiv}, !!ref{add.html}{add},
!!ref{sub.html}{sub}, !!ref{mul.html}{mul},
!!ref{div.html}{div}, !!ref{uset.html}{uset}.

!!end

!!begin
!!title   cmul to multiply an Ultracam frame by a constant
!!author  T.R. Marsh
!!created 16 May 2001
!!revised 11 February 2002
!!descr   Multiplies Ultracam frames
!!css   style.css
!!root    cmul
!!index   cmul
!!class   Programs
!!class   Arithematic
!!head1   cmul - multiplies Ultracam frames

!!emph{cmul} multiplies an Ultracam frame by a constant.

!!head2 Invocation

cmul input constant output [nccd nwin]!!break

!!head2 Arguments

!!table
!!arg{input}{Input frame}
!!arg{constant}{Constant to multiply by}
!!arg{output}{Output frame}
!!arg{nccd}{CCD number if only one to be multiplied, 
except that 0 means all but  then allows a particular 
window of each CCD to be handled.}
!!arg{nwin}{Window number if only one to processed.}
!!table

See also !!ref{cadd.html}{cadd}, !!ref{csub.html}{csub},
!!ref{cdiv.html}{cdiv}, !!ref{add.html}{add},
!!ref{sub.html}{sub}, !!ref{mul.html}{mul},
!!ref{div.html}{div}, !!ref{uset.html}{uset}.

!!end

!!begin
!!title   cdiv to divide an Ultracam frame by a constant
!!author  T.R. Marsh
!!created 16 May 2001
!!revised 11 February 2002
!!descr   Divides an Ultracam frame by a constant
!!css   style.css
!!root    cdiv
!!index   cdiv
!!class   Programs
!!class   Arithematic
!!head1   cdiv - divides an Ultracam frame by a constant

!!emph{cdiv} divides an Ultracam frame by a constant.

!!head2 Invocation

cdiv input constant output [nccd nwin]!!break

!!head2 Arguments

!!table
!!arg{input}{Input frame}
!!arg{constant}{Second input frame}
!!arg{output}{Constant to divide by}
!!arg{nccd}{CCD number if only one to be divided, except that 0 means all but 
then allows a particular window of each CCD to be handled.}
!!arg{nwin}{Window number if only one to be processed.}
!!table

See also !!ref{cadd.html}{cadd}, !!ref{csub.html}{csub},
!!ref{cmul.html}{cmul}, !!ref{add.html}{add},
!!ref{sub.html}{sub}, !!ref{mul.html}{mul},
!!ref{div.html}{div}, !!ref{uset.html}{uset}.

!!end

!!begin

!!title   cset 
!!author  T.R. Marsh
!!created 16 May 2001
!!revised 11 February 2002
!!descr   Sets an Ultracam frame to a constant
!!css   style.css
!!root    cset
!!index   cset
!!class   Programs
!!class   Arithematic
!!class   Testing
!!head1   cset - sets an Ultracam frame to a constant

!!emph{cset} sets an Ultracam frame or a particular CCD
and window of a frame to a constant.

!!head2 Invocation

cset input constant output [nccd nwin]!!break

!!head2 Arguments

!!table
!!arg{input}{Input frame}
!!arg{constant}{Constant to set}
!!arg{output}{Output frame}
!!arg{nccd}{CCD number if only one to be set, except that 0 means all but 
then allows a particular window of each CCD to be handled.}
!!arg{nwin}{Window number if only one to be processed.}
!!table

See also !!ref{cadd.html}{cadd}, !!ref{csub.html}{csub},
!!ref{cmul.html}{cmul}, !!ref{add.html}{add},
!!ref{sub.html}{sub}, !!ref{mul.html}{mul},
!!ref{div.html}{div}, !!ref{uset.html}{uset}.

!!end


*/

#include <cstdlib>
#include <cfloat>
#include <climits>
#include <string>
#include <map>
#include "trm/subs.h"
#include "trm/input.h"
#include "trm/frame.h"
#include "trm/ultracam.h"

int main(int argc, char* argv[]){

  using Ultracam::Input_Error;
  using Ultracam::Ultracam_Error;

  try{

    // Set comm to name following last slash,if any.

    std::string comm  = argv[0];
    size_t slash = comm.find_last_of('/');
    if(slash != std::string::npos) comm.erase(0,slash+1);

    const int NCOM = 5;
    std::string command[NCOM] = {"cadd", "csub", "cmul", "cdiv", 
			    "cset"};

    bool recog = false;
    for(int i=0; i<NCOM && !recog; i++) recog = (comm == command[i]);

    if(!recog) throw Input_Error(std::string("Could not recognise command = ") + comm);

    // Construct Input object

    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // sign-in input variables

    input.sign_in("input",    Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("constant", Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("output",   Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("nccd",     Subs::Input::LOCAL, Subs::Input::NOPROMPT);
    input.sign_in("nwin",     Subs::Input::LOCAL, Subs::Input::NOPROMPT);

    // Get inputs

    std::string infile;
    input.get_value("input", infile, "input", "input file");
    Ultracam::Frame frame(infile);

    std::string prompt = "constant to ";
    if(comm == "cadd"){
      prompt += "add";
    }else if(comm == "csub"){
      prompt += "subtract";
    }else if(comm == "cmul"){
      prompt += "multiply by";
    }else if(comm == "cdiv"){
      prompt += "divide by";
    }else if(comm == "cset"){
      prompt += "set image to";
    }
    float constant;
    input.get_value("constant", constant, 0.f, -FLT_MAX, FLT_MAX, prompt);

    std::string output;
    input.get_value("output", output, "output", "output file");

    int nccd;
    input.get_value("nccd", nccd, int(0), int(0), int(frame.size()), "CCD number");
    if(nccd > 0 && frame.size() > 1) std::cout << "Operation will be carried out on CCD " << nccd << " only." << std::endl;

    size_t nwin;
    if(nccd){
      input.get_value("nwin", nwin, size_t(0), size_t(0), frame[nccd-1].size(), "window number");
    }else{
      size_t wmax = frame[0].size();
      for(size_t nc=1; nc<frame.size(); nc++) wmax = std::max(wmax, frame[nc].size());
      input.get_value("nwin", nwin, size_t(0), size_t(0), wmax, "window number");
    }
    if(nwin > 0) std::cout << "Operation will be carried out on window " << nwin << " only." << std::endl;

    if(comm == "cadd"){
      if(nccd){
	nccd--;
	if(nwin){
	  nwin--;
	  frame[nccd][nwin] += constant;
	}else{
	  for(size_t iw=0; iw<frame[nccd].size(); iw++)
	    frame[nccd][iw] += constant;
	}
      }else if(nwin){
	nwin--;
	for(size_t ic=0; ic<frame.size(); ic++)
	  if(nwin < frame[ic].size())
	    frame[ic][nwin] += constant;
      }else{
	frame += constant;
      }
    }else if(comm == "csub"){
      if(nccd){
	nccd--;
	if(nwin){
	  nwin--;
	  frame[nccd][nwin] -= constant;
	}else{
	  for(size_t iw=0; iw<frame[nccd].size(); iw++)
	    frame[nccd][iw] -= constant;
	}
      }else if(nwin){
	nwin--;
	for(size_t ic=0; ic<frame.size(); ic++)
	  if(nwin < frame[ic].size())
	    frame[ic][nwin] -= constant;
      }else{
	frame -= constant;
      }
    }else if(comm == "cmul"){
      if(nccd){
	nccd--;
	if(nwin){
	  nwin--;
	  frame[nccd][nwin] *= constant;
	}else{
	  for(size_t iw=0; iw<frame[nccd].size(); iw++)
	    frame[nccd][iw] *= constant;
	}
      }else if(nwin){
	nwin--;
	for(size_t ic=0; ic<frame.size(); ic++)
	  if(nwin < frame[ic].size())
	    frame[ic][nwin] *= constant;
      }else{
	frame *= constant;
      }
    }else if(comm == "cdiv"){
      if(nccd){
	nccd--;
	if(nwin){
	  nwin--;
	  frame[nccd][nwin] /= constant;
	}else{
	  for(size_t iw=0; iw<frame[nccd].size(); iw++)
	    frame[nccd][iw] /= constant;
	}
      }else if(nwin){
	nwin--;
	for(size_t ic=0; ic<frame.size(); ic++)
	  if(nwin < frame[ic].size())
	    frame[ic][nwin] /= constant;
      }else{
	frame /= constant;
      }
    }else if(comm == "cset"){
      if(nccd){
	nccd--;
	if(nwin){
	  nwin--;
	  frame[nccd][nwin] = constant;
	}else{
	  for(size_t iw=0; iw<frame[nccd].size(); iw++)
	    frame[nccd][iw] = constant;
	}
      }else if(nwin){
	nwin--;
	for(size_t ic=0; ic<frame.size(); ic++)
	  if(nwin < frame[ic].size())
	    frame[ic][nwin] = constant;
      }else{
	frame = constant;
      }
    }else{
      std::string disaster = "Can't process command ";
      disaster += comm;
      throw comm;
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



