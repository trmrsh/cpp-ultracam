/*

!!begin
!!title   Plots a collapsed ultracam file
!!author  T.R. Marsh
!!created 17 November 2006
!!revised 19 November 2006
!!root    lplot
!!index   lplot
!!descr   plots a collapsed Ultracam data frame
!!css     style.css
!!class   Programs
!!class   Spectra
!!class   Display
!!head1   lplot - plots a collapsed Ultracam data file

!!emph{lplot} makes a line plot of collapsed Ultracam data files, i.e. files produced by the program !!ref{collapse.html}{collapse}.
These are frames in which all the windows have been collpased in either X or Y so that they are cross-cuts of a frame.
!!emph{lplot} plots these as bar charts, using colours to distinguish overlapping windows.

!!head2 Invocation

lplot data [device] nccd x1 x2 y1 y2 [width aspect reverse cheight font lwidth] !!break

!!head2 Command line arguments

!!table

!!arg{data}{An Ultracam data file from !!ref{collapse.html}{collapse}. All windows must be 1D or null.}

!!arg{device}{Display device}

!!arg{nccd}{The particular CCD to display}

!!arg{x1 x2 y1 y2}{The range of the plot. Set x1=x2 and/or y1=y2 to get automatic choice based on the
first valid window encountered.}

!!arg{width}{Width of plot in inches. 0 for the default width.}

!!arg{aspect}{aspectd ratio  (y/x) of the plot panel}

!!arg{reverse}{true/false to reverse the measuring of black and white.}

!!arg{cheight}{Character height, as a multiple of the default}

!!arg{font}{Character font, 1-4 PGPLOT fonts}

!!arg{lwidth}{Line width, integer multiple of default}

!!table

!!head2 See also

!!ref{collapse.html}{collapse}, !!ref{expand.html}{expand}, !!ref{ppos.html}{ppos}


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
    input.sign_in("device",  Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
    input.sign_in("nccd",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("x1",      Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("x2",      Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("y1",      Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("y2",      Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("width",   Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
    input.sign_in("aspect",  Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
    input.sign_in("reverse", Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
    input.sign_in("cheight", Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
    input.sign_in("font",    Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
    input.sign_in("lwidth",  Subs::Input::LOCAL,  Subs::Input::NOPROMPT);

    // Get inputs
    std::string name;
    input.get_value("data", name, "run001", "file or file list to plot");

    // Read file
    Ultracam::Frame frame(name);

    std::string device;
    input.get_value("device", device, "/xs", "plot device");

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

    float x1;
    input.get_value("x1", x1, 0.5f, -20.5f, float(frame[nccd].nxtot()+20.5), "left X limit of plot");
    float x2;
    input.get_value("x2", x2, float(frame[nccd].nxtot()), -20.5f, float(frame[nccd].nxtot()+20.5), "right X limit of plot");
    float y1;
    input.get_value("y1",  y1, 0.f,    -FLT_MAX, FLT_MAX,  "lower Y limit of plot");
    float y2;
    input.get_value("y2",  y2, 1000.f, -FLT_MAX, FLT_MAX,  "upper Y limit of plot");

    float width;
    input.get_value("width",  width, 0.f, 0.f, 100.f, "width of plot in inches (0 for default)");
    float aspect;
    if(width == 0.f)
      input.get_value("aspect", aspect, 0.6f, 0.f, 100.f, "aspect ratio of plot (0 for default)");
    else
      input.get_value("aspect", aspect, 0.6f, 1.e-2f, 100.f, "aspect ratio of plot");
    bool reverse;
    input.get_value("reverse", reverse, false, "do you want to reverse black and white?");
    float cheight;
    input.get_value("cheight", cheight, 1.f, 0.f, 100.f, "character height (multiple of default)");
    int font;
    input.get_value("font", font, 1, 1, 4, "character font (1-4)");
    int lwidth;
    input.get_value("lwidth", lwidth, 1, 1, 40, "line width (multiple of default)");

    input.save();

    // Open plot
    Subs::Plot plot(device);
    if(aspect > 0) cpgpap(width, aspect);
    if(reverse){
      cpgscr( 0, 1., 1., 1.);
      cpgscr( 1, 0., 0., 0.);
    } 
    cpgsch(cheight);
    cpgslw(lwidth);
    cpgscf(font);

    bool no_axes = true;

    Subs::Array1D<float> x, y;
    Subs::Array1D<int> cols(frame[nccd].size());
    cols = 1;
    for(size_t nwin=0; nwin<frame[nccd].size(); nwin++){
	const Ultracam::Windata& win = frame[nccd][nwin];

	if(win.is_not_null()){

	    // Make sure overlapping windows have different plot colours 
	    for(size_t nw=0; nw<nwin; nw++){
		const Ultracam::Windata& pwin = frame[nccd][nw];
		if(pwin.is_not_null()){
		    if(cdir == 'X'){
			if((win.bottom() >= pwin.bottom() && win.bottom() <= pwin.top()) ||
			   (win.top() >= pwin.bottom() && win.top() <= pwin.top()) ||
			   (win.bottom() < pwin.bottom() && win.top() > pwin.top()))
			    cols[nwin] = cols[nw] + 1;
		    }else{
			if((win.left() >= pwin.left() && win.left() <= pwin.right()) ||
			   (win.right() >= pwin.left() && win.right() <= pwin.right()) ||
			   (win.left() < pwin.left() && win.right() > pwin.right()))
			    cols[nwin] = cols[nw] + 1;
		    }
		}
	    }

	    if(cdir == 'X'){
		x.resize(win.ny());
		y.resize(win.ny());
		for(int ny=0; ny<win.ny(); ny++){
		    x[ny] = win.yccd(ny);
		    y[ny] = win[ny][0];
		}
	    }else{
		x.resize(win.nx());
		y.resize(win.nx());
		for(int nx=0; nx<win.nx(); nx++){
		    x[nx] = win.xccd(nx);
		    y[nx] = win[0][nx];
		}
	    }

	    // Wait until now to plot the axes so that they can be fixed from the
	    // data 
	    if(no_axes){
		cpgsci(4);
		if(x1 == x2){
		    x1 = x.min();
		    x2 = x.max();
		    float range = x2-x1;
		    x1 -= range/10.;
		    x2 += range/10.;
		    input.set_default("x1", x1);
		    input.set_default("x2", x2);
		}
		if(y1 == y2){
		    y1 = y.min();
		    y2 = y.max();
		    float range = y2-y1;
		    y1 -= range/10.;
		    y2 += range/10.;
		    input.set_default("y1", y1);
		    input.set_default("y2", y2);
		}
		cpgenv(x1, x2, y1, y2, 0, 0);
		cpgsci(2);
		cpglab(cdir == 'X' ? "Y pixels" : "X pixels", "Counts", " ");
		no_axes = false;
	    }

	    cpgsci(cols[nwin]);
	    pgbin(x, y);
		
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


