/*

!!begin
!!title   Window setup program
!!author  T.R. Marsh
!!created 21 February 2001
!!revised 02 December 2006
!!root    setwin
!!index   setwin
!!descr   sets up multi-CCD window format
!!css     style.css
!!class   Programs
!!class   Testing
!!class   Setup
!!head1   setwin for setting up multi-CCD multi-window formats

!!emph{setwin} is an interactive program for defining the format of multiple
windows of multiple CCDs including the overall dimensions and binning factors
as well as the size and locations of the windows. It can accept a window
format as input and display a data frame as a guide. At its end it dumps the
window format defined.  If a data frame is not displayed then only terminal
input is accepted. If a data frame is displayed, then the windows are forced
to be compatible with the data if possible. In the case of binned data this
means ensuring that the windows are 'in step'. This may be accomplished by
subtracting/adding a small amount to the starting X and Y value (less than one binned
pixel). This can occasionally lead to an invalid window, in which case you should re-define
it.  If the exact position is critical, you should re-display the windows
to check that they are OK, and possibly edit the file by hand to shift the
windows back a pixel or whatever. !!emph{setwin} does not force the new
windows to be enclosed by the data windows and thus you may end up making
windows that can be used by !!ref{window.html}{window} but not
!!ref{crop.html}{crop}.

!!emph{setwin} is little more than a front-end to the window
handling classes but it is designed to only generate valid objects.
Although it is possible to edit the file produced, be aware that
you may put into an invalid state. It is certainly worth running
!!emph{setwin} at least once to generate a template for editing.
If you do manage to corrupt the file later you will be told about
when trying to load the file.

Whatever file name you supply, a standard extension will be added
if you have not already specified it. This is to distinguish the
window files from others without you having to worry about it.

!!head2 Invocation

setwin data newfile window (numccd ncset xbin ybin nxtot nytot [device] xleft xright ylow yhigh iset ilow ihigh plow phigh)!!break

!!head2 Command line arguments

!!table
!!arg{data}{Name of data file to set from. This will be used for plotting
and also to ensure that the windows are compatible. Enter NONE if you don't want to specify one.}
!!arg{newfile}{true/false to determine whether the window file is to be new or not}
!!arg{window}{Name of window file. Will be overwritten if it already exists.}
!!arg{numccd}{If new, nccd = Number of CCDs.}
!!arg{ncset}{Number of CCD to set}
!!arg{xbin}{Binning factor in X, only prompted if not already set}
!!arg{ybin}{Binning factor in Y, only prompted if not already set}
!!arg{nxtot}{Unbinned chip X dimension, only prompted if not already set}
!!arg{nytot}{Unbinned chip Y dimension, only prompted if not already set}
!!arg{device}{If plot, device = plot device.}
!!arg{xleft xright}{X range for plot}
!!arg{ylow yhigh}{Y range for plot}
!!arg{iset}{'A', 'D' or 'P' for automatic, direct or percentile setting of intensity}
!!arg{ilow ihigh}{Display range if iset='d'}
!!arg{plow phigh}{Percentile range if iset='p'}
!!table

See also: !!ref{crop.html}{crop}, !!ref{bcrop.html}{bcrop} and !!ref{window.html}{window}

!!end

*/

#include <cstdlib>
#include <cfloat>
#include <string>
#include <map>
#include <fstream>
#include "cpgplot.h"
#include "trm/subs.h"
#include "trm/input.h"
#include "trm/plot.h"
#include "trm/frame.h"
#include "trm/mccd.h"
#include "trm/window.h"
#include "trm/ultracam.h"

int main(int argc, char* argv[]){

  using Ultracam::Input_Error;
  using Ultracam::Ultracam_Error;

  try{

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Define inputs
    input.sign_in("data",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("newfile",  Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("window",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("numccd",   Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("ncset",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("xbin",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("ybin",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("nxtot",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("nytot",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("device",   Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("xleft",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("xright",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ylow",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("yhigh",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("iset",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ilow",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ihigh",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("plow",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("phigh",    Subs::Input::GLOBAL, Subs::Input::PROMPT);

    // Get inputs
    std::string filename;
    input.get_value("data", filename, "input", "data file (NONE to skip)");
    Ultracam::Frame data;
    bool plotdata;
    if((plotdata = (filename != "NONE"))) data.read(filename);

    bool newfile;
    input.get_value("newfile", newfile, true, "do you want to open a new window file?");
    std::string window;
    input.get_value("window", window, "window", "window file");

    Ultracam::Mwindow win;
    if(newfile && plotdata){
    win = Ultracam::Mwindow(data.size());

    }else if(newfile){
    int numccd;
    input.get_value("numccd", numccd, int(3), int(1), int(10), "number of CCDs");
    win = Ultracam::Mwindow(numccd);

    }else{

    win.rasc(window);
    if(plotdata){
        if(data.size() != win.size())
        throw Ultracam::Ultracam_Error("incompatible numbers of CCDs in window and data files");

        // Run a few checks
        for(size_t nccd=0; nccd<data.size(); nccd++){
        if(data[nccd].size() && win[nccd].size()){
            if(data[nccd][0].xbin() != win[nccd][0].xbin())
            throw Ultracam::Ultracam_Error("X binning factors of CCD " + Subs::str(nccd+1) + " in the data and window files are not equal.");
            if(data[nccd][0].ybin() != win[nccd][0].ybin())
            throw Ultracam::Ultracam_Error("Y binning factors of CCD " + Subs::str(nccd+1) + " in the data and window files are not equal.");
            if(data[nccd][0].nxtot() != win[nccd][0].nxtot())
            throw Ultracam::Ultracam_Error("Total X pixels of CCD " + Subs::str(nccd+1) + " in the data and window files are not equal.");
            if(data[nccd][0].nytot() != win[nccd][0].nytot())
            throw Ultracam::Ultracam_Error("Total Y pixels of CCD " + Subs::str(nccd+1) + " in the data and window files are not equal.");
        }
        }
    }

    std::cout << "Loaded window format:\n\n" << win << std::endl;

    }

    int nccd = 1;
    input.get_value("ncset", nccd, int(1), int(1), int(win.size()), "number of CCD to set");
    nccd--;
    if(plotdata && data[nccd].size() == 0)
    throw Ultracam::Ultracam_Error("CCD " + Subs::str(nccd+1) + " has no data");

    // All windows for a given CCD must have the same binning
    // factors and total dimensions.
    int nxt, nyt;
    int xbin, ybin;
    if(win[nccd].size()){
      nxt  = win[nccd][0].nxtot();
      nyt  = win[nccd][0].nytot();
      xbin = win[nccd][0].xbin();
      ybin = win[nccd][0].ybin();
    }else if(plotdata){
      nxt  = data[nccd][0].nxtot();
      nyt  = data[nccd][0].nytot();
      xbin = data[nccd][0].xbin();
      ybin = data[nccd][0].ybin();
    }else{
      input.get_value("xbin", xbin, int(1),  int(1), Ultracam::Window::MAX_XBIN, "binning factor in X");
      input.get_value("ybin", ybin, int(1),  int(1), Ultracam::Window::MAX_YBIN, "binning factor in Y");
      input.get_value("nxtot", nxt, int(1024), int(1), Ultracam::Window::MAX_NXTOT, "total unbinned X dimension");
      input.get_value("nytot", nyt, int(1024), int(1), Ultracam::Window::MAX_NYTOT, "total unbinned Y dimension");
    }

    int llx, lly, nx, ny;

    if(plotdata){
      std::string device;
      input.get_value("device", device, "/xs", "plot device");

      float x1, x2, y1, y2;
      x2 = data[nccd].nxtot()+0.5;
      y2 = data[nccd].nytot()+0.5;
      input.get_value("xleft",  x1, 0.5f, 0.5f, x2, "left X limit of plot");
      input.get_value("xright", x2, x2,   0.5f, x2, "right X limit of plot");
      input.get_value("ylow",   y1, 0.5f, 0.5f, y2, "lower Y limit of plot");
      input.get_value("yhigh",  y2, 0.5f, 0.5f, y2, "upper Y limit of plot");
      char iset;
      input.get_value("iset", iset, 'a', "aAdDpP",
              "set intensity a(utomatically), d(irectly) or with p(ercentiles)?");
      iset = toupper(iset);
      float ilow, ihigh, plow, phigh;
      if(iset == 'D'){
    input.get_value("ilow",   ilow,  0.f, -FLT_MAX, FLT_MAX, "lower intensity limit");
    input.get_value("ihigh",  ihigh, 1000.f, -FLT_MAX, FLT_MAX, "upper intensity limit");
      }else if(iset == 'P'){
    input.get_value("plow",   plow,  1.f, 0.f, 100.f,  "lower intensity limit percentile");
    input.get_value("phigh",  phigh, 99.f, 0.f, 100.f, "upper intensity limit percentile");
    plow  /= 100.;
    phigh /= 100.;
      }

      Subs::Plot plot(device);
      cpgsch(1.5);
      cpgscf(2);
      Ultracam::plot_images(data,x1,x2,y1,y2,false,'X',iset,ilow,ihigh,plow,phigh,true,filename,nccd,false);
      cpgsci(Subs::WHITE);
      float x = (x1+x2)/2., y = (y1+y2)/2.;
      int xc1, yc1, xc2, yc2;
      char ret, reply = 'X';
      pgline(win[nccd]);

      std::cout << "Position cursor at a window corner (to add a window) or\n"
       << "inside a window to delete and hit the appropriate letter.\n"
       << std::endl;

      while(reply != 'Q'){
    reply = 'X';
    std::cout << "A(dd), R(emove), Q(uit)" << std::endl;
    if(!cpgcurs(&x,&y,&reply)) throw "Cursor error";
    reply = toupper(reply);

    if(reply == 'A'){

      xc1 = std::min(nxt,std::max(int(1),int(floor(x+0.5))));
      yc1 = std::min(nyt,std::max(int(1),int(floor(y+0.5))));

      std::cout << "Set other corner (any key except 'Q' to quit without setting the window)" << std::endl;
      x = float(xc1);
      y = float(yc1);

      if(cpgband(2,1,xc1,yc1,&x,&y,&ret) == 0) throw Ultracam_Error("Cursor error");

      ret = toupper(ret);
      if(ret != 'Q'){

        // Ensure within range

        xc2 = std::min(nxt,std::max(int(1),int(floor(x+0.5))));
        yc2 = std::min(nyt,std::max(int(1),int(floor(y+0.5))));

        llx = std::min(xc1, xc2);
        lly = std::min(yc1, yc2);
        nx  = int(floor(float(std::max(xc1, xc2) - llx + 1)/xbin+0.5));
        ny  = int(floor(float(std::max(yc1, yc2) - lly + 1)/ybin+0.5));

        const Ultracam::Window twin = Ultracam::Window(llx,lly,nx,ny,xbin,ybin,nxt,nyt);

        // Check against all current data windows
        int nfailx = 0, nfaily = 0;
        for(size_t nwin=0; nwin<data[nccd].size(); nwin++){
        const Ultracam::Window& dwin = data[nccd][nwin];
        if(Ultracam::clash(twin, dwin)){
            if((llx - dwin.llx()) % xbin != 0){
            llx -= (llx-dwin.llx()) % xbin;
            nfailx++;
            std::cerr << "Re-jigged X position of window so that it is in step with data window " << nwin+1 << std::endl;
            }
            if((lly - dwin.lly()) % ybin != 0){
            lly -= (lly-dwin.lly()) % ybin;
            nfaily++;
            std::cerr << "Re-jigged Y position of window so that it is in step with data window " << nwin+1 << std::endl;
            }
        }
        }
        if(nfailx > 1 || nfaily > 1){
        std::cerr << "Selected window is out of step with more than one data window in a way that cannot be corrected; choose again" << std::endl;
        }else{
        try{
            win[nccd].push_back(Ultracam::Window(llx,lly,nx,ny,xbin,ybin,nxt,nyt));
            pgline(win[nccd][win[nccd].size()-1]);
        }
        catch(std::string str){
            std::cerr << str <<std::endl;
        }
        }
      }
    }else if(reply == 'R'){
      try{
        Ultracam::Window w;
        if(win[nccd].del_obj(x,y,w)){
          cpgsci(Subs::RED);
          pgline(w);
        }
      }
      catch(std::string str){
        std::cerr << str <<std::endl;
      }
    }else if(reply != 'Q'){
      std::cout << "Position = " << x << ", " << y << std::endl;
    }
      }

    }else{

      char reply = 'X';

      while(reply != 'Q'){
    std::cout << "A(dd), R(emove), Q(uit): ";
    std::cin  >> reply;
    reply = toupper(reply);

    if(reply == 'A'){
      std::cout << "Enter lower left corner (x,y, unbinned) and binned window dimensions (separated by spaces, not commas): ";
      std::cin >> llx >> lly >> nx >> ny;
      if(!std::cin){
        std::cerr << "Invalid input" << std::endl;
        reply = 'Q';
        std::cin.clear();
      }
      try{
        win[nccd].push_back(Ultracam::Window(llx,lly,nx,ny,xbin,ybin,nxt,nyt));
      }
      catch(const std::string& str){
        std::cerr << str <<std::endl;
      }
    }
      }
    }

    // Dump out result

    win.wasc(window);

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



