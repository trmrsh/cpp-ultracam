/*

!!begin

!!title  add to add Ultracam frames
!!author T.R. Marsh
!!date   15 May 2001
!!descr  Adds Ultracam frames
!!css   style.css
!!root   add
!!index  add
!!class  Programs
!!class  Arithematic
!!head1  add - adds Ultracam frames

!!emph{add} adds two Ultracam frames and outputs a third.

!!head2 Invocation

add input1  input2 [coerce] output [nccd nwin]!!break

output = input1 + input2

!!head2 Arguments

!!table
!!arg{input1}{First input frame}
!!arg{input2}{Second input frame. The windows of this frame must at least match or enclose those
of the first frame. }
!!arg{coerce}{Coerce the second input file to match the first's format or not. Not always possible: needs
binning factors to be correctly related and window start positions to match up correctly.}
!!arg{output}{Output frame}
!!arg{nccd}{CCD number if only one to be added, except that 0 
means all but then allows a particular window of each CCD to be 
handled.}
!!arg{nwin}{Window number if only one to be added.}
!!table

See also !!ref{sub.html}{sub}, !!ref{mul.html}{mul},
!!ref{div.html}{div}, !!ref{uset.html}{uset},
!!ref{cadd.html}{cadd}, !!ref{csub.html}{csub}, 
!!ref{cmul.html}{cmul}, !!ref{cdiv.html}{cdiv}.

!!end

!!begin
!!title  sub to subtract Ultracam frames
!!author T.R. Marsh
!!date   15 May 2001
!!descr  Subtracts Ultracam frames
!!css   style.css
!!root   sub
!!index  sub
!!class  Programs
!!class  Arithematic
!!head1  sub - subtracts Ultracam frames

!!emph{sub} subtracts two Ultracam frames and outputs a third.

!!head2 Invocation

sub input1 input2 [coerce] output [nccd nwin]!!break

output = input1 - input2

!!head2 Arguments

!!table
!!arg{input1}{First input frame}
!!arg{input2}{Second input frame. The windows of this frame must at least match or enclose those
of the first frame. }
!!arg{coerce}{Coerce the second input file to match the first's format or not. Not always possible: needs
binning factors to be correctly related and window start positions to match up correctly.}
!!arg{output}{Output frame}
!!arg{nccd}{CCD number if only one to be subtracted, 
except that 0 means all but 
then allows a particular window of each CCD to be handled.}
!!arg{nwin}{Window number if only one to be subtracted.}
!!table

See also !!ref{add.html}{add}, !!ref{mul.html}{mul},
!!ref{div.html}{div}, !!ref{uset.html}{uset},
!!ref{cadd.html}{cadd}, !!ref{csub.html}{csub}, 
!!ref{cmul.html}{cmul}, !!ref{cdiv.html}{cdiv}.

!!end

!!begin
!!title  mul to multiply Ultracam frames
!!author T.R. Marsh
!!date   15 May 2001
!!descr  Multiplies Ultracam frames
!!css   style.css
!!root   mul
!!index  mul
!!class  Programs
!!class  Arithematic
!!head1  mul - multiplies Ultracam frames

!!emph{mul} multiplies two Ultracam frames and outputs a third.

!!head2 Invocation

mul input1 input2 [coerce] output [nccd nwin]!!break

output = input1 * input2
 
!!head2 Arguments

!!table
!!arg{input1}{First input frame}
!!arg{input2}{Second input frame. The windows of this frame must at least match or enclose those
of the first frame. }
!!arg{coerce}{Coerce the second input file to match the first's format or not. Not always possible: needs
binning factors to be correctly related and window start positions to match up correctly.}
!!arg{output}{Output frame}
!!arg{nccd}{CCD number if only one to be multiplied, 
except that 0 means all but 
then allows a particular window of each CCD to be handled.}
!!arg{nwin}{Window number if only one to multiplied.}
!!table

See also !!ref{add.html}{add}, !!ref{uset.html}{uset},
!!ref{sub.html}{sub}, !!ref{div.html}{div}, 
!!ref{cadd.html}{cadd}, !!ref{csub.html}{csub}, 
!!ref{cmul.html}{cmul}, !!ref{cdiv.html}{cdiv}.

!!end

!!begin
!!title  div to divide Ultracam frames
!!author T.R. Marsh
!!date   15 May 2001
!!descr  Divides Ultracam frames
!!css   style.css
!!root   div
!!index  div
!!class  Programs
!!class  Arithematic
!!head1  div - divides Ultracam frames

!!emph{div} divides two Ultracam frames and outputs a third.

!!head2 Invocation

div input1 input2 [coerce] output [nccd nwin]!!break

output = input1 /input2

!!head2 Arguments

!!table
!!arg{input1}{First input frame}
!!arg{input2}{Second input frame. The windows of this frame must at least match or enclose those
of the first frame. }
!!arg{coerce}{Coerce the second input file to match the first's format or not. Not always possible: needs
binning factors to be correctly related and window start positions to match up correctly.}
!!arg{output}{Output frame}
!!arg{nccd}{CCD number if only one to be divided, 
except that 0 means all but 
then allows a particular window of each CCD to be handled.}
!!arg{nwin}{Window number if only one to divided.}
!!table

See also !!ref{add.html}{add}, !!ref{sub.html}{sub},
!!ref{mul.html}{mul}, !!ref{uset.html}{uset},
!!ref{cadd.html}{cadd}, !!ref{csub.html}{csub}, 
!!ref{cmul.html}{cmul}, !!ref{cdiv.html}{cdiv}.

!!end

!!begin
!!title   uset to set Ultracam frames
!!author  T.R. Marsh
!!created 16 February 2002
!!descr   Sets/splices Ultracam frames
!!css   style.css
!!root    uset
!!index   uset
!!class   Programs
!!class   Arithematic
!!head1   uset - sets/splices Ultracam frames

!!emph{uset} sets a CCD or a single window of a CCD of
one Ultracam frame to its value in another. This can be used to splice together
frames. For instance to make a single flat field frame from
ones that are well exposed in each CCD separately. 

!!head2 Invocation

uset input1 input2 [coerce] output nccd nwin!!break

!!head2 Arguments

!!table
!!arg{input1}{First input frame.}
!!arg{input2}{Second input frame. The windows of this frame must at least match or enclose those
of the first frame. }
!!arg{coerce}{Coerce the second input file to match the first's format or not. Not always possible: needs
binning factors to be correctly related and window start positions to match up correctly.}
!!arg{output}{Output frame. This will be the same as the first input, but with
some parts replaced by data from the second input frame.}
!!arg{nccd}{CCD number. 0 means all, but allows a particular window of 
each CCD to be set}
!!arg{nwin}{Window number if only one to be set, 0 for all.}
!!table

See also !!ref{add.html}{add}, !!ref{sub.html}{sub},
!!ref{mul.html}{mul}, !!ref{div.html}{div},
!!ref{cadd.html}{cadd}, !!ref{csub.html}{csub}, 
!!ref{cmul.html}{cmul}, !!ref{cdiv.html}{cdiv}.

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

    // Set comm to name following last slash,if any.
    std::string comm  = argv[0];
    size_t slash = comm.find_last_of('/');
    if(slash != std::string::npos) comm.erase(0,slash+1);

    const int NCOM = 5; 
    std::string command[NCOM] = {"add", "sub", "mul", "div", "uset"};

    bool recog = false;
    for(int i=0; i<NCOM && !recog; i++)
      recog = (comm == command[i]);

    if(!recog) throw Ultracam::Input_Error(std::string("Could not recognise command = ") + comm);

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // sign-in input variables
    input.sign_in("input1", Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("input2", Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("coerce", Subs::Input::GLOBAL,Subs::Input::NOPROMPT);
    input.sign_in("output", Subs::Input::LOCAL, Subs::Input::PROMPT);
    if(comm == "uset"){
      input.sign_in("nccd",   Subs::Input::LOCAL, Subs::Input::PROMPT);
      input.sign_in("nwin",   Subs::Input::LOCAL, Subs::Input::PROMPT);
    }else{
      input.sign_in("nccd",   Subs::Input::LOCAL, Subs::Input::NOPROMPT);
      input.sign_in("nwin",   Subs::Input::LOCAL, Subs::Input::NOPROMPT);
    }


    // Get inputs
    std::string input1;
    input.get_value("input1", input1, "input1", "first input file");
    Ultracam::Frame frame1(input1);

    std::string input2;
    input.get_value("input2", input2, "input2", "second input file");
    Ultracam::Frame frame2(input2);

    bool coerce;
    input.get_value("coerce", coerce, true, "coerce second input file to match the first?");
    
    if(coerce){
      frame2.crop(frame1);
    }else{
      if(frame1 != frame2) throw Ultracam::Input_Error("Input frames do not match!");
    }

    std::string output;
    input.get_value("output", output, "output", "output file");

    int nccd;
    input.get_value("nccd", nccd, 0, 0, int(frame1.size()), "CCD number");
    if(nccd > 0 && frame1.size() > 1) std::cout << "Operation will be carried out on CCD " << nccd << " only." << std::endl;

    size_t nwin;
    if(nccd){
      input.get_value("nwin", nwin, size_t(0), size_t(0), frame1[nccd-1].size(), "window number");
    }else{
      size_t wmax = frame1[0].size();
      for(size_t nc=1; nc<frame1.size(); nc++) wmax = std::max(wmax, frame1[nc].size());
      input.get_value("nwin", nwin, size_t(0), size_t(0), wmax, "window number");
    }
    if(nwin > 0) std::cout << "Operation will be carried out on window " << nwin << " only." << std::endl;

    if(comm == "add"){
      if(nccd){
	nccd--;
	if(nwin){
	  nwin--;
	  frame1[nccd][nwin] += frame2[nccd][nwin];
	}else{
	  for(size_t iw=0; iw<frame1[nccd].size(); iw++)
	    frame1[nccd][iw] += frame2[nccd][iw];
	}
      }else if(nwin){
	nwin--;
	for(size_t ic=0; ic<frame1.size(); ic++)
	  if(frame1[ic].size() > nwin)
	    frame1[ic][nwin] += frame2[ic][nwin];
      }else{
	frame1 += frame2;
      }

    }else if(comm == "sub"){
      if(nccd){
	nccd--;
	if(nwin){
	  nwin--;
	  frame1[nccd][nwin] -= frame2[nccd][nwin];
	}else{
	  for(size_t iw=0; iw<frame1[nccd].size(); iw++)
	    frame1[nccd][iw] -= frame2[nccd][iw];
	}
      }else if(nwin){
	nwin--;
	for(size_t ic=0; ic<frame1.size(); ic++)
	  if(frame1[ic].size() > nwin)
	    frame1[ic][nwin] -= frame2[ic][nwin];
      }else{
	frame1 -= frame2;
      }

    }else if(comm == "mul"){
      if(nccd){
	nccd--;
	if(nwin){
	  nwin--;
	  frame1[nccd][nwin] *= frame2[nccd][nwin];
	}else{
	  for(size_t iw=0; iw<frame1[nccd].size(); iw++)
	    frame1[nccd][iw] *= frame2[nccd][iw];
	}
      }else if(nwin){
	nwin--;
	for(size_t ic=0; ic<frame1.size(); ic++)
	  if(frame1[ic].size() > nwin)
	    frame1[ic][nwin] *= frame2[ic][nwin];
      }else{
	frame1 *= frame2;
      }

    }else if(comm == "div"){
      if(nccd){
	nccd--;
	if(nwin){
	  nwin--;
	  frame1[nccd][nwin] /= frame2[nccd][nwin];
	}else{
	  for(size_t iw=0; iw<frame1[nccd].size(); iw++)
	    frame1[nccd][iw] /= frame2[nccd][iw];
	}
      }else if(nwin){
	nwin--;
	for(size_t ic=0; ic<frame1.size(); ic++)
	  if(frame1[ic].size() > nwin)
	    frame1[ic][nwin] /= frame2[ic][nwin];
      }else{
	frame1 /= frame2;
      }

    }else if(comm == "uset"){
      if(nccd){
	nccd--;
	if(nwin){
	  nwin--;
	  frame1[nccd][nwin] = frame2[nccd][nwin];
	}else{
	  for(size_t iw=0; iw<frame1[nccd].size(); iw++)
	    frame1[nccd][iw] = frame2[nccd][iw];
	}
      }else if(nwin){
	nwin--;
	for(size_t ic=0; ic<frame1.size(); ic++)
	  if(frame1[ic].size() > nwin)
	    frame1[ic][nwin] = frame2[ic][nwin];
      }else{
	frame1 = frame2;
      }

    }else{
      throw Ultracam::Ultracam_Error(std::string("Can't process command = ") + comm);
    }

    // Output
    frame1.write(output);

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


