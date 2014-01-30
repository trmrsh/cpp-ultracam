/*

!!begin

!!title  Joins windows in Ultracam frames
!!author T.R. Marsh
!!created 04 December 2002
!!revised 01 July2004
!!descr  Joins all windows into 1 large one
!!css   style.css
!!root   wjoin
!!index  wjoin
!!class  Programs
!!class  Manipulation
!!head1  wjoin joins all windows into 1.

!!emph{wjoin} puts all windows in each CCD of an ULTRACAM frame into one.
It finds the minimum window needed and pads any extra pixels out with a value
chosen by the user.

!!head2 Invocation

wjoin input null (output)!!break

!!head2 Arguments

!!table
!!arg{input}{Input frame or list of frames}
!!arg{null}{The null value to fill in empty pixels with.}
!!arg{output}{Output frame if only one frame is being joined. Otherwise
the !!emph{input files are overwritten.}}
!!table

!!end

*/

#include <cstdlib>
#include <cfloat>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <vector>
#include "trm/subs.h"
#include "trm/input.h"
#include "trm/frame.h"
#include "trm/mccd.h"
#include "trm/window.h"
#include "trm/ultracam.h"

int main(int argc, char* argv[]){

  using Ultracam::Ultracam_Error;


  try{

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Sign-in input variables
    input.sign_in("input",  Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("null",   Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("output", Subs::Input::LOCAL, Subs::Input::PROMPT);

    // Get inputs
    std::string sinput;
    input.get_value("input", sinput, "input", "file to join");

    // Read file or list modified to read lists for wjoin
    std::vector<std::string> slist;
    bool flist = true;
    if(Ultracam::Frame::is_ultracam(sinput)){
    slist.push_back(sinput);
    flist = false;
    }else{
      std::ifstream istr(sinput.c_str());
      while(istr >> sinput){
    slist.push_back(sinput);
      }
      istr.close();
      if(slist.size() == 0) throw Ultracam::Input_Error("No file names loaded");
    }

    float nvalue;
    input.get_value("null", nvalue, 0.f, -FLT_MAX, FLT_MAX, "value to fill empty pixels with");

    std::string output;
    if(!flist){
      input.get_value("output", output, "output", "file to dump result to");
    }else{
      std::cerr << "input files will be overwritten" << std::endl;
    }


    for(size_t nfile=0; nfile<slist.size(); nfile++){
      Ultracam::Frame indata(slist[nfile]);
      Ultracam::Frame outdata(indata.size());

      // Copy over headers
      (Subs::Header&)outdata = (Subs::Header&)indata;

      // Derive pixel limits of single window.
      int llx, lly, urx, ury, nxtot, nytot;
      int xbin, ybin, nx, ny;

      for(size_t nc=0; nc<indata.size(); nc++){

    // Following are assumed to be the same for each window
    xbin  = indata[nc][0].xbin();
    ybin  = indata[nc][0].ybin();
    nxtot = indata[nc][0].nxtot();
    nytot = indata[nc][0].nytot();

    // Next ones must be taken as maximum and minimum, where appropriate
    llx   = indata[nc][0].llx();
    lly   = indata[nc][0].lly();
    urx   = llx + xbin*indata[nc][0].nx();
    ury   = lly + ybin*indata[nc][0].ny();

    for(size_t nw=1; nw<indata[nc].size(); nw++){
      if(indata[nc][nw].xbin()  != xbin  ||
         indata[nc][nw].ybin()  != ybin  ||
         indata[nc][nw].nxtot() != nxtot ||
         indata[nc][nw].nytot() != nytot)
        throw Ultracam::Ultracam_Error("Mis-matching binning factors or CCD size");

      if((llx - indata[nc][nw].llx()) % xbin != 0 ||
         (lly - indata[nc][nw].lly()) % ybin != 0)
        throw Ultracam::Ultracam_Error("Mis-matching window locations");

      llx = std::min( llx, indata[nc][nw].llx());
      lly = std::min( lly, indata[nc][nw].lly());
      urx = std::max( urx, indata[nc][nw].llx() + xbin*indata[nc][nw].nx());
      ury = std::max( ury, indata[nc][nw].lly() + ybin*indata[nc][nw].ny());
    }

    nx = (urx - llx)/xbin;
    ny = (ury - lly)/ybin;

    // Construct single window
    outdata[nc].push_back(Ultracam::Windata(llx,lly,nx,ny,xbin,ybin,nxtot,nytot));

    // Initialise to nvalue
    outdata[nc][0] = nvalue;

    // Now set exposed regions
    for(size_t nw=0; nw<indata[nc].size(); nw++){
      int yoff = (indata[nc][nw].lly() - lly)/ybin;
      for(int iy=0; iy<indata[nc][nw].ny(); iy++, yoff++){
        int xoff = (indata[nc][nw].llx() - llx)/xbin;
        for(int ix=0; ix<indata[nc][nw].nx(); ix++, xoff++){
          outdata[nc][0][yoff][xoff] = indata[nc][nw][iy][ix];
        }
      }
    }
      }

      // Write out result
      if(flist)
    outdata.write(slist[nfile]);
      else
    outdata.write(output);

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



