#include <vector>
#include <string>

#include "trm/ccd.h"

using Ultracam::Ultracam_Error;
using Ultracam::Modify_Error;

// copy constructor

Ultracam::Image::Image(const CCD<Ultracam::Window>& win) : Ultracam::CCD<Ultracam::Windata>(win.size()) {
  for(size_t io=0; io<size(); io++)
    (*this)[io] = win[io];
}

// Definitions of member functions for specialisation

Ultracam::Image& Ultracam::Image::operator=(const Ultracam::internal_data& con){
  for(size_t io=0; io<this->size(); io++)
    (*this)[io] = con;
  return *this;
}

/**
 * This enables one to reformat a Ultracam::Image, but the pixel values are
 * left undefined.
 * \param win the Ultracam::CCD<Ultracam::Window> object to set the format to
 */
Ultracam::Image& Ultracam::Image::operator=(const Ultracam::CCD<Ultracam::Window>& win){
  if(this->size() != win.size()) this->resize(win.size());
  for(size_t io=0; io<this->size(); io++)
    (*this)[io] = win[io];
  return *this;
}

/**
 * Addition in place. The two Ultracam::Image objects must match in format.
 * \param obj the Ultracam::Image to add
 * \exception An Ultracam::Ultracam_Error is thrown if the number of
 * objects do not match
 */
void Ultracam::Image::operator+=(const Ultracam::Image& obj){
  if(this->size() == obj.size()){
    for(size_t io=0; io<this->size(); io++)
      (*this)[io] += obj[io];
  }else{
    throw Ultracam_Error("Conflicting numbers of objects in void Ultracam::Image::operator+=(const Ultracam::Image& obj)");
  }
}

/**
 * Subtraction in place. The two Ultracam::Image objects must match in format.
 * \param obj the Ultracam::Image to subtract
 * \exception An Ultracam::Ultracam_Error is thrown if the number of
 * objects do not match
 */
void Ultracam::Image::operator-=(const Ultracam::Image& obj){
  if(this->size() == obj.size()){
    for(size_t io=0; io<this->size(); io++)
      (*this)[io] -= obj[io];
  }else{
    throw Ultracam_Error("Conflicting numbers of objects in void Ultracam::Image::operator-=(const Ultracam::Image& obj)");
  }
}

/**
 * Multiplication in place. The two Ultracam::Image objects must match in format.
 * \param obj the Ultracam::Image to multiply by
  * \exception An Ultracam::Ultracam_Error is thrown if the number of
 * objects do not match
 */
void Ultracam::Image::operator*=(const Ultracam::Image& obj){
  if(this->size() == obj.size()){
    for(size_t io=0; io<this->size(); io++)
      (*this)[io] *= obj[io];
  }else{
    throw Ultracam_Error("Conflicting numbers of objects in void Ultracam::Image::operator*=(const Ultracam::Image& obj)");
  }
}

/**
 * Division in place. The two Ultracam::Image objects must match in format.
 * \param obj the Ultracam::Image to divide by
  * \exception An Ultracam::Ultracam_Error is thrown if the number of
 * objects do not match
 */
void Ultracam::Image::operator/=(const Ultracam::Image& obj){
  if(this->size() == obj.size()){
    for(size_t io=0; io<size(); io++)
      (*this)[io] /= obj[io];
  }else{
    throw Ultracam_Error("Conflicting numbers of objects in void Ultracam::Image::operator/=(const Ultracam::Image& obj)");
  }
}

void Ultracam::Image::operator+=(const Ultracam::internal_data& con){
  for(size_t io=0; io<this->size(); io++)
    (*this)[io] += con;
}

void Ultracam::Image::operator-=(const Ultracam::internal_data& con){
  for(size_t io=0; io<this->size(); io++)
    (*this)[io] -= con;
}

void Ultracam::Image::operator*=(const Ultracam::internal_data& con){
  for(size_t io=0; io<this->size(); io++)
    (*this)[io] *= con;
}

void Ultracam::Image::operator/=(const Ultracam::internal_data& con){
  for(size_t io=0; io<this->size(); io++)
    (*this)[io] /= con;
}

int Ultracam::Image::nxtot() const {
  if(this->size())
    return (*this)[0].nxtot();
  else
    return 0;
}

int Ultracam::Image::nytot() const {
  if(this->size())
    return (*this)[0].nytot();
  else
    return 0;
}

Ultracam::internal_data Ultracam::Image::min() const {
  if(this->size()){
    Ultracam::internal_data t = (*this)[0].min(), s;
    for(size_t io=1; io<this->size(); io++)
      if(t > (s = (*this)[io].min())) t = s;
    return t;
  }else{
    return 0.;
  }
}

Ultracam::internal_data Ultracam::Image::max() const {
  if(this->size()){
    Ultracam::internal_data t = (*this)[0].max(), s;
    for(size_t io=1; io<this->size(); io++)
      if(t < (s = (*this)[io].max())) t = s;
    return t;
  }else{
    return 0.;
  }
}

Ultracam::internal_data Ultracam::Image::mean() const {
  if(this->size()){
    double sum = 0;
    size_t np = 0;
    for(size_t nw=0; nw<this->size(); nw++){
      sum  += (*this)[nw].sum();
      np   += (*this)[nw].ntot();
    }
    if(np)
      return sum/np;
    else
      return 0.;
  }else{
    return 0.;
  }
}

/**
 * This version of \c centile computes a single percentile which can be useful in a number of
 * ways but especially for defining plot limits.
 * \param l the percentile level expressed as a number between 0 and 1
 * \param c the percentile calculated. Set to zero if the frame is blank.
 * \sa Ultracam::Image::centile(float, float, Ultracam::internal_data& , Ultracam::internal_data& )
 */
void Ultracam::Image::centile(float l, Ultracam::internal_data& c) const {
  if(this->size()){
    unsigned long int N = 0;
    for(size_t io=0; io<this->size(); io++)
      N += (unsigned long int)((*this)[io].ntot());

    // 'select' scrambles data so must copy first

    Ultracam::internal_data *p, *t;
    p = t = new Ultracam::internal_data [N];
    for(size_t io=0; io<this->size(); io++){
      (*this)[io].copy(t);
      t += (*this)[io].ntot();
    }

    unsigned long int k = (unsigned long int)(floor(N*l+0.5));

    c = Subs::select(p,N,k);
    delete[] p;
  }else{
    c = 0.;
  }
}

/**
 * This version of \c centile computes two percentiles. It is more efficient than calling the
 * single value version,  Ultracam::Image::centile(float, Ultracam::internal_data&),  twice. It is
 * most ueful for computing plot limits.
 * ways but especially for defining plot limits.
 * \param l1 the first percentile level expressed as a number between 0 and 1
 * \param l2 the second percentile level expressed as a number between 0 and 1
 * \param c1 the percentile corresponding to l1. Set to zero if the frame is blank.
 * \param c2 the percentile corresponding to l2. Set to zero if the frame is blank.
 * \sa Ultracam::Image::centile(float, Ultracam::internal_data&)
 */
void Ultracam::Image::centile(float l1, float l2, Ultracam::internal_data& c1, Ultracam::internal_data& c2) const {
  if(this->size()){
    unsigned long int N = 0;
    for(size_t io=0; io<size(); io++)
      N += (unsigned long int)((*this)[io].ntot());

    // 'select' scrambles data so must copy first

    Ultracam::internal_data *p, *t;
    p = t = new Ultracam::internal_data[N];
    if(!p)
      throw Ultracam::Ultracam_Error("void Ultracam::Image::centile(float, float, Ultracam::internal_data&, "
                     "Ultracam::internal_data&) const:"
                     " failed to allocate memory buffer for computation of centiles");
    for(size_t io=0; io<this->size(); io++){
      (*this)[io].copy(t);
      t += (*this)[io].ntot();
    }

    unsigned long int k1 = (unsigned long int)(floor(N*l1+0.5));
    unsigned long int k2 = (unsigned long int)(floor(N*l2+0.5));

    c1 = Subs::select(p,N,k1);
    c2 = Subs::select(p,N,k2);
    delete[] p;
  }else{
    c1 = c2 = 0.;
  }
}

/** Computes the maximum value over a sub-region of a Ultracam::CCD
 * \param window defines the region to compute the maximum value over
 * \return Returns the maximum. It will return zero if there are no
 * pixels in the region defined by window.
 */
Ultracam::internal_data Ultracam::Image::max(const Ultracam::CCD<Ultracam::Window>& window) const {
  Subs::Array1D<internal_data> buff;
  this->buffer(window, buff);
  return buff.max();
}

/** Computes the minimum value over a sub-region of a Ultracam::CCD
 * \param window defines the region to compute the minimum value over
 * \return Returns the minimum. It will return zero if there are no
 * pixels in the region defined by window.
 */
Ultracam::internal_data Ultracam::Image::min(const Ultracam::CCD<Ultracam::Window>& window) const {
  Subs::Array1D<internal_data> buff;
  this->buffer(window, buff);
  return buff.min();
}

void Ultracam::Image::centile(float l, Ultracam::internal_data& c, const Ultracam::CCD<Ultracam::Window>& window) const {
  Subs::Array1D<internal_data> buff;
  this->buffer(window, buff);
  if(buff.size()){
    int k = (int)(floor(buff.size()*l+0.5));
    c = buff.select(k);
  }else{
    c = 0;
  }
}

/**
 * This version of \c centile computes two percentiles. It is more efficient than calling the
 * single value version,  Ultracam::Image::centile(float, Ultracam::internal_data&),  twice. It is
 * most ueful for computing plot limits.
 * ways but especially for defining plot limits.
 * \param l1 the first percentile level expressed as a number between 0 and 1
 * \param l2 the second percentile level expressed as a number between 0 and 1
 * \param c1 the percentile corresponding to l1. Set to zero if the frame is blank.
 * \param c2 the percentile corresponding to l2. Set to zero if the frame is blank.
 * \param window the window over which to calculate the percentiles
 * \sa Ultracam::Image::centile(float, Ultracam::internal_data&)
 */
void Ultracam::Image::centile(float l1, float l2, Ultracam::internal_data& c1, Ultracam::internal_data& c2,
              const Ultracam::CCD<Ultracam::Window>& window) const {
  Subs::Array1D<internal_data> buff;
  this->buffer(window, buff);

  if(buff.size()){
    int k1 = (int)(floor(buff.size()*l1+0.5));
    int k2 = (int)(floor(buff.size()*l2+0.5));
    c1 = buff.select(k1);
    c2 = buff.select(k2);
  }else{
    c1 = c2 = 0;
  }
}


/**
 * crop re-formats a Ultracam::Image to match another. The original frame must
 * be a super-set of the one defining the new format. The pixel values are
 * transferred in this process. The routine also bins pixels if need be. The pixels of the new
 * format must have binnin factors which are multiples of the binning factors of the old format
 * and their positions must synchronise.
 * \param ccd the Ultracam::Image to crop to
 * \exception Ultracam::Modify_Error exceptions are thrown if the modification cannot be made.
 * \sa Ultracam::Image::crop(const Ultracam::CCD<Ultracam::Window>&)
 */

void Ultracam::Image::crop(const Ultracam::Image& ccd){

  // carry out the modification on a temporary object which
  // will be copied at the end
  Ultracam::Image temp = ccd;

  bool ok;

  // sref is a reference to the supposedly smaller window
  Ultracam::Windata* bref = NULL;

  size_t io;
  for(size_t iot=0; iot<temp.size(); iot++){

    Ultracam::Windata* sref = &temp[iot];

    // Look for a window entirely enclosing this one.
    ok = false;

    for(io=0; io<size(); io++){
      bref = &(*this)[io];
      if(bref->llx() <= sref->llx() &&
     bref->lly() <= sref->lly() &&
     bref->llx()+bref->xbin()*(bref->nx()-1) >= sref->llx()+sref->xbin()*(sref->nx()-1) &&
     bref->lly()+bref->ybin()*(bref->ny()-1) >= sref->lly()+sref->ybin()*(sref->ny()-1)){
    ok = true;
    break;
      }
    }

    // Checks for enclosure and compatibility
    std::string err;
    err = std::string("Ref window ") + Subs::str(iot+1);
    if(!ok) throw Modify_Error(err + "\nNo enclosing window in void Ultracam::CCD"
                      "<Ultracam::Windata>::crop(const Ultracam::Image& ccd)");

    err += std::string(", mod window ") + Subs::str(io+1);

    // check that the binning factors match up
    if(sref->xbin() % bref->xbin() != 0 || sref->ybin() % bref->ybin() != 0)
      throw Modify_Error(err + "\nIncompatible binning factors in void "
             "Ultracam::CCD <Ultracam::Windata>::crop(const Image& ccd). xbin: " +
             Subs::str(sref->xbin()) + ", " + Subs::str(bref->xbin()) );

    if((sref->llx()-bref->llx()) % bref->xbin() != 0 ||
       (sref->lly()-bref->lly()) % bref->ybin() != 0)
      throw Modify_Error(err + "\nUltracam::Windows out of step in void "
             "Ultracam::CCD <Ultracam::Windata>::crop(const Image& ccd).");

    if(sref->nxtot() != bref->nxtot() || sref->nytot() != bref->nytot())
      throw Modify_Error(err + "\nTotal dimensions clash in void"
             " Ultracam::Image::crop(const Ultracam::CCD<Ultracam::Window>& win)");

    // set up buffer for rebinning in the Y direction.
    double *buff = new double[sref->nx()];

    // Loop over Y in smaller window
    for(int iys=0; iys<sref->ny(); iys++){

      // initialise X buffer
      for(int ixs=0; ixs<sref->nx(); ixs++)
    buff[ixs] = 0.;

      // add up all contributing pixels from big window. This is written
      // to procede row by row up the big window without having to jump
      // around.
      int iyb1 = (sref->lly() + sref->ybin()*iys - bref->lly())/bref->ybin();
      int iyb2 = iyb1 + sref->ybin()/bref->ybin();

      for(int iyb=iyb1; iyb<iyb2; iyb++){
    for(int ixs=0; ixs<sref->nx(); ixs++){
      int ixb1 = (sref->llx() + sref->xbin()*ixs - bref->llx())/bref->xbin();
      int ixb2 = ixb1 + sref->xbin()/bref->xbin();
      for(int ixb=ixb1; ixb<ixb2; ixb++)
        buff[ixs] += (*bref)[iyb][ixb];

    }
      }

      // now transfer results
      for(int ixs=0; ixs<sref->nx(); ixs++)
    (*sref)[iys][ixs] = buff[ixs];
    }

    delete[] buff;
  }

  // Have got through all windows, copy to object
  *this = temp;
}

// Ultracam::Window an Ultracam frame to match a set of windows

/**
 * crop re-formats a Ultracam::Image to a format defined by a Ultracam::CCD<Ultracam::Window>. The original frame must
 * be a super-set of the one defining the new format. The pixel values are
 * transferred in this process. The routine also bins pixels if need be. The pixels of the new
 * format must have binning factors which are multiples of the binning factors of the old format
 * and their positions must synchronise.
 * \param win the Ultracam::CCD<Ultracam::Window> to crop to
 * \exception Ultracam::Modify_Error exceptions are thrown if the modification cannot be made.
 * \sa Ultracam::Image::crop(const Ultracam::Image&)
 */
void Ultracam::Image::crop(const Ultracam::CCD<Ultracam::Window>& win){

  // carry out the modification on a temporary object which
  // will be copied at the end.
  Ultracam::Image temp = win;

  bool ok;

  // sref is a reference to the supposedly smaller window
  // bref is a reference to the one that should enclose it
  Ultracam::Windata* bref = NULL;

  size_t io;
  for(size_t iot=0; iot<temp.size(); iot++){

    Ultracam::Windata* sref = &temp[iot];

    // Look for a window entirely enclosing this one.
    ok = false;
    for(io=0; io<size(); io++){
      bref = &(*this)[io];
      if(bref->llx() <= sref->llx() &&
     bref->lly() <= sref->lly() &&
     bref->llx()+bref->xbin()*(bref->nx()-1) >= sref->llx()+sref->xbin()*(sref->nx()-1) &&
     bref->lly()+bref->ybin()*(bref->ny()-1) >= sref->lly()+sref->ybin()*(sref->ny()-1)){
    ok = true;
    break;
      }
    }

    // Checks for enclosure and compatibility
    std::string err;
    err = std::string("Ref window ") + Subs::str(iot+1);
    if(!ok) throw Modify_Error(err +
                   "\nNo enclosing window in void Ultracam::CCD"
                   "<Ultracam::Windata>::crop(const CCD<Ultracam::Window>& win)");

    err += std::string(", mod window ") + Subs::str(io+1);

    // check that the binning factors match up
    if(sref->xbin() % bref->xbin() != 0 || sref->ybin() % bref->ybin() != 0)
      throw Modify_Error(err + "\nIncompatible binning factors in void "
             "CCD <Ultracam::Windata>::crop(const Ultracam::CCD<Ultracam::Window>& win). xbin: " +
             Subs::str(sref->xbin()) + ", " + Subs::str(bref->xbin()) );

    if((sref->llx()-bref->llx()) % bref->xbin() != 0 ||
       (sref->lly()-bref->lly()) % bref->ybin() != 0)
      throw Modify_Error(err + "\nUltracam::Windows out of step in void "
             "CCD <Ultracam::Windata>::crop(const CCD<Ultracam::Window>& win).");

    if(sref->nxtot() != bref->nxtot() || sref->nytot() != bref->nytot())
      throw Modify_Error(err + "\nTotal dimensions clash in void Ultracam::Image"
             "::crop(const Ultracam::CCD<Ultracam::Window>& win)");

    // set up buffer for rebinning in the Y direction.
    double *buff = new double[sref->nx()];

    // Loop over Y in smaller window
    for(int iys=0; iys<sref->ny(); iys++){

      // initialise X buffer
      for(int ixs=0; ixs<sref->nx(); ixs++)
    buff[ixs] = 0.;

      // add up all contributing pixels from big window. This is written
      // to procede row by row up the big window without having to jump
      // around.
      int iyb1 = (sref->lly() + sref->ybin()*iys - bref->lly())/bref->ybin();
      int iyb2 = iyb1 + sref->ybin()/bref->ybin();
      for(int iyb=iyb1; iyb<iyb2; iyb++){
    for(int ixs=0; ixs<sref->nx(); ixs++){
      int ixb1 = (sref->llx() + sref->xbin()*ixs - bref->llx())/bref->xbin();
      int ixb2 = ixb1 + sref->xbin()/bref->xbin();
      for(int ixb=ixb1; ixb<ixb2; ixb++)
        buff[ixs] += (*bref)[iyb][ixb];
    }
      }

      // now transfer results
      for(int ixs=0; ixs<sref->nx(); ixs++)
    (*sref)[iys][ixs] = buff[ixs];
    }

    delete[] buff;
  }

  // Have got through all windows, copy to object
  *this = temp;

}

// Ultracam::Window an Ultracam frame to match a set of windows

/**
 * window re-formats a Ultracam::Image to the union between its current format and
 * the format defined by a Ultracam::CCD<Ultracam::Window>. Is is effectively the result of viewing
 * the Ultracam::Image through the windows defined by the Ultracam::CCD<Ultracam::Window>. The binning
 * factors of the Ultracam::CCD<Ultracam::Window> must be divisors of the binning factors of the
 * Ultracam::Image and the pixels must synchronise. It is safest to make the window
 * binning factors = 1
 * \param win the Ultracam::CCD<Ultracam::Window> to crop to
 * \exception Ultracam::Modify_Error exceptions are thrown if the modification cannot be made.
 * \sa Ultracam::Image::crop(const Ultracam::Image&)
 */
void Ultracam::Image::window(const Ultracam::CCD<Ultracam::Window>& win){

  // Carry out the modification on a temporary object which
  // will be copied at the end.
  Ultracam::Image temp;

  // loop through each window of the data
  for(size_t iodat=0; iodat<this->size(); iodat++){

    const Ultracam::Windata &dwin = (*this)[iodat];

    // loop through each window of the windows
    for(size_t iowin=0; iowin<win.size(); iowin++){

      const Ultracam::Windata &wwin = win[iowin];

      // check for any overlap at all
      if(!overlap(dwin, wwin)) break;

      // create the overlap window
      temp.push_back(dwin.window(wwin));

    }
  }

  *this = temp;

}

/** Applies a step function transform to an Image to that every pixel
 * is converted to 0 or 1 according to whether it is <= or > the specified
 * threshold.
 * \param thresh the threshold to be applied
 */
void Ultracam::Image::step(Ultracam::internal_data thresh) {
    for(size_t nwin=0; nwin<this->size(); nwin++)
    (*this)[nwin].step(thresh);
}

// Unformatted I/O

/**
 * This function reads an Image stored in binary format. Checks
 * are made for the validity of the resulting Image.
 * \param fin input stream
 * \param swap_bytes whether or not to carry out byte swapping
 * \exception An Ultracam::Read_Error is thrown if the CCD is not valid.
 */
void Ultracam::Image::read(std::ifstream& fin, bool swap_bytes){
    int nobj;
    fin.read((char*)&nobj,sizeof(int));
    if(swap_bytes) nobj = Subs::byte_swap(nobj);
    if(nobj != int(this->size())) this->resize(nobj);

    for(int io=0; io<nobj; io++){
    (*this)[io].read(fin, swap_bytes);

    // Guard against inconsistent objects

    for(int ib=0; ib<io; ib++)
        if(clash((*this)[ib],(*this)[io]))
        throw Ultracam::Read_Error("Ultracam::Image::read(std::ifstream&): a " + Windata::name() + " clashed with one read earlier");
    }
}

/**
 * This function skips an Image stored in binary format.
 * \param fin input stream
 * \param swap_bytes whether or not to carry out byte swapping
 */
void Ultracam::Image::skip(std::ifstream& fin, bool swap_bytes){
    int nobj;
    fin.read((char*)&nobj,sizeof(int));
    if(swap_bytes) nobj = Subs::byte_swap(nobj);
    for(int io=0; io<nobj; io++)
    (*this)[io].skip(fin, swap_bytes);
}

/**
 * This function reads an Image stored in binary format. Checks
 * are made for the validity of the resulting CCD.
 * \param fin input stream
 * \param swap_bytes whether or not to carry out byte swapping
 * \exception An Ultracam::Read_Error is thrown if the CCD is not valid.
 */
void Ultracam::Image::read_old(std::ifstream& fin, bool swap_bytes){
    Subs::UINT4 nobj;
    fin.read((char*)&nobj,sizeof(Subs::UINT4));
    if(swap_bytes) nobj = Subs::byte_swap(nobj);
    if(nobj != this->size()) this->resize(nobj);
    for(size_t io=0; io<nobj; io++){
    (*this)[io].read_old(fin, swap_bytes);

    // Guard against inconsistent objects
    for(size_t ib=0; ib<io; ib++)
        if(clash((*this)[ib],(*this)[io]))
        throw Ultracam::Read_Error(std::string("Ultracam::Image::read_old(std::ifstream&): a ") + Windata::name() +
                       std::string(" clashed with one read earlier"));
    }
}

/**
 * This function skips a CCD stored in binary format.
 * \param fin input stream
 * \param swap_bytes whether or not to carry out byte swapping
 */
void Ultracam::Image::skip_old(std::ifstream& fin, bool swap_bytes){
    Subs::UINT4 nobj;
    fin.read((char*)&nobj,sizeof(Subs::UINT4));
    if(swap_bytes) nobj = Subs::byte_swap(nobj);
    for(Subs::UINT4 io=0; io<nobj; io++)
    (*this)[io].skip_old(fin,swap_bytes);
}

/**
 * write function which allows different output formats.
 * \param fout  the output stream
 * \param otype the output data type for storage on disk. The RAW type can save space
 * but may also be inaccurate. Best for storage of data direct from ULTRACAM.
 */
void Ultracam::Image::write(std::ofstream& fout, Ultracam::Windata::Out_type otype) const {
  Subs::INT4 nobj = Subs::INT4(this->size());
  fout.write((char*)&nobj,sizeof(Subs::INT4));
  for(int io=0; io<nobj; io++)
    (*this)[io].write(fout,otype);
}

/**
 * Computes statistics over a region defined by a set of windows. Any parts of these windows
 * outside the data windows are ignored. This allows one to compute statistics over arbitrary
 * sub-regions of a frame, as might be useful for computation of a bias offset for instance.
 * Raw stats are computed as some refined ones are a reject cycle are also computed.
 * \param statwin the region to compute the statitics over
 * \param sigma   RMS threshold for the reject loop
 * \param compute_median switch off to save a bit of time if you are not interested in the median value.
 * \param careful true/false for whether to clip the data by single pixel rejection, worst first, or by multiple rejection. The latter
 * is faster, but more likely to lead to incorrect rejections.
 * \return A Ultracam::Image::Stats object is returned holding the statistical information computed.
 * \exception Ultracam::Ultracam_Error exceptions may be thrown if a data buffer is not allocated.
 */
Ultracam::Image::Stats Ultracam::Image::statistics(const Ultracam::CCD<Ultracam::Window>& statwin, float sigma, bool compute_median, bool careful) const
{
  Subs::Array1D<internal_data> buff;
  this->buffer(statwin, buff);

  double rawmean,rawrms,mean,rms;
  int nrej;

  // use subs::sigma_reject for statistics data
  Subs::sigma_reject(buff.ptr(),buff.size(),sigma,careful,rawmean,rawrms,mean,rms,nrej);
  Stats stats;
  stats.max = buff.max();
  stats.min = buff.min();
  stats.raw_mean = rawmean;
  stats.raw_rms = rawrms;
  stats.npoints = buff.size();
  stats.clipped_mean = mean;
  stats.clipped_rms = rms;
  stats.nrejected = nrej;
  if(compute_median)
      stats.median = buff.median();
  return stats;
}

/** Returns a buffer containing data from a region defined by a set of windows
 * it is up to the user to deallocate the pointer returned in the structure Buff
 * if it is not NULL. This is for statistics and the pixels do not come back in any
 * particular order.
 * \param win  the region of the frame to return pixels values from.
 * \param buff the buffer returned
 */
void Ultracam::Image::buffer(const Ultracam::CCD<Ultracam::Window>& win, Subs::Array1D<Ultracam::internal_data>& buff) const {

  int xlo, xhi, ylo, yhi;
  const Ultracam::Windata *dwin;
  const Ultracam::Window *swin;
  size_t nbuff = 0;

  // First we need to compute the size of the data buffers we need to store the relevant pixels.

  // wind through each data window
  for(size_t ndwin=0; ndwin<size(); ndwin++){
    dwin = &(*this)[ndwin];

    // wind through each stats window
    for(size_t nswin=0; nswin<win.size(); nswin++){
      swin = &win[nswin];

      // Now determine overlap in terms of data pixels (the hi ones are one more than the max)
      xlo = std::max(0, int(ceil(float(swin->llx()-dwin->llx())/dwin->xbin())));
      xhi = std::min(dwin->nx(), (swin->llx()+swin->xbin()*swin->nx()-dwin->llx())/dwin->xbin());
      ylo = std::max(0, int(ceil(float(swin->lly()-dwin->lly())/dwin->ybin())));
      yhi = std::min(dwin->ny(), (swin->lly()+swin->ybin()*swin->ny()-dwin->lly())/dwin->ybin());

      if(xhi > xlo && yhi > ylo) nbuff += (xhi-xlo)*(yhi-ylo);
    }
  }

  buff.resize(nbuff);

  // wind through each data window
  size_t n = 0;
  for(size_t ndwin=0; ndwin<size(); ndwin++){
    dwin = &(*this)[ndwin];

    // wind through each stats window
    for(size_t nswin=0; nswin<win.size(); nswin++){
      swin = &win[nswin];

      // Now determine overlap in terms of data pixels (the hi ones are one more than the max)
      xlo = std::max(0, int(ceil(float(swin->llx()-dwin->llx())/dwin->xbin())));
      xhi = std::min(dwin->nx(), (swin->llx()+swin->xbin()*swin->nx()-dwin->llx())/dwin->xbin());
      ylo = std::max(0, int(ceil(float(swin->lly()-dwin->lly())/dwin->ybin())));
      yhi = std::min(dwin->ny(), (swin->lly()+swin->ybin()*swin->ny()-dwin->lly())/dwin->ybin());

      for(int iy=ylo; iy<yhi; iy++)
    for(int ix=xlo; ix<xhi; ix++)
      buff[n++] = (*dwin)[iy][ix];

    }
  }
}

/** This checks to see if any of the windows of a  Ultracam::CCD enclose a given position, and
 * if they do, it returns a reference to the relevant Ultracam::Windata, if not it throws an
 * exception.
 * \param x X ordinate of position in question
 * \param y Y ordinate of position in question
 * \exception Ultracam::Ultracam_Error exceptions are thrown if no window
 * encloses the point.
 */
Ultracam::Windata& Ultracam::Image::enclose(float x, float y) {

  for(size_t io=0; io<size(); io++)
    if((*this)[io].enclose(x,y))
      return (*this)[io];

  throw Ultracam::Ultracam_Error("Ultracam::Windata& Ultracam::Image::enclose(float x, float y) const:"
                 " position supplied = " + Subs::str(x) + ", " + Subs::str(y) + ", was not inside any window");
}

/** This checks to see if any of the windows of a  Ultracam::CCD enclose a given position, and
 * if they do, it returns a reference to the relevant Ultracam::Windata, if not it throws an
 * exception.
 * \param x X ordinate of position in question
 * \param y Y ordinate of position in question
 * \param which the index number of the enclosing window (starts at 0)
 * \exception Ultracam::Ultracam_Error exceptions are thrown if no window
 * encloses the point.
 */
Ultracam::Windata& Ultracam::Image::enclose(float x, float y, int& which) {

  for(size_t io=0; io<size(); io++)
    if((*this)[io].enclose(x,y)){
      which = io;
      return (*this)[io];
    }

  throw Ultracam::Ultracam_Error("Ultracam::Windata& Ultracam::Image::enclose(float x, float y, int&) const:"
                 " position supplied = " + Subs::str(x) + ", " + Subs::str(y) + ", was not inside any window");

}

/** This checks to see if any of the windows of a  Ultracam::CCD enclose a given position, and
 * if they do, it returns a reference to the relevant Ultracam::Windata, if not it throws an
 * exception.
 * \param x X ordinate of position in question
 * \param y Y ordinate of position in question
 * \exception Ultracam::Ultracam_Error exceptions are thrown if no window
 * encloses the point.
 */
const Ultracam::Windata& Ultracam::Image::enclose(float x, float y) const {

  for(size_t io=0; io<size(); io++)
    if((*this)[io].enclose(x,y))
      return (*this)[io];

  throw Ultracam::Ultracam_Error("const Ultracam::Windata& Ultracam::Image::enclose(float x, float y) const:"
                 " position supplied = " + Subs::str(x) + ", " + Subs::str(y) + ", was not inside any window");

}

/** This checks to see if any of the windows of a  Ultracam::CCD enclose a given position, and
 * if they do, it returns a reference to the relevant Ultracam::Windata, if not it throws an
 * exception.
 * \param x X ordinate of position in question
 * \param y Y ordinate of position in question
 * \param which the index number of the enclosing window (starts at 0)
 * \exception Ultracam::Ultracam_Error exceptions are thrown if no window
 * encloses the point.
 */
const Ultracam::Windata& Ultracam::Image::enclose(float x, float y, int& which) const {

  for(size_t io=0; io<size(); io++)
    if((*this)[io].enclose(x,y)){
      which = io;
      return (*this)[io];
    }

  throw Ultracam::Ultracam_Error("const Ultracam::Windata& Ultracam::Image::enclose(float x, float y, int&) const:"
                 " position supplied = " + Subs::str(x) + ", " + Subs::str(y) + ", was not inside any window");

}

// Non-member functions

/** Computes the minimum pixel value of all the Ultracam::Windata objects in the supplied
 * Ultracam::Image
 * \param obj the Ultracam::Image to take the minimum of.
 * \relates Ultracam::Image
 */
Ultracam::internal_data Ultracam::min(const Ultracam::Image& obj) {
  return obj.min();
}

/** Computes the maximum pixel value of all the Ultracam::Windata objects in the supplied
 * Ultracam::Image
 * \param obj the Ultracam::Image to take the maximum of.
 * \relates Ultracam::Image
 */
Ultracam::internal_data Ultracam::max(const Ultracam::Image& obj) {
  return obj.max();
}

/** Computes the minimum pixel value of all the Ultracam::Windata objects in the supplied
 * Ultracam::Image over a region.
 * \param obj the Ultracam::Image to take the minimum of.
 * \param window the region to compute the maximum over
 * \relates Ultracam::Image
 */
Ultracam::internal_data Ultracam::min(const Ultracam::Image& obj, const Ultracam::CCD<Ultracam::Window>& window) {
  return obj.min(window);
}

/** Computes the maximum pixel value of all the Ultracam::Windata objects in the supplied
 * Ultracam::Image over a region.
 * \param obj    the Ultracam::Image to take the maximum of.
 * \param window the region to compute the maximum over
 * \relates Ultracam::Image
 */
Ultracam::internal_data Ultracam::max(const Ultracam::Image& obj, const Ultracam::CCD<Ultracam::Window>& window) {
  return obj.max(window);
}

/** 'Equality' is defined as the two Ultracam::Image objects having the same
 * format for all of their separate Ultracam::Windata objects. Note it is assumed that
 * they match in storage order too, so two objects with identical windows
 * could fail this test if the windows were stored in a different order.
 *
 * \param ccd1 the first Ultracam::Image object
 * \param ccd2 the second Ultracam::Image object
 * \return \c true if the formats match
 * \relates Ultracam::Image
 */
bool Ultracam::operator==(const Ultracam::Image& ccd1,
              const Ultracam::Image& ccd2){
  if(ccd1.size() == ccd2.size()){
    for(size_t nobj=0; nobj<ccd1.size(); nobj++)
      if(ccd1[nobj] != ccd2[nobj]) return false;
    return true;
  }else{
    return false;
  }
}

/** 'Inequality' is defined as the two Ultracam::Image objects having a differnt
 * format for any of their separate Ultracam::Windata objects.
 *
 * \param ccd1 the first Ultracam::Image object
 * \param ccd2 the second Ultracam::Image object
 * \return \c true if the formats do no match
 * \relates Ultracam::Image
 */
bool Ultracam::operator!=(const Ultracam::Image& ccd1,
              const Ultracam::Image& ccd2){

  if(ccd1.size() == ccd2.size()){
    for(size_t nobj=0; nobj<ccd1.size(); nobj++)
      if(ccd1[nobj] != ccd2[nobj]) return true;
    return false;
  }else{
    return true;
  }
}

/**
 * Plots a Ultracam::Image as a greyscale image, with each window
 * bordered by a line.
 */
void Ultracam::pggray(const Ultracam::Image& ccd, float lo, float hi){
  for(size_t io=0; io<ccd.size(); io++)
    pggray(ccd[io],lo,hi);
}

/**
 * Labels the windows of a Ultracam::Image plot.
 */
void Ultracam::pgptxt(const Ultracam::Image& ccd){
  std::string label;
  for(size_t io=0; io<ccd.size(); io++){
    label = Subs::str(io+1);
    pgptxt(ccd[io],label);
  }
}
