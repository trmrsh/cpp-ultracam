/*

!!begin
!!title   Adds noise to Ultracam frames
!!author  T.R. Marsh
!!created 15 May 2001
!!revised 03 November 2005
!!descr   Adds noise to an Ultracam frame
!!css     style.css
!!root    noise
!!index   noise
!!class   Programs
!!class   Testing
!!head1   noise - adds noise to ultracam data

!!emph{noise} takes an Ultracam file or a list of Ultracam files, then adds
noise to simulate a CCD, including now an L3 CCD option. It treats the input
frames as if they are measured in electrons. It now has quite a bit of extra
stuff to cope with the rather tricky statistics of L3 CCDs. The effect of the
avalanche register in these devices is calculated by first computing cumulative distribution
functions (CDFs) using FFTs for a series of input electron number from 0 to a
user-defined maximum (see nimax below). The 0 electron input includes
clock-induced charges (CICs) generated within the avalanche register while the
non-zero values do not. These are then used to generate output numbers using
searches through the ordered CDF lists.

!!head2 Invocation

noise file bias ncosmic beta elow ehigh full adcmax type [nstage pmult pave ppar npar pser nimax nmax ngauss] seed n*[read gain]!!break

!!head2 Arguments

!!table
!!arg{file}{Either a file name or a list of files. If noise cannot open it as a standard
ultracam file, it will assume that it is a list of files. The values in the file or files are
assumed to represent mean electron numbers and must be positive. Poisson stats will be
used.}

!!arg{bias}{A bias frame to add to the result (without incurring extra noise)}

!!arg{ncosmic}{Mean number of cosmic rays/CCD (normalised to total area)}

!!arg{beta}{Power law exponent (flux ~ E^-beta where E is the number of electrons
from elow to ehigh}

!!arg{elow}{Lower cut-off of cosmic rays, > 0.}

!!arg{ehigh}{Upper cut-off of cosmic rays, > elow}

!!arg{full}{Full well capacity in electrons. Any value above this will be truncated to it. In the case
of L3 readouts, this is the full well of the avalanche part.}

!!arg{adcmax}{Maximum value set by the ADC, e.g. 65535 for a 16 bit ADC.}

!!arg{type}{Type of readout. Either 'normal' for standard CCDs or 'L3' for avalanche readout. In the latter
case a further 9 parameters are needed as described next.}

!!arg{nstage}{If type=L3, then you need to specify the number of mulitplication stages in the avalanche
register. This will be a fixed number that is a function of the chip only}

!!arg{pmult}{If type=L3, then you need to specify the probability of multiplication per stage of the avalanche
register. This leads to a mean gain of (1+pmult)^nstage. If you specify this as a negative number < -1, it will
be assumed to be the negative of the desired avalanche gain and the equivalent pmult will be computed.}

!!arg{pave}{If type=L3, then you need to specify the probability of clock-induced charges per stage of the
avalanche register. Such charges do not go through the complete set of avalanche multiplication steps and thus
lead to a distinctly different output distribution from any charges generated before the avalanche steps.}

!!arg{ppar}{If type=L3, then you need to specify the probability of clock-induced charges per shift in
the parallel register of the CCD. These should show up as a gradient in the parallel direction, rising
as one gets further away from the readout, which I assume to lie in the bottom-right corner of the chip.
The parallel direction is taken to lie along the Y direction, so if you set this to a large value you should
see the number at large Y values increase.}

!!arg{npar}{If type=L3, this is an extra number of parallel shifts to represent
a frame transfer device. This should of the same order of magnitude as the Y dimension of the chip
and simply increases the probability of CICs due to parallel shifts.}

!!arg{pser}{If type=L3, then you need to specify the probability of clock-induced charges per shift in
the serial register of the CCD *prior* to the avalanche register. Again this adds a gradient but now in
the serial (X) direction. The gradient should increase towards small X. I am not quite sure whether I should
have this parameter, but it is here for generality and can always to set = 0 if not thought to be significant.
These 'normal' CICs will be added as follows. The mean number expected just before the avalanche stage
will be calculated as ppar*(npar + y) + pser*(NXTOT-x) and a number of electrons generated following a Poisson
distribution with the same mean.}

!!arg{nimax}{If type=L3, the events at low electron input numbers will be computed by lookup in CDFs computed
at the start of the computation. 'nimax' is the number of CDFs including 0 to compute. Thus if you chose nimax=10,
you would get CDFs for n=0 to 9. For n=0 this is just avalanche register CICs only, while for n=1 to 9, they
are without CICs. Then when generating an event representatitve of an input of 6 electrons say, lookups of the
n=0 and n=6 cases will be added. This parameter is essentially a matter of storage capacity and 'page faults'.
The maximum input of nimax-1 will lead to a mean gain of (nimax-1)*g where g = (1+pmult)^nstage, with
an RMS spread of sqrt((nimax-1)*(1-pmult)/(1+pmult)*g(g-1)) which can be used to determine the value of
the previous parameter nmax. e.g. if pmult=0.015 and nstage=591 so g=6630, then if nimax=20 one would
expect a mean gain for the maximum number = 126,000 with an RMS of 28,500, and so one might want nmax ~ 200,000.
However, if one knew that the full-well capacity or the 16-bit ADU would limit this to a lower number, then you
might as well set the limit at this stage to save time. The events are generated such that any exceeding 'nmax' will actually
saturate at this number. Total memory implied by nimax and nmax (see next) is between 8*(6+nimax)*nmax and 8*(12+nimax)*nmax bytes.
You get near the lower limit if you use a value of nmax just below an exact power of 2 such as 65000.}

!!arg{nmax}{If type=L3, then you need to specify the maximum gain to compute up to. The absolute maximum
is 2^nstage but this is normally a huge number and far higher than typical gains in practice. nmax is essentially
a computational parameter. The larger it is, the slower things will run.}

!!arg{ngauss}{If type=L3, then the lookup table method is limited in its maximum number to nimax-1. Values above
nimax-1 can be generated by multiple lookup. e.g. if nimax=10 and we have an input of 25 electrons, we can add
events drawn from the n=0, 9, 9 & 7 CDFs. However, for very large inputs even this will become slow and one has
to make do with a gaussian of the the expected mean and variance of the true distribution. 'ngauss' is the number at which
to start doing this which must be >= nimax.}

!!arg{seed}{Random number seed integer.}

!!arg{read, gain}{RMS readout (electrons), gain on readout (electron/ADU).
Can be repeated many times and then will be applied successively
to different windows (continuing on to the next CCD etc). Must
be set at last once. If there are more windows than pairs of
values, then the last pair are used for the extra windows.}

!!table

!!head1 Related routines

To create the input files for this routine see also
!!ref{addfield.html}{addfield}, !!ref{addsky.html}{addsky}, !!ref{addspectrum.html}{addspectrum} etc.

!!end

*/

#include <climits>
#include <cstdlib>
#include <cfloat>
#include <string>
#include <map>
#include <fstream>
#include "trm/subs.h"
#include "trm/array1d.h"
#include "trm/format.h"
#include "trm/input.h"
#include "trm/frame.h"
#include "trm/ultracam.h"

int main(int argc, char* argv[]){

  using Ultracam::Input_Error;
  using Ultracam::Ultracam_Error;

  try{

    // Construct Input object

    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Sign-in input variables
    input.sign_in("file",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("bias",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("ncosmic", Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("beta",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("elow",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("ehigh",   Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("full",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("adcmax",  Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("type",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("nstage",  Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("pmult",   Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("pave",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("ppar",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("npar",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("pser",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("nimax",   Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("nmax",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("ngauss",  Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("seed",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("read",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("gain",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("more",    Subs::Input::LOCAL,  Subs::Input::PROMPT);

    // Get inputs

    std::string name;
    input.get_value("file", name, "run001", "file or file list to add noise to");

    Ultracam::Frame frame;

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

    std::string sbias;
    input.get_value("bias", sbias, "bias", "bias frame to add");
    Ultracam::Frame bias(sbias);
    int ncosmic;
    input.get_value("ncosmic", ncosmic, 100, 0, INT_MAX, "number of cosmic rays to add");
    float beta;
    input.get_value("beta", beta, 1.f, -20.f, 20.f, "exponent of CR power-law");
    float elow;
    input.get_value("elow", elow, 10.f, 1.e-5f, FLT_MAX, "lowest cosmic ray value");
    float ehigh;
    input.get_value("ehigh", ehigh, std::max(elow, 1000.f), elow, FLT_MAX, "highest cosmic ray value");
    float a=0.f, b=0.f, c=0.f;
    if(beta == 1.f){
      c = ehigh/elow;
    }else{
      b = pow(elow,1.f-beta);
      a = pow(ehigh,1.f-beta)-b;
    }
    int full;
    input.get_value("full", full, 200000, 0, INT_MAX, "full well capacity, electrons");
    int adcmax;
    input.get_value("adcmax", adcmax, 65535, 0, INT_MAX, "maximum set by the ADC");
    std::string type;

    input.get_value("type", type, "normal", "type of readout ['normal' or 'L3']");
    type = Subs::toupper(type);
    if(type != "NORMAL" && type != "L3")
      throw Ultracam_Error("type must be either = 'normal' or 'L3'");

    int nstage, npar, nmax, nimax, ngauss;
    double pmult, pave, ppar, pser;
    if(type == "L3"){
      input.get_value("nstage", nstage, 591, 1, 10000,   "number of avalanche multiplication steps");
      input.get_value("pmult",  pmult,  0.015, 0., 1.,   "multiplication probability per electron per step");
      double mean = pow(1.+pmult,nstage);
      Subs::Format form;
      std::cout << "Mean gain  = " << form(mean) << std::endl;
      input.get_value("pave",   pave,   0.001, 0., 1.,   "in avalanche CIC probability per step");
      input.get_value("ppar",   ppar,   0.001, 0., 1.,   "parallel shift CIC probability per step");
      input.get_value("npar",   npar,  1024, 0, 10000,   "number of extra parallel shifts to simulate frame transfer");
      input.get_value("pser",   pser,   0.001, 0., 1.,   "serial shift CIC probability per step");
      input.get_value("nimax", nimax, 20, 1, 1000,       "maximum number of CDFs (1 more than number of input electrons)");
      double var = (1.-pmult)/(1.+pmult)*mean*(mean-1.);
      double suggest = (nimax-1)*mean + 5.*sqrt((nimax-1)*var);
      std::cout << "5-sigma above mean for max input of (nimax-1) electrons = " << form(suggest) << std::endl;
      input.get_value("nmax", nmax, 10000, 1, 10000000,  "maximum gain to compute CDFs up to");
      input.get_value("ngauss", ngauss, 100, 10, 100000, "number of electrons at which to start using a gaussian approximation");
    }

    Subs::INT4 seed;
    input.get_value("seed", seed, 57576, INT_MIN, INT_MAX, "seed integer for random number generator");
    if(seed > 0) seed = -seed;

    // Read in read and gain parameters
    std::vector<float> vread, vgain;
    bool more = true;
    float read, gain;
    while(more){
      input.get_value("read", read, 3.f, 0.f, 1.e5f, "readout noise, RMS electrons");
      input.get_value("gain", gain, 1.f, -FLT_MAX, FLT_MAX, "gain, (+ve = electrons/ADU, -ve = ADUs/electron)");
      if(gain > 0.){
    vgain.push_back(gain);
    vread.push_back(read);
      }else if(gain < 0.){
    vgain.push_back(-1./gain);
    vread.push_back(read);
      }else{
    std::cerr << "Cannot have zero gain; nothing saved, try again" << std::endl;
      }
      input.get_value("more", more, true, "enter another read/gain value pair?");
    }

    // Compute L3 data
    Subs::Buffer1D<Subs::Array1D<double> > cdf;
    if(type == "L3"){
      cdf.resize(nimax);
      for(int n=0; n<nimax; n++) cdf[n].resize(nmax);
      Ultracam::lllccd(nstage, pmult, pave, cdf);
    }

    float x, y;
    int ix, iy;

    for(size_t nf=0; nf<flist.size(); nf++){

      // Read in data
      frame.read(flist[nf]);

      // Generate signal electrons according to a Poisson distribution
      for(size_t ic=0; ic<frame.size(); ic++){
    for(size_t iw=0; iw<frame[ic].size(); iw++){
      Ultracam::Windata& win  = frame[ic][iw];
      for(int iy=0; iy<win.ny(); iy++){
        for(int ix=0; ix<win.nx(); ix++){
          if(win[iy][ix] < 0.)
        std::cerr << "WARNING: value = " << win[iy][ix] << " < 0 at pixel " << ix << ", " << iy << " of window " << (iw+1) << " of CCD " << ic+1 << std::endl;
          win[iy][ix] = Subs::poisson2(win[iy][ix], seed);
        }
      }
    }
      }

      if(type == "L3"){

    // Add parallel and serial CICs
    for(size_t ic=0; ic<frame.size(); ic++){
      for(size_t iw=0; iw<frame[ic].size(); iw++){
        Ultracam::Windata& win  = frame[ic][iw];
        for(int iy=0; iy<win.ny(); iy++){
          for(int ix=0; ix<win.nx(); ix++){

        // Compute the mean number of CICs from the distance the pixel
        // has to travel first in the parallel and then the serial direction
        float mcic = ppar*(iy+npar)+pser*(win.nxtot()-ix);

        // Then add with Poisson probability
        win[iy][ix] += Subs::poisson2(mcic, seed);
          }
        }
      }
    }
      }

      // Add cosmic rays
      for(size_t ic=0; ic<frame.size(); ic++){
    int ncadd = int(Subs::poisson2(ncosmic, seed));
    for(int ncos=0; ncos<ncadd; ncos++){
      x = frame[ic].nxtot()*Subs::ran2(seed);
      y = frame[ic].nytot()*Subs::ran2(seed);
      for(size_t iw=0; iw<frame[ic].size(); iw++){
        Ultracam::Windata& win = frame[ic][iw];
        if(win.enclose(x,y)){
          ix = int((x+0.5-win.llx())/win.xbin());
          iy = int((y+0.5-win.lly())/win.ybin());
          if(beta == 1.){
        win[iy][ix] += floor(elow*pow(c,float(Subs::ran2(seed)))+0.5);
          }else{
        win[iy][ix] += floor(pow(float(a*Subs::ran2(seed)+b),float(1./(1.-beta)))+0.5);
          }
          break;
        }
      }
    }
      }

      if(type == "L3"){

    // Single electron input without CICs, mean and variance
    double msingle = pow(1.+pmult, nstage);
    double vsingle = (1.-pmult)/(1.+pmult)*msingle*(msingle-1.);

    // Zero electron input with CICs, mean and variance
    double mzero = pow(1.+pmult, nstage);
    double vzero = (1.-pmult)/(1.+pmult)*msingle*(msingle-1.);

    int noff = 0;

    // Add effect of avalanche readout. This is the slowest part
    for(size_t ic=0; ic<frame.size(); ic++){
      for(size_t iw=0; iw<frame[ic].size(); iw++){
        Ultracam::Windata& win  = frame[ic][iw];
        for(int iy=0; iy<win.ny(); iy++){
          for(int ix=0; ix<win.nx(); ix++){

        // Number of electrons
        int nelec = int(win[iy][ix]+0.5);
        if(nelec < ngauss){

          int nmult  = nelec / (nimax-1);
          int nextra = nelec % (nimax-1);
          int nout = 0;

          // Add in several drawn from the CDF of the highest input
          for(int nm=0; nm<nmult; nm++){
            int nadd = cdf[nimax-1].locate(Subs::ran2(seed));
            if(nadd == cdf[nimax-1].size()) noff++;
            nout += nadd;
          }

          // Add on extra to make up to nelec
          if(nextra){
            int nadd = cdf[nextra].locate(Subs::ran2(seed));
            if(nadd == cdf[nextra].size()) noff++;
            nout += nadd;
          }

          // Add on part for in-register CICs
          int nadd = cdf[0].locate(Subs::ran2(seed));
          if(nadd == cdf[0].size()) noff++;
          nout += nadd;

          win[iy][ix] = nout;

        }else{

          // Gaussian approximation having the correct mean and RMS
          double mean  = mzero + nelec*msingle;
          double sigma = sqrt(vzero + nelec*vsingle);
          win[iy][ix] = std::max(0., mean + sigma*Subs::gauss2(seed));

        }
          }
        }
      }
    }
    std::cout << noff << " pixels were off the end of the CDFs" << std::endl;
      }

      // Add gaussian readout noise
      std::vector<float>::const_iterator ri = vread.begin();

      for(size_t ic=0; ic<frame.size(); ic++){
    for(size_t iw=0; iw<frame[ic].size(); iw++){
      Ultracam::Windata& win  = frame[ic][iw];

      if(ri != vread.end()) read = *(ri++);

      for(int iy=0; iy<win.ny(); iy++){
        for(int ix=0; ix<win.nx(); ix++){
          win[iy][ix] += read*Subs::gauss2(seed);
        }
      }
    }
      }

      // Add in bias frame and digitize, applying full well and ADC limits
      std::vector<float>::const_iterator gi = vgain.begin();

      for(size_t ic=0; ic<frame.size(); ic++){
    for(size_t iw=0; iw<frame[ic].size(); iw++){
      Ultracam::Windata&  win  = frame[ic][iw];
      Ultracam::Windata& bwin  = bias[ic][iw];

      if(gi != vgain.end()) gain = *(gi++);

      for(int iy=0; iy<win.ny(); iy++){
        for(int ix=0; ix<win.nx(); ix++){
          win[iy][ix] = std::min(float(full), win[iy][ix]);
          win[iy][ix] = std::min(double(adcmax), floor((bwin[iy][ix]+win[iy][ix])/gain + 0.5));
        }
      }
    }
      }

      // Write out
      frame.write(flist[nf]);
      std::cout << "Written \"" << flist[nf] << "\" to disk." << std::endl;

    }
  }

  catch(const Input_Error& err){
    std::cerr << "Ultracam::Input_Error exception:" << std::endl;
    std::cerr << err << std::endl;
  }
  catch(const Ultracam_Error& err){
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


