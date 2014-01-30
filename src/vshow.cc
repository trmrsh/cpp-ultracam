/*

!!begin
!!title  Prints out variables in default files
!!author T.R. Marsh
!!created  10 Dec 2003
!!root   vshow
!!index  vshow
!!descr  prints out variables in default files
!!css   style.css
!!class  Programs
!!class  Information
!!head1  vshow - prints out variables in default files

The pipeline software commands store command defaults in
small files. These are located either in the directory ~/.ultracam
or in wherever ULTRACAM_ENV points, if it is defined. The files
are of the form <command>.def where command is the command
name. Also there is a file called GLOBAL.def which stores
those defaults accessed by multiple commands. !!emph{vshow}
just prints out these files. This is very similar to the FIGARO
equivalent. An additional feature of these files is that they
store the name of the last command to have accessed each variable.

!!head2 Invocation

vshow file

!!head2 Command line arguments

!!table
!!arg{file}{Name of default file, '.def' extension assumed, and the default location will be added unless
you specify a directory path.}
!!table

!!end

*/

#include <cstdlib>
#include <string>
#include <sstream>
#include <map>
#include "trm/subs.h"
#include "trm/input.h"
#include "trm/header.h"
#include "trm/ultracam.h"

int main(int argc, char* argv[]){

  try{

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Define inputs
    input.sign_in("file",     Subs::Input::LOCAL, Subs::Input::PROMPT);

    std::string file;
    input.get_value("file", file, "GLOBAL", "default file name");

    // work out directory for default files
    char *cp = getenv(Ultracam::ULTRACAM_ENV);
    std::string defaults_dir;
    if(cp == NULL){
      char *home = getenv("HOME");
      if(home == NULL) throw Ultracam::Ultracam_Error("Can't identify home directory to locate defaults directory");
      defaults_dir = std::string(home) + std::string("/") + std::string(Ultracam::ULTRACAM_DIR);

    }else{
      defaults_dir = std::string(cp);

    }

    std::string def_file;
    if(def_file.find("/") == std::string::npos)
      def_file = Subs::filnam(defaults_dir + std::string("/") + file, ".def");
    else
      def_file = Subs::filnam(file, ".def");

    Subs::Header head;
    std::ifstream istr(def_file.c_str(), std::ios::binary);
    if(istr){
      try{
    head.read(istr, false);
      }
      catch(...){
    throw Ultracam::Ultracam_Error("Error occurred while trying to read default file = " + def_file);
      }
    }else{
      throw Ultracam::Ultracam_Error("Error occurred trying to open default file = " + def_file);
    }
    istr.close();

    std::cout << "Default file = " << def_file << std::endl;
    std::cout << "Listing of default values stored:\n\n" << std::endl;
    std::cout << head << std::endl;
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

