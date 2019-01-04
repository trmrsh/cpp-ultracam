#include <cmath>
#include <string>
#include "trm/frame.h"
#include "trm/mccd.h"
#include "trm/subs.h"
#include "trm/ultracam.h"

Ultracam::Frame::Frame(const std::string& file, int nc){
    read(file,nc);
}

Ultracam::Frame::Frame(const Ultracam::Mwindow& mwin) : Mimage(mwin.size()), Subs::Header() {
  for(size_t ic=0; ic<size(); ic++)
    (*this)[ic] = mwin[ic];
}

Ultracam::Frame::Frame(const Ultracam::Mwindow& mwin, const Subs::Header& head) : Mimage(mwin.size()), Subs::Header(head) {
  for(size_t ic=0; ic<size(); ic++) (*this)[ic] = mwin[ic];
}

Ultracam::Frame& Ultracam::Frame::operator=(const Ultracam::internal_data& con){
  for(size_t ic=0; ic<size(); ic++)
    (*this)[ic] = con;
  return *this;
}

/**
 * This allows one to change the window and CCD format and headers of a frame.
 *  \param mwin the new windows format.
 *  \param head the new headers
 */
void Ultracam::Frame::format(const Ultracam::Mwindow& mwin, const Subs::Header& head){
  this->Subs::Header::operator=(head);
  this->Mimage::clear();
  this->Mimage::resize(mwin.size());
  for(size_t ic=0; ic<size(); ic++)
    (*this)[ic] = mwin[ic];
}

/**
 * This allows one to set the window and CCD format and headers of a frame to that
 * of another without copying any data.
 *  \param frame the frame defining the new headers and format. No data is copied.
 */
void Ultracam::Frame::format(const Ultracam::Frame& frame){
  this->Subs::Header::operator=(frame);
  this->Mimage::clear();
  this->Mimage::resize(frame.size());
  for(size_t ic=0; ic<size(); ic++){
    (*this)[ic].resize(frame[ic].size());
    for(size_t io=0; io<(*this)[ic].size(); io++){
      (*this)[ic][io] = (Window&)frame[ic][io];
    }
  }
}

// Read files

/**
 * This function reads in an ULTRACAM file from disk. \sa Ultracam::Frame::Frame(const string&, int)
 * \param file the file name to read
 * \param nc the CCD number to read, 0 for all of them.
 */
void Ultracam::Frame::read(const std::string& file, int nc){

  std::ifstream fin(Subs::filnam(file,Ultracam::Frame::extnam()).c_str(), std::ios::binary);

  if(!fin)
    throw File_Open_Error("Ultracam::Frame::read(std::string&, int): failed to open \"" + Subs::filnam(file,extnam()));

  // Read and test magic number which is supposed to indicate that this is a ucm file. This
  // was introduced only in Sept 2004 so there are backwards compatibility issues to deal with
  // too as the format changed slightly at the same time.
  Subs::INT4 magic;
  fin.read((char*)&magic,sizeof(Subs::INT4));
  if(!fin)
    throw Ultracam_Error("Ultracam::Frame::read(std::string&, int): failed to read ucm magic number");

  // Check for non-native data
  bool swap_bytes = (Subs::byte_swap(magic) == Ultracam::MAGIC);

  // Check here also allows for swapping
  bool old = !swap_bytes && (magic != Ultracam::MAGIC);

  // If it is old, and we are on a bigendian machine, then
  // we will have to swap bytes (because all old files were written on little-endian machines)
  if(old && Subs::is_big_endian()) swap_bytes = true;

  // If 'old' then no magic number and we should wind back to start
  if(old) fin.seekg(0);

  // Read header as usual
  Subs::Header::read(fin, swap_bytes);

  // backwards compatible stuff
  if(old){
    Mimage::read_old(fin,swap_bytes,nc);
  }else{
    Mimage::read(fin,swap_bytes,nc);
  }
  fin.close();
}

// Write files

/**
 * This function writes an ULTRACAM file to disk. It will over-write any existing files,
 * so be careful.
 * \param file  name of file to write.
 * \param otype data type to store on disk (RAW form can save on disk space but may lose precision)
 */

void Ultracam::Frame::write(const std::string& file,  Windata::Out_type otype) const {
  std::ofstream fout(Subs::filnam(file,Ultracam::Frame::extnam()).c_str(), std::ios::binary);

  if(!fout)
    throw File_Open_Error(std::string("Failed to open \"") +
              Subs::filnam(file,extnam()) +
              std::string("\" in void Ultracam::Frame::write(const std::string&)"));

  // Write Ultracam magic number to identify this as a ucm file.
  // 29/09/2004
  fout.write((char*)&Ultracam::MAGIC, sizeof(Subs::INT4));

  // Write header then data
  Subs::Header::write(fout);
  if(otype == Windata::NORMAL){
    Mimage::write(fout);
  }else{
    Subs::INT4 nccd = Subs::INT4(this->size());
    fout.write((char*)&nccd, sizeof(Subs::INT4));
    for(int ic=0; ic<nccd; ic++)
      (*this)[ic].write(fout,otype);
  }
  fout.close();
}

// addition etc in place

/**
 *  This carries out addition of a constant 'in place'. For instance
 * data += 10; would add 10 to all pixels of the Ultracam::Frame 'data'.
 * No other form of addition is supported because of efficiency considerations.
 */
void Ultracam::Frame::operator+=(const Ultracam::internal_data& constant){
  for(size_t ic=0; ic<size(); ic++)
    (*this)[ic] += constant;
}

/**
 * This carries out subtraction of a constant 'in place'. For instance
 * data -= 10; would subtract 10 off all pixels of the Ultracam::Frame 'data'.
 * No other form of subtraction is supported because of efficiency considerations.
 */
void Ultracam::Frame::operator-=(const Ultracam::internal_data& constant){
  for(size_t ic=0; ic<size(); ic++)
    (*this)[ic] -= constant;
}

/**
 *
 * This carries out multiplication by a constant 'in place'. For instance
 * data *= 10; would multiply all pixels of the Ultracam::Frame 'data' by 10.
 * No other form of multiplication is supported because of efficiency considerations.
 */
void Ultracam::Frame::operator*=(const Ultracam::internal_data& constant){
  for(size_t ic=0; ic<size(); ic++)
    (*this)[ic] *= constant;
}

/**
 * This carries out division by a constant 'in place'. For instance
 * data /= 10; would divide all pixels of the Ultracam::Frame 'data' by 10.
 * No other form of division is supported because of efficiency considerations.
 */
void Ultracam::Frame::operator/=(const Ultracam::internal_data& constant){
  for(size_t ic=0; ic<size(); ic++)
    (*this)[ic] /= constant;
}

/**
 * This carries out addition of one frame to another, overwriting the
 * frame on the left of the expression. The headers of the overwritten frame
 * are preserved.
 */
void Ultracam::Frame::operator+=(const Ultracam::Frame& obj){
  if(*this == obj){
    for(size_t ic=0; ic<size(); ic++)
    (*this)[ic] += obj[ic];
  }else{
    throw Ultracam_Error("Incompatible objects in void Ultracam::Frame::operator+=(const Ultracam::Frame&)");
  }
}

/**
 * This carries out subtraction of one frame from another, overwriting the
 * frame on the left of the expression. The headers of the overwritten frame
 * are preserved.
 */
void Ultracam::Frame::operator-=(const Ultracam::Frame& obj){
  if((*this) == obj){
    for(size_t ic=0; ic<size(); ic++)
    (*this)[ic] -= obj[ic];
  }else{
    throw Ultracam_Error("Incompatible objects in void Ultracam::Frame::operator-=(const Ultracam::Frame&)");
  }
}

/**
 * This carries out multiplication of two frames, overwriting the
 * frame on the left of the expression. The headers of the overwritten frame
 * are preserved.
 */
void Ultracam::Frame::operator*=(const Ultracam::Frame& obj){
  if((*this) == obj){
    for(size_t ic=0; ic<size(); ic++)
    (*this)[ic] *= obj[ic];
  }else{
    throw Ultracam_Error("Incompatible objects in void Ultracam::Frame::operator*=(const Ultracam::Frame&)");
  }
}

/**
 * This carries out division of two frames, overwriting the
 * frame on the left of the expression. The headers of the overwritten frame
 * are preserved.
 */
void Ultracam::Frame::operator/=(const Ultracam::Frame& obj){
  if((*this) == obj){
    for(size_t ic=0; ic<size(); ic++)
    (*this)[ic] /= obj[ic];
  }else{
    throw Ultracam_Error("Incompatible objects in void Ultracam::Frame::operator/=(const Ultracam::Frame&)");
  }
}

/**
 * This limits the values of a frame to be at the minimum the value specified.
 * \param low the lowest value allowed.
 */

void Ultracam::Frame::max(const Ultracam::internal_data& low){
  for(size_t ic=0; ic<size(); ic++)
    for(size_t io=0; io<(*this)[ic].size(); io++)
      for(int iy=0; iy<(*this)[ic][io].ny(); iy++)
    for(int ix=0; ix<(*this)[ic][io].nx(); ix++)
      if((*this)[ic][io][iy][ix] < low) (*this)[ic][io][iy][ix] = low;
}

/**
 * This crops the format of one frame to match another 'template' frame. It only
 * does so if the template is enclosed by the object frame, and if the two frames
 * match in total CCD size and binning.
 * \param obj the template frame to crop down to.
 */
void Ultracam::Frame::crop(const Ultracam::Frame& obj){
  if(this->size() == obj.size()){
    for(size_t ic=0; ic<size(); ic++)
      (*this)[ic].crop(obj[ic]);
  }else{
      throw Ultracam_Error("Incompatible objects in void Ultracam::Frame::crop(const Ultracam::Frame&): "
               + Subs::str(this->size()) + " vs " + Subs::str(obj.size()));
  }
}

/**
 * This crops the format of one frame to match a Ultracam::Mwindow multi-window object. It only
 * does so if the Ultracam::Mwindow format is enclosed by the object frame, and if the
 * total CCD size and binning match.
 */
void Ultracam::Frame::crop(const Ultracam::Mwindow& obj){
    if(this->size() == obj.size()){
    for(size_t ic=0; ic<size(); ic++)
        (*this)[ic].crop(obj[ic]);
    }else{
    throw Ultracam_Error("Incompatible objects in void Ultracam::Frame::crop(const Ultracam::Mwindow&): "
                 + Subs::str(this->size()) + " vs " + Subs::str(obj.size()));
    }
}

/** This changes a frame to be the overlap between it a Ultracam::Mwindow multi-window object.
 * Effectively it is the result of looking through the Ultracam::Mwindow at the Ultracam::Frame. It differs
 * from cropping in that the Windows need not be fully enclosed by the data windows.
 * \param obj the Ultracam::Mwindow multi-windows
 */
void Ultracam::Frame::window(const Ultracam::Mwindow& obj){
    if(this->size() == obj.size()){
    for(size_t ic=0; ic<size(); ic++)
        (*this)[ic].window(obj[ic]);
    }else{
    throw Ultracam_Error("Incompatible objects in void Ultracam::Frame::window(const Ultracam::Mwindow&): "
                 + Subs::str(this->size()) + " vs " + Subs::str(obj.size()));
    }
}

/**
 * This returns the maximum total X dimension of all the CCDs in the frame, as is
 * sometimes useful for setting plot limits.
 */
int Ultracam::Frame::nxtot() const {
  int n = 0;
  for(size_t ic=0; ic<size(); ic++)
    n = n >= (*this)[ic].nxtot() ? n : (*this)[ic].nxtot();
  return n;
}

/**
 * This returns the maximum total Y dimension of all the CCDs in the frame, as is
 * sometime useful for setting plot limits.
 */
int Ultracam::Frame::nytot() const {
  int n = 0;
  for(size_t ic=0; ic<size(); ic++)
    n = n >= (*this)[ic].nytot() ? n : (*this)[ic].nytot();
  return n;
}

std::ostream& Ultracam::operator<<(std::ostream& ostr, const Ultracam::Frame& frame) {
  ostr << "Header:\n" << std::endl;
  ostr << (const Subs::Header&)frame << "\n" << std::endl;
  ostr << "Data:\n" << std::endl;
  ostr << (const Ultracam::MCCD<Ultracam::Windata>&)frame << std::endl;
  return ostr;
}

void Ultracam::Frame::operator-=(const FCmul& obj) {
  if((*this) == obj.frame){
    for(size_t ic=0; ic<size(); ic++){
      for(size_t iw=0; iw<(*this)[ic].size(); iw++){
    for(int iy=0; iy<(*this)[ic][iw].ny(); iy++){
      for(int ix=0; ix<(*this)[ic][iw].nx(); ix++){
        (*this)[ic][iw][iy][ix] -= obj.con*obj.frame[ic][iw][iy][ix];
      }
    }
      }
    }
  }else{
    throw Ultracam_Error("Incompatible objects in void Ultracam::Frame::operator-=(const FCmul&)");
  }
}

/**
 * This function returns true/false as to whether a file is an ULTRACAM file or not,
 * based only upon the file extension, so easily fooled.
 */
bool Ultracam::Frame::is_ultracam(const std::string& name){
  // test if a file is an ultracam file or not ...
  // based only upon its extension, so easily fooled.
  std::ifstream ftest(Subs::filnam(name, Ultracam::Frame::extnam()).c_str());
  return (ftest.good());
}



