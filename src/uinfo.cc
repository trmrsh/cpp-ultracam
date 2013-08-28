/*

!!begin
!!title  Prints out information on an Ultracam frame
!!author T.R. Marsh
!!created  14 May 2001
!!revised  15 June 2001
!!root   uinfo
!!index  uinfo
!!descr  prints information on an Ultracam frame
!!css   style.css
!!class  Programs
!!class  Information
!!head1  uinfo - prints information on an Ultracam frame

!!emph{uinfo} prints information on an Ultracam file. 

!!head2 Invocation

uinfo data level

!!head2 Command line arguments

!!table
!!arg{data}{Ultracam data file}
!!arg{level}{level=1: basic info; level=2: basic info plus some stats;
level=3: basic info, stats plus full print out of data.}
!!table

!!end

*/

#include <cstdlib>
#include <string>
#include <sstream>
#include <map>
#include "trm/subs.h"
#include "trm/input.h"
#include "trm/frame.h"
#include "trm/ultracam.h"

int main(int argc, char* argv[]){

    try{
	
	// Construct Input object
	Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);
	
	// Define inputs
	input.sign_in("data",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("level",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
	
	std::string sdata;
	input.get_value("data", sdata, "run001", "data file");
	
	Ultracam::Frame data(sdata);
	int level;
	input.get_value("level", level, 2, 1, 3, "information level");
	Ultracam::Windata::set_print_level(level);
	
	std::cout << std::endl;
	std::cout << "Name of file = " << sdata << "\n" << std::endl;
	std::cout << data;
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

