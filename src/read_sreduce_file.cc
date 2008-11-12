#include <cstdlib>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include "trm_subs.h"
#include "trm_time.h"
#include "trm_specap.h"
#include "trm_mccd.h"
#include "trm_frame.h"
#include "trm_ultracam.h"

// External variables for communicating with sreduce. See 'sreduce.cc' for full list of meanings.

namespace Sreduce {

    // General
    extern Ultracam::Logger logger;
    extern ABORT_BEHAVIOUR abort_behaviour;      
    extern std::vector<float> saturation;
    extern TERM_OUT terminal_output;
    extern bool   coerce;                                  
    extern int    naccum;
    extern bool   threshold;                                  
    extern float  photon;

    // Calibration
    extern bool bias;                                    
    extern Ultracam::Frame bias_frame;                             
    extern bool dark;                                    
    extern Ultracam::Frame dark_frame;                             
    extern bool flat;                                    
    extern Ultracam::Frame flat_frame;                             
    extern bool bad_pixel;                                    
    extern Ultracam::Frame bad_pixel_frame;                             
    extern bool gain_const;                              
    extern float gain;                                   
    extern Ultracam::Frame gain_frame;                             
    extern bool readout_const;                           
    extern float readout;                                
    extern Ultracam::Frame readout_frame;                
          
    // Regions
    extern Ultracam::Mspecap region_master;
    extern REGION_REPOSITION_MODE region_reposition_mode;
    extern float  region_fwhm;                
    extern float  region_max_shift;                
    extern int    region_hwidth;                

    // Sky fitting
    extern bool  sky_fit;
    extern int   sky_npoly;
    extern float sky_reject;                               

    // The plot devices
    extern std::string spectrum_device;                          
    extern std::string trail_device;                          
    extern std::string hard_device;                          

    // Spectrum plot scaling
    extern bool   spectrum_scale_individual;
    extern PLOT_SCALING_METHOD spectrum_scale_method;
    extern float  spectrum_ylow;
    extern float  spectrum_yhigh;
    extern float  spectrum_plow;
    extern float  spectrum_phigh;

    // Trail plot scaling
    extern PLOT_SCALING_METHOD trail_scale_method;
    extern float  trail_ilow;
    extern float  trail_ihigh;
    extern float  trail_plow;
    extern float  trail_phigh;

    // Starting number of slots in trails
    extern int trail_start;

};


/**
 * read_reduce_file is a function dedicated to reading the data that controls
 * the ultracam pipeline reduction. It is basically a long series of inputs and tests
 * for validity, with exceptions thrown if errors are encountered. It reads the file
 * data into a series of external variables also defined in reduce.
 *
 * These are all defined within a namespace called "Sreduce" to aid identification and
 * for safety.
 *
 * \param file     filename with data to control the operation of reduce
 * \param logfile  file to store log of output from reduce.
 */

void Ultracam::read_sreduce_file(const std::string& file, const std::string& logfile){

    std::ifstream freduce(file.c_str());
    if(!freduce) throw File_Open_Error("Error opening " + file);

    char ch;
    std::string option, value;
    std::map<std::string,std::string> reduce;
    std::multimap<std::string,std::string> special;
    std::pair<std::map<std::string,std::string>::iterator,bool> pssins;
    const int MAX_LINE = 2048;
    int n = 0;
    while(freduce && !freduce.eof()){
	n++;
	ch = freduce.peek();
	if(freduce.eof()){
	    std::cout << "End of file reached." << std::endl;
	}else if(ch == '#' || ch == ' ' || ch == '\t' || ch == '\n'){
	    freduce.ignore(MAX_LINE, '\n'); // skip to next line
	}else{

	    if(freduce >> option){

		while(freduce.get(ch) && ch != '='); // skips up to = sign

		if(!freduce) 
		    throw Ultracam_Error("Line " + Subs::str(n) + " starting: " + option + " is invalid");
	
		getline(freduce,value);

		// Work out end of value string defined by first hash
		std::string::size_type ntemp, nhash = 0;
		while((ntemp = value.find('#',nhash)) != std::string::npos && 
		      ntemp > 0 && value[ntemp-1] == '\\'){
		    nhash = ntemp + 1;
		}
		if(ntemp != std::string::npos) value.erase(ntemp);

		// Chop off space at start and end
		std::string::size_type n1 = value.find_first_not_of(" \t");
		std::string::size_type n2 = value.find_last_not_of(" \t");

		if(n1 != std::string::npos){
		    value = value.substr(n1,n2-n1+1);
		}else{
		    value = "";
		}

		if(option == "lightcurve_targ" || 
		   option == "position_targ"   ||
		   option == "seeing_targ"){

		    // special cases which can be repeated multiple times
		    special.insert(std::make_pair(option, value));

		}else{

		    // the normal case
		    pssins = reduce.insert(std::make_pair(option, value));

		    if(!pssins.second)
			throw Ultracam_Error("Option = " + option + " is a repeat!");

		}

	    }else if(freduce.eof()){
		std::cout << "End of file reached." << std::endl;

	    }else{
		throw Ultracam_Error("Input failure on line " + Subs::str(n));
	    }
	}
    }
    freduce.close();
  
    std::cout << n << " lines read from " << file << "\n" << std::endl;
  
    // Now interpret input. This is very long and tedious with lots of
    // error checking which is why it is contained in this subroutine.
    typedef std::map<std::string,std::string>::const_iterator CI;
    CI p;
  
    // Version
    if(badInput(reduce, "version", p))
	throw Ultracam_Error("Version undefined. [option = \"version\"]");
  
    const std::string version= "04/12/2006"; // must match that in sreduce file
  
    if(p->second != version)
	throw Ultracam_Error("Version error. Expected " + version + " but found " + p->second +
			     "\nSee the ultracam documentation for the latest version of the sreduce file.");
  
    // Clobber log file or not?,
    if(badInput(reduce, "clobber", p))
	throw Ultracam_Error("Logfile clobber status undefined. [option = \"clobber\"]");
  
    // Open log file
    if(Subs::toupper(p->second) == "YES"){
	Sreduce::logger.open(logfile);
    }else if(Subs::toupper(p->second) == "NO"){
	Sreduce::logger.open(logfile,50,false);
    }else{
	throw Ultracam_Error("\"clobber\" must be either \"yes\" or \"no\".");
    }

    Sreduce::logger.logit("",false);

    Sreduce::logger.logit("Reduction file used", file);
    Sreduce::logger.logit("Version", version);
    Sreduce::logger.logit("Log file", logfile);

    Subs::Time tstart;
    tstart.set();
    Sreduce::logger.logit("Reduction started at", tstart, "(UT)");

    // Approach to take on encountering problems
    if(badInput(reduce,"abort_behaviour", p))
	throw Ultracam_Error("Behaviour on encountering difficulties undefined. [option = \"abort_behaviour\"]");
    
    if(Subs::toupper(p->second) == "FUSSY"){
	Sreduce::abort_behaviour = Sreduce::FUSSY;
    }else if(Subs::toupper(p->second) == "RELAXED"){
	Sreduce::abort_behaviour = Sreduce::RELAXED;
    }else if(Subs::toupper(p->second) == "VERY_RELAXED"){
	Sreduce::abort_behaviour = Sreduce::VERY_RELAXED;
    }else{
	throw Ultracam_Error("abort_behaviour must be one of 'fussy', 'relaxed' or 'very_relaxed'");
    }

    Sreduce::logger.logit("Behaviour on facing problems", p->second);

    // Aperture file
    if(badInput(reduce, "region_file", p))
	throw Ultracam_Error("Extraction region file undefined. [option = \"region_file\"]");
    
    Sreduce::region_master.rasc(p->second);
    Sreduce::logger.logit("Extraction region file", p->second);

    // Region reposition mode
    if(badInput(reduce, "region_reposition_mode", p))
	throw Ultracam_Error("Region reposition mode undefined. [option = \"region_reposition_mode\"]");

    if(Subs::toupper(p->second) == "STATIC"){
	Sreduce::region_reposition_mode = Sreduce::STATIC;
    }else if(Subs::toupper(p->second) == "INDIVIDUAL"){
	Sreduce::region_reposition_mode = Sreduce::INDIVIDUAL;
    }else if(Subs::toupper(p->second) == "REFERENCE"){
	Sreduce::region_reposition_mode = Sreduce::REFERENCE;
    }else{
	throw
	    Ultracam_Error("Invalid region reposition option. Must be one of:\n\n"
			   "static     -- positions static.\n"
			   "individual -- move each region separately.\n"
			   "reference  -- use reference star to measure the shift.\n"
		);
    }

    Sreduce::logger.logit("Extraction region reposition mode", p->second);

    std::string buff;
    std::istringstream istr(buff);

    // Get extra parameters for complex options
    if(Sreduce::region_reposition_mode == Sreduce::INDIVIDUAL ||
       Sreduce::region_reposition_mode == Sreduce::REFERENCE){
    
	if(badInput(reduce, "region_fwhm", p))
	    throw Ultracam_Error("Region gaussian fwhm undefined. [option = \"region_fwhm\"]");
    
	istr.str(p->second);
	istr >> Sreduce::region_fwhm;
	if(!istr) 
	    throw Ultracam_Error("Could not translate region_fwhm");
	istr.clear();

	if(Sreduce::region_fwhm <= 0)
	    throw Ultracam_Error("region_fwhm = " + Subs::str(Sreduce::region_fwhm) + " must be > 0");

	Sreduce::logger.logit("Region gaussian fwhm", Sreduce::region_fwhm, "pixels.");

	if(badInput(reduce, "region_max_shift", p))
	    throw Ultracam_Error("Region max shift undefined. [option = \"region_max_shift\"]");
    
	istr.str(p->second);
	istr >> Sreduce::region_max_shift;
	if(!istr) throw Ultracam_Error("Could not translate region_max_shift value");
	istr.clear();
	if(Sreduce::region_max_shift <= 0.) 
	    throw Ultracam_Error("region_max_shift = " + Subs::str(Sreduce::region_max_shift) + " must be > 0.");

	Sreduce::logger.logit("Region max shift", Sreduce::region_max_shift, "pixels.");


	if(badInput(reduce, "region_hwidth", p))
	    throw Ultracam_Error("Region median filter hald width undefined. [option = \"region_hwidth\"]");
    
	istr.str(p->second);
	istr >> Sreduce::region_hwidth;
	if(!istr) throw Ultracam_Error("Could not translate region_hwidth value");
	istr.clear();
	if(Sreduce::region_hwidth < 0) 
	    throw Ultracam_Error("region_hwidth = " + Subs::str(Sreduce::region_hwidth) + " must be >= 0.");

	Sreduce::logger.logit("Region median filter half-width", Sreduce::region_hwidth, "pixels.");

    }

    // Saturation warning
    if(badInput(reduce, "saturation", p))
	throw Ultracam_Error("No saturation levels set. [option = \"saturation\"]");
    istr.str(p->second);
    float sat;
    while(istr >> sat)
	Sreduce::saturation.push_back(sat);
    if(Sreduce::saturation.size() < Sreduce::region_master.size()) 
	throw Ultracam_Error("Only " + Subs::str(Sreduce::saturation.size()) + " saturation levels found compared to " + Subs::str(Sreduce::region_master.size()) + " CCDs in aperture file");
    istr.clear();
    Sreduce::logger.logit("Saturation levels", p->second);

    // Bias frame, if any
    if(badInput(reduce, "calibration_bias", p)){
	Sreduce::bias = false;
	Sreduce::logger.logit("No bias subtraction enabled.");
    }else{
	Sreduce::bias = true;
	Sreduce::bias_frame.read(p->second);
	Sreduce::logger.logit("Loaded bias frame", p->second);
    }

    // Dark frame, if any
    if(badInput(reduce, "calibration_dark", p)){
	Sreduce::dark = false;
	Sreduce::logger.logit("No dark subtraction enabled.");
    }else{
	Sreduce::dark = true;
	Sreduce::dark_frame.read(p->second);
	Sreduce::logger.logit("Loaded dark frame", p->second);
    }

    // Flat field frame, if any
    if(badInput(reduce, "calibration_flat", p)){
	Sreduce::flat = false;
	Sreduce::logger.logit("No flat fielding enabled.");
    }else{
	Sreduce::flat = true;
	Sreduce::flat_frame.read(p->second);
	Sreduce::logger.logit("Loaded flat field", p->second);
    }
  
    // Bad pixel frame, if any
    if(badInput(reduce, "calibration_bad", p)){
	Sreduce::dark = false;
	Sreduce::logger.logit("No bad pixel frame supplied.");
    }else{
	Sreduce::bad_pixel = true;
	Sreduce::bad_pixel_frame.read(p->second);
	Sreduce::logger.logit("Loaded bad pixel frame", p->second);
    }

    // Gain value or frame, if any
    if(badInput(reduce, "calibration_gain", p))
	throw Ultracam_Error("Gain frame or value undefined. [option = \"calibration_gain\"]");

    istr.str(p->second);
    istr >> Sreduce::gain;
    if(!istr){

	// Try instead to interpret as a file name
	Sreduce::gain_frame.read(p->second);

	Sreduce::logger.logit("Loaded gain frame", p->second);

	Sreduce::gain_const = false;

    }else{

	Sreduce::logger.logit("Using constant gain", Sreduce::gain, "electrons/ADU.");

	Sreduce::gain_const = true;
    
    }
    istr.clear();

    // Readout noise frame or value, if any
    if(badInput(reduce, "calibration_readout", p))
	throw Ultracam_Error("Readout noise frame or value undefined. [option = \"calibration_readout\"]");

    istr.str(p->second);
    istr >> Sreduce::readout;
    if(!istr){

	// OK, try instead to interpret as a file name
	Sreduce::readout_frame.read(p->second);
    
	Sreduce::logger.logit("Loaded readout frame", p->second);
    
	Sreduce::readout_const = false;

    }else{

	Sreduce::logger.logit("Using constant readout noise", Sreduce::readout, "RMS ADU.");

	Sreduce::readout_const = true;
    
    }
    istr.clear();

    // Coercion
    if(Sreduce::bias || Sreduce::dark || Sreduce::flat || !Sreduce::gain_const || !Sreduce::readout_const){
	if(badInput(reduce, "calibration_coerce", p))
	    throw Ultracam_Error("Coercion state undefined. [option = \"calibration_coerce\"]");
    
	if(Subs::toupper(p->second) == "YES"){
	    Sreduce::coerce = true;
	    Sreduce::logger.logit("Calibration frames will be coerced to match data.");

	}else if(Subs::toupper(p->second) == "NO"){
	    Sreduce::coerce = false;
	    Sreduce::logger.logit("Calibration frames will not be coerced to match data.");

	}else{
	    throw Ultracam_Error("\"calibration_coerce\" must be either \"yes\" or \"no\".");
	}
    }else{
	Sreduce::coerce = false;
    }

    // Frame accumulation
    if(badInput(reduce, "naccum", p))
	throw Ultracam_Error("Number of frames to accumulate per reduced spectrum undefined. [option = \"naccum\"]");

    istr.str(p->second);
    istr >> Sreduce::naccum;
    if(!istr) 
	throw Ultracam_Error("Could not translate naccum");
    istr.clear();
  
    if(Sreduce::naccum <= 0)
	throw Ultracam_Error("naccum = " + Subs::str(Sreduce::naccum) + " must be > 0");
  
    Sreduce::logger.logit("Frames/spectrum", Sreduce::naccum);

    // Thresholding
    if(badInput(reduce, "threshold", p))
	throw Ultracam_Error("thresholding undefined. [option = \"threshold\"]");
    
    if(Subs::toupper(p->second) == "YES"){
	Sreduce::threshold = true;
	Sreduce::logger.logit("A photon counting threshold will be applied.");

    }else if(Subs::toupper(p->second) == "NO"){
	Sreduce::threshold = false;
	Sreduce::logger.logit("No photon counting threshold will be applied.");
      
    }else{
	throw Ultracam_Error("\"threshold\" must be either \"yes\" or \"no\".");
    }

    if(Sreduce::threshold){

	// Photon threshold value
	if(badInput(reduce, "photon", p))
	    throw Ultracam_Error("Photon threshold value undefined. [option = \"photon\"]");
    
	istr.str(p->second);
	istr >> Sreduce::photon;
	if(!istr) throw Ultracam_Error("Could not translate photon value");
	istr.clear();
      
	if(Sreduce::photon <= 0.)
	    throw Ultracam_Error("photon = " + Subs::str(Sreduce::photon) + " must be > 0");

	Sreduce::logger.logit("Photon threshold", p->second);
      
    }

    // Sky fitting
    if(badInput(reduce, "sky_fit", p))
	throw Ultracam_Error("Need to specify whether you want to subtract the sky or not. [option = \"sky_fit\"]");
    
    if(Subs::toupper(p->second) == "YES"){
	Sreduce::sky_fit = true;
	Sreduce::logger.logit("Sky will be subtracted.");
	
    }else if(Subs::toupper(p->second) == "NO"){
	Sreduce::sky_fit = false;
	Sreduce::logger.logit("Sky will not be subtracted.");

    }else{
	throw Ultracam_Error("\"sky_fit\" must be either \"yes\" or \"no\".");
    }

    // Number of poly coefficients 
    if(badInput(reduce, "sky_npoly", p))
	throw Ultracam_Error("Number of poly coefficients for the sky undefined. [option = \"sky_npoly\"]");

    istr.str(p->second);
    istr >> Sreduce::sky_npoly;
    if(!istr) 
	throw Ultracam_Error("Could not translate sky_npoly");
    istr.clear();
  
    if(Sreduce::sky_npoly <= 0)
	throw Ultracam_Error("sky_npoly = " + Subs::str(Sreduce::sky_npoly) + " must be > 0");
  
    Sreduce::logger.logit("Npoly for sky", Sreduce::sky_npoly);

    // Sky clip value
    if(badInput(reduce, "sky_reject", p))
	throw Ultracam_Error("Sky rejection threshold undefined. [option = \"sky_reject\"]");
    
    istr.str(p->second);
    istr >> Sreduce::sky_reject;
    if(!istr) throw Ultracam_Error("Could not translate sky_reject value");
    istr.clear();

    if(Sreduce::sky_reject <= 0.)
	throw Ultracam_Error("sky_reject = " + Subs::str(Sreduce::sky_reject) + " must be > 0");

    Sreduce::logger.logit("Sky RMS reject threshold", p->second);

    // Plot device for spectra
    if(badInput(reduce, "spectrum_device", p))
	throw Ultracam_Error("Plot device for spectrum plots undefined. [option = \"spectrum_device\"]");

    Sreduce::spectrum_device = p->second;

    logit("Spectrum plot device", p->second);

    // Spectrum plot scaling
    if(badInput(reduce, "spectrum_scale_individual", p))
	throw Ultracam_Error("Need to specify whether you want to scale each spectrum individually. [option = \"spectrum_scale_individual\"]");
    
    if(Subs::toupper(p->second) == "YES"){
	Sreduce::spectrum_scale_individual = true;
	Sreduce::logger.logit("Spectra plots will be scaled individually.");
	
    }else if(Subs::toupper(p->second) == "NO"){
	Sreduce::spectrum_scale_individual = false;
	Sreduce::logger.logit("Spectra plots will be scaled over all.");

    }else{
	throw Ultracam_Error("\"spectrum_scale_individual\" must be either \"yes\" or \"no\".");
    }

    // Plot scaling method
    if(badInput(reduce, "spectrum_scale_method", p))
	throw Ultracam_Error("Scaling method for spectra. [option = \"spectrum_scale_method\"]");

    if(Subs::toupper(p->second) == "DIRECT"){
	Sreduce::spectrum_scale_method = Sreduce::DIRECT;
    }else if(Subs::toupper(p->second) == "AUTOMATIC"){
	Sreduce::spectrum_scale_method = Sreduce::AUTOMATIC;
    }else if(Subs::toupper(p->second) == "PERCENTILE"){
	Sreduce::spectrum_scale_method = Sreduce::PERCENTILE;
    }else{
	throw
	    Ultracam_Error("Invalid spectrum scaling optiob. Must be one of:\n\n"
			   "direct     -- user-defined fixed limits.\n"
			   "automatic  -- minimum to maximum.\n"
			   "percentile -- percentile range.\n"
		);
    }

    logit("Spectrum scale method", p->second);

    if(Sreduce::spectrum_scale_method == Sreduce::DIRECT){

	// Lower plot limit
	if(badInput(reduce, "spectrum_ylow", p))
	    throw Ultracam_Error("Lower direct limit for spectrum plots undefined. [option = \"spectrum_ylow\"]");
    
	istr.str(p->second);
	istr >> Sreduce::spectrum_ylow;
	if(!istr) throw Ultracam_Error("Could not translate spectrum_ylow value");
	istr.clear();

	Sreduce::logger.logit("Spectrum plot direct lower limit", p->second);

	// Upper plot limit
	if(badInput(reduce, "spectrum_yhigh", p))
	    throw Ultracam_Error("Upper direct limit for spectrum plots undefined. [option = \"spectrum_yhigh\"]");
    
	istr.str(p->second);
	istr >> Sreduce::spectrum_yhigh;
	if(!istr) throw Ultracam_Error("Could not translate spectrum_yhigh value");
	istr.clear();

	Sreduce::logger.logit("Spectrum plot direct upper limit", p->second);

    }else if(Sreduce::spectrum_scale_method == Sreduce::PERCENTILE){

	// Lower percentile limit
	if(badInput(reduce, "spectrum_plow", p))
	    throw Ultracam_Error("Lower percentile limit for spectrum plots undefined. [option = \"spectrum_plow\"]");
    
	istr.str(p->second);
	istr >> Sreduce::spectrum_plow;
	if(!istr) throw Ultracam_Error("Could not translate spectrum_plow value");
	istr.clear();

	Sreduce::logger.logit("Spectrum plot percentile lower limit", p->second);

	// Upper percentile limit
	if(badInput(reduce, "spectrum_phigh", p))
	    throw Ultracam_Error("Upper percentile limit for spectrum plots undefined. [option = \"spectrum_phigh\"]");
    
	istr.str(p->second);
	istr >> Sreduce::spectrum_phigh;
	if(!istr) throw Ultracam_Error("Could not translate spectrum_phigh value");
	istr.clear();

	Sreduce::logger.logit("Spectrum plot percentile upper limit", p->second);
    }

    // Plot device for trail
    if(badInput(reduce, "trail_device", p))
	throw Ultracam_Error("Plot device for trailed spectra plots undefined. [option = \"trail_device\"]");

    Sreduce::trail_device = p->second;

    logit("Trail plot device", p->second);

    // Trail number of slots at start
    if(badInput(reduce, "trail_start", p))
	throw Ultracam_Error("Start number of slots in the trail undefined. [option = \"trail_start\"]");

    istr.str(p->second);
    istr >> Sreduce::trail_start;
    if(!istr) 
	throw Ultracam_Error("Could not translate trail_start");
    istr.clear();
  
    if(Sreduce::trail_start <= 0)
	throw Ultracam_Error("trail_start = " + Subs::str(Sreduce::trail_start) + " must be > 0");
  
    Sreduce::logger.logit("Start number of slots in trail", Sreduce::trail_start);

    // Plot scaling method
    if(badInput(reduce, "trail_scale_method", p))
	throw Ultracam_Error("Scaling method for spectra. [option = \"trail_scale_method\"]");

    if(Subs::toupper(p->second) == "DIRECT"){
	Sreduce::trail_scale_method = Sreduce::DIRECT;
    }else if(Subs::toupper(p->second) == "AUTOMATIC"){
	Sreduce::trail_scale_method = Sreduce::AUTOMATIC;
    }else if(Subs::toupper(p->second) == "PERCENTILE"){
	Sreduce::trail_scale_method = Sreduce::PERCENTILE;
    }else{
	throw
	    Ultracam_Error("Invalid trail scaling optiob. Must be one of:\n\n"
			   "direct     -- user-defined fixed limits.\n"
			   "automatic  -- minimum to maximum.\n"
			   "percentile -- percentile range.\n"
		);
    }

    logit("Trail scale method", p->second);

    if(Sreduce::trail_scale_method == Sreduce::DIRECT){

	// Lower plot limit
	if(badInput(reduce, "trail_ilow", p))
	    throw Ultracam_Error("Lower direct limit for trail plots undefined. [option = \"trail_ilow\"]");
    
	istr.str(p->second);
	istr >> Sreduce::trail_ilow;
	if(!istr) throw Ultracam_Error("Could not translate trail_ilow value");
	istr.clear();

	Sreduce::logger.logit("Trail plot direct lower limit", p->second);

	// Upper plot limit
	if(badInput(reduce, "trail_ihigh", p))
	    throw Ultracam_Error("Upper direct limit for trail plots undefined. [option = \"trail_ihigh\"]");
    
	istr.str(p->second);
	istr >> Sreduce::trail_ihigh;
	if(!istr) throw Ultracam_Error("Could not translate trail_ihigh value");
	istr.clear();

	Sreduce::logger.logit("Trail plot direct upper limit", p->second);

    }else if(Sreduce::trail_scale_method == Sreduce::PERCENTILE){

	// Lower percentile limit
	if(badInput(reduce, "trail_plow", p))
	    throw Ultracam_Error("Lower percentile limit for trail plots undefined. [option = \"trail_plow\"]");
    
	istr.str(p->second);
	istr >> Sreduce::trail_plow;
	if(!istr) throw Ultracam_Error("Could not translate trail_plow value");
	istr.clear();

	Sreduce::logger.logit("Trail plot percentile lower limit", p->second);

	// Upper percentile limit
	if(badInput(reduce, "trail_phigh", p))
	    throw Ultracam_Error("Upper percentile limit for trail plots undefined. [option = \"trail_phigh\"]");
    
	istr.str(p->second);
	istr >> Sreduce::trail_phigh;
	if(!istr) throw Ultracam_Error("Could not translate trail_phigh value");
	istr.clear();

	Sreduce::logger.logit("Trail plot percentile upper limit", p->second);
    }

    // Hard copy device for trail
    if(badInput(reduce, "hard_device", p))
	throw Ultracam_Error("Plot device for hard copy of trailed spectra plots undefined. [option = \"hard_device\"]");

    Sreduce::hard_device = p->second;

    logit("Trail hard copy device", p->second);

    // Terminal output mode
    if(badInput(reduce, "terminal_output", p))
	throw Input_Error("Terminal output mode undefined. [option = \"terminal_output\"]");
    
    // This gets checked quite often so convert to integer to speed 
    // comparisons later on

    if(Subs::toupper(p->second) == "NONE"){
	Sreduce::terminal_output = Sreduce::NONE;
    }else if(Subs::toupper(p->second) == "LITTLE"){
	Sreduce::terminal_output = Sreduce::LITTLE;
    }else if(Subs::toupper(p->second) == "MEDIUM"){
	Sreduce::terminal_output = Sreduce::MEDIUM;
    }else if(Subs::toupper(p->second) == "FULL"){
	Sreduce::terminal_output = Sreduce::FULL;
    }else{
	throw Input_Error("terminal_output must be one of 'none', 'little', 'medium' or 'full'");
    }

    logit("Terminal output", p->second);

}


