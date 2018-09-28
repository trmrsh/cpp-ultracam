#ifndef TRM_ULTRACAM_CCD_H
#define TRM_ULTRACAM_CCD_H

#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include "trm/subs.h"
#include "trm/array1d.h"
#include "trm/windata.h"
#include "trm/ultracam.h"

namespace Ultracam {

    template <class T> class CCD;

    // These have to be declared before they are referred to in the class
    // which is why the forward reference to the class itself is needed. 
    template <class T>
    std::ostream& operator<<(std::ostream& s, const CCD<T>& ccd);
    
    template <class T>
    std::istream& operator>>(std::istream& s, CCD<T>& ccd);
    
    //! CCD<Obj> is a template class that represents a CCD's worth of Obj objects
    
    /**
     * CCD<Obj> represents a CCD's worth of objects such as apertures, targets or
     * windows. Inherited from vector, this allows all of vectors functions but beware
     * that it is possible to get an inconsistent object, e.g. with overlapping windows
     * if you are not careful. Try to use functions such as push_back when building a CCD<Obj>.
     *
     * The Obj object must supply the following functions: 'float how_far(float x, float y) const'
     * which returns the distance of an Obj from the point (x,y); 'bool near_enough(float x, float y) const'
     * which says whether an Obj is near enough to (x,y) to be selected (e.g. by cursor). There must also
     * be a friend 'bool clash(const Obj& obj1, const Obj& obj2)' which says whether two Objs clash or not.
     *
     * Note that for Obj = Windata one should normally use the Image class which is inherited from CCD<Windata>
     * rather than CCD<Windata> itself as the Image class is more fully-featured.
     */ 
    
    template <class Obj>
    class CCD : public std::vector<Obj> {
    public:
	
	//! \brief An iterator type for objects stored in a CCD
	/**
	 * CCD is a template class which bundles up objects of different
	 * types into a "CCDs-worth". Some functions return an iterator that
	 * depends upon the type of object bundled. optr provides a generic
	 * type for such iterators.
	 */
	typedef typename std::vector<Obj>::iterator optr;

	//! Default constructor
	CCD() {}
	
	//! Constructor of a CCD with nobj objects
	CCD(int nobj) : std::vector<Obj>(nobj) {}
	
	//! Return object closest to a given position, if one exists.
	bool closest(float x, float y, optr& itr);

	//! Return object closest to a given position if selected.
	bool selected(float x, float y, optr& itr);

	//! Add another object to a CCD
	void push_back(const Obj& obj);
	
	//! Delete an object from a CCD
	bool del_obj(float x, float y, Obj& deleted);

	//! Change an object
	void modify(int no, const Obj& obj);
	
	//! Checks that a CCD is valid.
	bool valid() const;
	
	//! ASCII output of a CCD
	friend std::ostream& operator<< <>(std::ostream& s, const CCD& ccd);
	
	//! ASCII input of a CCD
	friend std::istream& operator>> <>(std::istream& s, CCD& ccd);
	
    };

    //! Function object defines "less than" for two objects.
    
    /** Defines what it means for one object to be "less" than
     * another in terms of how far each is from a given point. This is 
     * used by "selected". It requires each object to have a function
     * called 'how_far'
     */
    template <class Obj>
    class Obj_less : public std::binary_function<Obj,Obj,bool> {
	float xc, yc;
    public:
	//! Constructor storing the fixed position
	Obj_less(float x, float y) : xc(x), yc(y) {}
	//! () operator to decide whether obj1 is closer than obj2
	bool operator()(const Obj& obj1, const Obj& obj2) const {
	    return obj1.how_far(xc,yc) < obj2.how_far(xc,yc);
	}
    };
    
    // Now the member function definitions

    /**
     * This function returns an iterator of type optr which refers to 
     * the object closest to position (x,y). This requires each supported object
     * to have a function called 'how_far' to say how far they are from a given point. See the
     * \ref Aperture class for example.
     * \param x    X coordinate of position of interest.
     * \param y    Y coordinate of position of interest.
     * \param itr  iterator, if valid object found
     * \return true if an object found, else false
     */
    template <class Obj> 
    bool CCD<Obj>::closest(float x, float y, optr& itr) {
	if(this->size()){
	    itr = this->begin();
	    float d, dis = itr->how_far(x,y);
	    for(optr it=itr+1; it != this->end(); it++){
		if((d = it->how_far(x,y)) < dis){
		    itr = it;
		    dis = d;
		}
	    }
	    return true;
	}else{
	    return false;
	} 
    }

    /**
     * This function returns an iterator of type optr which refers to 
     * the object closest to position (x,y). It only does so however if the
     * object is "close enough" as defined by an equivalent function of the 
     * object. See the \ref Aperture class for example. Otherwise it returns
     * NULL.
     * \param x   X coordinate of position of interest.
     * \param y   Y coordinate of position of interest.
     * \param itr iterator pointing to the object closest to the input x,y position
     * \return An iterator is returned which is more-or-less a pointer to the object of interest. If
     * no object is found this will be NULL.
     */
    template <class Obj> 
    bool CCD<Obj>::selected(float x, float y, optr& itr) {
  
	if(closest(x,y,itr)) {

	    // See if it is near enough

	    if(itr->near_enough(x,y)){
		return true;
	    }else{
		return false;
	    }
	}else{
	    return false;
	} 
    }

    /**
     * This function adds in another object to a CCD, checking for consistency with
     * any objects already stored. If there is a clash, it throws an Ultracam_Error.
     * \param obj the object to add in.
     */
    template <class Obj>
    void CCD<Obj>::push_back(const Obj& obj){
	for(size_t io=0; io<this->size(); io++)
	    if(clash(obj,(*this)[io])) 
		throw Ultracam::Ultracam_Error("New object overlaps an old one in void CCD<Obj>::push_back(const Obj& obj)");
	this->std::vector<Obj>::push_back(obj);
    }
  
    // Delete the closest object to (x,y)
    /**
     * This function deletes the object closest to the position (x,y), if it
     * is close enough (see selected(float, float) for example)
     * If returns with true if selected and passes back a copy of the deleted object
     * which can be useful for plotting for example.
     * \param x X coordinate of point close to object to be deleted
     * \param y Y coordinate of point close to object to be deleted
     * \param deleted a copy of the the deleted object
     * \return true/false according to whether the object was successfully deleted.
     */
    template <class Obj>
    bool CCD<Obj>::del_obj(float x, float y, Obj& deleted){
	if(this->size()){
	    optr sel;
	    if(selected(x, y, sel)){
		deleted = *sel;
		this->erase(sel);
		return true;
	    }else{
		std::cerr << "Not near enough to any object to count" << std::endl;
		return false;
	    }
	}else{
	    std::cerr << "Nothing to delete" << std::endl;
	    return false;
	} 
    }

    /**
     * This function allows you to change an object while checking for consistency with 
     * those already stored.
     * \param no object number to change, starting at 0
     * \param obj the new object
     * \exception Ultracam::Modify_Error thrown if new object inconsistent with old.
     */
    template <class Obj>
    void CCD<Obj>::modify(int no, const Obj& obj){
	if(no > this->size()-1)
	    throw Ultracam::Modify_Error("no out of range in void CCD<>::modify(int no, const Obj& obj");
	for(int io=0; io<this->size(); io++){
	    if(io != no && clash((*this)[io],obj))
		throw Ultracam::Modify_Error(std::string("New ") + Obj::name() +
					     std::string(" clashes with others in "
							 "void CCD<>::modify(int no, const Obj& obj"));
    
	}
	(*this)[no] = obj;
    }

    /**
     * Use of the index operator[](int i) can leave a CCD is an invalid
     * state. This is provided to test whether this has occurred.
     * \return \c true if CCD is OK, otherwise \c false.
     */

    template <class Obj>
    bool CCD<Obj>::valid() const {
	for(int io=1; io<this->size(); io++)
	    for(int ib=0; ib<io; ib++)
		if(clash((*this)[io],(*this)[ib])) return false;
	return true;
    }

    // ASCII I/O

    /**
     * Outputs every object of a CCD using its own << operator
     * and appending a number to the object
     * \param s output stream
     * \param ccd CCD to be output 
     */
    template <class Obj>
    std::ostream& operator<<(std::ostream& s, const CCD<Obj>& ccd){
	s << "Number of " << Obj::plural_name() << " = " << ccd.size() << "\n" << std::endl;
	std::string name = Obj::name();
	name[0] = toupper(name[0]);
	for(size_t io=0; io<ccd.size(); io++){
	    s << name << " " << io+1 << ": " << std::endl;
	    s << ccd[io] << std::endl;
	}
	return s;
    }

    /**
     * This function reads a CCD stored in binary format. Checks
     * are made for the validity of the resulting CCD.
     * \param s input stream
     * \param ccd CCD to be loaded
     * \exception An Ultracam::Read_Error is thrown if the CCD is not valid.
     */
    template <class Obj>
    std::istream& operator>>(std::istream& s, CCD<Obj>& ccd){
	char ch;

	size_t nobj;
	while(s.get(ch) && ch != '=');
	if(!s || !(s >> nobj)) 
	    throw Ultracam::Read_Error("Invalid input into CCD::operator>> (1)");
    
	if(nobj != ccd.size()) ccd.resize(nobj);
  
	for(size_t io=0; io<nobj; io++){
	    while(s.get(ch) && ch != ':');
	    if(!s || !(s >> ccd[io]))
		throw Ultracam::Read_Error("Invalid input into CCD::operator>> (2)");
  
	    // Guard against inconsistent objects

	    for(size_t ib=0; ib<io; ib++)
		if(clash(ccd[ib],ccd[io]))
		    throw Ultracam::Read_Error(std::string("One ") + Obj::name() + std::string(" clashed with one read earlier"));
	}
	return s;
    }

    //! Plot a CCD in line form

    /**
     * This function plots all the objects in a CCD in line form and labels
     * each with a number corresponding to their storage position, starting from 1.
     * \param ccd the CCD to plot.
     * \relates CCD
     */
    template <class Obj>
    void pgline(const CCD<Obj>& ccd){
	for(size_t io=0; io<ccd.size(); io++){
	    pgline(ccd[io]);
	    pgptxt(ccd[io],Subs::str(io+1));
	}
    }

    //! Image is a class for representing CCD images (1 CCD per Image)

    /** Image is inherited from the CCD<Obj> base template with Obj = Windata,
     * and adds extra functions that only mean something for Windata objects. This class 
     * is closest to representing what one thinks of as a CCD image, and hence its name.
     */ 

    class Image : public CCD<Windata> {
    public:
	
	//! Default constructor
	Image() : CCD<Windata>() {}
 
	//! Constructor of nobj Windata objects
	Image(int nobj) : CCD<Windata>(nobj) {}

	//! Constructor from a set of windows
	Image(const CCD<Window>& win);

	//! Set all pixels to a constant
	Image& operator=(const Ultracam::internal_data& con);

	//! Set format of an Image to match a set of Windows
	Image& operator=(const CCD<Window>& win);

	//! Add another Image in place
	void operator+=(const Image& obj);

	//! Subtract another Image in place
	void operator-=(const Image& obj);

	//! Mulitply by another Image in place
	void operator*=(const Image& obj);

	//! Divide by another Image in place
	void operator/=(const Image& obj);

	//! Add a constant to every pixel in place
	void operator+=(const Ultracam::internal_data& con);

	//! Subtract a constant from every pixel in place
	void operator-=(const Ultracam::internal_data& con);

	//! Multiply every pixel by a constant in place
	void operator*=(const Ultracam::internal_data& con);

	//! Divide every pixel by a constant in place
	void operator/=(const Ultracam::internal_data& con);

	//! Return unbinned X dimension of an Image
	int nxtot() const;

	//! Return unbinned Y dimension of an Image
	int nytot() const;

	//! Return minimum pixel value of an Image
	Ultracam::internal_data min() const;

	//! Return maximum pixel value of a Image
	Ultracam::internal_data max() const;

	//! Calculate the mean of a Image
	Ultracam::internal_data mean() const;

	//! Calculate a percentile over the whole frame
	void centile(float l, Ultracam::internal_data& c) const;

	//! Calculate two percentiles over the whole frame
	void centile(float l1, float l2, Ultracam::internal_data& c1, Ultracam::internal_data& c2) const;

	//! Return maximum pixel value of an Image over a region
	Ultracam::internal_data max(const CCD<Window>& window) const;

	//! Return minimum pixel value of a Image over a region
	Ultracam::internal_data min(const CCD<Window>& window) const;

	//! Calculate a percentile over a region of the frame
	void centile(float l, Ultracam::internal_data& c, const CCD<Window>& window) const;

	//! Calculate two percentiles over a region of the frame
	void centile(float l1, float l2, Ultracam::internal_data& c1, Ultracam::internal_data& c2, const CCD<Window>& window) const;

	//! Crop an Image to match the format of another
	void crop(const Image& ccd);

	//! Crop an Image to match the format of a set of Windows
	void crop(const CCD<Window>& win);
 
	//! Window an Image by a CCD<Window>
	void window(const CCD<Window>& win);

	//! Apply a step function transform to an Image
	void step(Ultracam::internal_data thresh);

	//! Structure containing some basic statistics
	/**
	 * This structure is used by statistics(const CCD<Window>&, float, bool) as a return
	 * value when computing statistics over an arbitrary region of a CCD.
	 */
	struct Stats {

	    //! The number of valid points in the region defined.
	    size_t npoints;

	    //! The maximum value
	    float  max;

	    //! The minimum value
	    float  min;

	    //! The mean of all the points
	    float  raw_mean;

	    //! The RMS of all the points
	    float  raw_rms;

	    //! The mean after outlier rejection
	    float  clipped_mean;

	    //! The RMS after outlier rejection
	    float  clipped_rms;

	    //! The number of outliers rejected
	    size_t nrejected;
	
	    //! The median value
	    float  median;
	
	};
	
	//! Statistics function
	Stats statistics(const CCD<Window>& statwin, float sigma, bool compute_median, bool careful) const;
	
	//! Function to return raw data over a special region.
	void buffer(const CCD<Window>& ccdwin, Subs::Array1D<internal_data>& buff) const;
	
	//! Returns reference to the Windata eclosing a given point
	Windata& enclose(float x, float y);
	
	//! Returns reference and number of the Windata eclosing a given point
	Windata& enclose(float x, float y, int& which);
	
	//! Returns reference to the Windata eclosing a given point
	const Windata& enclose(float x, float y) const;
	
	//! Returns reference and number of the Windata eclosing a given point
	const Windata& enclose(float x, float y, int& which) const;
	
	//! Reads an Image
	void read(std::ifstream& fin, bool swap_bytes);
	
	//! Skips an Image
	void skip(std::ifstream& fin, bool swap_bytes);
	
	//! Reads an Image, old format
	void read_old(std::ifstream& fin, bool swap_bytes);
	
	//! Skips an Image, old format
	void skip_old(std::ifstream& fin, bool swap_bytes);
	
	//! Writes an Image
	void write(std::ofstream& fout, Windata::Out_type otype=Windata::NORMAL) const;
	
    };
    
    //! Compute the minimum value
    Ultracam::internal_data min(const Image& obj);
    
    //! Compute the maximum value
    Ultracam::internal_data max(const Image& obj);
    
    //! Compute the minimum value over a region
    Ultracam::internal_data min(const Image& obj, const CCD<Window>& window);
    
    //! Compute the maximum value over a region
    Ultracam::internal_data max(const Image& obj, const CCD<Window>& window);
    
    //! Test two Image objects for equality
    bool operator==(const Image& ccd1, const Image& ccd2);
    
    //! Test two Image objects for inequality
    bool operator!=(const Image& ccd1, const Image& ccd2);
    
    //! Draw an Image as a greyscale image
    void pggray(const Ultracam::Image& ccd, float lo, float hi);
    
    //! Label an Image
    void pgptxt(const Ultracam::Image& ccd);
    
};

#endif









