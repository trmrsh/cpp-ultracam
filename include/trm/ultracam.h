#ifndef TRM_ULTRACAM_H
#define TRM_ULTRACAM_H

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "trm/subs.h"
#include "trm/plot.h"
#include "trm/header.h"
#include "trm/array2d.h"

// enums and class forward declarations

#include "trm/ultracam_enums.h"

//! Structure to contain data passed back by servers
/** This is used by WriteMemoryCallback which has to be in C
 * and hence cannot be part of the Ultracam namespace.
 */

struct MemoryStruct {
  
  //! Pointer to the memory buffer
  char *memory;
  
  //! Number of chars in the buffer
  size_t size;
  
  //! Current position for any writing of data
  size_t posn;
};


// Ultracam namespace

//! Namespace of extra stuff for the ULTRACAM pipeline software

/** This namespace bundles up several helper functions, classes, typedefs
 * and other odds and ends that are used in several places throughout the 
 * ULTRACAM pipeline software.
 */

namespace Ultracam {

  //! 4-byte magic number for start of ucm files
  const Subs::INT4 MAGIC = 47561009;

  using Subs::operator+;

  // Forward declarations

  class Aperture;
  class Specap;
  class Defect;
  class Frame;
  class Skyline;
  class Spectrum;
  class Target;
  class Windata;
  class Window;
  template <class Obj> class CCD;
  template <class Obj> class MCCD;
  class Image;

  //! Multiple apertures
  typedef MCCD<Ultracam::Aperture> Maperture;

  //! Multiple spectroscopic regions
  typedef MCCD<Ultracam::Specap>   Mspecap;

  //! Multiple defects
  typedef MCCD<Ultracam::Defect>   Mdefect;

  //! Multiple sky lines
  typedef MCCD<Ultracam::Skyline>  Mskyline;

  //! Multiple spectra
  typedef MCCD<Ultracam::Spectrum> Mspectrum;

  //! Multiple targets
  typedef MCCD<Ultracam::Target>   Mtarget;

  //! Multiple windows
  typedef MCCD<Ultracam::Window>   Mwindow;

  //! Type for storage of CCD data within programs
  typedef Subs::REAL4 internal_data;

  //! Type for storage of CCD data within programs
  typedef Subs::Array2D<internal_data> Array;

  //! Data type of raw data from ULTRACAM
  typedef Subs::UINT2 raw_data;

  //! Data type for storage of two floats
  typedef Subs::xy<float,float> Fxy;

  //! Typedef for offsets to mask stars in sky aperture
  /** This is the type used to mask out stars from the sky aperture. Although
   * the clipped mean approach should mean that stars in the sky annulus are not
   * too much of a problem, it is better if possible to kick them out from the start.
   */
  typedef Subs::xyz<float,float,float> sky_mask;

  //! Typedef for extra stars to add in to main target
  /** This is the type used to add additional stars flux if they would be diufficult to ignore.
   * The same radius as for the main target will be used.
   */
  typedef Subs::xy<float,float> extra_star;

  // Error classes

  //! Ultracam_Error is the base class for exceptions.

  /** The pipeline software handles errors by throwing exceptions.
   * Ultracam_Error is the base class for all of these, which means
   * that it can be the only one trapped if one is not interested in 
   * the type of exception being thrown.
   *
   * The Ultracam_Error class is inherited from the standard C++ string class.
   * You can pass it a message for later printing if you want. Thus usually when you
   * catch an exception say called 'err', a line of the form \code cerr << err << endl;\endcode
   * will tell you something useful.
   */
  class Ultracam_Error : public std::string {
  public:

    //! Default constructor
    Ultracam_Error() : std::string() {}

    //! Constructor from a string (e.g. an error message).
    Ultracam_Error(const std::string& err) : std::string(err) {} 
  };

  //! File_Open_Error is an exception class for file I/O

  /** File_Open_Error exceptions are thrown when an unsuccessful 
   * attempt has been made to open a file.
   */
  class File_Open_Error : public Ultracam_Error {
  public:
    //! Default constructor
    File_Open_Error() : Ultracam_Error() {}
    //! Constructor from a string (e.g. an error message).
    File_Open_Error(const std::string& err) : Ultracam_Error(err) {}
  };

  //! Modify_Error is an exception class for failures to change objects.

  /** Modify_Error exceptions are supposed to be associated with failed attempts
   * to change objects.
   */
  class Modify_Error : public Ultracam_Error {
  public:
    //! Default constructor
    Modify_Error() : Ultracam_Error() {}
    //! Constructor from a string (e.g. an error message).
    Modify_Error(const std::string& err) : Ultracam_Error(err) {}
  };

  //! Input_Error is an exception class for command input failures.

  /** Input_Error is a fairly heavily used class for when a command input is
   * invalid.
   */
  class Input_Error : public Ultracam_Error {
  public:
    //! Default constructor
    Input_Error() : Ultracam_Error() {}
    //! Constructor from a string (e.g. an error message).
    Input_Error(const std::string& err) : Ultracam_Error(err) {}

  };

  //! Read_Error is an exception class for errors during file input
  class Read_Error : public Ultracam_Error {
  public:
    //! Default constructor
    Read_Error() : Ultracam_Error() {}
    //! Constructor from a string (e.g. an error message).
    Read_Error(const std::string& err) :  Ultracam_Error(err) {}
  };

  //! Write_Error is an exception class for errors during file output
  class Write_Error : public Ultracam_Error {
  public:
    //! Default constructor
    Write_Error() : Ultracam_Error() {}
    //! Constructor from a string (e.g. an error message).
    Write_Error(const std::string& err) : Ultracam_Error(err) {}
  };

#ifdef DEBUG

  //! CheckX is fior checked index access for debugging purposes.

  /**
   * If DEBUG is set as a compiler flag, then the X index of Windata
   * objects is accessed via a CheckX object checked which allows
   * the index to be range-checked. This is slow of course compared to 
   * unchecked access and should only be used as a last resort.
   */
  class CheckX{

  public:
    
    //! Default constructor
    CheckX() : n_x(0), dat(NULL) {}

    //! Constructor of nx elements
    CheckX(int nx_) : n_x(nx_) {dat = new internal_data [n_x];}

    //! Copy constructor 
    CheckX(const CheckX& obj) : n_x(obj.n_x) {
      dat = new internal_data [n_x];
      if(dat){
	for(int i=0; i<n_x; i++) dat[i] = obj.dat[i];
      }else{
	std::cerr << "Disaster inside CheckX copy constructor" << std::endl;
      }
    }

    //! Destructor
    ~CheckX(){if(dat) delete [] dat;}
    
    //! Returns number of elements
    int nx() const {return n_x;}
    
    //! Sets number of elements
    void set(int nx_){
      if(dat) delete [] dat;
      n_x  = nx_;
      dat = new internal_data [n_x];
    }
    
    //! Get ix-th element, checking range
    internal_data& operator[](int ix){
      if(ix < 0 || ix >= nx())
	throw Ultracam_Error(
			     std::string("CheckX::operator[](int) error: ix = ") + ix +
			     std::string(" but should lie from 0 to ") + (nx()-1) );
      
      return dat[ix];
    }
    
    //! Get ix-th element, checking range
    const internal_data& operator[](int ix) const {
      if(ix < 0 || ix >= nx())
	throw  Ultracam_Error(
			      std::string("CheckX::operator[](int) error: ix = ") + ix +
			      std::string(" but should lie from 0 to ") + (nx()-1));
      return dat[ix];
    }
    
    //! Conversion to normal C-type array
    operator internal_data*() const { return dat;}
    
  private:
    int n_x;
    internal_data *dat;
  };
  
#endif

  
  // Utility functions
  
#ifndef DEBUG

  //! findpos determines the x,y position of a source
  void findpos(internal_data **dat, internal_data **var, int nx, int ny, float fwhm_x, float fwhm_y, 
	       int hwidth_x, int hwidth_y, float xstart, float ystart, bool bias,
	       double& xpos, double &ypos, float& ex, float& ey);

  //! function to remove cosmic rays (no DEBUG version)
  void zapcosmic(internal_data **dat, int nx, int ny, 
		 int hwidth_x, int hwidth_y, 
		 float xcen, float ycen,
		 float thresh_height, float thresh_ratio,
		 std::vector<std::pair<int, int> >& zapped);
#else

  //! function to remove cosmic rays (DEBUG version)
  void zapcosmic(CheckX* dat, int nx, int ny, 
		 int hwidth_x, int hwidth_y, 
		 float xcen, float ycen,
		 float thresh_height, float thresh_ratio,
		 std::vector<std::pair<int, int> >& zapped);
#endif

  //! Parameters defining stellar profiles and which are variable
  
  /**
   * Ppars contains the values of all the parameters which define 2D gaussian or Moffat profiles,
   * including the shape, height and background and also whether the fit is to be symmetric or elliptical. 
   * In addition it stores variables which show whether a given parameter is to be kept fixed or not. 
   * Finally it provides functions which interface to routines which expect vectors of variables.
   * 
   * The Gaussian profiles are defined by \f[ y = s + h \exp - \left(a x^2 + 2 b x y + c y^2\right) , \f]
   * where \c s is the sky background, taken to be constant in the region of the fit, \c h is the
   * height of the centre of the gaussian, and \c a, \c b and \c c define the elliptical gaussian.
   *
   * Moffat profiles are defined by \f[ sky + h/\left(1 + a x^2 + 2 b x y + c y^2\right)^\beta \f].
   * Moffat profiles are a generalisation of Gaussian profiles with the essential difference being that
   * the wings can be more extended. Moffat profiles become Gaussian in the limit of large \f[ \beta \f] 
   *
   */

  struct Ppars{

    enum PROFILE_TYPE {GAUSSIAN, MOFFAT};

    //! Default constructor (moffat by default)
    Ppars() : ptype(MOFFAT), sky(0.), x(0.), y(0.), height(0.), a(0.01), b(0.), c(0.01), beta(4.), symm(true), var_sky(true),
	 var_height(true), var_x(true), var_y(true), var_a(true), var_b(true), var_c(true), var_beta(true) {}

    //! Constructor for gaussian profiles
    Ppars(double sky_, double x_, double y_, double height_, double a_, double b_, double c_, bool symm_) :
      ptype(GAUSSIAN), sky(sky_), x(x_), y(y_), height(height_), a(a_), b(b_), c(c_), beta(0.), symm(symm_), var_sky(true),
	 var_height(true), var_x(true), var_y(true), var_a(true), var_b(true), var_c(true), var_beta(false) {}

    //! Constructor for moffat profiles
    Ppars(double sky_, double x_, double y_, double height_, double a_, double b_, double c_, double beta_, bool symm_) :
      ptype(MOFFAT), sky(sky_), x(x_), y(y_), height(height_), a(a_), b(b_), c(c_), beta(beta_), symm(symm_), var_sky(true),
	 var_height(true), var_x(true), var_y(true), var_a(true), var_b(true), var_c(true), var_beta(true) {}

    //! set gaussian profile parameters
    void set(double sky_, double x_, double y_, double height_, double a_, double b_, double c_, bool symm_) {
      ptype  = GAUSSIAN;
      sky    = sky_;
      x      = x_;
      y      = y_;
      height = height_;
      a      = a_;
      b      = b_;
      c      = c_;
      symm   = symm_;
    }

    //! set moffat profile parameters
    void set(double sky_, double x_, double y_, double height_, double a_, double b_, double c_, double beta_, bool symm_) {
      ptype  = MOFFAT;
      sky    = sky_;
      x      = x_;
      y      = y_;
      height = height_;
      a      = a_;
      b      = b_;
      c      = c_;
      beta   = beta_;
      symm   = symm_;
    }


    //! Returns the number of parameters to be fitted
    int npar() const;

    //! Returns the maximum number of parameters to be fitted
    int nmax() const;

    //! Returns the value of parameter i
    double get_param(int i) const;

    //! Sets the value of parameter i
    void set_param(int i, double val);

    //! Return the index of the sky parameter
    int sky_index() const {return 0;}

    //! Return the index of the x parameter
    int x_index() const {return 1;}

    //! Return the index of the y parameter
    int y_index() const {return 2;}

    //! Return the index of the height parameter
    int height_index() const {return 3;}

    //! Return the index of the a parameter
    int a_index() const {return 4;}

    //! Return the index of the b parameter
    int b_index() const {return 5;}

    //! Return the index of the c parameter
    int c_index() const {return 6;}

    //! Return the index of the beta parameter
    int beta_index() const {
      if(symm){
	return 5;
      }else{
	return 7;
      }
    }

    //! Return threshold of exponential beyond which no computation is made in the case of Gaussian fits
    static double thresh() {return 20.;}

    //! Returns fitting value of parameter i
    bool get_param_state(int i) const;

    // Now come the data

    //! Defines the fitting method, gaussian or moffat.
    PROFILE_TYPE ptype;
 
    //! Sky background value
    double sky;

    //! X ordinate of centre of gaussian
    double x;

    //! Y ordinate of centre of gaussian
    double y;

    //! Height of gaussian
    double height;

    //! XX coefficient of shape
    double a;

    //! XY coefficient of shape
    double b;

    //! YY coefficient of shape
    double c;

    //! beta exponent for Moffat fits
    double beta;

    //! Are profiles circularly symmetric or not?
    bool symm;

    //! Is \c sky variable or fixed?
    bool var_sky;

    //! Is \c height variable or fixed?
    bool var_height;

    //! Is \c x variable or fixed?
    bool var_x;

    //! Is \c y variable or fixed?
    bool var_y;

    //! Is \c a variable or fixed?
    bool var_a;
    
    //! Is \c b variable or fixed?
    bool var_b;

    //! Is \c c variable or fixed?
    bool var_c;

    //! Is \c beta variable or fixed?
    bool var_beta;

  };

  inline bool Ppars::get_param_state(int i) const {

    if(symm){
 
      switch(i){
      case 0:
	return var_sky;
      case 1:
	return var_x;
      case 2:
	return var_y;
      case 3:
	return var_height;
      case 4:
	return var_a;
      case 5:
	return var_beta;
      default:
	throw Ultracam::Ultracam_Error("double Ultracam::Ppars::get_param_state(int) const: index out of range (1)");
      }

    }else{

      switch(i){
      case 0:
	return var_sky;
      case 1:
	return var_x;
      case 2:
	return var_y;
      case 3:
	return var_height;
      case 4:
	return var_a;
      case 5:
	return var_b;
      case 6:
	return var_c;
      case 7:
	return var_beta;
      default:
	throw Ultracam::Ultracam_Error("double Ultracam::Ppars::get_param_state(int) const: index out of range (2)");
      }
    }
  }

  //! Sorts profile fit covariances into correct order (accounting for fixed & variable parameters)
  void covsrt(Subs::Buffer2D<double>& covar, const Ultracam::Ppars& params, int nvar);  

  //! Performs a 2D gaussian fit to a Windata
  void fitgaussian(const Windata& data, Windata& sigma, int xlo, int xhi, 
		   int ylo, int yhi, Ultracam::Ppars& params, double& chisq, 
		   double& alambda, Subs::Buffer2D<double>& covar);

  //! Performs a Moffat fit to a Windata
  void fitmoffat(const Windata& data, Windata& sigma, int xlo, int xhi, 
		 int ylo, int yhi, Ultracam::Ppars& params, double& chisq, 
		 double& alambda, Subs::Buffer2D<double>& covar);

  //! Tweaks stellar position using gaussian cross-correlation of 1D profiles
  void pos_tweak(const Windata& win, const Windata& var, float fwhm, int hwidth, float xinit, float yinit, double& xnew, double& ynew);

  //! Information structure returned by fit_plot_profile
  struct Iprofile{
    double  chisq;    /**< final chi-squared of fit */ 
    int     ndof;     /**< final number of degrees of freedom of fit */ 
    int     nrej;     /**< total number of points rejected during fit */ 
    int     nits;     /**< total number of iterations */ 
    int     ncycle;   /**< number of reject cycles */ 
    float   rmax;     /**< maximum value in fit region */ 
    float   fwhm;     /**< FWHM of final profile */ 
    float   efwhm;    /**< uncertainty on FWHM of final profile */ 
    float   fwhm_min; /**< minimum FWHM of final profile in elliptical case */ 
    float   fwhm_max; /**< maximum FWHM of final profile in elliptical case */ 
    float   angle;    /**< angle of ellipse */ 
    float   esky;     /**< uncertainty on sky value */ 
    float   epeak;    /**< uncertainty on peak value */ 
    float   ex;       /**< uncertainty on X position */ 
    float   ey;       /**< uncertainty on Y position */ 
    float   ebeta;    /**< uncertainty on value of beta */ 
    int xlo;      /**< lower X limit of fit region */ 
    int xhi;      /**< upper X limit of fit region */ 
    int ylo;      /**< lower Y limit of fit region */ 
    int yhi;      /**< upper Y limit of fit region */
    Subs::Buffer2D<double> covar; /**< 2D array of covariances */
  };

  //! High-level routine for profile fits
  void fit_plot_profile(const Image& data, const Image& dvar, Ppars& profile, bool initial_search, bool initialise,
			float xinit, float yinit, const std::vector<sky_mask>& skymask, float fwhm1d, int hwidth1d, 
			int hwidth, const Subs::Plot& fplot, float sigrej, Iprofile& iprofile, bool print);

  //! Plot images
  void plot_images(const Frame& data, float x1, float x2, float y1, float y2, 
		   bool all, char stackdirn, char iset, float& ilow, float& ihigh, float plow, 
		   float phigh, bool first, const std::string& fname, int nccd, bool termio);

  //! Plot apertures
  void plot_apers(const Maperture& apers, float x1, float x2, float y1, float y2, 
			    bool all, char stackdirn, int nccd);

  //! Plot apertures
  void plot_defects(const Mdefect& defect, float x1, float x2, float y1, float y2, 
		    bool all, char stackdirn, int nccd);

  //! Plots setup windows
  void plot_setupwins(const std::string& setwin, int numccd, float x1, float x2, float y1, float y2, 
		      bool all, char stackdirn, int nccd, bool ultraspec);

  // Logging class prints to a log file and standard out, allowing you
  // to pad messages so that the '=' part comes at the same place each time which
  // looks better and is easier to follow.

  //! Data logger class

  /** Logger defines a class useful for printing out information to standard out
   * terminal and also a file. Its main feature is that it can align output messages
   * on an = sign which makes for rather easier scanning of the results. A Logger
   * object is attached to a disk file where messages are sent.
   */
  class Logger {
  public:

    //! Default constructor
    Logger() : log_file(), npad_(50) {}

    //! General Constructor

    /** This constructs a Logger with an optional length to make the string before
     * the equals sign. This is a minimum. If the length of the first string actually needs
     * to be larger, it will be.
     * \param logfile name of disk file to send output to.
     * \param npad    minimum length of first string before = sign
     * \param clobber overwrite files with the same name as logfile or not
     */
    Logger(const std::string& logfile, int npad=50, bool clobber=true) : npad_(npad){
      if(!clobber){
	std::ifstream iftest(logfile.c_str());
	if(iftest){
	  iftest.close();
	  throw Input_Error(std::string("Logger::Logger(const std::string&, int, bool): log file = ") + logfile +
			    std::string(" already exists!"));
	}
      }
      log_file.open(logfile.c_str());
      if(log_file.fail()) 
	throw Input_Error("Logger::Logger(const std::string&, int, bool): failed to open log file = " + logfile);
    }

    //! Destructor

    /**
     * The destructor closes down the disk file 
     */
    ~Logger(){log_file.close();}

    //! Open a new disk file

    /** This function closes any existing log files and opens a new one, allowing
     * one to alter the minimum string lengths at the same time.
     * \param logfile name of disk file to open
     * \param npad minimum length of string before the = sign
     * \param clobber overwrite files with the same name as logfile or not
     */
    void open(const std::string& logfile, int npad=50, bool clobber=true){
      if(log_file) log_file.close();
      log_file.clear();

      if(clobber){
	log_file.open(logfile.c_str());
      }else{
	std::ifstream iftest(logfile.c_str());
	if(iftest){
	  iftest.close();
	  throw Input_Error(std::string("Logger::open(const std::string&, int, bool): log file = ") + logfile +
			    std::string(" already exists!"));
	}
	log_file.open(logfile.c_str(), std::ios::out);
      }
      if(!log_file) throw Input_Error("Logger::open(const std::string&, int, bool): failed to open log file = " + logfile);
    }

    //! Log a simple message

    /** This function simply logs a single string, which is sent to stdout if wanted,
     * but always to the disk file. A '#' is 'pre-pended' to act as a comment flag.
     * \param message the string to log
     * \param tostdout controls whether it should be logged to stdout as well as the disk file
     */
    void logit(const std::string& message, bool tostdout=true) const {
      if(tostdout) std::cout     << message << std::endl;
      log_file << "# " << message << std::endl;
    }
    
    // Message + value

    //! Log a message plus a value

    /** This function logs a single string followed by = value, which is sent to stdout if wanted,
     * but always to the disk file. A '#' is 'pre-pended' to act as a comment flag.
     * \param start_string string to start the message with
     * \param val the value to output
     * \param tostdout controls whether it should be logged to stdout as well as the disk file
     */
    template <class T>
    void logit(const std::string& start_string, const T& val, bool tostdout=true) const{
      int l = start_string.size();

      if(tostdout){
	std::cout << start_string;
	if(l < npad_)
	  for(int i=0; i<npad_-l; i++) std::cout << ' ';
	std::cout << " = " << val << " " << std::endl;
      }

      log_file << "# " << start_string;
      if(l < npad_)
	for(int i=0; i<npad_-l; i++) log_file << ' ';
      log_file << " = " << val << " " << std::endl;
    }

    // Message + value + end

    // Message + value

    //! Log a message plus a value plus a trailer

    /** This function logs a single string followed by = value, and then a trailing message.
     * The result can be sent to stdout if wanted but always to the disk file. A '#' is 'pre-pended' to act as a comment flag.
     * \param start_string the start string to log
     * \param val the value to output
     * \param end_string the trailing string 
     * \param tostdout controls whether it should be logged to stdout as well as the disk file
     */
    template <class T>
    void logit(const std::string& start_string, const T& val, const std::string& end_string, bool tostdout=true) const {

      int l = start_string.size();

      if(tostdout){
	std::cout << start_string;
	if(l < npad_)
	  for(int i=0; i<npad_-l; i++) std::cout << ' ';
	std::cout << " = " << val << " " << end_string << std::endl;
      }
      
      log_file << "# " << start_string;
      if(l < npad_)
	for(int i=0; i<npad_-l; i++) log_file << ' ';
      log_file << " = " << val << " " << end_string << std::endl;
    }

    //! Returns file stream associated with the logger

    /** This allows one to send arbitrary messages to the disk file using
     * the usual << operator.
     */
    std::ofstream& ofstr(){return log_file;}

  private:
    Logger(const Logger& log){} // no copying
    mutable std::ofstream log_file;
    int npad_;
  };
 
  //! Log a message plus a value plus a trailer to stdout only
  
  /** This function logs a single string followed by = value, and optionally a trailing message.
   * The result can be sent to stdout if wanted but always to the disk file. A '#' is 'pre-pended' to act as a comment flag.
   * \param start_string the start string to log
   * \param val the value to output
   * \param end_string the trailing string 
   * \param npad amount of padding to get to the '=' sign
   */
  template <class T>
  void logit(const std::string& start_string, const T& val, const std::string& end_string="", int npad=50) {
    
    int l = start_string.size();
    
    std::cout << start_string;
    if(l < npad)
      for(int i=0; i<npad-l; i++) std::cout << ' ';
    std::cout << " = " << val;
    if(end_string != "")
      std::cout << " " << end_string << std::endl;
    else
      std::cout << std::endl;
  }


  //! Structure containg x,y of the lower left pixel, binned dimensions and binning factors of a window
  /** This is a simple structure for use 
   * in parseXML and the ServerData class
   */
  struct Wind{
    int llx, lly, nx, ny;
  };


  //! Class returning a few bits and pieces of info from the server.
  struct ServerData {
      
      //! Constructor to define the initial state of some variables
      ServerData() : timestamp_default(true) {}
      
      //! Which time stamp correction to apply
      /** In July 2003 and again in March 2010 upgrades caused a change in when timestamps 
       * were taken such that were taken immediately after rather before readouts. The
       * original problem was fixed in Dec 2004, but then re-appeared in March 2010
       * This parameter if True will mean that corrections are made according to these dates.
       * If False, the reverse corrections are applied.
       */
      bool timestamp_default;
      
      //! Number of bytes in a frame
      int  framesize;
      
      //! Number of bytes in a word
      int  wordsize;
      
      //! Number of words in the header
      int  headerwords;
      
      //! Exposure time, seconds (as written in XML file, even though not exactly this in reality)
      float expose_time;
      
      //! Readout mode
      enum READOUT_MODE {
	  FULLFRAME_CLEAR,    /**< Full frame mode with a clear at the start of each exposure */ 
	  FULLFRAME_NOCLEAR,  /**< Full frame mode with a clear only before the first one */
	  FULLFRAME_OVERSCAN, /**< Full frame mode with an overscan region */
	  WINDOWS,            /**< 2, 4 or 6 Windows mode */
	  DRIFT,              /**< Drift mode */
	  WINDOWS_CLEAR,      /**< 2 windows mode with a clear at the start of each exposure */
	  L3CCD_WINDOWS,      /**< Standard L3CCD application, 1/2 windows, full frame etc. */
	  L3CCD_DRIFT,        /**< L3CCD drift application, 2 windows */
      } readout_mode;
      
      //! Y binning factor, needed for computation of drift-mode times
      int ybin;
      
      //! X binning factor, needed to estimate readout times
      int xbin;
      
      //! The window dimensions and locations (left, right up the chip)
      std::vector<Wind> window;
      
      //! Row transfer time byte
      unsigned char v_ft_clk;
      
      //! Gain speed setting
      std::string gain_speed;
      
      //! Which observing run
      enum WHICH_RUN {
	  MAY_2002,    /**< May 2002 was first run with some special features */ 
	  OTHERS       /**< Other runs */
      } which_run;
      
      //! Instrument, ULTRACAM or ULTRASPEC
      std::string instrument;
      
      //! Version of XML
      int version;
      
      //! Period of good blue frames (1 ==> all ok, 2 ==> skip every other one)
      int nblue;

      //! Application name
      std::string application;

      //! Units of exposure time
      float time_units;

      //! Sub-class for L3CCD
      struct L3data {
	  int led_flsh; /**< LED setting */
	  int rd_time;  /**< unsure of meaning of this */
	  int rs_time;  /**< unsure of meaning of this */
	  int en_clr;   /**< Clear enabled or not */
	  int hv_gain;  /**< Avalanche gain parameter, 0 to 9 */
	  int gain;     /**< Normal gain parameter */
	  int output;   /**< Which output, normal or avalanche */
	  int speed;    /**< Which output, normal or avalanche */
	  std::vector<int> nchop; /**< Number of first-read pixels to trash on side of windows */
      } l3data;
      
      //! Indicates whether we are in a fullframe mode
      bool fullframe() const{
	  return (readout_mode == FULLFRAME_CLEAR || readout_mode == FULLFRAME_NOCLEAR || readout_mode == FULLFRAME_OVERSCAN);
      }
      
      //! Indicates whether a given frame is junk.
      bool is_junk(int frame) const {
	  return (
	      (readout_mode == DRIFT    && frame <= int((1033./window[0].ny/ybin+1.)/2.)) ||
	      (readout_mode == WINDOWS  && frame == 1));
      }
      
  };
  
  //! Timing information structure
  struct TimingInfo{

      //! UTC at mid-exposure
      Subs::Time ut_date;

      //! UTC at mid-exposure for the blue frames
      Subs::Time ut_date_blue;
      
      //! Exposure time in seconds
      float exposure_time;

      //! Blue exposure time in seconds
      float exposure_time_blue;
      
      //! Flag to indicate whether the timing info is reliable
      bool reliable;

      //! Reason why time is considered unreliable
      std::string reason;

      //! Flag to indicate whether the timing info for the blue frame is reliable
      bool reliable_blue;
      
      //! Frame number
      int frame_number;
      
      //! Raw GPS time stamp
      Subs::Time gps_time;
      
      //! Format of timing data 1 = < Mar 2010, 2 = Mar 2010 --
      int format;

      //! Number of satellites (only format = 1)
      int nsatellite;

      //! Time stamp status (since March 2010)
      unsigned short int tstamp_status;
      
      //! The vertical row transfer time used
      float vclock_frame;
      
      //! This shows how we are treating the times
      /** If true, this mean that we are assuming the timestamping works as
       * it originally did, else as it did July 2003 to Dec 2004, and from
       * March 2010
       */
      bool default_tstamp;

      //! Is the blue frame junk? (owing to accumulation option of spring 2007)
      bool blue_is_bad;
  };

  //! De-multiplexes raw ULTRACAM data
  void de_multiplex_ultracam(char *buffer, Frame& data);

  //! De-multiplexes raw ULTRASPEC data
  void de_multiplex_ultraspec(char *buffer, Frame& data, const std::vector<int>& nchop);

  //! De-multiplexes raw ULTRASPEC drift-mode data
  void de_multiplex_ultraspec_drift(char *buffer, Frame& data, const std::vector<int>& nchop);

  //! Interprets time from raw header
  void read_header(char* buffer, const Ultracam::ServerData& serverdata, Ultracam::TimingInfo& timing);

  //! Reads photometric reduction option file
  void read_reduce_file(const std::string& file, const std::string& logfile);

  //! Reads spectroscopic reduction option file
  void read_sreduce_file(const std::string& file, const std::string& logfile);

  //! Reads and parses XML file
  void parseXML(char source, const std::string& XML_URL, Mwindow& mwindow, Subs::Header& header, 
		Ultracam::ServerData& serverdata, bool trim, int ncol, int nrow, double twait, double tmax);

  //! Name of environment variable which can be set to specify location of default files
  const char ULTRACAM_ENV[]         = "ULTRACAM_ENV";

  //! Standard name of directory for default files if environment variable not set.
  const char ULTRACAM_DIR[]         = ".ultracam";

  //! Name of environment variable specifying default URL
  const char ULTRACAM_DEFAULT_URL[] = "ULTRACAM_DEFAULT_URL";

  //! Default string to add for server running on local host if environment variable not set
  const char ULTRACAM_LOCAL_URL[]   = "http://127.0.0.1:8007/";

  //! Possible methods of shift and adding
  /**
   * \enum NEAREST_PIXEL        the shift will be carried out to the nearest pixel. Simple and fast but crude
   * \enum LINEAR_INTERPOLATION the shift will be carried out by linearly interpolating between 4 surrounding pixels.
   */
  enum SHIFT_METHOD {NEAREST_PIXEL, LINEAR_INTERPOLATION};

  //! Structure for shift_and_add
  struct Shift_info {

    //! Shift in X
    float dx;

    //! Shift in Y
    float dy;

    //! OK to add?
    bool ok;
  };

  //! Shift and add function
  void shift_and_add(Frame& sum, const Frame& extra, const std::vector<Shift_info>& shift, 
		     internal_data multiplier, SHIFT_METHOD shift_method);

  //! Rejection function for 2D gaussian fits
  void gauss_reject(const Windata& data, Windata& sigwin, int xlo, int xhi, 
		    int ylo, int yhi, const Ultracam::Ppars& params, float thresh, int& nrej);
  
  //! Rejection function for Moffat fits
  void moffat_reject(const Windata& data, Windata& sigwin, int xlo, int xhi, 
		     int ylo, int yhi, const Ultracam::Ppars& params, float thresh, int& nrej);

  //! Performs initial preparation for profile fits
  void profit_init(const Image& data, const Image& dvar, double& x, double& y, bool initial_search, 
		   float fwhm1d, int hwidth1d, int hwidth, float& sky, float& peak, bool skip_sky);

  //! Gets a frame from a server file
  bool get_server_frame(char source, const std::string& url, Frame& data, const Ultracam::ServerData& serverdata, 
			size_t& nfile, double twait, double tmax, bool reset=false, bool demultiplex=true);

  //! Loads local XML file into a buffer for use by XML parser routines
  void loadXML(const std::string& name, MemoryStruct& buff);


  //! Redefines PGPLOT colours
  void def_col(bool reverse=true);

//  class Reduce::Meanshape;

  //! Updates the aperture file, also returns shape and uncertainty structures
  void rejig_apertures(const Frame& data, const Frame& dvar, const Subs::Plot& profile_fit_plot, bool blue_is_bad,
		       Maperture& aperture, std::vector<Reduce::Meanshape>& shape, std::vector<std::vector<Fxy> >& errors);

  //! Estimates the sky in an aperture annulus
  void sky_estimate(const Aperture& aperture, const Windata& dwin, const Windata& vwin, const Windata& bwin,
		    Reduce::SKY_METHOD sky_method, float sky_clip, Reduce::SKY_ERROR sky_error,
		    float& sky, float& sky_sigma, double& rms, int& nsky, int& nrej, bool& overlap);

  //! Extracts flux in an aperture
  void extract_flux(const Image& data, const Image& dvar, const Image& bad,
		    const Image& gain, const Image& bias, const Aperture& aperture, Reduce::SKY_METHOD sky_method, 
		    float sky_clip, Reduce::SKY_ERROR sky_error, Reduce::EXTRACTION_METHOD extraction_method,
		    const std::vector<std::pair<int,int> >& zapped, const Reduce::Meanshape& shape, float pepper, float saturate,
		    float& counts, float& sigma, float& sky, int& nsky, int& nrej,
		    Reduce::ERROR_CODES& ecode, int& worst);

  //! Light curve plotter for reduce
  void light_plot(const Subs::Plot& lcurve_plot, const std::vector<std::vector<Reduce::Point> >& all_ccds, 
		  const Subs::Time& ut_date, bool makehcopy, const std::string& hcopy, const std::string& title);

  //! Structure for relative CCD orientation
  struct Transform {

    //! General constructor
    Transform(double scale, double angle, double xshift, double yshift) :
      scale(scale), angle(angle), xshift(xshift), yshift(yshift) {}

    //! Scale factor
    double scale;
    
    //! Anti-clockwise rotation in degrees
    double angle;

    //! X translation
    double xshift;

    //! Y translation
    double yshift;
  };

  //! Computes CDFs for L3 CCD simulation
  void lllccd(int nstage, double p, double pcic, Subs::Buffer1D<Subs::Array1D<double> >& cdf);    

  //! Check that variables from file input for 'reduce' and 'sreduce' have names which are not blank
  bool badInput(const std::map<std::string,std::string>& reduce, const std::string& name, std::map<std::string,std::string>::const_iterator& p);

  //! Make a profile from a Windata
  bool make_profile(const Windata& data, const Windata& dvar, float x1, float x2, float y1, float y2, int hwidth, 
		    Subs::Array1D<float>& prof, Subs::Array1D<float>& pvar, Subs::Array1D<int>& npix);

  //! Moves sky regions for spectrum extraction
  void sky_move(const Frame& data, const Frame& dvar, const Mspecap& master, Sreduce::REGION_REPOSITION_MODE reposition_mode,
		float fwhm, float max_shift, int hwdith, Sreduce::ERROR_CODES& error_code, Mspecap& region);

  //! Carries out poly fits with rejections
  void sky_fit(const Frame& data, const Frame& dvar, const Mspecap& region, int npoly, float reject, Frame& sky);

  //! Normal spectrum extraction
  void ext_nor(const Frame& data, const Frame& dvar, const Mspecap& region, int npoly, const Frame& sky,
	       std::vector<std::vector<Subs::Array1D<float> > >& sdata, std::vector<std::vector<Subs::Array1D<float> > >& serror);

  //! Plots extracted spectra
  void plot_spectrum(const std::vector<std::vector<Subs::Array1D<float> > >& sdata, const std::vector<std::vector<Subs::Array1D<float> > >& serror,
		     bool individual_scale, Sreduce::PLOT_SCALING_METHOD scale_method, float ylow, float yhigh, float plow, float phigh);

  //! Plot trailed spectra
  void plot_trail(const std::vector<std::vector<std::vector<Subs::Array1D<float> > > >& sdata, int step, bool reset,
		  Sreduce::PLOT_SCALING_METHOD scale_method, float ilow, float ihigh, float plow, float phigh);

 };

//! Function for getting data from server
extern "C" {
  size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data);
}

#endif
