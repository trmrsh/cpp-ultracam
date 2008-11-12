/*

!!begin
!!title   Lists headers of Ultracam frames
!!author  T.R. Marsh
!!created 17 May 2002
!!created 15 May 2006
!!descr   Lists headers of Ultracam frames
!!css     style.css
!!root    hedlis
!!index   hedlis
!!class   Programs
!!class   Information
!!head1   hedlis - lists headers of Ultracam frames

!!head2 Invocation

hedlis list

!!head2 Arguments

!!table

!!arg{list}{List of ultracam files}

!!table

!!end

*/

#include <cstdlib>
#include <string>
#include <map>
#include <fstream>
#include <cerrno>
#include <cfloat>
#include "trm_subs.h"
#include "trm_input.h"
#include "trm_header.h"
#include "trm_ultracam.h"

int main(int argc, char* argv[]){

  using Ultracam::Input_Error;

  try{

    // Construct Input object

    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // sign-in input variables

    input.sign_in("list",    Subs::Input::LOCAL, Subs::Input::PROMPT);

    // Get input

    std::string list_name;
    input.get_value("list", list_name, "list", "name of list of ultracam files");

    // Read file names
    std::string name;
    std::vector<std::string> flist;
    std::ifstream istr(list_name.c_str());
    while(istr >> name){
      flist.push_back(name);
    }
    istr.close();
    size_t nfile = flist.size();
    if(nfile == 0) throw Input_Error("No file names loaded");

    // Read the headers from the start of the ULTRACAM files
    Subs::Header header;

    for(size_t nf=0; nf<nfile; nf++){
      std::cout << "\nFile = " << flist[nf] << ":\n" << std::endl;
      std::ifstream istr(flist[nf].c_str(), std::ios::binary);

      // Read and test magic number which is supposed to indicate that this is a ucm file. This
      // was introduced only in Sept 2004 so there are backwards compatibility issues to deal with
      // too as the format changed slightly at the same time.
      int magic;
      istr.read((char*)&magic,sizeof(int));
      if(!istr)
	throw Ultracam::Ultracam_Error("Dailed to read ucm magic number");

      // Check for non-native data
      bool swap_bytes = (Subs::byte_swap(magic) == Ultracam::MAGIC);

      // Check here also allows for swapping
      bool old = !swap_bytes && (magic != Ultracam::MAGIC);

      // If it is old, and we are on a bigendian machine, then
      // we will have to swap bytes 
      if(old && Subs::is_big_endian()) swap_bytes = true;
      
      // If 'old' then no magic number and we should wind back to start
      if(old) istr.seekg(0);

      // Finally read the header
      header.read(istr, swap_bytes);
      std::cout << header;
      istr.close();
    }
  }

  catch(const Ultracam::Input_Error& err){
    std::cerr << "Ultracam::Input_Error exception:" << std::endl;
    std::cerr << err << std::endl;
  }
  catch(const Ultracam::Ultracam_Error& err){
    std::cerr << "Ultracam::Ultracam_Error exception:" << std::endl;
    std::cerr << err << std::endl;
  }
  catch(const Subs::Subs_Error& err){
    std::cerr << "Subs::Subs_Error exception:" << std::endl;
    std::cerr << err << std::endl;
  }
  catch(const std::string& err){
    std::cerr << err << std::endl;
  }
}



