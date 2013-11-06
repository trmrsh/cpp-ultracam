#ifndef TRM_REDUCE_H
#define TRM_REDUCE_H

#include "trm/ultracam.h"

//! Namespace for 'reduce' related items
namespace Reduce {

  //! Structure for extraction control, 1 per CCD
  struct Extraction {

    //! Default constructor
    Extraction() :
      aperture_type(FIXED), extraction_method(NORMAL), 
	 star_scale(1.5f),      star_min(3.f),      star_max(10.f),
	 inner_sky_scale(2.5f), inner_sky_min(5.f), inner_sky_max(15.f),
	 outer_sky_scale(3.5f), outer_sky_min(15.f), outer_sky_max(30.f) {}


    //! General constructor
    Extraction(APERTURE_TYPE aperture_type, EXTRACTION_METHOD extraction_method, 
	       float star_scale,      float star_min,      float star_max,
	       float inner_sky_scale, float inner_sky_min, float inner_sky_max, 
	       float outer_sky_scale, float outer_sky_min, float outer_sky_max) :
      aperture_type(aperture_type), extraction_method(extraction_method), 
	 star_scale(star_scale),           star_min(star_min),           star_max(star_max),
	 inner_sky_scale(inner_sky_scale), inner_sky_min(inner_sky_min), inner_sky_max(inner_sky_max),
	 outer_sky_scale(outer_sky_scale), outer_sky_min(outer_sky_min), outer_sky_max(outer_sky_max) {}

    APERTURE_TYPE aperture_type;     /**< type of aperture */
    EXTRACTION_METHOD extraction_method; /**< extraction method to use */
    float star_scale;                /**< star radius scaling factor */
    float star_min;                  /**< minimum radius of star aperture, unbinned pixels */
    float star_max;                  /**< maximum radius of star aperture, unbinned pixels */
    float inner_sky_scale;           /**< inner radius of sky annulus scaling factor */
    float inner_sky_min;             /**< minimum inner radius of sky annulus, unbinned pixels */
    float inner_sky_max;             /**< maximum inner radius of sky annulus, unbinned pixels */
    float outer_sky_scale;           /**< outer radius of sky annulus scaling factor */
    float outer_sky_min;             /**< minimum outer radius of sky annulus, unbinned pixels */
    float outer_sky_max;             /**< maximum outer radius of sky annulus, unbinned pixels */
  };

  //! Stores shape parameters from profile fits
  struct Meanshape {

    //! Default constructor
    Meanshape() : 
      profile_fit_symm(true), profile_fit_method(MOFFAT), fwhm(0.), a(0.), b(0.), c(0.), beta(0.), set(false) {}

    bool profile_fit_symm; /**< Symmetric profile or not */
    PROFILE_FIT_METHOD profile_fit_method; /**< Method of profile fitting */            
    PROFILE_FIT_METHOD extraction_weights; /**< Weights to use when extracting */            
    double fwhm; /**< FWHM of profile fit */
    double a; /**< Coefficient of x*x */
    double b; /**< Coefficient of x*y */
    double c; /**< Coefficient of y*y */
    double beta; /**< Beta exponent of Moffat fit */
    bool set; /**< Is this shape structure set or not? */
  };

  //! Stores info for one aperture of one CCD
  struct Point{
    Point() : flux(0.), ferr(0.), xpos(0.), ypos(0.), fwhm(0.), code(OK), exposure(1.f), time_ok(true)  {}
    Point(float flx, float f_err, float xp, float yp, float fwhm, ERROR_CODES cde, float exposure, bool time_ok) :
        flux(flx), ferr(f_err), xpos(xp), ypos(yp), fwhm(fwhm), code(cde), exposure(exposure), time_ok(time_ok) {}
    float flux, ferr;
    double xpos, ypos;
    float fwhm;
    ERROR_CODES code;
    float exposure;
    bool time_ok;
  };

  //! Structure to store light curve plot information
  struct Laps {

    //! Default constructor
    Laps() {}

    //! General constructor
    Laps(size_t nccd_, size_t targ_, bool use_comp_, size_t comp_, float offset_, Subs::PLOT_COLOUR colour_, Subs::PLOT_COLOUR errcol_) 
      : nccd(nccd_), targ(targ_), use_comp(use_comp_), comp(comp_), offset(offset_), colour(colour_), errcol(errcol_) {}

    //! CCD number
    size_t nccd;

    //! Target aperture number
    size_t targ;

    //! Bother with a comparison or not
    bool use_comp;

    //! Comparison aperture number
    size_t comp;

    //! Offset
    float offset;

    //! Colour to plot point
    Subs::PLOT_COLOUR colour;

    //! Colour to plot error bars
    Subs::PLOT_COLOUR errcol;

  };

  //! Structure to store info for position plots
  struct Paps {

    //! Default constructor
    Paps() {}

    //! General constructor
    Paps(size_t nccd_, size_t targ_, float off_, Subs::PLOT_COLOUR colour_, Subs::PLOT_COLOUR errcol_) 
      : nccd(nccd_), targ(targ_), off(off_), colour(colour_), errcol(errcol_) {}

    //! CCD number
    size_t nccd;

    //! Target aperture number
    size_t targ;

    //! Offset
    float off;

    //! Colour to plot points
    Subs::PLOT_COLOUR colour;

    //! Colour to plot error bars
    Subs::PLOT_COLOUR errcol;
  };

  //! Structure to store info for transmission plots
  struct Taps {

    //! Default constructor
    Taps() {}

    //! General constructor
    Taps(size_t nccd_, size_t targ_, Subs::PLOT_COLOUR colour_) 
      : nccd(nccd_), targ(targ_), colour(colour_), fmax(0.) {}

    //! CCD number
    size_t nccd;

    //! Target aperture number
    size_t targ;

    //! Colour to plot points
    Subs::PLOT_COLOUR colour;

    //! Maximum flux measured so far
    float fmax;

  };

  //! Structure to store info for seeing plot
  struct Faps {

    //! Default constructor
    Faps() {}

    //! General constructor
    Faps(size_t nccd_, size_t targ_, Subs::PLOT_COLOUR colour_) 
      : nccd(nccd_), targ(targ_), colour(colour_) {}

    //! CCD number
    size_t nccd;

    //! Target aperture number
    size_t targ;

    //! Colour to plot points
    Subs::PLOT_COLOUR colour;

  };

  //! Position offset information structure for an aperture
  struct Offset {
    
    //! Default constructor
    Offset() :
      x(0.), y(0.), xe(1.), ye(1.), ok(false) {}

    //! General constructor
    Offset(double x, double y, double xe, double ye, bool ok) :
      x(x), y(y), xe(xe), ye(ye), ok(ok) {}

    //! X position offset
    double x;

    //! Y position offset
    double y;

    //! X position offset error
    double xe;

    //! Y position offset error
    double ye;

    //! Are the values thought to be OK
    bool ok;
  };

  //! Structure for storage of two-pass information
  struct Twopass {

    //! Default constructor
    //    Twopass() : time(0.), shape(), ref_pos(), ref_valid(), offset() {}

    //! Time for this point
    double time;

    //! Reference positions for all CCDs
    std::vector<Reduce::Meanshape> shape;

    //! Reference positions for all CCDs
    std::vector<Ultracam::Fxy> ref_pos;

    //! Validity of reference positions for all CCDs
    std::vector<bool> ref_valid;

    //! Offsets for all apertures of all CCDs
    std::vector<std::vector<Offset> > offset;
  };

};

#endif
