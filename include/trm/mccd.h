#ifndef TRM_MCCD_H
#define TRM_MCCD_H

#include <string>
#include "trm/subs.h"
#include "trm/ccd.h"
#include "trm/ultracam.h"

namespace Ultracam {

  template <class T>
  class MCCD;
  
  template <class T>
  std::ostream& operator<<(std::ostream& s, const MCCD<T>& obj);
  
  template <class T>
  std::istream& operator>>(std::istream& s, MCCD<T>& obj);  
  
  //! MCCD<Obj> groups CCD objects together.
  /**
   * MCCD<Obj> represents multiple CCD<Obj> objects as needed to 
   * represent multi-CCD data like ULTRACAM. This template is to handle the various
   * objects such as Windows, Apertures etc that are read from and written to ASCII
   * files. Data is handled through binary I/O and has a related class 'Mimage' 
   * rather than MCCD<Windata>, which allows it to access the individual CCDs as
   * Images rather than CCD<Windata> objects.
   */ 
  
  template <class Obj>
  class MCCD : public std::vector<CCD<Obj> > {
    
  public:
    
    //! Default constructor
    MCCD() : std::vector<CCD<Obj> >() {}
    
    //! Constructor of nccd CCDs
    MCCD(int nccd) : std::vector<CCD<Obj> >(nccd) {}
    
    //! Constructor from an ASCII file
    MCCD(const std::string& file) {rasc(file);}
    
    //! ASCII output
    friend std::ostream& operator<< <>(std::ostream& s, const MCCD& obj);
    
    //! ASCII input
    friend std::istream& operator>> <>(std::istream& s, MCCD& obj);  
    
    //! ASCII input from a named file
    void rasc(const std::string& file);
    
    //! ASCII output to a named file
    void wasc(const std::string& file) const;
    
    //! Destructor for inheritance
    virtual ~MCCD(){};
    
  };

  // ASCII I/O
  
  template <class Obj>
  std::ostream& operator<<(std::ostream& s, const MCCD<Obj>& mccd){
    s << "Number of CCDs = " << mccd.size() << std::endl;
    for(size_t ic=0; ic<mccd.size(); ic++){
      s << "\nCCD " << ic+1 << ": " << std::endl;
      s << mccd[ic];
    }
    return s;
  }
  
  template <class Obj>
  std::istream& operator>>(std::istream& s, MCCD<Obj>& mccd){
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
  
  template <class Obj>
  void Ultracam::MCCD<Obj>::rasc(const std::string& file){
    std::string nfile = Subs::filnam(file, Obj::extnam());
    std::ifstream fin(nfile.c_str());
    if(!fin) throw File_Open_Error("Failed to open \"" + nfile + "\" in void Ultracam::MCCD<Obj>::rasc(const std::string& file)");
    fin >> *this;
    if(!fin) throw Ultracam_Error("Failed to read \"" + nfile + "\" in void Ultracam::MCCD<Obj>::rasc(const std::string& file)");
    fin.close();
  }
  
  template <class Obj>
  void Ultracam::MCCD<Obj>::wasc(const std::string& file) const {
    std::string nfile = Subs::filnam(file, Obj::extnam());
    std::ofstream fout(nfile.c_str());
    if(!fout) throw File_Open_Error("Failed to open \"" + nfile + "\" in void Ultracam::MCCD<Obj>::wasc(const std::string& file)");
    fout << *this;
    fout.close();
  }

  //! Mimage represents multiple CCDs of data
  /**
   * Mimage is closely related to the template MCCD<Obj> which represents multiple CCD<Obj> 
   * objects. The difference is that it is specialised to Windata objects via the Image
   * class rather than the CCD<Windata> class. This allows it access to the extra functionality of
   * the Image class and that it deals with binary I/O.
   */ 
  class Mimage : public std::vector<Ultracam::Image> {
    
  public:
    
    //! Default constructor
    Mimage() : std::vector<Image>() {}
    
    //! Constructor of nccd CCDs
    Mimage(int nccd) : std::vector<Ultracam::Image>(nccd) {}
    
    //! Binary input
    void read(std::ifstream& fin, bool swap_bytes, int nc);
    
    //! Binary input from a named old-format file
    void read_old(std::ifstream& file, bool swap_bytes, int nc);
    
    //! Binary output
    void write(std::ofstream& fout, Windata::Out_type otype=Windata::NORMAL) const;
    
    //! Binary input from a named file
    //    void read(const string& file, int nc=0);
    
    //! Binary output to a named file
    void write(const std::string& file) const;
    
    //! Apply a step function transform to an Mimage
    void step(Ultracam::internal_data thresh);

    //! Destructor for inheritance
    virtual ~Mimage(){};
    
  };

};

#endif





















