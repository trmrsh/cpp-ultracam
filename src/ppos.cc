/*

!!begin
!!title   Measures peak positions
!!author  T.R. Marsh
!!created 19 November 2006
!!revised 02 December 2006
!!root    ppos
!!index   ppos
!!descr   measures peak positions
!!css     style.css
!!class   Programs
!!class   Spectra
!!head1   ppos - measures positions of peaks 

!!emph{ppos} measures the positions of peaks in a collapsed Ultracam data files, i.e. files produced by the program !!ref{collapse.html}{collapse}.
These are frames in which all the windows have been collpased in either X or Y so that they are cross-cuts of a frame.
!!emph{ppos} measures the positions of these using cross-correlation with a gaussian. This will grind through each window.
Peaks will be reported in left-to-right or bottom-to-top order within a given window. When the position of a local maximum is measured 
by cross-correlation is will lock onto the most significant peak within ~FWHM of the starting position. This can mean that multiple local maxima will lead to
the same final position. Therefore the routines has an option to skip peaks with the same position as the previous one if it is not sufficiently far away.

!!head2 Invocation

ppos data nccd fwhm height gain minsep!!break

!!head2 Command line arguments

!!table

!!arg{data}{An Ultracam data file from !!ref{collapse.html}{collapse}. All windows
must be 1D or null.}

!!arg{nccd}{The particular CCD to measure}

!!arg{fwhm}{The full width half-maximum of the gaussian in pixels to use for cross-correlation.
Note that this is measured in !!emph{binned} pixels and must be at least 2}

!!arg{height}{Minimum height above the background (measured from the median) for a peak
to be included.}

!!arg{gain}{Number of photons/ADU to estimate the uncertainties; an uncertainty for the
background will be estimated from the difference between the the 16 and 50th perecentiles}

!!arg{minsep}{Minimum separation of peaks to be reported in terms of a multiple of the FWHM.
This prevents identical peaks being reported.}

!!table

!!head2 See also

!!ref{collapse.html}{collapse}, !!ref{expand.html}{expand}, !!ref{lplot.html}{lplot}

!!end

*/

#include <cstdlib>
#include <cfloat>
#include <string>
#include <map>
#include <vector>
#include "cpgplot.h"
#include "trm_constants.h"
#include "trm_subs.h"
#include "trm_format.h"
#include "trm_array1d.h"
#include "trm_input.h"
#include "trm_plot.h"
#include "trm_frame.h"
#include "trm_aperture.h"
#include "trm_mccd.h"
#include "trm_reduce.h"
#include "trm_ultracam.h"

int main(int argc, char* argv[]){

    using Ultracam::Input_Error;
    using Ultracam::Ultracam_Error;

    try{

	// Construct Input object

	Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

	// sign-in input variables
	input.sign_in("data",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("nccd",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("fwhm",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("height",  Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("gain",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("minsep",  Subs::Input::LOCAL,  Subs::Input::PROMPT);

	// Get inputs
	std::string name;
	input.get_value("data", name, "run001", "file or file list to plot");

	// Read file
	Ultracam::Frame frame(name);

	size_t nccd = 1;
	if(frame.size() > 1)
	    input.get_value("nccd", nccd, size_t(1), size_t(1), frame.size(), "CCD number to plot");
	nccd--;

	if(frame[nccd].size() == 0)
	    throw Ultracam::Ultracam_Error("CCD " + Subs::str(nccd+1) + " has no windows.");

	// Test the windows
	char cdir = 'D';
	for(size_t nwin=0; nwin<frame[nccd].size(); nwin++){
	    const Ultracam::Windata& win = frame[nccd][nwin];
	    if(win.is_oned()){
		if(cdir == 'D'){
		    if(win.nx() > 1)
			cdir = 'Y';
		    else if(win.ny() > 1)
			cdir = 'X';
		}else{
		    if((cdir == 'X' && win.nx() > 1) || (cdir == 'Y' && win.ny() > 1))
			throw Ultracam::Ultracam_Error("Different windows seem to have different collapse directions");
		}
	    }else if(win.is_not_null()){
		throw Ultracam::Ultracam_Error("This is not the result of the program 'collapse'");
	    }
	}
	if(cdir == 'D')
	    throw Ultracam::Ultracam_Error("Failed to find any non-null 1D windows");
    
	float fwhm;
	input.get_value("fwhm", fwhm, 4.f, 2.f, 10000.f, "FWHM for gaussian cross-correlation, unbinned pixels");
	float height;
	input.get_value("height", height, 1000.f, -FLT_MAX, FLT_MAX, "minimum peak height above the background");
	float gain;
	input.get_value("gain", gain, 1.f, FLT_MIN, FLT_MAX, "photons/count for uncertainty estimation");
	float minsep;
	input.get_value("minsep", minsep, 0.5f, 0.f, FLT_MAX, "minimum separation of peaks to report");
	input.save();
    
	Subs::Array1D<float> data, buff, vars;

	for(size_t nwin=0; nwin<frame[nccd].size(); nwin++){

	    const Ultracam::Windata& win = frame[nccd][nwin];
	
	    if(win.is_not_null()){
	    
		// Extract the profile
		if(cdir == 'X'){
		    data.resize(win.ny());
		    for(int ny=0; ny<win.ny(); ny++)
			data[ny] = win[ny][0];
		}else{
		    data.resize(win.nx());
		    for(int nx=0; nx<win.nx(); nx++)
			data[nx] = win[0][nx];
		}
	    
		// copy because order is detroyed by median
		buff = data;

		// Compute the median as an estimate of the background.
		float back       = buff.median();
		float sigma_back = back - buff.select(int(0.16*data.size()));
	    
		vars.resize(data.size());
		for(int ix=0; ix<data.size(); ix++)
		    vars[ix] = Subs::sqr(sigma_back) + std::max(data[ix]-back,0.f)/gain;
	    
		// Now go through looking for peaks
		int npeak = 0;
		double pos;
		float epos, lpos = 0.f;
		for(int ix=1; ix<data.size()-1; ix++){
		    if(data[ix-1] <= data[ix] && data[ix] >= data[ix+1] && data[ix] > back + height){
			Subs::centroid(data, vars, std::max(0, int(ix-10*fwhm)), std::min(data.size()-1, int(ix+10*fwhm)), fwhm, float(ix), true, pos, epos);
			if(npeak == 0 || std::abs(pos-lpos) > minsep*fwhm){
			    npeak++;
			    std::cout << "CCD " << nccd+1 << ", window " << nwin+1 << ", background = " << back 
				      << ", RMS = " << sigma_back << ", peak number " << npeak << ", position = ";
			    if(cdir == 'X'){
				std::cout << win.yccd(pos) << " +/- " << win.ybin()*epos;
			    }else{
				std::cout << win.xccd(pos) << " +/- " << win.xbin()*epos;
			    }
			    std::cout << ", height = " << data[ix] << std::endl;
			    lpos = pos;
			}
		    }
		}		
	    }
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


