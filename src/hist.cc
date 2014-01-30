/*

!!begin
!!title   Plots histograms of Ultracam data
!!author  T.R. Marsh
!!created 10 May 2002
!!created 03 December 2006
!!root    hist
!!index   hist
!!descr   plots histograms of Ultracam frame
!!css   style.css
!!class   Programs
!!class   Information
!!head1   hist - plots histograms of Ultracam frames

!!emph{hist} plots a histogram of a CCD of a frame or set of frames,
evaluated over a user-defined region defined with a windows file.

!!head2 Invocation

hist [device] [dump] data window nccd nhist x1 x2 (y1 y2)/(output)

!!head2 Command line arguments

!!table
!!arg{device}{Plot device}
!!arg{dump}{Dump to disk file or not, as opposed to plotting.}
!!arg{data}{Ultracam data file or list of data files (ucm only)}
!!arg{window}{A windows file (e.g. as generated by !!ref{setwin.html}{setwin} defining
the regions to be evaluated. Each CCD is treated separately. Enter 'ALL' to select
everything.}
!!arg{nccd}{CCD number}
!!arg{nhist}{Number of points in histogram}
!!arg{normalise}{true to divide all values by the total number, i.e to get  a PDF.}
!!arg{x1}{Lower x limit. This is the left-edge of the first bin.}
!!arg{x2}{Upper x limit. This is the right-edge of the last bin. Enter x1=x2 to get automatic choice. If you
select x2=x1+n*nhist where n is an integer, then the bins will be exactly n in width.}
!!arg{y1}{Lower y limit.}
!!arg{y2}{Upper y limit. Enter y1=y2 to get automatic choice}
!!arg{output}{File to dump to, if not plotting. Will be 3 colums, value, number and sqrt(max(1,number)
as an approximate uncertainty.}
!!table

!!end

*/

#include <cstdlib>
#include <cfloat>
#include <string>
#include <sstream>
#include <map>
#include "cpgplot.h"
#include "trm/subs.h"
#include "trm/array1d.h"
#include "trm/format.h"
#include "trm/input.h"
#include "trm/plot.h"
#include "trm/frame.h"
#include "trm/mccd.h"
#include "trm/ultracam.h"

int main(int argc, char* argv[]){


  try{

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // sign-in input variables
    input.sign_in("device",   Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("dump",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("data",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("window",   Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("nccd",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("nhist",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("normalise",Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("x1",       Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("x2",       Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("y1",       Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("y2",       Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("output",   Subs::Input::LOCAL,  Subs::Input::PROMPT);

    // Get inputs

    std::string device;
    input.get_value("device", device, "/xs", "plot device");

    bool dump;
    input.get_value("dump", dump, false, "do you want to dump to disk rather than plot?");

    std::string name;
    input.get_value("data", name, "run001", "file or file list for histogram");

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
      if(flist.size() == 0) throw Ultracam::Ultracam_Error("No file names loaded");
    }

    // Read first file for establishing defaults
    Ultracam::Frame data(flist[0]);

    std::string swindow;
    input.get_value("window", swindow, "window", "window over which histogram will be computed");
    Ultracam::Mwindow mwindow;
    if(swindow == "ALL"){
    mwindow.resize(data.size());
    for(size_t nccd=0; nccd<data.size(); nccd++)
        for(size_t nwin=0; nwin<data[nccd].size(); nwin++)
        mwindow[nccd].push_back(data[nccd][nwin]);
    }else{
    mwindow.rasc(swindow);
    if(data.size() != mwindow.size())
        throw Ultracam::Input_Error("Data frame and window files have differing numbers of CCDs");
    }

    int nccd;
    input.get_value("nccd", nccd, int(1), int(1), int(data.size()), "CCD number for histogram");
    nccd--;

    size_t nhist;
    input.get_value("nhist", nhist, size_t(100), size_t(1), size_t(10000), "number of bins for the histogram");

    bool normalise;
    input.get_value("normalise", normalise, true, "normalise to get a PDF?");

    float x1, x2, y1, y2;
    input.get_value("x1", x1, 0.f, -FLT_MAX, FLT_MAX, "left X limit for histogram");
    input.get_value("x2", x2, 1000.f, -FLT_MAX, FLT_MAX, "left X limit for histogram");
    if(!dump){
      input.get_value("y1", y1, 0.f, -FLT_MAX, FLT_MAX, "lower Y limit for histogram");
      input.get_value("y2", y2, 1000.f, -FLT_MAX, FLT_MAX, "upper Y limit for histogram");
    }

    std::string dfile;
    if(dump)
      input.get_value("output", dfile, "hist.dat", "output file of histogram");

    // Histogram buffers
    Subs::Array1D<int> hist(nhist);
    Subs::Array1D<float> xval(nhist);
    hist = 0;
    Subs::Array1D<Ultracam::internal_data> buff;

    // Get data for histogram
    bool first = true;
    int ind;
    for(size_t ifile=0; ifile<flist.size(); ifile++){

      // Read data
      Ultracam::Frame frame(flist[ifile]);
      std::cout << "Read file = " <<  flist[ifile] << std::endl;

      frame[nccd].buffer(mwindow[nccd], buff);
      if(!buff.size()) throw Ultracam::Input_Error("No data in overlap region, file = " + flist[ifile]);

      if(first){
    if(x1 == x2){
      x1 = buff.min();
      x2 = buff.max();
      input.set_default("x1", x1);
      input.set_default("x2", x2);
    }
    for(size_t i=0; i<nhist; i++)
      xval[i] = x1 + (x2-x1)*(i+0.5)/nhist;
    first = false;
      }

      // Add into histogram
      for(int i=0; i<buff.size(); i++){
    ind = int(nhist*(buff[i] - x1)/(x2-x1));
    if(ind >= 0 && ind < int(nhist)) hist[ind]++;
      }
    }

    double nfac = normalise ? flist.size()*buff.size() : 1.;

    if(y1 == y2){
      y1 = 0;
      if(normalise)
      y2 = 1.3*hist.max();
      else
      y2 = 1.3*hist.max()/nfac;
      input.set_default("y1", y1);
      input.set_default("y2", y2);
    }

    if(dump){
      std::ofstream ofstr(dfile.c_str());
      Subs::Format form(10);
      for(size_t i=0; i<nhist; i++){
      if(normalise)
          ofstr << form(xval[i]) << " " << form(hist[i]/nfac) << " " << sqrt(float(std::max(1,hist[i])))/nfac << std::endl;
      else
          ofstr << form(xval[i]) << " " << hist[i] << " " << sqrt(float(std::max(1,hist[i]))) << std::endl;
      }

    }else{

      if(x1 == x2) throw Ultracam::Input_Error("Null x range specified");
      if(y1 == y2) throw Ultracam::Input_Error("Null y range specified");

      Subs::Array1D<float> phist(nhist);
      for(size_t i=0; i<nhist; i++)
      phist[i] = hist[i];

      if(normalise) phist /= nfac;

      // Open plot
      Subs::Plot plot(device);
      cpgsci(Subs::BLUE);
      cpgenv(x1,x2,y1,y2,0,0);
      cpgsci(Subs::RED);
      if(normalise)
      cpglab("Data value", "Number of pixels", "Histogram");
      else
      cpglab("Data value", "Probability", "PDF");
      cpgsci(Subs::WHITE);
      cpgbin(nhist, xval, phist, 1);
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

