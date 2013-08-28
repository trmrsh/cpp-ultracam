/*

!!begin
!!title    shifter
!!author   T.R. Marsh
!!created  01 July 2004
!!revised  08 November 2007
!!root     shifter
!!index    shifter
!!descr    shifts and combines data frames allowing selection of good frames
!!class    Reduction
!!class    Manipulation
!!css      style.css
!!head1    shifter -- shifts and combines data frames allowing selection of good frames

!!emph{shifter} adds frames correcting for shifts and selecting by fwhm. It ignores junk blue data from the NBLUE option.
The final file contains the count RATES, i.e. counts/sec not the total counts. Exposure times are in the header so
you can switch back to counts if you prefer.

!!head2 Invocation

shifter [source] ((url)/(file) first trim [(ncol nrow) twait tmax])/(flist)
nsave bias (biasframe) flat (flatframe) aperture [xshift yshift] smethod [fwhm1d hwidth1d]
profit ([method symm (beta) fwhm hwidth readout gain sigrej] plo phi) output 

!!head2 Arguments

!!table

!!arg{source}{Data source, either 'l' for local, 's' for server or 'u' for ucm files. 'Local' means the 
usual .xml and .dat files accessed directly. Do not add either .xml or .dat to the file name; these are assumed.
'u' means you will need to specify a list of files which should all be .ucm files (either with or without
the extension)}

!!arg{url/file}{If source='S', 'url' is the complete URL of the file, e.g. 'http://127.0.0.1:8007/run00000012',
or just the file part in which case the program will try to find a default part to
add from the environment variable ULTRACAM_DEFAULT_URL. Failing this it will add
http://127.0.0.1:8007/, i.e. the local host. If source='L' then this should be plain file name
without .xml or .dat}

!!arg{first}{If source = 'S' or 'L', this is the number of the first file, starting from 1.}

!!arg{last}{If source = 'S' or 'L', this is the number of the last file, 0 for the whole lot}

!!arg{trim}{If source = 'S' or 'L', set trim=true to enable trimming of potential junk rows and
columns of each window}

!!arg{ncol}{If trim, then this specifies the number of columns nearest the readouts of each window to be snipped
off as these can be corrupted.}

!!arg{nrow}{If trim, then this specifies the number of rows to snip off the bottom of each window as these 
can be corrupted.}

!!arg{twait}{If source = 'S' or 'L', time to wait between attempts to find a new exposure (seconds).}

!!arg{tmax}{If source == 'S' or 'L', maximum time to wait before giving up (seconds). 
Set = 0 to quit as soon as a frame is not found.}

!!arg{flist}{If source = 'U', this is the name of a list of .ucm files to read.}

!!arg{bias}{true/false according to whether you want to subtract a bias frame. You can specify a full-frame 
bias because it will be cropped to match whatever your format is. This is useful for ultracam because of
the different bias levels of the 6 readouts.}

!!arg{biasframe}{If bias, then you need to specify the name of the bias frame}

!!arg{flat}{true/false according to whether you want to flat field the data. You can specify a full-frame 
flat because it will be cropped to match whatever your format is. The flat will be !!emph{divided} into
your data.}

!!arg{flatframe}{If flat, then you need to specify the name of the flatfield}

!!arg{aperture}{The file of apertures. You must mark the stars you want to use for determination of
the shifts as 'reference' stars using the 'S' option in !!ref{setaper.html}{setaper}. If any CCD has no reference stars marked,
a warning will be issued and no shift applied. If you are combining data from a number of runs, this file must be the same every
time. To minimise edge effects, you should try to define the aperture from a frame that represents the median position.}

!!arg{xshift}{Initial shift in X. This is useful if the targets have shifted grossly with respect to their position when
you set up the aperture file.}

!!arg{yshift}{Initial shift in Y. This is useful if the targets have shifted grossly with respect to their position when
you set up the aperture file.}

!!arg{smethod}{Shift method. 'N' for nearest pixel, 'L' for linear interpolation in both X and Y from 4 nearest pixels.
NB only simple translations are supported, no rotations or other distortions. 'N' is faster than, but not as good as
'L'. Linear interpolation is recommended unless speed is critical.}

!!arg{fwhm1d}{FWHM for 1D search used to re-position apertures. Unbinned pixels}

!!arg{hwidth1d}{Half-width in unbinned pixels for 1D search used to re-position apertures.}

!!arg{profit}{yes/no for profile fits or not}

!!arg{method}{There are two options for profile fitting: 2D Gaussian or a Moffat profile. 
The latter is a much better representation of seeing broadened profiles. The Gaussian can
be symmetric or elliptical. The Moffat profile is of the form h/(1+(r/R)**2)**beta where
r is the distance from the centre of the profile, R is a scale length and beta is a parameter
that you will be prompted for an intial value. As beta gets large, the Moffat profile tends
to Gaussian. The intial value of R is computed from a FWHM value. The routine plots a
line at 2 times the measured FWHM on the image (not yet for the asymmetric gaussian).}

!!arg{symm}{Yes/no for symmetric versus elliptical gaussians in the
case of gaussian fits. I have not fully tested this option.}

!!arg{fwhm}{fwhm is the initial FWHM to use for the profile fits.}

!!arg{hwidth}{The half-width of the region to be used when fitting a
target. Should be larger than the fwhm, but not so large as to include
multiple targets if possible. This is also the region used to compute the
maximum value that the program will report.}

!!arg{readout}{Readout noise, RMS ADU in order for the program to come
back with an uncertainty. NB It is important in this case to carry out
bias subtraction if you want half-way decent error estimates.}

!!arg{gain}{Gain, electrons/ADU, again for uncertainty estimates}

!!arg{sigrej}{The fits can include rejection of poor pixels. This is
the threshold, measured in sigma. Should not be too small as this may
cause rejection of many points which can slow the routine down a fair
bit as well as leading to calamity in some cases.}

!!arg{fdevice}{Plot device for fits, specify as fdevice="" if you don't want any plots.}

!!arg{plo}{If you fit profilesin then frames are accepted according to whether their FWHM
lies in a given percentile range or not. This is the lower limit in percent.}

!!arg{phi}{If you fit profilesin then frames are accepted according to whether their FWHM
lies in a given percentile range or not. This is the upperlimit in percent}

!!arg{output}{Name of the output frame}

!!table

!!end

*/

#include <cstdlib>
#include <cfloat>
#include <climits>
#include <string>
#include <fstream>
#include "trm/subs.h"
#include "trm/format.h"
#include "trm/array1d.h"
#include "trm/ephem.h"
#include "trm/position.h"
#include "trm/input.h"
#include "trm/header.h"
#include "trm/mccd.h"
#include "trm/frame.h"
#include "trm/window.h"
#include "trm/aperture.h"
#include "trm/ultracam.h"

// Main program

int main(int argc, char* argv[]){
  
    using Ultracam::File_Open_Error;
    using Ultracam::Ultracam_Error;
    using Ultracam::Input_Error;

    try{

	// Construct Input object
	Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

	// Sign-in input variables
	input.sign_in("source",    Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("url",       Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("file",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("first",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("last",      Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("trim",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("ncol",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("nrow",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("twait",     Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("tmax",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("flist",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("bias",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("biasframe", Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("flat",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("flatframe", Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("aperture",  Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("xshift",    Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
	input.sign_in("yshift",    Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
	input.sign_in("smethod",   Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("fwhm1d",    Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("hwidth1d",  Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("profit",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("method",    Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
	input.sign_in("symm",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("beta",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("fwhm",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("hwidth",    Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("readout",   Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("gain",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("sigrej",    Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("fdevice",   Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("plo",       Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("phi",       Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("output",    Subs::Input::LOCAL,  Subs::Input::PROMPT);

	// Get inputs
	char source;
	input.get_value("source", source, 'S', "sSlLuU", "data source: L(ocal), S(erver) or U(cm)?");
	source = toupper(source);
	std::string url;
	if(source == 'S'){
	    input.get_value("url", url, "url", "url of file");
	}else if(source == 'L'){
	    input.get_value("file", url, "file", "name of local file");
	}
	size_t first, last;
	bool trim;
	std::vector<std::string> file;
	Ultracam::Mwindow mwindow;
	Subs::Header header;
	Ultracam::ServerData serverdata;
	Ultracam::Frame data, dvar;
	double twait, tmax;
	int ncol, nrow;
	if(source == 'S' || source == 'L'){
	    input.get_value("first", first, size_t(1), size_t(1), size_t(9999999), "first frame to access (starting from 1)");
	    input.get_value("last", last, size_t(0), size_t(0), size_t(9999999), "last frame to access (0 for all)");
	    if(last != 0 && last < first)
		throw Ultracam::Input_Error("last must be either 0 or >= first");

	    input.get_value("trim", trim, true, "trim junk lower rows from windows?");
	    if(trim){
		input.get_value("ncol", ncol, 0, 0, 100, "number of columns to trim from each window");
		input.get_value("nrow", nrow, 0, 0, 100, "number of rows to trim from each window");
	    }
	    input.get_value("twait", twait, 1., 0., 1000., "time to wait between attempts to find a frame (seconds)");
	    input.get_value("tmax", tmax, 2., 0., 100000., "maximum time to wait before giving up trying to find a frame (seconds)");

	    // Add extra stuff to URL if need be.
	    if(url.find("http://") == std::string::npos && source == 'S'){
		char *DEFAULT_URL = getenv(Ultracam::ULTRACAM_DEFAULT_URL);
		if(DEFAULT_URL != NULL){
		    url = DEFAULT_URL + url;
		}else{
		    url = Ultracam::ULTRACAM_LOCAL_URL + url;
		}
	    }else if(url.find("http://") == 0 && source == 'L'){
		throw Ultracam::Input_Error("Should not specify local file as a URL");
	    }

	    parseXML(source, url, mwindow, header, serverdata, trim, ncol, nrow, twait, tmax);
	    data.format(mwindow, header);

	}else{

	    std::string flist;
	    input.get_value("flist", flist, "files.lis", "name of local file list");

	    // Read file list
	    std::string name;
	    std::ifstream istr(flist.c_str());
	    while(istr >> name){
		file.push_back(name);
	    }
	    istr.close();
	    if(file.size() == 0) 
		throw Input_Error("No file names loaded");

	    data.read(file[0]);

	    first = 0;
	}

	bool bias;
	input.get_value("bias", bias, true, "do you want to subtract a bias frame?");
	Ultracam::Frame bias_frame;
	if(bias){
	    std::string sbias;
	    input.get_value("biasframe", sbias, "bias", "name of bias frame");
	    bias_frame.read(sbias);
	    bias_frame.crop(data);
	}

	bool flat;
	input.get_value("flat", flat, true, "do you want to apply a flat field?");
	Ultracam::Frame flat_frame;
	if(flat){
	    std::string sflat;
	    input.get_value("flatframe", sflat, "flat", "name of flatfield frame");
	    flat_frame.read(sflat);
	    flat_frame.crop(data);
	}

	std::string saper;
	input.get_value("aperture", saper, "aper", "enter aperture file with reference stars");
	Ultracam::Maperture master_aperture(saper);
	if(master_aperture.size() != data.size())
	    throw Ultracam::Input_Error("Number of CCDs in aperture file does not match number in data file");

	float xshift;
	input.get_value("xshift",  xshift,  0.f, -1000.f, 1000.f, "initial shift in X to help acquire reference stars");
	float yshift;
	input.get_value("yshift",  yshift,  0.f, -1000.f, 1000.f, "initial shift in Y to help acquire reference stars");
	char smethod;
	input.get_value("smethod", smethod, 'L', "nNlL", "method to use for shifting");
	Ultracam::SHIFT_METHOD shift_method;
	if(toupper(smethod) == 'N'){
	    shift_method = Ultracam::NEAREST_PIXEL;
	}else if(toupper(smethod) == 'L'){
	    shift_method = Ultracam::LINEAR_INTERPOLATION;
	}else{
	    throw Ultracam::Input_Error("Shift method unrecognised");
	}
	float fwhm1d;
	input.get_value("fwhm1d",   fwhm1d,  10.f, 2.f, 1000.f, "FWHM for 1D search for aperture re-positioning");
	int   hwidth1d;
	input.get_value("hwidth1d", hwidth1d, int(2.*fwhm1d)+1, int(fwhm1d+1), INT_MAX, "half-width of 1D search region");

	// Profile fit stuff
	char  profit;
	input.get_value("profit",  profit, 'n', "nNyY", "do you want profile fits? N(o), Y(es)");
	profit = std::toupper(profit);
	float fwhm, readout = 4, gain=1.1, beta, sigrej, plo, phi;
	int   hwidth;
	bool  symm;
	std::string fdevice;
	char method='M';
	if(profit == 'Y'){
	    input.get_value("method",  method,  'm', "mMgG", "G(aussian) or M(offat) profile?");
	    method = toupper(method);
	    input.get_value("symm",    symm, true, "force symmetric profiles?");
	    if(method == 'M')
		input.get_value("beta",   beta, 4.f, 0.5f, 1000.f, "default beta exponent for Moffat fits");

	    input.get_value("fwhm",    fwhm,  10.f, 2.f, 1000.f, "initial FWHM for profile fits");
	    input.get_value("hwidth",  hwidth, 31, int(fwhm)+1, INT_MAX, 
			    "half-width of fit region for profile fits (unbinned pixels)");
	    input.get_value("readout", readout,  4.f, 0.f, FLT_MAX, "readout noise for fits (RMS ADU)");
	    input.get_value("gain",    gain,  1.f, 0.01f, 100.f, "electrons/ADU for fits");
	    input.get_value("sigrej",  sigrej, 5.f, 0.f, FLT_MAX, "threshold for masking pixels (in sigma)");
	    input.get_value("fdevice", fdevice, "", "plot device for profile fits"); 
	    input.get_value("plo",  plo,  0.f, 0.f, 100.f, "lowest FWHM percentile to accept");
	    input.get_value("phi",  phi,  std::max(plo, 20.f), std::max(plo, 0.f), 100.f, "highest FWHM percentile to accept");
	}

	// variance frame
	dvar = data;
	dvar.max(0.);
	dvar /= gain;
	dvar += readout*readout;

	std::string output;
	input.get_value("output", output, "output", "name of the output shift-and-added file");

	// Save inputs
	input.save();
    
	Subs::Format form(6);

	// Now create sum frame
	Ultracam::Frame sum(data);
	sum = 0.f;

	for(size_t nccd=0; nccd<sum.size(); nccd++){
	    std::string sccd = Subs::str(nccd+1);
	    sum.set("CCD" + sccd,  new Subs::Hdirectory("Information for CCD " + sccd));
	}

	// Create timing info, which has to be different for each CCD.
	std::vector<int> nused(data.size(),0);
	std::vector<int> njunk(data.size(),0);
	std::vector<int> ntotal(data.size(),0);
	std::vector<double> tottime(data.size(),0.);
	std::vector<float> texposure(data.size(),0.f);
	std::vector<float> tfwhm(data.size(),0.f);

	bool initialised = false;
	size_t nfile;
	float exposure = 0.f, exposure_blue = 0.f;
	Subs::Time ut_date, ut_date_blue, ttime(1,Subs::Date::May,2002);
	Ultracam::Maperture last_aperture;
	bool reliable = false, reliable_blue = false;
	Subs::Header::Hnode *hnode;
	std::vector<Ultracam::sky_mask> skymask;

	// Open plot
	Subs::Plot fplot;
	if(fdevice != "") fplot.open(fdevice);

	int maxpass = 1;
	if(profit == 'Y')
	    maxpass = 2;

	// Shift and fwhm arrays 
	std::vector<std::vector<Ultracam::Shift_info> > shift_info;
	std::vector<std::vector<float> > fwhm_obs;
	std::vector<float> flo(data.size());
	std::vector<float> fhi(data.size());
	for(int npass=1; npass<=maxpass; npass++){
	
	    nfile      = first;
	    if(maxpass == 2){
		if(npass == 1){
		    std::cout << "Carrying out first pass to measure FWHMs" << std::endl;
		}else{
		    std::cout << "Carrying out second pass to compute FWHM ranges and add in frames" << std::endl;
		
		    // Calculate percentile limits
		    float *p = new float [fwhm_obs.size()];
		    for(size_t nccd=0;nccd<data.size(); nccd++){
			unsigned long int ntot = 0;
			for(size_t i=0; i<fwhm_obs.size(); i++)
			    if(shift_info[i][nccd].ok)
				p[ntot++] = fwhm_obs[i][nccd];
			unsigned long int k1 = std::max((unsigned long int)(0), (unsigned long int)(floor(ntot*plo/100.+0.5)));
			unsigned long int k2 = std::min((unsigned long int)(ntot-1),(unsigned long int)(floor(ntot*phi/100.+0.5)));
			flo[nccd] = Subs::select(p,ntot,k1);
			fhi[nccd] = Subs::select(p,ntot,k2);
			std::cout << "CCD " << nccd + 1 << " FWHM range  = " << form(flo[nccd]) << " to " << form(fhi[nccd]) << std::endl;
		    }
		    delete[] p;
		
		}
	      
	    }

	    int nexp = 0;
	
	    for(;;){
	    
		// Get data
		if(source == 'S' || source == 'L'){
		
		    // Carry on reading until data & time are OK
		    bool get_ok = false, reset = (npass == 2 && nfile == first);
		    while(last == 0 || nfile <= last){
			if(!(get_ok = Ultracam::get_server_frame(source, url, data, serverdata, nfile, twait, tmax, reset))) break;
			ut_date       = data["UT_date"]->get_time();
			ut_date_blue  = serverdata.nblue > 1 ? data["UT_date_blue"]->get_time() : ut_date;
			reliable      = data["Frame.reliable"]->get_bool();
			reliable_blue = serverdata.nblue > 1 ? data["Frame.reliable_blue"]->get_bool() : reliable;
			exposure      = data["Exposure"]->get_float();
			exposure_blue = serverdata.nblue > 1 ? data["Exposure_blue"]->get_float() : exposure;
			if(serverdata.is_junk(nfile)){
			    std::cerr << "Skipping file number " << nfile << " which has junk data" << std::endl;
			    nfile++;
			}else if(ut_date < ttime){
			    std::cerr << "Skipping file number " << nfile << " which has junk time = " << ut_date << std::endl;
			    nfile++;
			}else{
			    break;
			}
		    }
		    if(!get_ok) break;
		
		    std::cout << "Processing frame number " << nfile << ", time = " << ut_date << std::endl;	
		
		
		}else{
	  
		    while(nfile < file.size()){
			data.read(file[nfile]);
			ut_date  = data["UT_date"]->get_time();
			hnode    = data.find("Frame.reliable");
			reliable = (hnode->has_data() && hnode->value->get_bool());
			exposure = data["Exposure"]->get_float();
			if((hnode = data.find("Instrument.nblue"))->has_data() && hnode->value->get_int() > 1){
			    ut_date_blue   = data["UT_date_blue"]->get_time();
			    exposure_blue  = data["Exposure_blue"]->get_float();
			    reliable_blue  = data["Frame.reliable_blue"]->get_bool();
			}else{
			    ut_date_blue  = ut_date;
			    exposure_blue = exposure;
			    reliable_blue = reliable;
			}

			if(ut_date < ttime){
			    std::cerr << "Skipping file " << file[nfile] << " which has junk time = " << ut_date << std::endl;
			    nfile++;
			}else{
			    break;
			}
		    }
		    if(nfile == file.size()) break;
		
		    std::cout << "Processing file = " << file[nfile] << ", time = " << ut_date << std::endl;	
		
		}
	    
		Subs::Header::Hnode *hnode = data.find("Frame.bad_blue");
		bool blue_is_bad = hnode->has_data() ? hnode->value->get_bool() : false;

		// Apply calibrations
		if(bias) data -= bias_frame;
		dvar = data;
		dvar.max(0.);
		dvar /= gain;
		dvar += readout*readout;
		if(flat) data /= flat_frame;
	    	    
		if(npass == 1){
		
		    // add new elements
		    shift_info.push_back(std::vector<Ultracam::Shift_info>(data.size()));
		    fwhm_obs.push_back(std::vector<float>(data.size()));
		
		    Ultracam::Maperture aperture;
		
		    if(!initialised) {
			aperture = master_aperture;
		    
			// Apply initial shift
			for(size_t nccd=0; nccd<data.size(); nccd++){
			    for(size_t naper=0; naper<aperture[nccd].size(); naper++){
				Ultracam::Aperture* app = &aperture[nccd][naper];
				app->set_xref(app->xref() + xshift);
				app->set_yref(app->yref() + yshift);
			    }
			}
			initialised = true;
	    
		    }else{
			aperture = last_aperture;
		    }
		
		    for(size_t nccd=0; nccd<data.size(); nccd++){
	    
			if((nccd == 2 && !blue_is_bad && reliable_blue) || (nccd != 2 && reliable)){
		
			    ntotal[nccd]++;

			    // Loop over apertures
			    float sx = 0., sy = 0., afwhm = 0.;
			    int nap = 0, nfwhm = 0;
			    shift_info[nexp][nccd].ok = true;
			    
			    for(size_t naper=0; naper<aperture[nccd].size(); naper++){
				
				Ultracam::Aperture* app  = &aperture[nccd][naper];
				Ultracam::Aperture* rapp = &master_aperture[nccd][naper];
				
				// Only consider valid reference apertures
				if(app->valid() && app->ref()){
				    
				    try{
					
					const Ultracam::Windata &dwin = data[nccd].enclose(app->xref(), app->yref());
					const Ultracam::Windata &vwin = dvar[nccd].enclose(app->xref(), app->yref());
					
					// window found for this aperture, but need to check that
					// star aperture is fully enclosed by it
					if(
					    dwin.left()   < app->xref()-app->rstar() &&
					    dwin.bottom() < app->yref()-app->rstar() &&
					    dwin.right()  > app->xref()+app->rstar() &&
					    dwin.top()    > app->yref()+app->rstar()){
					    
					    float xstart   = dwin.xcomp(app->xref());
					    float ystart   = dwin.ycomp(app->yref());
					    float xref     = dwin.xcomp(rapp->xref());
					    float yref     = dwin.ycomp(rapp->yref());
					    
					    float fwhm_x   = fwhm1d/dwin.xbin();
					    fwhm_x = fwhm_x > 2.f ? fwhm_x : 2.f;
					    float fwhm_y   = fwhm1d/dwin.ybin();
					    fwhm_y = fwhm_y > 2.f ? fwhm_y : 2.f;
					    
					    int   hwidth_x = hwidth1d/dwin.xbin();
					    hwidth_x = hwidth_x > int(fwhm_x+1.) ? hwidth_x : int(fwhm_x+1.);
					    int   hwidth_y = hwidth1d/dwin.ybin();
					    hwidth_y = hwidth_y > int(fwhm_y+1.) ? hwidth_y : int(fwhm_y+1.);
					    double xpos, ypos;
					    float xe, ye;
					    
					    // Remeasure the position
					    Ultracam::findpos(dwin, vwin, dwin.nx(), dwin.ny(), fwhm_x, fwhm_y, hwidth_x, hwidth_y, xstart, ystart, true, 
							      xpos, ypos, xe, ye);
					    
					    sx += dwin.xbin()*(xpos-xref);
					    sy += dwin.ybin()*(ypos-yref);
					    nap++;
					    
					    if(profit == 'Y'){
						
						xpos = dwin.xccd(xpos);
						ypos = dwin.yccd(ypos);
						
						float sky, peak;
						Ultracam::profit_init(data[nccd], dvar[nccd], xpos, ypos, false, fwhm1d, hwidth1d, hwidth, sky, peak, false);
						Ultracam::Ppars profile;
						Ultracam::Iprofile iprofile;
						
						// obtain initial value of 'a'
						double a = 1./2./Subs::sqr(fwhm/Constants::EFAC);
						
						if(method == 'G'){	    
						    std::cout << "\nFitting 2D gaussian ...\n" << std::endl;
						    profile = Ultracam::Ppars(sky, xpos, ypos, peak, a, 0., a, symm);
						    
						}else if(method == 'M'){	    
						    std::cout << "\nFitting moffat profile ...\n" << std::endl;
						    profile = Ultracam::Ppars(sky, xpos, ypos, peak, a, 0., a, beta, symm);
						}
						
						Ultracam::fit_plot_profile(data[nccd], dvar[nccd], profile, false, false, 0., 0., skymask,
									   fwhm1d, hwidth1d, hwidth, fplot, sigrej, iprofile, true);
						
						afwhm += iprofile.fwhm;
						nfwhm++;
					    }
					}
				    }
				    catch(const std::string& err){
					std::cerr << "Aperture number " << naper+1 << std::endl;
					std::cerr << err << std::endl;
				    }
				}
			    }
	    
			    // Update aperture file
			    if(nap){
				sx /= nap;
				sy /= nap;
				// -ve shifts to compensate
				shift_info[nexp][nccd].dx = -sx;
				shift_info[nexp][nccd].dy = -sy;
				for(size_t naper=0; naper<aperture[nccd].size(); naper++){	      
				    Ultracam::Aperture* app  = &aperture[nccd][naper];
				    Ultracam::Aperture* rapp = &master_aperture[nccd][naper];
				    app->set_xref(rapp->xref() + sx);
				    app->set_yref(rapp->yref() + sy);
				}
				std::cout << "Will apply a shift of (" << form(shift_info[nexp][nccd].dx) << "," << form(shift_info[nexp][nccd].dy) << ") to CCD " << nccd + 1 << std::endl;
				
			    }else{
				std::cerr << "No valid reference apertures located for CCD number " << nccd + 1 << std::endl;
				std::cerr << "This CCD will not be added in. " << std::endl;
				shift_info[nexp][nccd].ok = false;
			    }
	    
			    if(nfwhm){
				afwhm /= nfwhm;
				fwhm_obs[nexp][nccd] = afwhm;
			    }else{
				std::cout << "No FWHM measured for CCD = " << nccd+1 << std::endl;
				std::cerr << "This CCD will not be added in. " << std::endl;
				shift_info[nexp][nccd].ok = false;
			    }
			}else{
			    // Frames with bad times
			    if((nccd == 2 && !reliable_blue) || (nccd != 2 && !reliable)) njunk[nccd]++;
			    shift_info[nexp][nccd].ok = false;
			}
		    }
		    last_aperture = aperture;

		}else{

		    for(size_t nccd=0; nccd<data.size(); nccd++){
			if(shift_info[nexp][nccd].ok){
			    if(fwhm_obs[nexp][nccd] < flo[nccd] || fwhm_obs[nexp][nccd] > fhi[nccd]){
				std::cout << "CCD " << nccd+1 << " of frame " << nfile << " has FWHM = " << form(fwhm_obs[nexp][nccd]) << " which is out of range " 
					  << form(flo[nccd]) << " to " << form(fhi[nccd]) << " and will be skipped." << std::endl;
				shift_info[nexp][nccd].ok = false;
			    }else{
				std::cout << "CCD " << nccd+1 << " of frame " << nfile << " has FWHM = " << form(fwhm_obs[nexp][nccd]) << " which is in range "
					  << form(flo[nccd]) << " to " << form(fhi[nccd]) << " and will be included." << std::endl;
			    }
			}else{
			    std::cout << "CCD " << nccd+1 << " of frame " << nfile << " is flagged to be skipped" << std::endl;
			}
		    }

		}

		// add it in
		if(npass == maxpass){
		    // Add into sum with unit weight, keep track of numbers added in
		    Ultracam::shift_and_add(sum, data, shift_info[nexp], Ultracam::internal_data(1.), shift_method);
		    for(size_t nccd=0; nccd<data.size(); nccd++){
			if(shift_info[nexp][nccd].ok){
			    nused[nccd]++;
			    if(nccd == 2){
				std::cerr << "nfile = " << nfile << ", exposure_blue = " << exposure_blue << std::endl;

				tottime[nccd]   += ut_date_blue.mjd();
				texposure[nccd] += exposure_blue;
				tfwhm[nccd]     += exposure_blue*fwhm_obs[nexp][nccd];
			    }else{
				tottime[nccd]   += ut_date.mjd();
				texposure[nccd] += exposure;
				tfwhm[nccd]     += exposure*fwhm_obs[nexp][nccd];
			    }
			}
		    }
		}

		nfile++;	
		nexp++;
	    }
	}
    
	// normalise, set headers and dump to disk
	for(size_t nccd=0; nccd<data.size(); nccd++){

	    std::string sccd = "CCD" + Subs::str(nccd+1) + ".";
	    if(nused[nccd]){
		sum[nccd]       /= texposure[nccd];

		sum.set(sccd + "UT_date",  new Subs::Htime(Subs::Time(tottime[nccd]/nused[nccd]), "UTC date and time"));
		sum.set(sccd + "Exposure", new Subs::Hfloat(texposure[nccd],  "Exposure time, seconds"));

		if(profit == 'Y'){
		    sum.set(sccd + "FWHM",     new Subs::Hfloat(tfwhm[nccd]/texposure[nccd], "Exposure-weighted mean FWHM, pixels"));
		    sum.set(sccd + "FWHMLO",   new Subs::Hfloat(flo[nccd], "Lowest FWHM included, pixels"));
		    sum.set(sccd + "FWHMHI",   new Subs::Hfloat(fhi[nccd], "Highest FWHM included, pixels"));
		}
	    }
	    sum.set(sccd + "NTOT",   new Subs::Hint(ntotal[nccd], "Total number of images available"));
	    sum.set(sccd + "NADD",   new Subs::Hint(nused[nccd],  "Number of images used"));
	    sum.set(sccd + "NJUNK",  new Subs::Hint(njunk[nccd],  "Number ignored because of bad times"));

	    std::cout << "CCD " << nccd+1 << " had " << nused[nccd] << " valid images" << std::endl;
	}

	sum.write(output);
    }


    // Handle errors
    catch(const Input_Error& err){
	std::cerr << "\nUltracam::Input_Error:" << std::endl;
	std::cerr << err << std::endl;
    }
    catch(const File_Open_Error& err){
	std::cerr << "\nUltracam::File_Open_error:" << std::endl;
	std::cerr << err << std::endl;
    }
    catch(const Ultracam_Error& err){
	std::cerr << "\nUltracam::Ultracam_Error:" << std::endl;
	std::cerr << err << std::endl;
    }
    catch(const Subs::Subs_Error& err){
	std::cerr << "\nSubs::Subs_Error:" << std::endl;
	std::cerr << err << std::endl;
    }
    catch(const std::string& err){
	std::cerr << "\n" << err << std::endl;
    }
}



