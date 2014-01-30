/*

!!begin
!!title   Start field generation program
!!author  T.R. Marsh
!!created 14 February 2002
!!revised 09 May 2006
!!root    addfield
!!index   addfield
!!descr   adds a star field to a data frame
!!css     style.css
!!class   Programs
!!class   Testing
!!head1   addfield for adding a star field

!!emph{addfield} adds a star field to a data frame generated with
!!ref{setfield.html}{setfield} and generates one or more images. It is
possible to define a linear drift plus random jitter plus corrections
every few frames to add to the positions for simulation of typical
tracking. The offsets used are reported along with the frame name.
See also !!ref{noise.html}{noise}.

!!head2 Invocation

addfield field data over [seed xd yd xrms yrms seeing1 seeing2 nreset nvary avary tvary pvary]!!break

!!head2 Command line arguments

!!table

!!arg{field}{Name of the star field.}

!!arg{data}{data file or list of data files to add star field to.}

!!arg{over}{Oversampling factor (in terms of unbinned pixels). i.e. the stellar
profile is computed on a grid spaced by 1/over pixels. 'over' must be > 0. This keeps better
photometry especially when the profile is narrow, but takes longer to compute of course.}

!!arg{seed}{Random number seed. This and succeeding arguments is only prompted for
there is more than 1 image}

!!arg{xdrift, ydrift}{Drift per image in x and y.}

!!arg{xrms, yrms}{RMS scatter in x and y.}

!!arg{seeing1}{Starting seeing, in pixels. It is assumed that the profiles given are without any seeing added. Then a linear
ramp of seeing will be added to them with image number. The seeing is added in quadrature to the semi-major and semi-minor axes
of the ellipses.}

!!arg{seeing2}{Ending seeing, in pixels. It is assumed that the profiles given are without any seeing added.}

!!arg{nreset}{Reset drift every nreset images. Simulates a position
correction at the telescope.}

!!arg{nvary}{Number of aperture to vary sinusoidally in brightness}

!!arg{avary}{Fractional semi-amplitude of variation to impose}

!!arg{tvary}{t0 of variation, marking peak of brightness.}

!!arg{pvary}{Period of variation, in days}

!!table

!!end

*/

#include <cfloat>
#include <climits>
#include <cstdlib>
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include "trm/subs.h"
#include "trm/input.h"
#include "trm/frame.h"
#include "trm/mccd.h"
#include "trm/target.h"
#include "trm/ultracam.h"


std::string fname(const std::string& root, int nim, int ndig);

int main(int argc, char* argv[]){

  using Ultracam::Input_Error;
  using Ultracam::Ultracam_Error;

  try{

    // Construct Input object

    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Define inputs

    input.sign_in("field",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("data",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("over",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("seed",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("xdrift",   Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("ydrift",   Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("xrms",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("yrms",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("seeing1",  Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("seeing2",  Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("nreset",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("nvary",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("avary",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("tvary",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("pvary",    Subs::Input::GLOBAL, Subs::Input::PROMPT);

    // Get inputs

    std::string sfield;
    input.get_value("field", sfield, "field", "star field file");
    Ultracam::Mtarget field, mfield(sfield);

    std::string name;
    input.get_value("data", name, "blank", "file or file list to add sky lines to");

    // Read file or list
    std::vector<std::string> flist;
    if(Ultracam::Frame::is_ultracam(name)){
      flist.push_back(name);
    }else{
      std::ifstream istr(name.c_str());
      while(istr >> name){
    flist.push_back(name);
      }
      istr.close();
      if(flist.size() == 0) throw Input_Error("No file names loaded");
    }

    // Read first file for establishing defaults
    Ultracam::Frame frame(flist[0]);

    if(mfield.size() != frame.size())
      throw Ultracam::Input_Error("Conflicting numbers of CCDs in star field and first data file");

    int over;
    input.get_value("over", over, 1, 1, 100, "oversampling factor (subdivisions/unbinned pixel)");

    float xdrift, ydrift, xrms, yrms, seeing1, seeing2;
    Subs::INT4 seed;
    int nreset = 1, nvary = 1;
    double pvary, avary, tvary;
    if(flist.size() > 1){
      input.get_value("seed",   seed,   657687, INT_MIN, INT_MAX, "seed integer for random number generator");
      input.get_value("xdrift", xdrift, 0.f, -100.f, 100.f, "drift in X per image");
      input.get_value("ydrift", ydrift, 0.f, -100.f, 100.f, "drift in Y per image");
      input.get_value("xrms",   xrms,   0.f, 0.f, 100.f, "RMS scatter in X");
      input.get_value("yrms",   yrms,   0.f, 0.f, 100.f, "RMS scatter in Y");
      input.get_value("nreset", nreset, 1, 1, 1000000, "number of images before resetting drift");
      input.get_value("seeing1", seeing1, 0.f, 0.f, 1000.f, "seeing at start of image sequence");
      input.get_value("seeing2", seeing2, 0.f, 0.f, 1000.f, "seeing at end of image sequence");
      input.get_value("nvary",   nvary, 1, 1, 50, "aperture number to vary");
      nvary--;
      input.get_value("avary",   avary, 0.05, 0., 1., "fractional semi-amplitude of variation");
      input.get_value("pvary",   pvary, 0.02, 0., 1000., "period of variation (days)");
      input.get_value("tvary",   tvary, 55000., DBL_MIN, DBL_MAX, "time of maximum brightness (MJD, days)");

    }

    const float CLEVEL = 0.01;

    float xoff, yoff, dx, dy, dxs, dys;
    float xlo, xhi, ylo, yhi, seeing;
    int pxlo, pxhi, pylo, pyhi, nxs, nys;
    Ultracam::Target star;
    double sum;
    field = mfield;

    for(size_t im=0; im<flist.size(); im++){

      // Read data
      Ultracam::Frame data(flist[im]);

      xoff = xdrift*(im % nreset);
      yoff = ydrift*(im % nreset);

      if(flist.size() > 1){

    xoff  += xrms*Subs::gauss2(seed);
    yoff  += yrms*Subs::gauss2(seed);

    // blurr the star field and modulate whichever star was chosen to vary
    seeing = seeing1 + (seeing2-seeing1)*im/(flist.size()-1);
    field  = mfield;
    double time = data["UT_date"]->get_double();
    for(size_t nc=0; nc<field.size(); nc++){ // CCDs
      for(size_t ns=0; ns<field[nc].size(); ns++){ // stars
        field[nc][ns].blurr(seeing);
        if(int(ns) == nvary)
          field[nc][ns].set_counts(field[nc][ns].get_counts()*(1.+avary*cos(Constants::TWOPI*(time-tvary)/pvary)));
      }
    }
      }

      // Now add stars
      for(size_t nc=0; nc<data.size(); nc++){ // CCDs

    for(size_t nw=0; nw<data[nc].size(); nw++){ // windows

      Ultracam::Windata& win = data[nc][nw];
      nxs = win.xbin()*over;
      nys = win.ybin()*over;

      for(size_t ns=0; ns<field[nc].size(); ns++){ // stars

        // Add x,y offset and determine region over which to add star.
        star = field[nc][ns];
        star.set_xc(star.get_xc()+xoff);
        star.set_yc(star.get_yc()+yoff);
        star.dist(CLEVEL, dx, dy);
        xlo  = star.get_xc()-dx;
        xhi  = star.get_xc()+dx;
        ylo  = star.get_yc()-dy;
        yhi  = star.get_yc()+dy;
        pxlo = int(floor(win.xcomp(xlo)));
        pxhi = int(ceil(win.xcomp(xhi)));
        pylo = int(floor(win.ycomp(ylo)));
        pyhi = int(ceil(win.ycomp(yhi)));

        // truncate to valid pixel range
        pxlo = pxlo > 0 ? pxlo : 0;
        pxhi = pxhi < win.nx() ? pxhi : win.nx() - 1;
        pylo = pylo > 0 ? pylo : 0;
        pyhi = pyhi < win.ny() ? pyhi : win.ny() - 1;

        // now add in star
        for(int iy=pylo; iy<pyhi; iy++){
          dy = win.yccd(iy) - star.get_yc();
          for(int ix=pxlo; ix<pxhi; ix++){
        dx = win.xccd(ix) - star.get_xc();
        sum = 0.;
        for(int iys=0; iys<nys; iys++){
          dys = dy + (iys+0.5)/nys - 0.5;
          for(int ixs=0; ixs<nys; ixs++){
            dxs  = dx + (ixs+0.5)/nxs - 0.5;
            sum += star.height(dxs, dys);
          }
        }
        win[iy][ix] += sum/Subs::sqr(over);
          }
        }
      }
    }
      }

      data.write(flist[im]);
      std::cout << "Written " << flist[im] << " to disk" << std::endl;

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





