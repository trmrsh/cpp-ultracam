/*

!!begin
!!title   setreg
!!author  T.R. Marsh
!!created 26 May 2006
!!revised 02 Dec 2006
!!root    setreg
!!index   setreg
!!descr   sets up spectroscopic extraction regions
!!css     style.css
!!class   Programs
!!class   Spectra
!!class   Setup
!!head1   setreg for setting up spectroscopic extraction regions

!!emph{setreg} is an interactive program for defining spectroscopic
extraction regions. It requires a data frame to exist. It assumes that
the dispersion direction is more or less horizontal and allows the user
to define Y ranges to select objects and associated sky. It does so by collapsing in the X
direction and plotting the resulting profiles.

To fully specify an object you need to mark the region over which to extract it and the region which can
be searched for it when re-positioning. Sky regions can only be defined with respect to an object so at
least one object must have been defined. If you define more than one then you must place the cursor near whichever
object you wish to attach a sky region to when adding sky regions.  During extraction, to determine whether a pixel
is in the sky, the sky regions are gone through in turn, so whichever one is defined last has precedence. Thus if
a pixel is include in the sky in the first region defined but excluded in the third, it will be excluded unless there
is a later region which includes it once more. This allows you to define a sloppy all inclusive sky and then to exclude
parts of it that are no good.

!!head2 Invocation

setreg [device] data newfile region nccd xleft xright ylow yhigh iset (ilow ihigh)/(plow phigh)
hwidth fwhm readout gain]!!break

!!head2 Command line arguments

!!table

!!arg{device}{The image display device.}

!!arg{data}{Ultracam data file.}

!!arg{newfile}{flag to indicate that the aperture file is new.}

!!arg{region}{Region file (new or old).}

!!arg{nccd}{The number of the CCD to set the regions for.}

!!arg{xleft xright}{X range to use when collapsing to make profiles}

!!arg{ylow yhigh}{Range in Y of the frame which will be considered}

!!arg{iset}{How to set intendity limits (i.e. the Y axis of the plots): 'a' for automatic, min to max,
'd' for direct input of range, 'p' for percentiles, probably the most useful option.}

!!arg{ilow ihigh}{the intensity range is i1 to i2 if iset='d'}

!!arg{plow phigh}{p1 to p2 are percentiles to set the intensity if iset = 'p'}

!!arg{hwidth}{The half-width of the median filter applied when collapsing to make the mean profile.
0 for no median filter. Measured in binned X pixels.}

!!arg{fwhm}{This is the fwhm of the gaussian to be used to cross-correlate with an object
to measure its position. Units of binned pixels.}

!!arg{readout}{Readout noise, RMS ADU in order for the program to come back with an uncertainty.}

!!arg{gain}{Gain, electrons/ADU, again for uncertainty estimates}

!!table

!!head1 Interactive options

There are several interactive options, all single letters, which are as follows:

!!table

!!arg{O(bject)}{Define a new object region. You will be asked to define the region over which the object is extracted
and then the region over which to search for the object when re-positioning. The latter region must enclose the former.
You will be asked whether you want to measure the object's position. This will be done with gaussian cross-correlation
(see fwhm etc above). It there is no object, you can elect not to attempt to measure a position, but you should realise
that this may affect any later attempt to reposition the extraction apertures using the position of the object since this
is carried out relative to the position measured at this stage. (This is equivalent to the use of 'skymov' for those familiar
with 'pamela'.)
}

!!arg{S(ky)}{Define a new sky region which moves with an object which must be attached to an existing object region. You
must have defined an object first. If you have defined more than one you will have to select the one you wish to associate the sky
region with.}

!!arg{A(ntisky)}{Define a new non-sky region which moves with an object and must be attached to an existing object region.
You must have defined an object first. If you have defined more than one you will have to select the one you wish to associate the sky
region with.}

!!arg{B(ad)}{Define a region of bad sky which does not move with an object but must be attached to an existing object region. This
can be used to mark out the edges of a dekker for example.}

!!arg{Q(uit)}{Quit the program.}

!!table

!!end

*/

#include <cstdlib>
#include <cfloat>
#include <string>
#include <sstream>
#include <map>
#include <fstream>
#include "cpgplot.h"
#include "trm/subs.h"
#include "trm/array1d.h"
#include "trm/plot.h"
#include "trm/input.h"
#include "trm/frame.h"
#include "trm/mccd.h"
#include "trm/specap.h"
#include "trm/ultracam.h"

int main(int argc, char* argv[]){

  void plot_profile(float y1, float y2, float ilow, float ihigh, const std::vector<Subs::Array1D<float> >& ypos, const std::vector<Subs::Array1D<float> >& profile);
  void plot_regions(const Ultracam::CCD<Ultracam::Specap>& region, bool profile);
  int which_win(float x1, float x2, float y1, float y2, const Ultracam::CCD<Ultracam::Windata>& data, int& nwpick);

  using Subs::sqr;
  using Ultracam::Ultracam_Error;
  using Ultracam::Input_Error;

  try{

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Sign-in input variables
    input.sign_in("device",  Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("data",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("newfile", Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("region",  Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("nccd",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("xleft",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("xright",  Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ylow",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("yhigh",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("iset",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ilow",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ihigh",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("plow",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("phigh",   Subs::Input::GLOBAL, Subs::Input::PROMPT);

    // Profile fit settings
    input.sign_in("hwidth",  Subs::Input::LOCAL,   Subs::Input::PROMPT);
    input.sign_in("fwhm",    Subs::Input::GLOBAL,  Subs::Input::NOPROMPT);
    input.sign_in("readout", Subs::Input::GLOBAL,  Subs::Input::NOPROMPT);
    input.sign_in("gain",    Subs::Input::GLOBAL,  Subs::Input::NOPROMPT);

    // Get inputs
    std::string device;
    input.get_value("device", device, "/xs", "plot device");
    std::string name;
    input.get_value("data", name, "run001", "data file to plot");
    Ultracam::Frame data(name), dvar;
    bool newfile;
    input.get_value("newfile", newfile, true, "do you want to open a new region file?");
    std::string regname;
    input.get_value("region", regname, "region", "region file name");

    // Create or open an aperture file
    Ultracam::Mspecap region;
    std::string entry;
    if(newfile){
      region = Ultracam::Mspecap(data.size());

    }else{
      region.rasc(regname);

      if(region.size() != data.size())
    throw Ultracam_Error("Data frame and region file have conflicting CCD numbers");
    }

    int nccd;
    input.get_value("nccd", nccd, int(1), int(1), int(data.size()), "CCD number to set regions for");
    nccd--;

    float x1, x2, y1, y2;
    x2 = data[nccd].nxtot()+0.5;
    y2 = data[nccd].nytot()+0.5;
    input.get_value("xleft",  x1, 0.5f, 0.5f, x2, "left X limit for collapse of profiles");
    input.get_value("xright", x2, x2,   0.5f, x2, "right X limit for collapse of profiles");
    input.get_value("ylow",   y1, 0.5f, 0.5f, y2, "lower Y limit to ");
    input.get_value("yhigh",  y2, 0.5f, 0.5f, y2, "upper Y limit to plot");
    char iset;
    input.get_value("iset", iset, 'a', "aAdDpP", "set intensity a(utomatically), d(irectly) or with p(ercentiles)?");
    iset = toupper(iset);
    float ilow, ihigh, plow, phigh;
    if(iset == 'D'){
      input.get_value("ilow",   ilow,  0.f,    -FLT_MAX, FLT_MAX, "lower intensity limit");
      input.get_value("ihigh",  ihigh, 1000.f, -FLT_MAX, FLT_MAX, "upper intensity limit");
    }else if(iset == 'P'){
      input.get_value("plow",   plow,  1.f,  0.f, 100.f,  "lower intensity limit percentile");
      input.get_value("phigh",  phigh, 99.f, 0.f, 100.f, "upper intensity limit percentile");
      plow  /= 100.;
      phigh /= 100.;
    }

    // Position measurement
    int hwidth;
    input.get_value("hwidth", hwidth,  0, 0, 1000, "half-width of median filter for profile collapse (binned X pixels)");
    float fwhm;
    input.get_value("fwhm",   fwhm,  10.f, 2.f, 1000.f, "FWHM for gaussian position measurement (binned pixels)");
    float readout;
    input.get_value("readout",   readout,  4.f, 0.f, FLT_MAX, "readout noise for profile fits (RMS ADU)");
    float gain;
    input.get_value("gain",   gain,  1.f, 0.01f, 100.f, "electrons/ADU for profile fits");

    // Save defaults now because one often wants to terminate this program early
    input.save();

    dvar = data;
    dvar.max(0);
    dvar /= gain;
    dvar += readout*readout;

    // Collapse to get profile, normalising by the number of pixels
    // in each column; collapse each window individually since we
    // don't know whether they are registered with each other.
    std::vector<Subs::Array1D<float> > profile(data[nccd].size()), pvar(data[nccd].size()), ypos(data[nccd].size());
    std::vector<Subs::Array1D<int> > npix(data[nccd].size());
    Subs::Array1D<float> all;

    for(size_t nwin=0; nwin<data[nccd].size(); nwin++){

      const Ultracam::Windata &win = data[nccd][nwin];
      const Ultracam::Windata &var = dvar[nccd][nwin];

      if(make_profile(win, var, x1, x2, y1, y2, hwidth, profile[nwin], pvar[nwin], npix[nwin])){

    // Add in value into a single array for percentile calculation
    ypos[nwin].resize(profile[nwin].size());
    for(int iy=0; iy<profile[nwin].size(); iy++){
      if(npix[nwin][iy]){
        ypos[nwin][iy] = win.yccd(iy);
        all.push_back(profile[nwin][iy]);
      }
    }
      }else{
    profile[nwin].clear();
      }
    }

    if(all.size() == 0)
      throw Ultracam::Ultracam_Error("No valid pixels found");

    if(iset == 'A'){
      ilow  = all[0];
      ihigh = ilow;
      for(int i=1; i<all.size(); i++){
    ilow  = ilow  > all[i] ? all[i] : ilow;
    ihigh = ihigh < all[i] ? all[i] : ihigh;
      }
    }else if(iset == 'P'){
      ilow  = all.select(int(all.size()*plow+0.5));
      ihigh = all.select(int(all.size()*phigh+0.5));
    }

    Subs::Plot plot(device);
    plot.focus();

    // Plot the profiles
    plot_profile(y1, y2, ilow, ihigh, ypos, profile);

    // Plot the regions
    plot_regions(region[nccd], true);

    float x = (y1+y2)/2., y = (ilow+ihigh)/2.;
    char ret = 'X', reply;
    std::string lab;

    std::cout << "Hit the appropriate letter and you will be prompted for more cursor input.\n" << std::endl;

    // Now aperture addition loop

    while(ret != 'Q'){
      ret = 'X';

      // Next lines define the prompt:
      if(region[nccd].size() == 0){
    std::cout << "Choices: O(bject) or Q(uit)" << std::endl;
      }else{
    std::cout << "Choices: O(bject), S(ky), A(nti-sky), B(ad) or Q(uit)" << std::endl;
      }

      // Now get cursor input.
      if(!cpgcurs(&x,&y,&ret)) throw Ultracam_Error("Cursor error");
      ret = toupper(ret);

      if(ret == 'O'){

    cpgsci(3);
    std::cout << "\nMark the edges of the object extraction region using the cursor, Q to quit" << std::endl;
    std::cout << "\nThe first edge ..." << std::endl;

    float xs=x, ys=y;
    if(!cpgcurs(&xs,&ys,&reply)) throw Ultracam_Error("Cursor error");
    if(std::toupper(reply) == 'Q'){
      std::cerr << "Object selection aborted" << std::endl;
      continue;
    }

    std::cout << "... now the second" << std::endl;
    float xe=xs, ye=ys;
    if(cpgband(1,1,xe,ye,&xs,&ys,&reply)){

      if(std::toupper(reply) == 'Q'){
        std::cerr << "Object definition aborted" << std::endl;
        continue;
      }

      int nwpick;
      if(which_win(x1, x2, xs, xe, data[nccd], nwpick) != 1){
        std::cerr << "Object definition aborted" << std::endl;
        continue;
      }

      double ylow  = xe > xs ? xs : xe;
      double yhigh = xe > xs ? xe : xs;

      // draw for reference when marking search region
      cpgmove(ylow, ilow);
      cpgdraw(ylow, ihigh);
      cpgmove(yhigh, ilow);
      cpgdraw(yhigh, ihigh);
      cpgmove(ylow, (ilow+ihigh)/2.);
      cpgmove(yhigh, (ilow+ihigh)/2.);

      std::cout << "\nMark the limits of the region over which to search for the object when re-positioning during extraction, Q to quit" << std::endl;
      std::cout << "These limits must enclose the extraction region." << std::endl;
      std::cout << "\nThe first limit ..." << std::endl;

      xs=x; ys=y;
      if(!cpgcurs(&xs,&ys,&reply)) throw Ultracam_Error("Cursor error");
      if(std::toupper(reply) == 'Q'){
        std::cerr << "Object definition aborted" << std::endl;
        continue;
      }

      std::cout << "... now the second" << std::endl;
      xe=xs; ye=ys;
      if(cpgband(1,1,xe,ye,&xs,&ys,&reply)){

        if(std::toupper(reply) == 'Q'){
          std::cerr << "Object definition aborted" << std::endl;
          continue;
        }

        int nwpick2;
        if(which_win(x1, x2, xs, xe, data[nccd], nwpick2) != 1){
          std::cerr << "Object definition aborted" << std::endl;
          continue;
        }
        if(nwpick != nwpick2){
          std::cerr << "\nThe search limits and object extraction limits are in two different windows" << std::endl;
          std::cerr << "Object definition aborted" << std::endl;
          continue;
        }

        double yslow  = xe > xs ? xs : xe;
        double yshigh = xe > xs ? xe : xs;

        double yp;
        float epos;
        bool pos_is_accurate = false;

        // Try to measure position
        try{
          bool again = true;
          while(again){
          std::cout << "Measure centroid of object, 'y' or 'n'?" << std::endl;
          if(!cpgcurs(&xs,&ys,&reply)) throw Ultracam_Error("Cursor error");
          if(reply == ' ' || reply == 'y' || reply == 'Y'){
              float start = data[nccd][nwpick].ycomp((ylow+yhigh)/2.);
              Subs::centroid(profile[nwpick].ptr(), pvar[nwpick].ptr(), 0, profile[nwpick].size()-1, fwhm, start, true, yp, epos);
              yp = data[nccd][nwpick].yccd(yp);
              if(yp < ylow || yp > yhigh)
              throw Ultracam::Ultracam_Error("Measured position = " + Subs::str(yp) + " is outside extraction region " + Subs::str(ylow) + " to " + Subs::str(yhigh));
              again = false;
              pos_is_accurate = true;

          }else if(reply == 'n' || reply == 'N'){
              again = false;
              std::cout << "Will use mid-point of extraction region for target position" << std::endl;
              std::cout << "This is likely to make extraction region repositioning during extraction unreliable." << std::endl;
              yp = (ylow+yhigh)/2.;
          }else{
              std::cout << "Reply = '" << reply << "' is not valid. Valid responses are 'y', 'Y', 'n' and 'N' only" << std::endl;
          }
          }
        }
        catch(const Subs::Subs_Error& err){
          std::cerr << "Failed to measure accurate position of the target; will use mid-point of extraction region instead" << std::endl;
          std::cerr << "This is likely to make extraction region repositioning during extraction unreliable." << std::endl;
          yp = (ylow+yhigh)/2.;
        }
        catch(const Ultracam::Ultracam_Error& err){
          std::cerr << err << "; will use mid-point of extraction region instead" << std::endl;
          std::cerr << "This is likely to make extraction region repositioning during extraction unreliable." << std::endl;
          yp = (ylow+yhigh)/2.;
        }

        try {
          Ultracam::Specap new_specap(yslow, ylow, yp, yhigh, yshigh, pos_is_accurate, x1, x2);

          // Finally add it in the new Specap
          region[nccd].push_back(new_specap);

          std::cout << "New object added " << std::endl;

          // Re-plot
          plot_profile(y1, y2, ilow, ihigh, ypos, profile);
          plot_regions(region[nccd], true);

        }
        catch(const Ultracam_Error& err){
          std::cerr << err << std::endl;
          std::cerr << "Object definition aborted" << std::endl;
          continue;
        }

      }else{
        std::cerr << "Cursor error" << std::endl;
      }
    }else{
      std::cerr << "Cursor error" << std::endl;
    }

      }else if(region[nccd].size() && (ret == 'S' || ret == 'A' || ret == 'B')){

    std::string sky;
    if(ret == 'S'){
      sky = "sky";
      cpgsci(5);
    }else if(ret == 'A'){
      sky = "anti-sky";
      cpgsci(7);
    }else if(ret == 'B'){
      sky = "bad sky";
      cpgsci(2);
    }

    // First work out which object to associate the new sky region with.
    Ultracam::CCD<Ultracam::Specap>::optr object;

    // reversal of x and y in the next line is deliberate
    if(!region[nccd].selected(y,x,object)){
        std::cerr << "Sorry, no object selected" << std::endl;
        std::cerr << "When using 'S', 'A' or 'B' you must position the cursor on an object." << std::endl;
        continue;
    }

    std::cout << "Use the cursor to mark the extent of the " << sky << " region, hit any key to register the positions, Q to quit" << std::endl;
    std::cout << "\nMark the first boundary" << std::endl;

    float xs=x, ys=y;
    if(!cpgcurs(&xs,&ys,&reply)) throw Ultracam_Error("Cursor error");
    if(std::toupper(reply) == 'Q') continue;

    std::cout << "Now the second" << std::endl;
    float xe=xs, ye=ys;
    if(cpgband(1,1,xe,ye,&xs,&ys,&reply)){

      if(std::toupper(reply) == 'Q') continue;

      double ylow  = xe > xs ? xs : xe;
      double yhigh = xe > xs ? xe : xs;

      object->push_back(Ultracam::Specap::Skyreg(ylow, yhigh, ret == 'S', ret == 'B'));

      // Re-plot
      plot_profile(y1, y2, ilow, ihigh, ypos, profile);
      plot_regions(region[nccd], true);

    }else{
      std::cerr << "Cursor error" << std::endl;
    }

      }else if(ret != 'Q'){
    std::cerr << "Input = " << ret << " not recognised." << std::endl;
      }

    }

    // Dump the result
    region.wasc(regname);

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


void plot_profile(float y1, float y2, float ilow, float ihigh, const std::vector<Subs::Array1D<float> >& ypos, const std::vector<Subs::Array1D<float> >& profile){

  cpgeras();
  cpgsls(1);
  cpgsch(1.5);
  cpgscf(2);
  cpgsci(4);
  cpgvstd();
  cpgswin(y1, y2, ilow, ihigh);
  cpgbox("BCNST", 0, 0, "BCNST", 0, 0);
  cpgsci(2);
  cpglab("Y position", "Counts/pixel", " ");
  cpgsci(1);

  for(size_t nwin=0; nwin<profile.size(); nwin++)
    pgbin(ypos[nwin], profile[nwin]);

}

// selects which window, if any, is uniquely defined by the range y1, y2
int which_win(float x1, float x2, float y1, float y2, const Ultracam::CCD<Ultracam::Windata>& data, int& nwpick){

  // Check that only one window is implied
  int ninside = 0;
  nwpick = 0;
  for(size_t nwin=0; nwin<data.size(); nwin++){
    const Ultracam::Windata &win = data[nwin];
    if(win.bottom() <= y1 && win.top() >= y1 && win.bottom() <= y2 && win.top() >= y2 && win.left() < x2 && win.right() > x1){
      ninside++;
      nwpick = nwin;
    }
  }
  if(ninside == 0)
    std::cerr << "Range specified in not associated with the Y span of any window" << std::endl;

  if(ninside > 1)
    std::cerr << "Range specified is associated with more than one window" << std::endl;

  return ninside;
}

void plot_regions(const Ultracam::CCD<Ultracam::Specap>& region, bool profile) {
  for(size_t i=0; i<region.size(); i++)
    pgline(region[i],profile);
}
