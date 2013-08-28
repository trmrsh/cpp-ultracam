#ifndef TRM_ULTRACAM_TRACE_H
#define TRM_ULTRACAM_TRACE_H

#include <iostream>
#include <string>
#include "trm/poly.h"
#include "trm/ultracam.h"


namespace Ultracam {

  //! The Trace class defines objects which trace spectra
  
  /** Trace objects represent the positions of spectra, which are assumed to lie almost
   * parallel to the rows so that Trace object give the Y position as a function of X.
   * started 22/11/06
   */
  
  class Trace {
    
  public:
    
    //! Name to use when referring to these objects in the singular
    static std::string name(){return "trace";}

    //! Name to use when referring to these objects in the plural
    static std::string plural_name(){return "traces";}

    //! Extension to use when saving in MCCD form
    static std::string extnam(){return ".trc";}

    //! Default constructor
    Trace() : 
      position(), fwhm(), continuum(), lines(), xcen(), told(0.), compute_positions(true) {}

    //! Constructor from a file
    Trace(const std::string& file);
    
    // In order for object deletion in CCD to work.
    
    //! A measure of distance from a point
    float how_far(float x, float y) const; 

    //! Do two Traces clash?
    friend bool clash(const Trace& trace1, const Trace& trace2);
          
    //! ASCII output
    friend std::ostream& operator<<(std::ostream& s, const Trace& obj);
    
    //! ASCII input
    friend std::istream& operator>>(std::istream& s, Trace& obj);
    
    //! Get Y position at x
    double get_position(double x) const;

    //! Set the position
    void set_position(const Subs::Poly& position);

  private:
    
    // Y position as a function of X
    Subs::Poly position;
    
    // Integer identifier that must be unique for all Traces on a single CCD
    int id;

    // Link this trace to another
    bool linked;

    // ID of the Trace this is linked to if any
    int link_id;

    // Offset in Y if linked
    Subs::Poly offset;
    
  };

  std::ostream& operator<<(std::ostream& s, const Trace::Line& line); 

  std::istream& operator>>(std::istream& s, Trace::Line& line); 

};

#endif

