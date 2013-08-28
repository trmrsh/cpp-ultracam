/*

!!begin
!!title   Edits the headers of Ultracam frames
!!author  T.R. Marsh
!!created 22 August 2002
!!revised 06 July 2007
!!descr   Edits the headers of Ultracam frames
!!css   style.css
!!root    hedit
!!index   hedit
!!class   Programs
!!class   Testing
!!head1   hedit - edits the headers of ULTRACAM frames

This allows you to edit the !!emph{values} of header items already present. The input of the value
is via a string which is then translated into the particular item type. Expect a rather unintelligible
error message if you specify an incorrect format.

!!head2 Invocation

hedit data item value

!!head2 Arguments

!!table

!!arg{data}{Name of an ultracam file.}
!!arg{item}{Name of item to change. This should be the full path of the item.}
!!arg{value}{Value of item.}

!!table

!!end

*/

#include <cstdlib>
#include <string>
#include <map>
#include <fstream>
#include <cerrno>
#include <cfloat>
#include "trm/subs.h"
#include "trm/input.h"
#include "trm/header.h"
#include "trm/frame.h"
#include "trm/ultracam.h"

int main(int argc, char* argv[]){

    using Ultracam::Input_Error;
    
    try{
	
	// Construct Input object
	
	Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);
	
	// sign-in input variables
	
	input.sign_in("data",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("item",     Subs::Input::LOCAL, Subs::Input::PROMPT);
	input.sign_in("value",    Subs::Input::LOCAL, Subs::Input::PROMPT);
	
	// Get input	
	std::string sdata;
	input.get_value("data",  sdata, "dname", "name of ULTRACAM file to edit");
	std::string item;
	input.get_value("item",   item, "item", "name of header item");
	
	Ultracam::Frame data(sdata);
	Subs::Header::Hnode *hnode = data.find(item);
	if(!hnode->has_data())
	    throw Ultracam::Input_Error(std::string("Item = ") + item + std::string(" not found."));
	if(hnode->value->is_a_dir()) 
	    throw Ultracam::Ultracam_Error(std::string("Item = ") + item + std::string(" is a directory."));
	
	std::string value = hnode->value->get_string();
	input.set_default("value", value);
	input.get_value("value", value, "value", "value of header item");
	hnode->value->set_value(value);
	
	// Write out result
	data.write(sdata);
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



