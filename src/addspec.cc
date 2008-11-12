/*

!!begin
!!title   Spectrum generation program
!!author  T.R. Marsh
!!created 04 October 2005
!!revised 02 December 2006
!!root    addspec
!!index   addspec
!!descr   adds spectra to a frame
!!css     style.css
!!class   Programs
!!class   Spectra
!!class   Testing
!!head1   addspec for generating a series of spectra

!!emph{addpec} loads a file defining spectra and from this can generate a time series of
spectra. See below for the format of the spectrum definition file.

!!head2 Invocation

addspec spectra data scale xover yover [seed xd yd yrms]!!break

!!head2 Command line arguments

!!table

!!arg{spectra}{Name of the spectrum definition file}

!!arg{data}{Name of file or a file list to add spectra to. The times will be used if there are
moving lines.}

!!arg{scale}{Scale factor to multiply the input strengths by to give an easy way to change the effective
brightness, telescope size etc. 1 implies no change.}

!!arg{xover}{Oversampling factor (in terms of unbinned pixels) in the X direction, i.e. spectral direction.}

!!arg{yover}{Oversampling factor (in terms of unbinned pixels) in the Y direction, i.e. spatial direction.}

!!arg{seed}{Random number seed. This and succedding arguments is only prompted for
if there is more than 1 image.}

!!arg{xdrift, ydrift}{Drift per image in x and y. Note that X is the dispersion direction and therefore represents flexure while 
Y includes guiding as well.}

!!arg{yrms}{RMS scatter in Y}

!!arg{seeing1}{Starting seeing, in pixels. It is assumed that the spectrum profile is gaussian. Then a linear
ramp of seeing will be added to them with image number. The seeing is added in quadrature to the specified width.}

!!arg{seeing2}{Ending seeing, in pixels. It is assumed that the profiles given are without any seeing added.}

!!arg{nreset}{Reset drift every nreset images. Simulates a position
correction at the telescope.}

!!table

!!head2 Spectrum definition file format.

The best way to make a spectrum definition file is to generate one using !!ref{gentemp.html}{gentemp}
and then to edit it directly. This will contain lines to define the position of the spectra such as
<pre>
Position  = 1 500.5 499.5 2 450 2 
</pre>
The parameters define a polynomial as a function of x giving the y position of the spectrum concerned.
The initial '1' indicates a 'normal' polynomial; the other option is not relevant. Then come the middle
and half range of the polynomial, the number of coefficients (2) and their values. So in this case the 
Y position is 450 with a tilt of 2 pixels.

The FWHM of the profile is similarly defined:
<pre>
FWHM      = 1 500.5 499.5 3 3 0 0.5 
</pre>
giving the width of the profile as a function of x position. Similarly the continuum flux:
<pre>
Continuum = 1 500.5 499.5 2 100 20 
</pre>
Finally there are lines that can be added to the continuum of the form
<pre>
Lines     = 2 300 1000 20 50000 0.05 0 550 300 15 50000 0.05 0 
</pre>
This means add 2 gaussian lines each of which is defined by 6 parameters which are the
central X position (pixels), the peak height in counts, the FWHM in pixels, a zero
point for sinusoidal motion ephemeris (MJD), a period in days, and a semi-amplitude in
pixels. 

!!end

*/

#include <cstdlib>
#include <climits>
#include <cfloat>
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include "trm_subs.h"
#include "trm_input.h"
#include "trm_frame.h"
#include "trm_spectrum.h"
#include "trm_mccd.h"
#include "trm_ultracam.h"

int main(int argc, char* argv[]){

  using Ultracam::Input_Error;
  using Ultracam::Ultracam_Error;

  try{

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Define inputs
    input.sign_in("spectra",  Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("data",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("scale",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("xover",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("yover",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("seed",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("xdrift",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ydrift",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("yrms",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("seeing1",  Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("seeing2",  Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("nreset",   Subs::Input::GLOBAL, Subs::Input::PROMPT);

    // Get inputs
    std::string sspectra;
    input.get_value("spectra", sspectra, "spectra", "spectrum definition file");
    Ultracam::Mspectrum mspectrum(sspectra);

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

    if(mspectrum.size() != frame.size())
      throw Ultracam::Input_Error("Conflicting numbers of CCDs in spectrum file and first data file");

    double scale;
    input.get_value("scale", scale, 1., DBL_MIN, DBL_MAX, "intensity scaling factor");

    int xover;
    input.get_value("xover", xover, 1, 1, 100, "oversampling factor in X (subdivisions/unbinned pixel)");

    int yover;
    input.get_value("yover", yover, 1, 1, 100, "oversampling factor in Y (subdivisions/unbinned pixel)");

    float xdrift=0.f, ydrift=0.f, yrms, seeing1, seeing2;
    Subs::INT4 seed;
    int nreset = 1;
    Subs::Time utdate;
    if(flist.size() > 1){
      input.get_value("seed",   seed,   657687, INT_MIN, INT_MAX, "seed integer for random number generator");
      input.get_value("xdrift", xdrift, 0.f, -100.f, 100.f, "drift in X per image");
      input.get_value("ydrift", ydrift, 0.f, -100.f, 100.f, "drift in Y per image");
      input.get_value("yrms",   yrms,   0.f, 0.f, 100.f, "RMS scatter in Y");
      input.get_value("seeing1", seeing1, 0.f, 0.f, 1000.f, "seeing at start of image sequence");
      input.get_value("seeing2", seeing2, 0.f, 0.f, 1000.f, "seeing at end of image sequence");
      input.get_value("nreset", nreset, 1, 1, 1000000, "number of images before resetting drift");
    }

    float xoff, yoff=0.f, seeing=0.f;
    int nxs, nys;

    // Loop over frames
    for(size_t im=0; im<flist.size(); im++){

      // Read data get time
      Ultracam::Frame data(flist[im]);
      Subs::Time time = data["UT_date"]->get_time();

      // Compute the amount of drift
      xoff = xdrift*(im % nreset);
      yoff = ydrift*(im % nreset);

      if(flist.size() > 1){

	// Add jitter
	yoff  += yrms*Subs::gauss2(seed);

	// Compute seeing
	seeing = seeing1 + (seeing2-seeing1)*im/(flist.size()-1);

      }

      // Now add spectra to each window of each CCD

      for(size_t nccd=0; nccd<data.size(); nccd++){ // CCDs

	for(size_t nwin=0; nwin<data[nccd].size(); nwin++){ // windows

	  Ultracam::Windata& win = data[nccd][nwin];
	  nxs = win.xbin()*xover;
	  nys = win.ybin()*yover;

	  for(size_t nspec=0; nspec<mspectrum[nccd].size(); nspec++){ // targets

	    Ultracam::Spectrum& spec = mspectrum[nccd][nspec];

	    // Now add in the star to every pixel
	    for(int ix=0; ix<win.nx(); ix++){

	      // X, Y position of the centre of the pixel relative to the spectrum
	      double x  = win.xccd(ix) + xoff;
	      double y  = spec.get_position(x) + yoff;

	      // Compute total counts for this column and width
	      double total = spec.get_continuum(x);
	      double width = sqrt(Subs::sqr(seeing) + Subs::sqr(spec.get_fwhm(x))); 
	      double sline = 0.;

	      for(int ixs=0; ixs<nxs; ixs++){

		double sx = x + (ixs+0.5)/nxs - 0.5;
		if(flist.size() > 1)
		  sline += spec.get_line(sx, time.mjd());
		else 
		  sline += spec.get_line(sx);
	      }
	      total += sline/nxs;
	      total /= yover;
	      total /= sqrt(Constants::TWOPI*width/Constants::EFAC);
	      total *= scale;

	      for(int iy=0; iy<win.ny(); iy++){
		double dy  = win.yccd(iy) - y;
		double sum = 0.;
		for(int iys=0; iys<nys; iys++){
		  double diff = Subs::sqr((dy + (iys+0.5)/nys - 0.5)/(width/Constants::EFAC))/2;
		  if(diff < 80.)
		    sum += total*exp(-diff);
		}
		win[iy][ix] += sum;
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


