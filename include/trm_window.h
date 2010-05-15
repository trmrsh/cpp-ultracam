#ifndef TRM_ULTRACAM_WINDOW_H
#define TRM_ULTRACAM_WINDOW_H

#include <iostream>
#include <fstream>
#include <string>
#include "trm_ultracam.h"

namespace Ultracam {

  //! The Window class defines objects which represent a single window
  
  /** Window objects define a readout region of a CCD. They store the position,
   * size and binning factors of the window, and also the total unbinned dimensions
   * of the CCD. The latter allows them to be plotted in context.
   * Window objects do not store any data.
   *
   * Window positions are measured from the lower-left corner of a CCD. Their dimensions
   * are stored in binned pixels. They can plot themselves, be output, written to disk
   * etc. 
   *
   * Note that the positions within a CCD obey the convention that the lower-left unbinned pixel
   * is (1,1), but pixels within a window start at (0,0). This makes it easy to add window coordinates
   * to the lower-left position to get CCD coordinates, and to obey standard indexing when winding
   * through window pixels.
   */
  
  class Window{
    
  public:
    
    // Some limits to guard against stupid values
    
    //! Maximum unbinned X dimension of any CCD
    static const int  MAX_NXTOT = 10000; 
    //! Maximum unbinned Y dimension of any CCD
    static const int  MAX_NYTOT = 10000;
    //! Maximum binning factor in X
    static const int MAX_XBIN  = 100;
    //! Maximum binning factor in Y
    static const int MAX_YBIN  = 100;
    
    //! Name to use when referring to these objects in the singular
    static std::string name(){return "window";}
    
    //! Name to use when referring to these objects in the plural
    static std::string plural_name(){return "windows";}
    
    //! Extension to use when saving in MCCD form
    static std::string extnam(){return ".win";}
    
    //! Default constructor
    /*
     * Constructs a 1 by 1 unbinned window at the lower-left corner of the CCD
     */
    Window() : 
      ll_x(1), ll_y(1), n_x(1), n_y(1), x_bin(1), y_bin(1), 
      nx_tot(1), ny_tot(1) {}
    
    //! General constructor
    /*
     * Constructs a general window.
     * \param llx the left-most pixel of the window, starting from 1
     * \param lly the bottom pixel of the window, starting from 1
     * \param nx  the binned X dimension of the window
     * \param ny  the binned Y dimension of the window
     * \param xbin  the binning factor in X
     * \param ybin  the binning factor in Y
     * \param nxtot  the unbinned X dimension of the CCD containing the window
     * \param nytot  the unbinned Y dimension of the CCD containing the window
     */
    Window(int llx, int lly, int nx, int ny, int xbin, int ybin, int nxtot, int nytot);
    
    // Retrieve data
    
    /** @name Get methods
     * These are methods for retrieval of information which leave
     * the object untouched.
     */
    
    //@{
    
    /*! \brief Returns the X value of the left-most pixels.
     *
     * This function returns the X position of the window in the form of its
     * left-hand edge X-value. The first pixel of the CCD counts as 1. The value
     * returned is in terms of unbinned pixels, so if binning is being used, it refers
     * to the left-most unbinned pixel which makes up the left-most binned pixel
     * of a window.
     */
    int  llx()   const {return ll_x;}
    
    /*! \brief Returns the Y value of the bottom row of pixels.
     *
     * This function returns the Y position of the window in the form of its
     * bottom edge Y-value. The first pixel of the CCD counts as 1. The value
     * returned is in terms of unbinned pixels, so if binning is being used, it refers
     * to the bottom unbinned pixel which makes up the bottom binned pixels
     * of a window.
     */
    int  lly()   const {return ll_y;}
    
    /*! Returns the X dimension of the window.
     *
     * The dimension returned is measured in \b binned pixels.
     */
    int  nx()    const {return n_x;}
    
    /*! \brief Returns the Y dimension of the window.
     *
     * The dimension returned is measured in \b unbinned pixels.
     */
    int  ny()    const {return n_y;}

    /*! \brief Returns the total number of pixels.
     *
     * The total number of \b binned pixels is returned as can be useful
     * in computing array sizes.
     */
    int  ntot()  const {return n_x*n_y;} // total number of pixels

    //! Returns the binning factor in X.
    int xbin()  const {return x_bin;}

    //! Returns the binning factor in Y.
    int ybin()  const {return y_bin;}

    //! Returns the unbinned total CCD X dimension.
    int  nxtot() const {return nx_tot;}

    //! Returns the unbinned total CCD Y dimension.
    int  nytot() const {return ny_tot;}

    // Boundaries of window.

    /*! \brief Returns the left-hand edge of the window.
     *
     * The pixels are regarded as being centred upon integer values, so that
     * the pixel at (9,5) extends from 8.5 to 9.5 in X and 4,5 to 5.5 in Y.
     * This function returns the left edge of the window under this convention.
     */
    float left()  const {return float(ll_x)-0.5;}

    /*! \brief Returns the right-hand edge of the window.
     *
     * The pixels are regarded as being centred upon integer values, so that
     * the pixel at (9,5) extends from 8.5 to 9.5 in X and 4,5 to 5.5 in Y.
     * This function returns the right edge of the window under this convention.
     */
    float right() const {return float(ll_x+x_bin*n_x)-0.5;}

    /*! \brief Returns the top edge of the window.
     *
     * The pixels are regarded as being centred upon integer values, so that
     * the pixel at (9,5) extends from 8.5 to 9.5 in X and 4,5 to 5.5 in Y.
     * This function returns the top edge of the window under this convention.
     */
    float top()  const {return float(ll_y+y_bin*n_y)-0.5;}

    //! Returns the bottom edge of the window.

    /** The pixels are regarded as being centred upon integer values, so that
     * the pixel at (9,5) extends from 8.5 to 9.5 in X and 4,5 to 5.5 in Y.
     * This function returns the bottom edge of the window under this convention.
     */
    float bottom() const {return float(ll_y)-0.5;}

    // Centres of pixels in x and y

    //! Returns the CCD X coordinate equivalent to a computer X coordinate

    /** The pixels are regarded as being centred upon integer values, so that
     * pixel (9,5) is centred on coordinate (9,5). We need to distinguish 
     * 'CCD' coordinates which are always measured from the lower-left pixel
     * of the CCD (1,1) in terms of unbinned pixels from 'computer' coordinates
     * which are the direct position in terms of array position following the usual
     * C convention of starting from 0.
     *
     * This routine translates from computer --> CCD for X values.
     * \param x the computer X pixel value, starting from 0.
     * \return The CCD X coordinate corresponding to the supplied computer value.
     */
    double xccd(double x) const { return ll_x+x_bin*(x+0.5)-0.5;}

    //! Returns the CCD Y coordinate equivalent to a binned pixel value

    /** The pixels are regarded as being centred upon integer values, so that
     * pixel (9,5) is centred on coordinate (9,5). We need to distinguish 
     * 'CCD' coordinates which are always measured from the lower-left pixel
     * of the CCD (1,1) in terms of unbinned pixels from 'computer' coordinates
     * which are the direct position in terms of array position following the usual
     * C convention of starting from 0.
     *
     * This routine translates from computer --> CCD for Y values.
     * \param y the computer Y pixel value, starting from 0.
     * \return The CCD Y coordinate corresponding to the supplied computer value.
     */
    double yccd(double y) const { return ll_y+y_bin*(y+0.5)-0.5;}

    //! Returns the binned X value equivalent to a CCD X coordinate

    /** The pixels are regarded as being centred upon integer values, so that
     * pixel (9,5) is centred on coordinate (9,5). We need to distinguish 
     * 'CCD' coordinates which are always measured from the lower-left pixel
     * of the CCD (1,1) in terms of unbinned pixels from binned pixel coordinates
     * within a given window, the lower-left pixel of which is (0,0) following 
     * the C convention. The latter coordinates are 'computer' coordinates.
     *
     * This routine translates from CCD --> computer for X values.
     * \param x the CCD X value.
     * \return The computer X coordinate corresponding to the supplied CCD X value.
     */
    float xcomp(float x) const { return (x-ll_x+0.5f)/x_bin-0.5f;}

    //! Returns the binned X value equivalent to a CCD X coordinate

    /** The pixels are regarded as being centred upon integer values, so that
     * pixel (9,5) is centred on coordinate (9,5). We need to distinguish 
     * 'CCD' coordinates which are always measured from the lower-left pixel
     * of the CCD (1,1) in terms of unbinned pixels from binned pixel coordinates
     * within a given window, the lower-left pixel of which is (0,0) following 
     * the C convention. The latter coordinates are 'computer' coordinates.
     *
     * This routine translates from CCD --> computer for X values.
     * \param x the CCD X value.
     * \return The computer X coordinate corresponding to the supplied CCD X value.
     */
    double xcomp(double x) const { return (x-ll_x+0.5)/x_bin-0.5;}

    //! Returns the binned Y value equivalent to a CCD Y coordinate


    /** The pixels are regarded as being centred upon integer values, so that
     * pixel (9,5) is centred on coordinate (9,5). We need to distinguish 
     * 'CCD' coordinates which are always measured from the lower-left pixel
     * of the CCD (1,1) in terms of unbinned pixels from binned pixel coordinates
     * within a given window, the lower-left pixel of which is (0,0) following 
     * the C convention. The latter coordinates are 'computer' coordinates.
     *
     * This routine translates from CCD --> computer for X values.
     * \param y the CCD Y value.
     * \return The computer Y coordinate corresponding to the supplied CCD Y value.
     */
    float ycomp(float y) const { return (y-ll_y+0.5f)/y_bin-0.5f;}

    //! Returns the binned Y value equivalent to a CCD Y coordinate


    /** The pixels are regarded as being centred upon integer values, so that
     * pixel (9,5) is centred on coordinate (9,5). We need to distinguish 
     * 'CCD' coordinates which are always measured from the lower-left pixel
     * of the CCD (1,1) in terms of unbinned pixels from binned pixel coordinates
     * within a given window, the lower-left pixel of which is (0,0) following 
     * the C convention. The latter coordinates are 'computer' coordinates.
     *
     * This routine translates from CCD --> computer for X values.
     * \param y the CCD Y value.
     * \return The computer Y coordinate corresponding to the supplied CCD Y value.
     */
    double ycomp(double y) const { return (y-ll_y+0.5)/y_bin-0.5;}

    //@}

    // Set data 

    /** @name Set methods
     * These are methods for modification of the object's internal data.
     */

    //@{

    /*! \brief Sets the X value of the left-most pixels.
     *
     * This function sets the X position of the window in the form of its
     * left-hand edge pixel X-value. The first pixel of the CCD counts as 1. The value
     * set must be in terms of unbinned pixels, so if binning is being used, it should
     * refer to the left-most unbinned pixel which makes up the left-most binned pixel
     * of a window.
     * \param llx the left-most pixel of the window
     */
    void set_llx(int llx);

    /*! \brief Sets the Y value of the left-most pixels.
     *
     * This function sets the Y position of the window in the form of its
     * bottom edge pixel Y-value. The first pixel of the CCD counts as 1. The value
     * set must be in terms of unbinned pixels, so if binning is being used, it should
     * refer to the bottom unbinned pixel which makes up the bottom binned pixel
     * of a window.
     * \param lly the bottom pixel of the window
     */
    void set_lly(int lly);

    /*! \brief Sets the X dimension of a window.
     *
     * The dimension must be specified in terms of binned pixels.
     * \param nx the binned X dimension of the window
     */
    void set_nx(int nx);

    /*! \brief Sets the Y dimension of a window.
     *
     * The dimension must be specified in terms of binned pixels.
     * \param ny the binned Y dimension of the window
     */
    void set_ny(int ny);

    /*! \brief Sets the X binning factor of a window.
     *
     * \param xbin the binning factor in X
     */
    void set_xbin(int xbin);

    /*! \brief Sets the Y binning factor of a window.
     *
     * \param ybin the biining factor in Y
     */
    void set_ybin(int ybin);

    /*! \brief Sets the unbinned total X dimension of the CCD containing the window
     *
     * \param nxtot the unbinned X dimension of the CCD
     */
    void set_nxtot(int nxtot);

    /*! \brief Sets the unbinned total Y dimension of the CCD containing the window
     *
     * \param nytot the unbinned Y dimension of the CCD
     */
    void set_nytot(int nytot);

    /*! Sets all data at once.
     *
     * This function is provided for convenience when setting many parameters at once.
     * See individual functions for more details of the meaning of the arguments.
     */
    void set(int llx, int lly, int nx, int ny, int xbin, int ybin, int nxtot, int nytot);

    //@}

    // In order for object deletion in CCD to work.

    /*! \brief Returns indication of distance of an Window from a position
     *
     * This function returns a number that increases with the distance from
     * the coordinates entered as its arguments. This can be used to work out
     * which of several Windows is closest to a particular position, and is
     * need by the CCD class.
     * \param x X position of point of interest
     * \param y Y position of point of interest
     */
    float how_far(float x, float y) const;

    //! Is a Window "near enough" to a point?
    /**
     * When picking a Window by position, it is useful to know when a position is near enough
     * to consider that the Window has indeed been selected. This function accomplishes this,
     * returning "true" if the condition is met. It is needed by the CCD class.
     * \param x X coordinate of point of interest
     * \param y Y coordinate of point of interest
     */
    bool near_enough(float x, float y) const;

    /*! \brief Does a Window enclose a position?
     *
     * This function tells you whether position (x,y) is enclosed by a window or not.
     * \param x X coordinate of point of interest
     * \param y Y coordinate of point of interest
     */
    bool enclose(float x, float y) const;

    /*! \brief Binary output of a Window
     *
     * This function is used by the Frame class when writing out window formats.
     * \param fout the output stream
     */
    void write(std::ofstream& fout) const;

    /*! \brief Binary input of a Window
     *
     * This function is used by the Frame class when reading window formats.
     * \param fin the input stream
     */
    void read(std::ifstream& fin, bool swap_bytes);

    /*! \brief Binary input of a Window, old format
     *
     * This function is used by the Frame class when reading window formats.
     * \param fin the input stream
     */
    void read_old(std::ifstream& fin, bool swap_bytes);

    //! Checks whether a window is not null (both dimensions > 0)
    bool is_not_null() const;

    //! Checks whether a window is 1D (has at least one dimension = 1 and the other > 0)
    bool is_oned() const;
	
  private:

    // (ll_x,ll_y)      = lower-left corner pixel (unbinned)
    // (n_x,n_y)        = binned dimensions
    // (x_bin,y_bin)    = binning factors
    // (nx_tot, ny_tot) = total CCD dimensions (unbinned)

    Subs::INT4 ll_x, ll_y, n_x, n_y;
    Subs::INT4 x_bin, y_bin;
    Subs::INT4 nx_tot, ny_tot;

  };

  /*! \brief Draws a Window
   *
   * This function draws a Window as a line running along its outer edge.
   * \param window the Window to be drawn.
   */

  //! \relates Window

  void pgline(const Window& window);

  /*! \brief Label a Window
   *
   * This function labels a Window near its lower-left corner.
   * \param window the Window to be labelled.
   * \param label the label to attach.
   */

  //! \relates Window
  void pgptxt(const Window& window, const std::string& label);

  /*! \brief Checks a potential window
   *
   * This function checks a potential Window against various constraints
   * and returns true if the Window is not valid (e.g. it extends off CCD).
   * \param llx X value of lower-left pixel
   * \param lly Y value of lower-left pixel
   * \param nx binned X dimension of Window
   * \param ny binned Y dimension of Window
   * \param xbin binning factor in X
   * \param ybin binning factor in Y
   * \param nxtot unbinned X dimension of CCD
   * \param nytot unbinned Y dimension of CCD
   * \param Max_nxtot maximum unbinned X dimension of CCD
   * \param Max_nytot maximum unbinned Y dimension of CCD
   * \param Max_xbin maximum binning factor in X
   * \param Max_ybin maximum binning factor in Y
   */

  //! \relates Window
  bool bad_window(int llx, int lly, 
		  int nx, int ny,
		  int xbin, int ybin, 
		  int nxtot, int nytot, 
		  int Max_nxtot, int  Max_nytot,
		  int Max_xbin, int Max_ybin);

  /*! \brief Outputs a Window
   *
   * This extractor outputs Window information in a human-readable form.
   * \param s  the output stream
   * \param obj the Window to list.
   */

  //! \relates Window
  std::ostream& operator<<(std::ostream& s, const Window& obj);

  /*! \brief Inputs a Window
   *
   * This inserter inputs Window information as written by the equivalent
   * extractor.
   * \param s  the input stream
   * \param obj the Window to read.
   */

  //! \relates Window
  std::istream& operator>>(std::istream& s, Window& obj);  

  //! \brief Do two Windows clash?
  /**
   * This function checks that two Windows do not clash in any way, as
   * one must know when adding Windows to a group. 'Clashing' in this case
   * means that the windows do not overlap at all.
   * \param obj1 first window
   * \param obj2 second window
   * \relates Window
   */
  bool clash(const Window& obj1, const Window& obj2);

  //! \brief Are two Windows identical?
  /**
   * This function compares two windows to see if they are the same
   * (same position, dimensions, binning factors etc). This is an
   * elementary test needed if two windows are to be combined for instance.
   * \param obj1 first window
   * \param obj2 second window
   * \relates Window
   */
  bool operator==(const Window& obj1, const Window& obj2);

  //! \brief Are two Windows different?
  /**
   * This function checks whether two windows differ in any way
   * (position, dimensions, binning factors etc). This is an
   * elementary test needed if two windows are to be combined for instance.
   * \param obj1 first window
   * \param obj2 second window
   * \relates Window
   */
  bool operator!=(const Window& obj1, const Window& obj2);

  //! Do two windows overlap at all?
  bool overlap(const Window& obj1, const Window& obj2);

};

#endif

















