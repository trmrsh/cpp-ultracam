#ifndef TRM_ULTRACAM_APERTURE_H
#define TRM_ULTRACAM_APERTURE_H

// The code starts here

#include <iostream>
#include <string>
#include "trm_ultracam.h"

namespace Ultracam {

  //! The Aperture class defines objects which represent photometric apertures.

  /** Aperture objects describe circular photometric apertures. They contain information on
   * the position and sizes of the photometric apertures (object plus sky annulus) as well as
   * data on whether the apertures are tied to each other and their current status. 
   *
   * Apertures are defined by an offset from a reference target which can be used to refine
   * their positions if need be. Typically the offset will be 0,0 if the reference object is
   * the target itself. Apertures themselves can be marked as 'reference' apertures,
   * i.e. ones of special significance, perhaps for measurement of seeing for instance.
   *
   * Aperture objects also can carry a list of offsets to stars that should be eliminated 
   * during sky estimation as well as stars that should be included in order to reduce the 
   * chances of contamination or poor photometry owing to nearby stars.
   *
   * The idea behind adding another star is as follows. Suppose you are carrying out photometry 
   * of an object which has a nearby star only 3" away. If you try to ignore this star it will
   * probably ruin your photometry as seeing sometimes makes it add more or less flux. A common
   * technique is to bite the bullet and create an aperture that includes both stars. This is
   * of course unecessarily large. The idea here is that you can define offset apertures whose
   * pixels will be added in without repetition to give the whole flux.
   */

  class Aperture {

  public:

    //! Name to use when referring to these objects in the singular
    static std::string name(){return "aperture";}
    
    //! Name to use when referring to these objects in the plural
    static std::string plural_name(){return "apertures";}
    
    //! Extension to use when saving in MCCD form
    static std::string extnam(){return ".ape";}

    /*! \brief Special colour for reference apertures.
     *
     * This is a PGPLOT colour index to use when plotting reference apertures.
     */
    static int Ref_col;

    /*! \brief Special colour for invalid apertures.
     *
     * This is a PGPLOT colour index to use when plotting invalid apertures.
     */
    static int Invalid_col;

    /*! \brief Default constructor.
     *
     * Constructs a default aperture, positioned at 0,0 with radii 1, 2, 3.
     */
    Aperture() : x_r(0.), y_r(0.), x_off(0.), y_off(0.), 
		 r_star(1.), r_sky1(2.), r_sky2 (3.), ref_star(false), ap_ok(true), mask_() {};

    //! Full constructor (except for mask)
    Aperture(double xr, double yr, double xoff, double yoff, float rstar, float rsky1, 
	     float rsky2, bool ref=false);
  
    /*! \brief Returns X reference position
     *
     * This function comes back with the X position of the reference target
     * of this aperture. The value returned is measured in unbinned pixels.
     */
    double xref() const {return x_r;}

    /*! \brief Returns Y reference position
     *
     * This function comes back with the Y position of the reference target
     * of this aperture. The value returned is measured in unbinned pixels.
     */
    double yref() const {return y_r;}

    /*! \brief Returns X position.
     *
     *   This function comes back with the X position of the aperture, which
     *   is the sum of the reference position and offset from it.
     */
    double xpos() const {return x_r+x_off;}

    /*! \brief Returns Y position.
     *
     *   This function comes back with the Y position of the aperture, which
     *   is the sum of the reference position and offset from it.
     */
    double ypos() const {return y_r+y_off;}

    /*! \brief Returns X offset.
     *
     *   This function comes back with the X offset of the aperture from its
     *   reference target.
     */
    double xoff() const {return x_off;}

    /*! \brief Returns Y offset.
     *
     * This function comes back with the Y offset of the aperture from its
     * reference target.
     */
    double yoff() const {return y_off;}

    /*! \brief Returns radius of star aperture.
     *
     * This function comes back with the radius of the (inner) star aperture
     * measured in unbinned pixels.
     */
    float rstar() const {return r_star;}

    /*! \brief Returns inner radius of sky annulus.
     *
     * This function comes back with the inner radius of the sky annulus
     * measured in unbinned pixels.
     */
    float rsky1() const {return r_sky1;}

    /*! \brief Returns inner radius of sky annulus.
     *
     * This function comes back with the outer radius of the sky annulus
     * measured in unbinned pixels.
     */
    float rsky2() const {return r_sky2;}

    //! Is this a reference aperture or not?
    bool ref() const {return ref_star;}

    //! Is this aperture valid or not?
    bool valid() const {return ap_ok;}

    //! Is this aperture linked or not?
    bool linked() const {return (x_off != 0. || y_off != 0.);}

    // Setting

    /*! \brief Sets the X position of the reference object.
     *
     * This function sets the X position of the object to which the aperture
     * \param xref the X position of the reference object (unbinned pixels)
     */
    void set_xref(double xref){x_r = xref;}

    /*! \brief Sets the Y position of the reference object.
     *
     * This function sets the Y position of the object to which the aperture
     * \param yref the X position of the reference object (unbinned pixels)
     */
    void set_yref(double yref){y_r = yref;}

    /*! \brief Sets the X offset from the reference object.
     *
     * This function sets the X offset from the reference object to the aperture.
     * \param xoff the offset in X from reference to aperture (unbinned pixels)
     */
    void set_xoff(double xoff){x_off = xoff;}

    /*! \brief Sets the Y offset from the reference object.
     *
     * This function sets the Y offset from the reference object to the aperture.
     * \param yoff the offset in Y from reference to aperture (unbinned pixels)
     */
    void set_yoff(double yoff){y_off = yoff;}

    //! Sets the radius of the star aperture
    void set_rstar(float rstar);

    //! Sets the inner radius of the sky annulus
    void set_rsky1(float rsky1);

    //! Sets the outer radius of the sky annulus
    void set_rsky2(float rsky2);

    //! Sets all aperture radii
    void set_radii(float rstar, float rsky1, float rsky2);

    //! Sets aperture to be a reference one or not

    /** set_ref sets a flag to say that a given aperture
     * counts as a 'reference' aperture which means for instance
     * that it would be used for profile fits.
     * \param ref true/false to set reference status of aperture.
     */
    void set_ref(bool ref){ref_star = ref;}

    //! Sets aperture validity state.

    /** set_valid sets the validity state of an aperture. It can be used to
     * indicate that an aperture has become ciorrupted in some way.
     * \param valid  true/false to set validity status of aperture.
     */
    void set_valid(bool valid){ap_ok = valid;}

    //! Sets all position and size info at once.
    void set(double xr, double yr, double xoff, 
	     double yoff, float rstar, float rsky1, 
	     float rsky2);
      
    // In order for object deletion in CCD to work.

    //! Returns indication of distance of an Aperture from a position
    float how_far(float x, float y) const;

    //! Is an Aperture "near enough" to a point?
    bool near_enough(float x, float y) const;

    friend std::ostream& operator<<(std::ostream& s, const Aperture& obj);

    friend std::istream& operator>>(std::istream& s, Aperture& obj);

    friend bool clash(const Aperture& obj1, const Aperture& obj2);

    //! Adds an offset to the mask list
    void push_back(const Ultracam::sky_mask& skymask){
      mask_.push_back(skymask);
    }

    //! Returns numbers of masks associated with this aperture
    int nmask() const {return mask_.size();}

    //! Gets the i-th mask
    const Ultracam::sky_mask& mask(int i) const {return mask_[i];}

    //! Gets the whole mask
    const std::vector<Ultracam::sky_mask>& mask() const {return mask_;}

    //! Deletes mask i
    void del_mask(int i);

    //! Adds an extra star 
    void push_back(const Ultracam::extra_star& extra){
      extra_.push_back(extra);
    }

    //! Returns numbers of extra stars associated with this aperture
    int nextra() const {return extra_.size();}

    //! Gets the i-th extra star
    const Ultracam::extra_star& extra(int i) const {return extra_[i];}

    //! Gets the whole mask
    const std::vector<Ultracam::extra_star>& extra() const {return extra_;}

    //! Deletes extra star i
    void del_extra(int i);

  private:

    double x_r, y_r;      // reference position
    double x_off, y_off;  // offset from reference position, = 0 if not linked.
    float r_star, r_sky1, r_sky2; // aperture radii
    bool ref_star;              // use this as a position reference or not
    bool ap_ok;                 // Is this aperture valid?
    std::vector<Ultracam::sky_mask> mask_;       // List of offsets to stars that need to be masked from the sky.
    std::vector<Ultracam::extra_star> extra_;       // List of offsets to stars that need to be added into target's flux
  };

  // Related function declarations.

  //! Draw an Aperture
  void pgline(const Aperture& aperture);

  //! Label an Aperture
  void pgptxt(const Aperture& aperture, const std::string& lab);

  //! Check potential Aperture radii.
  bool bad_aper(float rstar, float rsky1, float rsky2);

    //! Extractor.
    std::ostream& operator<<(std::ostream& s, const Aperture& obj);

    //! Inserter.
    std::istream& operator>>(std::istream& s, Aperture& obj);

    //! Do two Apertures clash?
    bool clash(const Aperture& obj1, const Aperture& obj2);
};

#endif





















