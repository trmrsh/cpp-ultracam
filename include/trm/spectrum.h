#ifndef TRM_ULTRACAM_SPECTRUM_H
#define TRM_ULTRACAM_SPECTRUM_H

#include <iostream>
#include <string>
#include "trm/poly.h"
#include "trm/ultracam.h"


namespace Ultracam {

  //! The Spectrum class defines a spectrum for testing purposes
  
  /** Spectrum objects define spectra for test purposes. Spectra are represented by polynomials
   * for their continuua, positions and widths as a function of X (the dispersion direction). They 
   * can have atomic lines with gaussian profiles that can move if wanted. 
   */
  
  class Spectrum {
    
  public:
    
    //! Name to use when referring to these objects in the singular
    static std::string name(){return "spectrum";}

    //! Name to use when referring to these objects in the plural
    static std::string plural_name(){return "spectra";}

    //! Extension to use when saving in MCCD form
    static std::string extnam(){return ".spc";}

    //! Structure to define an atomic line
    struct Line {
      
      //! Default constructor
      Line() : centre(1000.), height(1.), fwhm(10.), T0(50000.), period(1.), semiamp(0.) {}

      //! Constructor without motion
      Line(double centre, double height, double fwhm) :
	centre(centre), height(height), fwhm(fwhm), T0(50000.), period(1.), semiamp(0.) {}
      
      //! General constructor
      Line(double centre, double height, double fwhm, double T0, double period, double semiamp) :
	centre(centre), height(height), fwhm(fwhm), T0(T0), period(period), semiamp(semiamp) {}
      
      double centre;   /**< The line centre, pixels */
      double height;   /**< The line peak height */
      double fwhm;     /**< The line FWHM, pixels */
      double T0;       /**< Zero-point for ephemeris, MJD */
      double period;   /**< Period for ephemeris, days */
      double semiamp;  /**< Semi-amplitude of sinusoidal motion, pixels. */
    };
    
    //! Default constructor
    Spectrum() : 
      position(), fwhm(), continuum(), lines(), xcen(), told(0.), compute_positions(true) {}

    //! Constructor from a file
    Spectrum(const std::string& file);
    
    // In order for object deletion in CCD to work.
    
    //! A measure of distance from a point
    float how_far(float x, float y) const; 

    friend bool clash(const Spectrum& spectrum1, const Spectrum& spectrum2);
          
    friend std::ostream& operator<<(std::ostream& s, const Spectrum& obj);
    
    friend std::istream& operator>>(std::istream& s, Spectrum& obj);
    
    //! Get Y position at x
    double get_position(double x) const;

    //! Get FWHM at x
    double get_fwhm(double x) const;

    //! Get total counts in the continuum at x
    double get_continuum(double x) const;

    //! Get total counts in the lines at x and time
    double get_line(double x, double time);

    //! Get total counts in the lines at x
    double get_line(double x) const;

    //! Add a line
    void add_line(const Line& line);

    //! Set the position
    void set_position(const Subs::Poly& position);

    //! Set the FWHM
    void set_fwhm(const Subs::Poly& fwhm);

    //! Set the continuum
    void set_continuum(const Subs::Poly& continuum);

  private:
    
    // Y position as a function of X
    Subs::Poly position;
    
    // Y FWHM as a function of X
    Subs::Poly fwhm;
    
    // Total counts in the continuum as a function of X
    Subs::Poly continuum;
    
    // The lines 
    Subs::Buffer1D<Line> lines;

    // Time saver stuff. Stores line position for a 
    // particular time
    Subs::Array1D<double> xcen;

    // Stores the times used
    double told;

    // Lines have just been reset, so force computation of positions if this is true
    // Any function that alters 'lines' must set this to true
    bool compute_positions;
    
  };

  std::ostream& operator<<(std::ostream& s, const Spectrum::Line& line); 

  std::istream& operator>>(std::istream& s, Spectrum::Line& line); 

  //! Do two spectrums clash?
  bool clash(const Spectrum& spectrum1, const Spectrum& spectrum2);
          
  //! ASCII output
  std::ostream& operator<<(std::ostream& s, const Spectrum& obj);
    
  //! ASCII input
  std::istream& operator>>(std::istream& s, Spectrum& obj);
  
};

#endif

