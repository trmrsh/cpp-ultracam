#include <vector>
#include "trm_subs.h"
#include "trm_array1d.h"
#include "trm_array2d.h"
#include "trm_ultracam.h"

/**
 * Plots trailed spectrum.
 * \param sdata  the spectra data for all frames, CCDs and objects
 * \param step   the number of slot to start with and to extend by when needed
 * \param reset  set = true to make sure it does a plot from scratch
 * \param ilow  lower intensity limit if user-defined direct scaling being used
 * \param ihigh upper intensity limit if user-defined direct scaling being used
 * \param plow  lower y percentile if percentile scaling being used
 * \param phigh upper y percentile if percentile scaling being used
 */
void Ultracam::plot_trail(const std::vector<std::vector<std::vector<Subs::Array1D<float> > > >& sdata, int step, bool reset,
			  Sreduce::PLOT_SCALING_METHOD scale_method, float ilow, float ihigh, float plow, float phigh){

    if(sdata.size() == 0){
	std::cerr << "Ultracam::plot_trail: no data to plot\n";
	return;
    }

    static bool first = true;
    static int nslot  = step;
    static Subs::Buffer2D<float> ilo, ihi;
    static int nx_old = 0, ny_old = 0;
    static Subs::Buffer2D<int> npix;
    
    int nnslot = step*(sdata.size()/step);
    if(nnslot < int(sdata.size())) nnslot += step;
    
    // Compute number of panels
    int nx = sdata[0].size(), ny = sdata[0][0].size();
    for(int nspec=0; nspec<int(sdata.size()); nspec++){
	nx = nx > int(sdata[nspec].size()) ? nx : sdata[nspec].size();
	for(int nccd=0; nccd<int(sdata[nspec].size()); nccd++)
	    ny = ny > int(sdata[nspec][nccd].size()) ? ny : sdata[nspec][nccd].size();
    }
    
    bool replot = (first || reset || nnslot > nslot || nx != nx_old || ny != ny_old);
    
    if(replot){
	nslot = nnslot;
	ilo.resize(nx, ny);
	ihi.resize(nx, ny);
	npix.resize(nx, ny);
    }
    
    nx_old = nx;
    ny_old = ny;
    
    // Space different CCDs horizontally, different objects vertically
    cpgsubp(nx, ny);

    // transformation coeffs
    float tr[6];
    tr[0] = 0;
    tr[1] = 1;
    tr[2] = 0;
    tr[3] = 0;
    tr[4] = 0;
    tr[5] = 1;
  
    // wind through each panel
    for(int nccd=0; nccd<nx; nccd++){
	for(int nobj=0; nobj<ny; nobj++){

	    cpgpanl(nccd+1, nobj+1);

	    // Compute limits
	    if(replot){
		ilo[nccd][nobj]  =  1e30;
		ihi[nccd][nobj]  = -1e30;
		npix[nccd][nobj] = 0;
		for(size_t nspec=0; nspec<sdata.size(); nspec++){
		    if(nccd < int(sdata[nspec].size()) && nobj < int(sdata[nspec][nccd].size())){
			const Subs::Array1D<float>& spec = sdata[nspec][nccd][nobj];
			npix[nccd][nobj] = npix[nccd][nobj] > spec.size() ? npix[nccd][nobj] : spec.size();

			float low = 0., high = 1.f;
			if(scale_method == Sreduce::AUTOMATIC){
			    low  = spec.min();
			    high = spec.max();
			}else if(scale_method == Sreduce::DIRECT){
			    low  = ilow;
			    high = ihigh;
			}else if(scale_method == Sreduce::PERCENTILE){
			    Subs::Array1D<float> temp = spec;
			    low  = temp.centile(plow);
			    high = temp.centile(phigh);
			}
			ilo[nccd][nobj] = ilo[nccd][nobj] < low ? ilo[nccd][nobj] : low;
			ihi[nccd][nobj] = ihi[nccd][nobj] > high ? ihi[nccd][nobj] : high;
		    }
		}

		// erase and re-plot completely
		cpgbbuf();
		cpgeras();
		if(npix[nccd][nobj]){
		    cpgvstd();
		    cpgswin(0.,npix[nccd][nobj]+1,0.,nslot+1);
		    for(size_t nspec=0; nspec<sdata.size(); nspec++){
			if(nccd < int(sdata[nspec].size()) && nobj < int(sdata[nspec][nccd].size())){
			    Subs::Array1D<float> temp = sdata[nspec][nccd][nobj];
			    tr[3] = nspec;
			    cpggray(temp, temp.size(), 1, 1, temp.size(), 1, 1, ihi[nccd][nobj], ilo[nccd][nobj], tr);
			}
		    }
		}
		cpgebuf();

	    }else{
		// Just add one more in this case
		if(nccd < int(sdata.back().size()) && nobj < int(sdata.back()[nccd].size())){
		    tr[3] = sdata.size()-1;
		    Subs::Array1D<float> temp = sdata.back()[nccd][nobj];
		    cpggray(temp, temp.size(), 1, 1, temp.size(), 1, 1, ihi[nccd][nobj], ilo[nccd][nobj], tr);
		}
	    }

	    // Re-plot the axes
	    cpgsci(4);
	    cpgbox("BCNST", 0, 0, "BCNST", 0, 0);
	    cpgsci(2);
	    cpglab("Pixels", "Spectrum", " ");
	    cpgsci(1);

	}
    }
    first = false;
}


