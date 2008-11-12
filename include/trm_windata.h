#ifndef TRM_ULTRACAM_WINDATA_H
#define TRM_ULTRACAM_WINDATA_H

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include "trm_subs.h"
#include "trm_window.h"
#include "trm_array2d.h"
#include "trm_ultracam.h"

namespace Ultracam {

  //! Windata objects represent the data and format of individual readout windows

  /** Windata objects are inherited from Window objects which specify the window format and Array2D 
   * objects to add data. Bundled up into CCDs and ultimately, multi-CCDs they form the basis of the storage 
   * of Frame objects.
   */ 

  class Windata : public Window, public Ultracam::Array {

  public:

    //! Enumeration defining the possible disk output data types
    /** \param NORMAL data type corresponding to the internally stored type, Ultracam::internal_data
     * \param RAW data type corresponding to the raw data from ULTRACAM
     */
    enum Out_type {NORMAL = 0, RAW = 1};

    //! Name to use when referring to Windata objects
    static std::string name(){return "windata";}

    //! Default constructor
    Windata() : Window(), Ultracam::Array() {}

    //! General constructor
    Windata(int llx, int lly, int nx, int ny, int xbin, int ybin, int nxtot, int nytot);

    //! Copy constructor from a Window
    Windata(const Window& window); 

    //! Assignment to a Window
    Windata& operator=(const Window& window); 

    //! Assignment to a constant
    Windata& operator=(const Ultracam::internal_data& con); 

    //! Resize a Windata
    void resize(int nyd, int nxd);

    //! Binary output
    void write(std::ofstream& fout, Out_type otype=NORMAL) const;

    //! Binary input
    void read(std::ifstream& fin, bool swap_bytes);

    //! Binary input, old format
    void read_old(std::ifstream& fin, bool swap_bytes);

    //! Skip binary data
    void skip(std::ifstream& fin, bool swap_bytes);

    //! Skip binary data, old format
    void skip_old(std::ifstream& fin, bool swap_bytes);

    //! ASCII output
    friend std::ostream& operator<<(std::ostream& s, const Windata& obj);

    //! Calculates a percentile
    void centile(float l, Ultracam::internal_data& c) const;

    //! Window a Windata to joint overlap with a Window
    Windata window(const Window& win) const;

    //! Defines the level of ASCII output
    static void set_print_level(int level);

    //! Returns the level of ASCII output
    static int print_level(){return plevel;}

    //! Copies data to a 1D array which is created on the fly
    Ultracam::internal_data* buffer() const;

    //! Copy data into a 1D array provided by the user
    void copy(Ultracam::internal_data *ptr) const {
      if(ptr) get(ptr);
    }

    //! Return C-style pointer to a row
    const Ultracam::internal_data* row(int iy) const {return this->ptr()[iy];}

    //! Return C-style pointer to a row
    Ultracam::internal_data* row(int iy) {return this->ptr()[iy];}
    
  private:
  
    static int plevel;       // defines nature of ASCII output
  
    // Prevent access to Window::set_nx and Window::set_ny these cannot
    // be changed without modifying the data array
    void set_nx(int);
    void set_ny(int);

  };

  // Non-member functions
  
  //! Plot as a greyscale image
  void pggray(const Ultracam::Windata& obj, float lo, float hi);

};

#endif

















