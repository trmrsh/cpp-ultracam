#include <vector>
#include "trm/subs.h"
#include "trm/array1d.h"
#include "trm/ultracam.h"

/**
 * Plots extracted spectra.
 * \param sdata  the spectrum data for each defined object of a set of CCDs
 * \param serror the equivalent errors 
 * \param individual_scale scale each spectrum individually or all together
 * \param ylow  lower y limit if user-defined direct scaling being used
 * \param yhigh upper y limit if user-defined direct scaling being used
 * \param plow  lower y percentile if percentile scaling being used
 * \param phigh upper y percentile if percentile scaling being used
 */

void Ultracam::plot_spectrum(const std::vector<std::vector<Subs::Array1D<float> > >& sdata, const std::vector<std::vector<Subs::Array1D<float> > >& serror,
			     bool individual_scale, Sreduce::PLOT_SCALING_METHOD scale_method, float ylow, float yhigh, float plow, float phigh){

    // Plot limits
    static bool first = true;
    static std::vector<std::vector<float> > ylo, yhi;
    
    Subs::Array1D<float> xarr;

    // Compute number of panels
    int nx = sdata.size();
    size_t ny = sdata[0].size();
    if(first){
	ylo.resize(nx);
	yhi.resize(nx);
    }
    for(size_t nccd=0; nccd<sdata.size(); nccd++){
	if(first){
	    ylo[nccd].resize(sdata[nccd].size());
	    yhi[nccd].resize(sdata[nccd].size());
	}
	ny = ny > sdata[nccd].size() ? ny : sdata[nccd].size();
    }

    // Space different CCDs horizontally, different objects vertically
    cpgsubp(nx, ny);

    for(size_t nccd=0; nccd<sdata.size(); nccd++){

	for(size_t nreg=0; nreg<sdata[nccd].size(); nreg++){

	    const Subs::Array1D<float>& spec_dat = sdata[nccd][nreg];
	    const Subs::Array1D<float>& spec_err = serror[nccd][nreg];
	    xarr.resize(spec_dat.size());
	    for(int ix=0; ix<xarr.size(); ix++)
		xarr[ix] = ix + 1;

	    if(first || individual_scale){
		if(scale_method == Sreduce::AUTOMATIC){
		    ylo[nccd][nreg] = spec_dat.min() > spec_err.min() ? spec_err.min() : spec_dat.min();
		    yhi[nccd][nreg] = spec_dat.max() < spec_err.max() ? spec_err.max() : spec_dat.max();
		}else if(scale_method == Sreduce::DIRECT){
		    ylo[nccd][nreg] = ylow;
		    yhi[nccd][nreg] = yhigh;
		}else if(scale_method == Sreduce::PERCENTILE){
		    Subs::Array1D<float> temp = spec_dat;
		    ylo[nccd][nreg] = temp.centile(plow);
		    yhi[nccd][nreg] = temp.centile(phigh);
		}
	    }else{
		float nlo=0.f, nhi=1.f;
		if(scale_method == Sreduce::AUTOMATIC){
		    nlo = spec_dat.min() > spec_err.min() ? spec_err.min() : spec_dat.min();
		    nhi = spec_dat.max() < spec_err.max() ? spec_err.max() : spec_dat.max();
		}else if(scale_method == Sreduce::DIRECT){
		    nlo = ylow;
		    nhi = yhigh;
		}else if(scale_method == Sreduce::PERCENTILE){
		    Subs::Array1D<float> temp = spec_dat;
		    nlo = temp.centile(plow);
		    nhi = temp.centile(phigh);
		}
		ylo[nccd][nreg] = ylo[nccd][nreg] > nlo ? nlo : ylo[nccd][nreg];
		yhi[nccd][nreg] = yhi[nccd][nreg] < nhi ? nhi : yhi[nccd][nreg];
	    }

	    cpgpanl(nccd+1, nreg+1);
	    cpgeras();
	    cpgvstd();
	    cpgswin(0., spec_dat.size()+1, ylo[nccd][nreg], yhi[nccd][nreg]);
	    cpgsci(4);
	    cpgbox("BCNST", 0, 0, "BCNST", 0, 0);
	    cpgsci(2);
	    cpglab("Pixels", "Counts", " ");
	    cpgsci(1);
	    pgbin(xarr, spec_dat);
	    cpgsci(2);
	    pgbin(xarr, spec_err);
	}
    }
    first = false;
}


