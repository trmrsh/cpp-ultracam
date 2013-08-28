#include "cpgplot.h"
#include "trm/subs.h"
#include "trm/windata.h"
#include "trm/ultracam.h"

int Ultracam::Windata::plevel = 1;

/** There are three levels of ASCII output -- see ostream& operator<<(ostream&, const Ultracam::Windata&)
 * These are specified by an integer starting at 1. This function sets in, checking that the value
 * is OK.
 * \param level the output level to set.
 */
void Ultracam::Windata::set_print_level(int level){
  if(level < 1 || level > 3) throw
			       Ultracam::Ultracam_Error("Attempt to set invalid"
					      " print level for Ultracam::Windata");
  plevel = level;
}

/**
 * Constructs Winadata of full generailty, but does not set the data.
 * \param llx X ordinate of lower-left corner pixel
 * \param lly Y ordinate of lower-left corner pixel
 * \param nx binned X dimension of window
 * \param ny binned Y dimension of window
 * \param xbin X binning factor
 * \param ybin Y binning factor
 * \param nxtot Total unbinned X dimension of CCD
 * \param nytot Total unbinned X dimension of CCD
 */
Ultracam::Windata::Windata(int llx, int lly, int nx, int ny, int xbin, int ybin, int nxtot, int nytot) :
  Window(llx, lly, nx, ny, xbin, ybin, nxtot, nytot), Ultracam::Array(ny,nx) {}

//! Constructors from windows
Ultracam::Windata::Windata(const Window& window) : Window(window), Ultracam::Array(window.ny(),window.nx()) {}

//! Assignment to a Window
Ultracam::Windata& Ultracam::Windata::operator=(const Window& window){
  if(nx() != window.nx() || ny() != window.ny())
    this->Ultracam::Array::resize(window.ny(), window.nx());
  this->Window::operator=(window);
  return *this;
}

//! Assignment to a constant
Ultracam::Windata& Ultracam::Windata::operator=(const Ultracam::internal_data& con){
  this->Ultracam::Array::operator=(con);
  return *this;
}

/** This function changes the dimensions of the window, ensuring
 * that the new one is valid. Whatever data is present will be lost
 * in the process.
 * \param nyd new binned Y dimension
 * \param nxd new binned X dimension
 * \exception An Ultracam::Ultracam_Error will be thrown if the new
 * window is not valid.
 */
void Ultracam::Windata::resize(int nyd, int nxd){
  if(nx() != nxd || ny() != nyd){
    if(bad_window(llx(), lly(), nxd, nyd, xbin(), ybin(), nxtot(), nytot(), MAX_NXTOT, MAX_NYTOT, MAX_XBIN, MAX_YBIN))
      throw 
	Ultracam::Ultracam_Error("Invalid new window dimensions in void Ultracam::Windata<>::resize(int ny, int nx): " +
				 Subs::str(llx()) + ", " + Subs::str(lly()) + ", " + Subs::str(nxd) + ", " + Subs::str(nyd) + 
				 ", " + Subs::str(xbin()) + ", " + Subs::str(ybin()) + ", " + Subs::str(nxtot()) + ", " + 
				 Subs::str(nytot()) );
    
    this->Window::set_nx(nxd);
    this->Window::set_ny(nyd);
    this->Ultracam::Array::resize(nyd, nxd);
  }
}

/**
 * This function outputs a Ultracam::Windata in binary form, the usual choice
 * since Ultracam::Windata objects can be large. It can do so in different types.
 * See Ultracam::Windata::Out_type for a description of the options.
 * \param fout  output stream, opened for binary I/O
 * \param otype the data type to be stored.
 * \exception Ultracam::Write_Error exceptions may be thrown for a number
 * of reasons.
 */
void Ultracam::Windata::write(std::ofstream& fout, Out_type otype) const {

  Window::write(fout);
  Subs::INT4 iout = Subs::INT4(otype);
  fout.write((char*)&iout,sizeof(Subs::INT4));
  if(!fout) 
    throw Ultracam::Write_Error("Error writing data type in void Ultracam::Windata<T>::write(std::ofstream&, Out_type) const");

  if(otype == NORMAL){

    // usual case: write out as native type
    for(int iy=0; iy<ny(); iy++){
      fout.write((char*)ptr()[iy],sizeof(Ultracam::internal_data[nx()]));
      if(!fout) 
	throw Ultracam::Write_Error("Error writing data in void Ultracam::Windata<T>::write(std::ofstream&, Out_type) const");
    }

  }else if(otype == RAW){

    // for storing raw data
    Ultracam::raw_data *buff = new Ultracam::raw_data[ntot()];
    int n=0;
    for(int iy=0; iy<ny(); iy++)
      for(int ix=0; ix<nx(); ix++)
	buff[n++] = Ultracam::raw_data(std::max(Ultracam::internal_data(0),ptr()[iy][ix]+Ultracam::internal_data(0.5)));
    fout.write((char*)buff,sizeof(Ultracam::raw_data[ntot()]));
    delete[] buff;
    if(!fout) 
      throw Ultracam::Write_Error("Error writing data in void Ultracam::Windata::write(std::ofstream&, Out_type) const");
  }   
}

/**
 * This function inputs a Ultracam::Windata in binary form, the usual choice
 * since Ultracam::Windata objects can be large. It copes automatically with 
 * the different types that can be output by void Ultracam::Windata::write(ofstream&, Out_type),
 * carrying out a conversion to the Ultracam::internal_data type used in the
 * software.
 * \param fin  input stream, opened for binary I/O
 * \exception Ultracam::Read_Error exceptions may be thrown for a number of reasons
 */
void Ultracam::Windata::read(std::ifstream& fin, bool swap_bytes) {

  Window t;
  t.read(fin, swap_bytes);
  if(!fin) throw Ultracam::Read_Error("Error reading window in void Ultracam::Windata::read(std::ifstream&, bool)");

  if(nx() != t.nx() || ny() != t.ny())
    this->Ultracam::Array::resize(t.ny(), t.nx());
  this->Window::operator=(t);

  Subs::INT4 iout;
  fin.read((char*)&iout,sizeof(Subs::INT4));
  if(!fin) throw Ultracam::Read_Error("Error reading data type in void Ultracam::Windata::read(std::ifstream&, bool)");
  if(swap_bytes) iout = Subs::byte_swap(iout);

  Out_type otype = Out_type(iout);
  if(otype == NORMAL){

    // Read directly
    for(int iy=0; iy<ny(); iy++){
      fin.read((char*)ptr()[iy],sizeof(Ultracam::internal_data[nx()]));
      if(!fin) throw Ultracam::Read_Error("Error reading data in void Ultracam::Windata::read(std::ifstream&, bool)");
      if(swap_bytes) Subs::byte_swap(ptr()[iy],nx());
    }

  }else if(otype == RAW){

    // Read into a buffer
    Ultracam::raw_data *buff = new Ultracam::raw_data[ntot()];
    fin.read((char*)buff,sizeof(Ultracam::raw_data[ntot()]));
    if(!fin){
      delete[] buff;
      throw Ultracam::Read_Error("Error reading data in void Ultracam::Windata::read(std::ifstream& fin)");
    }

    if(swap_bytes) Subs::byte_swap(buff,ntot());

    // Convert type
    int n=0;
    for(int iy=0; iy<ny(); iy++)
      for(int ix=0; ix<nx(); ix++)
	ptr()[iy][ix] = Ultracam::internal_data(buff[n++]);

    delete[] buff;
  }else{
    throw Ultracam_Error("Ultracam::Windata::read(std::ifstream&, bool): unrecognised value of data type");
  } 
}

/**
 * This function skips a Ultracam::Windata in binary form while reading a file.
 * It copes automatically with 
 * the different types that can be output by void Ultracam::Windata::write(ofstream&, Out_type).
 * \param fin  input stream, opened for binary I/O
 * \exception Ultracam::Read_Error exceptions may be thrown for a number of reasons
 */
void Ultracam::Windata::skip(std::ifstream& fin, bool swap_bytes) {

  Window t;
  t.read(fin, swap_bytes);
  if(!fin) 
    throw Ultracam::Read_Error("Error reading window in void Ultracam::Windata::skip(std::ifstream&, bool)");

  Subs::INT4 iout;
  fin.read((char*)&iout,sizeof(Subs::INT4));
  if(!fin) 
    throw Ultracam::Read_Error("Error reading data type in void Ultracam::Windata::skip(std::ifstream&, bool)");
  if(swap_bytes) iout = Subs::byte_swap(iout);

  Out_type otype = Out_type(iout);
  if(otype == NORMAL){
    fin.ignore(sizeof(Ultracam::internal_data[t.ntot()]));
  }else if(otype == RAW){
    fin.ignore(sizeof(Ultracam::raw_data[t.ntot()]));
  }else{
    throw Ultracam_Error("Ultracam::Windata::skip(std::ifstream&, bool): unrecognised value of data type");
  }
  if(!fin) 
    throw Ultracam::Read_Error("Error skipping data in void Ultracam::Windata::skip(std::ifstream&, bool)");
}

/**
 * This function inputs a Ultracam::Windata in binary form, using the old format.
 * \param fin  input stream, opened for binary I/O
 * \exception Ultracam::Read_Error exceptions may be thrown for a number of reasons
 */
void Ultracam::Windata::read_old(std::ifstream& fin, bool swap_bytes) {

  Window t;
  t.read_old(fin, swap_bytes);
  if(!fin) 
    throw Ultracam::Read_Error("Error reading window in void Ultracam::Windata::read_old(std::ifstream&, bool)");

  if(nx() != t.nx() || ny() != t.ny())
    this->Ultracam::Array::resize(t.ny(), t.nx());
  this->Window::operator=(t);

  Subs::INT4 iout;
  fin.read((char*)&iout, sizeof(Subs::INT4));
  if(!fin) throw Ultracam::Read_Error("Error reading data type in void Ultracam::Windata::read(std::ifstream&, bool)");
  if(swap_bytes) iout = Subs::byte_swap(iout);
  Out_type otype = Out_type(iout);

  if(otype == NORMAL){

    // Read directly
    for(int iy=0; iy<ny(); iy++){
      fin.read((char*)ptr()[iy],sizeof(Ultracam::internal_data[nx()]));
      if(!fin) throw Ultracam::Read_Error("Error reading data in void Ultracam::Windata::read(std::ifstream&, bool)");
      if(swap_bytes) Subs::byte_swap(ptr()[iy],nx());
    }

  }else if(otype == RAW){

    // Read into a buffer
    Ultracam::raw_data *buff = new Ultracam::raw_data[ntot()];
    fin.read((char*)buff,sizeof(Ultracam::raw_data[ntot()]));
    if(!fin){
      delete[] buff;
      throw Ultracam::Read_Error("Error reading data in void Ultracam::Windata::read(std::ifstream& fin, bool)");
    }
    if(swap_bytes) Subs::byte_swap(buff,ntot());

    // Convert type
    int n=0;
    for(int iy=0; iy<ny(); iy++)
      for(int ix=0; ix<nx(); ix++)
	ptr()[iy][ix] = Ultracam::internal_data(buff[n++]);

    delete[] buff;
  }else{
    throw Ultracam_Error("Ultracam::Windata::read_old(std::ifstream&, bool): unrecognised value of data type");
  }
}

/**
 * This function skips a Ultracam::Windata in binary form while reading a file.
 * It copes automatically with 
 * the different types that can be output by void Ultracam::Windata::write(ofstream&, Out_type).
 * \param fin  input stream, opened for binary I/O
 * \exception Ultracam::Read_Error exceptions may be thrown for a number of reasons
 */
void Ultracam::Windata::skip_old(std::ifstream& fin, bool swap_bytes) {

  Window t;
  t.read_old(fin, swap_bytes);
  if(!fin) 
    throw Ultracam::Read_Error("Error reading data type in void Ultracam::Windata::skip_old(std::ifstream&, bool)");

  Subs::INT4 iout;
  fin.read((char*)&iout,sizeof(Subs::INT4));
  if(!fin) 
    throw Ultracam::Read_Error("Error reading data type in void Ultracam::Windata::skip_old(std::ifstream&, bool)");
  if(swap_bytes) iout = Subs::byte_swap(iout);

  Out_type otype = Out_type(iout);

  if(otype == NORMAL){
    fin.ignore(sizeof(Ultracam::internal_data[t.ntot()]));
  }else if(otype == RAW){
    fin.ignore(sizeof(Ultracam::raw_data[t.ntot()]));
  }else{
    throw Ultracam_Error("Ultracam::Windata::skip_old(std::ifstream&, bool): unrecognised value of data type");
  }
  if(!fin) 
    throw Ultracam::Read_Error("Error skipping data in void Ultracam::Windata::skip(std::ifstream&, bool)");
}

/** This function calculates a percentile of a Ultracam::Windata.
 * \param l the percentile to calculate, expressed as a fraction between 0 and 1
 * \param c the calculated percentile, returned.
 */
void Ultracam::Windata::centile(float l, Ultracam::internal_data& c) const {
  c = Ultracam::Array::centile(l);
}

/** This function windows a Ultracam::Windata to its joint overlap region with a Window.
 * The binning factors must be identical and the pixels in step and there must
 * be some overlap. Exceptions are thrown if any of these are not the case.
 * This routine is effectively what one would get by "looking through" the Window at the Ultracam::Windata.
 * \param win the Window to "look through"
 * \return The new windowed version is returned.
 * \exception Ultracam::Modify_Error exceptions are thrown if the modification cannot be made.
 */
Ultracam::Windata Ultracam::Windata::window(const Window& win) const {

  // check that the binning factors match up
  if(this->xbin() != win.xbin() || this->ybin() != win.ybin())
      throw Ultracam::Modify_Error("Windata::window(const Window&): incompatible binning factors");

  // .. and that the windows are in step.
  if((this->llx()-win.llx()) % win.xbin() != 0 || (this->lly()-win.lly()) % win.ybin() != 0)
    throw Ultracam::Modify_Error("Windata::window(const Window&): windows out  of step");
  
  // define new window from overlap between the current two under consideration
  int new_llx = std::max(this->llx(), win.llx());
  int new_lly = std::max(this->lly(), win.lly());
  int new_nx  = (std::min(this->llx()+this->xbin()*this->nx(), win.llx()+win.xbin()*win.nx())-new_llx)/this->xbin();
  int new_ny  = (std::min(this->lly()+this->ybin()*this->ny(), win.lly()+win.ybin()*win.ny())-new_lly)/this->ybin();

  if(new_nx < 1 || new_ny < 1)
    throw Ultracam::Modify_Error("Windata::window(const Window&): null overlap");

  // Create new Ultracam::Windata
  Ultracam::Windata temp(new_llx, new_lly, new_nx, new_ny, this->xbin(), this->ybin(), this->nxtot(), this->nytot());
 
  // Transfer data
  for(int iynew=0,iyold=(new_lly-this->lly())/this->ybin(); iynew<temp.ny(); iynew++, iyold++)
    for(int ixnew=0,ixold=(new_llx-this->llx())/this->xbin(); ixnew<temp.nx(); ixnew++, ixold++)
      temp[iynew][ixnew] = (*this)[iyold][ixold];

  return temp;
}

/** This routine creates, loads and returns a standard 1D pointer
 * containing the data of a Ultracam::Windata. It is up to the user to delete
 * the memory after using this pointer.
 */
Ultracam::internal_data* Ultracam::Windata::buffer() const {
  Ultracam::internal_data *ptr;
  ptr = new Ultracam::internal_data [ntot()];
  if(ptr) get(ptr);
  return ptr;
}

// ASCII output 

/** ASCII output is not normally what you want with a Ultracam::Windata, but is
 * provided as a last resort. It operates at three levels defined by
 * an integer: (1) minimal -- just prints the Window format. (2) Prints
 * the window format and some statistics of the data. (3) Prints the same
 * as level=2 but also prints out the pixel values, row by row.
 */ 
std::ostream& Ultracam::operator<<(std::ostream& s, const Ultracam::Windata& obj) {
  s << (Ultracam::Window&)obj << std::endl;
  
  if(Windata::print_level() > 1){
    internal_data p1, p2;
    obj.Ultracam::Array::centile(0.1f,0.9f,p1,p2);
    s << "Range: " << obj.min() << " to " << obj.max() << "; mean = " << obj.mean() << "; RMS = " << obj.rms()
      << "; 10 - 90 centiles = " << p1 << " to " << p2 << std::endl;
  }

  if(Windata::print_level() == 3){
    s << "   Now the data, row by row (starting from the bottom): " 
      << std::endl;
    for(int iy=0; iy<obj.ny(); iy++){
      s << "  ";
      for(int ix=0; ix<obj.nx(); ix++)
	s << " " << obj[iy][ix];
      if(iy<obj.ny()-1) s << std::endl;
    }
  }
  return s;
}

/** Provided as an alternative to the equivalent member function.
 * \param obj Ultracam::Windata object to take maximum of
 * \relates Ultracam::Windata
 */
Ultracam::internal_data max(const Ultracam::Windata& obj){
  return obj.max();
}

/** Provided as an alternative to the equivalent member function.
 * \param obj Ultracam::Windata object to take minimum of
 * \relates Ultracam::Windata
 */
Ultracam::internal_data min(const Ultracam::Windata& obj){
  return obj.min();
}

/** This function plots a Ultracam::Windata as a greyscale image
 * \param obj the Ultracam::Windata to plot
 * \param lo the lower inteneity level
 * \param hi the upper intensity level
 * \exception     Ultracam::Ultracam_Error exceptions are thrown if the routine fails
 * to allocate a data buffer needed to interface with PGPLOT.
 * \relates Ultracam::Windata
 */
void Ultracam::pggray(const Ultracam::Windata& obj, float lo, float hi){
  float tr[6];
  tr[0] = obj.llx()-(obj.xbin()+1)/2.;
  tr[1] = obj.xbin();
  tr[2] = 0.;
  tr[3] = obj.lly()-(obj.ybin()+1)/2.;
  tr[4] = 0.;
  tr[5] = obj.ybin();
  pggray(obj, lo, hi, tr);
}


