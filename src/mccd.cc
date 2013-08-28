#include "trm/mccd.h"

/**
 * Binary input from  a file with the option of reading only 1 CCD
 * \param fin input stream
 * \param swap_bytes whether to swap bytes or not
 * \param nc number of CCD to read, 0 for them all
 */
void Ultracam::Mimage::read(std::ifstream& fin, bool swap_bytes, int nc){
    
    Subs::INT4 nccd;
    if(!fin.read((char*)&nccd,sizeof(Subs::INT4)))
      throw Ultracam_Error("Ultracam::Mimage::read(std::ifstream&, bool, int): failed to read nccd");
    if(swap_bytes) nccd = Subs::byte_swap(nccd);
    if(nccd != Subs::INT4(this->size())) this->resize(nccd);

    if(nc == 0){
      
	// read every thing
	for(int ic=0; ic<nccd; ic++)
	    (*this)[ic].read(fin, swap_bytes);
	
    }else if(nc <= nccd){
	
	// read specific object of interest
	for(int ic=0; ic<nc-1; ic++)
	    (*this)[ic].skip(fin, swap_bytes);
	
	(*this)[nc-1].read(fin, swap_bytes);
	
	for(int ic=nc; ic<nccd; ic++)
	    (*this)[ic].skip(fin, swap_bytes);
	
    }else{
	throw Ultracam_Error("nc = " + Subs::str(nc) + " too large. Max value = " + Subs::str(nccd) + " in void Ultracam::Mimage::read(std::ifstream&, bool, int)");
    }
}
  
/**
 * Binary input from  a file with the option of reading only 1 CCD
 * \param fin input stream
 * \param swap_bytes whether to swap bytes or not
 * \param nc number of CCD to read, 0 for them all
 */
void Ultracam::Mimage::read_old(std::ifstream& fin, bool swap_bytes, int nc){
    
    Subs::INT4 nccd;
    if(!fin.read((char*)&nccd, sizeof(Subs::INT4)))
	throw Ultracam_Error("Ultracam::Mimage::read_old(std::ifstream&, bool, int): failed to read nccd");
    if(swap_bytes) nccd = Subs::byte_swap(nccd);    
    if(nccd != Subs::INT4(this->size())) this->resize(nccd);
    
    if(nc == 0){
	
      // read every thing
	for(Subs::INT4 ic=0; ic<nccd; ic++)
	    (*this)[ic].read_old(fin, swap_bytes);
	
    }else if(nc <= int(nccd)){
	
	// read specific object of interest
	
	for(int ic=0; ic<nc-1; ic++)
	    (*this)[ic].skip_old(fin, swap_bytes);
	
	(*this)[nc-1].read_old(fin, swap_bytes);
	
	for(Subs::INT4 ic=nc; ic<nccd; ic++)
	    (*this)[ic].skip_old(fin, swap_bytes);
	
    }else{
	throw Ultracam_Error("Mimage::read_old(std::ifstream&, bool, int): nc = " + Subs::str(nc) + " too large. Max value = " + Subs::str(nccd) );
    }
}

void Ultracam::Mimage::write(std::ofstream& fout, Windata::Out_type otype) const {
    Subs::INT4 nccd = Subs::INT4(this->size());
    fout.write((char*)&nccd,sizeof(Subs::INT4));
    for(Subs::INT4 ic=0; ic<nccd; ic++)
	(*this)[ic].write(fout);
}

void Ultracam::Mimage::write(const std::string& file) const {
    std::ofstream fout(file.c_str(), std::ios::out | std::ios::binary);
    if(!fout) throw Ultracam_Error("Failed to open \"" + file + "\" in void Ultracam::MCCD<>::write(const std::string& file)");
    write(fout);
    fout.close();
}

// ASCII I/O

std::ostream& operator<<(std::ostream& s, const Ultracam::Mimage& mccd){
    s << "Number of CCDs = " << mccd.size() << std::endl;
    for(size_t ic=0; ic<mccd.size(); ic++){
	s << "\nCCD " << ic+1 << ": " << std::endl;
	s << mccd[ic];
    }
    return s;
}

std::istream& operator>>(std::istream& s, Ultracam::Mimage& mccd){
    char ch;
    
    int nccd;
    while(s.get(ch) && ch != '=');
    if(!s || !(s >> nccd)) 
	return s;
    
    if(nccd != int(mccd.size())) mccd.resize(nccd);
    
    for(int ic=0; ic<nccd; ic++){
	while(s.get(ch) && ch != ':');
	if(!s || !(s >> mccd[ic]))
	    return s;
    }
    return s;
}

/** Applies a step function transform to an Mimage to that every pixel
 * is converted to 0 or 1 according to whether it is <= or > the specified
 * threshold.
 * \param thresh the threshold to be applied
 */
void Ultracam::Mimage::step(Ultracam::internal_data thresh) {
    for(size_t nwin=0; nwin<this->size(); nwin++)
	(*this)[nwin].step(thresh);
}





















