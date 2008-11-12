#ifndef TRM_ULTRACAM_SPECAP_H
#define TRM_ULTRACAM_SPECAP_H

// The code starts here

#include <iostream>
#include <string>
#include "trm_ultracam.h"

namespace Ultracam {

  //! The Specap class defines objects which represent spectroscopic regions

  /** Specap objects describe sky and object regions for spectrum extraction. They
   * each consist of one object plus associated sky regions.
   */

  class Specap {

  public:

    //! Structure of the basic information needed for each sky region.
    struct Skyreg {

      // start and end of the region and a reference X position (needed for
      // tilted regions in the future)
      double ylow, yhigh;

      // whether the region represent good or bad sky and whether it should be fixed
      // in position or move with the object
      bool good, fixed;

      //! Default constructor
      Skyreg() : ylow(0.), yhigh(1.), good(true), fixed(false) {}

      //! General constructor
      Skyreg(double ylow, double yhigh, bool good, bool fixed) :
	ylow(ylow), yhigh(yhigh), good(good), fixed(fixed) {
	if(ylow > yhigh)
	  throw Ultracam_Error("Specap::Skyreg(double, double, bool, bool): ylow > yhigh");
      }
      
      //! Extractor of Skyreg-s
      friend std::ostream& operator<<(std::ostream& s, const Specap::Skyreg& obj);
      
      //! Inserter of Skyreg-s
      friend std::istream& operator>>(std::istream& s, Specap::Skyreg& obj);

    };

    //! Name to use when referring to these objects in the singular
    static std::string name(){return "spectrum extraction region";}
    
    //! Name to use when referring to these objects in the plural
    static std::string plural_name(){return "spectrum extraction regions";}
    
    //! Extension to use when saving in MCCD form
    static std::string extnam(){return ".spa";}

    //! Default constructor.
    /**
     * Constructs a default specap
     */
    Specap() : yslow(0.), ylow(1.), ypos(2.), yhigh(3.), yshigh(4.), xleft(0.), xright(1.), sky_regions() {}
    
    //! Constructor from object data (no sky)
    /** \param yslow the lower edge of the region to be searched when re-locating the object
     * \param ylow the lower edge of the region to extract the object flux. Must be > yslow.
     * \param ypos   the position of the object. Must be > ylow
     * \param yhigh   the upper edge of the region to extract the object flux. Must be > ypos.
     * \param yshigh  the upper edge of the region to be searched when re-locating the object. Must be > yhigh.
     * \param xref   Position in X at which all the Y values are effectively measured 
     */
    Specap(double yslow, double ylow, double ypos, double yhigh, double yshigh, bool pos_is_accurate, double xleft, double xright) : 
      yslow(yslow), ylow(ylow), ypos(ypos), yhigh(yhigh), yshigh(yshigh), pos_is_accurate(pos_is_accurate), xleft(xleft), xright(xright), sky_regions() {

      if(yslow > ylow)
	throw Ultracam_Error("Specap(double, double, double, double, double, double, double): yslow > ylow");
      if(ylow > ypos)
	throw Ultracam_Error("Specap(double, double, double, double, double, double, double): ylow > ypos");
      if(ypos > yhigh)
	throw Ultracam_Error("Specap(double, double, double, double, double, double, double): ypos > yhigh");
      if(yhigh > yshigh)
	throw Ultracam_Error("Specap(double, double, double, double, double, double, double): yhigh > yshigh");
      if(xleft > xright)
	throw Ultracam_Error("Specap(double, double, double, double, double, double, double): xleft > xright");
    }
  
    // In order for object deletion in CCD to work.

    //! Returns indication of distance of an Specap from a position
    float how_far(float x, float y) const;

    //! Is an Specap "near enough" to a point?
    bool near_enough(float x, float y) const;

    //! Gets the lower end of object search range
    double get_yslow() const {return yslow;}

    //! Gets the lower edge of object region
    double get_ylow() const {return ylow;}

    //! Gets the object position
    double get_ypos() const {return ypos;}

    //! Gets the Y end value
    double get_yhigh() const {return yhigh;}

    //! Gets the upper end of object search range
    double get_yshigh() const {return yshigh;}

    //! Is the position stored an accurate one?
    bool is_pos_accurate() const {return pos_is_accurate;}

    //! Gets the left-hand X extraction limit
    double get_xleft() const {return xleft;}

    //! Gets the right-hand X extraction limit
    double get_xright() const {return xright;}

    //! Sets the lower end of object search range
    void set_yslow(double yslow);

    //! Sets the lower edge of object extraction region
    void set_ylow(double ylow);

    //! Sets the object position
    void set_ypos(double ypos);

    //! Sets the Y end value
    void set_yhigh(double yhigh);

    //! Sets the upper edge of object search range
    void set_yshigh(double yshigh);

    //! Sets the left-hand X extraction limit
    void set_xleft(double xleft);

    //! Sets the right-hand X extraction limit
    void set_xright(double xright);

    //! Do two Specaps clash?
    friend bool clash(const Specap& obj1, const Specap& obj2);

    //! Adds in a sky region
    void push_back(const Skyreg& skyreg){
      sky_regions.push_back(skyreg);
    }

    //! Returns numbers of sky regions associated 
    int nsky() const {return sky_regions.size();}

    //! Gets the i-th sky region
    const Skyreg& sky(int i) const;

    //! Deletes sky region i
    void delete_sky(int i);

    //! Deletes all the sky region
    void delete_sky() {
      sky_regions.clear();
    }

    //! Moves a region
    void add_shift(double shift);

    //! Checks for a unique window overlap
    int unique_window(const CCD<Windata>& windows) const;

    //! Extractor of Specaps
    friend std::ostream& operator<<(std::ostream& s, const Specap& obj);
    
    //! Inserter of Specaps
    friend std::istream& operator>>(std::istream& s, Specap& obj);

  private:

    // Start of object search range, left edge of extraction region, object position, right edge of extraction region,
    // end of search range
    double yslow, ylow, ypos, yhigh, yshigh;

    // Flag to say whether the position was measured accurately or not
    bool pos_is_accurate;

    // Start and end of extraction region in X
    double xleft, xright;

    // Associated sky regions
    std::vector<Skyreg> sky_regions;

  };

  //! Plots a Specap
  void pgline(const Specap& specap, bool profile);

};

#endif





















