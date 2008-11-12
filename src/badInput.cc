#include <string>
#include <map>
#include "trm_ultracam.h"

/** Routine to test that a given variable has a value stored in the structure reduce, and that it is 
 * not blank. The structure reduce is constructed from reading in the reduce data file and consists
 * of a series of name, value strings.
 * \param reduce map of value strings keyed on variable name strings
 * \param name   variable name to search for
 * \param p      pointer to the value if found.
 * \return true if non-blank value is found.
 */
bool Ultracam::badInput(const std::map<std::string,std::string>& reduce, const std::string& name, std::map<std::string,std::string>::const_iterator& p){
  return ((p = reduce.find(name)) == reduce.end() || p->second == "");
}
