#ifndef TRM_ULTRACAM_TARGET_H
#define TRM_ULTRACAM_TARGET_H

#include <iostream>
#include <string>
#include "trm_ultracam.h"


namespace Ultracam {

  //! The Target class represents artificial sources
  
  /** Target objects define artificial targets for test purposes. The sources are
   * represented by a 2D Moffat profile of the form
   * \f$ h \frac{1}{\left(1+ a_{xx}(x-xc)^2+2 a_{xy}(x-xc)(y-yc)+a_{yy}(y-yc)^2\right)^\beta}\f$
   * where \f$(xc,yc)\f$ is the centre of the source and the various \c a coefficients
   * define the width and orientation. The height h is actually set by the total number of counts
   * in the profile which equals \f$ \pi*a*b/(\beta-1) \f$ where a and b are the semi-major and semi-minor
   * lengths correspoinding to the axx etc.
   */
  class Target{
    
  public:
    
    //! Name to use when referring to these objects in the singular
    static std::string name(){return "target";}
    
    //! Name to use when referring to these objects in the plural
    static std::string plural_name(){return "targets";}
    
    //! Extension to use when saving in MCCD form
    static std::string extnam(){return ".str";}
    
    //! Default constructor
    Target() : xc(0.), yc(0.), counts(1000.), axx(0.01), axy(0.), ayy(0.01), mbeta(3.) {}
    
    //! General constructor
    Target(float xc, float yc, float counts, float axx, float axy, float ayy, double beta);
    
    
    //! Returns height
    float  height() const;
    
    //! Returns X ordinate of centre
    float  get_xc() const {return xc;}
    
    //! Returns Y ordinate of centre
    float  get_yc() const {return yc;}
    
    //! Returns \f$a_{xx}\f$
    float  get_axx() const {return axx;}
    
    //! Returns \f$a_{xy}\f$
    float  get_axy() const {return axy;}
    
    //! Returns \f$a_{yy}\f$
    float  get_ayy() const {return ayy;}
    
    //! Returns \f$a_{yy}\f$
    float  get_counts() const {return counts;}
    
    //! Returns \f$\beta\f$
    double get_beta() const {return mbeta;}
    
    //! Sets the total number of counts
    void set_counts(float counts) {this->counts = counts;}
    
    //! Sets X ordinate of centre
    void set_xc(float xc) {this->xc = xc;}
    
    //! Sets Y ordinate of centre
    void set_yc(float yc) {this->yc = yc;}
    
    //! Sets \f$a_{xx}\f$
    void set_axx(float axx);
    
    //! Sets \f$a_{xy}\f$
    void set_axy(float axy);
    
    //! Sets \f$a_{yy}\f$
    void set_ayy(float ayy);
    
    //! Sets \f$\beta\f$
    void set_beta(double beta);
    
    //! Blurr the profile parameters
    void blurr(float seeing);
    
    // Set all a coefficients together
    void set(float axx, float axy, float ayy);
    
    // In order for object deletion in CCD to work.
    
    //! A measure of distance from a point
    float how_far(float x, float y) const;
    
    //! Is a Target near enough to a point to be selected?
    bool near_enough(float x, float y) const;
    
    //! Computes the height of the profile at given offset from centre
    float height(float dx, float dy) const {
      return height()*(pow(1. + dx*(get_axx()*dx+2.*get_axy()*dy) + get_ayy()*dy*dy,-mbeta));
    }
    
    //! Compute distance in X and Y to go to get into wings of profile
    void dist(float level, float& dx, float& dy) const;
    
    friend std::ostream& operator<<(std::ostream& s, const Target& obj);
    
    friend std::istream& operator>>(std::istream& s, Target& obj);
    
    friend bool clash(const Target& target1, const Target& target2);
    
  private:
    float  xc, yc;
    float  counts;
    float  axx, axy, ayy;
    float det() const {return axx*ayy-axy*axy;}
    double mbeta;
  };
  
  //! Plot a source as an ellipse
  void pgline(const Target& target, float level=1.);
  
  //! Label a source
  void pgptxt(const Target& target, const std::string& label, 
	      float level=1.);
  
  //! Checks a set of \c a coefficients
  bool bad_targ(float axx, float axy, float ayy);

  //! ASCII output
  std::ostream& operator<<(std::ostream& s, const Target& obj);
    
  //! ASCII input
  std::istream& operator>>(std::istream& s, Target& obj);
    
  //! Do two Targets clash?
  bool clash(const Target& target1, const Target& target2);

};

#endif
  
