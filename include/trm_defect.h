#ifndef TRM_ULTRACAM_DEFECT_H
#define TRM_ULTRACAM_DEFECT_H

#include <iostream>
#include <string>
#include "trm_ultracam.h"

namespace Ultracam {

  //! The Defect class defines object which represent CCD defects for helping position objects

  /**
   * The class can store two type of defect, which are 'pixel' and 'line' defects. Line defects
   * are normally parallel to row or columns of course but no restriction is placed on them
   * in this class. Each defect contains an indication of its severity.
   */

  class Defect{

  public:

    //! Name to use when referring to these objects in the singular
    static std::string name(){return "defect";}
    
    //! Name to use when referring to these objects in the plural
    static std::string plural_name(){return "defects";}
    
    //! Extension to use when saving in MCCD form
    static std::string extnam(){return ".dft";}

    //! Severity indicator
    enum how_bad {MODERATE, DISASTER};

    //! Default constructor.
    Defect() : x1_(0), y1_(0), x2_(0), y2_(0), severity_(MODERATE), cps_(-1) {} 

    //! Pixel defect constructor
    Defect(float x, float y, how_bad severity);

    //! Hot pixel defect constructor
    Defect(float x, float y, int cps);

    //! Line defect constructor
    Defect(float x1, float y1, float x2, float y2, how_bad severity);
  
    //! Returns indication of distance of an Defect from a position
    float how_far(float x, float y) const;

    //! Is a Defect "near enough" to a point?
    bool near_enough(float x, float y) const;

    //! Returns x1
    float x1() const {return x1_;}

    //! Returns y1
    float y1() const {return y1_;}

    //! Returns x2
    float x2() const {return x2_;}

    //! Returns y2
    float y2() const {return y2_;}

    //! Returns severity
    how_bad effect() const {return severity_;}

    //! Returns hotness of a hot pixel
    int how_hot() const {return cps_;}

    //! Is it a pixel defect?
    bool is_a_pixel() const {return (x1_ == x2_ && y1_ == y2_ && cps_ == -1);}

    //! Is it a hot pixel?
    bool is_a_hot_pixel() const {return cps_ > -1;}

    //! Value to set a bad pixel 
    float bad_value(int ix, int iy, float low, float high) const;

    friend std::ostream& operator<<(std::ostream& s, const Defect& obj);

    friend std::istream& operator>>(std::istream& s, Defect& obj);

    friend bool clash(const Defect& obj1, const Defect& obj2);

    //! Transforms Defect position
    void transform(const Ultracam::Transform& trans, bool forward);

  private:

    float x1_, y1_, x2_, y2_;
    how_bad severity_;
    int cps_;

  };

  //! Draw a Defect
  void pgline(const Defect& defect);

  //! Draw a Defect
  void pgtxtline(const Defect& defect);

  //! Label a Defect
  void pgptxt(const Defect& defect, const std::string& lab);

    //! Extractor.
    std::ostream& operator<<(std::ostream& s, const Defect& obj);

    //! Inserter.
    std::istream& operator>>(std::istream& s, Defect& obj);

    //! Do two Defects clash?
    bool clash(const Defect& obj1, const Defect& obj2);
};

#endif





















