#ifndef TRM_ULTRACAM_SKYLINE_H
#define TRM_ULTRACAM_SKYLINE_H

#include <iostream>
#include <string>
#include "trm/poly.h"
#include "trm/ultracam.h"


namespace Ultracam {

  //! The Skyline class defines a sky line for testing purposes
  
  /** Skyline objects define night sky emission lines for test purposes. They are represented by polynomials
   * as a function of Y for their position and width.
   */
  
  class Skyline {
    
  public:
    
    //! Name to use when referring to these objects in the singular
    static std::string name(){return "skyline";}

    //! Name to use when referring to these objects in the plural
    static std::string plural_name(){return "skylines";}

    //! Extension to use when saving in MCCD form
    static std::string extnam(){return ".sky";}

    //! Default constructor
    Skyline() : 
      position(), fwhm(), strength(0) {}

    //! Constructor from a file
    Skyline(const std::string& file);
    
    // In order for object deletion in CCD to work.
    
    //! A measure of distance from a point
    float how_far(float x, float y) const; 

    friend bool clash(const Skyline& skyline1, const Skyline& skyline2);
          
    friend std::ostream& operator<<(std::ostream& s, const Skyline& obj);
    
    friend std::istream& operator>>(std::istream& s, Skyline& obj);
    
    //! Get X position at y
    double get_position(double y) const;

    //! Get FWHM at x
    double get_fwhm(double y) const;

    //! Get total counts across the line
    double get_strength() const;

    //! Set the position
    void set_position(const Subs::Poly& position);

    //! Set the FWHM
    void set_fwhm(const Subs::Poly& fwhm);

    //! Set the strength
    void set_strength(double strength);

  private:
    
    // X position as a function of Y
    Subs::Poly position;
    
    // X FWHM as a function of Y
    Subs::Poly fwhm;
    
    // Total counts across the line
    double strength;
    
  };

  //! Do two skyliness clash?
  bool clash(const Skyline& skyline1, const Skyline& skyline2);
  
  //! ASCII output
  std::ostream& operator<<(std::ostream& s, const Skyline& obj);
  
  //! ASCII input
  std::istream& operator>>(std::istream& s, Skyline& obj);
};

#endif

