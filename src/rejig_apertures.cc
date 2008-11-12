#include <map>
#include "trm_subs.h"
#include "trm_ultracam.h"
#include "trm_aperture.h"
#include "trm_mccd.h"
#include "trm_frame.h"
#include "trm_reduce.h"

// Globals read by read_reduce_file

namespace Reduce {


    extern ABORT_BEHAVIOUR abort_behaviour; 
    extern Ultracam::Maperture aperture_master;        
    extern APERTURE_REPOSITION_MODE aperture_reposition_mode;
    extern bool  aperture_positions_stable;                  
    extern int   aperture_search_half_width;                 
    extern float aperture_search_fwhm;                       
    extern float aperture_search_max_shift;                  
    extern int   aperture_tweak_half_width;                   
    extern float aperture_tweak_fwhm;                         
    extern float aperture_tweak_max_shift;                    
    extern bool  aperture_position_twopass;
    extern std::map<int,Reduce::Extraction> extraction_control;    
    extern PROFILE_FIT_METHOD profile_fit_method;             
    extern PROFILE_FIT_METHOD extraction_weights;             
    extern float profile_fit_fwhm;                            
    extern int   profile_fit_hwidth;                          
    extern bool  profile_fit_symm;                            
    extern float profile_fit_beta;                            
    extern float profile_fit_sigma;                          
    extern Ultracam::Frame gain_frame;                                  
    extern Ultracam::Frame readout_frame;                               
};

/** Routine to handle the aperture updating part of 'reduce'. This is essentially to isolate this
 * long section out from 'reduce'. The best way to get an idea what this does would be to read the
 * help in 'reduce'. To save time various operations are done on the first call to this routine
 * which assume that subsequent calls are made with the same number of CCDs and apertures, therefore
 * these should not be altered.
 * \param data      the data frame, bias & dark subtracted and flat-fielded
 * \param dvar      estimated variances on each point of data frame. Assumed to have identical format to data
 * \param profile_fit_plot Plot for profile fits
 * \param blue_is_bad is the blue data bad or not in the sense of there is data rather than junk from the nblue option
 * \param aperture  the aperture file, input and returned
 * \param shape     profile fitting shape parameters, returned
 * \param errors    uncertainties on aperture positions, returned. For the linked apertures these will be set equal to
 * the uncertainties on the master apertures that they are linked to. If the static reposition option is used,
 * they will all be set equal to 0
 */

void Ultracam::rejig_apertures(const Frame& data, const Frame& dvar, const Subs::Plot& profile_fit_plot, bool blue_is_bad, 
			       Maperture& aperture, std::vector<Reduce::Meanshape>& shape, std::vector<std::vector<Fxy> >& errors){

    // Stuff that must be saved between calls
    static bool first = true;
    static std::vector<std::map<int,int> > aperture_link;
    static Maperture previous_aperture;

    // To save time, we will not re-do linked apertures, but first we need
    // to work out which apertures they are linked to. Store the results in the following maps
    // Also use this chance to check validity of apertures
    Aperture *app, *app1;
    bool ap_ok;
    float rstar = 0, rsky1 = 0, rsky2 = 0;

    if(first){
	first = false;
   
	// Check validity of apertures
	for(size_t nccd=0; nccd<Reduce::aperture_master.size(); nccd++){ 
	    for(size_t naper=0; naper<Reduce::aperture_master[nccd].size(); naper++){
		if(!Reduce::aperture_master[nccd][naper].valid())
		    throw Ultracam_Error("Ultracam::rejig_apertures: at least one of the supplied apertures is already marked invalid.\nPlease fix this.");
	    }
	}

	shape.resize(data.size());
	aperture_link.resize(data.size());
	previous_aperture = aperture = Reduce::aperture_master;

	for(size_t nccd=0; nccd<Reduce::aperture_master.size(); nccd++){ 
      
	    if(nccd != 2 || !blue_is_bad){
		if(Reduce::extraction_control.find(nccd) != Reduce::extraction_control.end()){

		    ap_ok = (Reduce::extraction_control[nccd].aperture_type     == Reduce::FIXED &&
			     Reduce::extraction_control[nccd].extraction_method == Reduce::NORMAL);
		
		    for(size_t naper=0; naper<Reduce::aperture_master[nccd].size(); naper++){
			app = &Reduce::aperture_master[nccd][naper];
			if(app->ref()) ap_ok = true;
			if(app->linked()){
			    bool link_found = false;
			    for(size_t naper1=0; naper1<Reduce::aperture_master[nccd].size(); naper1++){
				app1 = &Reduce::aperture_master[nccd][naper1];
				if(!app1->linked() && app1->xref() == app->xref() && app1->yref() == app->yref()){
				    aperture_link[nccd][naper] = naper1;
				    link_found = true;
				    break;
				}		
			    }
			    if(!link_found) 
				throw Ultracam_Error("Ultracam::rejig_apertures: no master aperture found for linked aperture " + Subs::str(naper+1) + " of CCD " + Subs::str(nccd+1));
			}
		    }
		    if(!ap_ok) 
			throw Ultracam_Error("Ultracam::rejig_apertures: no reference aperture found for CCD " + Subs::str(nccd+1) + " even though profile fitting required.");
		}
	    }
	}

	// Make sure errors structure has correct sizes and initialize to zero
	errors.resize(aperture.size());
	for(size_t nccd=0; nccd<aperture.size(); nccd++){
	    errors[nccd].resize(aperture[nccd].size());
	    for(size_t naper=0; naper<aperture[nccd].size(); naper++)
		errors[nccd][naper] = Fxy(0.f,0.f);
	}
    
	// Clamp radii of apertures. This allows the user effectively to override the aperture sizes 
	for(size_t nccd=0; nccd<data.size(); nccd++){ 
	    if(Reduce::extraction_control.find(nccd) != Reduce::extraction_control.end()){
		for(size_t naper=0; naper<aperture[nccd].size(); naper++){
		    app = &aperture[nccd][naper];
		    rstar = Subs::clamp(Reduce::extraction_control[nccd].star_min,      app->rstar(), Reduce::extraction_control[nccd].star_max);
		    rsky1 = Subs::clamp(Reduce::extraction_control[nccd].inner_sky_min, app->rsky1(), Reduce::extraction_control[nccd].inner_sky_max);
		    rsky2 = Subs::clamp(Reduce::extraction_control[nccd].outer_sky_min, app->rsky2(), Reduce::extraction_control[nccd].outer_sky_max);
		    app->set_radii(rstar, rsky1, rsky2);
		}
	    }
	}      
    }

    // OK now onto stuff that gets done every frame

    // Check validity of current apertures; if any have gone off then
    // retrieve old version (the apertures are certified correct at the
    // start so there is no chance of trying to retrieve apertures before
    // storage. If all OK, then we store the current ones for possible future
    // use. The different CCDs are regarded as independent.
    for(size_t nccd=0; nccd<aperture.size(); nccd++){ 

	if(Reduce::extraction_control.find(nccd) != Reduce::extraction_control.end()){
      
	    ap_ok = true;
	    for(size_t naper=0; naper<aperture[nccd].size(); naper++){
		if(!aperture[nccd][naper].valid()){
		    ap_ok = false;
		    break;
		}
	    }
      
	    if(ap_ok)
		previous_aperture[nccd] = aperture[nccd];
	    else
		aperture[nccd] = previous_aperture[nccd];
	}
    }
  
    float xstart, ystart, fwhm_x, fwhm_y, ex, ey, shift, max_shift;
    float read, gain, offset_x, offset_y, xref, yref;
    int hwidth_x, hwidth_y, nref, nref_check;
    double xpos, ypos;

    if(Reduce::aperture_reposition_mode == Reduce::STATIC){
    
	// Do nothing
    
    }else if(Reduce::aperture_reposition_mode == Reduce::INDIVIDUAL ||
	     Reduce::aperture_reposition_mode == Reduce::INDIVIDUAL_PLUS_TWEAK ||
	     Reduce::aperture_reposition_mode == Reduce::REFERENCE_PLUS_TWEAK ){
    
	// Go through adjusting the position of every aperture for every CCD
	for(size_t nccd=0; nccd<data.size(); nccd++){ 
   
	    if(nccd != 2 || !blue_is_bad){
   
		if(Reduce::extraction_control.find(nccd) != Reduce::extraction_control.end()){
		    nref = 0;
		    offset_x = offset_y = 0.f;
		
		    if(Reduce::aperture_reposition_mode == Reduce::REFERENCE_PLUS_TWEAK && aperture[nccd].size() > 0){
		    
			// In this case try first to determine a shift from reference apertures
		    
			// Loop over apertures
			for(size_t naper=0; naper<aperture[nccd].size(); naper++){
			
			    app = &aperture[nccd][naper];
			
			    // Only consider reference apertures
			    if(app->valid() && app->ref()){
			    
				try{
				
				    const Windata &dwin = data[nccd].enclose(app->xref(), app->yref());
				    const Windata &vwin = dvar[nccd].enclose(app->xref(), app->yref());
				
				    xstart   = dwin.xcomp(app->xref());
				    ystart   = dwin.ycomp(app->yref());
				    fwhm_x   = Reduce::aperture_search_fwhm/dwin.xbin();
				    fwhm_x   = fwhm_x > 1.f ? fwhm_x : 1.f;
				    fwhm_y   = Reduce::aperture_search_fwhm/dwin.ybin();
				    fwhm_y   = fwhm_y > 1.f ? fwhm_y : 1.f;
				    hwidth_x = Reduce::aperture_search_half_width/dwin.xbin();
				    hwidth_x = hwidth_x > int(fwhm_x+1.) ? hwidth_x : int(fwhm_x+1.);
				    hwidth_y = Reduce::aperture_search_half_width/dwin.ybin();
				    hwidth_y = hwidth_y > int(fwhm_y+1.) ? hwidth_y : int(fwhm_y+1.);
				
				    // Remeasure the position
				
				    Ultracam::findpos(dwin, vwin, dwin.nx(), dwin.ny(), fwhm_x, fwhm_y, hwidth_x, hwidth_y, 
						      xstart, ystart, Reduce::aperture_positions_stable, xpos, ypos, ex, ey);
				
				    // Check that position has not shifted more than expected
				    if((shift = sqrt(Subs::sqr(dwin.xbin()*(xpos-xstart)) + Subs::sqr(dwin.ybin()*(ypos-ystart)))) 
				       < Reduce::aperture_search_max_shift){
				    
					offset_x += xpos-xstart;
					offset_y += ypos-ystart;
				    
					nref++;
					app->set_xref(dwin.xccd(xpos));
					app->set_yref(dwin.yccd(ypos));
				    
					errors[nccd][naper] = Fxy(ex, ey);
				    
				    }else{
				    
					app->set_valid(false);
					if(Reduce::abort_behaviour == Reduce::RELAXED){
					    std::cout << "Ultracam::rejig_apertures 1: CCD " << nccd + 1 << ", aperture " << naper+1 
						      << " shifted by more than the maximum. Shift = " << shift << " cf " << Reduce::aperture_search_max_shift << std::endl;
					}else if(Reduce::abort_behaviour == Reduce::FUSSY){
					    throw Ultracam_Error("Ultracam::rejig_apertures 1: Fussy mode: CCD " + Subs::str(nccd+1) + ", aperture " + Subs::str(naper+1) +
								 " shifted by more than the maximum.");
					}
				    }	      
				}
				catch(const Ultracam_Error& err){
				    app->set_valid(false);
				    if(Reduce::abort_behaviour == Reduce::FUSSY) 
					throw Ultracam_Error("Ultracam::rejig_apertures: fussy mode: " + err);
				}
			    }
			}
			if(nref > 0){
			    offset_x /= nref;
			    offset_y /= nref;
			
			}else{
			    if(Reduce::abort_behaviour == Reduce::FUSSY)
				throw Ultracam_Error("Ultracam::rejig_apertures: fussy mode: CCD " + Subs::str(nccd+1) + ", failed to lock on to any reference star.");
			
			    std::cerr << "Ultracam::rejig_apertures: CCD " << (nccd+1) << ", failed to lock on to any reference star." << std::endl;
			}   
		    }else{
			nref = 0;
		    }
	
		    // Only carry out next bit if a reference aperture was located
		    if(Reduce::aperture_reposition_mode != Reduce::REFERENCE_PLUS_TWEAK || nref > 0){
		    
			// Loop over apertures
			for(size_t naper=0; naper<aperture[nccd].size(); naper++){
			
			    // Only adjust unlinked apertures. Note that if reference_plus_tweak is in effect
			    // the reference apertures will get re-adjusted here but that is OK because the
			    // first time may be rather crude
			    app = &aperture[nccd][naper];
			
			    if(app->valid() && !app->linked()){
			    
				try{
				
				    // Get references to windata
				    const Windata &dwin = data[nccd].enclose(app->xref(), app->yref());
				    const Windata &vwin = dvar[nccd].enclose(app->xref(), app->yref());
				
				    // Avoid double compensating the change for reference apertures.
				    if(app->ref() && nref > 0){
					xstart   = dwin.xcomp(app->xref());
					ystart   = dwin.ycomp(app->yref());
				    }else{
					xstart   = dwin.xcomp(app->xref()) + offset_x;
					ystart   = dwin.ycomp(app->yref()) + offset_y;
				    }
				
				    // If reference_plus_tweak, then we use more restrictive criteria
				    // in repositioning the apertures because we believe that we are already
				    // close the right value from the reference reposition, unless the reference
				    // reposition failed (nref == 0)
				    if(Reduce::aperture_reposition_mode == Reduce::REFERENCE_PLUS_TWEAK && nref > 0){
					fwhm_x    = Reduce::aperture_tweak_fwhm/dwin.xbin();
					fwhm_y    = Reduce::aperture_tweak_fwhm/dwin.ybin();
					hwidth_x  = Reduce::aperture_tweak_half_width/dwin.xbin();
					hwidth_y  = Reduce::aperture_tweak_half_width/dwin.ybin();
					max_shift = Reduce::aperture_tweak_max_shift;
				    }else{
					fwhm_x    = Reduce::aperture_search_fwhm/dwin.xbin();
					fwhm_y    = Reduce::aperture_search_fwhm/dwin.ybin();
					hwidth_x  = Reduce::aperture_search_half_width/dwin.xbin();
					hwidth_y  = Reduce::aperture_search_half_width/dwin.ybin();
					max_shift = Reduce::aperture_search_max_shift;
				    }
				    fwhm_x   = fwhm_x > 1.f ? fwhm_x : 1.f;
				    fwhm_y   = fwhm_y > 1.f ? fwhm_y : 1.f;
				    hwidth_x = hwidth_x > int(fwhm_x+1.) ? hwidth_x : int(fwhm_x+1.);
				    hwidth_y = hwidth_y > int(fwhm_y+1.) ? hwidth_y : int(fwhm_y+1.);
				
				    // Remeasure the position
				    if(Reduce::aperture_reposition_mode == Reduce::REFERENCE_PLUS_TWEAK && nref > 0){
					Ultracam::findpos(dwin, vwin, dwin.nx(), dwin.ny(), fwhm_x, fwhm_y, hwidth_x, hwidth_y, 
							  xstart, ystart, true, xpos, ypos, ex, ey);
				    }else{
					Ultracam::findpos(dwin, vwin, dwin.nx(), dwin.ny(), fwhm_x, fwhm_y, hwidth_x, hwidth_y, 
							  xstart, ystart, Reduce::aperture_positions_stable, xpos, ypos, ex, ey);
				    }
				
				    //		cerr << nccd+1 << " " << naper+1 << " " << xstart << " " << ystart << " " << xpos << " " << ypos << endl;
				
				    // Check that the shift is not too large
				    if((shift = sqrt(Subs::sqr(dwin.xbin()*(xpos-xstart)) + Subs::sqr(dwin.ybin()*(ypos-ystart)))) < max_shift){
				    
					errors[nccd][naper] = Fxy(ex, ey);
				    
					app->set_xref(dwin.xccd(xpos));
					app->set_yref(dwin.yccd(ypos));
		  
					// If the aperture is offset from another position, then it will be tweaked
					// in some cases using more restrictive criteria. 
					if((app->xoff() != 0. || app->yoff() != 0.) && Reduce::aperture_reposition_mode == Reduce::INDIVIDUAL_PLUS_TWEAK){
		    
					    xstart   = dwin.xcomp(app->xpos());
					    ystart   = dwin.ycomp(app->ypos());
					    hwidth_x = Reduce::aperture_tweak_half_width/dwin.xbin();
					    hwidth_y = Reduce::aperture_tweak_half_width/dwin.ybin();
					    fwhm_x   = Reduce::aperture_tweak_fwhm/dwin.xbin();
					    fwhm_y   = Reduce::aperture_tweak_fwhm/dwin.ybin();
		    
					    Ultracam::findpos(dwin, vwin, dwin.nx(), dwin.ny(), fwhm_x, fwhm_y, hwidth_x, hwidth_y, 
							      xstart, ystart, true, xpos, ypos, ex, ey);
		    
					    // Check shift is not too large
					    if((shift = sqrt(Subs::sqr(dwin.xbin()*(xpos-xstart)) + Subs::sqr(dwin.ybin()*(ypos-ystart)))) 
					       < Reduce::aperture_tweak_max_shift){
		      
						errors[nccd][naper] = Fxy(ex, ey);
		      
						app->set_xoff(dwin.xccd(xpos) - app->xref());
						app->set_yoff(dwin.yccd(ypos) - app->yref());
		      
					    }else{
		      
						app->set_valid(false);
						if(Reduce::abort_behaviour == Reduce::RELAXED){
						    std::cerr << "Ultracam::rejig_apertures 2: CCD " << nccd + 1 << ", aperture " << naper+1 
							      << " shifted by more than the maximum. Shift = " << shift << " cf " << Reduce::aperture_tweak_max_shift << std::endl;
						}else if(Reduce::abort_behaviour == Reduce::FUSSY){
						    throw Ultracam_Error("Ultracam::rejig_apertures 2: fussy mode: CCD " + Subs::str(nccd+1) + ", aperture " + Subs::str(naper+1) +
									 " shifted by more than the maximum.");
						}
					    }
					}
		  
				    }else{
		  
					app->set_valid(false);
					if(Reduce::abort_behaviour == Reduce::RELAXED){
					    std::cerr << "Ultracam::rejig_apertures 3: CCD " << nccd + 1 << ", aperture " << naper+1 
						      << " shifted by more than the maximum. Shift = "  << shift << " cf " << max_shift << std::endl;
					}else if(Reduce::abort_behaviour == Reduce::FUSSY){
					    throw Ultracam_Error("Ultracam::rejig_apertures 3: fussy mode: CCD " + Subs::str(nccd+1) +
								 ", aperture " + Subs::str(naper+1) + " shifted by more than the maximum.");
					}
				    }
				}
				catch(const Ultracam_Error& err){
				    app->set_valid(false);
				    if(Reduce::abort_behaviour == Reduce::FUSSY) 
					throw Ultracam_Error("Ultracam::rejig_apertures: fussy mode: " + err);
				}
			    }
			}
	
			// Now update the linked apertures, without fitting
			for(size_t naper=0; naper<aperture[nccd].size(); naper++){  
			    app  = &aperture[nccd][naper];
			    if(app->linked()){
	      
				app1 = &aperture[nccd][aperture_link[nccd][naper]];
				errors[nccd][naper] = errors[nccd][aperture_link[nccd][naper]];
	      
				if(app->valid() && app1->valid()){
				    app->set_xref(app1->xref());
				    app->set_yref(app1->yref());
				}else{
				    app->set_valid(false);
				}
			    }
			}
		    }else{
			// No reference stars located, invalidate all other apertures
			for(size_t naper=0; naper<aperture[nccd].size(); naper++)
			    aperture[nccd][naper].set_valid(false);
		    }
		}
	    }
	}
    }

    // Now adjust aperture positions with profile fits, if wanted and if reference apertures are set 
    for(size_t nccd=0; nccd<data.size(); nccd++){ 
    
	if(nccd != 2 || !blue_is_bad){

	    if(Reduce::extraction_control.find(nccd) != Reduce::extraction_control.end()){
      
		if(aperture[nccd].size()){
	
		    // Initialise.
		    // The profile fit method and extraction weights may look unnecessary in the context
		    // of 'reduce', but are needed for 'plot'
		    shape[nccd].set  = false;
		    shape[nccd].profile_fit_symm     = Reduce::profile_fit_symm;
		    shape[nccd].profile_fit_method   = Reduce::profile_fit_method;
		    shape[nccd].extraction_weights   = Reduce::extraction_weights;
		    shape[nccd].fwhm = shape[nccd].a = shape[nccd].b = shape[nccd].c = shape[nccd].beta = 0.;  
	
		    if(Reduce::extraction_control[nccd].aperture_type     == Reduce::VARIABLE || 
		       Reduce::extraction_control[nccd].extraction_method == Reduce::OPTIMAL){
	  
			// First deal with reference targets, if there are any.
			double wgt, sumw = 0.;
			nref_check = 0;
			xref = yref = 0.f;
			for(size_t naper=0; naper<aperture[nccd].size(); naper++){
			    app = &aperture[nccd][naper];
			    if(app->valid() && app->ref()){
	      
				try{
		
				    // Obtain initial value of 'a'
				    double a = 1./2./Subs::sqr(Reduce::profile_fit_fwhm/Constants::EFAC);
		
				    Ultracam::Ppars profile;
				    if(Reduce::profile_fit_method == Reduce::GAUSSIAN){
		  
					// Gaussian fit section.
					profile.set(0., 0., 0., 0., a, 0., a, Reduce::profile_fit_symm);
		  
				    }else if(Reduce::profile_fit_method == Reduce::MOFFAT){
		  
					// Moffat fit section.
					profile.set(0., 0., 0., 0., a, 0., a, Reduce::profile_fit_beta, Reduce::profile_fit_symm);
		  
				    }
		
				    Ultracam::Iprofile iprofile;
				    Ultracam::fit_plot_profile(data[nccd], dvar[nccd], profile, false, true, app->xref(), app->yref(), app->mask(), 0., 0, 
							       Reduce::profile_fit_hwidth, profile_fit_plot, Reduce::profile_fit_sigma, iprofile, false);
		
				    // Check shift is not too large (compared agaiinst fine tweak settings)
				    if((shift = sqrt(Subs::sqr(profile.x-app->xref()) + Subs::sqr(profile.y-app->yref()))) < Reduce::aperture_tweak_max_shift){	      
		  
					errors[nccd][naper] = Fxy(iprofile.ex, iprofile.ey);
		  
					// Update aperture position
					app->set_xref(profile.x);
					app->set_yref(profile.y);
		  
					// Average shape values. Use single weight.
					wgt   = 1./iprofile.covar[profile.a_index()][profile.a_index()];
					shape[nccd].fwhm  += wgt*iprofile.fwhm;
					shape[nccd].a     += wgt*profile.a;
					shape[nccd].b     += wgt*profile.b;
					shape[nccd].c     += wgt*profile.c;
					if(Reduce::profile_fit_method == Reduce::MOFFAT)
					    shape[nccd].beta += wgt*profile.beta;
					sumw += wgt;
					shape[nccd].set = true;
		  
				    }else{
		  
					app->set_valid(false);
					if(Reduce::abort_behaviour == Reduce::RELAXED){
					    std::cerr << "4. CCD " << nccd + 1 << ", aperture " << naper+1 
						      << " shifted by more than the maximum. Shift = " << shift << " cf " << Reduce::aperture_tweak_max_shift << std::endl;
					}else if(Reduce::abort_behaviour == Reduce::FUSSY){
					    throw Ultracam_Error("Ultracam::rejig_apertures 4: fussy mode: CCD " + Subs::str(nccd+1) +
								 ", aperture " + Subs::str(naper+1) + " shifted by more than the maximum.");
					}
				    }
				}
				catch(const std::string& err){
				    if(Reduce::abort_behaviour == Reduce::FUSSY){
					throw Ultracam_Error("Ultracam::rejig_apertures: fussy mode, reference fit: " + err);		
				    }else if(Reduce::abort_behaviour == Reduce::RELAXED){
					std::cerr << "Reference fit, CCD " << nccd+1 << ", aperture " << naper+1 << ": " << err << std::endl;
					app->set_valid(false);
				    }
				} 
			    }
			}

			// Now adjust non-reference apertures
			if(!shape[nccd].set){
	    
			    // No valid fit made, invalidate all apertures
			    for(size_t naper=0; naper<aperture[nccd].size(); naper++)
				aperture[nccd][naper].set_valid(false);
	    
			}else{
	    
			    // Derive mean shape parameters
			    shape[nccd].fwhm /= sumw;
			    shape[nccd].a    /= sumw;
			    shape[nccd].b    /= sumw;
			    shape[nccd].c    /= sumw;
			    if(Reduce::profile_fit_method == Reduce::MOFFAT)
				shape[nccd].beta /= sumw;
	    
			    // Recompute aperture radii if not fixed
			    if(Reduce::extraction_control[nccd].aperture_type == Reduce::VARIABLE){
				rstar = Subs::clamp(Reduce::extraction_control[nccd].star_min,      float(Reduce::extraction_control[nccd].star_scale*shape[nccd].fwhm),      Reduce::extraction_control[nccd].star_max);
				rsky1 = Subs::clamp(Reduce::extraction_control[nccd].inner_sky_min, float(Reduce::extraction_control[nccd].inner_sky_scale*shape[nccd].fwhm), Reduce::extraction_control[nccd].inner_sky_max);
				rsky2 = Subs::clamp(Reduce::extraction_control[nccd].outer_sky_min, float(Reduce::extraction_control[nccd].outer_sky_scale*shape[nccd].fwhm), Reduce::extraction_control[nccd].outer_sky_max);
	      
				// A check for something which should never happen
				if(rsky1 >= rsky2)
				    throw Ultracam_Error("rejig_apertures: inner radius of sky annulus >= outer; should not happen");
			    }
	    
			    for(size_t naper=0; naper<aperture[nccd].size(); naper++){
	      
				app = &aperture[nccd][naper];
	      
				// Adjust radii of apertures if need be 
				if(Reduce::extraction_control[nccd].aperture_type == Reduce::VARIABLE)
				    app->set_radii(rstar, rsky1, rsky2);
	      
				// Only fit unlinked, unreferenced apertures
				if(app->valid() && !app->ref() && aperture_link[nccd].find(naper) == aperture_link[nccd].end()){
		
				    try{
		  
					// Get references to windatas
					const Windata &rwin = Reduce::readout_frame[nccd].enclose(app->xref(), app->yref());
					const Windata &gwin = Reduce::gain_frame[nccd].enclose(app->xref(), app->yref());
		
					read = rwin[0][0], gain = gwin[0][0];
		  
					Ultracam::Ppars profile;
					if(Reduce::profile_fit_method == Reduce::GAUSSIAN){
					    // Gaussian fit section.
					    profile.set(0., 0., 0., 0., shape[nccd].a, shape[nccd].b, shape[nccd].c, Reduce::profile_fit_symm);
		    
					}else if(Reduce::profile_fit_method == Reduce::MOFFAT){
		    
					    // Moffat fit section.
					    profile.set(0., 0., 0., 0., shape[nccd].a, shape[nccd].b, shape[nccd].c, shape[nccd].beta, Reduce::profile_fit_symm);
					    profile.var_beta = false;
		    
					}
					profile.var_a = profile.var_b = profile.var_c = false; 
		  
					Ultracam::Iprofile iprofile;
					Ultracam::fit_plot_profile(data[nccd], dvar[nccd], profile, false, true, app->xref(), app->yref(), app->mask(), 0., 0, 
								   Reduce::profile_fit_hwidth, profile_fit_plot, Reduce::profile_fit_sigma, iprofile, false);
		  
					// Check shift is not too large (compared agaiinst fine tweak settings)
					if((shift = sqrt(Subs::sqr(profile.x-app->xref()) + Subs::sqr(profile.y-app->yref()))) < Reduce::aperture_tweak_max_shift){	      
		    
					    errors[nccd][naper] = Fxy(iprofile.ex, iprofile.ey);
					    // Update aperture position
					    app->set_xref(profile.x);
					    app->set_yref(profile.y);
		    
					}else{
		    
					    app->set_valid(false);
					    if(Reduce::abort_behaviour == Reduce::RELAXED){
						std::cerr << "5. CCD " << nccd + 1 << ", aperture " << naper+1 
							  << " shifted by more than the maximum. Shift = " << shift << " cf " << Reduce::aperture_tweak_max_shift << std::endl;
					    }else if(Reduce::abort_behaviour == Reduce::FUSSY){
						throw Ultracam_Error("Ultracam::rejig_apertures 5: fussy mode: CCD " + Subs::str(nccd+1) + ", aperture " + Subs::str(naper+1) +
								     " shifted by more than the maximum.");
					    }
					}		
				    }
				    catch(const std::string& err){
					if(Reduce::abort_behaviour == Reduce::FUSSY){
					    throw Ultracam_Error("Ultracam::rejig_apertures: fussy mode, position fit: " + err);
					}else if(Reduce::abort_behaviour == Reduce::RELAXED){
					    std::cerr << "Position fit, CCD " << nccd+1 << ", aperture " << naper+1 << ": " << err << std::endl;
					    app->set_valid(false);
					}
				    } 		
				}
			    }
	  
			    // Finally update the linked apertures
			    for(size_t naper=0; naper<aperture[nccd].size(); naper++){
	      
				app  = &aperture[nccd][naper];
				if(app->linked()){
		
				    app1 = &aperture[nccd][aperture_link[nccd][naper]];
				    errors[nccd][naper] = errors[nccd][aperture_link[nccd][naper]];
		
				    if(app->valid() && app1->valid()){
					app->set_xref(app1->xref());
					app->set_yref(app1->yref());
				    }else{
					app->set_valid(false);
				    }
				}
			    }
			}
		    }
		}
	    }
	}
    }
}

