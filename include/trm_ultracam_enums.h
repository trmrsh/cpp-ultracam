#ifndef TRM_ULTRACAM_ENUMS_H
#define TRM_ULTRACAM_ENUMS_H

// this file collects all enums and namespace dependent 
// forward declarations to be included in trm_ultracam.h
// which are not part of the Ultracam namespace

namespace Reduce {

    class Meanshape;
    class Point;
    class Moffset;
        
    //! Defines the behaviour on encountering problems
    enum ABORT_BEHAVIOUR {
	FUSSY,    /**< Give up the first sign of a problem */ 
	RELAXED   /**< Try to carry on regardless */ 
    };
    
    //! Sky estimation method
    enum SKY_METHOD {
	CLIPPED_MEAN,  /**< Mean after rejection of outliers */
	MEDIAN,        /**< Median (suffers from digitisation) */
	MODE           /**< Mode -- not implemented yet */
    };
    
    //! Method for estimating errors in sky
    enum SKY_ERROR {
	VARIANCE, /**< Work out fluctuations in the sky */
	PHOTON    /**< Work from the supplied readout and gain parameters */
    };
    
    //! Methods of flux extraction
    enum EXTRACTION_METHOD {
	NORMAL,     /**< Straight sum over the aperture */
	OPTIMAL     /**< Sum weighted according to a profile fit */
    };
    
    //! Error codes
    enum ERROR_CODES {
	OK, /**< All OK */
	COSMIC_RAY_DETECTED_IN_TARGET_APERTURE, /**< A cosmic ray/bad pixel was found in the aperture */
	SKY_OVERLAPS_EDGE_OF_WINDOW, /**< The sky annulus oversteps the edge of the CCD window */
	SKY_OVERLAPS_AND_COSMIC_RAY_DETECTED, /**< Sky overlaps edge of window and a bad pixel was found in the aperture */
	SKY_NEGATIVE, /**< Sky has a negative value */
	PEPPERED, /**< Data value above peppering threshold */
	NO_SKY, /**< No sky pixels found at all */
	EXTRA_APERTURES_IGNORED, /**< The extra apertures cannot be handled with optimally extraction */
	SATURATION, /**< Data value above saturation level */
	APERTURE_OUTSIDE_WINDOW, /**< Aperture was not inside any data window */
	TARGET_APERTURE_AT_EDGE_OF_WINDOW, /**< Aperture half in/half out of data window */
	APERTURE_INVALID, /**< The aperture was invalid */
	BLUE_IS_JUNK, /**< The blue frame was junk because of the co-add option */
    };
    
    //! Profile fit methods
    enum PROFILE_FIT_METHOD {
	GAUSSIAN, /**< Fit profiles with gaussians (symmetric or elliptical) */
	MOFFAT    /**< Fit profiles with moffat profiles */
    };
    
    //! How photometric apertures are re-jigged
    enum APERTURE_REPOSITION_MODE {
	STATIC, /**< No change in position */ 
	INDIVIDUAL, /**< Each changed one by one, offset apertures remaining locked to their references */ 
	INDIVIDUAL_PLUS_TWEAK, /**< Each changed, offset apertures refined as well */
	REFERENCE_PLUS_TWEAK /**< Reference stars first used to get shift, then individual as usual */
    };
    
    //! Level of terminal output
    enum TERM_OUT {
	NONE,    /**< No output at all */
	LITTLE,  /**< A small amount to show that something is happening */
	MEDIUM,  /**< Fairly full */
	FULL     /**< Quite a bit of output */
    };
    
    //! Units for X-axis of light-curve plots
    enum X_UNITS {
	SECONDS,  /**< Units of seconds */
	MINUTES,  /**< Units of minutes */
	HOURS,    /**< Units of hours */
	DAYS      /**< Units of days */
    };
    
    //! Types of aperture
    enum APERTURE_TYPE {
	FIXED,       /**< Fixed radii */
	VARIABLE     /**< Radii scaled by fitted FWHM */
    };
    
};

// sreduce enums

namespace Sreduce {

  //! Level of terminal output
  enum TERM_OUT {
    NONE,    /**< No output at all */
    LITTLE,  /**< A small amount to show that something is happening */
    MEDIUM,  /**< Fairly full */
    FULL     /**< Quite a bit of output */
  };

  //! How spectroscopic regions are re-jigged
  enum REGION_REPOSITION_MODE {
    STATIC,     /**< No change in position. */ 
    INDIVIDUAL, /**< Each changed one by one. */ 
    REFERENCE   /**< A single 'reference' star is used to measure the shift for all. */
  };

  //! Scaling of spectrum plots
  enum PLOT_SCALING_METHOD {
    DIRECT,     /**< user specified fixed limits. */ 
    AUTOMATIC,  /**< minimum to maximum. */ 
    PERCENTILE  /**< Percentile range. */
  };

  //! Defines the behaviour on encountering problems
  enum ABORT_BEHAVIOUR {
    FUSSY,         /**< Give up the first sign of a problem */ 
    RELAXED,       /**< Try to carry on regardless */ 
    VERY_RELAXED   /**< Don't even worry about bad times */ 
  };

  //! Error codes
  enum ERROR_CODES {
    OK,                          /**< All OK */
    SKY_OVERLAPS_EDGE_OF_WINDOW, /**< The sky regions overstep the edge of the CCD window */
    NO_SKY,                      /**< No valid sky at all */
    SATURATION,                  /**< Data value above saturation level */
    OBJECT_AT_EDGE_OF_WINDOW,    /**< Object half in/half out of data window */
    OBJECT_OUTSIDE_WINDOW,       /**< Object was not inside any data window */
    OBJECT_IN_MULTIPLE_WINDOWS,  /**< Object overlaps more thasn one window */
    REGION_INVALID,              /**< The extraction regions were invalid */
  };

};

#endif

