#include <cstdlib>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include "trm_subs.h"
#include "trm_time.h"
#include "trm_aperture.h"
#include "trm_mccd.h"
#include "trm_frame.h"
#include "trm_ultracam.h"
#include "trm_reduce.h"

// External variables for communicating with reduce. See 'reduce.cc' for full list
// of meanings.

namespace Reduce {


  extern bool cr_to_start;                     
  extern Ultracam::Logger logger;
  extern ABORT_BEHAVIOUR abort_behaviour;      
  extern bool cosmic_clean;
  extern float cosmic_height;
  extern float cosmic_ratio;
  extern std::vector<float> pepper;
  extern std::vector<float> saturation;
  extern TERM_OUT terminal_output;
  extern bool gain_const;                              
  extern float gain;                                   
  extern Ultracam::Frame gain_frame;                             
  extern bool readout_const;                           
  extern float readout;                                
  extern Ultracam::Frame readout_frame;                          
  extern bool coerce;                                  
                     
  // Apertures
  extern Ultracam::Maperture aperture_master;            
  extern APERTURE_REPOSITION_MODE aperture_reposition_mode;
  extern bool  aperture_positions_stable;;
  extern int   aperture_search_half_width;             
  extern float aperture_search_fwhm;                   
  extern float aperture_search_max_shift;              
  extern int   aperture_tweak_half_width;              
  extern float aperture_tweak_fwhm;                    
  extern float aperture_tweak_max_shift;     
          
  extern bool  aperture_twopass;
  extern float aperture_twopass_counts;
  extern int   aperture_twopass_npoly;
  extern float aperture_twopass_sigma;

  // Extraction and profiles
  extern std::map<int,Reduce::Extraction> extraction_control;
  extern std::vector<float> star_radius;
  extern PROFILE_FIT_METHOD profile_fit_method;
  extern PROFILE_FIT_METHOD extraction_weights;
  extern float profile_fit_fwhm;                
  extern int profile_fit_hwidth;                
  extern bool profile_fit_symm;                 
  extern float profile_fit_beta;                
  extern float profile_fit_sigma;              
  extern SKY_METHOD sky_method;                        
  extern SKY_ERROR  sky_error;                         
  extern float sky_thresh;                               

  // Calibration
  extern bool bias;                                    
  extern Ultracam::Frame bias_frame;                             
  extern bool dark;                                    
  extern Ultracam::Frame dark_frame;                             
  extern bool flat;                                    
  extern Ultracam::Frame flat_frame;                             
  extern bool bad_pixel;                                    
  extern Ultracam::Frame bad_pixel_frame;                             

  // Lightcurves
  extern std::string image_device;                          
  extern float  lightcurve_frac;                  
  extern std::string lightcurve_device;                         
  extern float  lightcurve_max_xrange;                     
  extern X_UNITS lightcurve_xunits;
  extern float  lightcurve_extend_xrange;                  
  extern bool   lightcurve_linear;                  
  extern bool   lightcurve_yrange_fixed;                   
  extern bool   lightcurve_invert;                   
  extern float  lightcurve_y1;                       
  extern float  lightcurve_y2;                       
  extern float  lightcurve_extend_yrange;       
  extern std::vector<Laps> lightcurve_targ;    

  // Positions
  extern bool  position_plot;
  extern float position_frac;
  extern std::vector<Paps> position_targ;    
  extern bool  position_x_yrange_fixed;                       
  extern float position_x_y1;                     
  extern float position_x_y2;                     
  extern bool  position_y_yrange_fixed;                       
  extern float position_y_y1;                     
  extern float position_y_y2;                     
  extern float position_extend_yrange;

  // Transmission
  extern bool  transmission_plot; 
  extern float transmission_frac;
  extern float transmission_ymax;
  extern std::vector<Taps> transmission_targ;

  // Seeing
  extern bool seeing_plot;
  extern float seeing_frac;
  extern std::vector<Faps> seeing_targ;
  extern float seeing_scale; 
  extern float seeing_ymax; 
  extern float seeing_extend_yrange;  
               
};

/**
 * read_reduce_file is a function dedicated to reading the data that controls
 * the ultracam pipeline reduction. It is basically a long series of inputs and tests
 * for validity, with exceptions thrown if errors are encountered. It reads the file
 * data into a series of external variables also defined in reduce.
 *
 * These are all defined within a namespace called "Reduce" to aid identification and
 * for safety.
 *
 * \param file     filename with data to control the operation of reduce
 * \param logfile  file to store log of output from reduce.
 */

void Ultracam::read_reduce_file(const std::string& file, const std::string& logfile){

  using Ultracam::File_Open_Error;
  using Ultracam::Input_Error;

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
	  throw Input_Error("Line " + Subs::str(n) + " starting: " + option + " is invalid");
	
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
	   option == "extraction_control" ||
	   option == "transmission_targ" ||
	   option == "seeing_targ"){

	  // special cases which can be repeated multiple times
	  special.insert(std::make_pair(option, value));

	}else{

	  // the normal case
	  pssins = reduce.insert(std::make_pair(option, value));

	  if(!pssins.second)
	    throw Input_Error("Option = " + option + " is a repeat!");

	}

      }else if(freduce.eof()){
	std::cout << "End of file reached." << std::endl;

      }else{
	throw Input_Error("Input failure on line " + Subs::str(n));
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
    throw Input_Error("Version undefined. [option = \"version\"]");
  
  const std::string version= "19/12/2005"; // must match that in reduce file
  
  if(p->second != version)
    throw Input_Error("Version error. Expected " + version + " but found " + p->second +
		      "\nSee the ultracam documentation for the latest version of the reduce file.");
  
  // Carriage return to start or not.
  if(badInput(reduce, "cr_to_start", p))
    throw Input_Error("Start input undefined. [option = \"cr_to_start\"]");
  
  if(Subs::toupper(p->second) == "YES"){
    Reduce::cr_to_start = true;
  }else if(Subs::toupper(p->second) == "NO"){
    Reduce::cr_to_start = false;
  }else{
    throw Input_Error("\"cr_to_start\" must be either"
		      " \"yes\" or \"no\".");
  }

  // Clobber log file or not?,
  if(badInput(reduce, "clobber", p))
    throw Input_Error("Logfile clobber status undefined. [option = \"clobber\"]");
  
  // Open log file
  if(Subs::toupper(p->second) == "YES"){
    Reduce::logger.open(logfile);
  }else if(Subs::toupper(p->second) == "NO"){
    Reduce::logger.open(logfile,50,false);
  }else{
    throw Input_Error("\"clobber\" must be either \"yes\" or \"no\".");
  }

  Reduce::logger.logit("",false);

  Reduce::logger.logit("Reduction file used", file);
  Reduce::logger.logit("Version", version);
  Reduce::logger.logit("Log file", logfile);

  Subs::Time tstart;
  tstart.set();
  Reduce::logger.logit("Reduction started at", tstart, "(UT)");

  // Approach to take on encountering problems
  if(badInput(reduce,"abort_behaviour", p))
    throw Input_Error("Behaviour on encountering difficulties undefined. [option = \"abort_behaviour\"]");
    
  if(Subs::toupper(p->second) == "FUSSY"){
    Reduce::abort_behaviour = Reduce::FUSSY;
  }else if(Subs::toupper(p->second) == "RELAXED"){
    Reduce::abort_behaviour = Reduce::RELAXED;
  }else{
    throw Input_Error("abort_behaviour must be one of 'fussy' or 'relaxed'");
  }

  Reduce::logger.logit("Behaviour on facing problems", p->second);

  // Cosmic rays
  if(badInput(reduce, "cosmic_clean", p))
    throw Input_Error("Cosmic ray cleaning state undefined. [option = \"cosmic_clean\"]");
  
  if(Subs::toupper(p->second) == "YES"){
    Reduce::cosmic_clean = true;
    Reduce::logger.logit("Cosmic ray cleaning enabled.");
    
  }else if(Subs::toupper(p->second) == "NO"){
    Reduce::cosmic_clean = false;
    Reduce::logger.logit("Cosmic ray cleaning disabled.");
    
  }else{
    throw Input_Error("\"cosmic_clean\" must be either \"yes\" or \"no\".");
  }

  std::string buff;
  std::istringstream istr(buff);
  if(Reduce::cosmic_clean){

    if(badInput(reduce, "cosmic_height", p))
      throw Input_Error("Cosmic ray height above surroundings undefined. [option = \"cosmic_height\"]");

    istr.str(p->second);
    istr >> Reduce::cosmic_height;
    if(!istr) 
      throw Input_Error("Could not translate cosmic_height value");
    istr.clear();

    if(Reduce::cosmic_height <= 0.f) 
      throw Input_Error("cosmic_height = " + Subs::str(Reduce::cosmic_height) + " must be > 0");
    Reduce::logger.logit("Cosmic ray height above surroundings", Reduce::cosmic_height);


    if(badInput(reduce, "cosmic_ratio", p))
      throw Input_Error("Cosmic ray ratio relative to surroundings undefined. [option = \"cosmic_ratio\"]");
    istr.str(p->second);
    istr >> Reduce::cosmic_ratio;
    if(!istr) 
      throw Input_Error("Could not translate cosmic_ratio value");
    istr.clear();

    if(Reduce::cosmic_ratio <= 1.f) 
      throw Input_Error("cosmic_ratio = " + Subs::str(Reduce::cosmic_height) + " must be > 1");
    Reduce::logger.logit("Cosmic ray ratio relative to surroundings", Reduce::cosmic_ratio);

  }

  // Extraction control parameters
  std::string message;
  typedef std::multimap<std::string,std::string>::iterator MI;
  std::pair<MI,MI> mmpr;
  size_t nccd;

  // extraction control lines. These have structure:
  // nccd [static/variable] [normal/optimal] star_scale inner_sky_scale min_inner_sky max_inner_sky outer_sky_scale min_outer_sky max_outer_sky
  bool profile_fits_needed = false;
  mmpr = special.equal_range("extraction_control");
  for(MI mmp=mmpr.first; mmp != mmpr.second; mmp++){
    
    if(mmp->second == "")
      throw Input_Error("No parameters specified for extraction control. [option = \"extraction_control\"]");
    
    // Translate the extraction control parameters
    istr.str(mmp->second);
    float star_scale, inner_sky_scale, outer_sky_scale;
    float star_min, star_max, inner_sky_min, inner_sky_max, outer_sky_min, outer_sky_max;
    std::string aptype, extype;
    istr >> nccd >> aptype >> extype >> star_scale >> star_min >> star_max >> inner_sky_scale >> inner_sky_min >> inner_sky_max >> outer_sky_scale >> outer_sky_min >> outer_sky_max;
    if(!istr) throw Input_Error("Could not translate extraction_control parameters: " + mmp->second);
    istr.clear();
    
    // Run a few idiot checks
    if(nccd  <  1)       throw Input_Error("extraction_control: CCD number must be > 0");
    aptype = Subs::toupper(aptype);
    Reduce::APERTURE_TYPE aperture_type;
    if(aptype == "FIXED"){
      aperture_type = Reduce::FIXED;
    }else if(aptype == "VARIABLE"){
      profile_fits_needed = true;
      aperture_type = Reduce::VARIABLE;
      if(star_scale <= 0.) 
	throw Input_Error("extraction_control: star scale factor must be > 0.");
      if(star_min > star_max) 
	throw Input_Error("extraction_control: star minimum radius must be <= maximum radius.");
      if(outer_sky_scale < inner_sky_scale) 
	throw Input_Error("extraction_control: outer sky scale factor must be >= inner sky scale factor");
      if(inner_sky_min < 0.) 
	throw Input_Error("extraction_control: inner sky minimum radius must be >= 0.");
      if(inner_sky_min > inner_sky_max) 
	throw Input_Error("extraction_control: inner sky maximum radius must be >= inner sky minimum radius.");
      if(outer_sky_max <= inner_sky_max) 
	throw Input_Error("extraction_control: outer sky maximum radius must be > inner sky maximum radius.");
      if(outer_sky_min <= inner_sky_min) 
	throw Input_Error("extraction_control: outer sky minimum radius must be > inner sky minimum radius.");
      if(outer_sky_max < outer_sky_min) 
	throw Input_Error("extraction_control: outer sky maximum radius must be >= outer sky minimum radius.");
    }else{
      throw Input_Error("extraction_control: aperture type must be either 'fixed' or 'variable'");
    }
    
    extype = Subs::toupper(extype);
    Reduce::EXTRACTION_METHOD extraction_method;
    if(extype == "NORMAL"){
      extraction_method = Reduce::NORMAL;
    }else if(extype == "OPTIMAL"){
      profile_fits_needed = true;
      extraction_method = Reduce::OPTIMAL;
    }else{
      throw Input_Error("extraction_control: extraction method must be either 'normal' or 'optimal'");
    }
    
    nccd--;
    
    // Guard against repeats of the same CCD and then store
    if(Reduce::extraction_control.find(nccd) == Reduce::extraction_control.end())
      Reduce::extraction_control[nccd] = Reduce::Extraction(aperture_type, extraction_method, 
							    star_scale,      star_min,      star_max,
							    inner_sky_scale, inner_sky_min, inner_sky_max,
							    outer_sky_scale, outer_sky_min, outer_sky_max);
    
    else
      throw Input_Error("extraction_control: at least one CCD entry has been repeated.");
    
    Reduce::logger.logit("Extraction control", mmp->second);
  }


  if(badInput(reduce, "star_aperture_radii", p)) {
    Reduce::logger.logit("Aperture radii", "radii taken from extraction lines");
  }else{
    istr.str(p->second);
    float r;
    while(istr >> r)
      Reduce::star_radius.push_back(r);
    if(Reduce::star_radius.size() == 0) 
      throw Input_Error("No radii found in star_aperture_radii line");
    istr.clear();
    Reduce::logger.logit("Aperture radii", p->second);
  }

  // Aperture file
  if(badInput(reduce, "aperture_file", p))
    throw Input_Error("Aperture file undefined. [option = \"aperture_file\"]");
    
  Reduce::aperture_master.rasc(p->second);

  // Check that there are apertures available for each extraction control entry
  for(std::map<int,Reduce::Extraction>::const_iterator cit= Reduce::extraction_control.begin(); cit!=Reduce::extraction_control.end(); cit++)
    if(Reduce::aperture_master[cit->first].size() == 0)
      throw Input_Error("There are no apertures defined for CCD " + Subs::str(cit->first+1) + " although an extraction_control line has been defined");
      
  if(Reduce::extraction_control.find(nccd) == Reduce::extraction_control.end())

  Reduce::logger.logit("Aperture file", p->second);

  // Aperture reposition mode
  if(badInput(reduce, "aperture_reposition_mode", p))
    throw Input_Error("Aperture reposition mode undefined. [option = \"aperture_reposition_mode\"]");

  if(Subs::toupper(p->second) == "STATIC"){
    Reduce::aperture_reposition_mode = Reduce::STATIC;
  }else if(Subs::toupper(p->second) == "INDIVIDUAL"){
    Reduce::aperture_reposition_mode = Reduce::INDIVIDUAL;
  }else if(Subs::toupper(p->second) == "INDIVIDUAL_PLUS_TWEAK"){
    Reduce::aperture_reposition_mode = Reduce::INDIVIDUAL_PLUS_TWEAK;
  }else if(Subs::toupper(p->second) == "REFERENCE_PLUS_TWEAK"){
    Reduce::aperture_reposition_mode = Reduce::REFERENCE_PLUS_TWEAK;
  }else{
    throw
      Input_Error("Invalid aperture reposition option. Must be one of:\n\n"
		  "static                -- positions static.\n"
		  "individual            -- move each aperture separately.\n"
		  "individual_plus_tweak -- move each aperture separately then tweak offset apertures.\n"
		  "reference_plus_tweak  -- use reference stars to provide first estimate of shift.\n"
		  );
  }

  Reduce::logger.logit("Aperture reposition mode", p->second);

  if(badInput(reduce, "aperture_positions_stable", p))
    throw Input_Error("Stability of apertures undefined. [option = \"aperture_positions_stable\"]");

  if(Subs::toupper(p->second) == "YES"){
    Reduce::aperture_positions_stable = true;
    Reduce::logger.logit("Aperture positions defined to be stable.");
    
  }else if(Subs::toupper(p->second) == "NO"){
    Reduce::aperture_positions_stable = false;
    Reduce::logger.logit("Aperture positions defined to be erratic.");
    
  }else{
    throw Input_Error("\"aperture_positions_stable\" must be either \"yes\" or \"no\".");
  }

  // Get extra parameters for complex options
  if(Reduce::aperture_reposition_mode == Reduce::INDIVIDUAL ||
     Reduce::aperture_reposition_mode == Reduce::INDIVIDUAL_PLUS_TWEAK || 
     Reduce::aperture_reposition_mode == Reduce::REFERENCE_PLUS_TWEAK || 
     Reduce::cosmic_clean){
    
    if(badInput(reduce, "aperture_search_half_width", p))
      throw Input_Error("Aperture search half width undefined. [option = \"aperture_search_half_width\"]");
    
    istr.str(p->second);
    istr >> Reduce::aperture_search_half_width;
    if(!istr) 
      throw Input_Error("Could not translate aperture_search_half_width value");
    istr.clear();

    if(Reduce::aperture_search_half_width <= 0)
      throw Input_Error("aperture_search_half_width = " + Subs::str(Reduce::aperture_search_half_width) + " must be > 0");

    Reduce::logger.logit("Aperture search half width", Reduce::aperture_search_half_width, "pixels.");
  }

  if(Reduce::aperture_reposition_mode == Reduce::INDIVIDUAL ||
     Reduce::aperture_reposition_mode == Reduce::INDIVIDUAL_PLUS_TWEAK ||
     Reduce::aperture_reposition_mode == Reduce::REFERENCE_PLUS_TWEAK){
    if(badInput(reduce, "aperture_search_fwhm", p))
      throw Input_Error("Aperture search fwhm undefined. [option = \"aperture_search_fwhm\"]");
    
    istr.str(p->second);
    istr >> Reduce::aperture_search_fwhm;
    if(!istr) throw Input_Error("Could not translate aperture_search_fwhm value");
    istr.clear();
    if(Reduce::aperture_search_fwhm <= 0.)
      throw Input_Error("aperture_search_fwhm = " + Subs::str(Reduce::aperture_search_fwhm) + " must be > 0.");

    Reduce::logger.logit("Aperture search fwhm", Reduce::aperture_search_fwhm, "pixels.");

    if(badInput(reduce, "aperture_search_max_shift", p))
      throw Input_Error("Aperture search max shift undefined. [option = \"aperture_search_max_shift\"]");
    
    istr.str(p->second);
    istr >> Reduce::aperture_search_max_shift;
    if(!istr) throw Input_Error("Could not translate aperture_search_max_shift value");
    istr.clear();
    if(Reduce::aperture_search_max_shift <= 0.) 
      throw Input_Error("aperture_search_max_shift = " + Subs::str(Reduce::aperture_search_max_shift) + " must be > 0.");

    Reduce::logger.logit("Aperture search max shift", Reduce::aperture_search_max_shift, "pixels.");

    // Special extras in these cases
    if(Reduce::aperture_reposition_mode == Reduce::INDIVIDUAL_PLUS_TWEAK ||
       Reduce::aperture_reposition_mode == Reduce::REFERENCE_PLUS_TWEAK){

      if(badInput(reduce, "aperture_tweak_half_width", p))
	throw Input_Error("Aperture tweak half width undefined. [option = \"aperture_tweak_half_width\"]");
      
      istr.str(p->second);
      istr >> Reduce::aperture_tweak_half_width;
      if(!istr) throw Input_Error("Could not translate aperture_tweak_half_width value");
      istr.clear();
 
      if(Reduce::aperture_tweak_half_width <= 0) 
	throw Input_Error("aperture_tweak_half_width = " + Subs::str(Reduce::aperture_tweak_half_width) + " must be > 0");
      
      Reduce::logger.logit("Aperture tweak half width", Reduce::aperture_tweak_half_width, "pixels.");
      
      if(badInput(reduce, "aperture_tweak_fwhm", p))
	throw Input_Error("Aperture tweak fwhm undefined. [option = \"aperture_tweak_fwhm\"]");
      
      istr.str(p->second);
      istr >> Reduce::aperture_tweak_fwhm;
      if(!istr) throw Input_Error("Could not translate aperture_tweak_fwhm value");
      istr.clear();

      if(Reduce::aperture_tweak_fwhm <= 0.)
	throw Input_Error("aperture_tweak_fwhm = " + Subs::str(Reduce::aperture_tweak_fwhm) + " must be > 0.");
      
      Reduce::logger.logit("Aperture tweak fwhm", Reduce::aperture_tweak_fwhm, "pixels.");
      
    }

    if(Reduce::aperture_reposition_mode == Reduce::INDIVIDUAL_PLUS_TWEAK ||
       Reduce::aperture_reposition_mode == Reduce::REFERENCE_PLUS_TWEAK || profile_fits_needed){

      if(badInput(reduce, "aperture_tweak_max_shift", p))
	throw Input_Error("Aperture tweak max shift undefined. [option = \"aperture_tweak_max_shift\"]");
      
      istr.str(p->second);
      istr >> Reduce::aperture_tweak_max_shift;
      if(!istr) throw Input_Error("Could not translate aperture_tweak_max_shift value");
      istr.clear();

      if(Reduce::aperture_tweak_max_shift <= 0.) 
	throw Input_Error("aperture_tweak_max_shift = " + Subs::str(Reduce::aperture_tweak_max_shift) + " must be > 0.");
      
      Reduce::logger.logit("Aperture tweak max shift", Reduce::aperture_tweak_max_shift, "pixels.");

    }
  }

  // One or two passes. With two passes, positions will first be measured and then fitted with
  // polynomials. With one pass, positions will be determined on the fly.
  if(badInput(reduce, "aperture_twopass", p))
    throw Input_Error("Twopass mode is undefined. [option = \"aperture_twopass\"]");

  if(Subs::toupper(p->second) == "YES"){
    if(Reduce::aperture_reposition_mode != Reduce::REFERENCE_PLUS_TWEAK)
      throw Input_Error("Two pass position determination only supported if aperture_reposition_mode == reference_plus_tweak");
    Reduce::aperture_twopass = true;
    Reduce::logger.logit("Two passes used for aperture positions.");
    
  }else if(Subs::toupper(p->second) == "NO"){
    Reduce::aperture_twopass = false;
    Reduce::logger.logit("Aperture positions computed on the fly.");
    
  }else{
    throw Input_Error("\"aperture_position_twopass\" must be either \"yes\" or \"no\".");
  }

  if(Reduce::aperture_twopass){
    
    // Aperture must have a minimum number of counts for its poisition to count as valid in two pass mode
    if(badInput(reduce, "aperture_twopass_counts", p))
      throw Input_Error("Minimum number of counts for valid apertures in two pass mode undefined. [option = \"aperture_twopass_counts\"]");
    
    istr.str(p->second);
    istr >> Reduce::aperture_twopass_counts;
    if(!istr) throw Input_Error("Could not translate aperture_twopass_counts value");
    istr.clear();
    
    Reduce::logger.logit("Minimum counts in two pass mode", Reduce::aperture_twopass_counts);

    // Number of poly coefficients
    if(badInput(reduce, "aperture_twopass_npoly", p))
      throw Input_Error("Number of poly coefficients in two pass mode undefined. [option = \"aperture_twopass_npoly\"]");
    
    istr.str(p->second);
    istr >> Reduce::aperture_twopass_npoly;
    if(!istr) throw Input_Error("Could not translate aperture_twopass_npoly value");
    istr.clear();
    
    if(Reduce::aperture_twopass_npoly <= 0) 
      throw Input_Error("aperture_twopass_npoly = " + Subs::str(Reduce::aperture_twopass_npoly) + " must be > 0");
    
    Reduce::logger.logit("Number of poly coefficients for two pass mode", Reduce::aperture_twopass_npoly);

    // Rejection threshold
    if(badInput(reduce, "aperture_twopass_sigma", p))
      throw Input_Error("Rejection threshold in two pass mode undefined. [option = \"aperture_twopass_sigma\"]");

    istr.str(p->second);
    istr >> Reduce::aperture_twopass_sigma;
    if(!istr) throw Input_Error("Could not translate aperture_twopass_sigma value");
    istr.clear();
    
    if(Reduce::aperture_twopass_sigma <= 1.f) 
      throw Input_Error("aperture_twopass_sigma = " + Subs::str(Reduce::aperture_twopass_sigma) + " must be > 1");
    
    Reduce::logger.logit("Rejection threshold for two pass mode", Reduce::aperture_twopass_sigma, " sigma.");
    
  }
   
  if(profile_fits_needed){
    
    // Method used to fit profiles
    if(badInput(reduce, "profile_fit_method", p))
      throw Input_Error("Profile fitting method undefined. [option = \"profile_fit_method\"]");
    
    if(Subs::toupper(p->second) == "GAUSSIAN"){
      Reduce::profile_fit_method = Reduce::GAUSSIAN;
    }else if(Subs::toupper(p->second) == "MOFFAT"){
      Reduce::profile_fit_method = Reduce::MOFFAT;
    }else{
      throw
	Input_Error("Invalid profile fitting method. Must be one of:\n\n"
		    "gaussian       -- 2D Gaussian.\n"
		    "moffat         -- Moffat (generalised gaussian)\n"
		    );
    }
    
    Reduce::logger.logit("Profile fitting method", p->second);

    // Method used to weight profiles
    if(badInput(reduce, "extraction_weights", p) || Reduce::profile_fit_method == Reduce::GAUSSIAN){
      Reduce::logger.logit("Extraction weights undefined [option = \"extraction_weights\"] or gaussian fitting being used. Will assume same as fit method.");
      Reduce::extraction_weights = Reduce::profile_fit_method;
    }else{    
      if(Subs::toupper(p->second) == "GAUSSIAN"){
	Reduce::extraction_weights = Reduce::GAUSSIAN;
      }else if(Subs::toupper(p->second) == "MOFFAT"){
	Reduce::extraction_weights = Reduce::MOFFAT;
      }else{
	throw
	  Input_Error("Invalid extraction weights. Must be one of:\n\n"
		      "gaussian       -- 2D Gaussian.\n"
		      "moffat         -- Moffat (generalised gaussian)\n"
		      );
      }
      Reduce::logger.logit("Extraction weights", p->second);
    }

    // Profile fit fwhm
    if(badInput(reduce, "profile_fit_fwhm", p))
      throw Input_Error("Default value of FWHM for profile fits undefined. [option = \"profile_fit_fwhm\"]");
    
    istr.str(p->second);
    istr >> Reduce::profile_fit_fwhm;
    if(!istr) throw Input_Error("Could not translate profile_fit_fwhm value");
    istr.clear();
    
    if(Reduce::profile_fit_fwhm <= 0.) 
      throw Input_Error("profile_fit_fwhm = " + Subs::str(Reduce::profile_fit_fwhm) + " must be > 0");
    
    Reduce::logger.logit("Default value of FWHM for profile fits", Reduce::profile_fit_fwhm);    

    // profile fit half-width
    if(badInput(reduce, "profile_fit_hwidth", p))
      throw Input_Error("Profile fit half-width undefined. [option = \"profile_fit_hwidth\"]");
    
    istr.str(p->second);
    istr >> Reduce::profile_fit_hwidth;
    if(!istr) throw Input_Error("Could not translate profile_fit_hwidth value");
    istr.clear();
    
    if(Reduce::profile_fit_hwidth < 1) 
      throw Input_Error("profile_fit_hwidth = " + Subs::str(Reduce::profile_fit_hwidth) + " must be > 0");
    
    Reduce::logger.logit("Profile fit half-width", Reduce::profile_fit_hwidth);    

    // profile fit symmetric or not
    if(badInput(reduce, "profile_fit_symm", p))
      throw Input_Error("Symmetry or not of profiles undefined. [option = \"profile_fit_symm\"]");
    
    if(Subs::toupper(p->second) == "YES"){
      Reduce::profile_fit_symm = true;
      Reduce::logger.logit("Symmetric profile fits used.");
      
    }else if(Subs::toupper(p->second) == "NO"){
      Reduce::profile_fit_symm = false;
      Reduce::logger.logit("Elliptical profile fits used.");
      
    }else{
      throw Input_Error("\"profile_fit_symm\" must be either \"yes\" or \"no\".");
    }

    // extra parameter for Moffat fits
    if(Reduce::profile_fit_method == Reduce::MOFFAT){

      // profile fit beta
      if(badInput(reduce, "profile_fit_beta", p))
	throw Input_Error("Default vakue of beta exponent for Moffat fits undefined. [option = \"profile_fit_beta\"]");
      
      istr.str(p->second);
      istr >> Reduce::profile_fit_beta;
      if(!istr) throw Input_Error("Could not translate profile_fit_beta value");
      istr.clear();
      
      if(Reduce::profile_fit_beta <= 1.) 
	throw Input_Error("profile_fit_beta = " + Subs::str(Reduce::profile_fit_beta) + " must be > 1");
      
      Reduce::logger.logit("Default value of beta exponent for moffat fits", Reduce::profile_fit_beta);    

    }

    // profile fit sigma rejection threshold
    if(badInput(reduce, "profile_fit_sigma", p))
      throw Input_Error("Sigma rejection threshold for profile fits undefined. [option = \"profile_fit_sigma\"]");
    
    istr.str(p->second);
    istr >> Reduce::profile_fit_sigma;
    if(!istr) throw Input_Error("Could not translate profile_fit_sigma value");
    istr.clear();
    
    if(Reduce::profile_fit_sigma <= 0.) 
      throw Input_Error("profile_fit_sigma = " + Subs::str(Reduce::profile_fit_sigma) + " must be > 0");
    
    Reduce::logger.logit("Sigma rejection threshold for profile fits", Reduce::profile_fit_sigma);    
    
  }

  // Pepper warning
  if(badInput(reduce, "pepper", p))
    throw Input_Error("No peppering levels set. [option = \"pepper\"]");
  istr.str(p->second);
  float pep;
  while(istr >> pep)
    Reduce::pepper.push_back(pep);
  if(Reduce::pepper.size() < Reduce::aperture_master.size()) 
    throw Input_Error("Only " + Subs::str(Reduce::pepper.size()) + " peppering levels found compared to " + Subs::str(Reduce::aperture_master.size()) + " CCDs in aperture file");
  istr.clear();
  Reduce::logger.logit("Pepper levels", p->second);

  // Saturation warning
  if(badInput(reduce, "saturation", p))
    throw Input_Error("No saturation levels set. [option = \"saturation\"]");
  istr.str(p->second);
  float sat;
  while(istr >> sat)
    Reduce::saturation.push_back(sat);
  if(Reduce::saturation.size() < Reduce::aperture_master.size()) 
    throw Input_Error("Only " + Subs::str(Reduce::saturation.size()) + " saturation levels found compared to " + Subs::str(Reduce::aperture_master.size()) + " CCDs in aperture file");
  istr.clear();
  Reduce::logger.logit("Saturation levels", p->second);


  // Bias frame, if any
  if(badInput(reduce, "calibration_bias", p)){
    Reduce::bias = false;
    Reduce::logger.logit("No bias subtraction enabled.");
  }else{
    Reduce::bias = true;
    Reduce::bias_frame.read(p->second);
    Reduce::logger.logit("Loaded bias frame", p->second);
  }

  // Dark frame, if any
  if(badInput(reduce, "calibration_dark", p)){
    Reduce::dark = false;
    Reduce::logger.logit("No dark subtraction enabled.");
  }else{
    Reduce::dark = true;
    Reduce::dark_frame.read(p->second);
    Reduce::logger.logit("Loaded dark frame", p->second);
  }

  // Flat field frame, if any
  if(badInput(reduce, "calibration_flat", p)){
    Reduce::flat = false;
    Reduce::logger.logit("No flat fielding enabled.");
  }else{
    Reduce::flat = true;
    Reduce::flat_frame.read(p->second);
    Reduce::logger.logit("Loaded flat field", p->second);
  }
  
  // Bad pixel frame, if any
  if(badInput(reduce, "calibration_bad", p)){
    Reduce::dark = false;
    Reduce::logger.logit("No bad pixel frame supplied.");
  }else{
    Reduce::bad_pixel = true;
    Reduce::bad_pixel_frame.read(p->second);
    Reduce::logger.logit("Loaded bad pixel frame", p->second);
  }

  // Gain value or frame, if any
  if(badInput(reduce, "calibration_gain", p))
    throw Input_Error("Gain frame or value undefined. [option = \"calibration_gain\"]");

  istr.str(p->second);
  istr >> Reduce::gain;
  if(!istr){

    // Try instead to interpret as a file name
    Reduce::gain_frame.read(p->second);

    Reduce::logger.logit("Loaded gain frame", p->second);

    Reduce::gain_const = false;

  }else{

    Reduce::logger.logit("Using constant gain", Reduce::gain, "electrons/ADU.");

    Reduce::gain_const = true;
    
  }
  istr.clear();

  // Readout noise frame or value, if any
  if(badInput(reduce, "calibration_readout", p))
    throw Input_Error("Readout noise frame or value undefined. [option = \"calibration_readout\"]");

  istr.str(p->second);
  istr >> Reduce::readout;
  if(!istr){

    // OK, try instead to interpret as a file name
    Reduce::readout_frame.read(p->second);
    
    Reduce::logger.logit("Loaded readout frame", p->second);
    
    Reduce::readout_const = false;

  }else{

    Reduce::logger.logit("Using constant readout noise", Reduce::readout, "RMS ADU.");

    Reduce::readout_const = true;
    
  }
  istr.clear();

  // Coercion
  if(Reduce::bias || Reduce::dark || Reduce::flat || !Reduce::gain_const || !Reduce::readout_const){
    if(badInput(reduce, "calibration_coerce", p))
      throw Input_Error("Coercion state undefined. [option = \"calibration_coerce\"]");
    
    if(Subs::toupper(p->second) == "YES"){
      Reduce::coerce = true;
      Reduce::logger.logit("Calibration frames will be coerced to match data.");

    }else if(Subs::toupper(p->second) == "NO"){
      Reduce::coerce = false;
      Reduce::logger.logit("Calibration frames will not be coerced to match data.");

    }else{
      throw Input_Error("\"calibration_coerce\" must be either \"yes\" or \"no\".");
    }
  }else{
    Reduce::coerce = false;
  }

  // Sky estimation method
  if(badInput(reduce, "sky_method", p))
    throw Input_Error("Sky background estimation method undefined. [option = \"sky_method\"]");
    
  if(Subs::toupper(p->second) == "CLIPPED_MEAN"){
    Reduce::sky_method = Reduce::CLIPPED_MEAN;
  }else if(Subs::toupper(p->second) == "MEDIAN"){
    Reduce::sky_method = Reduce::MEDIAN;
  }else{
    throw Input_Error("sky_method must be one of 'clipped_mean' or 'median'");
  }

  Reduce::logger.logit("Sky estimation method", p->second);

  // Sky error estimation method
  if(badInput(reduce, "sky_error", p))
    throw Input_Error("Sky error estimation method undefined. [option = \"sky_error\"]");
    
  if(Subs::toupper(p->second) == "VARIANCE"){
    Reduce::sky_error = Reduce::VARIANCE;
  }else if(Subs::toupper(p->second) == "PHOTON"){
    Reduce::sky_error = Reduce::PHOTON;
  }else{
    throw Input_Error("sky_error must be one of 'variance' or 'photon'");
  }

  Reduce::logger.logit("Sky error estimation method", p->second);

  // Sky clip value
  if(badInput(reduce, "sky_thresh", p))
    throw Input_Error("Sky clip value undefined. [option = \"sky_thresh\"]");
    
  istr.str(p->second);
  istr >> Reduce::sky_thresh;
  if(!istr) throw Input_Error("Could not translate sky_thresh value");
  istr.clear();

  if(Reduce::sky_thresh <= 0.)
    throw Input_Error("sky_thresh = " + Subs::str(Reduce::sky_thresh) + " must be > 0");

  Reduce::logger.logit("Sky RMS clip threshold", p->second);

  // Image display device
  if(badInput(reduce, "image_device", p))
    throw Input_Error("Image plot device undefined. [option = \"image_device\"]");

  Reduce::image_device = p->second;

  logit("Images will be plotted to device", p->second);

  // Define light curve display
  Reduce::lightcurve_yrange_fixed = false;
  Reduce::lightcurve_invert = false;
  Reduce::position_x_yrange_fixed = false;
  Reduce::position_y_yrange_fixed = false;
  
  // Fraction occupied by light curve
  if(badInput(reduce, "lightcurve_frac", p))
    throw Input_Error("Vertical extent taken by light curve. [option = \"lightcurve_frac\"]");
  
  istr.str(p->second);
  istr >> Reduce::lightcurve_frac;
  if(!istr) throw Input_Error("Could not translate lightcurve_frac value");
  istr.clear();
  
  if(Reduce::lightcurve_frac <= 0.)
    throw Input_Error("lightcurve_frac must be > 0.");
  
  logit("Vertical extent weight of light curve", Reduce::lightcurve_frac);
  
  // plot device
  if(badInput(reduce, "lightcurve_device", p))
    throw Input_Error("Light curve plot device undefined. [option = \"lightcurve_device\"]");
  
  Reduce::lightcurve_device = p->second;
  
  logit("Light curves will be plotted to device",p->second);
  
  // X units
  if(badInput(reduce, "lightcurve_xunits", p))
    throw Input_Error("Light curve X units undefined. [option = \"lightcurve_xunits\"]");
  
  if(Subs::toupper(p->second) == "SECONDS"){
    Reduce::lightcurve_xunits = Reduce::SECONDS;
  }else if(Subs::toupper(p->second) == "MINUTES"){
    Reduce::lightcurve_xunits = Reduce::MINUTES;
  }else if(Subs::toupper(p->second) == "HOURS"){
    Reduce::lightcurve_xunits = Reduce::HOURS;
  }else if(Subs::toupper(p->second) == "DAYS"){
    Reduce::lightcurve_xunits = Reduce::DAYS;
  }else{
    throw Input_Error("lightcurve_xunits must be one of 'seconds', 'minutes', 'hours' or 'days'");
  }
  
  logit("Light curve X units", p->second);
  
  // maximum X range to plot
  if(badInput(reduce, "lightcurve_max_xrange", p))
    throw Input_Error("Maximum X range for light curves undefined. [option = \"lightcurve_max_xrange\"]");
  istr.str(p->second);
  istr >> Reduce::lightcurve_max_xrange;
  if(!istr) throw Input_Error("Could not translate lightcurve_max_xrange value");
  istr.clear();
  
  logit("Maximum X range for light curves", Reduce::lightcurve_max_xrange);
  
  // X range extension when points run out of room.
  if(badInput(reduce, "lightcurve_extend_xrange", p))
    throw Input_Error("X range extension undefined. [option = \"lightcurve_extend_xrange\"]");
  istr.str(p->second);
  istr >> Reduce::lightcurve_extend_xrange;
  if(!istr) throw Input_Error("Could not translate lightcurve_extend_xrange value");
  istr.clear();
  
  if(Reduce::lightcurve_extend_xrange <= 0.)
    throw Input_Error("lightcurve_extend_xrange must be > 0.");
  
  logit("Amount of X range extension for light curves", Reduce::lightcurve_extend_xrange);
  
  // linear or logarithmic (magnitude) scale
  if(badInput(reduce, "lightcurve_linear_or_log", p))
    throw Input_Error("Light curve linear or log undefined. [option = \"lightcurve_linear_or_log\"]");
  
  if(Subs::toupper(p->second) == "LINEAR"){
    Reduce::lightcurve_linear = true;
    std::cout << "Light curve will be plotted on a linear scale." << std::endl;
    
  }else if(Subs::toupper(p->second) == "LOG"){
    Reduce::lightcurve_linear = false;
    std::cout << "Light curve plotted on a magnitude scale." << std::endl;
    
  }else{
    throw Input_Error("\"lightcurve_linear_or_log\" must be either \"linear\" or \"log\".");
  }
  
  // Y-range options for light curve
  if(badInput(reduce, "lightcurve_yrange_fixed", p))
    throw Input_Error("Light curve Y range definition undefined. [option = \"lightcurve_yrange_fixed\"]");
  
  if(Subs::toupper(p->second) == "YES"){
    Reduce::lightcurve_yrange_fixed = true;
    std::cout << "User-defined Y range will be used for light curve." << std::endl;
    
  }else if(Subs::toupper(p->second) == "NO"){
    Reduce::lightcurve_yrange_fixed = false;
    std::cout << "Y range for light curve will be set automatically." << std::endl;
  }else{
    throw Input_Error("\"lightcurve_yrange_fixed\" must be either \"yes\" or \"no\".");
  }
  
  
  if(Reduce::lightcurve_yrange_fixed){
    
    // Lower Y limit
    if(badInput(reduce, "lightcurve_y1", p))
      throw Input_Error("Lower Y limit for light curve undefined. [option = \"lightcurve_y1\"]");
    istr.str(p->second);
    istr >> Reduce::lightcurve_y1;
    if(!istr) throw Input_Error("Could not translate lightcurve_y1 value");
    istr.clear();
    
    logit("Lower Y limit for light curves", Reduce::lightcurve_y1);
    
    // Upper Y limit
    if(badInput(reduce, "lightcurve_y2", p))
      throw Input_Error("Upper Y limit for light curves undefined. [option = \"lightcurve_y2\"]");
    istr.str(p->second);
    istr >> Reduce::lightcurve_y2;
    if(!istr) throw Input_Error("Could not translate lightcurve_y2 value");
    istr.clear();
    
    logit("Upper Y limit for light curves", Reduce::lightcurve_y2);
    
  }else{
    
    // invert y limits or not
    if(badInput(reduce, "lightcurve_invert", p))
      throw Input_Error("Light curve Y inversion undefined. [option = \"lightcurve_invert\"]");
    
    if(Subs::toupper(p->second) == "YES"){
      Reduce::lightcurve_invert = true;
      std::cout << "User-defined Y range will be used for light curve." << std::endl;
      
    }else if(Subs::toupper(p->second) == "NO"){
      Reduce::lightcurve_invert = false;
      std::cout << "Y range for light curve will be set automatically." << std::endl;
    }else{
      throw Input_Error("\"lightcurve_invert\" must be either \"yes\" or \"no\".");
    }
    
    // y range extension factor
    if(badInput(reduce, "lightcurve_extend_yrange", p))
      throw Input_Error("Y range extension factor undefined. [option = \"lightcurve_extend_yrange\"]");
    istr.str(p->second);
    istr >> Reduce::lightcurve_extend_yrange;
    if(!istr) throw Input_Error("Could not translate lightcurve_extend_yrange value");
    istr.clear();
    
    if(Reduce::lightcurve_extend_yrange <= 1.) throw Input_Error("lightcurve_extend_yrange must be > 1.");
    
    logit("Y range extension factor for light curves", Reduce::lightcurve_extend_yrange);
    
  }

  // Light curve apertures
  mmpr = special.equal_range("lightcurve_targ");
  if(mmpr.first == mmpr.second)
    throw Input_Error("No light curve apertures defined. [option = \"lightcurve_targ\"]");
  
  for(MI mmp=mmpr.first; mmp != mmpr.second; mmp++){
    
    if(mmp->second == "")
      throw Input_Error("No light curve apertures defined. [option = \"lightcurve_targ\"]");
    
    istr.str(mmp->second);
    size_t target, comparison;
    std::string colour, errcol; 
    float offset;
    istr >> nccd >> target >> comparison >> offset >> colour >> errcol;
    if(!istr) throw Input_Error("Could not translate lightcurve_targ CCD number/target/comparison/offset/colour/errcol entry");
    istr.clear();
    if(nccd < 1 || nccd > Reduce::aperture_master.size()) 
      throw Input_Error("lightcurve_targ: CCD number out of range 1 to " + Subs::str(Reduce::aperture_master.size()));
    nccd--;
    if(Reduce::extraction_control.find(nccd) == Reduce::extraction_control.end())
      throw Input_Error("lightcurve_targ: no extraction_control line found for CCD = " + Subs::str(nccd+1));
    if(target < 1 || target > Reduce::aperture_master[nccd].size()) 
      throw Input_Error("lightcurve_targ: aperture number out of range 1 to " + Subs::str(Reduce::aperture_master[nccd].size()));
    target--;
    Subs::PLOT_COLOUR pcol = Subs::what_colour(colour);
    Subs::PLOT_COLOUR ecol = Subs::what_colour(errcol);
    
    bool use_comp;
    if(comparison < 1){
      use_comp = false;
    }else{
      if(comparison > Reduce::aperture_master[nccd].size()) 
	throw Input_Error("lightcurve_targ: comparison aperture number > max = " + Subs::str(Reduce::aperture_master[nccd].size()));
      use_comp = true;
      comparison--;
    }
    
    Reduce::lightcurve_targ.push_back(Reduce::Laps(nccd, target, use_comp, comparison, offset, pcol, ecol));
    
    if(use_comp){
      message = "CCD " + Subs::str(nccd+1) + ", target = " + Subs::str(target+1) + ", comparison= " + Subs::str(comparison+1) + 
	", offset = " + Subs::str(offset) + ", colours (point,error) = " + colour + ", " + errcol; 
    }else{
      message = "CCD " + Subs::str(nccd+1) + ", target = " + Subs::str(target+1) + ", offset = " + Subs::str(offset) + 
	", colours (point,error) = " + Subs::str(colour) + ", " + errcol; 
    }
    std::cout << message << std::endl;
  }
  

  if(badInput(reduce, "position_plot", p))
    throw Input_Error("whether to plot positions is undefined. [option = \"position_plot\"]");
  
  if(Subs::toupper(p->second) == "YES"){
    Reduce::position_plot = true;
    std::cout << "Positional info will be plotted." << std::endl;
    
  }else if(Subs::toupper(p->second) == "NO"){
    Reduce::position_plot = false;
    std::cout << "Positional info will not be plotted." << std::endl;
    
  }else{
    throw Input_Error("\"position_plot\" must be either \"yes\" or \"no\".");
  }

  if(Reduce::position_plot){

    // Fraction occupied by positions
    if(badInput(reduce, "position_frac", p))
      throw Input_Error("Vertical extent taken by positions. [option = \"position_frac\"]");
    
    istr.str(p->second);
    istr >> Reduce::position_frac;
    if(!istr) throw Input_Error("Could not translate position_frac value");
    istr.clear();
    
    if(Reduce::position_frac <= 0.)
      throw Input_Error("position_frac must be > 0.");
    
    logit("Vertical extent weight of positions", Reduce::position_frac);
    
    // Y-range options for positions
    if(badInput(reduce, "position_x_yrange_fixed", p))
      throw Input_Error("X position Y range definition undefined. [option = \"position_x_yrange_fixed\"]");
    
    if(Subs::toupper(p->second) == "YES"){
      Reduce::position_x_yrange_fixed = true;
      std::cout << "User-defined Y range will be used for X positions." << std::endl;
      
    }else if(Subs::toupper(p->second) == "NO"){
      Reduce::position_x_yrange_fixed = false;
      std::cout << "Y range for X positions will be set automatically." << std::endl;
    }else{
      throw Input_Error("\"position_x_yrange_fixed\" must be either \"yes\" or \"no\".");
    }
    
    // Lower Y limit
    if(badInput(reduce, "position_x_y1", p))
      throw Input_Error("Lower Y limit for x positions undefined. [option = \"position_x_y1\"]");
    istr.str(p->second);
    istr >> Reduce::position_x_y1;
    if(!istr) throw Input_Error("Could not translate position_x_y1 value");
    istr.clear();
    
    logit("Lower Y limit for X positions", Reduce::position_x_y1);
    
    // Upper Y limit
    if(badInput(reduce, "position_x_y2", p))
      throw Input_Error("Upper Y limit for X positions undefined. [option = \"position_x_y2\"]");
    istr.str(p->second);
    istr >> Reduce::position_x_y2;
    if(!istr) throw Input_Error("Could not translate position_x_y2 value");
    istr.clear();
    
    logit("Upper Y limit for X positions", Reduce::position_x_y2);
    
    // Now Y position version 
    if(badInput(reduce, "position_y_yrange_fixed", p))
      throw Input_Error("Y position Y range definition undefined. [option = \"position_y_yrange_fixed\"]");
    
    if(Subs::toupper(p->second) == "YES"){
      Reduce::position_y_yrange_fixed = true;
      std::cout << "User-defined Y range will be used for Y positions." << std::endl;
      
    }else if(Subs::toupper(p->second) == "NO"){
      Reduce::position_y_yrange_fixed = false;
      std::cout << "Y range for y positions will be set automatically." << std::endl;
    }else{
      throw Input_Error("\"position_y_yrange_fixed\" must be either \"yes\" or \"no\".");
    }
    
    // Lower Y limit
    if(badInput(reduce, "position_y_y1", p))
      throw Input_Error("Lower Y limit for Y positions undefined. [option = \"position_y_y1\"]");
    istr.str(p->second);
    istr >> Reduce::position_y_y1;
    if(!istr) throw Input_Error("Could not translate position_y_y1 value");
    istr.clear();
    
    logit("Lower Y limit for Y positions", Reduce::position_y_y1);
    
    // Upper Y limit
    if(badInput(reduce, "position_x_y2", p))
      throw Input_Error("Upper Y limit for Y positions undefined. [option = \"position_x_y2\"]");
    istr.str(p->second);
    istr >> Reduce::position_y_y2;
    if(!istr) throw Input_Error("Could not translate position_y_y2 value");
    istr.clear();
    
    logit("Upper Y limit for Y positions", Reduce::position_y_y2);
    
    if(!Reduce::position_x_yrange_fixed || !Reduce::position_y_yrange_fixed){
      
      if(badInput(reduce, "position_extend_yrange", p))
	throw Input_Error("Y range extension factor for positions undefined. [option = \"position_extend_yrange\"]");
      istr.str(p->second);
      istr >> Reduce::position_extend_yrange;
      if(!istr) throw Input_Error("Could not translate position_extend_yrange value");
      istr.clear();
      
      if(Reduce::position_extend_yrange <= 1.) 
	throw Input_Error("position_extend_yrange must be > 1.");
      
      logit("Y range extension factor for positions", Reduce::position_extend_yrange);
      
    }

    // Position apertures
    mmpr = special.equal_range("position_targ");
    if(mmpr.first == mmpr.second)
      throw Input_Error("No position apertures defined. [option = \"position_targ\"]");
    
    for(MI mmp=mmpr.first; mmp != mmpr.second; mmp++){
      
      if(mmp->second == "")
	throw Input_Error("No position apertures defined. [option = \"position_targ\"]");
      
      istr.str(mmp->second);
      size_t posap;
      float offset;
      std::string colour, errcol;
      istr >> nccd >> posap >> offset >> colour >> errcol;
      if(!istr) throw Input_Error("Could not translate position_targ CCD number/aperture/offset combination");
      istr.clear();
      
      if(nccd < 1 || nccd > Reduce::aperture_master.size()) 
	throw Input_Error("position_targ: CCD number out of range 1 to " + Subs::str(Reduce::aperture_master.size()));
      nccd--;
      if(Reduce::extraction_control.find(nccd) == Reduce::extraction_control.end())
	throw Input_Error("position_targ: no extraction_control line found for CCD = " + Subs::str(nccd+1));
      if(posap < 1 || posap > Reduce::aperture_master[nccd].size()) 
	throw Input_Error("position_targ: aperture number out of range 1 to " + Subs::str(Reduce::aperture_master[nccd].size()));
      posap--;
      Subs::PLOT_COLOUR pcol = Subs::what_colour(colour);
      Subs::PLOT_COLOUR ecol = Subs::what_colour(errcol);
      
      Reduce::position_targ.push_back(Reduce::Paps(nccd, posap, offset, pcol, ecol));
      
      message = "CCD " + Subs::str(nccd+1) + ", position aperture = " + Subs::str(posap+1) + ", offset = " + Subs::str(offset) + 
	", colours (point,error) = " + colour + ", " + errcol; 
      std::cout << message << std::endl;
    } 

  }

  // Transmission stuff
  if(badInput(reduce, "transmission_plot", p))
    throw Input_Error("whether to plot transmission is undefined. [option = \"transmission_plot\"]");
  
  if(Subs::toupper(p->second) == "YES"){
    Reduce::transmission_plot = true;
    std::cout << "Transmission info will be plotted." << std::endl;
    
  }else if(Subs::toupper(p->second) == "NO"){
    Reduce::transmission_plot = false;
    std::cout << "Transmission info will not be plotted." << std::endl;
    
  }else{
    throw Input_Error("\"transmission_plot\" must be either \"yes\" or \"no\".");
  }

  if(Reduce::transmission_plot){

    // Fraction occupied by transmission
    if(badInput(reduce, "transmission_frac", p))
      throw Input_Error("Vertical extent taken by transmission undefined. [option = \"transmission_frac\"]");
    
    istr.str(p->second);
    istr >> Reduce::transmission_frac;
    if(!istr) throw Input_Error("Could not translate transmission_frac value");
    istr.clear();
    
    if(Reduce::transmission_frac <= 0.)
      throw Input_Error("transmission_frac must be > 0.");
    
    logit("Vertical extent weight of transmission", Reduce::transmission_frac);

    // Maximum transmission at which to re-draw
    if(badInput(reduce, "transmission_ymax", p))
      throw Input_Error("Maximum percentage transmission to plot undefined. [option = \"transmission_ymax\"]");
    
    istr.str(p->second);
    istr >> Reduce::transmission_ymax;
    if(!istr) throw Input_Error("Could not translate transmission_ymax value");
    istr.clear();
    
    if(Reduce::transmission_ymax < 100)
      throw Input_Error("transmission_ymax must be >= 100");
    
    logit("Maximum percentage transmission", Reduce::transmission_ymax);

    // Transmission apertures
    mmpr = special.equal_range("transmission_targ");
    if(mmpr.first == mmpr.second)
      throw Input_Error("No transmission apertures defined. [option = \"transmission_targ\"]");
    
    for(MI mmp=mmpr.first; mmp != mmpr.second; mmp++){
      
      if(mmp->second == "")
	throw Input_Error("No transmission apertures defined. [option = \"transmission_targ\"]");
      
      istr.str(mmp->second);
      size_t transap;
      std::string colour;
      istr >> nccd >> transap >> colour;
      if(!istr) throw Input_Error("Could not translate transmission_targ CCD number/aperture/colour combination");
      istr.clear();
      
      if(nccd < 1 || nccd > Reduce::aperture_master.size()) 
	throw Input_Error("transmission_targ: CCD number out of range 1 to " + Subs::str(Reduce::aperture_master.size()));
      nccd--;
      if(Reduce::extraction_control.find(nccd) == Reduce::extraction_control.end())
	throw Input_Error("transmission_targ: no extraction_control line found for CCD = " + Subs::str(nccd+1));
      if(transap < 1 || transap > Reduce::aperture_master[nccd].size()) 
	throw Input_Error("transmission_targ: aperture number out of range 1 to " + Subs::str(Reduce::aperture_master[nccd].size()));
      transap--;
      Subs::PLOT_COLOUR pcol = Subs::what_colour(colour);
      
      Reduce::transmission_targ.push_back(Reduce::Taps(nccd, transap, pcol));
      
      message = "CCD " + Subs::str(nccd+1) + ", transmission aperture = " + Subs::str(transap+1) + ", colour = " + colour;
      std::cout << message << std::endl;
    } 

  }

  // Seeing stuff
  if(badInput(reduce, "seeing_plot", p))
    throw Input_Error("whether to plot seeing is undefined. [option = \"seeinf_plot\"]");
  
  if(Subs::toupper(p->second) == "YES"){
    Reduce::seeing_plot = true;
    std::cout << "Seeing info will be plotted." << std::endl;
    
  }else if(Subs::toupper(p->second) == "NO"){
    Reduce::seeing_plot = false;
    std::cout << "Seeing info will not be plotted." << std::endl;
    
  }else{
    throw Input_Error("\"seeing_plot\" must be either \"yes\" or \"no\".");
  }

  if(Reduce::seeing_plot){

    // Fraction occupied by transmission
    if(badInput(reduce, "seeing_frac", p))
      throw Input_Error("Vertical extent taken by seeing undefined. [option = \"seeing_frac\"]");
    
    istr.str(p->second);
    istr >> Reduce::seeing_frac;
    if(!istr) throw Input_Error("Could not translate seeing_frac value");
    istr.clear();
    
    if(Reduce::seeing_frac <= 0.)
      throw Input_Error("seeing_frac must be > 0.");
    
    logit("Vertical extent weight of seeing", Reduce::seeing_frac);

    // Scale factor to multiply plot height by when seeing goes higher than maximum
    if(badInput(reduce, "seeing_extend_yrange", p))
      throw Input_Error("Seeing plot rescaling factor undefined. [option = \"seeing_extend_yrange\"]");
    
    istr.str(p->second);
    istr >> Reduce::seeing_extend_yrange;
    if(!istr) throw Input_Error("Could not translate seeing_extend_yrange value");
    istr.clear();
    
    if(Reduce::seeing_extend_yrange <= 1)
      throw Input_Error("seeing_extend_yrange must be > 1");
    
    logit("Seeing plot rescaling factor", Reduce::seeing_extend_yrange);

    // Maximum seeing to start plot
    if(badInput(reduce, "seeing_ymax", p))
      throw Input_Error("Initial maximum of seeing plot undefined. [option = \"seeing_ymax\"]");
    
    istr.str(p->second);
    istr >> Reduce::seeing_ymax;
    if(!istr) throw Input_Error("Could not translate seeing_ymax value");
    istr.clear();
    
    if(Reduce::seeing_ymax <= 0)
      throw Input_Error("seeing_ymax must be > 0");
    
    logit("Initial maximum for seeing plot", Reduce::seeing_ymax);

    // Plate scale
    if(badInput(reduce, "seeing_scale", p))
      throw Input_Error("Plate scale (arcsec/pixel) undefined. [option = \"seeing_scale\"]");
    
    istr.str(p->second);
    istr >> Reduce::seeing_scale;
    if(!istr) throw Input_Error("Could not translate seeing_scale value");
    istr.clear();
    
    if(Reduce::seeing_scale <= 0)
      throw Input_Error("seeing_scale must be > 0");
    
    logit("Plate scale (arcsec/pixel)", Reduce::seeing_scale);

    // Seeing info
    mmpr = special.equal_range("seeing_targ");
    if(mmpr.first == mmpr.second)
      throw Input_Error("No seeing entries defined. [option = \"seeing_targ\"]");
    
    for(MI mmp=mmpr.first; mmp != mmpr.second; mmp++){
      
      if(mmp->second == "")
	throw Input_Error("No seeing entries defined. [option = \"seeing_targ\"]");
      
      istr.str(mmp->second);
      std::string colour;
      size_t fwhmap;
      istr >> nccd >> fwhmap >> colour;
      if(!istr) throw Input_Error("Could not translate seeing_targ CCD number/colour combination");
      istr.clear();
      
      if(nccd < 1 || nccd > Reduce::aperture_master.size()) 
	throw Input_Error("seeing_targ: CCD number out of range 1 to " + Subs::str(Reduce::aperture_master.size()));
      nccd--;
      if(Reduce::extraction_control.find(nccd) == Reduce::extraction_control.end())
	throw Input_Error("seeing_targ: no extraction_control line found for CCD = " + Subs::str(nccd+1));
      if(fwhmap < 1 || fwhmap > Reduce::aperture_master[nccd].size()) 
	throw Input_Error("seeing_targ: aperture number out of range 1 to " + Subs::str(Reduce::aperture_master[nccd].size()));
      fwhmap--;
      Subs::PLOT_COLOUR pcol = Subs::what_colour(colour);

      Reduce::seeing_targ.push_back(Reduce::Faps(nccd, fwhmap, pcol));      
      
      message = "CCD " + Subs::str(nccd+1) + ", seeing aperture = " + Subs::str(fwhmap) + ", colour = " + colour;
      std::cout << message << std::endl;

    } 

  }

  // Terminal output mode
  if(badInput(reduce, "terminal_output", p))
    throw Input_Error("Terminal output mode undefined. [option = \"terminal_output\"]");
    
  // This gets checked quite often so convert to integer to speed 
  // comparisons later on

  if(Subs::toupper(p->second) == "NONE"){
    Reduce::terminal_output = Reduce::NONE;
  }else if(Subs::toupper(p->second) == "LITTLE"){
    Reduce::terminal_output = Reduce::LITTLE;
  }else if(Subs::toupper(p->second) == "MEDIUM"){
    Reduce::terminal_output = Reduce::MEDIUM;
  }else if(Subs::toupper(p->second) == "FULL"){
    Reduce::terminal_output = Reduce::FULL;
  }else{
    throw Input_Error("terminal_output must be one of 'none', 'little'"
		      " 'medium' or 'full'");
  }

  logit("Terminal output", p->second);

}

