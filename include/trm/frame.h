#ifndef TRM_FRAME_H
#define TRM_FRAME_H

#include <string>
#include "trm/header.h"
#include "trm/mccd.h"
#include "trm/windata.h"
#include "trm/window.h"
#include "trm/ultracam.h"

namespace Ultracam {

    // Forward declaration
    struct FCmul;
  
    //! The Frame class defines object which represent ULTRACAM frames.
    /**
     * Frame objects contain everything one needs to represent
     * ULTRACAM data frames. Thus they include headers, multiple CCDs, and multiple
     * windows for each CCD. This is the class to use for reading and writing single
     * ULTRACAM exposures, adding a constant to them, etc, and is one of the more
     * commonly encountered classes.
     *
     * Various obvious operators are supported, for example "-=", "+=" etc, but others
     * such as "*" and "+" are not. This is because in the simplest implementation of an operation 
     * like "new = old1 + old2", a temporary object is created and then copied. For large objects
     * this is inefficient. Instead the user is encouraged to use "in place" operations like "+=".
     *
     * One exception is the operation "frame -= constant*dark", useful for scaled dark subtraction.
     * For this a special optimisation has been written. This is an exception however. More often,
     * the user may want to code up loops over CCDs, windows and pixels. This is possible, because
     * an expression like frame[0] refers to the first CCD of a frame, while frame[2][1] refers to
     * the second window of the third CCD, and finally frame[nccd][nwin][iy][ix] refers to the
     * (ix,iy) pixel of window nwin, CCD nccd (all indices starting from 0).
     */
  
    class Frame : public Mimage, public Subs::Header {
    
    public:
    
	//! Default constructor. 
	/**
	 * Constructs a null frame with no header.
	 */
	Frame() : Mimage(), Subs::Header() {};
    
	//! Constructor for defined number of CCDs. 
	/**
	 * Constructs a frame of nccd null CCDs with no header.
	 * \param nccd the number of CCDs
	 */
	Frame(int nccd) : Mimage(nccd), Subs::Header() {};
    
	//! Copy constructor. 
	/**
	 * Constructs a frame with same CCDs, windows and header.
	 * obj the ULTRACAM frame to copy.
	 */
	Frame(const Frame& obj) : Mimage(obj), Subs::Header(obj) {};
    
	//! Constructor from reading a file. 
	/**
	 * Constructs a frame by reading it from a disk file.
	 * \param file the file name
	 * \param nc which CCD to read, 0 by default implying all of them.
	 */
	Frame(const std::string& file, int nc=0);
    
	//! Constructor from a multiple-windows object. 
	/**
	 * A Frame is constructed to have windows matching
	 * those supplied. The Frame is then ready to be added etc to other similarly
	 * dimensioned frames.
	 * \param mwin the multiple windows defining the format.
	 */
	Frame(const Mwindow& mwin);
    
	//! Constructor from windows and a header. 
	/**
	 * A Frame is constructed to have windows and header matching
	 * those supplied. The Frame is then ready to be added etc to other similarly
	 * dimensioned frames.
	 * \param mwin the multiple windows defining the format.
	 * \param head the header to set the Frame to have.
	 */
	Frame(const Mwindow& mwin, const Subs::Header& head);
    
	//! Assignment to a constant.
	/**
	 * This sets the values of all the pixels of all windows of all CCDs to a constant.
	 * \param con the value to set all pixels to.
	 */
	Frame& operator=(const Ultracam::internal_data& con);
    
	//! Returns number of CCDs
	size_t size() const {return Mimage::size();}
    
	//! Reset the format and headers of a Frame
	void format(const Mwindow& mwin, const Subs::Header& head);
    
	//! Reset the format and headers of a Frame (no data copied)
	void format(const Frame& frame);
    
	//! Adds a constant to a Frame.
	void operator+=(const Ultracam::internal_data& con);
    
	//! Subtracts a constant from a Frame.
	void operator-=(const Ultracam::internal_data& con);
    
	//! Multiplies a Frame by a constant
	void operator*=(const Ultracam::internal_data& con);
    
	//! Divides a Frame by a constant
	void operator/=(const Ultracam::internal_data& con);
    
	//! Adds one frame to another.
	void operator+=(const Frame& obj);
    
	//! Subtracts one frame from another.
	void operator-=(const Frame& obj);
    
	//! Multiples a frame by another.
	void operator*=(const Frame& obj);
    
	//! Divides a frame by another.
	void operator/=(const Frame& obj);
    
	//! Applies a lower limit to a Frame
	void max(const Ultracam::internal_data& low);
    
	//! Crops a frame.
	void crop(const Frame& obj);
    
	//! Crops a frame.
	void crop(const Mwindow& obj);
    
	//! Windows a frame
	void window(const Mwindow& obj);
    
	//! Returns the maximum X dimension.
	int nxtot() const;
    
	//! Returns the maximum Y dimension.
	int nytot() const;
    
	//! Returns standard extension of ULTRACAM files.
	/**
	 * ULTRACAM file names have a standard extension added on. This allows the user to use the same
	 * name for a file, associated windows and aperture files, while these are in fact kept on disk
	 * with different, easily recognisable extensions. The standard one for ULTRACAM files is ".ucm".
	 */
	static std::string extnam() {return ".ucm";}
    
	//! Determines whether a file is an ULTRACAM file.
	static bool is_ultracam(const std::string& name);
    
	//! Read an ULTRACAM file.
	void read(const std::string& file, int nc=0);
    
	//! Write an ULTRACAM file.
	void write(const std::string& file, Windata::Out_type otype=Windata::NORMAL) const;
    
	/*! \brief Access a CCD
	 *
	 * This function returns a reference to one CCD of a Frame.
	 * \param nccd the CCD number to access, starting from 0. 
	 * Further indices can be used to  access a particular window, row and pixel.
	 */
	Image& operator[](size_t nccd){return Mimage::operator[](nccd);}
    
	/*! \brief Access a CCD
	 *
	 * This function returns a reference to one CCD of a Frame.
	 * \param nccd the CCD number to access, starting from 0.
	 * Further indices can be used to  access a particular window, row and pixel.
	 */
	const Image& operator[](size_t nccd) const {return Mimage::operator[](nccd);}
    
	/*! \brief Header access.
	 *
	 * This function returns a pointer to an Hitem corresponding to the name of a header item.
	 */
	Subs::Hitem* operator[](const std::string& name) { return Subs::Header::operator[](name);}
    
	/*! \brief Special subtraction operator
	 *
	 * This function in combination with the FCmul class allows optimised
	 * evaluation of expressions of the form \code frame1 -= constant*frame2 \endcode 
	 */
	void operator-=(const FCmul& obj);

	// Resolve some amibguities
	Subs::Header::const_iterator begin() const {return this->Subs::Header::begin();}
	Subs::Header::const_iterator end() const {return this->Subs::Header::end();}
    };
  
    /*! \brief ASCII output operator
     *
     * This function carries out ASCII output of a Frame. It is provided as a last
     * resort, as usually only the binary I/O should be used \sa Frame::read and
     * Frame::write
     */
    std::ostream& operator<<(std::ostream& ostr, const Frame& frame);
  
    /*! \brief Optimisation helper class.
     *
     * This class just stores the value and reference of a float and
     * a Frame for optimising the operation data \code -= constant*bias \endcode
     * It is unlikely that you will ever need to use it explicitly, but it is
     * documented here in case you want to do something similar.
     *
     * The idea is that when faced with an expression like "frame -= c*bias", had you
     * defined an operator for "c*bias", this would have been carried out first, producing
     * a temporary that is passed to the -= operator. This is inefficient. A different scheme
     * is implemented here in which the "c*bias" operation is turned into a simple construction
     * operation in which the value of \b c is stored along with a reference to \b bias.
     *
     * We then face a "-=" operation with an FCmul on the right-hand side. Thus the special
     * operator Frame::operator-=(const FCmul&) is defined which actually carries out the full
     * operation of multiplication and subtraction pixel by pixel, avoiding the need for a temporary.
     *
     * This is called a 'closure' by Strousup.
     */
    struct FCmul{
	//! stored value of "constant" in "-= constant*frame" type operations
	float con;
	//! stored reference to "frame" in "-= constant*frame" type operations
	const Frame& frame;
	//! constructor to store "constant" and "frame" in "-= constant*frame" type operations
	FCmul(float con_, const Frame& frame_) : con(con_), frame(frame_) {}
    };
  
  
    /*! \brief Optimisation function
     *
     * This function uses the FCmul class to convert an expression of
     * the form float * Frame into an object with minimal copying.= and
     * overheads with the use of 'inline'.
     */
    inline FCmul operator*(float con, const Frame& frame) {
	return FCmul(con, frame);
    }

  
};

#endif








