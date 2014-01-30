#include "trm/frame.h"
#include "trm/header.h"
#include "trm/fdisk.h"

/** Constructor of an Fdisk which opens a disk file, reads the start and positions an internal
 * pointer just before the start of the data
 *
 * \param file  file containing the data
 * \param nbuff the buffer size to allocate to this Fdisk.
 * \param wccd  which CCD to access. 0 for all, > 0 for a specific CCD, others skipped.
 */
Ultracam::Fdisk::Fdisk(const std::string& file, int nbuff, int wccd) : nbuff_(nbuff), wccd_(wccd) {

  fin.open(Subs::filnam(file,Frame::extnam()).c_str(), std::ios::binary);
  if(!fin)
    throw Ultracam::File_Open_Error(std::string("Ultracam::Fdisk::Fdisk(const std::string&, int, int): failed to open ") +
                    Subs::filnam(file,Frame::extnam()));

  // Allocate buffer
  buff = new Ultracam::internal_data [nbuff];
  if(buff == NULL)
    throw Ultracam::File_Open_Error("Ultracam::Fdisk::Fdisk(const std::string&, int, int): failed to allocate read buffer");

  // Read and test magic number which is supposed to indicate that this is a ucm file. I don't attempt
  // backwards compatibility here because I reckon that it is easy to recreate the ucm files for the routines
  // that use 'Ultracam::Fdisk'
  int magic;
  fin.read((char*)&magic,sizeof(int));
  if(!fin)
    throw Ultracam::Ultracam_Error("Frame::read(std::string&, int): failed to read ucm magic number");

  if(magic != Ultracam::MAGIC)
    throw Ultracam::Ultracam_Error("Frame::read(std::string&, int): did not recognise file = " + file + " as a ucm file");

  // Check for non-native data
  swap_bytes = (Subs::byte_swap(magic) == Ultracam::MAGIC);

  // Check here also allows for swapping
  bool old = !swap_bytes && (magic != Ultracam::MAGIC);

  // If 'old' then no magic number and we should wind back to start
  if(old) fin.seekg(0);

  // Skip the header
  Subs::Header::skip(fin, swap_bytes);

  // Read number of CCDs
  if(!fin.read((char*)&num_ccd,sizeof(int)))
    throw Ultracam::Ultracam_Error("Ultracam::Fdisk::Fdisk(const std::string&, int, int): failed to read number of ccds");
  if(swap_bytes) num_ccd = Subs::byte_swap(num_ccd);

  if(wccd_ && wccd_ > num_ccd)
    throw Ultracam::Ultracam_Error("Ultracam::Fdisk::Fdisk(const std::string&, int, int): failed to read number of ccds");

  // Now get to start of the CCD requested
  if(wccd_ > 1){

    int nsofar = 1;
    while(nsofar < wccd_){

      // Read number of Windows of CCD
      if(!fin.read((char*)&num_win,sizeof(int)))
    throw Ultracam::Ultracam_Error("Ultracam::Fdisk::Fdisk(const std::string&, int, int): failed to read number of windows of ccd");
      if(swap_bytes) num_win = Subs::byte_swap(num_win);

      for(int nw=0; nw<num_win; nw++){

    // Read window formats and data type
    window.read(fin, swap_bytes);
    if(!fin) throw Ultracam::Ultracam_Error("Ultracam::Fdisk::Fdisk(const std::string&, int, int): failed to read window");
    fin.read((char*)&out_type_,sizeof(Windata::Out_type));
    if(!fin)
      throw Ultracam::Read_Error("Ultracam::Fdisk::Fdisk(const std::string&, int, int): error reading data type");
    if(swap_bytes) out_type_ = Windata::Out_type(Subs::byte_swap(out_type_));

    // skip data
    if(out_type_ == Windata::NORMAL){
      if(!fin.ignore(sizeof(Ultracam::internal_data[window.ntot()])))
        throw Ultracam::Read_Error("Ultracam::Fdisk::Fdisk(const std::string&, int, int): error skipping data");

    }else if(out_type_ == Windata::RAW){
      if(!fin.ignore(sizeof(Ultracam::raw_data[window.ntot()])))
        throw Ultracam::Read_Error("Ultracam::Fdisk::Fdisk(const std::string&, int, int): error skipping data");
    }
      }
      nsofar++;
    }
  }

  // Read number of Windows of first CCD
  if(!fin.read((char*)&num_win,sizeof(int)))
    throw Ultracam::Ultracam_Error("Ultracam::Fdisk::Fdisk(const std::string&, int, int): failed to read number of windows of ccd");
  if(swap_bytes) num_win = Subs::byte_swap(num_win);

  // Read first window
  window.read(fin, swap_bytes);
  if(!fin) throw Ultracam::Ultracam_Error("Ultracam::Fdisk::Fdisk(const std::string&, int, int): failed to read first window");

  fin.read((char*)&out_type_,sizeof(Windata::Out_type));
  if(!fin)
    throw Ultracam::Read_Error("Ultracam::Fdisk::Fdisk(const std::string&, int, int): error reading data type");
  if(swap_bytes) out_type_ = Windata::Out_type(Subs::byte_swap(out_type_));

  // Read in some data
  int nread = std::min(nbuff_, window.ntot());

  if(out_type_ == Windata::NORMAL){

    // Read directly
    if(!fin.read((char*)buff,sizeof(Ultracam::internal_data[nread])))
      throw Ultracam::Read_Error("Ultracam::Fdisk::Fdisk(const std::string&, int, int): error reading data");

    if(swap_bytes) Subs::byte_swap(buff, nread);

  }else if(out_type_ == Windata::RAW){

    // Read into a buffer
    Ultracam::raw_data *rbuff = new Ultracam::raw_data[nread];
    fin.read((char*)rbuff,sizeof(Ultracam::raw_data[nread]));
    if(!fin){
      delete[] rbuff;
      throw Ultracam::Read_Error("Ultracam::Fdisk::Fdisk(const std::string&, int, int): error reading data into raw buffer");
    }
    if(swap_bytes) Subs::byte_swap(rbuff, nread);

    // Convert type
    for(int i=0; i<nread; i++)
    buff[i] = Ultracam::internal_data(rbuff[i]);
    delete[] rbuff;
  }

  // All done, now just define parameters
  nccd_  =  wccd_ - 1;   // CCD
  nwin_  =  0;           // first window of first CCD
  ptr    = -1;           // just before the first pixel
  nx_    = -1;           // just before the first pixel
  ny_    =  0;           // first row
  nread_ = nread;

}

Ultracam::Fdisk::~Fdisk() {
  delete[] buff;
  fin.close();
}

Ultracam::internal_data Ultracam::Fdisk::get_next() {
  nx_++;
  bool newwin = false;
  if(nx_ == window.nx()){
    nx_ = 0;
    ny_++;
    if(ny_ == window.ny()){
      ny_ = 0;
      nwin_++;
      nread_ = 0;
      if(nwin_ == num_win){
    nwin_ = 0;
    nccd_++;
    if((wccd_ && nccd_ == wccd_) || nccd_ == num_ccd)
      throw Ultracam::Ultracam_Error("Ultracam::internal_data Ultracam::Fdisk::get_next(): error trying to access beyond end of data");

    // Read number of Windows of next CCD
    if(!fin.read((char*)&num_win,sizeof(int)))
      throw Ultracam::Ultracam_Error("Ultracam::internal_data Ultracam::Fdisk::get_next(): failed to read number of windows of next ccd");
    if(swap_bytes) num_win = Subs::byte_swap(num_win);

      }

      // Read window
      window.read(fin, swap_bytes);
      if(!fin) throw Ultracam::Ultracam_Error("Ultracam::internal_data Ultracam::Fdisk::get_next(): failed to read window");

      newwin = true;
    }
  }

  // update pointer
  ptr++;

  if(newwin || ptr == nbuff_){

    if(newwin)
      if(!fin.read((char*)&out_type_,sizeof(Windata::Out_type)))
    throw Ultracam::Read_Error("Ultracam::internal_data Ultracam::Fdisk::get_next(): error reading data type");
    if(swap_bytes) out_type_ = Windata::Out_type(Subs::byte_swap(out_type_));

    // Read in some data, taking care not to read too far
    int nread = std::min(nbuff_, window.ntot()-nread_);
    nread_ += nread;

    if(out_type_ == Windata::NORMAL){

      // Read directly
      if(!fin.read((char*)buff,sizeof(Ultracam::internal_data[nread])))
    throw Ultracam::Read_Error("Ultracam::internal_data Ultracam::Fdisk::get_next(): error reading data");
      if(swap_bytes) Subs::byte_swap(buff, nread);

    }else if(out_type_ == Windata::RAW){

      // Read into a buffer
      Ultracam::raw_data *rbuff = new Ultracam::raw_data[nread];
      fin.read((char*)rbuff,sizeof(Ultracam::raw_data[nread]));
      if(!fin){
    delete[] rbuff;
    throw Ultracam::Read_Error("Ultracam::internal_data Ultracam::Fdisk::get_next(): error reading data into raw buffer");
      }

      if(swap_bytes) Subs::byte_swap(rbuff, nread);

      // Convert type
      for(int i=0; i<nread; i++)
    buff[i] = Ultracam::internal_data(rbuff[i]);
      delete[] rbuff;
    }
    ptr = 0;
  }
  return buff[ptr];
}

