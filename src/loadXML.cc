#include <cstdlib>
#include <iostream>
#include <fstream>
#include "trm/ultracam.h"

/** Function to load XML data directly as opposed to doing so via a server.
 * \param name  name of the XML file
 * \param buff  structure containing a pointer to the data. You should always 'free' (not 'delete') the pointer
 * between calls to loadXML and when you have no further use for it. This is for compatibilty with the cURL software.
 * The use of this buffer instead of a plain pointer is also for compatibility with cURL inside parseXML
 */
void Ultracam::loadXML(const std::string& name, MemoryStruct& buff){
  std::ifstream fin(name.c_str(), std::ios::binary);
  if(!fin) throw File_Open_Error("void Ultracam::loadXML(const std::string&, MemoryStruct&): failed to open " + name);

  // Find out the size of the file
  fin.seekg(0, std::ios::end);
  if(!fin)
    throw Ultracam::Ultracam_Error("void Ultracam::loadXML(const std::string&, MemoryStruct&): "
                   " could not move to the end of the file");

  std::streamsize nbytes = fin.tellg();
  if(!fin)
    throw Ultracam::Ultracam_Error("void Ultracam::loadXML(const std::string&, MemoryStruct&): "
                   " could not determine position of 'get' pointer");

  // Allocate memory, move back to start
  buff.memory = (char*)malloc(nbytes);
  if(buff.memory == NULL)
    throw Ultracam::Ultracam_Error("void Ultracam::loadXML(const std::string&, MemoryStruct&): "
                   " failed to allocate data buffer");

  fin.seekg(0, std::ios::beg);
  if(!fin)
    throw Ultracam::Ultracam_Error("void Ultracam::loadXML(const std::string&, MemoryStruct&): "
                   " could not move to the start of the file");
  fin.read(buff.memory, nbytes);
  if(!fin)
    throw Ultracam::Ultracam_Error("void Ultracam::loadXML(const std::string&, MemoryStruct&): "
                   " failed to read data.");
  buff.posn = buff.size = nbytes;
  fin.close();
}
