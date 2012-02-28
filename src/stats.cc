/*

!!begin
!!title   Prints out stats of an Ultracam frame
!!author  T.R. Marsh
!!created 10 May 2002
!!revised 02 Feb 2005
!!root    stats
!!index   stats
!!descr   prints stats of an Ultracam frame
!!css   style.css
!!class   Programs
!!class   Information
!!head1   stats - prints stats of an Ultracam frame

!!emph{stats} prints statistical information for an Ultracam frame,
evaluated over a user-defined region as defined by a windows file. 
 The user must also supply a threshold for rejection in order to carry 
out computation of the mean and RMS after rejection of outliers. 

!!head2 Invocation

stats image window sigma (median)

!!head2 Command line arguments

!!table
!!arg{image}{Ultracam data file}
!!arg{window}{A windows file (e.g. as generated by !!ref{setwin.html}{setwin} defining
the regions to be evaluated. Each CCD is treated separately. NB this is slight overkill; in 
particular the binning factors are irrelevant and are best set = 1. If you enter 'ALL', it will
generate its own windows matching the full frame.}
!!arg{sigma}{Threshold number of sigma for rejecction in evaluating clipped mean and rms}
!!arg{median}{true/false to compute the median as well. Switch it off to speed things
up if you are not interested in it.}
!!table

!!end

*/

#include <cstdlib>
#include <cfloat>
#include <string>
#include <sstream>
#include <map>
#include "trm_subs.h"
#include "trm_format.h"
#include "trm_input.h"
#include "trm_frame.h"
#include "trm_mccd.h"
#include "trm_window.h"
#include "trm_ultracam.h"


int main(int argc, char* argv[]){

  try{

    // Construct Input object

    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Define inputs

    input.sign_in("data",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("window",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("sigma",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("median",   Subs::Input::LOCAL,  Subs::Input::NOPROMPT);

    std::string sdata;
    input.get_value("data", sdata, "run001", "data file");
    Ultracam::Frame data(sdata);
    std::string window;
    input.get_value("window", window, "window", "window file");
    Ultracam::Mwindow mwindow;
    if(window == "ALL"){
	mwindow.resize(data.size());
	for(size_t nccd=0; nccd<data.size(); nccd++)
	    for(size_t nwin=0; nwin<data[nccd].size(); nwin++)
		mwindow[nccd].push_back(data[nccd][nwin]);
    }else{
	mwindow.rasc(window);
	if(data.size() != mwindow.size())
	    throw Ultracam::Input_Error("Data frame and window files have differing numbers of CCDs");
    }
    float sigma;
    input.get_value("sigma", sigma, 3.f, 1.e-10f, FLT_MAX, "sigma reject threshold");
    bool median;
    input.get_value("median", median, true, "do you want to compute the median too?");

    Ultracam::Image::Stats stats;
    Subs::Format form;

    for(size_t nccd=0; nccd<data.size(); nccd++){
      stats = data[nccd].statistics(mwindow[nccd],sigma,median,false);
      std::cout << "\nCCD number " << nccd+1 << ":\n" << std::endl;
      std::cout << "Total number of pixels    = " << stats.npoints << std::endl;
      input.add_to_global("stats_npoints", stats.npoints);
      if(stats.npoints){
	std::cout << "Minimum                   = " << form(stats.min) << std::endl;
	input.add_to_global("stats_min", stats.min);
	std::cout << "Maximum                   = " << form(stats.max) << std::endl;
	input.add_to_global("stats_max", stats.max);
	std::cout << "Raw mean                  = " << form(stats.raw_mean) << std::endl;
	input.add_to_global("stats_raw_mean", stats.raw_mean);
	if(stats.npoints > 1){
	  std::cout << "Raw RMS                   = " << form(stats.raw_rms) << std::endl;
	  input.add_to_global("stats_raw_rms", stats.raw_mean);
	}
	std::cout << "Number of points rejected = " << stats.nrejected << std::endl;
	input.add_to_global("stats_nrejected", stats.nrejected);
	std::cout << "Clipped mean              = " << form(stats.clipped_mean) << std::endl;
	input.add_to_global("stats_clipped_mean", stats.clipped_mean);
	if(stats.npoints > stats.nrejected + 1){
	  std::cout << "Clipped RMS               = " << form(stats.clipped_rms) << std::endl;
	  input.add_to_global("stats_clipped_rms", stats.clipped_rms);
	}
	if(median){
	  std::cout << "Median                    = " << form(stats.median) << std::endl;
	  input.add_to_global("stats_median", stats.median);
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

