#ifndef TRM_ULTRACAM_FDISK_H
#define TRM_ULTRACAM_FDISK_H

#include <string>
#include <iostream>
#include "trm_window.h"
#include "trm_ultracam.h"

namespace Ultracam {

  //! Class to load pixels from ULTRACAM files sequentially
  
  /** Fdisk is designed to allow very many frames to be accessed simultaneously.
   * This is to facilitate operations such as taking the median of a large number
   * of frames where it is not possible to store everything in core using Frame.
   * Fdisk is therefore very closely connected to Frame. The idea is to open
   * a file, read the header, the Window format etc and then read a small amount
   * of the data into a buffer which is updated as one marches through the data.
   * All of this violates "encapsulation" but at least the format dependent stuff
   * should be restricted to Fdisk. 
   *
   * It is possible to access just one of the CCDs as well so that each get_next operation
   * produces the next pixel of that CCD alone.
   */
  
  class Fdisk {
    
  public:
    
    //! Constructor from a file, with an optional integer to specify just one of the CCDs
    Fdisk(const std::string& file, int nbuff, int wccd=0);
    
    //! Destructor
    ~Fdisk();
    
    //! Get next pixel value
    Ultracam::internal_data get_next();
    
  private:
    
    // the size of the buffer
    int nbuff_;
    
    // the CCD to work on, 0 if all.
    int wccd_;
    
    // the input stream
    std::ifstream fin; 
    
    // the buffer
    Ultracam::internal_data *buff;
    
    // Pointer to current location in buffer
    int ptr;
    
    // Number of pixels of current window read so far
    int nread_;
    
    // the number of CCDs, the CCD equivalent to the last data access
    int num_ccd, nccd_;
    
    // the number of windows of the current CCD, the window number we have reached
    int num_win, nwin_;
    
    // the coordinates of the most recently read pixel
    int ny_, nx_;
    
    // the window format of the current window
    Window window;
    
    // data type of the current window
    Windata::Out_type out_type_;

    // whether to swap bytes or not
    bool swap_bytes;

  };
};


#endif








